// =========================================================================
// EventBindings.h
//
// Declaration and binding boilerplate for controls that expose events and
// accept them as construction properties.
// =========================================================================
#pragma once

// -------------------------------------------------------------------------
// Declaration
// -------------------------------------------------------------------------

// Declares one event on a control: the connect helper, and the name a caller
// spells to pass a handler as a construction property.
//
//     DECLARE_EVENT(ClickEvent, OnClick, onClick)
//
//     button.onClick([](ClickEvent& event) { ... });   // connect later
//     Button{ params, Button::OnClick{ handler } }     // connect at construction
//
// handlerAlias is a spelling of OnEvent, which reads the event off the handler's
// own parameter. The alias states intent and gives a reader something to search
// for; the event type in the handler is what actually selects the channel.
// Control's constructor connects every OnEvent in its pack, so a line here is all
// it takes to add an event - there is no binding to write anywhere.
//
// An event type is a channel. Two handlers that mean different things get two
// event types: the type is what a reader sees at the connect site, and the only
// thing that tells them which of the two they are on.
#define DECLARE_EVENT(eventType, handlerAlias, methodName) \
    template <typename F> using handlerAlias = OnEvent<F>;\
    EventConnection methodName(auto&& callback) {\
        return connectEvent<eventType>(std::forward<decltype(callback)>(callback));\
    }


// -------------------------------------------------------------------------
// Property binding
// -------------------------------------------------------------------------

// A property the class keeps nowhere of its own - it lands in a field of another
// member, goes through a setter, or is read straight into a call. The binding line
// IS the declaration: it names the property's type, and that is what the scanner
// reads the surface off. One comment line, like every other declaration.
//
//     // Space kept inside the control's border, around its content.
//     BIND_PROPERTY_MEMBER(Padding, m_metrics.padding);
//
// No default is stated, and none should be: what the control starts at is whatever
// the storage already held, and that belongs to whoever declared the storage. Where
// the control has to be told even when the pack says nothing, READ_PROPERTY carries
// the default instead.
//
// All of these expect the pack to be named `args`.

// Runs the statement when a property of the given type is present. Inside the
// statement the property is named `p`. The three below are the shorthands worth
// having; reach for this one when what you do with the property is not one of them.
#define BIND_PROPERTY_ACTION(type, statement) \
    Props::ifThereIs<type>([&](auto&& p) { statement; }, std::forward<Args>(args)...)

// Assigns the property to a member, or to a field of one.
#define BIND_PROPERTY_MEMBER(type, member) BIND_PROPERTY_ACTION(type, (member) = p)

// Hands the property to a setter. Event handlers do not need this: a control takes
// them as OnEvent, and EventComponent's constructor connects them.
#define BIND_PROPERTY_CALL(type, func) BIND_PROPERTY_ACTION(type, func(p))

// Same, for property wrappers that carry their payload in `value`.
#define BIND_PROPERTY_VALUE(type, func) BIND_PROPERTY_ACTION(type, func(p.value))

// Reads the property out of the pack as an expression, with the default it answers
// when the pack says nothing. The read a constructor makes anyway, with the type and
// the default on the line the scanner finds them on:
//
//     // Whether the control's text wraps rather than being trimmed.
//     setWordWrap(READ_PROPERTY(WordWrap, WordWrap::Yes));
//
// The pack is passed as it stands rather than forwarded. This one is read from plain
// functions as well as from constructors - DropdownControlBase::makeConfig takes a
// `const Args&...` - and forwarding a const lvalue as an rvalue does not compile.
// Nothing is lost: Props::get copies out of the pack and never moves.
#define READ_PROPERTY(type, ...) \
    Props::get(type{ __VA_ARGS__ }, args...)


// -------------------------------------------------------------------------
// Property declaration
// -------------------------------------------------------------------------

// Declares one construction property: the constant holding its default, the
// member, and the getter that reads it back.
//
//     DECLARE_PROPERTY(SelectionMode, selectionMode, SelectionMode::None)
//
//     StackView{ params, SelectionMode::Multi }        // set at construction
//     view.selectionMode()                             // read back
//
// The member is m_ plus the name, the default is s_ plus the name plus Default,
// and both are private. The line belongs in a public block and leaves the block
// public.
//
// The default is everything after the name, so a value with commas in it needs no
// protection. It is always spelled - what a control starts as is part of the
// surface a designer tool reads - and it is spelled once: the member initialiser
// and INIT_PROPERTY below both read the constant.
//
// The type has to be a literal type, which is what makes the default a constant
// expression. A type whose default cannot be one takes DECLARE_REF_PROPERTY.
#define DECLARE_PROPERTY(type, name, ...) \
    [[nodiscard]] type name() const { return m_##name; } \
    private: \
        static constexpr type s_##name##Default{ __VA_ARGS__ }; \
        type m_##name{ s_##name##Default }; \
    public:

// Same, for a property that can also be set after construction. It names the setter
// rather than declaring it: a change that has to invalidate something cannot have a
// generated body, and a one-operation setter belongs inline where it is written. The
// scanner checks that the class has the setter it names.
#define DECLARE_WRITABLE_PROPERTY(type, name, setterName, ...) \
    [[nodiscard]] type name() const { return m_##name; } \
    private: \
        static constexpr type s_##name##Default{ __VA_ARGS__ }; \
        type m_##name{ s_##name##Default }; \
    public:

// Same, for a type not worth copying out of the getter. Its default is a const
// object rather than a constant expression, which is also what a type that
// allocates needs - the two are the same set of types in practice.
#define DECLARE_REF_PROPERTY(type, name, ...) \
    [[nodiscard]] const type& name() const { return m_##name; } \
    private: \
        static inline const type s_##name##Default{ __VA_ARGS__ }; \
        type m_##name{ s_##name##Default }; \
    public:

// Same, where the class writes the getter itself - one that overrides a base virtual, or
// that hands back something other than the member. The declaration still states the type,
// the default and the storage, so the surface stays complete; the scanner checks that a
// getter of the property's name is there.
#define DECLARE_PROPERTY_STORAGE(type, name, ...) \
    private: \
        static constexpr type s_##name##Default{ __VA_ARGS__ }; \
        type m_##name{ s_##name##Default }; \
    public:

// Reads one declared property off the constructor pack, as an initializer:
//
//     StackView::StackView(const CreateParams& params, Args&&... args)
//         :
//         StackPanel{ params, std::forward<Args>(args)... },
//         INIT_PROPERTY(selectionMode),
//         INIT_PROPERTY(dragMode)
//
// Names the property alone - the declaration has stated the member, its type and
// its default. Expects the pack to be named `args`. The initializers follow the
// order the properties are declared in, which is what the compiler asks for.
//
// A property left out of the list keeps its declared default rather than holding
// nothing, and the scanner reports the omission.
#define INIT_PROPERTY(name) \
    m_##name{ Props::get(s_##name##Default, std::forward<Args>(args)...) }
