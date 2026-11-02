export module ClaFi.Core.DomEngine :MultiTypeSequence;

import :Core;
import :Section;
import :Value;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    // A list kept in the order written, each item named and read into its name's blueprint. See Dom
    export class MultiTypeSequence : public DomNodeBase
    {
    public:
        // One item of the list: the name it was written under, and the node it was read into.
        struct Item
        {
            std::wstring name{};
            std::unique_ptr<DomNodeBase> node{};
        };

        using Items = std::vector<Item>;

    public:
        using DomNodeBase::DomNodeBase;

        MultiTypeSequence(const MultiTypeSequence& other);
        MultiTypeSequence& operator=(const MultiTypeSequence& other);
        MultiTypeSequence(MultiTypeSequence&& other) noexcept;
        MultiTypeSequence& operator=(MultiTypeSequence&& other) noexcept;

    public:
        [[nodiscard]] DomNodeType type() const noexcept override
        {
            return DomNodeType::MultiTypeSequence;
        }
        void assign(const DomNodeBase& source) override;
        void merge(const DomNodeBase& source) override;
        [[nodiscard]] std::unique_ptr<DomNodeBase> clone(DomNodeBase* parent) const override;
        void forEachChild(const std::function<void(DomNodeBase&)>& action) override;
        void removeChild(DomNodeBase* child) override;

        // Names an item may carry, and what one holds before anything is stated - a value or a schema.
        template <typename ValueT>
            requires std::is_same_v<ValueT, Section> || IsScalarValue<ValueT> || IsCompositeValue<ValueT>
        void addBlueprint(std::wstring_view name, const ValueT& defaultValue);
        // The blueprint an item of that name is read into, or nothing for a name no blueprint has.
        [[nodiscard]] const DomNodeBase* blueprint(std::wstring_view name) const;
        // An item of that name before anything is stated about it, held by no list.
        [[nodiscard]] std::unique_ptr<DomNodeBase> makeDefaultItem(std::wstring_view name) const;

        [[nodiscard]] std::size_t size() const noexcept { return m_items.size(); }
        [[nodiscard]] const Items& items() const { return m_items; }
        // An item of that name added at the end, or nothing for a name no blueprint has.
        DomNodeBase* add(std::wstring_view name);
        // A node put at the end as it stands, whatever its name's blueprint says.
        void addItem(std::wstring_view name, std::unique_ptr<DomNodeBase> node);
        void clear() { m_items.clear(); }

    private:
        using Blueprints = std::unordered_map<std::wstring, std::unique_ptr<DomNodeBase>,
            StringViewHash, std::equal_to<>>;

    private:
        void copyItemsOf(const MultiTypeSequence& source);
        void adoptItems();

    private:
        Blueprints m_blueprints;
        Items m_items;
    };
}

// =========================================================================
// **Implementations**
// =========================================================================

namespace ClaFi::Dom
{
    // --- MultiTypeSequence ---

    MultiTypeSequence::MultiTypeSequence(const MultiTypeSequence& other)
        :
        DomNodeBase{ other }
    {
        for (const auto& [name, blueprint] : other.m_blueprints)
            m_blueprints[name] = blueprint->clone(nullptr);
        copyItemsOf(other);
    }

    MultiTypeSequence& MultiTypeSequence::operator=(const MultiTypeSequence& other)
    {
        if (this != &other)
        {
            DomNodeBase::operator=(other);
            m_blueprints.clear();
            for (const auto& [name, blueprint] : other.m_blueprints)
                m_blueprints[name] = blueprint->clone(nullptr);
            m_items.clear();
            copyItemsOf(other);
        }
        return *this;
    }

    MultiTypeSequence::MultiTypeSequence(MultiTypeSequence&& other) noexcept
        :
        DomNodeBase{ std::move(other) },
        m_blueprints{ std::move(other.m_blueprints) },
        m_items{ std::move(other.m_items) }
    {
        adoptItems();
    }

    MultiTypeSequence& MultiTypeSequence::operator=(MultiTypeSequence&& other) noexcept
    {
        if (this != &other)
        {
            DomNodeBase::operator=(std::move(other));
            m_blueprints = std::move(other.m_blueprints);
            m_items = std::move(other.m_items);
            adoptItems();
        }
        return *this;
    }

