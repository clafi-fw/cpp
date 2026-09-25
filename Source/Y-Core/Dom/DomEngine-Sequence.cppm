export module ClaFi.Core.DomEngine :Sequence;

import :Core;
import :Section;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    export class SequenceBase : public DomNodeBase
    {
    public:
        using DomNodeBase::DomNodeBase;

        SequenceBase(const SequenceBase& other);
        SequenceBase& operator=(const SequenceBase& other);
        SequenceBase(SequenceBase&& other) noexcept = default;
        SequenceBase& operator=(SequenceBase&& other) noexcept = default;

        [[nodiscard]] DomNodeType type() const noexcept override { return DomNodeType::Sequence; }
        [[nodiscard]] std::size_t size() const noexcept { return m_children.size(); }
        [[nodiscard]] DomNodeBase* child(std::size_t index) { return m_children[index].get(); }
        [[nodiscard]] const DomNodeBase* child(std::size_t index) const { return m_children[index].get(); }
        void forEachChild(const std::function<void(DomNodeBase&)>& action) override;

        std::vector<std::unique_ptr<DomNodeBase>>& children() { return m_children; }
        void clear() { m_children.clear(); }
        virtual DomNodeBase& addNode() = 0;
        void removeChild(DomNodeBase* child) override;

        void assign(const DomNodeBase& source) override;
        void merge(const DomNodeBase& source) override;

        template <typename T>
        [[nodiscard]] DomNodeBase* childWhere(const T& targetValue);

        template <typename T>
        [[nodiscard]] const DomNodeBase* childWhere(const T& targetValue) const;

        template <typename T>
        [[nodiscard]] DomNodeBase* childWhere(std::wstring_view key, const T& targetValue);

        template <typename T>
        [[nodiscard]] const DomNodeBase* childWhere(std::wstring_view key, const T& targetValue) const;

        template <typename T>
        [[nodiscard]] DomNodeBase* childWhere(std::initializer_list<std::wstring_view> path, const T& targetValue);

        template <typename T>
        [[nodiscard]] const DomNodeBase* childWhere(std::initializer_list<std::wstring_view> path, const T& targetValue) const;

        template <typename T>
        [[nodiscard]] DomNodeBase* childWhere(std::span<const std::wstring_view> path, const T& targetValue);

        template <typename T>
        [[nodiscard]] const DomNodeBase* childWhere(std::span<const std::wstring_view> path, const T& targetValue) const;

        // Predicate form — closer to SQL's actual WHERE (an arbitrary boolean
        // expression over a row), rather than the path+equality shorthand
        // above. The predicate receives the whole child and decides for
        // itself, so there's no separate path/key/initializer_list variant
        // here: a predicate can already do any of that navigation internally.
        template <typename Predicate>
            requires std::is_invocable_r_v<bool, Predicate, const DomNodeBase&>
        [[nodiscard]] DomNodeBase* childWhere(Predicate predicate);

        template <typename Predicate>
            requires std::is_invocable_r_v<bool, Predicate, const DomNodeBase&>
        [[nodiscard]] const DomNodeBase* childWhere(Predicate predicate) const;

        const DomNodeBase& operator[](std::size_t index) const;
        DomNodeBase& operator[](std::size_t index);

    private:
        std::vector<std::unique_ptr<DomNodeBase>> m_children;
    };

    export template <typename ItemValueT>
        class Sequence : public SequenceBase
    {
    public:
        using ChildNodeType = dom_node_t<ItemValueT>;

        // Plain random-access iterators over the existing child nodes —
        // dereferencing calls operator[] above, the same zero-copy path.
        // Nothing here ever constructs or clones an ItemValueT; a range-for
        // over a Sequence<Section> visits the live child Sections directly.
        class const_iterator
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = ChildNodeType;
            using difference_type = std::ptrdiff_t;
            using pointer = const ChildNodeType*;
            using reference = const ChildNodeType&;

            const_iterator() = default;
            const_iterator(const Sequence* seq, std::size_t index) : m_seq{ seq }, m_index{ index } {}

            reference operator*() const { return (*m_seq)[m_index]; }
            pointer operator->() const { return &(*m_seq)[m_index]; }

            const_iterator& operator++() { ++m_index; return *this; }
            const_iterator operator++(int) { auto tmp = *this; ++m_index; return tmp; }
            const_iterator& operator--() { --m_index; return *this; }
            const_iterator operator--(int) { auto tmp = *this; --m_index; return tmp; }

            [[nodiscard]] bool operator==(const const_iterator& other) const
            {
                return m_seq == other.m_seq && m_index == other.m_index;
            }

        private:
            const Sequence* m_seq{};
            std::size_t m_index{};
        };

        class iterator
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = ChildNodeType;
            using difference_type = std::ptrdiff_t;
            using pointer = ChildNodeType*;
            using reference = ChildNodeType&;

            iterator() = default;
            iterator(Sequence* seq, std::size_t index) : m_seq{ seq }, m_index{ index } {}

            reference operator*() const { return (*m_seq)[m_index]; }
            pointer operator->() const { return &(*m_seq)[m_index]; }

            iterator& operator++() { ++m_index; return *this; }
            iterator operator++(int) { auto tmp = *this; ++m_index; return tmp; }
            iterator& operator--() { --m_index; return *this; }
            iterator operator--(int) { auto tmp = *this; --m_index; return tmp; }

            [[nodiscard]] bool operator==(const iterator& other) const
            {
                return m_seq == other.m_seq && m_index == other.m_index;
            }

        private:
            Sequence* m_seq{};
            std::size_t m_index{};
        };

    public:
        Sequence(DomNodeBase* parent, const ItemValueT& defaultItemValue)
            :
            SequenceBase{ parent },
            m_defaultItemValue{ defaultItemValue }
        {
        }

    public:
        [[nodiscard]] const ItemValueT& defaultItemValue() const
        {
            return m_defaultItemValue;
        }

        DomNodeBase& addNode() override
        {
            return add();
        }

        ChildNodeType& add()
        {
            auto node = makeChildNode();
            auto& ref = static_cast<ChildNodeType&>(*node);
            children().push_back(std::move(node));
            return ref;
        }

        [[nodiscard]] auto get() const
        {
            return std::views::iota(std::size_t{ 0 }, this->size())
                | std::views::transform([this](std::size_t index) -> ItemValueT {
                    return extractItemValue(*this->child(index));
                    });
        }

        template <typename RangeT>
        void getTo(RangeT& out) const requires IsSequenceValue<RangeT>
        {
            using InnerItemT = std::ranges::range_value_t<dom_storage_t<RangeT>>;

            if constexpr (requires { out.resize(std::size_t{}); out[0]; })
            {
                out.resize(this->size());
                for (std::size_t i = 0; i < this->size(); ++i)
                {
                    this->child(i)->getTo(out[i]);
                }
            }
            else if constexpr (requires { out.push_back(std::declval<InnerItemT>()); } ||
                requires { out.insert(out.end(), std::declval<InnerItemT>()); })
            {
                out.clear();
                for (std::size_t i = 0; i < this->size(); ++i)
                {
                    InnerItemT item{};
                    this->child(i)->getTo(item);
                    if constexpr (requires { out.push_back(std::move(item)); })
                    {
                        out.push_back(std::move(item));
                    }
                    else
                    {
                        out.insert(out.end(), std::move(item));
                    }
                }
            }
            else
            {
                // Fixed-size container with no resize/push_back/insert
                // (e.g. std::array<T, N>): assign by index instead, up to
                // whichever is smaller -- the container's own fixed
                // capacity, or how many items the sequence actually has.
                // Any slots beyond that keep whatever value they already
                // held.
                const std::size_t count = out.size() < this->size() ? out.size() : this->size();
                for (std::size_t i = 0; i < count; ++i)
                {
                    this->child(i)->getTo(out[i]);
                }
            }
        }

        template <typename RangeT>
        void set(const RangeT& value) requires IsSequenceValue<RangeT>
        {
            clear();
            for (const auto& item : value)
            {
                assignItemValue(addNode(), item);
            }
        }

        [[nodiscard]] std::unique_ptr<DomNodeBase> clone(DomNodeBase* parent) const override
        {
            auto copy = std::make_unique<Sequence<ItemValueT>>(parent, m_defaultItemValue);
            for (std::size_t i = 0; i < size(); ++i)
            {
                copy->children().push_back(child(i)->clone(copy.get()));
            }
            return copy;
        }

        template <typename T>
        [[nodiscard]] ChildNodeType* childWhere(const T& targetValue)
        {
            auto* res = SequenceBase::childWhere(targetValue);
            return res ? &res->template as<ChildNodeType>() : nullptr;
        }

        template <typename T>
        [[nodiscard]] const ChildNodeType* childWhere(const T& targetValue) const
        {
            return const_cast<Sequence*>(this)->childWhere(targetValue);
        }

        template <typename T>
        [[nodiscard]] ChildNodeType* childWhere(std::wstring_view key, const T& targetValue)
        {
            auto* res = SequenceBase::childWhere(key, targetValue);
            return res ? &res->template as<ChildNodeType>() : nullptr;
        }

        template <typename T>
        [[nodiscard]] const ChildNodeType* childWhere(std::wstring_view key, const T& targetValue) const
        {
            return const_cast<Sequence*>(this)->childWhere(key, targetValue);
        }

        template <typename T>
        [[nodiscard]] ChildNodeType* childWhere(std::initializer_list<std::wstring_view> path, const T& targetValue)
        {
            auto* res = SequenceBase::childWhere(path, targetValue);
            return res ? &res->template as<ChildNodeType>() : nullptr;
        }

        template <typename T>
        [[nodiscard]] const ChildNodeType* childWhere(std::initializer_list<std::wstring_view> path, const T& targetValue) const
        {
            return const_cast<Sequence*>(this)->childWhere(path, targetValue);
        }

        // Sole real implementation: every const overload above and below
        // delegates here via const_cast rather than re-deriving the same
        // "call SequenceBase, downcast the result" logic.
        template <typename T>
        [[nodiscard]] ChildNodeType* childWhere(std::span<const std::wstring_view> path, const T& targetValue)
        {
            auto* res = SequenceBase::childWhere(path, targetValue);
            return res ? &res->template as<ChildNodeType>() : nullptr;
        }

        template <typename T>
        [[nodiscard]] const ChildNodeType* childWhere(std::span<const std::wstring_view> path, const T& targetValue) const
        {
            return const_cast<Sequence*>(this)->childWhere(path, targetValue);
        }

        // Predicate form, typed: the predicate receives the actual
        // ChildNodeType (e.g. const Section&, not const NodeBase&), so no
        // manual .as<T>() cast is needed inside the lambda. Forwards to
        // SequenceBase's generic predicate overload underneath.
        template <typename Predicate>
            requires std::is_invocable_r_v<bool, Predicate, const ChildNodeType&>
        [[nodiscard]] ChildNodeType* childWhere(Predicate predicate)
        {
            auto* res = SequenceBase::childWhere([&predicate](const DomNodeBase& node)
                {
                    return predicate(node.template as<ChildNodeType>());
                });
            return res ? &res->template as<ChildNodeType>() : nullptr;
        }

        template <typename Predicate>
            requires std::is_invocable_r_v<bool, Predicate, const ChildNodeType&>
        [[nodiscard]] const ChildNodeType* childWhere(Predicate predicate) const
        {
            return const_cast<Sequence*>(this)->childWhere(predicate);
        }

        const ChildNodeType& operator[](std::size_t index) const
        {
            return SequenceBase::operator[](index).template as<ChildNodeType>();
        }

        ChildNodeType& operator[](std::size_t index)
        {
            return SequenceBase::operator[](index).template as<ChildNodeType>();
        }

        [[nodiscard]] const_iterator begin() const { return const_iterator{ this, 0 }; }
        [[nodiscard]] const_iterator end() const { return const_iterator{ this, size() }; }
        [[nodiscard]] const_iterator cbegin() const { return const_iterator{ this, 0 }; }
        [[nodiscard]] const_iterator cend() const { return const_iterator{ this, size() }; }
        [[nodiscard]] iterator begin() { return iterator{ this, 0 }; }
        [[nodiscard]] iterator end() { return iterator{ this, size() }; }

    private:
        // Single source of truth for how an ItemValueT maps onto a concrete child
        // node type — Section, scalar-or-composite Value, or nested Sequence.
        // add(), get() and set() all route through here, so the mapping is stated
        // once and cannot drift between them.

        [[nodiscard]] std::unique_ptr<DomNodeBase> makeChildNode()
        {
            if constexpr (std::is_same_v<ItemValueT, Section>)
            {
                auto node = std::make_unique<Section>(this);
                node->assign(m_defaultItemValue);
                return node;
            }
            else if constexpr (IsScalarValue<ItemValueT> || IsCompositeValue<ItemValueT>)
            {
                return std::make_unique<Value<ItemValueT>>(this, m_defaultItemValue);
            }
            else if constexpr (IsSequenceValue<ItemValueT>)
            {
                using InnerItemT = std::ranges::range_value_t<dom_storage_t<ItemValueT>>;

                InnerItemT defaultInnerItemValue{};
                auto it = std::begin(m_defaultItemValue);
                if (it != std::end(m_defaultItemValue))
                {
                    defaultInnerItemValue = *it;
                }

                return std::make_unique<Sequence<InnerItemT>>(this, defaultInnerItemValue);
            }
        }

        [[nodiscard]] static ItemValueT extractItemValue(const DomNodeBase& node)
        {
            if constexpr (std::is_same_v<ItemValueT, Section>)
            {
                Section copy{ nullptr };
                copy.assign(node);
                return copy;
            }
            else if constexpr (IsScalarValue<ItemValueT> || IsCompositeValue<ItemValueT>)
            {
                return static_cast<const Value<ItemValueT>&>(node).get();
            }
            else if constexpr (IsSequenceValue<ItemValueT>)
            {
                return node.template get<ItemValueT>();
            }
            else
            {
                return ItemValueT{};
            }
        }

        static void assignItemValue(DomNodeBase& node, const ItemValueT& value)
        {
            if constexpr (std::is_same_v<ItemValueT, Section>)
            {
                static_cast<Section&>(node).assign(value);
            }
            else if constexpr (IsScalarValue<ItemValueT> || IsCompositeValue<ItemValueT>)
            {
                static_cast<Value<ItemValueT>&>(node).set(value);
            }
            else if constexpr (IsSequenceValue<ItemValueT>)
            {
                using InnerItemT = std::ranges::range_value_t<dom_storage_t<ItemValueT>>;
                static_cast<Sequence<InnerItemT>&>(node).set(value);
            }
        }

    private:
        ItemValueT m_defaultItemValue;
    };
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom
{
    namespace detail
    {
        // Only used by SequenceBase::childWhere below. range_to_container stays
        // in :Core since NodeBase::get<T>() (also in :Core) needs it too.
        inline const DomNodeBase* traverseSafe(const DomNodeBase* node, std::span<const std::wstring_view> path) noexcept
        {
            const DomNodeBase* current = node;
            for (auto key : path)
            {
                if (!current)
                {
                    return nullptr;
                }
                if (current->type() == DomNodeType::Section || current->type() == DomNodeType::CompositeValue)
                {
                    current = static_cast<const Section*>(current)->child(key);
                }
                else
                {
                    return nullptr;
                }
            }
            return current;
        }
    }

    // --- SequenceBase ---

    SequenceBase::SequenceBase(const SequenceBase& other)
        :
        DomNodeBase{ other }
    {
        for (std::size_t i = 0; i < other.size(); ++i)
        {
            m_children.push_back(other.child(i)->clone(this));
        }
    }

    SequenceBase& SequenceBase::operator=(const SequenceBase& other)
    {
        if (this != &other)
        {
            DomNodeBase::operator=(other);
            assign(other);
        }
        return *this;
    }

    void SequenceBase::forEachChild(const std::function<void(DomNodeBase&)>& action)
    {
        for (const auto& child : m_children)
        {
            if (child)
            {
                action(*child);
            }
        }
    }

    // --- SequenceBase::removeChild ---
    // deleteSelf() itself moved back to :Core — it now just calls
    // m_parent->removeChild(this) virtually, no downcast needed. This
    // override is where actual removal happens for sequence children.

    void SequenceBase::removeChild(DomNodeBase* child)
    {
        std::erase_if(m_children, [child](const auto& c) { return c.get() == child; });
        emitChange();
    }

    void SequenceBase::assign(const DomNodeBase& source)
    {
        clear();
        merge(source);
    }

    void SequenceBase::merge(const DomNodeBase& source)
    {
        if (source.type() != DomNodeType::Sequence)
        {
            return;
        }

        auto& srcSeq = static_cast<const SequenceBase&>(source);
        clear();

        for (std::size_t i = 0; i < srcSeq.size(); ++i)
        {
            const DomNodeBase* srcChild = srcSeq.child(i);
            DomNodeBase& dstChild = addNode();
            dstChild.merge(*srcChild);
        }
    }

    template <typename T>
    DomNodeBase* SequenceBase::childWhere(const T& targetValue)
    {
        return childWhere(std::span<const std::wstring_view>{}, targetValue);
    }

    template <typename T>
    const DomNodeBase* SequenceBase::childWhere(const T& targetValue) const
    {
        return const_cast<SequenceBase*>(this)->childWhere(targetValue);
    }

    template <typename T>
    DomNodeBase* SequenceBase::childWhere(std::wstring_view key, const T& targetValue)
    {
        const std::wstring_view path[] = { key };
        return childWhere(std::span<const std::wstring_view>{ path }, targetValue);
    }

    template <typename T>
    const DomNodeBase* SequenceBase::childWhere(std::wstring_view key, const T& targetValue) const
    {
        return const_cast<SequenceBase*>(this)->childWhere(key, targetValue);
    }

    template <typename T>
    DomNodeBase* SequenceBase::childWhere(std::initializer_list<std::wstring_view> path, const T& targetValue)
    {
        return childWhere(std::span<const std::wstring_view>{ path.begin(), path.end() }, targetValue);
    }

    template <typename T>
    const DomNodeBase* SequenceBase::childWhere(std::initializer_list<std::wstring_view> path, const T& targetValue) const
    {
        return const_cast<SequenceBase*>(this)->childWhere(path, targetValue);
    }

    // Sole real implementation of the search: walks each child, resolves the
    // (possibly empty) path under it, and compares the leaf's value. Every
    // other childWhere overload — non-const and const alike — funnels into this
    // one through the delegations above/below.
    template <typename T>
    DomNodeBase* SequenceBase::childWhere(std::span<const std::wstring_view> path, const T& targetValue)
    {
        using LogicalT = dom_storage_t<T>;
        for (auto& child : children())
        {
            const DomNodeBase* leaf = detail::traverseSafe(child.get(), path);
            if (leaf && leaf->get<LogicalT>() == targetValue)
            {
                return child.get();
            }
        }
        return nullptr;
    }

    template <typename T>
    const DomNodeBase* SequenceBase::childWhere(std::span<const std::wstring_view> path, const T& targetValue) const
    {
        return const_cast<SequenceBase*>(this)->childWhere(path, targetValue);
    }

    template <typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate, const DomNodeBase&>
    DomNodeBase* SequenceBase::childWhere(Predicate predicate)
    {
        for (auto& child : children())
        {
            if (predicate(*child))
            {
                return child.get();
            }
        }
        return nullptr;
    }

    template <typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate, const DomNodeBase&>
    const DomNodeBase* SequenceBase::childWhere(Predicate predicate) const
    {
        return const_cast<SequenceBase*>(this)->childWhere(predicate);
    }

    const DomNodeBase& SequenceBase::operator[](std::size_t index) const
    {
        if (index >= size())
        {
            throw std::out_of_range("Dom: Sequence index out of bounds.");
        }
        return *child(index);
    }

    DomNodeBase& SequenceBase::operator[](std::size_t index)
    {
        if (index >= size())
        {
            throw std::out_of_range("Dom: Sequence index out of bounds.");
        }
        return *child(index);
    }
}
