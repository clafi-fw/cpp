module Themes_App.DesignPage;

import Themes_App.ApplyToControl;
import Themes_App.Consts;
import Themes_App.ElementPage;
import Themes_App.RuleSlider;
import Themes_App.SizesPage;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Base.StackBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
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

        constexpr SharedRules k_anyNonWindow{
            .name = L"Any Non-Window",
            .token = k_anyNonWindowRulesToken,
            .rules = &ThemeRules::anyNonWindow
        };

        constexpr auto k_windowRoots = std::to_array<CategoryEntry>({
            UiElement::Dialog,
            UiElement::Menu,
            UiElement::Tooltip
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
            UiElement::SelectionIndicator,
            UiElement::HoverIndicator,
            UiElement::SelectedText,
            UiElement::FoundText
        });

        constexpr auto k_blocks = std::to_array<CategoryEntry>({
            UiElement::InfoCallout,
            UiElement::QuestionCallout,
            UiElement::WarningCallout,
            UiElement::ErrorCallout,
            UiElement::OkCallout,
            UiElement::Do,
            UiElement::Dont,
            UiElement::CodeBlock
        });

        constexpr auto k_testSubjects = std::to_array<CategoryEntry>({
            UiElement::Testee,
            UiElement::Bestee
        });

        constexpr auto k_sizes = std::to_array<CategoryEntry>({
            MetricsSet::Button,
            MetricsSet::ToolButton,
            MetricsSet::ListItem,
            MetricsSet::CheckMark,
            MetricsSet::Tab,
            MetricsSet::Page,
            MetricsSet::GridCell,
            MetricsSet::DialogTitle,
            MetricsSet::PrimaryWindow,
            MetricsSet::SecondaryWindow,
            MetricsSet::ScrollButton,
            MetricsSet::ScrollThumb,
            MetricsSet::Callout,
            MetricsSet::DoDont,
            MetricsSet::CodeBlock,
            MetricsSet::TextBox,
            MetricsSet::Slider
        });

        constexpr std::array k_categories{
            Category{ L"Window roots", L"WindowRoots", k_windowRoots },
            Category{ L"Surfaces", L"Surfaces", k_surfaces },
            Category{ L"Controls", L"Controls", k_controls },
            Category{ L"Focus & Selection", L"FocusAndSelection", k_focusAndSelection },
            Category{ L"Blocks", L"Blocks", k_blocks },
            Category{ L"Test subjects", L"TestSubjects", k_testSubjects },
            Category{ L"Sizes", L"Sizes", k_sizes }
        };

        using SizesTokens = std::array<std::wstring, k_metricsSetCount>;

        // A sizes page's token, set apart from the element that goes by the same name.
        [[nodiscard]] std::wstring_view sizesToken(const MetricsSet set)
        {
            static const SizesTokens tokens = []() {
                SizesTokens result{};
                for (std::size_t i = 0; i != k_metricsSetCount; ++i)
                    result[i] = std::wstring{ L"Sizes" } + std::wstring{ k_metricsSets[i].token };
                return result;
            }();
            return tokens[static_cast<std::size_t>(set)];
        }

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
        for (TreeEntry& entry : m_entries)
        {
            if (!entry.rules)
                continue;
            ColorRules& list = entry.element
                ? rules.of(*entry.element)
                : rules.*entry.shared->rules;
            entry.list = &list;
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

    void DesignPage::bindSizes(ThemeMetrics& metrics, const OnSizesChanged& onSizesChanged)
    {
        for (const TreeEntry& entry : m_entries)
            if (entry.sizesPage)
                entry.sizesPage->bind(metrics, m_defaultMetrics, onSizesChanged);
    }

    void DesignPage::rebuildRules()
    {
        for (const TreeEntry& entry : m_entries)
            if (entry.rules)
                entry.rules->rebuild();
    }

    void DesignPage::showSizes()
    {
        for (const TreeEntry& entry : m_entries)
            if (entry.sizesPage)
                entry.sizesPage->refresh();
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
        addItemEntry(addRootItem(), TreeEntry{ .page = &m_palettePage });
        addItemEntry(addRootItem(), TreeEntry{ .shared = &k_anyElement });
        addItemEntry(addRootItem(), TreeEntry{ .shared = &k_anyWindow });
        addItemEntry(addRootItem(), TreeEntry{ .shared = &k_anyNonWindow });
        // Sets the theme-wide pages apart, so they read as peers of the categories.
        m_tree.addDivider(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        for (const Category& category : k_categories)
        {
            // The header carries the name muted, set apart from the pages under it. It is a row
            // of its own, and its page lists those pages - built once they stand.
            TreeNode& node = m_tree.addNode(HeaderText{
                ColoredText{ InkGrade::Muted, category.name }
            });
            const std::size_t categoryIndex = addEntry(node.header(),
                TreeEntry{ .category = &category });
            EntryIndexes members;
            for (const CategoryEntry& entry : category.entries)
            {
                members.push_back(addItemEntry(node.addItem(), TreeEntry{
                    .element = entry.element,
                    .shared = entry.shared,
                    .sizes = entry.sizes
                }));
            }
            buildCategoryPage(categoryIndex, members);
        }
        showPickedPage();
    }

    TreeItem& DesignPage::addRootItem()
    {
        return m_tree.addItem(TextFormat{ TextStyles::SubHeading });
    }

    // An item is named after the page it opens.
    std::size_t DesignPage::addItemEntry(TreeItem& item, TreeEntry entry)
    {
        item.text() << nameOf(entry);
        return addEntry(item, std::move(entry));
    }

    std::size_t DesignPage::addEntry(RichControl& row, TreeEntry entry)
    {
        const std::size_t index = m_entries.size();
        row.setTag(Tag{ index });
        entry.item = &row;
        // A page of rules or of sizes is made here - the palette's page stands already, and a
        // category's is built once the pages it lists do.
        if (entry.sizes)
        {
            entry.sizesPage = &m_pages.add<SizesPage>(*entry.sizes);
            entry.page = entry.sizesPage;
        }
        else if (!entry.page && !entry.category)
        {
            entry.rules = &m_pages.add<ElementPage>(nameOf(entry));
            entry.page = entry.rules;
        }
        m_entries.push_back(std::move(entry));
        // The first item stands picked, so the body never opens empty.
        if (!m_tree.currentItem())
            m_tree.setCurrentItem(row);
        return index;
    }

    // Titled as an element page is, with a row per page under the category. A row's text is
    // written when the page is shown, since it carries a count that edits move.
    void DesignPage::buildCategoryPage(const std::size_t category, const EntryIndexes& members)
    {
        TreeEntry& entry = m_entries[category];
        Panel& page = m_pages.add<Panel>();
        page.createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ).createBody<Label>(
            VerticalTextAnchor::Center,
            Text{ StyledText{ TextStyles::SubTitle, nameOf(entry) } }
        );
        StackPanel& rows = page.createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f }
        );
        for (const std::size_t member : members)
        {
            ToolButton& row = rows.add<ToolButton>(HorizontalTextAnchor::Left);
            row.setTag(Tag{ member });
            row.onClick([this, member](ClickEvent&) {
                openEntry(member);
            });
            entry.indexRows.push_back(&row);
        }
        entry.page = &page;
    }

    // Opened from a category's page: the tree picks the row, and the keyboard goes with it the
    // way a pick in the tree takes it.
    void DesignPage::openEntry(const std::size_t index)
    {
        const TreeEntry& entry = m_entries[index];
        pickPage(tokenOf(entry));
        entry.item->setFocus();
    }

    void DesignPage::refreshIndex(const TreeEntry& category)
    {
        for (RichControl* row : category.indexRows)
            row->text() = indexRowText(m_entries[row->tag<std::size_t>()]);
    }

    void DesignPage::showPickedPage()
    {
        const TreeEntry* entry = pickedEntry();
        if (!entry)
            return;
        if (entry->category)
            refreshIndex(*entry);
        m_pages.setCurrentItem(entry->page);
    }

    const DesignPage::TreeEntry* DesignPage::pickedEntry() const
    {
        if (const Control* item = m_tree.currentItem())
            return &m_entries[item->tag<std::size_t>()];
        return nullptr;
    }

    // The page's name, and after it how many rules the page holds, muted. A sizes page holds none.
    Text DesignPage::indexRowText(const TreeEntry& entry)
    {
        if (entry.sizes)
            return Text{ nameOf(entry) };
        const std::size_t count = entry.list ? entry.list->size() : 0;
        Text tally{};
        if (count == 0)
            tally << L"no rules";
        else
            tally << count << (count == 1 ? L" rule" : L" rules");
        Text text{ nameOf(entry) };
        text << L"  " << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, tally } };
        return text;
    }

    std::wstring_view DesignPage::nameOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).name;
        if (entry.sizes)
            return metricsSetOf(*entry.sizes).name;
        if (entry.shared)
            return entry.shared->name;
        if (entry.category)
            return entry.category->name;
        return k_paletteTitle;
    }

    std::wstring_view DesignPage::tokenOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).token;
        if (entry.sizes)
            return sizesToken(*entry.sizes);
        if (entry.shared)
            return entry.shared->token;
        if (entry.category)
            return entry.category->token;
        return k_paletteToken;
    }
}