    void MultiTypeSequence::assign(const DomNodeBase& source)
    {
        clear();
        merge(source);
    }

    // A list is answered whole, so what is merged in replaces every item. An item whose name has
    // no blueprint here is copied as it stands.
    void MultiTypeSequence::merge(const DomNodeBase& source)
    {
        if (source.type() != DomNodeType::MultiTypeSequence)
            return;

        const MultiTypeSequence& sourceList = static_cast<const MultiTypeSequence&>(source);
        if (&sourceList == this)
            return;

        clear();
        for (const Item& item : sourceList.m_items)
        {
            DomNodeBase* added = add(item.name);
            if (added)
            {
                added->merge(*item.node);
                continue;
            }
            addItem(item.name, item.node->clone(this));
        }
    }

    std::unique_ptr<DomNodeBase> MultiTypeSequence::clone(DomNodeBase* parent) const
    {
        std::unique_ptr<MultiTypeSequence> copy = std::make_unique<MultiTypeSequence>(*this);
        copy->setParent(parent);
        return copy;
    }

    void MultiTypeSequence::forEachChild(const std::function<void(DomNodeBase&)>& action)
    {
        for (const Item& item : m_items)
            action(*item.node);
    }

    void MultiTypeSequence::removeChild(DomNodeBase* child)
    {
        std::erase_if(m_items, [child](const Item& item){
            return item.node.get() == child;
        });
        emitChange();
    }

    template <typename ValueT>
        requires std::is_same_v<ValueT, Section> || IsScalarValue<ValueT> || IsCompositeValue<ValueT>
    void MultiTypeSequence::addBlueprint(std::wstring_view name, const ValueT& defaultValue)
    {
        std::unique_ptr<DomNodeBase> blueprint;
        if constexpr (std::is_same_v<ValueT, Section>)
        {
            std::unique_ptr<Section> schema = std::make_unique<Section>(nullptr);
            schema->assign(defaultValue);
            blueprint = std::move(schema);
        }
        else
        {
            blueprint = std::make_unique<Value<ValueT>>(nullptr, defaultValue);
        }
        m_blueprints[std::wstring{ name }] = std::move(blueprint);
    }

    const DomNodeBase* MultiTypeSequence::blueprint(std::wstring_view name) const
    {
        const Blueprints::const_iterator found = m_blueprints.find(name);
        return found == m_blueprints.end() ? nullptr : found->second.get();
    }

    std::unique_ptr<DomNodeBase> MultiTypeSequence::makeDefaultItem(std::wstring_view name) const
    {
        const DomNodeBase* found = blueprint(name);
        return found ? found->clone(nullptr) : nullptr;
    }

    DomNodeBase* MultiTypeSequence::add(std::wstring_view name)
    {
        const DomNodeBase* found = blueprint(name);
        if (!found)
            return nullptr;
        m_items.push_back({ std::wstring{ name }, found->clone(this) });
        return m_items.back().node.get();
    }

    void MultiTypeSequence::addItem(std::wstring_view name, std::unique_ptr<DomNodeBase> node)
    {
        node->setParent(this);
        m_items.push_back({ std::wstring{ name }, std::move(node) });
    }

    void MultiTypeSequence::copyItemsOf(const MultiTypeSequence& source)
    {
        for (const Item& item : source.m_items)
            m_items.push_back({ item.name, item.node->clone(this) });
    }

    void MultiTypeSequence::adoptItems()
    {
        for (const Item& item : m_items)
            item.node->setParent(this);
    }

    // --- Section ---

    // Defined here rather than beside Section's other adders, since a section is declared before
    // the list it can hold.
    MultiTypeSequence& Section::addMultiTypeSequence(std::wstring_view key)
    {
        std::unique_ptr<MultiTypeSequence> child = std::make_unique<MultiTypeSequence>(this);
        MultiTypeSequence& result = *child;
        m_children[std::wstring{ key }] = std::move(child);
        return result;
    }
}
