// =========================================================================
// ClaFi.Core.Dt
//
// The machinery every declarative-tree DSL needs, and nothing that belongs to
// any one of them in particular. A DSL built on this brings its own build
// context types and its own node vocabulary. What it gets from here is:
//
//   - a node base per build context, plus references so one declared node can
//     be shared by several parents without copying
//   - the pack routing that splits a constructor pack into child nodes and
//     properties at compile time
//   - OnEvent, which reads the event type off the handler's own parameter, and
//     the Init / PostCreate pair that runs against a freshly created object
//
// Grid_Dt and Dom_Dt both import this. Inside either of them the name Dt
// resolves to their own namespace, so they alias this one.
// =========================================================================
export module ClaFi.Core.Dt;

import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi::Dt
{
    // =====================================================================
    // Constructor guards
    // =====================================================================

    // A variadic constructor is a better match for a non-const lvalue of its own
    // type than the copy constructor is. Without this guard `Column b{ a };`
    // would treat `a` as a property, or in Dom's case as a child node, instead
    // of copying it. Every greedy node constructor carries it.
    // Restated rather than pulled in by a using-declaration, which cannot name a concept.
    export template <typename Self, typename... Args>
    concept NotSelfCopy = ::ClaFi::NotSelfCopy<Self, Args...>;

    // =====================================================================
    // Nodes
    // =====================================================================

    // One node hierarchy per build context. A DSL that builds a single kind of
    // thing has one context. Dom's is the parent section a node applies itself
    // to. A DSL that builds several kinds has one context per kind.
    //
    // The context type keeps the hierarchies apart. A Grid RowNode will not bind
    // where a CellNode is expected, and the pack routing below tells them apart
    // by it.
    export template <typename Context>
        struct NodeBase
    {
        virtual ~NodeBase() = default;
        virtual void apply(Context&) const = 0;
    };

    export template <typename Context>
        using NodePtr = std::unique_ptr<NodeBase<Context>>;

    export template <typename Context>
        using NodeList = std::vector<NodePtr<Context>>;

    export template <typename T, typename Context>
    concept IsNode = std::is_base_of_v<NodeBase<Context>, std::decay_t<T>>;

    // References let a node declared once, as a const member or a namespace
    // constant, be reused by several parents without copying. The referent has
    // to outlive the apply.
    export template <typename Context>
        struct Reference : public NodeBase<Context>
    {
        const NodeBase<Context>& ref;

        explicit Reference(const NodeBase<Context>& node);

        void apply(Context& context) const override;
    };

    // =====================================================================
    // Handlers
    // =====================================================================

    // The handler machinery lives in ClaFi.Core.System.Events, next to the
    // dispatcher it drives, so that anything with an EventDispatcher can accept
    // a handler without depending on this module. Re-exported here so call sites
    // keep writing Dt::OnEvent, and so Grid_Dt and Dom_Dt keep re-exporting it
    // from here.
    export using ::ClaFi::OnEventTag;
    export using ::ClaFi::OnEvent;
    export using ::ClaFi::makeOnEvent;

    // Restated rather than pulled in by a using-declaration, which cannot name a concept. Each
    // is defined as the original, so normalization reaches the same atomic constraints and a
    // Dt::IsOnEvent still subsumes - and is subsumed by - the ::ClaFi one.
    export template <typename F>
    concept NamesAnEvent = ::ClaFi::NamesAnEvent<F>;

    export template <typename T>
    concept IsOnEvent = ::ClaFi::IsOnEvent<T>;

    export template <typename T>
    concept IsHandler = ::ClaFi::IsHandler<T>;

    // Wraps a bare handler so that a DSL can accept `[](SomeEvent& event){ ... }`
    // written directly, without the OnEvent around it.
    //
    // Only for a DSL with a closed vocabulary, where every argument is either a
    // child node or a handler - Dom. A DSL that forwards whatever it does not
    // recognise as a property to an imperative add() must not do this. There, a
    // bare callable is ambiguous between a property and a handler, and the
    // OnEvent wrapper is what resolves it.
    export template <typename T>
    [[nodiscard]] decltype(auto) asHandler(T&& arg);

    // =====================================================================
    // Parts
    // =====================================================================

    // What every Part derives from, so a pack can tell a part from a property.
    export struct PartBase
    {
    };

    // A PART OF A CONTAINER, as opposed to a child sitting inside it:
    //
    //     Expander{ Header{ Text{ ... } }, Rows{ ... } }
    //     Grid{ Columns{ ... }, Header{}, Rows{ ... } }
    //
    // A part is deliberately not a node of any hierarchy. If it were a node of the
    // one its container's children belong to, a container could not tell its part
    // apart from its first child - they would arrive in the same vector. So a part
    // keeps its argument pack and the enclosing container routes it, which is also
    // why this module needs to know nothing about what any particular part holds.
    //
    // The consequence worth knowing: a part only means anything to the container
    // that directly encloses it. One nested a level deeper, inside a Rows{} for
    // instance, is not a child of anything that looks for it. Containers that could
    // receive one by mistake reject it rather than dropping it.
    //
    // Header is the part this module names. A DSL names one of its own by deriving
    // from Part, the way Grids names a group's Span.
    export template <typename... Args>
        struct Part : public PartBase
    {
        using Pack = std::tuple<std::decay_t<Args>...>;

        Pack pack;

        explicit Part(Args&&... args);

        Part(Part&&) noexcept = default;
        Part& operator=(Part&&) noexcept = default;
        Part(const Part&) = delete;
        Part& operator=(const Part&) = delete;
    };

    export template <typename T>
    concept IsPart = std::is_base_of_v<PartBase, std::decay_t<T>>;

    template <template <typename...> typename Named, typename T>
    struct IsPartNamedImpl : std::false_type {};
    template <template <typename...> typename Named, typename... Args>
    struct IsPartNamedImpl<Named, Named<Args...>> : std::true_type {};

    // Whether T is the part a container asks for by name - Header, or one a DSL names.
    export template <typename T, template <typename...> typename Named>
    concept IsPartNamed = IsPartNamedImpl<Named, std::decay_t<T>>::value;

    // Calls func with the contents of the Named part in this pack, expanded as an
    // argument list, and reports whether there was one. The container then routes
    // those arguments with the same propsOf / makeNodeVector it uses for its own.
    export template <template <typename...> typename Named, typename Func, typename... Args>
    bool withPart(Func&& func, Args&&... args);

    // A container's own header. What a header accepts is the container's business. A
    // grid's header takes nothing - the columns already say what belongs in it. An
    // expander's header is a single text control, so it takes text and rejects cells.
    export template <typename... Args>
        struct Header : public Part<Args...>
    {
        explicit Header(Args&&... args);
    };

    export template <typename... Args> Header(Args&&...) -> Header<Args...>;

    // Init<T>{ lambda } - runs against the object right after it is created, for
    // the setters that have no constructor property (Slider::setMaxPosition and
    // friends). Pass it anywhere in the argument list.
    export template <typename Target>
        struct Init
    {
        using Action = std::function<void(Target&)>;

        Action func;

        template <typename F>
        explicit Init(F&& initializer);
    };

    template <typename T> struct IsAnyInitImpl : std::false_type {};
    template <typename Target> struct IsAnyInitImpl<Init<Target>> : std::true_type {};

    export template <typename T>
    concept IsAnyInit = IsAnyInitImpl<std::decay_t<T>>::value;

    export template <typename T, typename Target>
    concept IsInit = std::is_same_v<std::decay_t<T>, Init<Target>>;

    // Everything a pack says about an object that is not a constructor argument:
    // the events to connect, and the initialiser to run.
    export template <typename Target>
        struct PostCreate
    {
        using Action = std::function<void(Target&)>;
        using ActionCollection = std::vector<Action>;

        // A pack can hold several handlers, each with its own event and its own
        // handler type, so they cannot be collected as one type the way Init is.
        ActionCollection connectors;
        Action init;

        void operator()(Target& target) const;
        void merge(PostCreate&& other);
    };

    // =====================================================================
    // Pack routing
    // =====================================================================

    template <typename Context, typename T>
    [[nodiscard]] auto propOf(T&& arg);

    template <typename Target, typename T>
    [[nodiscard]] auto ctorPropOf(T&& arg);

    template <typename Context, typename T>
    [[nodiscard]] auto containerCtorPropOf(T&& arg);

    // Child nodes are taken by value when they arrive as rvalues and by
    // reference when they arrive as lvalues. Anything else in the pack is a
    // property or a handler, and is picked up by propsOf, ctorPropsOf or
    // postCreateOf instead, so this skips it rather than rejecting it. A DSL
    // with a closed vocabulary constrains its own constructor pack; see
    // Dom_Dt's IsSchemaArg.
    export template <typename Context, typename T>
    void appendNode(NodeList<Context>& dest, T&& arg);

    export template <typename Context, typename... Args>
    [[nodiscard]] NodeList<Context> makeNodeVector(Args&&... args);

    // Splits a constructor pack into properties to forward and child nodes, at
    // compile time. The discriminator is the node hierarchy: whatever is not a
    // child of this context is a property.
    export template <typename Context, typename... Args>
    [[nodiscard]] auto propsOf(Args&&... args);

    // Splits a pack into constructor arguments for Target and things to do to
    // Target once it exists. The discriminator here is the handler types, not
    // the node hierarchy: the node creates a live object and holds no children.
    //
    // ctorPropsOf moves out of the pack, postCreateOf only copies. The two see a
    // disjoint set of arguments, so calling both on one pack is safe whatever
    // order the captures are initialised in.
    export template <typename Target, typename... Args>
    [[nodiscard]] auto ctorPropsOf(Args&&... args);

    export template <typename Target, typename... Args>
    [[nodiscard]] PostCreate<Target> postCreateOf(Args&&... args);

    // ctorPropsOf for a node that holds child nodes besides: those and its parts are left out too.
    export template <typename Context, typename... Args>
    [[nodiscard]] auto containerCtorPropsOf(Args&&... args);
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dt
{
    // --- Reference ---

    template <typename Context>
    Reference<Context>::Reference(const NodeBase<Context>& node)
        :
        ref{ node }
    {
    }

    template <typename Context>
    void Reference<Context>::apply(Context& context) const
    {
        ref.apply(context);
    }

    template <typename T>
    decltype(auto) asHandler(T&& arg)
    {
        // Already wrapped, or not a handler at all: pass it through untouched,
        // so this can be mapped over a whole constructor pack.
        if constexpr (IsOnEvent<T> || !NamesAnEvent<std::decay_t<T>>)
        {
            return std::forward<T>(arg);
        }
        else
        {
            return OnEvent{ std::forward<T>(arg) };
        }
    }

    // --- Parts ---

    template <typename... Args>
    Part<Args...>::Part(Args&&... args)
        :
        pack{ std::forward<Args>(args)... }
    {
    }

    template <template <typename...> typename Named, typename Func, typename... Args>
    bool withPart(Func&& func, Args&&... args)
    {
        bool found = false;
        auto probe = [&func, &found](auto&& arg) {
            if constexpr (IsPartNamed<decltype(arg), Named>)
            {
                found = true;
                std::apply(func, arg.pack);
            }
        };
        (probe(args), ...);
        return found;
    }

    template <typename... Args>
    Header<Args...>::Header(Args&&... args)
        :
        Part<Args...>{ std::forward<Args>(args)... }
    {
    }

    // --- Init ---

    template <typename Target>
    template <typename F>
    Init<Target>::Init(F&& initializer)
        :
        func{ std::forward<F>(initializer) }
    {
    }

    // --- PostCreate ---

    template <typename Target>
    void PostCreate<Target>::operator()(Target& target) const
    {
        // Connect before init: an Init that sets a value should be able to fire
        // the handlers the caller just attached.
        for (const Action& connect : connectors)
        {
            connect(target);
        }
        if (init)
        {
            init(target);
        }
    }

    template <typename Target>
    void PostCreate<Target>::merge(PostCreate&& other)
    {
        connectors.reserve(connectors.size() + other.connectors.size());
        for (Action& connect : other.connectors)
        {
            connectors.push_back(std::move(connect));
        }
        other.connectors.clear();
        if (!init)
        {
            init = std::move(other.init);
        }
    }

    // --- Pack routing ---

    template <typename Context, typename T>
    void appendNode(NodeList<Context>& dest, T&& arg)
    {
        if constexpr (IsNode<T, Context>)
        {
            if constexpr (std::is_lvalue_reference_v<T>)
            {
                // Shared, not copied. An lvalue node keeps living where it was
                // declared.
                dest.push_back(std::make_unique<Reference<Context>>(arg));
            }
            else
            {
                dest.push_back(std::make_unique<std::decay_t<T>>(std::forward<T>(arg)));
            }
        }
    }

    template <typename Context, typename... Args>
    NodeList<Context> makeNodeVector(Args&&... args)
    {
        NodeList<Context> nodes = {};
        nodes.reserve(sizeof...(args));
        (appendNode<Context>(nodes, std::forward<Args>(args)), ...);
        return nodes;
    }

    template <typename Context, typename T>
    auto propOf(T&& arg)
    {
        if constexpr (IsNode<T, Context> || IsPart<T>)
        {
            // A part is consumed by the container that encloses it, so it must not
            // reach the imperative call the other properties are forwarded to.
            return std::tuple<>{};
        }
        else
        {
            // A handler in a pack whose properties go straight to an imperative
            // add() would be forwarded as a property, and fail inside the
            // overload set. Say so here instead.
            static_assert(!IsOnEvent<T>,
                "Dt: this node forwards its properties to the imperative API and does not "
                "connect handlers. Put OnEvent on a node that creates an emitter of its own.");
            static_assert(!IsAnyInit<T>,
                "Dt: this node forwards its properties to the imperative API and does not "
                "run initialisers. Put Init on a node that creates an object of its own.");
            return std::tuple<std::decay_t<T>>{ std::forward<T>(arg) };
        }
    }

    template <typename Context, typename... Args>
    auto propsOf(Args&&... args)
    {
        return std::tuple_cat(propOf<Context>(std::forward<Args>(args))...);
    }

    template <typename Target, typename T>
    auto ctorPropOf(T&& arg)
    {
        if constexpr (IsOnEvent<T> || IsAnyInit<T>)
        {
            return std::tuple<>{};
        }
        else
        {
            return std::tuple<std::decay_t<T>>{ std::forward<T>(arg) };
        }
    }

    template <typename Target, typename... Args>
    auto ctorPropsOf(Args&&... args)
    {
        return std::tuple_cat(ctorPropOf<Target>(std::forward<Args>(args))...);
    }

    template <typename Context, typename T>
    auto containerCtorPropOf(T&& arg)
    {
        if constexpr (IsNode<T, Context> || IsPart<T> || IsOnEvent<T> || IsAnyInit<T>)
        {
            return std::tuple<>{};
        }
        else
        {
            return std::tuple<std::decay_t<T>>{ std::forward<T>(arg) };
        }
    }

    template <typename Context, typename... Args>
    auto containerCtorPropsOf(Args&&... args)
    {
        return std::tuple_cat(containerCtorPropOf<Context>(std::forward<Args>(args))...);
    }

    template <typename Target, typename... Args>
    PostCreate<Target> postCreateOf(Args&&... args)
    {
        PostCreate<Target> result = {};
        auto probe = [&result](auto&& arg) {
            using Arg = decltype(arg);
            if constexpr (IsOnEvent<Arg>)
            {
                result.connectors.push_back([node = arg](Target& target) {
                    node.connectTo(target);
                });
            }
            else if constexpr (IsAnyInit<Arg>)
            {
                static_assert(IsInit<Arg, Target>,
                    "Dt: Init<T> names a different type than the object this node creates.");
                result.init = arg.func;
            }
        };
        (probe(args), ...);
        return result;
    }
}
