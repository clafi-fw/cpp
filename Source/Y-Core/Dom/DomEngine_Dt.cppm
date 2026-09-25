export module ClaFi.Core.DomEngine_Dt;

import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Events;

import ClaFi.Core.Dt;
import ClaFi.StdLib;

namespace ClaFi::Dom::Rt
{
    using Section = ::ClaFi::Dom::Section;
}


namespace ClaFi::Dom::Dt
{
    // The shared declarative-tree machinery. It needs an alias: `Dt` on its own
    // resolves to this namespace, not to ClaFi::Dt.
    namespace Tree = ::ClaFi::Dt;

    // Re-exported so call sites write Dom::Dt::OnEvent, next to Grid::Dt::OnEvent.
    export using ::ClaFi::Dt::OnEvent;
    export using ::ClaFi::Dt::makeOnEvent;

    template <typename T>
    concept IsSectionKey = std::is_convertible_v<std::decay_t<T>, std::wstring_view>;

    // The Dom builds one kind of thing, so it has one build context: the parent
    // section a node applies itself to.
    export using Node = Tree::NodeBase<Rt::Section>;
    export using NodePtr = Tree::NodePtr<Rt::Section>;
    export using NodeList = Tree::NodeList<Rt::Section>;
    export using NodeReference = Tree::Reference<Rt::Section>;

    // Handlers are attached to whatever a node creates, so each node kind knows
    // the concrete Dom type it emits from.
    export using SectionHandlers = Tree::PostCreate<Rt::Section>;

    // A handler, wrapped or bare. Dom accepts `[](ChangeEvent& event){ ... }`
    // written straight into the pack. The parameter already names the event, and
    // Dom forwards nothing to an imperative API, so there is nothing a bare
    // callable could be confused with. This is also what kept the old
    // `Value{ key, def, someLambda }` spelling working: OnChange's converting
    // constructor was implicit.
    export template <typename T>
    concept IsHandlerArg = Tree::IsHandler<T>;

    // Dom's vocabulary is closed - section, value, sequence - so an argument that
    // is neither a child node nor a handler is a mistake. Saying so in the
    // constraint puts the diagnostic at the call site, naming the offending type,
    // instead of inside the routing helpers. This is what the old static_assert
    // in appendToVector was for.
    export template <typename T>
    concept IsSchemaArg = Tree::IsNode<T, Rt::Section> || IsHandlerArg<T>;

    // =====================================================================
    // Compatibility
    // =====================================================================

    // OnChange and OnNestedChange are named handler types kept so existing schemas
    // compile unchanged. New code writes OnEvent{ [](ChangeEvent& event){ ... } }
    // and lets the parameter choose the event.
    //
    // Both are handlers rather than nodes, so they connect as soon as the section
    // exists, before any child is applied, wherever they sit in the child list.
    // Nothing in the Dom raises an event while a schema is being applied -
    // addValue, addSection and addSequence only insert - so a handler cannot
    // observe the ordering.

    export struct OnChange : public Tree::OnEvent<std::function<void(ChangeEvent&)>>
    {
        using Base = Tree::OnEvent<std::function<void(ChangeEvent&)>>;
        using Handler = std::function<void(ChangeEvent&)>;

        template <typename F>
            requires std::is_invocable_v<F&, ChangeEvent&>
        OnChange(F&& handler);

        template <typename TObject, typename Method>
            requires std::is_member_function_pointer_v<Method>
        OnChange(TObject* object, Method method);
    };

    export struct OnNestedChange : public Tree::OnEvent<std::function<void(NestedChangeEvent&)>>
    {
        using Base = Tree::OnEvent<std::function<void(NestedChangeEvent&)>>;
        using Handler = std::function<void(NestedChangeEvent&)>;

        template <typename F>
            requires std::is_invocable_v<F&, NestedChangeEvent&>
        OnNestedChange(F&& handler);

        template <typename TObject, typename Method>
            requires std::is_member_function_pointer_v<Method>
        OnNestedChange(TObject* object, Method method);
    };

    // =====================================================================
    // Section
    // =====================================================================

    // A section of the described tree, keyed or a grouping in the source alone. See Dom
    export struct Section : public Node
    {
        std::wstring key;
        NodeList children;
        SectionHandlers handlers;

        Section() = default;

        template <typename... Args>
            requires (IsSchemaArg<Args> && ...)
        Section(std::wstring_view key, Args&&... args);

        template <typename First, typename... Rest>
            requires (!IsSectionKey<First>)
                  && Tree::NotSelfCopy<Section, First, Rest...>
                  && IsSchemaArg<First> && (IsSchemaArg<Rest> && ...)
        Section(First&& first, Rest&&... rest);

        Section(Section&&) noexcept = default;
        Section& operator=(Section&&) noexcept = default;
        Section(const Section&) = delete;
        Section& operator=(const Section&) = delete;

