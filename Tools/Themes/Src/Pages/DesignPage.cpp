module Themes_App.DesignPage;

import Themes_App.ApplyToControl;
import Themes_App.Consts;
import Themes_App.ElementPage;
import Themes_App.PageKinds;
import Themes_App.PreviewPage;
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

import ClaFi.Icons.RowIcons;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace RowIcons = ::ClaFi::Icons::RowIcons;

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
            .rules = &ThemeRules::anyWindow,
            .preview = PreviewPage::Windows
        };

        constexpr SharedRules k_anyNonWindow{
            .name = L"Any Non-Window",
            .token = k_anyNonWindowRulesToken,
            .rules = &ThemeRules::anyNonWindow
        };

        constexpr auto k_windowRoots = std::to_array<CategoryEntry>({
            { UiElement::Dialog, PreviewPage::Windows },
            { UiElement::Menu, PreviewPage::Windows },
            { UiElement::Tooltip, PreviewPage::Windows }
        });

        constexpr auto k_surfaces = std::to_array<CategoryEntry>({
            { UiElement::Page, PreviewPage::Surfaces },
            { UiElement::Section, PreviewPage::Surfaces },
            { UiElement::SectionHeader, PreviewPage::Surfaces },
            { UiElement::Divider, PreviewPage::Surfaces },
            { UiElement::ToolBar, PreviewPage::Surfaces },
            { UiElement::DialogTitle, PreviewPage::Windows },
            { UiElement::Grid, PreviewPage::Grid },
            { UiElement::GridHeader, PreviewPage::Grid },
            { UiElement::GridRow, PreviewPage::Grid }
        });

        constexpr auto k_controls = std::to_array<CategoryEntry>({
            { UiElement::Button, PreviewPage::Inputs },
            { UiElement::ToolButton, PreviewPage::Inputs },
            { UiElement::Tab, PreviewPage::Tabs },
            { UiElement::ScrollButton, PreviewPage::Scrolling },
            { UiElement::ScrollThumb, PreviewPage::Scrolling }
        });

        constexpr auto k_focusAndSelection = std::to_array<CategoryEntry>({
            { UiElement::SelectionIndicator, PreviewPage::Selection },
            { UiElement::HoverIndicator, PreviewPage::Selection },
            { UiElement::SelectedText, PreviewPage::Selection },
            { UiElement::FoundText, PreviewPage::Selection }
        });

        constexpr auto k_blocks = std::to_array<CategoryEntry>({
            { UiElement::InfoCallout, PreviewPage::Callouts },
            { UiElement::QuestionCallout, PreviewPage::Callouts },
            { UiElement::WarningCallout, PreviewPage::Callouts },
            { UiElement::ErrorCallout, PreviewPage::Callouts },
            { UiElement::OkCallout, PreviewPage::Callouts },
            { UiElement::Do, PreviewPage::DoDont },
            { UiElement::Dont, PreviewPage::DoDont },
            { UiElement::CodeBlock, PreviewPage::CodeBlock }
        });

        constexpr auto k_testSubjects = std::to_array<CategoryEntry>({
            { UiElement::Testee, PreviewPage::TestSubjects },
            { UiElement::Bestee, PreviewPage::TestSubjects }
        });

        constexpr auto k_windowRootSizes = std::to_array<CategoryEntry>({
            { MetricsSet::PrimaryWindow, PreviewPage::Windows },
            { MetricsSet::SecondaryWindow, PreviewPage::Windows }
        });

        constexpr auto k_surfaceSizes = std::to_array<CategoryEntry>({
            { MetricsSet::Page, PreviewPage::Surfaces },
            { MetricsSet::DialogTitle, PreviewPage::Windows },
            { MetricsSet::GridCell, PreviewPage::Grid }
        });

        constexpr auto k_controlSizes = std::to_array<CategoryEntry>({
            { MetricsSet::Button, PreviewPage::Inputs },
            { MetricsSet::ToolButton, PreviewPage::Inputs },
            { MetricsSet::Tab, PreviewPage::Tabs },
            { MetricsSet::ListItem, PreviewPage::Selection },
            { MetricsSet::CheckMark, PreviewPage::Selection },
            { MetricsSet::TextBox, PreviewPage::Inputs },
            { MetricsSet::Slider, PreviewPage::Inputs },
            { MetricsSet::ScrollButton, PreviewPage::Scrolling },
            { MetricsSet::ScrollThumb, PreviewPage::Scrolling }
        });

        constexpr auto k_blockSizes = std::to_array<CategoryEntry>({
            { MetricsSet::Callout, PreviewPage::Callouts },
            { MetricsSet::DoDont, PreviewPage::DoDont },
            { MetricsSet::CodeBlock, PreviewPage::CodeBlock }
        });

        constexpr std::array k_colorCategories{
            Category{ L"Window roots", L"WindowRoots", k_windowRoots },
            Category{ L"Surfaces", L"Surfaces", k_surfaces },
            Category{ L"Controls", L"Controls", k_controls },
            Category{ L"Focus & Selection", L"FocusAndSelection", k_focusAndSelection },
            Category{ L"Blocks", L"Blocks", k_blocks },
            Category{ L"Test subjects", L"TestSubjects", k_testSubjects }
        };

        // The tokens are set apart from those of the colour categories going by the same names.
        constexpr std::array k_metricsCategories{
            Category{ L"Window roots", L"MetricsWindowRoots", k_windowRootSizes },
            Category{ L"Surfaces", L"MetricsSurfaces", k_surfaceSizes },
            Category{ L"Controls", L"MetricsControls", k_controlSizes },
            Category{ L"Blocks", L"MetricsBlocks", k_blockSizes }
        };

        constexpr TreeRoot k_colorsRoot{
            .name = L"Colors",
            .token = L"Colors",
            .categories = k_colorCategories
        };

        constexpr TreeRoot k_metricsRoot{
            .name = L"Metrics",
            .token = L"Metrics",
            .categories = k_metricsCategories
        };

        constexpr TreeRoot k_textStylesRoot{
            .name = L"Text Styles",
            .token = L"TextStyles"
        };

        using SizesTokens = std::array<std::wstring, k_metricsSetCount>;

        // A sizes page's token, set apart from the element that goes by the same name.
        [[nodiscard]] std::wstring_view sizesToken(const MetricsSet set)
        {
            static const SizesTokens tokens = []() {
                SizesTokens result{};
                for (std::size_t i = 0; i != k_metricsSetCount; ++i)
                    result[i] = std::wstring{ L"Metrics" } + std::wstring{ k_metricsSets[i].token };
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

    NodePreview DesignPage::pickedPreview() const
    {
        if (const TreeEntry* entry = pickedEntry())
            return entry->preview;
        return {};
    }

    void DesignPage::pickPage(const std::wstring_view token)
    {
        auto found = std::ranges::find(m_entries, token, &DesignPage::tokenOf);
        if (found == m_entries.end())
            found = std::ranges::find(m_entries, k_paletteToken, &DesignPage::tokenOf);
        m_tree.setCurrentItem(found->item);
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
        writePageTitle(m_paletteTitle.text(), PageKind::Palette, k_paletteTitle);
        const std::array themeWidePages{
            TreeEntry{ .page = &m_palettePage },
            entryOf(k_anyElement),
            entryOf(k_anyWindow),
            entryOf(k_anyNonWindow)
        };
        addRoot(k_colorsRoot, themeWidePages);
        m_tree.addDivider(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        addRoot(k_metricsRoot, {});
        m_tree.addDivider(Thickness::Heavy, Padding{ 4.0f, 6.0f });
        addRoot(k_textStylesRoot, {});
        pickPage(k_paletteToken);
    }

    // A root's row is a step larger than the rows under it. A root holding nothing is an item: it
    // has no rows for a node to open.
    void DesignPage::addRoot(const TreeRoot& root, const std::span<const TreeEntry> pages)
    {
        const TreeEntry rootEntry{ .root = &root };
        if (pages.empty() && root.categories.empty())
        {
            TreeItem& item = m_tree.addItem(TextFormat{ TextStyles::SubHeading });
            buildIndexPage(addItemEntry(item, rootEntry), {});
            return;
        }
        HeaderText header{};
        writeRowText(header, rootEntry);
        TreeNode& node = m_tree.addNode(std::move(header));
        node.header().setTextFormat(TextFormat{ TextStyles::SubHeading });
        const std::size_t rootIndex = addEntry(node.header(), rootEntry);
        EntryIndexes members{};
        for (const TreeEntry& page : pages)
            members.push_back(addItemEntry(node.addItem(), page));
        if (!pages.empty() && !root.categories.empty())
            node.addDivider();
        for (const Category& category : root.categories)
            members.push_back(addCategory(node, category));
        m_entries[rootIndex].preview = folderPreview(members);
        buildIndexPage(rootIndex, members);
    }

    // The header is a row of its own, and its page lists the pages under it - built once they
    // stand.
    std::size_t DesignPage::addCategory(TreeNode& rootNode, const Category& category)
    {
        const TreeEntry categoryEntry{ .category = &category };
        HeaderText header{};
        writeRowText(header, categoryEntry);
        TreeNode& node = rootNode.addNode(std::move(header));
        const std::size_t categoryIndex = addEntry(node.header(), categoryEntry);
        EntryIndexes members{};
        for (const CategoryEntry& entry : category.entries)
            members.push_back(addItemEntry(node.addItem(), entryOf(entry)));
        m_entries[categoryIndex].preview = folderPreview(members);
        buildIndexPage(categoryIndex, members);
        return categoryIndex;
    }

    std::size_t DesignPage::addItemEntry(TreeItem& item, TreeEntry entry)
    {
        writeRowText(item.text(), entry);
        return addEntry(item, std::move(entry));
    }

    std::size_t DesignPage::addEntry(RichControl& row, TreeEntry entry)
    {
        const std::size_t index = m_entries.size();
        row.setTag(Tag{ index });
        entry.item = &row;
        // A page of rules or of sizes is made here - the palette's page stands already, and a
        // root's or a category's is built once the pages it lists do.
        if (entry.sizes)
        {
            entry.sizesPage = &m_pages.add<SizesPage>(*entry.sizes);
            entry.page = entry.sizesPage;
        }
        else if (entry.element || entry.shared)
        {
            entry.rules = &m_pages.add<ElementPage>(nameOf(entry));
            entry.page = entry.rules;
        }
        m_entries.push_back(std::move(entry));
        return index;
    }

    // A folder is a set of sizes where every member is one, or a folder holding nothing else.
    NodePreview DesignPage::folderPreview(const EntryIndexes& members) const
    {
        if (members.empty())
            return {};
        const PreviewPage firstPage = m_entries[members.front()].preview.page;
        const bool samePage = std::ranges::all_of(members, [this, firstPage](std::size_t member) {
            return m_entries[member].preview.page == firstPage;
        });
        const bool editsSizes = std::ranges::all_of(members, [this](std::size_t member) {
            return m_entries[member].preview.editsSizes;
        });
        return {
            .page = samePage ? firstPage : PreviewPage::Overview,
            .editsSizes = editsSizes
        };
    }

    // Titled as an element page is, with a row per page or category under it. A row's text is
    // written when the page is shown, since it carries a count that edits move.
    void DesignPage::buildIndexPage(const std::size_t entryIndex, const EntryIndexes& members)
    {
        TreeEntry& entry = m_entries[entryIndex];
        Panel& page = m_pages.add<Panel>();
        Label& title = page.createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ).createBody<Label>(
            VerticalTextAnchor::Center
        );
        writePageTitle(title.text(), kindOf(entry), nameOf(entry));
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

    void DesignPage::refreshIndex(const TreeEntry& entry)
    {
        for (RichControl* row : entry.indexRows)
            row->text() = indexRowText(m_entries[row->tag<std::size_t>()]);
    }

    void DesignPage::showPickedPage()
    {
        const TreeEntry* entry = pickedEntry();
        if (!entry)
            return;
        if (entry->root || entry->category)
            refreshIndex(*entry);
        m_pages.setCurrentItem(entry->page);
    }

    const DesignPage::TreeEntry* DesignPage::pickedEntry() const
    {
        if (const Control* item = m_tree.currentItem())
            return &m_entries[item->tag<std::size_t>()];
        return nullptr;
    }

    DesignPage::TreeEntry DesignPage::entryOf(const CategoryEntry& entry)
    {
        return {
            .element = entry.element,
            .shared = entry.shared,
            .sizes = entry.sizes,
            .preview = { .page = entry.preview, .editsSizes = entry.sizes.has_value() }
        };
    }

    // A root's icon is a step larger, as its words are. A category's name is muted, setting it
    // apart from the pages under it.
    void DesignPage::writeRowText(Text& text, const TreeEntry& entry)
    {
        const float iconSize = entry.root ? k_rootIconSize : RowIcons::Size;
        text << pageIcon(kindOf(entry), iconSize) << Space{ RowIcons::Gap };
        if (entry.category)
            text << ColoredText{ InkGrade::Muted, nameOf(entry) };
        else
            text << nameOf(entry);
        const std::wstring_view kindWord = kindWordOf(kindOf(entry));
        if (!kindWord.empty())
            RowIcons::writeKindWord(text, kindWord);
    }

    // The page's icon and name, followed for a page of rules by how many it holds, muted.
    Text DesignPage::indexRowText(const TreeEntry& entry)
    {
        Text text{
            pageIcon(kindOf(entry), RowIcons::Size),
            Space{ RowIcons::Gap },
            nameOf(entry)
        };
        if (!entry.rules)
            return text;
        const std::size_t count = entry.list ? entry.list->size() : 0;
        Text tally{};
        if (count == 0)
            tally << L"no rules";
        else
            tally << count << (count == 1 ? L" rule" : L" rules");
        text << L"  " << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, tally } };
        return text;
    }

    PageKind DesignPage::kindOf(const TreeEntry& entry)
    {
        if (entry.root || entry.category)
            return PageKind::Folder;
        if (entry.sizes)
            return PageKind::Sizes;
        if (entry.element || entry.shared)
            return PageKind::Rules;
        return PageKind::Palette;
    }

    std::wstring_view DesignPage::nameOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).name;
        if (entry.sizes)
            return metricsSetOf(*entry.sizes).name;
        if (entry.shared)
            return entry.shared->name;
        if (entry.root)
            return entry.root->name;
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
        if (entry.root)
            return entry.root->token;
        if (entry.category)
            return entry.category->token;
        return k_paletteToken;
    }
}
