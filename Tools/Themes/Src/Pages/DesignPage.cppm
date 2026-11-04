export module Themes_App.DesignPage;

import Themes_App.ApplyToControl;
import Themes_App.ElementPage;
import Themes_App.PageKinds;
import Themes_App.PreviewPage;
import Themes_App.RuleSlider;
import Themes_App.SizesPage;

import ClaFi.Controls.InPlaceEdit;
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
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The colour a rule of a set is applied to, the set named as the theme names it.
    export using OnGetElementRuleBase =
        std::function<RuleBase(std::wstring_view set, const ColorRule&, RuleChannel)>;

    // A set no framework element names - a base the sets of a kind stand over, or one of this
    // application's own - and what its tree item and its page go by.
    struct NamedSet
    {
        std::wstring_view name{};
        std::wstring_view token{};
        PaintChannels outputs{}; // the channels the set's rules may write
        PreviewPage preview{ PreviewPage::Overview };
    };

    // A page a branch of the tree lists - a set of rules or a set of sizes - and its preview page.
    struct CategoryEntry
    {
        // Not explicit, so a branch lists its entries as braced pairs.
        constexpr CategoryEntry(UiElement value, PreviewPage page)
            :
            element{ value },
            preview{ page }
        {
        }
        // The set names its own preview page.
        constexpr CategoryEntry(const NamedSet& value)
            :
            named{ &value },
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
        const NamedSet* named{};
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
        // The token of the page that shows - a set's or a category's - or nothing yet.
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
        // Hands every page its set of the theme's rules, its ramps' base and what to call.
        void bind(ThemeColors&, const OnGetElementRuleBase&, const OnRulesChanged&);
        // Hands every sizes page the theme's sizes and what to call after an edit.
        void bindSizes(ThemeMetrics&, const OnSizesChanged&);
        // Lists the theme's own sets again and builds every page's rows from the rules as they
        // stand.
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
            OptionalUiElement element{}; // an element's set, nothing for every other row
            const NamedSet* named{};     // a set no element names, null for every other row
            std::wstring ownSet{};       // a set of the theme's own, empty for every other row
            const TreeRoot* root{};      // a root's row, null for every other row
            const Category* category{};  // a category's node, null for every other row
            OptionalMetricsSet sizes{};  // a set of sizes, none for every other row
            NodePreview preview{};       // a folder's is its members', where they agree
            Control* item{};             // the row: an item, or a node's header
            TreeNode* node{};            // the node, where the row heads one
            Control* page{};
            ElementPage* rules{};        // the page, where it shows a set of rules
            SizesPage* sizesPage{};      // the page, where it shows a set of sizes
            StackPanel* indexBody{};     // where a root's or a category's page lists its rows
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
        // Adds a row to an index page, opening the entry at the index.
        void addIndexRow(TreeEntry& page, std::size_t index);
        void openEntry(std::size_t index);
        // Writes each row of an index page again, with the rule count as it stands.
        void refreshIndex(const TreeEntry&);
        void bindPage(std::size_t index); // hands a page of rules its set and what to call
        // Lists the theme's own sets under their category as they stand, a row and a page each.
        void syncOwnSets();
        // The names of the sets the theme states that no row of the framework's shows.
        [[nodiscard]] std::vector<std::wstring> ownSetNames() const;
        // Every set a row shows, as the tree names it, the framework's first.
        [[nodiscard]] SetNames setNames() const;
        // Whether the theme states a set the typed name would clash with, by token or by name.
        [[nodiscard]] bool namesSet(std::wstring_view typed) const;
        // Adds a set of the theme's own over Any Non-Window, and opens its row for its name.
        void addSet();
        // Opens the row of the set just added for its name, once the rows stand.
        void nameAddedSet();
        // Takes the name typed over a set's row, or refuses it; the rename follows on a tick.
        void renameSet(std::size_t index, AcceptEditEvent&);
        void renamePendingSet();
        void deleteSet(std::size_t index); // asked on the set's own page, which goes on a tick
        void deletePendingSet();
        // Records a step made on the page that shows: a set added, renamed or deleted.
        void setsChanged(const Text& what);
        void showPickedPage();
        [[nodiscard]] const TreeEntry* pickedEntry() const;
        // The row a branch lists for the entry, with the preview page the entry names.
        [[nodiscard]] static TreeEntry entryOf(const CategoryEntry&);
        // Writes a tree row's text: its kind's icon, its name and its kind word.
        static void writeRowText(Text&, const TreeEntry&);
        [[nodiscard]] Text indexRowText(const TreeEntry&) const;
        [[nodiscard]] static PageKind kindOf(const TreeEntry&);
        [[nodiscard]] static std::wstring_view nameOf(const TreeEntry&);
        [[nodiscard]] static std::wstring_view tokenOf(const TreeEntry&);
        // The set a row's page shows, as the theme names it - empty for every other row.
        [[nodiscard]] static std::wstring_view setOf(const TreeEntry&);
        // The channels a set's rules may write - every channel where nothing says otherwise.
        [[nodiscard]] static PaintChannels outputsOf(const TreeEntry&);
    private:
        static constexpr std::wstring_view k_paletteTitle{ L"Palette" };
        static constexpr float k_rootIconSize = 16.0f; // beside a root's SubHeading words

        TreeEntries m_entries{}; // an item's tag is its place here
        std::size_t m_staticCount{}; // the entries the tree starts with; the theme's own follow
        std::size_t m_ownSetsIndex{}; // the entry of the category the theme's own sets stand in
        ThemeColors* m_colors{}; // the theme's colours, from bind on
        ThemeRules* m_rules{}; // the theme's sets, from bind on
        OnGetElementRuleBase m_ruleBase{}; // what every page's ramps read, from bind on
        OnRulesChanged m_onRulesChanged{}; // what every page calls after an edit, from bind on
        const ThemeRules m_defaultRules{ ThemeColors{}.rules }; // what each page's reset puts back
        const ThemeMetrics m_defaultMetrics{}; // what each sizes page's reset puts back
        // Each of the three waits out the event that asked - a click on the page that goes, an
        // editor over the row that goes - and acts on the set named beside it.
        UiTimer m_nameTimer{};
        std::wstring m_setToName{};
        UiTimer m_renameTimer{};
        std::wstring m_setToRename{};
        std::wstring m_newName{};
        UiTimer m_deleteTimer{};
        std::wstring m_setToDelete{};

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
        m_nameTimer.onTick([this](TimerEvent&) {
            nameAddedSet();
        });
        m_renameTimer.onTick([this](TimerEvent&) {
            renamePendingSet();
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingSet();
        });
        // A show enters the picked page, past the tree.
        setCurrentItem(m_pages);
    }

    template<typename F>
    EventConnection DesignPage::onPagePick(F&& callback)
    {
        return m_tree.onCurrentItemChange(std::forward<F>(callback));
    }
}
