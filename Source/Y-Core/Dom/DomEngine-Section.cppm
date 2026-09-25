export module ClaFi.Core.DomEngine :Section;

import :Core;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    export class Section : public DomNodeBase
    {
    public:
        using ChildrenMap = std::unordered_map<std::wstring, std::unique_ptr<DomNodeBase>, StringViewHash, std::equal_to<>>;
        using DomNodeBase::DomNodeBase;

        Section(const Section& other);
        Section& operator=(const Section& other);
        Section(Section&& other) noexcept;
        Section& operator=(Section&& other) noexcept;

        [[nodiscard]] DomNodeType type() const noexcept override { return DomNodeType::Section; }
        [[nodiscard]] std::unique_ptr<DomNodeBase> clone(DomNodeBase* parent) const override;

        void assign(const DomNodeBase& source) override;
        void merge(const DomNodeBase& source) override;

        [[nodiscard]] std::vector<std::wstring_view> getKeys() const;
        ChildrenMap& children() { return m_children; };
        DomNodeBase* child(std::wstring_view key);
        const DomNodeBase* child(std::wstring_view key) const;
        const Section& childSection(std::wstring_view key) const;
        Section& childSection(std::wstring_view key);
        void deleteChildren() { m_children.clear(); }
        void setChild(std::wstring_view key, std::unique_ptr<DomNodeBase> node);
        void forEachChild(const std::function<void(DomNodeBase&)>& action) override;

        template <typename ValueT>
        auto& addValue(std::wstring_view key, const ValueT& defaultValue);

        template <typename ValueT>
        auto& addSequence(std::wstring_view key, const ValueT& defaultValue);

        Section& addSection(std::wstring_view key);

        const DomNodeBase& operator/(std::wstring_view key) const override;
        DomNodeBase& operator/(std::wstring_view key) override;

    protected:
        ChildrenMap m_children;
    };
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom
{
    // Section's override, right below, is what resolves a path for Section and
    // CompositeValue nodes. The fallback for node kinds that don't support path
    // traversal lives in ClaFi.Core.Dom.Classes:Core.
    //
    // Read a child through path resolution: (section / key).get<T>(),
    // .getTo(out) or .set(value). A missing key is an error rather than a
    // default — operator/ emits a SchemaErrorEvent and then throws
    // std::runtime_error.

    // --- Section ---

    Section::Section(const Section& other)
        :
        DomNodeBase{ other }
    {
        for (const auto& key : other.getKeys())
        {
            m_children[std::wstring{ key }] = other.child(key)->clone(this);
        }
    }

    Section& Section::operator=(const Section& other)
    {
        if (this != &other)
        {
            DomNodeBase::operator=(other);
            assign(other);
        }
        return *this;
    }

    Section::Section(Section&& other) noexcept
        :
        DomNodeBase{ std::move(other) },
        m_children{ std::move(other.m_children) }
    {
        for (auto& [key, child] : m_children)
        {
            if (child)
            {
                child->setParent(this);
            }
        }
    }

    Section& Section::operator=(Section&& other) noexcept
    {
        if (this != &other)
        {
            m_children = std::move(other.m_children);
            DomNodeBase::operator=(std::move(other)); // does actually nothing
            for (auto& [key, child] : m_children)
            {
                if (child)
                {
                    child->setParent(this);
                }
            }
        }
        return *this;
    }

    std::unique_ptr<DomNodeBase> Section::clone(DomNodeBase* parent) const
    {
        auto copy = std::make_unique<Section>(parent);
        for (const auto& key : getKeys())
        {
            copy->setChild(key, child(key)->clone(copy.get()));
        }
        return copy;
    }

    void Section::assign(const DomNodeBase& source)
    {
        deleteChildren();
        merge(source);
    }

    void Section::merge(const DomNodeBase& source)
    {
        if (source.type() != DomNodeType::Section && source.type() != DomNodeType::CompositeValue)
        {
            return;
        }

        auto& srcSec = static_cast<const Section&>(source);

        for (const auto& key : srcSec.getKeys())
        {
            const DomNodeBase* srcChild = srcSec.child(key);
            DomNodeBase* dstChild = child(key);

            if (!dstChild)
            {
                m_children[std::wstring{ key }] = srcChild->clone(this);
                continue;
            }

            if (dstChild->type() != srcChild->type())
            {
                emitSchemaError(*this, L"Schema mismatch: Type mismatch for key '" + std::wstring{ key } + L"' during merge.");
                continue;
            }

            dstChild->merge(*srcChild);
        }
    }

    std::vector<std::wstring_view> Section::getKeys() const
    {
        std::vector<std::wstring_view> keys = std::vector<std::wstring_view>{};
        keys.reserve(m_children.size());
        for (const auto& [k, v] : m_children)
        {
            keys.push_back(k);
        }
        return keys;
    }

    DomNodeBase* Section::child(std::wstring_view key)
    {
        auto it = m_children.find(key);
        if (it != m_children.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    const DomNodeBase* Section::child(std::wstring_view key) const
    {
        auto it = m_children.find(key);
        if (it != m_children.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    const Section& Section::childSection(std::wstring_view key) const
    {
        const DomNodeBase* childPtr = this->child(key);
        if (childPtr)
        {
            const auto childType = childPtr->type();
            if (childType == DomNodeType::Section || childType == DomNodeType::CompositeValue)
            {
                return *static_cast<const Section*>(childPtr);
            }
        }
        unreachable("Dom: Requested childSection does not exist in the schema.");
    }

    Section& Section::childSection(std::wstring_view key)
    {
        DomNodeBase* childPtr = this->child(key);
        if (childPtr)
        {
            const auto childType = childPtr->type();
            if (childType == DomNodeType::Section || childType == DomNodeType::CompositeValue)
            {
                return *static_cast<Section*>(childPtr);
            }
        }
        unreachable("Dom: Requested childSection does not exist in the schema.");
    }

    void Section::setChild(std::wstring_view key, std::unique_ptr<DomNodeBase> node)
    {
        m_children[std::wstring{ key }] = std::move(node);
    }

    void Section::forEachChild(const std::function<void(DomNodeBase&)>& action)
    {
        for (const auto& [key, child] : m_children)
        {
            if (child)
            {
                action(*child);
            }
        }
    }

    template <typename ValueT>
    auto& Section::addValue(std::wstring_view key, const ValueT& defaultValue)
    {
        auto child = std::make_unique<Value<ValueT>>(this, defaultValue);
        auto& ref = *child;
        m_children[std::wstring{ key }] = std::move(child);
        return ref;
    }

    template <typename ValueT>
    auto& Section::addSequence(std::wstring_view key, const ValueT& defaultValue)
    {
        auto child = std::make_unique<Sequence<ValueT>>(this, defaultValue);
        auto& ref = *child;
        m_children[std::wstring{ key }] = std::move(child);
        return ref;
    }

    Section& Section::addSection(std::wstring_view key)
    {
        auto child = std::make_unique<Section>(this);
        auto& ref = *child;
        m_children[std::wstring{ key }] = std::move(child);
        return ref;
    }

    const DomNodeBase& Section::operator/(std::wstring_view key) const
    {
        const DomNodeBase* childPtr = child(key);
        if (!childPtr)
        {
            const_cast<Section*>(this)->emitSchemaError(
                const_cast<Section&>(*this),
                L"Traversal error: Key '" + std::wstring(key) + L"' not found."
            );
            throw std::runtime_error("Dom: Key not found during path traversal.");
        }
        return *childPtr;
    }

    DomNodeBase& Section::operator/(std::wstring_view key)
    {
        DomNodeBase* childPtr = child(key);
        if (!childPtr)
        {
            emitSchemaError(*this, L"Traversal error: Key '" + std::wstring(key) + L"' not found.");
            throw std::runtime_error("Dom: Key not found during path traversal.");
        }
        return *childPtr;
    }
}
