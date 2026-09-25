module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.TextItems;

import ClaFi.StdLib;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;

import ClaFi.Controls.Base.ButtonBase;

namespace ClaFi::Controls
{

    // TextItem

    export class TextItem
    {
    public:
        template<typename... Args>
            requires (sizeof...(Args) > 0 && !std::is_same_v<std::decay_t<std::tuple_element_t<0, std::tuple<Args...>>>, TextItem>)
        explicit TextItem(Args&&...);
        TextItem(const TextItem&) = default;
    public:
        DECLARE_REF_PROPERTY(Text, text, Text{}) // what the item says
        // Shown while the item's text is empty, for the item that stands for no value. See Controls
        DECLARE_REF_PROPERTY(PlaceHolderText, placeHolderText, PlaceHolderText{})
        DECLARE_REF_PROPERTY(TooltipText, tooltipText, TooltipText{}) // the hint the item carries
        DECLARE_PROPERTY(Tag, tag, Tag{}) // whatever the caller hangs on the item
    private:
    private:
    };

    // TextItems

    export class TextItems : public std::vector<TextItem>
    {
    public:
        using std::vector<TextItem>::vector;
    };

    // A text's characters as a user would type them - the plain text, inline objects dropped.
    export [[nodiscard]] std::wstring typedForm(const Text&);
    // What an item is named by when typed - its text, or its placeholder while that is empty.
    export [[nodiscard]] std::wstring typedName(const TextItem&);
    // Whether a name begins with what was typed, case aside. Nothing typed begins every name.
    export [[nodiscard]] bool startsWithFolded(std::wstring_view name, std::wstring_view typed);

    export class TextItemsContainer
    {
    public:
        template <typename... Args>
        TextItemsContainer(TextItems* ownItems, TextItems* sharedItems, Args&&...);
    public:
        // Shared items
        template <typename... Args>
        explicit TextItemsContainer(TextItems&, Args&&...);
        // Own items
        template <typename... Args>
        explicit TextItemsContainer(TextItems&&, Args&&...);
        virtual ~TextItemsContainer();
    public:
        inline const TextItem* selectedItem() const
            // It's an MSVC quirk - without inline the linker gives 'unresolved' error
        {
            if (m_itemIndex)
                return &m_items[m_itemIndex.value()];
            return nullptr;
        }
        void setItemIndex(std::size_t);
        ItemIndexValue itemIndex() const { return  m_itemIndex; }
        const TextItems& items() const { return m_items; }
    protected:
        virtual void itemIndexChanged() = 0;
    private:
        const TextItems& m_items;
        TextItems* m_ownItems;
        ItemIndexValue m_itemIndex;
    };


    //-------------------------------------------------------------------------


    // TextItem

    template<typename ...Args>
        requires (sizeof...(Args) > 0 && !std::is_same_v<std::decay_t<std::tuple_element_t<0, std::tuple<Args...>>>, TextItem>)
    TextItem::TextItem(Args && ... args)
        :
        INIT_PROPERTY(text),
        INIT_PROPERTY(placeHolderText),
        INIT_PROPERTY(tooltipText),
        INIT_PROPERTY(tag)
    {
    }

    // TextItemsContainer

    template <typename ... Args>
    TextItemsContainer::TextItemsContainer(TextItems* ownItems, TextItems* sharedItems, Args&&... args)
        :
        m_items{ ownItems ? *ownItems : *sharedItems },
        m_ownItems{ ownItems },
        m_itemIndex{ READ_PROPERTY(ItemIndex, ItemIndex{}).value } // which item is current
    {
    }

    template <typename ... Args>
    TextItemsContainer::TextItemsContainer(TextItems& items, Args&&... args)
        :
        TextItemsContainer{ nullptr, &items, std::forward<Args>(args)... }
    {
    }

    template <typename ... Args>
    TextItemsContainer::TextItemsContainer(TextItems&& items, Args&&... args)
        :
        TextItemsContainer{
            new TextItems(std::move(items)),
            nullptr,
            std::forward<Args>(args)...
    }
    {
    }


    //-------------------------------------------------------------------------


    std::wstring typedForm(const Text& text)
    {
        // What an inline object - a gap, an icon, a tab stop - stands as in the plain text.
        constexpr wchar_t k_objectCharacter{ L'\uFFFC' };
        std::wstring result{ text.plainText() };
        std::erase(result, k_objectCharacter);
        return result;
    }

    std::wstring typedName(const TextItem& item)
    {
        std::wstring name = typedForm(item.text());
        if (name.empty())
            name = typedForm(item.placeHolderText());
        return name;
    }

    bool startsWithFolded(const std::wstring_view name, const std::wstring_view typed)
    {
        const auto foldCase = [](const wchar_t value){
            return static_cast<wchar_t>(std::towlower(value));
        };
        return name.size() >= typed.size()
            && std::ranges::equal(name.substr(0, typed.size()), typed, std::ranges::equal_to{},
                foldCase, foldCase);
    }

    // TextItemsContainer

    TextItemsContainer::~TextItemsContainer()
    {
        delete m_ownItems;
    }

    void TextItemsContainer::setItemIndex(std::size_t value)
    {
        if (m_itemIndex == value)
            return;
        m_itemIndex = value;
        itemIndexChanged();
    }

}
