export module ThisApp.Design2Page;

import ThisApp.ElementPage;
import ThisApp.RuleSlider;

import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TreeView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The colour a rule of an element's list, or of a shared list, is applied to.
    export using OnGetElementRuleBase =
        std::function<RuleBase(OptionalUiElement, const ColorRule2&, RuleChannel)>;

    // A list of rules no one element owns, and what its tree item and its page go by.
    struct SharedRules
    {
        std::wstring_view name{};
        std::wstring_view token{};
        ColorRules2 ThemeRules2::* rules{ nullptr };
    };

    // A theme's design under the new color theme architecture, one element page at a time.
    export class Design2Page : public Panel
    {
    public:
        template<typename... Args>
        explicit Design2Page(const CreateParams&, Args&&...);
    public:
        // The token of the page that shows - an element's or a shared list's - or nothing yet.
        [[nodiscard]] std::wstring_view pickedPage() const;
        void pickPage(std::wstring_view token);
        // Connects a handler raised after another page is picked.
        template<typename F>
        EventConnection onPagePick(F&& callback);
        // Hands every page its list of the theme's rules, its ramps' base and what to call.
        void bind(ThemeColors&, const OnGetElementRuleBase&, const OnRulesChanged&);
        // Builds every page's rows again from the rules as they stand.
        void rebuildRules();
    private:
        // An item of the tree and the page it opens, named by the element whose rules it shows.
        struct TreeEntry
        {
            OptionalUiElement element{}; // nothing for a shared list
            const SharedRules* shared{}; // null for an element's own list
            Control* item{};
            ElementPage* page{};
        };
        using TreeEntries = std::vector<TreeEntry>;
    private:
        void buildTree();
        void addEntry(TreeItem&, TreeEntry);
        void showPickedPage();
        [[nodiscard]] const TreeEntry* pickedEntry() const;
        [[nodiscard]] static std::wstring_view tokenOf(const TreeEntry&);
    private:
        TreeEntries m_entries{}; // an item's tag is its place here

        TreeView& m_tree{ createLeftBar<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section
        ).createBody<TreeView>(
            Padding{ 4.0f }
        ) };

        PageControl& m_pages{ createBody<PageControl>() };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    Design2Page::Design2Page(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        buildTree();
    }

    template<typename F>
    EventConnection Design2Page::onPagePick(F&& callback)
    {
        return m_tree.onCurrentItemChange(std::forward<F>(callback));
    }
}