        void apply(Rt::Section& parent) const override;
        void append(Section&& other);
        void extractTo(NodeList& dest);
    };

    // =====================================================================
    // Value
    // =====================================================================

    // Value{ L"key", defaultValue, OnEvent{ ... }, OnEvent{ ... } }
    //
    // Any number of handlers, each naming its own event.
    export template <typename T>
        struct Value : public Node
    {
        using LogicalT = dom_storage_t<T>;
        using DomValue = Dom::Value<LogicalT>;
        using ValueHandlers = Tree::PostCreate<DomValue>;

        std::wstring key;
        LogicalT defaultValue;
        ValueHandlers handlers;

        template <typename U, typename... Handlers>
            requires (IsHandlerArg<Handlers> && ...)
        Value(std::wstring_view key, U&& defaultValue, Handlers&&... eventHandlers);

        void apply(Rt::Section& parent) const override;
    };

    export template <typename U, typename... Handlers>
        Value(std::wstring_view, U&&, Handlers&&...) -> Value<std::decay_t<U>>;

    // =====================================================================
    // Sequence
    // =====================================================================

    export template <typename T>
        struct Sequence : public Node
    {
        using LogicalT = dom_storage_t<T>;
        using DomSequence = Dom::Sequence<LogicalT>;
        using SequenceHandlers = Tree::PostCreate<DomSequence>;

        std::wstring key;
        LogicalT itemBlueprint;
        SequenceHandlers handlers;

        template <typename U, typename... Handlers>
            requires (IsHandlerArg<Handlers> && ...)
        Sequence(std::wstring_view key, U&& itemBlueprint, Handlers&&... eventHandlers);

        void apply(Rt::Section& parent) const override;
    };

    // A sequence of sections takes a child schema instead of an item value. The
    // schema is applied once to a template section, and that template is what the
    // sequence clones per item.
    //
    // Handlers passed here connect to the sequence node itself. Handlers written
    // inside the child schema connect to the template section, which is cloned per
    // item. Connections are not cloned, so they will not reach the items. Connect
    // to the sequence and read the origin off the event instead.
    template <>
    struct Sequence<Section> : public Node
    {
        using DomSequence = Dom::Sequence<Rt::Section>;
        using SequenceHandlers = Tree::PostCreate<DomSequence>;
        using SectionPtr = std::unique_ptr<Section>;

        std::wstring key;
        SectionPtr ownChildSchema;
        const Section* childSchema;
        SequenceHandlers handlers;

        template <typename... Handlers>
            requires (IsHandlerArg<Handlers> && ...)
        Sequence(std::wstring_view key, const Section& childSchema, Handlers&&... eventHandlers);

        template <typename... Handlers>
            requires (IsHandlerArg<Handlers> && ...)
        Sequence(std::wstring_view key, Section&& childSchema, Handlers&&... eventHandlers);

        Sequence(Sequence&& other) noexcept;
        Sequence& operator=(Sequence&& other) noexcept;

        Sequence(const Sequence&) = delete;
        Sequence& operator=(const Sequence&) = delete;

        void apply(Rt::Section& parent) const override;
    };

