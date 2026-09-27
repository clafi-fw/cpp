module ThisApp.Design2Page;

import ThisApp.Consts;
import ThisApp.ElementPage;
import ThisApp.RuleSlider;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Divider;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.TreeView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        constexpr SharedRules k_anyElement{
            .name = L"Any element",
            .token = k_sharedRulesToken,
            .rules = &ThemeRules2::shared
        };

        constexpr SharedRules k_anyWindow{
            .name = L"Any window",
            .token = k_anyWindowRulesToken,
            .rules = &ThemeRules2::anyWindow
        };

        // A branch of the tree: its name, and the elements under it in the order they are listed.
        struct ElementCategory
        {
            std::wstring_view name;
            std::span<const UiElement> elements;
            const SharedRules* shared{ nullptr }; // listed ahead of the elements
        };

        constexpr std::array k_windowRoots{
            UiElement::Dialog,
            UiElement::Menu,
            UiElement::Tooltip
        };

        constexpr std::array k_surfaces{
            UiElement::Page,
            UiElement::TabLine,
            UiElement::Section,
            UiElement::Header,
            UiElement::Divider,
            UiElement::Bar,
            UiElement::DialogTitle,
            UiElement::Grid,
            UiElement::GridRow,
            UiElement::GridLine
        };

        constexpr std::array k_controls{
            UiElement::Button,
            UiElement::ScrollButton,
            UiElement::ScrollThumb
        };

        constexpr std::array k_focusAndSelection{
            UiElement::Accent,
            UiElement::Spot,
            UiElement::SelectionIndicator,
            UiElement::SelectedText
        };

        constexpr std::array k_elementCategories{
            ElementCategory{ L"Window roots", k_windowRoots, &k_anyWindow },
            ElementCategory{ L"Surfaces", k_surfaces },
            ElementCategory{ L"Controls", k_controls },
            ElementCategory{ L"Focus and Selection", k_focusAndSelection }
        };
    }

    std::wstring_view Design2Page::pickedPage() const
    {
        if (const TreeEntry* entry = pickedEntry())
            return tokenOf(*entry);
        return {};
    }

    void Design2Page::pickPage(const std::wstring_view token)
    {
        for (const TreeEntry& entry : m_entries)
            if (tokenOf(entry) == token)
                m_tree.setCurrentItem(entry.item);
    }

    void Design2Page::bind(ThemeColors& colors, const OnGetElementRuleBase& ruleBase,
        const OnRulesChanged& onRulesChanged)
    {
        ThemeRules2& rules = colors.rules2;
        for (const TreeEntry& entry : m_entries)
        {
            ColorRules2& list = entry.element
                ? rules.of(*entry.element)
                : rules.*entry.shared->rules;
            const OptionalUiElement element = entry.element;
            OnGetListRuleBase listRuleBase = [ruleBase, element](const ColorRule2& rule,
                const RuleChannel channel) {
                return ruleBase(element, rule, channel);
            };
            entry.page->bind(list, colors, std::move(listRuleBase), onRulesChanged);
        }
    }

    void Design2Page::rebuildRules()
    {
        for (const TreeEntry& entry : m_entries)
            entry.page->rebuild();
    }

    void Design2Page::buildTree()
    {
        m_tree.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            showPickedPage();
        });

        addEntry(m_tree.addItem(), TreeEntry{ .shared = &k_anyElement });
        // Sets Any element apart, so it reads as a peer of the categories.
        m_tree.add<Divider>(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        for (const ElementCategory& category : k_elementCategories)
        {
            TreeNode& node = m_tree.addNode(HeaderText{ category.name });
            if (category.shared)
                addEntry(node.addItem(), TreeEntry{ .shared = category.shared });
            for (const UiElement element : category.elements)
                addEntry(node.addItem(), TreeEntry{ .element = element });
        }
    }

    void Design2Page::addEntry(TreeItem& item, TreeEntry entry)
    {
        const std::wstring_view name = entry.element
            ? uiElementOf(*entry.element).name
            : entry.shared->name;
        item.text() << name;
        item.setTag(Tag{ m_entries.size() });
        entry.item = &item;
        entry.page = &m_pages.add<ElementPage>(name);
        m_entries.push_back(entry);
        // The first item stands picked, so the body never opens empty.
        if (!m_tree.currentItem())
            m_tree.setCurrentItem(item);
    }

    void Design2Page::showPickedPage()
    {
        if (const TreeEntry* entry = pickedEntry())
            m_pages.setCurrentItem(entry->page);
    }

    const Design2Page::TreeEntry* Design2Page::pickedEntry() const
    {
        if (const Control* item = m_tree.currentItem())
            return &m_entries[item->tag<std::size_t>()];
        return nullptr;
    }

    std::wstring_view Design2Page::tokenOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).token;
        return entry.shared->token;
    }
}
