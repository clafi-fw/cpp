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
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        constexpr SharedRules k_anyElement{
            .name = L"Any Element",
            .token = k_sharedRulesToken,
            .rules = &ThemeRules2::shared
        };

        constexpr SharedRules k_anyWindow{
            .name = L"Any Window",
            .token = k_anyWindowRulesToken,
            .rules = &ThemeRules2::anyWindow
        };

        // A branch of the tree: its name, and the elements under it in the order they are listed.
        struct ElementCategory
        {
            std::wstring_view name;
            std::span<const UiElement> elements;
        };

        constexpr std::array k_windowRoots{
            UiElement::Dialog,
            UiElement::Menu,
            UiElement::Tooltip
        };

        constexpr std::array k_surfaces{
            UiElement::Page,
            UiElement::Section,
            UiElement::SectionHeader,
            UiElement::Divider,
            UiElement::ToolBar,
            UiElement::DialogTitle,
            UiElement::Grid,
            UiElement::GridHeader,
            UiElement::GridRow
        };

        constexpr std::array k_controls{
            UiElement::Button,
            UiElement::ToolButton,
            UiElement::Tab,
            UiElement::ScrollButton,
            UiElement::ScrollThumb
        };

        constexpr std::array k_focusAndSelection{
            UiElement::SelectionIndicator,
            UiElement::HoverIndicator,
            UiElement::SelectedText
        };

        constexpr std::array k_elementCategories{
            ElementCategory{ L"Window roots", k_windowRoots },
            ElementCategory{ L"Surfaces", k_surfaces },
            ElementCategory{ L"Controls", k_controls },
            ElementCategory{ L"Focus & Selection", k_focusAndSelection }
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
            if (!entry.rules)
                continue;
            ColorRules2& list = entry.element
                ? rules.of(*entry.element)
                : rules.*entry.shared->rules;
            const ColorRules2& defaults = entry.element
                ? m_defaultRules.of(*entry.element)
                : m_defaultRules.*entry.shared->rules;
            const OptionalUiElement element = entry.element;
            OnGetListRuleBase listRuleBase = [ruleBase, element](const ColorRule2& rule,
                const RuleChannel channel) {
                return ruleBase(element, rule, channel);
            };
            entry.rules->bind(list, defaults, colors, std::move(listRuleBase), onRulesChanged);
        }
    }

    void Design2Page::rebuildRules()
    {
        for (const TreeEntry& entry : m_entries)
            if (entry.rules)
                entry.rules->rebuild();
    }

    void Design2Page::buildTree()
    {
        m_tree.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            showPickedPage();
        });

        addEntry(addRootItem(), TreeEntry{ .page = &m_palettePage });
        addEntry(addRootItem(), TreeEntry{ .shared = &k_anyElement });
        addEntry(addRootItem(), TreeEntry{ .shared = &k_anyWindow });
        // Sets the theme-wide pages apart, so they read as peers of the categories.
        m_tree.add<Divider>(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        for (const ElementCategory& category : k_elementCategories)
        {
            TreeNode& node = m_tree.addNode(HeaderText{ InkGrade::Muted, category.name });
            for (const UiElement element : category.elements)
                addEntry(node.addItem(), TreeEntry{ .element = element });
        }
    }

    // The style goes in ahead of the name addEntry writes after it.
    TreeItem& Design2Page::addRootItem()
    {
        TreeItem& item = m_tree.addItem();
        item.text() << TextStyleId::SubHeading;
        return item;
    }

    void Design2Page::addEntry(TreeItem& item, TreeEntry entry)
    {
        const std::wstring_view name = nameOf(entry);
        item.text() << name;
        item.setTag(Tag{ m_entries.size() });
        entry.item = &item;
        // A page of rules is made here - the palette's page stands already.
        if (!entry.page)
        {
            entry.rules = &m_pages.add<ElementPage>(name);
            entry.page = entry.rules;
        }
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

    std::wstring_view Design2Page::nameOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).name;
        if (entry.shared)
            return entry.shared->name;
        return k_paletteTitle;
    }

    std::wstring_view Design2Page::tokenOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).token;
        if (entry.shared)
            return entry.shared->token;
        return k_paletteToken;
    }
}
