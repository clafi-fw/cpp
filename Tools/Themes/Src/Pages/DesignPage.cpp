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
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;

import ClaFi.Icons.PlusMark;
import ClaFi.Icons.RowIcons;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace RowIcons = ::ClaFi::Icons::RowIcons;

    namespace
    {
        constexpr std::array k_selectedTextOutputs{ PaintChannel::Surface, PaintChannel::Text };
        constexpr std::array k_foundTextOutputs{ PaintChannel::Surface };

        constexpr NamedSet k_anyElement{
            .name = L"Any Element",
            .token = BaseElements::AnyElement
        };

        constexpr NamedSet k_anyWindow{
            .name = L"Any Window",
            .token = BaseElements::AnyWindow,
            .preview = PreviewPage::Windows
        };

        constexpr NamedSet k_anyNonWindow{
            .name = L"Any Non-Window",
            .token = BaseElements::AnyNonWindow
        };

        constexpr NamedSet k_anyButton{
            .name = L"Any Button",
            .token = BaseElements::AnyButton,
            .preview = PreviewPage::Inputs
        };

        constexpr NamedSet k_anyIndicator{
            .name = L"Any Indicator",
            .token = BaseElements::AnyIndicator,
            .preview = PreviewPage::Selection
        };

        constexpr NamedSet k_anyCallout{
            .name = L"Any Callout",
            .token = BaseElements::AnyCallout,
            .preview = PreviewPage::Callouts
        };

        constexpr NamedSet k_doDont{
            .name = L"Do and Don't",
            .token = BaseElements::DoDont,
            .preview = PreviewPage::DoDont
        };

        constexpr NamedSet k_testee{
            .name = k_testeeName,
            .token = k_testeeName,
            .preview = PreviewPage::TestSubjects
        };

        constexpr NamedSet k_bestee{
            .name = k_besteeName,
            .token = k_besteeName,
            .preview = PreviewPage::TestSubjects
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
            { k_anyButton },
            { UiElement::Button, PreviewPage::Inputs },
            { UiElement::ToolButton, PreviewPage::Inputs },
            { UiElement::TextBox, PreviewPage::Inputs },
            { UiElement::Tab, PreviewPage::Tabs },
            { UiElement::ScrollButton, PreviewPage::Scrolling },
            { UiElement::ScrollThumb, PreviewPage::Scrolling }
        });

        constexpr auto k_focusAndSelection = std::to_array<CategoryEntry>({
            { k_anyIndicator },
            { UiElement::SelectionIndicator, PreviewPage::Selection },
            { UiElement::HoverIndicator, PreviewPage::Selection },
            { UiElement::SelectedText, PreviewPage::Selection },
            { UiElement::FoundText, PreviewPage::Selection }
        });

        constexpr auto k_blocks = std::to_array<CategoryEntry>({
            { k_anyCallout },
            { UiElement::InfoCallout, PreviewPage::Callouts },
            { UiElement::QuestionCallout, PreviewPage::Callouts },
            { UiElement::WarningCallout, PreviewPage::Callouts },
            { UiElement::ErrorCallout, PreviewPage::Callouts },
            { UiElement::OkCallout, PreviewPage::Callouts },
            { k_doDont },
            { UiElement::Do, PreviewPage::DoDont },
            { UiElement::Dont, PreviewPage::DoDont },
            { UiElement::CodeBlock, PreviewPage::CodeBlock }
        });

        constexpr auto k_testSubjects = std::to_array<CategoryEntry>({
            { k_testee },
            { k_bestee }
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

        // The theme's own sets stand under the last category, listed as the theme is read.
        constexpr std::wstring_view k_ownSetsToken{ L"OwnSets" };

        constexpr std::array k_colorCategories{
            Category{ L"Window roots", L"WindowRoots", k_windowRoots },
            Category{ L"Surfaces", L"Surfaces", k_surfaces },
            Category{ L"Controls", L"Controls", k_controls },
            Category{ L"Focus & Selection", L"FocusAndSelection", k_focusAndSelection },
            Category{ L"Blocks", L"Blocks", k_blocks },
            Category{ L"Test subjects", L"TestSubjects", k_testSubjects },
            Category{ L"Own sets", k_ownSetsToken, {} }
        };

        // What a set of the theme's own is called until it is named, counted up past the first.
        constexpr std::wstring_view k_newSetName{ L"New set" };

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

        // A row for a set of the theme's own, which opens for the set's name.
        class OwnSetRow : public WithInPlaceEdit<TreeItem>
        {
        public:
            template<typename... Args>
            OwnSetRow(const CreateParams&, TreeView& tree, std::size_t level, Args&&...);
        public:
            void setSetName(const std::wstring_view value) { m_setName = value; }
        protected:
            // The name alone: the icon and the kind word around it are the row's, not the set's.
            void getEditorText(Text& text) const override { text << m_setName; }
        private:
            std::wstring m_setName{};
        };

        template<typename... Args>
        OwnSetRow::OwnSetRow(const CreateParams& params, TreeView& tree, const std::size_t level,
            Args&&... args)
            :
            WithInPlaceEdit<TreeItem>{ params, tree, level, std::forward<Args>(args)... }
        {
        }

        [[nodiscard]] std::wstring_view trimmed(std::wstring_view text)
        {
            trimLeft(text);
            trimRight(text);
            return text;
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

    // A token no page goes by names a set the theme no longer states - undone, or another
    // theme's - so it lands where that set would be listed. No token at all is the palette.
    void DesignPage::pickPage(const std::wstring_view token)
    {
        auto found = std::ranges::find(m_entries, token, &DesignPage::tokenOf);
        if (found == m_entries.end())
        {
            const std::wstring_view fallback = token.empty() ? k_paletteToken : k_ownSetsToken;
            found = std::ranges::find(m_entries, fallback, &DesignPage::tokenOf);
        }
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

    // The theme's own sets are listed once the theme is known, their pages bound as they are made.
    void DesignPage::bind(ThemeColors& colors, const OnGetElementRuleBase& ruleBase,
        const OnRulesChanged& onRulesChanged)
    {
        m_colors = &colors;
        m_rules = &colors.rules;
        m_ruleBase = ruleBase;
        m_onRulesChanged = onRulesChanged;
        for (std::size_t i = 0ull; i != m_staticCount; ++i)
            bindPage(i);
        syncOwnSets();
    }

    void DesignPage::bindSizes(ThemeMetrics& metrics, const OnSizesChanged& onSizesChanged)
    {
        for (const TreeEntry& entry : m_entries)
            if (entry.sizesPage)
                entry.sizesPage->bind(metrics, m_defaultMetrics, onSizesChanged);
    }

    void DesignPage::rebuildRules()
    {
        syncOwnSets();
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
        m_staticCount = m_entries.size();
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
        m_entries[categoryIndex].node = &node;
        if (category.token == k_ownSetsToken)
            m_ownSetsIndex = categoryIndex;
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
        else if (entry.element || entry.named || !entry.ownSet.empty())
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
    // written when the page is shown, since it carries a count that edits move. The page of the
    // theme's own sets carries the tool that adds one.
    void DesignPage::buildIndexPage(const std::size_t entryIndex, const EntryIndexes& members)
    {
        TreeEntry& entry = m_entries[entryIndex];
        Panel& page = m_pages.add<Panel>();
        Panel& topBar = page.createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        );
        Label& title = topBar.createBody<Label>(
            VerticalTextAnchor::Center
        );
        writePageTitle(title.text(), kindOf(entry), nameOf(entry));
        if (entry.category && entry.category->token == k_ownSetsToken)
        {
            ToolButton& addButton = topBar.createRightBar<ToolButton>(
                IconSize{ 18.0f },
                ButtonViewMode::IconOnly,
                OnDrawIcon{ Icons::PlusMark::draw },
                L"Add set"
            );
            addButton.onClick([this](ClickEvent&) {
                addSet();
            });
        }
        entry.indexBody = &page.createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f }
        );
        entry.page = &page;
        for (const std::size_t member : members)
            addIndexRow(entry, member);
    }

    void DesignPage::addIndexRow(TreeEntry& page, const std::size_t index)
    {
        ToolButton& row = page.indexBody->add<ToolButton>(HorizontalTextAnchor::Left);
        row.setTag(Tag{ index });
        row.onClick([this, index](ClickEvent&) {
            openEntry(index);
        });
        page.indexRows.push_back(&row);
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
            .named = entry.named,
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
    Text DesignPage::indexRowText(const TreeEntry& entry) const
    {
        Text text{
            pageIcon(kindOf(entry), RowIcons::Size),
            Space{ RowIcons::Gap },
            nameOf(entry)
        };
        if (!entry.rules)
            return text;
        const ElementRules* set = m_rules ? m_rules->of(setOf(entry)) : nullptr;
        const std::size_t count = set ? set->rules.size() : 0;
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
        if (entry.element || entry.named || !entry.ownSet.empty())
            return PageKind::Rules;
        return PageKind::Palette;
    }

    // A set of the theme's own goes by the name the theme states it under.
    std::wstring_view DesignPage::nameOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).name;
        if (entry.sizes)
            return metricsSetOf(*entry.sizes).name;
        if (entry.named)
            return entry.named->name;
        if (!entry.ownSet.empty())
            return entry.ownSet;
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
        if (entry.named)
            return entry.named->token;
        if (!entry.ownSet.empty())
            return entry.ownSet;
        if (entry.root)
            return entry.root->token;
        if (entry.category)
            return entry.category->token;
        return k_paletteToken;
    }

    std::wstring_view DesignPage::setOf(const TreeEntry& entry)
    {
        if (entry.element)
            return uiElementOf(*entry.element).token;
        if (entry.named)
            return entry.named->token;
        return entry.ownSet;
    }

    // A text band has no stroke and casts no shadow, and only the selection re-inks its text.
    PaintChannels DesignPage::outputsOf(const TreeEntry& entry)
    {
        if (entry.named)
            return entry.named->outputs;
        if (entry.element == UiElement::SelectedText)
            return k_selectedTextOutputs;
        if (entry.element == UiElement::FoundText)
            return k_foundTextOutputs;
        return {};
    }

    // Only a set of the theme's own can be taken out: a framework set's page offers no Delete set.
    void DesignPage::bindPage(const std::size_t index)
    {
        TreeEntry& entry = m_entries[index];
        if (!entry.rules)
            return;
        const std::wstring set{ setOf(entry) };
        const ElementRules* defaults = m_defaultRules.of(set);
        const OnGetElementRuleBase ruleBase = m_ruleBase;
        PageBinding binding{
            .rules = m_rules,
            .set = set,
            .defaults = defaults,
            .outputs = outputsOf(entry),
            .colors = m_colors,
            .ruleBase = [ruleBase, set](const ColorRule& rule, const RuleChannel channel) {
                return ruleBase(set, rule, channel);
            },
            .setNames = [this]() {
                return setNames();
            },
            .onRulesChanged = m_onRulesChanged,
            .onSetsChanged = [this]() {
                rebuildRules();
            }
        };
        if (!entry.ownSet.empty())
            binding.onDeleteSet = [this, index]() {
                deleteSet(index);
            };
        entry.rules->bind(std::move(binding));
    }

    // **The rows of the theme's own sets are built again whenever the names change.** They stand
    // at the end of the entries, after the rows the tree starts with, so the rows and the pages
    // past that count go and the names as they stand now are listed afresh. A page showing one of
    // them gives way to the category's first, so no page goes from under the view.
    void DesignPage::syncOwnSets()
    {
        if (!m_rules)
            return;
        const std::vector<std::wstring> names = ownSetNames();
        std::vector<std::wstring> shown{};
        for (std::size_t i = m_staticCount; i < m_entries.size(); ++i)
            shown.push_back(m_entries[i].ownSet);
        if (names == shown)
            return;

        const TreeEntry* picked = pickedEntry();
        const std::wstring pickedToken{ picked ? tokenOf(*picked) : std::wstring_view{} };
        if (picked && !picked->ownSet.empty())
            pickPage(k_ownSetsToken);
        for (std::size_t i = m_staticCount; i < m_entries.size(); ++i)
        {
            m_entries[i].item->deleteSelf();
            m_entries[i].page->deleteSelf();
        }
        m_entries.resize(m_staticCount);
        for (RichControl* row : m_entries[m_ownSetsIndex].indexRows)
            row->deleteSelf();
        m_entries[m_ownSetsIndex].indexRows.clear();

        // The entries move as rows are added, so the category is reached by its index each time.
        TreeNode& node = *m_entries[m_ownSetsIndex].node;
        for (const std::wstring& name : names)
        {
            OwnSetRow& row = node.addItem(std::type_identity<OwnSetRow>{});
            row.setSetName(name);
            const std::size_t index = addItemEntry(row, TreeEntry{ .ownSet = name });
            row.onAcceptEdit([this, index](AcceptEditEvent& event) {
                renameSet(index, event);
            });
            bindPage(index);
            addIndexRow(m_entries[m_ownSetsIndex], index);
        }
        refreshIndex(m_entries[m_ownSetsIndex]);
        // The page a renamed set stands on again, under its new name.
        if (std::ranges::find(names, pickedToken) != names.end())
            pickPage(pickedToken);
        form().invalidateAlign();
    }

    std::vector<std::wstring> DesignPage::ownSetNames() const
    {
        std::vector<std::wstring> result{};
        for (const ElementRules& set : m_rules->elements)
        {
            bool shown = false;
            for (std::size_t i = 0ull; i != m_staticCount; ++i)
                if (setOf(m_entries[i]) == set.name)
                    shown = true;
            if (!shown)
                result.push_back(set.name);
        }
        return result;
    }

    SetNames DesignPage::setNames() const
    {
        SetNames result{};
        for (const TreeEntry& entry : m_entries)
        {
            if (entry.rules)
                result.push_back(SetName{
                    .token = std::wstring{ setOf(entry) },
                    .name = std::wstring{ nameOf(entry) }
                });
        }
        return result;
    }

    bool DesignPage::namesSet(const std::wstring_view typed) const
    {
        for (const TreeEntry& entry : m_entries)
        {
            if (entry.rules && (setOf(entry) == typed || nameOf(entry) == typed))
                return true;
        }
        return m_rules->of(typed) != nullptr;
    }

    // Over Any Non-Window and on a section, as most sets of the framework stand. The step is
    // made on the new set's page, and then its row opens for the name.
    void DesignPage::addSet()
    {
        std::wstring name{ k_newSetName };
        for (std::size_t count = 2ull; namesSet(name); ++count)
            name = std::format(L"{} {}", k_newSetName, count);
        m_rules->add(ElementRules{
            .name = name,
            .base = std::wstring{ BaseElements::AnyNonWindow },
            .standsOn = std::wstring{ uiElementOf(UiElement::Section).token }
        });
        rebuildRules();
        pickPage(name);
        Text what{};
        what << L"Add set " << ColoredText{ InkWell::accentInk(), name };
        setsChanged(what);
        m_setToName = name;
        m_nameTimer.start(MilliSeconds{ 0u });
    }

    // **Asked for on a tick**, because the click that added the set is still on the stack and
    // the editor runs a message loop of its own. The row is found again by its name: a rebuild
    // may have run in between and taken the row with it.
    void DesignPage::nameAddedSet()
    {
        const std::wstring name = std::exchange(m_setToName, {});
        const auto found = std::ranges::find(m_entries, name, &DesignPage::tokenOf);
        if (found == m_entries.end() || found->ownSet.empty())
            return;
        // The focus, not just the current item: it is still on the button that added the set.
        found->item->setFocus();
        static_cast<OwnSetRow*>(found->item)->openEditor();
    }

    // A name the theme states already is refused. The editor is still over the row, and the
    // rename builds the rows again, so it waits for the editor to go.
    void DesignPage::renameSet(const std::size_t index, AcceptEditEvent& event)
    {
        const std::wstring typed{ trimmed(event.text.plainText()) };
        const std::wstring& before = m_entries[index].ownSet;
        if (typed == before)
            return;
        if (typed.empty())
        {
            event.refuse(L"A set needs a name.");
            return;
        }
        if (namesSet(typed))
        {
            event.refuse(L"The theme states a set of that name already.");
            return;
        }
        m_setToRename = before;
        m_newName = typed;
        m_renameTimer.start(MilliSeconds{ 0u });
    }

    // The name is written over wherever another set names this one as its base or what it
    // stands on, and the step is made on the set's page under its new name.
    void DesignPage::renamePendingSet()
    {
        if (InPlaceEdit::isRunning())
        {
            m_renameTimer.start(MilliSeconds{ 0u });
            return;
        }
        const std::wstring before = std::exchange(m_setToRename, {});
        const std::wstring after = std::exchange(m_newName, {});
        if (!m_rules->of(before) || namesSet(after))
            return;
        for (ElementRules& set : m_rules->elements)
        {
            if (set.name == before)
                set.name = after;
            if (set.base == before)
                set.base = after;
            if (set.standsOn == before)
                set.standsOn = after;
        }
        rebuildRules();
        pickPage(after);
        Text what{};
        what << L"Rename set " << ColoredText{ InkWell::accentInk(), before };
        what << L" to " << ColoredText{ InkWell::accentInk(), after };
        setsChanged(what);
    }

    // Asked by the Delete set tool on the set's own page, which goes with the set - on a tick,
    // outside the click.
    void DesignPage::deleteSet(const std::size_t index)
    {
        m_setToDelete = m_entries[index].ownSet;
        m_deleteTimer.start(MilliSeconds{ 0u });
    }

    // The sets naming this one as their base or what they stand on name none afterwards. The
    // step is made on the category's page, where the set stood.
    void DesignPage::deletePendingSet()
    {
        const std::wstring name = std::exchange(m_setToDelete, {});
        if (!m_rules->of(name))
            return;
        pickPage(k_ownSetsToken);
        for (ElementRules& set : m_rules->elements)
        {
            if (set.base == name)
                set.base.clear();
            if (set.standsOn == name)
                set.standsOn.clear();
        }
        m_rules->remove(name);
        rebuildRules();
        Text what{};
        what << L"Delete set " << ColoredText{ InkWell::accentInk(), name };
        setsChanged(what);
    }

    void DesignPage::setsChanged(const Text& what)
    {
        if (const TreeEntry* entry = pickedEntry())
            if (entry->root || entry->category)
                refreshIndex(*entry);
        if (m_onRulesChanged)
            m_onRulesChanged(EditPhase::Settled, what, RuleSelection{});
    }
}
