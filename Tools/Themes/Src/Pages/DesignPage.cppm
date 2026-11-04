export module Themes_App.DesignPage;

import Themes_App.ApplyToControl;
import Themes_App.ElementPage;
import Themes_App.PageKinds;
import Themes_App.PreviewPage;
import Themes_App.RuleSlider;
import Themes_App.SizesPage;

import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The colour a rule of an element's list, or of a shared list, is applied to.
    export using OnGetElementRuleBase =
        std::function<RuleBase(OptionalUiElement, const ColorRule&, RuleChannel)>;

    // A list of rules no one element owns, and what its tree item and its page go by.
    struct SharedRules
    {
        std::wstring_view name{};
        std::wstring_view token{};
        ColorRules ThemeRules::* rules{ nullptr };
        PaintChannels outputs{}; // the channels the list's rules may write
        PreviewPage preview{ PreviewPage::Overview };
    };

    // A page a branch of the tree lists - rules, a shared list or sizes - and its preview page.
    struct CategoryEntry
    {
        // Not explicit, so a branch lists its entries as braced pairs.
        constexpr CategoryEntry(UiElement value, PreviewPage page)
            :
            element{ value },
            preview{ page }
        {
        }
        // The list names its own preview page.
        constexpr CategoryEntry(const SharedRules& value)
            :
            shared{ &value },
            preview{ value.preview }
        {
        }
        constexpr CategoryEntry(MetricsSet value, PreviewPage page)
            :
            sizes{ value },
            preview{ page }
        {
        }
        OptionalUiElement element{};
        const SharedRules* shared{};
        OptionalMetricsSet sizes{};
        PreviewPage preview{ PreviewPage::Overview };
    };

    // A branch of the tree: its name, what its page goes by, and the pages under it in the order
    // they are listed.
    struct Category
    {
        std::wstring_view name{};
        std::wstring_view token{};
        std::span<const CategoryEntry> entries{};
    };

    // A root of the tree: its name, what its page goes by, and the categories under it.
    struct TreeRoot
    {
        std::wstring_view name{};
        std::wstring_view token{};
        std::span<const Category> categories{};
    };

    // Where in the design an edit is made, put back beside the theme an undo restores.
    export struct DesignPlace
    {
        std::wstring page{}; // the token of the page the tree has picked
        RuleSelection selection{}; // what that page's grid held, nothing for the palette
    };

    // A theme's design, one element page at a time.
    export class DesignPage : public Panel
    {
    public:
        template<typename... Args>
        explicit DesignPage(const CreateParams&, Args&&...);
    public:
        // The token of the page that shows - an element's, a shared list's or a category's - or
        // nothing yet.
        [[nodiscard]] std::wstring_view pickedPage() const;
        // What the preview shows for the node picked - the Overview while none is.
        [[nodiscard]] NodePreview pickedPreview() const;
        // Picks and shows the page a token names, and the palette for a token naming none.
        void pickPage(std::wstring_view token);
        // Picks the page a place names and puts its grid's selection back, once the rows stand.
        void showPlace(const DesignPlace&);
        // Connects a handler raised when the tree picks another page, which pickPage then shows.
        template<typename F>
        EventConnection onPagePick(F&& callback);
        // The Palette page's body, which the theme page fills.
        [[nodiscard]] StackPanel& paletteView() { return m_paletteView; }
        // Hands every page its list of the theme's rules, its ramps' base and what to call.
        void bind(ThemeColors&, const OnGetElementRuleBase&, const OnRulesChanged&);
        // Hands every sizes page the theme's sizes and what to call after an edit.
        void bindSizes(ThemeMetrics&, const OnSizesChanged&);
        // Builds every page's rows again from the rules as they stand.
        void rebuildRules();
        // Shows every sizes page's sizes as they stand.
        void showSizes();
    protected:
        void visibilityChanged() override;
    private:
        using IndexRows = std::vector<RichControl*>;
        using EntryIndexes = std::vector<std::size_t>;

        // A row of the tree and the page it opens.
        struct TreeEntry
        {
            OptionalUiElement element{}; // an element's own list, nothing for every other row
            const SharedRules* shared{}; // a list no one element owns, null for every other row
            const TreeRoot* root{};      // a root's row, null for every other row
            const Category* category{};  // a category's node, null for every other row
            OptionalMetricsSet sizes{};  // a set of sizes, none for every other row
            NodePreview preview{};       // a folder's is its members', where they agree
            Control* item{};             // the row: an item, or a node's header
            Control* page{};
            ElementPage* rules{};        // the page, where it shows a list of rules
            SizesPage* sizesPage{};      // the page, where it shows a set of sizes
            const ColorRules* list{};    // the rules that page shows, from bind on
            IndexRows indexRows{};       // the rows of a root's or a category's page
        };
        using TreeEntries = std::vector<TreeEntry>;
    private:
        void buildTree();
        // Adds a root's row and page - under it the pages given, then a node per category.
        void addRoot(const TreeRoot&, std::span<const TreeEntry> pages);
        // Adds a category's node, its rows and its page under a root, and answers the index.
        std::size_t addCategory(TreeNode& rootNode, const Category&);
        std::size_t addItemEntry(TreeItem&, TreeEntry); // names the item, answers the index
        std::size_t addEntry(RichControl& row, TreeEntry); // answers the entry's index
        // The page a folder's members all show, and the Overview where they differ.
        [[nodiscard]] NodePreview folderPreview(const EntryIndexes& members) const;
        // Builds a root's or a category's page - a row that opens each page or category under it.
        void buildIndexPage(std::size_t entryIndex, const EntryIndexes& members);
        void openEntry(std::size_t index);
        // Writes each row of an index page again, with the rule count as it stands.
        void refreshIndex(const TreeEntry&);
        void showPickedPage();
        [[nodiscard]] const TreeEntry* pickedEntry() const;
        // The row a branch lists for the entry, with the preview page the entry names.
        [[nodiscard]] static TreeEntry entryOf(const CategoryEntry&);
        // Writes a tree row's text: its kind's icon, its name and its kind word.
        static void writeRowText(Text&, const TreeEntry&);
        [[nodiscard]] static Text indexRowText(const TreeEntry&);
        [[nodiscard]] static PageKind kindOf(const TreeEntry&);
        [[nodiscard]] static std::wstring_view nameOf(const TreeEntry&);
        [[nodiscard]] static std::wstring_view tokenOf(const TreeEntry&);
    private:
        static constexpr std::wstring_view k_paletteTitle{ L"Palette" };
        static constexpr float k_rootIconSize = 16.0f; // beside a root's SubHeading words

        TreeEntries m_entries{}; // an item's tag is its place here
        const ThemeRules m_defaultRules{ ThemeColors{}.rules }; // what each page's reset puts back
        const ThemeMetrics m_defaultMetrics{}; // what each sizes page's reset puts back

        TreeView& m_tree{ createLeftBar<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section
        ).createBody<TreeView>(
            Padding{ 4.0f }
        ) };

        PageControl& m_pages{ createBody<PageControl>() };

        // The theme-wide settings, titled as an element page is.
        Panel& m_palettePage{ m_pages.add<Panel>() };

        Label& m_paletteTitle{ m_palettePage.createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ).createBody<Label>(
            VerticalTextAnchor::Center
        ) };

        StackPanel& m_paletteView{ m_palettePage.createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f },
            Spacing{ 8.0f }
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    DesignPage::DesignPage(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        buildTree();
        // A show enters the picked page, past the tree.
        setCurrentItem(m_pages);
    }

    template<typename F>
    EventConnection DesignPage::onPagePick(F&& callback)
    {
        return m_tree.onCurrentItemChange(std::forward<F>(callback));
    }
}