    export template <typename U, typename... Handlers>
        Sequence(std::wstring_view, U&&, Handlers&&...) -> Sequence<std::decay_t<U>>;
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom::Dt
{
    // --- OnChange ---

    template <typename F>
        requires std::is_invocable_v<F&, ChangeEvent&>
    OnChange::OnChange(F&& handler)
        :
        Base{ Handler{ std::forward<F>(handler) } }
    {
    }

    template <typename TObject, typename Method>
        requires std::is_member_function_pointer_v<Method>
    OnChange::OnChange(TObject* object, Method method)
        :
        Base{ Handler{ [object, method](ChangeEvent& event) {
            (object->*method)(event);
        } } }
    {
    }

    // --- OnNestedChange ---

    template <typename F>
        requires std::is_invocable_v<F&, NestedChangeEvent&>
    OnNestedChange::OnNestedChange(F&& handler)
        :
        Base{ Handler{ std::forward<F>(handler) } }
    {
    }

    template <typename TObject, typename Method>
        requires std::is_member_function_pointer_v<Method>
    OnNestedChange::OnNestedChange(TObject* object, Method method)
        :
        Base{ Handler{ [object, method](NestedChangeEvent& event) {
            (object->*method)(event);
        } } }
    {
    }

    // --- Section ---

    template <typename... Args>
        requires (IsSchemaArg<Args> && ...)
    Section::Section(std::wstring_view key, Args&&... args)
        :
        key{ key },
        children{ Tree::makeNodeVector<Rt::Section>(std::forward<Args>(args)...) },
        handlers{ Tree::postCreateOf<Rt::Section>(Tree::asHandler(args)...) }
    {
    }

    template <typename First, typename... Rest>
        requires (!IsSectionKey<First>)
              && Tree::NotSelfCopy<Section, First, Rest...>
              && IsSchemaArg<First> && (IsSchemaArg<Rest> && ...)
    Section::Section(First&& first, Rest&&... rest)
        :
        key{},
        children{ Tree::makeNodeVector<Rt::Section>(std::forward<First>(first), std::forward<Rest>(rest)...) },
        handlers{ Tree::postCreateOf<Rt::Section>(Tree::asHandler(first), Tree::asHandler(rest)...) }
    {
    }

    void Section::apply(Rt::Section& parent) const
    {
        if (key.empty())
        {
            // Anonymous: no section of its own, so children and handlers alike
            // belong to the parent.
            handlers(parent);
            for (const NodePtr& child : children)
            {
                child->apply(parent);
            }
        }
        else
        {
            Rt::Section& domSection = parent.addSection(key);
            handlers(domSection);
            for (const NodePtr& child : children)
            {
                child->apply(domSection);
            }
        }
    }

    void Section::append(Section&& other)
    {
        other.extractTo(children);
        handlers.merge(std::move(other.handlers));
    }

    void Section::extractTo(NodeList& dest)
    {
        dest.reserve(dest.size() + children.size());
        for (NodePtr& child : children)
        {
            dest.push_back(std::move(child));
        }
        children.clear();
    }

    // --- Value ---

    template <typename T>
    template <typename U, typename... Handlers>
        requires (IsHandlerArg<Handlers> && ...)
    Value<T>::Value(std::wstring_view key, U&& defaultValue, Handlers&&... eventHandlers)
        :
        key{ key },
        defaultValue{ std::forward<U>(defaultValue) },
        handlers{ Tree::postCreateOf<DomValue>(Tree::asHandler(eventHandlers)...) }
    {
    }

    template <typename T>
    void Value<T>::apply(Dom::Section& parent) const
    {
        DomValue& domNode = parent.addValue(key, defaultValue);
        handlers(domNode);
    }

    // --- Sequence ---

    template <typename T>
    template <typename U, typename... Handlers>
        requires (IsHandlerArg<Handlers> && ...)
    Sequence<T>::Sequence(std::wstring_view key, U&& itemBlueprint, Handlers&&... eventHandlers)
        :
        key{ key },
        itemBlueprint{ std::forward<U>(itemBlueprint) },
        handlers{ Tree::postCreateOf<DomSequence>(Tree::asHandler(eventHandlers)...) }
    {
    }

    // --- Sequence<Section> ---

    template <typename... Handlers>
        requires (IsHandlerArg<Handlers> && ...)
    Sequence<Section>::Sequence(std::wstring_view key, const Section& childSchema, Handlers&&... eventHandlers)
        :
        key{ key },
        ownChildSchema{ nullptr },
        childSchema{ &childSchema },
        handlers{ Tree::postCreateOf<DomSequence>(Tree::asHandler(eventHandlers)...) }
    {
    }

    template <typename... Handlers>
        requires (IsHandlerArg<Handlers> && ...)
    Sequence<Section>::Sequence(std::wstring_view key, Section&& childSchema, Handlers&&... eventHandlers)
        :
        key{ key },
        ownChildSchema{ std::make_unique<Section>(std::move(childSchema)) },
        childSchema{ ownChildSchema.get() },
        handlers{ Tree::postCreateOf<DomSequence>(Tree::asHandler(eventHandlers)...) }
    {
    }

    Sequence<Section>::Sequence(Sequence&& other) noexcept
        :
        key{ std::move(other.key) },
        ownChildSchema{ std::move(other.ownChildSchema) },
        childSchema{ ownChildSchema ? ownChildSchema.get() : other.childSchema },
        handlers{ std::move(other.handlers) }
    {
        other.childSchema = nullptr;
    }

    Sequence<Section>& Sequence<Section>::operator=(Sequence&& other) noexcept
    {
        if (this != &other)
        {
            key = std::move(other.key);
            ownChildSchema = std::move(other.ownChildSchema);
            childSchema = ownChildSchema ? ownChildSchema.get() : other.childSchema;
            handlers = std::move(other.handlers);
            other.childSchema = nullptr;
        }
        return *this;
    }

    template <typename T>
    void Sequence<T>::apply(Rt::Section& parent) const
    {
        DomSequence& domNode = parent.addSequence(key, itemBlueprint);
        handlers(domNode);
    }

    void Sequence<Section>::apply(Rt::Section& parent) const
    {
        Rt::Section templateSection{ nullptr };
        childSchema->apply(templateSection);
        DomSequence& domNode = parent.addSequence<Rt::Section>(key, std::move(templateSection));
        handlers(domNode);
    }
}
