module ThisApp.DesignPage;

import ThisApp.ApplyToControl;
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
        constexpr std::array k_strokeOutput{ PaintChannel::Stroke };
        constexpr std::array k_selectedTextOutputs{ PaintChannel::Surface, PaintChannel::Text };
        constexpr std::array k_foundTextOutputs{ PaintChannel::Surface };

        constexpr SharedRules k_anyElement{
            .name = L"Any Element",
            .token = k_sharedRulesToken,
            .rules = &ThemeRules::shared
        };

        constexpr SharedRules k_anyWindow{
            .name = L"Any Window",
            .token = k_anyWindowRulesToken,
            .rules = &ThemeRules::anyWindow
        };

        constexpr SharedRules k_focusRing{
            .name = L"Focus Ring",
            .token = k_focusRingRulesToken,
            .rules = &ThemeRules::focusRing,
            .outputs = k_strokeOutput
        };

        // A page a branch of the tree lists: an element's own rules, or a list no one element owns.
        struct CategoryEntry
        {
            // Not explicit, so a branch lists elements and shared lists side by side.
            constexpr CategoryEntry(UiElement value)
                :
                element{ value }
            {
            }
            constexpr CategoryEntry(const SharedRules& value)
                :
                shared{ &value }
            {
            }
            OptionalUiElement element{};
            const SharedRules* shared{};
        };

        // A branch of the tree: its name, and the pages under it in the order they are listed.
        struct Category
        {
            std::wstring_view name{};
            std::span<const CategoryEntry> entries{};
        };

        constexpr auto k_windowRoots = std::to_array<CategoryEntry>({
            UiElement::Dialog,
            UiElement::Menu,
            UiElement::Hint
        });

        constexpr auto k_surfaces = std::to_array<CategoryEntry>({
            UiElement::Page,
            UiElement::Section,
            UiElement::SectionHeader,
            UiElement::Divider,
            UiElement::ToolBar,
            UiElement::DialogTitle,
            UiElement::Grid,
            UiElement::GridHeader,
            UiElement::GridRow
        });

        constexpr auto k_controls = std::to_array<CategoryEntry>({
            UiElement::Button,
            UiElement::ToolButton,
            UiElement::Tab,
            UiElement::ScrollButton,
            UiElement::ScrollThumb
        });

        constexpr auto k_focusAndSelection = std::to_array<CategoryEntry>({
            k_focusRing,
            UiElement::SelectionIndicator,
            UiElement::HoverIndicator,
            UiElement::SelectedText,
            UiElement::FoundText
        });

        constexpr auto k_testSubjects = std::to_array<CategoryEntry>({
            UiElement::Testee,
            UiElement::Bestee
        });

        constexpr std::array k_categories{
            Category{ L"Window roots", k_windowRoots },
            Category{ L"Surfaces", k_surfaces },
            Category{ L"Controls", k_controls },
            Category{ L"Focus & Selection", k_focusAndSelection },
            Category{ L"Test subjects", k_testSubjects }
        };

        // A text band has no stroke and casts no shadow, and only the selection re-inks its text.
        [[nodiscard]] PaintChannels elementOutputs(const UiElement element)
        {
            switch (element)
            {
                case UiElement::SelectedText:
                    return k_selectedTextOutputs;
                case UiElement::FoundText:
                    return k_foundTextOutputs;
                default:
                    return {};
            }
        }
    }

    std::wstring_view DesignPage::pickedPage() const
    {
        if (const TreeEntry* entry = pickedEntry())
            return tokenOf(*entry);
        return {};
    }

    void DesignPage::pickPage(const std::wstring_view token)
    {
        const auto found = std::ranges::find(m_entries, token, &DesignPage::tokenOf);
        const TreeEntry& entry = found != m_entries.end() ? *found : m_entries.front();
        m_tree.setCurrentItem(entry.item);
        showPickedPage();
    }

    // The keyboard follows only while the design is on screen - with a code page up it stays in
    // the code. A page change takes it to the tree item, as a click on the item does, and the
    // page's selection takes it on into the grid.
    void DesignPage::showPlace(const DesignPlace& place)
    {
        const TreeEntry* before = pickedEntry();
        pickPage(place.page);
        const TreeEntry* entry = pickedEntry();
        if (!entry)
            return;
        const TakeFocus takeFocus = visible() ? TakeFocus::Yes : TakeFocus::No;
        if (entry != before && takeFocus == TakeFocus::Yes)
            entry->item->setFocus();
        if (entry->rules)
            entry->rules->select(place.selection, takeFocus);
    }

    void DesignPage::bind(ThemeColors& colors, const OnGetElementRuleBase& ruleBase,
        const OnRulesChanged& onRulesChanged)
    {
        ThemeRules& rules = colors.rules;
        for (const TreeEntry& entry : m_entries)
        {
            if (!entry.rules)
                continue;
            ColorRules& list = entry.element
                ? rules.of(*entry.element)
                : rules.*entry.shared->rules;
            const ColorRules& defaults = entry.element
                ? m_defaultRules.of(*entry.element)
                : m_defaultRules.*entry.shared->rules;
            const PaintChannels outputs = entry.shared
                ? entry.shared->outputs
                : elementOutputs(*entry.element);
            const OptionalUiElement element = entry.element;
            OnGetListRuleBase listRuleBase = [ruleBase, element](const ColorRule& rule,
                const RuleChannel channel) {
                return ruleBase(element, rule, channel);
            };
            entry.rules->bind(list, defaults, outputs, colors, std::move(listRuleBase),
                onRulesChanged);
        }
    }

    void DesignPage::rebuildRules()
    {
        for (const TreeEntry& entry : m_entries)
            if (entry.rules)
                entry.rules->rebuild();
    }

    // A hidden page is not laid out, and a scroll asked for a control the pass has not placed is
    // refused - see Control::scrollIntoView. So the item picked while this page was off screen is
    // brought into view as the page comes on, by the pass that places it.
    void DesignPage::visibilityChanged()
    {
        if (visible() && m_tree.currentItem())
            m_tree.currentItem()->scrollIntoViewOnAlign();
    }

    void DesignPage::buildTree()
    {
        addEntry(addRootItem(), TreeEntry{ .page = &m_palettePage });
        addEntry(addRootItem(), TreeEntry{ .shared = &k_anyElement });
        addEntry(addRootItem(), TreeEntry{ .shared = &k_anyWindow });
        // Sets the theme-wide pages apart, so they read as peers of the categories.
        m_tree.add<Divider>(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        for (const Category& category : k_categories)
        {
            TreeNode& node = m_tree.addNode(HeaderText{ InkGrade::Muted, category.name });
            for (const CategoryEntry& entry : category.entries)
            {
                addEntry(node.addItem(), TreeEntry{
                    .element = entry.element,
                    .shared = entry.shared
                });
            }
        }
        showPickedPage();
    }

    TreeItem& DesignPage::addRootItem()
    {
        return m_tree.addItem(TextFormat{ TextStyleId::SubHeading });
    }

    void DesignPage::addEntry(TreeItem& item, TreeEntry entry)
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

    void DesignPage::showPickedPage()
    {
        if (const TreeEntry* entry = pickedEntry())
            m_pages.setCurrentItem(entry->page);
    }

    const DesignPage::TreeEntry* DesignPage::pickedEntry() const
    {
        if (const Control* item = m_tree.currentItem())
            return &m_entries[item->tag<std::size_t>()];
        return nullptr;
    }

    std::wstring_view DesignPage::nameOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).name;
        if (entry.shared)
            return entry.shared->name;
        return k_paletteTitle;
    }

    std::wstring_view DesignPage::tokenOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).token;
        if (entry.shared)
            return entry.shared->token;
        return k_paletteToken;
    }
}
