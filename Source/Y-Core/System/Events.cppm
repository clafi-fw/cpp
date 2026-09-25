export module ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace ClaFi
{

    using EventTypeId = std::size_t;
    using EventConnectionId = std::size_t;

    // One byte per event type, never read. Its address is the type's identity:
    // unique across types, stable for the process, and needing no registry.
    template<typename T>
    struct EventTypeIdGenerator
    {
        static EventTypeId id()
        {
            static const char identifier{};
            return std::bit_cast<EventTypeId>(&identifier);
        }
    };

    export struct Event
    {
    public:
        virtual ~Event() = default;
        void stopPropagation() { m_propatationStopped = true; }
        bool propagationStopped() const { return m_propatationStopped; }
    private:
        bool m_propatationStopped{};
    };

    export template<typename Sender>
    class EventOf : public Event
    {
    public:
        EventOf(Sender& sender) : m_sender{ sender } {}
        Sender& sender() { return m_sender; }
        const Sender& sender() const { return m_sender; }
    private:
        Sender& m_sender;
    };

    export class EventDispatcher;

    export class ScopedEventConnection;

    export class EventConnection
    {
        friend ScopedEventConnection;
        friend EventDispatcher;
    public:
        EventConnection(EventTypeId, EventConnectionId, EventDispatcher*);
        EventConnection(EventConnection&& other) noexcept;
        // Every constructor registers with the dispatcher and this removes the registration, so
        // that a dispatcher destroyed first can null the pointer held here. See the destructor of
        // EventDispatcher for what that is worth.
        ~EventConnection();
    public:
        EventTypeId eventTypeId() const { return m_eventTypeId; };
        EventConnectionId connectionId() const { return m_connectionId; }
        void disconnect() const;
    private:
        EventConnection(const EventConnection& other);
        EventConnection& operator=(const EventConnection& other);
        // Forgets the dispatcher without disconnecting from it - what a connection handed over to
        // somebody else leaves behind. Registering is not something a caller can skip, so this is
        // the one way out of the list that is not a destructor.
        void release();
    private:
        EventTypeId m_eventTypeId;
        EventConnectionId m_connectionId;
        EventDispatcher* m_dispatcher;
    };

    export class ScopedEventConnection
    {
    public:
        ScopedEventConnection() : ScopedEventConnection{ {0, 0, nullptr} } {}
        // R-Values only - that intentionally!
        ScopedEventConnection(const EventConnection&& source) : m_source{ source } {}
        ScopedEventConnection(const ScopedEventConnection&) = delete;
        ScopedEventConnection& operator=(const ScopedEventConnection&) = delete;
        ScopedEventConnection(ScopedEventConnection&& other) noexcept : m_source{ other.m_source }
            { other.m_source.release(); }
        ScopedEventConnection& operator=(ScopedEventConnection&& other) noexcept
        {
            if (this != &other)
            {
                m_source.disconnect();
                m_source = other.m_source;
                other.m_source.release();
            }
            return *this;
        }
        ~ScopedEventConnection() { m_source.disconnect(); };
    private:
        EventConnection m_source;
    };

    export template<typename T>
    concept IsEvent = std::derived_from<T, Event>;

    template<typename T>
    concept IsMethod = std::is_member_function_pointer_v<T>;

    // The event a handler names in its own parameter list.
    //
    // Called EventParamOf and not EventOf on purpose: EventOf, just above, is the
    // base class template for events themselves. Two templates of one name in one
    // namespace, one of them also reachable through a using-directive, produces
    // diagnostics that are hard to read.
    //
    // The primary is defined and empty rather than left undeclared, so that
    // NamesAnEvent below can be asked about an arbitrary type. An undefined
    // primary would make EventParamOf's base class incomplete, and that is a
    // hard error rather than a substitution failure. A caller that constrains a
    // whole constructor pack on NamesAnEvent needs the SFINAE-friendly version.
    template <typename T> struct EventParamOfImpl {};
    template <typename C, typename R, typename A> struct EventParamOfImpl<R(C::*)(A&)>                { using type = std::remove_const_t<A>; };
    template <typename C, typename R, typename A> struct EventParamOfImpl<R(C::*)(A&) const>          { using type = std::remove_const_t<A>; };
    template <typename C, typename R, typename A> struct EventParamOfImpl<R(C::*)(A&) noexcept>       { using type = std::remove_const_t<A>; };
    template <typename C, typename R, typename A> struct EventParamOfImpl<R(C::*)(A&) const noexcept> { using type = std::remove_const_t<A>; };
    template <typename R, typename A>             struct EventParamOfImpl<R(*)(A&)>                   { using type = std::remove_const_t<A>; };

    // Only the primary is exported. A partial specialization is not an
    // exportable declaration, and it does not need to be: it is reachable from
    // this module wherever the primary is used.
    export template <typename F, typename = void>
        struct EventParamOf : EventParamOfImpl<std::decay_t<F>> {};          // function pointer
    template <typename F>
    struct EventParamOf<F, std::void_t<decltype(&std::decay_t<F>::operator())>>
        : EventParamOfImpl<decltype(&std::decay_t<F>::operator())> {};       // lambda or functor

    export template <typename F>
        using event_param_t = typename EventParamOf<F>::type;

    // Satisfied when the handler names exactly one parameter and that parameter
    // is an Event. Everything else - generic lambdas, wrong arity, overloaded
    // operator() - fails here rather than inside EventDispatcher.
    export template <typename F>
    concept NamesAnEvent = requires { typename EventParamOf<F>::type; }
                        && IsEvent<typename EventParamOf<F>::type>;

    // A tag base rather than a template-id match, so that a caller can keep a
    // thin named subclass for compatibility and still have it recognised as a
    // handler by the routing below.
    export struct OnEventTag {};

    export template <typename T>
    concept IsOnEvent = std::is_base_of_v<OnEventTag, std::decay_t<T>>;

    // Anything a constructor pack accepts as a handler: a bare callable that names its event, or
    // one already wrapped in OnEvent - which covers the named aliases and makeOnEvent's result.
    export template <typename T>
    concept IsHandler = IsOnEvent<T> || NamesAnEvent<T>;

    // Guards a greedy variadic constructor against swallowing its own copy constructor: a
    // single argument of the class's own type is a copy, not a property pack of one.
    export template <typename Self, typename... Args>
    concept NotSelfCopy = !(sizeof...(Args) == 1
                            && (std::is_same_v<std::decay_t<Args>, Self> && ...));

    export class EventDispatcher
    {
        friend EventConnection;

    public:
        template<IsEvent TEvent>
        using Callback = std::function<void(TEvent&)>;

    public:
        EventDispatcher() = default;
        // WHAT KEEPS A CONNECTION FROM OUTLIVING ITS EMITTER. An EventConnection holds a raw
        // pointer here, and a ScopedEventConnection disconnects through it when it goes. Two
        // controls in one container have no order between them - a std::vector destroys its
        // elements front to back under libstdc++ and back to front under libc++, and the standard
        // states neither - so which of an emitter and its listener dies first is not something a
        // caller can arrange. Every live connection is told here instead, and finds a null pointer
        // rather than a freed one.
        ~EventDispatcher();
        // The tracked connections are NOT carried. A connection names the dispatcher it was made
        // on; copying the listeners does not move the connections that reach them.
        EventDispatcher(const EventDispatcher&);
        EventDispatcher& operator=(const EventDispatcher&);

    public:
        template<IsEvent TEvent>
        EventConnection connect(auto&& callback);

        template<IsEvent TEvent, typename TObject, IsMethod Method>
        EventConnection connect(TObject* object, Method method);

        void disconnect(const EventConnection& conn);

        template<IsEvent TEvent, typename... Args>
        void emit(Args&&... args) const;

        template<IsEvent TEvent>
        void emit(TEvent& event) const;

        // Whether anything is connected to this channel. A disconnect leaves its now-empty list
        // behind under the key, so the list is what answers, not the key.
        template<IsEvent TEvent>
        [[nodiscard]] bool hasListeners() const;

    private:
        using InternalEntry = std::pair<EventConnectionId, std::function<void(Event&)>>;

    private:
        void track(EventConnection&);
        void untrack(const EventConnection&);
    private:
        std::size_t m_nextConnectionId{ 0 };
        std::map<EventTypeId, std::vector<InternalEntry>> m_listeners;
        // The connection objects that name this dispatcher, non-owning. A connection puts itself
        // here for its lifetime; the destructor above is the only reader.
        std::vector<EventConnection*> m_connections;
    };

    export class EventComponent
        // - must not inherit from anything.
        // - can be used as a base class,
        // - can be used for injecting events support through multi-inheritance
    {
    public:
        inline EventDispatcher& events() noexcept { return m_events; };
        inline const EventDispatcher& events() const { return m_events; };

        template<IsEvent TEvent, typename... Args>
        inline void emitEvent(Args&&... args) const noexcept {
            m_events.emit<TEvent>(std::forward<Args>(args)...);
        }
        template<IsEvent TEvent>
        inline void emitEvent(TEvent& event) const noexcept {
            m_events.emit<TEvent>(event);
        }
        // Whether a handler is connected for this event. Wanted rarely and for one shape of
        // thing: a control deciding whether it has an answer of its own to defend against
        // whatever would otherwise answer for it - see Control::getControlState.
        template<IsEvent TEvent>
        [[nodiscard]] inline bool hasEventListeners() const noexcept {
            return m_events.hasListeners<TEvent>();
        }
        // Naming the event explicitly. Kept for the call sites that spell it, and for a handler
        // that cannot name it - a generic lambda, or one taking a base of the event.
        template<IsEvent TEvent>
        EventConnection connectEvent(auto&& callback) noexcept {
            return m_events.connect<TEvent>(std::forward<decltype(callback)>(callback));
        }
        template<IsEvent TEvent, typename TObject, IsMethod Method>
        EventConnection connectEvent(TObject* object, Method method) noexcept {
            return m_events.connect<TEvent, TObject, Method>(object, method);
        }
        // Reading the event off the handler's own parameter, so it is written once:
        //
        //     control.connectEvent([](ClickEvent& event) { ... });
        //     control.connectEvent(this, &Page::colorModeClicked);
        //
        // The guard is what keeps these apart from the two above. With TEvent given explicitly,
        // the first template parameter here binds to that event type, and an event is neither a
        // handler nor an object, so this overload drops out and only the explicit one remains.
        template<typename F>
            requires (!IsEvent<std::decay_t<F>>)
        EventConnection connectEvent(F&& callback) noexcept {
            static_assert(NamesAnEvent<F>,
                "connectEvent: the handler must take exactly one parameter and it must be an "
                "Event. A generic [](auto&) handler names no event - write the event type in "
                "the parameter, or pass the event type explicitly as connectEvent<TEvent>(...).");
            return m_events.connect<event_param_t<F>>(std::forward<F>(callback));
        }
        template<typename TObject, IsMethod Method>
            requires (!IsEvent<TObject>)
        EventConnection connectEvent(TObject* object, Method method) noexcept {
            static_assert(NamesAnEvent<Method>,
                "connectEvent: the method must take exactly one parameter and it must be an "
                "Event. Pass the event type explicitly as connectEvent<TEvent>(...) if it "
                "cannot.");
            return m_events.connect<event_param_t<Method>, TObject, Method>(object, method);
        }
    protected:
        // Constructors are protected for the reason the destructor is: EventComponent is only
        // ever a base.
        EventComponent() = default;

        // Connects every OnEvent in a constructor pack, whatever events they name, and ignores
        // everything else. A derived type forwards its pack here and is done - there is no
        // per-event binding to write, and nothing to remember to call.
        //
        // sizeof...(Args) > 0 keeps this away from the default constructor. NotSelfCopy keeps it
        // away from the copy constructor of the derived type, whose pack arrives here verbatim.
        template <typename... Args>
            requires (sizeof...(Args) > 0 && NotSelfCopy<EventComponent, Args...>)
        explicit EventComponent(const Args&... args) noexcept
        {
            auto connectOne = [&](const auto& arg) {
                if constexpr (IsOnEvent<decltype(arg)>)
                    arg.connectTo(*this);
            };
            (connectOne(args), ...);
        }
        // protected ~EventComponent prevents accidental slicing
        // EventComponent* base = new Button();
        // delete base; // bad
        ~EventComponent() = default;
    private:
        EventDispatcher m_events;
    };

    // =========================================================================
    // Handler properties
    // =========================================================================

    // Deduces the event from the handler's own parameter, so one property type
    // connects any event on any emitter:
    //
    //     OnEvent{ [](ComboboxChangeEvent& event) { ... } }
    //     OnEvent{ [this](ChangeEvent& event) { ... } }
    //     makeOnEvent(this, &Page::stateWanted)      // parameter read off the method
    //
    // One property type serves every event, so an emitter needs no named handler
    // type per event it raises. The event is already written in the handler, and
    // a name for it would only say the same thing twice.
    //
    // A generic `[](auto&)` handler names no event and is rejected. With OnEvent
    // the parameter type is how you choose which event to connect to.
    export template <typename F>
        requires NamesAnEvent<F>
    struct OnEvent : public OnEventTag
    {
        using Event = event_param_t<F>;

        F handler;

        explicit OnEvent(F handler);

        // Connects to anything with EventComponent's interface.
        template <typename Emitter>
        void connectTo(Emitter& emitter) const;
    };

    export template <typename F> OnEvent(F) -> OnEvent<F>;

    // Several handlers as one property, so an argument list does not repeat OnEvent per handler:
    //
    //     Button{ params, Events{
    //         [](ClickEvent& event) { ... },
    //         [](PaintIconEvent& event) { ... },
    //     } }
    //
    // Each handler names its own event, so the list is heterogeneous and its order says nothing.
    // An already wrapped handler is accepted alongside bare ones, for a call site that wants the
    // event named: Events{ Button::OnClick{ handler }, [](PaintIconEvent& event) { ... } }.
    export template <typename... Handlers>
        requires (sizeof...(Handlers) > 0 && (IsHandler<Handlers> && ...))
    struct Events : public OnEventTag
    {
        std::tuple<Handlers...> handlers;

        explicit Events(Handlers... handlers);

        // Connects to anything with EventComponent's interface, one handler at a time.
        template <typename Emitter>
        void connectTo(Emitter& emitter) const;
    };

    export template <typename... Handlers> Events(Handlers...) -> Events<Handlers...>;

    // makeOnEvent(this, &Page::method) - the event comes off the method
    // signature. A function rather than a constructor: a deduction guide cannot
    // synthesise the std::function wrapper the pointer-to-member pair needs.
    export template <typename TObject, typename Method>
        requires std::is_member_function_pointer_v<Method>
    [[nodiscard]] auto makeOnEvent(TObject* object, Method method);


    //-------------------------------------------------------------------------


    // EventConnection

    EventConnection::EventConnection(EventTypeId typeId, EventConnectionId id, EventDispatcher* dispatcher)
        :
        m_eventTypeId{ typeId },
        m_connectionId{ id },
        m_dispatcher{ dispatcher }
    {
        if (m_dispatcher)
            m_dispatcher->track(*this);
    }

    EventConnection::EventConnection(EventConnection&& other) noexcept
        :
        EventConnection{ other.m_eventTypeId, other.m_connectionId, other.m_dispatcher }
    {
        other.release();
    }

    EventConnection::~EventConnection()
    {
        release();
    }

    void EventConnection::disconnect() const
    {
        if (m_dispatcher)
            m_dispatcher->disconnect(*this);
    }

    EventConnection::EventConnection(const EventConnection& other)
        :
        EventConnection{ other.m_eventTypeId, other.m_connectionId, other.m_dispatcher }
    {
    }

    EventConnection& EventConnection::operator=(const EventConnection& other)
    {
        if (this == &other)
            return *this;

        release();
        m_eventTypeId = other.m_eventTypeId;
        m_connectionId = other.m_connectionId;
        m_dispatcher = other.m_dispatcher;
        if (m_dispatcher)
            m_dispatcher->track(*this);
        return *this;
    }

    void EventConnection::release()
    {
        if (m_dispatcher)
            m_dispatcher->untrack(*this);
        m_dispatcher = nullptr;
    }

    // EventDispatcher

    EventDispatcher::~EventDispatcher()
    {
        // The pointer, not the listener list: what a connection does with a dispatcher that is
        // gone is nothing at all, and that is the whole of the fix. The listeners go with the map.
        for (EventConnection* connection : m_connections)
            connection->m_dispatcher = nullptr;
    }

    EventDispatcher::EventDispatcher(const EventDispatcher& other)
        :
        m_nextConnectionId{ other.m_nextConnectionId },
        m_listeners{ other.m_listeners }
    {
    }

    EventDispatcher& EventDispatcher::operator=(const EventDispatcher& other)
    {
        if (this == &other)
            return *this;

        m_nextConnectionId = other.m_nextConnectionId;
        m_listeners = other.m_listeners;
        return *this;
    }

    template<IsEvent TEvent>
    EventConnection EventDispatcher::connect(auto&& callback)
    {
        EventConnectionId connectionId = m_nextConnectionId++;
        EventTypeId typeId = EventTypeIdGenerator<std::decay_t<TEvent>>::id();
        m_listeners[typeId].emplace_back(
            connectionId,
            [cb = std::forward<decltype(callback)>(callback)](Event& e) mutable {
                cb(static_cast<TEvent&>(e));
            }
        );
        return { typeId, connectionId, this };
    }

    template<IsEvent TEvent, typename TObject, IsMethod Method>
    EventConnection EventDispatcher::connect(TObject* object, Method method)
    {
        return connect<TEvent>(
            [object, method](TEvent& e) {
                (object->*method)(e);
            }
        );
    }

    void EventDispatcher::disconnect(const EventConnection& conn)
    {
        if (auto it = m_listeners.find(conn.eventTypeId()); it != m_listeners.end())
        {
            auto& list = it->second;
            list.erase(std::remove_if(list.begin(), list.end(),
                [&](const auto& pair) { return pair.first == conn.connectionId(); }),
                list.end());
        }
    }

    template<IsEvent TEvent, typename ...Args>
    void EventDispatcher::emit(Args && ...args) const
    {
        TEvent event(std::forward<Args>(args)...);
        emit<TEvent>(event);
    }

    template<IsEvent TEvent>
    void EventDispatcher::emit(TEvent& event) const
    {
        if (m_listeners.empty())
        {
            return;
        }
        EventTypeId eventTypeId = EventTypeIdGenerator<std::decay_t<TEvent>>::id();
        if (auto it = m_listeners.find(eventTypeId); it != m_listeners.end())
            for (const auto& [id, callback] : it->second)
            {
                callback(event);
                if (event.propagationStopped())
                    break;
            }
    }

    template<IsEvent TEvent>
    bool EventDispatcher::hasListeners() const
    {
        if (m_listeners.empty())
            return false;

        const auto it = m_listeners.find(EventTypeIdGenerator<std::decay_t<TEvent>>::id());
        return it != m_listeners.end() && !it->second.empty();
    }

    void EventDispatcher::track(EventConnection& connection)
    {
        m_connections.push_back(&connection);
    }

    void EventDispatcher::untrack(const EventConnection& connection)
    {
        // The last one first. A connection is usually released close to where it was made, and a
        // temporary handed straight into a ScopedEventConnection is released before anything else
        // has registered.
        for (std::size_t at = m_connections.size(); at != 0; --at)
        {
            if (m_connections[at - 1] != &connection)
                continue;

            m_connections.erase(m_connections.begin() + (at - 1));
            return;
        }
    }

    // OnEvent

    template <typename F>
        requires NamesAnEvent<F>
    OnEvent<F>::OnEvent(F handler)
        :
        handler{ std::move(handler) }
    {
    }

    template <typename F>
        requires NamesAnEvent<F>
    template <typename Emitter>
    void OnEvent<F>::connectTo(Emitter& emitter) const
    {
        // The deducing overload reads the same event off the handler, which is where Event came
        // from, so naming it here would say it twice - and would need a `.template` to do so.
        emitter.connectEvent(handler);
    }

    template <typename TObject, typename Method>
        requires std::is_member_function_pointer_v<Method>
    auto makeOnEvent(TObject* object, Method method)
    {
        using Event = typename EventParamOfImpl<Method>::type;
        using Handler = std::function<void(Event&)>;

        Handler handler = [object, method](Event& event) {
            (object->*method)(event);
        };
        return OnEvent<Handler>{ std::move(handler) };
    }

    // Events

    template <typename... Handlers>
        requires (sizeof...(Handlers) > 0 && (IsHandler<Handlers> && ...))
    Events<Handlers...>::Events(Handlers... handlers)
        :
        handlers{ std::move(handlers)... }
    {
    }

    template <typename... Handlers>
        requires (sizeof...(Handlers) > 0 && (IsHandler<Handlers> && ...))
    template <typename Emitter>
    void Events<Handlers...>::connectTo(Emitter& emitter) const
    {
        auto connectOne = [&emitter](const auto& handler) {
            if constexpr (IsOnEvent<decltype(handler)>)
                handler.connectTo(emitter);
            else
                emitter.connectEvent(handler);
        };
        std::apply([&](const Handlers&... handler) {
            (connectOne(handler), ...);
        }, handlers);
    }

}
