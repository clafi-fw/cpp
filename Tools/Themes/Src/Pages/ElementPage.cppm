export module Themes_App.ElementPage;

import Themes_App.ApplyToControl;
import Themes_App.MetricsGrid;
import Themes_App.PageKinds;
import Themes_App.RuleSlider;

import ClaFi.Icons.HueIcon;
import ClaFi.Icons.LuminosityIcon;
import ClaFi.Icons.PlusMark;
import ClaFi.Icons.ResetIcon;
import ClaFi.Icons.SaturationIcon;
import ClaFi.Icons.XMark;

import ClaFi.Controls.Button;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Controls.TextItems;
import ClaFi.Controls.ToolBar;
import ClaFi.Controls.ToolBarBox;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A style as the pages name it: the name the theme states it under, and the words it reads by.
    export struct SetName
    {
        std::wstring token{};
        std::wstring name{};
    };

    export using SetNames = std::vector<SetName>;

    // What an element page holds, by the rules' places since its rows are rebuilt: the selected
    // cell where there is one, and the style's own rules whose rows are held beside it.
    export struct RuleSelection
    {
        struct Cell
        {
            std::size_t level{};  // 0 in the style's own rules, then each base up the chain
            std::size_t rule{};   // the place of the rule the cell stands in, in that style's list
            std::optional<Tag> column{}; // the cell's column, none for a row selected with no cell
        };
        std::optional<Cell> cell{};
        std::vector<std::size_t> held{};
    };

    // Whether a selection put on from code takes the keyboard with it.
    export enum class TakeFocus
    {
        No, // the focus stays where it is - the page is off screen
        Yes
    };

    // What an element page calls after it has changed its style: where the edit stands, its
    // name, and what the grid held as it was made.
    export using OnRulesChanged =
        std::function<void(EditPhase, const Text& what, const RuleSelection& at)>;
    // The colour a rule of the list is applied to, which its value ramps are drawn from.
    export using OnGetListRuleBase = std::function<RuleBase(const ColorRule&, RuleChannel)>;
    // The theme's styles as the tree lists them, asked for as a page's rows are built.
    export using OnGetStyleNames = std::function<SetNames()>;
    // What a page calls to take its style out of the theme - empty for one the framework states.
    export using OnDeleteStyle = std::function<void()>;
    // What a page calls once it has put its style into the theme, which moves every style: every
    // page's editors hold their rules by address, so every page is built again.
    export using OnStylesChanged = std::function<void()>;

    // What a page shows and what it calls, handed over whole.
    export struct PageBinding
    {
        ThemeStyles* styles{};          // the theme's styles
        std::wstring style{};           // the name of the style the page shows
        const ControlStyle* defaults{}; // what a reset puts back, null where there are none
        PaintChannels outputs{};        // the channels the style's rules may write
        const ThemeColors* colors{};    // what the hue editors read the palette from
        OnGetListRuleBase ruleBase{};
        OnGetStyleNames styleNames{};
        OnRulesChanged onRulesChanged{};
        OnDeleteStyle onDeleteStyle{};      // empty for a style the framework states
        OnStylesChanged onStylesChanged{};
    };

    // The grid's columns, as their tags name them.
    enum class RuleColumn : TagValue
    {
        ApplyTo,
        Hue,
        Saturation,
        Elevation
    };

    // One style's page: the three facts it states, then a Rules section - its own rules under
    // their tool bar, and under them the rules its chain gives it, each base's in a group of its
    // own - and a Sizes section. The whole page is one focus scope, as a SeeDocs page is, holding
    // the current item and the rules held for Delete and a drag. See Grids#grid
    export class ElementPage : public Panel
    {
    public:
        template<typename... Args>
        explicit ElementPage(const CreateParams&, std::wstring_view title, Args&&...);
    public:
        void bind(PageBinding);
        // Shows the facts as they stand and builds a row for every rule, own and inherited.
        void rebuild();
        // What the grid holds selected, as an edit made on it is recorded.
        [[nodiscard]] RuleSelection selection() const;
        // Puts a selection back on the rows as they stand; a cell past them lands on the last.
        void select(const RuleSelection&, TakeFocus);
        // Whether Delete has a rule to act on: own rules held, or the cell on a rule of a base
        // that still reaches the style.
        [[nodiscard]] bool canDelete() const;
    private:
        using RuleRows = std::vector<Grids::RowContainer*>;
        using Groups = std::vector<Grids::RowExpander*>;
        // The row of a rule a base gives the style: the base's level in the chain, the rule's
        // place in its list, and whether a style below has dropped it.
        struct InheritedRow
        {
            std::size_t level{};
            std::size_t rule{};
            Grids::RowBase* row{};
            bool dropped{};
        };
        using InheritedRows = std::vector<InheritedRow>;
        // Whether a style below a base has dropped a rule of it, and which.
        struct Dropping
        {
            bool dropped{};
            std::size_t level{}; // the level of the style that dropped it, 0 for this one
        };
    private:
        // The style, and nullptr while the theme states no style of that name.
        [[nodiscard]] ControlStyle* shownStyle() const;
        // The style's own rules, and nullptr while the theme states no style of that name.
        [[nodiscard]] ColorRules* rules() const;
        // The style, made with no base where the theme states none - an edit is about to land.
        [[nodiscard]] ControlStyle& statedStyle();
        void addRule();
        void deleteSelectedRules();
        void deletePendingRules();
        void resetRules();
        void rulesMoved(const ItemsMoveEvent&);
        void addRow(std::size_t index);
        // Puts the page on its first rule while nothing is current, so a show enters it there.
        void selectFirstRule();
        void bindEditors(Grids::RowContainer&);
        [[nodiscard]] bool isOwnRow(const Control&) const; // one of the style's own rules' rows
        // One expander per base of the chain, the nearest first, holding the base's rules.
        void addInheritedGroups();
        void addInheritedRow(Grids::GridBase& body, const StyleChain&, std::size_t level,
            std::size_t rule);
        // Which style of the chain below the level has dropped the rule, the lowest first.
        [[nodiscard]] Dropping droppingOf(const StyleChain&, std::size_t level,
            const ColorRule&) const;
        void dropRule(std::size_t level, std::size_t rule);
        void restoreRule(std::size_t level, std::size_t rule);
        // Fills the three lists with what the style may state and puts its facts on them.
        void showFacts();
        void basePicked();
        void standsOnPicked();
        void seedPicked();
        [[nodiscard]] Grids::Column& column(RuleColumn);
        [[nodiscard]] OnGetRuleBase ruleBaseOf(std::size_t index, RuleChannel) const;
        // The words a style reads by, as the tree names it, and its token where the tree does not.
        [[nodiscard]] std::wstring_view nameOf(std::wstring_view token) const;
        // What a change to one thing of the style is called.
        [[nodiscard]] Text changeStepName(std::wstring_view what) const;
        // What a change to one column of one rule is called - the rule named by its place.
        [[nodiscard]] Text ruleStepName(std::size_t index, std::wstring_view column) const;
        // A rule of a base named by its place and the base, as a drop or a restore is recorded.
        [[nodiscard]] Text inheritedRuleName(const StyleChain&, std::size_t level,
            std::size_t rule) const;
        void rulesChanged(EditPhase, const Text& what, const RuleSelection& at);
    private:
        static constexpr float k_pagePadding = 24.0f;    // at the sides of the page's parts
        static constexpr float k_headPadding = 12.0f;    // above and below the parts
        static constexpr float k_sectionSpacing = 18.0f; // between the parts down the page
        static constexpr float k_headerSpacing = 6.0f;   // between a section's header and its body
        static constexpr float k_toolsSpacing = 4.0f;    // between a tool bar and the grid under it
        const std::wstring m_name; // what the style is called, which every step of it starts with
        PageBinding m_binding{}; // styles null until bound
        OnRuleChanged m_onCellEdit; // what the cells' editors call, held by address
        UiTimer m_deleteTimer{}; // deletes on the next tick, outside the event that asked
        RuleSelection m_pendingDelete{}; // what Delete was asked on, its rows still to go
        // Puts the rows in order on the next tick, outside the move that left them out of it.
        UiTimer m_rebuildTimer{};
        RuleSelection m_pendingSelection{}; // what the rows held before they were rebuilt
        RuleRows m_ruleRows{}; // one per own rule, the header aside
        Groups m_groups{}; // one per base of the chain, the nearest first
        std::set<std::wstring> m_shutBases{}; // the bases whose groups stay shut through rebuilds
        InheritedRows m_inheritedRows{}; // one per rule of a base, dropped ones too
        SetNames m_names{}; // the styles as the tree names them, read as the rows are built
        TextItems m_baseItems{}; // none, then the styles the base may be
        TextItems m_standsOnItems{}; // none, then the styles the style may stand on
        TextItems m_seedItems{}; // none, then the pigments
        std::vector<std::wstring> m_baseTokens{}; // the style each base item stands for
        std::vector<std::wstring> m_standsOnTokens{}; // the style each stands-on item stands for
        bool m_showingFacts{ false }; // set while showFacts puts the picks on, which is no edit

        Panel& m_topBar{ createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ) };

        Label& m_title{ m_topBar.createBody<Label>(
            VerticalTextAnchor::Center
        ) };

        // Reset and Delete style act on the whole style, so they stand with its title.
        ToolBar& m_styleTools{ m_topBar.createRightBar<ToolBar>() };

        ToolButton& m_resetButton{ m_styleTools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::ResetIcon::draw },
            L"Reset to defaults",
            NavigationReach::No,
            MouseFocus::Keep
        ) };

        // Shown on a style of the theme's own: a style the framework states stays.
        ToolButton& m_deleteStyleButton{ m_styleTools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::XMark::draw },
            L"Delete style"
        ) };

        // The scope every item of the page stands in - the facts, the bars' buttons and both
        // grids' rows - and the set the style's own rules are held in.
        ScrollBoxWith<StackView>& m_pageBox{ createBody<ScrollBoxWith<StackView>>(
            HostProps{
                ScrollBars::Vertical
            },
            BodyProps{
                Orientation::Vertical,
                SelectionMode::Multi,
                DragMode::EasyDrag,
                Padding{ k_pagePadding, k_headPadding },
                Spacing{ k_sectionSpacing }
            }
        ) };

        // The three facts a style states beside its rules, each a framed field with its list. The
        // fields wrap where the page is narrower than the three, and stand packed: each is
        // aligned to the start, so the row hands its surplus to none of them.
        StackPanel& m_facts{ m_pageBox.body().add<StackPanel>(
            Orientation::HorizontalWrap,
            Spacing{ 12.0f }
        ) };

        StackPanel& m_baseFact{ m_facts.add<StackPanel>(
            Orientation::Horizontal,
            HorizontalAlign::Left,
            UiElement::Field,
            Spacing{ 6.0f }
        ) };

        Label& m_baseLabel{ m_baseFact.add<Label>(
            Text{ ColoredText{ InkGrade::Muted, L"Base" } },
            VerticalTextAnchor::Center
        ) };

        ComboBox& m_basePick{ m_baseFact.add<ComboBox>(
            m_baseItems
        ) };

        StackPanel& m_standsOnFact{ m_facts.add<StackPanel>(
            Orientation::Horizontal,
            HorizontalAlign::Left,
            UiElement::Field,
            Spacing{ 6.0f }
        ) };

        Label& m_standsOnLabel{ m_standsOnFact.add<Label>(
            Text{ ColoredText{ InkGrade::Muted, L"Stands on" } },
            VerticalTextAnchor::Center
        ) };

        ComboBox& m_standsOnPick{ m_standsOnFact.add<ComboBox>(
            m_standsOnItems
        ) };

        StackPanel& m_seedFact{ m_facts.add<StackPanel>(
            Orientation::Horizontal,
            HorizontalAlign::Left,
            UiElement::Field,
            Spacing{ 6.0f }
        ) };

        Label& m_seedLabel{ m_seedFact.add<Label>(
            Text{ ColoredText{ InkGrade::Muted, L"Seed" } },
            VerticalTextAnchor::Center
        ) };

        ComboBox& m_seedPick{ m_seedFact.add<ComboBox>(
            m_seedItems
        ) };

        Expander& m_rulesSection{ m_pageBox.body().add<Expander>(
            VerticalAlign::Top,
            ExpanderViewMode::Ellipsis,
            Padding{ 0.0f },
            Spacing{ k_headerSpacing },
            HeaderText{ StyledText{ TextStyles::Heading, L"Rules" } }
        ) };

        // The rules under their tool bar, the two as wide as the table.
        ToolBarBox& m_rulesBox{ m_rulesSection.createBody<ToolBarBox>(
            ToolsProps{
                FocusScope::No, // the page is the scope; the bar's buttons are its items
                NavigationReach::No,
                MouseFocus::Parent
            },
            HorizontalAlign::Left,
            Spacing{ 0.0f, k_toolsSpacing }
        ) };

        ToolButton& m_addButton{ m_rulesBox.tools().add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::PlusMark::draw },
            L"Add rule"
        ) };

        // Words, icon and state come from the action; Keep leaves the focus in the grid.
        ToolButton& m_deleteButton{ m_rulesBox.tools().add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            StdActions::del,
            NavigationReach::No,
            MouseFocus::Keep
        ) };

        // A row names its rule by the rule's place in the list, carried as the row's tag.
        Grids::Dt::Grid& m_grid{ m_rulesBox.createBody<Grids::Dt::Grid>(
            UiElement::Grid,
            HorizontalAlign::Left,
            Grids::GridLines::Horizontal,
            FocusScope::No, // the page is the scope; the rows are its items, the held ones its set
            NavigationReach::No,
            MouseFocus::Parent,
            DragMode::EasyDrag,
            ItemsOrder::Rearrangeable,
            Grids::Dt::Columns{
                Grids::Dt::Column{ Tag{ RuleColumn::ApplyTo },
                    Text{ L"Apply to" },
                    Grids::ColumnWidthMode::FitContent
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Hue },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::HueIcon::draw }, L" Hue" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Saturation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::SaturationIcon::draw },
                        L" Saturation" },
                    TextAlign::Right
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Elevation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::draw }, L" Elevation" },
                    TextAlign::Right
                }
            },
            Grids::Dt::Header{}
        ) };

        Expander& m_sizesSection{ m_pageBox.body().add<Expander>(
            VerticalAlign::Top,
            ExpanderViewMode::Ellipsis,
            Padding{ 0.0f },
            Spacing{ k_headerSpacing },
            HeaderText{ StyledText{ TextStyles::Heading, L"Sizes" } }
        ) };

        // The style's sizes under their tool bar: a row per size, with where each one comes from.
        MetricsGrid& m_metricsGrid{ m_sizesSection.createBody<MetricsGrid>(
            HorizontalAlign::Left,
            Spacing{ 0.0f, k_toolsSpacing }
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ElementPage::ElementPage(const CreateParams& params, const std::wstring_view title,
        Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_name{ title },
        m_onCellEdit{ [this](const EditPhase phase, const Text& what) {
            rulesChanged(phase, what, selection());
        } }
    {
        writePageTitle(m_title.text(), PageKind::Style, title);
        m_addButton.onClick([this](ClickEvent&) {
            m_grid.requestNewItem();
        });
        m_grid.onNewItem([this](Grids::NewItemEvent&) {
            addRule();
        });
        m_grid.onItemsMove([this](ItemsMoveEvent& event) {
            rulesMoved(event);
        });
        // Only the style's own rows are carried or held: a base's rows and their groups keep
        // their places under them, and the facts, the bars' buttons and the sizes stand outside
        // the page's set.
        m_grid.onCanMoveItem([this](CanMoveItemEvent& event) {
            if (!isOwnRow(event.item))
                event.canMove = false;
        });
        m_pageBox.body().onCanSelectItem([this](CanSelectItemEvent& event) {
            event.canSelect = isOwnRow(event.item);
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingRules();
        });
        m_rebuildTimer.onTick([this](TimerEvent&) {
            rebuild();
            select(m_pendingSelection, TakeFocus::Yes);
        });
        // Delete reads the held rules and the selected cell, so a move of either asks again. The
        // cell is asked on, not the page's current item: the state is read as it is told, and the
        // item moves before the cell follows it.
        m_pageBox.body().onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
        });
        m_grid.onSelectedCellChange([](Grids::SelectedCellChangeEvent&) {
            StdActions::del.invalidateState();
        });
        // Delete is answered on the rules box, so its tool bar's button and the key in the grid
        // reach one handler, and the sizes' Delete never lands on a rule - see MetricsGrid.
        m_rulesBox.onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = canDelete() });
        });
        m_rulesBox.onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::del)
                deleteSelectedRules();
        });
        m_resetButton.onClick([this](ClickEvent&) {
            resetRules();
        });
        // Offered only where a reset would change something.
        m_resetButton.onGetState([this](GetStateEvent& event) {
            const ControlStyle* stated = shownStyle();
            event.state.enabled = stated && (m_binding.defaults
                ? *stated != *m_binding.defaults
                : !stated->rules.empty() || !stated->dropped.empty()
                    || !stated->metrics.empty());
            event.stopPropagation();
        });
        m_deleteStyleButton.onClick([this](ClickEvent&) {
            if (m_binding.onDeleteStyle)
                m_binding.onDeleteStyle();
        });
        m_basePick.onChange([this](ComboBoxChangeEvent&) {
            basePicked();
        });
        m_standsOnPick.onChange([this](ComboBoxChangeEvent&) {
            standsOnPicked();
        });
        m_seedPick.onChange([this](ComboBoxChangeEvent&) {
            seedPicked();
        });
        // A show enters the page in its body, past the title and its tools; the body stands on
        // its first rule, past the facts - see selectFirstRule.
        setCurrentItem(m_pageBox);
    }
}
