module ThisApp.Design2Page;

import ThisApp.Consts;
import ThisApp.ElementPage;
import ThisApp.RuleSlider;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Expander;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        using CategoryNode = ExpanderWith<StackPanel>;

        // A branch of the tree: its name, and the elements under it in the order they are listed.
        struct ElementCategory
        {
            std::wstring_view name;
            std::span<const UiElement> elements;
        };

        constexpr std::array k_windowRoots{
            UiElement::Form,
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
            UiElement::FormTitle,
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
            UiElement::InactiveIndicator,
            UiElement::SelectedText
        };

        constexpr std::array k_elementCategories{
            ElementCategory{ L"Window roots", k_windowRoots },
            ElementCategory{ L"Surfaces", k_surfaces },
            ElementCategory{ L"Controls", k_controls },
            ElementCategory{ L"Focus and Selection", k_focusAndSelection }
        };

        // How far an element's name stands in from its category's.
        constexpr float k_itemIndent{ 16.0f };
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
            ColorRules2& list = entry.element ? rules.of(*entry.element) : rules.shared;
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
        m_tree.onCanFocusItem([this](CanFocusItemEvent& event) {
            event.canFocus = std::ranges::any_of(m_entries, [&event](const TreeEntry& entry) {
                return entry.item == &event.item;
            });
        });
        m_tree.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            showPickedPage();
        });

        addEntry(m_tree, std::nullopt, L"Any element");
        // Sets the shared rules apart, so they read as a peer of the categories.
        m_tree.add<Divider>(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        for (const ElementCategory& category : k_elementCategories)
        {
            CategoryNode& node = m_tree.add<CategoryNode>(
                HostProps{
                    VerticalAlign::Top,
                    ExpanderViewMode::TreeNode,
                    HeaderText{ category.name }
                },
                BodyProps{
                    Orientation::Vertical,
                    Padding{ k_itemIndent, 0.0f }
                }
            );
            for (const UiElement element : category.elements)
                addEntry(node.body(), element, uiElementOf(element).name);
        }
    }

    void Design2Page::addEntry(StackPanel& parent, const OptionalUiElement element,
        const std::wstring_view name)
    {
        ToolButton& item = parent.add<ToolButton>(
            Text{ name },
            Tag{ m_entries.size() },
            HorizontalTextAnchor::Left,
            ShowSelectionOnSurface::Yes
        );
        m_entries.push_back(TreeEntry{
            .element = element,
            .item = &item,
            .page = &m_pages.add<ElementPage>(name)
        });
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
        return k_sharedRulesToken;
    }
}
