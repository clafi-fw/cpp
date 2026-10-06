export module Themes_App.DesignPage;

import Themes_App.ApplyToControl;
import Themes_App.ElementPage;
import Themes_App.RuleSlider;

import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TreeView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
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
        // The token of the page that shows - an element's or a shared list's - or nothing yet.
        [[nodiscard]] std::wstring_view pickedPage() const;
        // Picks and shows the page a token names, and the first page for a token naming none.
        void pickPage(std::wstring_view token);
        // Picks the page a place names and puts its grid's selection back, once the rows stand.
        void showPlace(const DesignPlace&);
        // Connects a handler raised when the tree picks another page, which pickPage then shows.
        template<typename F>
        EventConnection onPagePick(F&& callback);
        // The Palette page's body, which the theme page fills.
        [[nodiscard]] Stack& paletteView() { return m_paletteView; }
        // Hands every page its list of the theme's rules, its ramps' base and what to call.
        void bind(ThemeColors&, const OnGetElementRuleBase&, const OnRulesChanged&);
        // Builds every page's rows again from the rules as they stand.
        void rebuildRules();
    protected:
        void visibilityChanged() override;
    private:
        // An item of the tree and the page it opens.
        struct TreeEntry
        {
            OptionalUiElement element{}; // nothing for a shared list and for the palette
            const SharedRules* shared{}; // null for an element's own list and for the palette
            Control* item{};
            Control* page{};
            ElementPage* rules{}; // the page, where it shows a list of rules
        };
        using TreeEntries = std::vector<TreeEntry>;
    private:
        void buildTree();
        [[nodiscard]] TreeItem& addRootItem(); // a theme-wide page's row, a step larger
        void addEntry(TreeItem&, TreeEntry);
        void showPickedPage();
        [[nodiscard]] const TreeEntry* pickedEntry() const;
        [[nodiscard]] static std::wstring_view nameOf(const TreeEntry&);
        [[nodiscard]] static std::wstring_view tokenOf(const TreeEntry&);
    private:
        static constexpr std::wstring_view k_paletteTitle{ L"Palette" };

        TreeEntries m_entries{}; // an item's tag is its place here
        const ThemeRules m_defaultRules{ ThemeColors{}.rules }; // what each page's reset puts back

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
            VerticalTextAnchor::Center,
            Text{ TextStyleId::SubTitle, k_paletteTitle }
        ) };

        Stack& m_paletteView{ m_palettePage.createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<Stack>(
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
    }

    template<typename F>
    EventConnection DesignPage::onPagePick(F&& callback)
    {
        return m_tree.onCurrentItemChange(std::forward<F>(callback));
    }
}
