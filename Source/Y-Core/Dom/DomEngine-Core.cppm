export module ClaFi.Core.DomEngine :Core;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Serialization;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    export class Section;

    // What one node of a document is.
    export enum class DomNodeType
    {
        ScalarValue,
        CompositeValue,
        Section,
        Sequence
    };

    export struct StringViewHash
    {
        using is_transparent = void;
        [[nodiscard]] std::size_t operator()(std::wstring_view str) const noexcept
        {
            return std::hash<std::wstring_view>{}(str);
        }
    };

    export template <typename Class, typename Type, typename Getter = Type Class::*, typename Setter = std::nullptr_t>
        struct SerializedField
    {
        using ClassType = Class;
        using ValueType = Type;
        using GetterType = Getter;
        using SetterType = Setter;

        std::wstring_view name{};
        Getter member{};
        Setter setter{};
    };

    // CTAD Deduction Guides for backward-compatible member pointers
    template <typename Class, typename Type>
    SerializedField(std::wstring_view, Type Class::*)
        -> SerializedField<Class, Type, Type Class::*, std::nullptr_t>;

    // CTAD Deduction Guides for getters and setters
    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, Type(Class::*)() const, Setter)
        -> SerializedField<Class, Type, Type(Class::*)() const, Setter>;

    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, Type(Class::*)(), Setter)
        -> SerializedField<Class, Type, Type(Class::*)(), Setter>;

    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, const Type& (Class::*)() const, Setter)
        -> SerializedField<Class, Type, const Type& (Class::*)() const, Setter>;

    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, Type& (Class::*)(), Setter)
        -> SerializedField<Class, Type, Type& (Class::*)(), Setter>;

    export template <typename T>
        struct ScalarSerializer;

    export template <typename T>
        struct CompositeSerializer;

    template <typename T>
    concept HasNestedStorageType = requires
    {
        typename ScalarSerializer<std::decay_t<T>>::StorageType;
    };

    template <typename T, typename = void>
    struct DomStorageTypeImpl
    {
        using type = std::decay_t<T>;
    };

    template <HasNestedStorageType T>
    struct DomStorageTypeImpl<T, void>
    {
        using type = ScalarSerializer<std::decay_t<T>>::StorageType;
    };

    export template <typename T>
        using dom_storage_t = DomStorageTypeImpl<T>::type;

    export template <typename T>
        concept IsScalarValue = requires(const dom_storage_t<T>&v, std::wstring_view sv, dom_storage_t<T>&out)
    {
        { ScalarSerializer<dom_storage_t<T>>::toWString(v) } -> std::same_as<std::wstring>;
        { ScalarSerializer<dom_storage_t<T>>::fromWString(sv, out) } -> std::same_as<void>;
    };

    export template <typename T>
        concept IsSequenceValue = !IsScalarValue<T> && std::ranges::range<dom_storage_t<T>>;

    export template <typename T>
        concept IsCompositeValue = !std::is_same_v<dom_storage_t<T>, Section> && !IsScalarValue<T> && !IsSequenceValue<T>&& std::is_class_v<dom_storage_t<T>>;

    export template <typename ValueT> class Value;
    export template <typename ItemValueT> class Sequence;

    template <typename T>
    struct DomNodeResolver;

    template <typename T>
        requires std::is_same_v<T, Section>
    struct DomNodeResolver<T>
    {
        using type = Section;
    };

    template <typename T>
        requires (!std::is_same_v<T, Section> && (IsScalarValue<T> || IsCompositeValue<T>))
    struct DomNodeResolver<T>
    {
        using type = Value<T>;
    };

    template <IsSequenceValue T>
    struct DomNodeResolver<T>
    {
        using type = Sequence<std::ranges::range_value_t<dom_storage_t<T>>>;
    };

    export template <typename T>
        using dom_node_t = DomNodeResolver<T>::type;

    export class DomNodeBase;

    export class ChangeEvent : public EventOf<DomNodeBase>
    {
    public:
        explicit ChangeEvent(DomNodeBase& sender);
    };

    export class NestedChangeEvent : public EventOf<DomNodeBase>
    {
    public:
        explicit NestedChangeEvent(DomNodeBase& sender);
    };

    export class SchemaErrorEvent : public EventOf<DomNodeBase>
    {
    public:
        SchemaErrorEvent(DomNodeBase& sender, DomNodeBase& origin, std::wstring_view errorText);

        [[nodiscard]] DomNodeBase& origin() const { return m_origin; }
        [[nodiscard]] std::wstring_view errorText() const { return m_errorText; }

    private:
        DomNodeBase& m_origin;
        std::wstring m_errorText;
    };

    export class Transaction
    {
    public:
        explicit Transaction(DomNodeBase& node);
        ~Transaction();

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        Transaction(Transaction&& other) noexcept;
        Transaction& operator=(Transaction&& other) noexcept;

    private:
        DomNodeBase* m_target;
    };

    export class DomNodeBase : public EventComponent
    {
        friend Transaction;

    public:
        DomNodeBase() = default;
        explicit DomNodeBase(DomNodeBase* parent);
        virtual ~DomNodeBase() = default;

        DomNodeBase(const DomNodeBase& other);
        DomNodeBase& operator=(const DomNodeBase& other);
        DomNodeBase(DomNodeBase&& other) noexcept;
        DomNodeBase& operator=(DomNodeBase&& other) noexcept;

    public:
        [[nodiscard]] virtual DomNodeType type() const noexcept = 0;
        virtual void assign(const DomNodeBase& source) = 0;
        virtual void merge(const DomNodeBase& source) = 0;
        [[nodiscard]] virtual std::unique_ptr<DomNodeBase> clone(DomNodeBase* parent) const = 0;

        [[nodiscard]] DomNodeBase* parent() const { return m_parent; }
        void setParent(DomNodeBase* parent) { m_parent = parent; }
        [[nodiscard]] DomNodeBase* root() noexcept;
        virtual void forEachChild(const std::function<void(DomNodeBase&)>&) {}

        void emitSchemaError(DomNodeBase& origin, std::wstring_view errorText);

        virtual const DomNodeBase& operator/(std::wstring_view key) const;
        virtual DomNodeBase& operator/(std::wstring_view key);

        template <typename T>
        dom_storage_t<T> get() const;

        template <typename T>
        void getTo(T& out) const;

        template <typename T>
        void set(T&& value);

        template <typename T>
        [[nodiscard]] T& as() { return static_cast<T&>(*this); }

        template <typename T>
        [[nodiscard]] const T& as() const { return static_cast<const T&>(*this); }

        void deleteSelf();

        // Called by a child's deleteSelf() on its parent. Default throws —
        // schema-driven containers (Section, CompositeValue) can't have
        // fields removed. SequenceBase overrides this to actually remove
        // the child, since sequences are the one node kind where deletion
        // makes sense.
        virtual void removeChild(DomNodeBase* child);

        void emitChange();
        [[nodiscard]] Transaction startTransaction();
        [[nodiscard]] bool inTransaction() const noexcept;

    private:
        void incrementTransactionDepth() noexcept { m_transactionDepth++; }
        void decrementTransactionDepth() noexcept;
        void bubbleNestedChange();
        void flushNestedChanges(bool isBoundary);

    private:
        DomNodeBase* m_parent{};
        std::uint32_t m_transactionDepth{ 0 };
        bool m_pendingNestedChange{ false };
    };

    export class ScalarValueBase : public DomNodeBase
    {
    public:
        using DomNodeBase::DomNodeBase;

        ScalarValueBase(const ScalarValueBase& other) = default;
        ScalarValueBase& operator=(const ScalarValueBase& other) = default;
        ScalarValueBase(ScalarValueBase&& other) noexcept = default;
        ScalarValueBase& operator=(ScalarValueBase&& other) noexcept = default;

        [[nodiscard]] DomNodeType type() const noexcept final { return DomNodeType::ScalarValue; }

        virtual void setFromRaw(std::wstring_view raw) = 0;
        [[nodiscard]] virtual std::wstring getAsRaw() const = 0;

        void assign(const DomNodeBase& source) override;
        void merge(const DomNodeBase& source) override;
    };

    // Section moved to ClaFi.Core.Dom.Classes:Section

    // SequenceBase moved to ClaFi.Core.Dom.Classes:Sequence

    // Value<T> (both the scalar/ScalarValueBase and composite/Section
    // specializations) moved to ClaFi.Core.Dom.Classes:Value

    // Sequence<ItemValueT> moved to ClaFi.Core.Dom.Classes:Sequence

    // StructSerializerUtils and CompositeSerializer moved to
    // ClaFi.Core.Dom.Classes:Serialization
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom
{
    namespace detail
    {
        template <typename Container, typename Range>
        Container range_to_container(Range&& r)
        {
            Container c = Container{};
            using ElemT = std::ranges::range_value_t<std::remove_reference_t<Range>>;

            if constexpr (requires { c.push_back(std::declval<ElemT>()); } ||
                requires { c.insert(c.end(), std::declval<ElemT>()); })
            {
                if constexpr (requires { c.reserve(std::size_t{}); })
                {
                    if constexpr (requires { std::ranges::size(r); })
                    {
                        c.reserve(std::ranges::size(r));
                    }
                }
                for (auto&& elem : r)
                {
                    if constexpr (requires { c.push_back(elem); })
                    {
                        c.push_back(std::forward<decltype(elem)>(elem));
                    }
                    else
                    {
                        c.insert(c.end(), std::forward<decltype(elem)>(elem));
                    }
                }
            }
            else
            {
                // Fixed-size container (e.g. std::array<T, N>): it can't
                // grow, so assign by index instead, up to whichever is
                // smaller -- the container's own fixed capacity, or how
                // many elements the range actually has. Any slots beyond
                // that keep their default value.
                std::size_t i = 0;
                for (auto&& elem : r)
                {
                    if (i >= c.size())
                    {
                        break;
                    }
                    c[i] = std::forward<decltype(elem)>(elem);
                    ++i;
                }
            }
            return c;
        }

        // traverseSafe moved to ClaFi.Core.Dom.Classes:Sequence
        // (only used by SequenceBase::childWhere, which lives there now)
    }

    // --- ChangeEvent ---

    ChangeEvent::ChangeEvent(DomNodeBase& sender)
        :
        EventOf<DomNodeBase>{ sender }
    {
    }

    // --- NestedChangeEvent ---

    NestedChangeEvent::NestedChangeEvent(DomNodeBase& sender)
        :
        EventOf<DomNodeBase>{ sender }
    {
    }

    // --- SchemaErrorEvent ---

    SchemaErrorEvent::SchemaErrorEvent(DomNodeBase& sender, DomNodeBase& origin, std::wstring_view errorText)
        :
        EventOf<DomNodeBase>{ sender },
        m_origin{ origin },
        m_errorText{ errorText }
    {
    }

    // --- Transaction ---

    Transaction::Transaction(DomNodeBase& node)
        :
        m_target{ &node }
    {
        m_target->incrementTransactionDepth();
    }

    Transaction::~Transaction()
    {
        if (m_target)
        {
            m_target->decrementTransactionDepth();
        }
    }

    Transaction::Transaction(Transaction&& other) noexcept
        :
        m_target{ other.m_target }
    {
        other.m_target = nullptr;
    }

    Transaction& Transaction::operator=(Transaction&& other) noexcept
    {
        if (this != &other)
        {
            if (m_target)
            {
                m_target->decrementTransactionDepth();
            }
            m_target = other.m_target;
            other.m_target = nullptr;
        }
        return *this;
    }

    // --- NodeBase ---

    DomNodeBase::DomNodeBase(DomNodeBase* parent)
        :
        m_parent{ parent }
    {
    }

    // *** Copy/move operators and ctors below drop other intentionally
    // - only events ids and subscribers are there, which are not needed in the new node
    // and the actual data is handled correctly in derived classes

    DomNodeBase::DomNodeBase(const DomNodeBase&)
        :
        EventComponent{}
    {
        // *** see comment above
    }

    DomNodeBase& DomNodeBase::operator=(const DomNodeBase&)
    {
        // *** see comment above
        return *this;
    }

    DomNodeBase::DomNodeBase(DomNodeBase&&) noexcept
        :
        EventComponent{}
    {
        // *** see comment above
    }

    DomNodeBase& DomNodeBase::operator=(DomNodeBase&&) noexcept
    {
        // *** see comment above
        return *this;
    }

    DomNodeBase* DomNodeBase::root() noexcept
    {
        DomNodeBase* current = this;
        while (current->m_parent)
        {
            current = current->m_parent;
        }
        return current;
    }

    void DomNodeBase::emitSchemaError(DomNodeBase& origin, std::wstring_view errorText)
    {
        SchemaErrorEvent event = SchemaErrorEvent{ *this, origin, errorText };
        this->emitEvent(event);
        if (!event.propagationStopped() && m_parent)
        {
            m_parent->emitSchemaError(origin, errorText);
        }
    }

    // Section (and Value<CompositeValue>, which inherits it) already override
    // operator/ virtually — see :Section and the Value<CompositeValue>
    // inheritance in :Value. Any node whose type() is Section or
    // CompositeValue is therefore guaranteed to BE one of those, and a call
    // to operator/ on it dispatches straight to that override, never
    // reaching this base body. So this body only ever runs for node kinds that
    // don't support path traversal, and for those it should simply refuse.
    const DomNodeBase& DomNodeBase::operator/(std::wstring_view /*key*/) const
    {
        throw std::runtime_error("Dom: Cannot traverse path through a non-composite node.");
    }

    DomNodeBase& DomNodeBase::operator/(std::wstring_view /*key*/)
    {
        throw std::runtime_error("Dom: Cannot traverse path through a non-composite node.");
    }
    template <typename T>
    dom_storage_t<T> DomNodeBase::get() const
    {
        using StorageT = dom_storage_t<T>;

        if constexpr (std::is_same_v<StorageT, Section>)
        {
            if (type() == DomNodeType::Section || type() == DomNodeType::CompositeValue)
            {
                StorageT s{ nullptr };
                s.assign(*this);
                return s;
            }
        }
        else if constexpr (IsScalarValue<StorageT> || IsCompositeValue<StorageT>)
        {
            if (type() == DomNodeType::ScalarValue || type() == DomNodeType::CompositeValue)
            {
                return static_cast<const Value<StorageT>&>(*this).get();
            }
        }
        else if constexpr (IsSequenceValue<StorageT>)
        {
            if (type() == DomNodeType::Sequence)
            {
                using ItemT = StorageT::value_type;
                auto& seqNode = static_cast<const Sequence<ItemT>&>(*this);
                return detail::range_to_container<StorageT>(seqNode.get());
            }
        }

        const_cast<DomNodeBase*>(this)->emitSchemaError(*const_cast<DomNodeBase*>(this), L"Schema mismatch: Cannot extract requested type using get().");
        return StorageT{};
    }

    template <typename T>
    void DomNodeBase::getTo(T& out) const
    {
        using StorageT = dom_storage_t<T>;

        if constexpr (std::is_same_v<StorageT, Section>)
        {
            if (type() == DomNodeType::Section || type() == DomNodeType::CompositeValue)
            {
                out.assign(*this);
                return;
            }
        }
        else if constexpr (IsScalarValue<StorageT> || IsCompositeValue<StorageT>)
        {
            if (type() == DomNodeType::ScalarValue || type() == DomNodeType::CompositeValue)
            {
                static_cast<const Value<StorageT>&>(*this).getTo(out);
                return;
            }
        }
        else if constexpr (IsSequenceValue<StorageT>)
        {
            if (type() == DomNodeType::Sequence)
            {
                using ItemT = std::ranges::range_value_t<StorageT>;
                auto& seqNode = static_cast<const Sequence<ItemT>&>(*this);
                seqNode.getTo(out);
                return;
            }
        }
        const_cast<DomNodeBase*>(this)->emitSchemaError(*const_cast<DomNodeBase*>(this), L"Schema mismatch: Cannot extract requested type using getTo().");
    }

    template <typename T>
    void DomNodeBase::set(T&& value)
    {
        using StorageT = dom_storage_t<T>;

        if constexpr (std::is_same_v<StorageT, Section>)
        {
            if (type() == DomNodeType::Section || type() == DomNodeType::CompositeValue)
            {
                this->assign(std::forward<T>(value));
                return;
            }
        }
        else if constexpr (IsScalarValue<StorageT> || IsCompositeValue<StorageT>)
        {
            if (type() == DomNodeType::ScalarValue || type() == DomNodeType::CompositeValue)
            {
                static_cast<Value<StorageT>&>(*this).set(std::forward<T>(value));
                return;
            }
        }
        else if constexpr (IsSequenceValue<StorageT>)
        {
            if (type() == DomNodeType::Sequence)
            {
                using ItemT = StorageT::value_type;
                static_cast<Sequence<ItemT>&>(*this).set(std::forward<T>(value));
                return;
            }
        }
        emitSchemaError(*this, L"Schema mismatch: Cannot set value. Type mismatch.");
    }

    void DomNodeBase::deleteSelf()
    {
        if (!m_parent)
            return;
        m_parent->removeChild(this);
    }

    void DomNodeBase::removeChild(DomNodeBase*)
    {
        // deleting a field from a schema-driven container makes no sense:
        throw std::logic_error(
            "Dom: Cannot delete a node that belongs to a Section or CompositeValue. "
            "Schema-driven fields are static."
        );
    }


    void DomNodeBase::emitChange()
    {
        ChangeEvent event = ChangeEvent{ *this };
        this->emitEvent(event);

        if (!event.propagationStopped() && m_parent)
        {
            m_parent->bubbleNestedChange();
        }
    }

    Transaction DomNodeBase::startTransaction()
    {
        return Transaction{ *this };
    }

    bool DomNodeBase::inTransaction() const noexcept
    {
        const DomNodeBase* current = this;
        while (current)
        {
            if (current->m_transactionDepth > 0)
            {
                return true;
            }
            current = current->m_parent;
        }
        return false;
    }

    void DomNodeBase::decrementTransactionDepth() noexcept
    {
        if (m_transactionDepth == 0)
        {
            return;
        }

        m_transactionDepth--;
        if (m_transactionDepth == 0)
        {
            if (!inTransaction() && m_pendingNestedChange)
            {
                this->flushNestedChanges(true);
            }
        }
    }

    void DomNodeBase::bubbleNestedChange()
    {
        if (m_transactionDepth > 0)
        {
            m_pendingNestedChange = true;
            return;
        }

        if (inTransaction())
        {
            m_pendingNestedChange = true;
            if (m_parent)
            {
                m_parent->bubbleNestedChange();
            }
            return;
        }

        m_pendingNestedChange = false;
        NestedChangeEvent event = NestedChangeEvent{ *this };
        this->emitEvent(event);

        if (m_parent)
        {
            m_parent->bubbleNestedChange();
        }
    }

    void DomNodeBase::flushNestedChanges(bool isBoundary)
    {
        if (!m_pendingNestedChange)
        {
            return;
        }

        forEachChild([](DomNodeBase& child)
            {
                child.flushNestedChanges(false);
            });

        m_pendingNestedChange = false;
        NestedChangeEvent event = NestedChangeEvent{ *this };
        this->emitEvent(event);

        if (isBoundary && m_parent && !event.propagationStopped())
        {
            m_parent->bubbleNestedChange();
        }
    }

    // --- ScalarValueBase ---

    void ScalarValueBase::assign(const DomNodeBase& source)
    {
        if (source.type() == DomNodeType::ScalarValue)
        {
            setFromRaw(static_cast<const ScalarValueBase&>(source).getAsRaw());
        }
    }

    void ScalarValueBase::merge(const DomNodeBase& source)
    {
        assign(source);
    }

    // Section implementations moved to ClaFi.Core.Dom.Classes:Section

    // SequenceBase implementations moved to ClaFi.Core.Dom.Classes:Sequence

    // --- CompositeSerializer ---
    // Implementations moved to ClaFi.Core.Dom.Classes:Serialization
}
