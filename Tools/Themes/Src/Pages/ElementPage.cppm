export module Themes_App.ElementPage;

import Themes_App.ApplyToControl;
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
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Controls.TextItems;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A set as the pages name it: the name the theme states it under, and the words it reads by.
    export struct SetName
    {
        std::wstring token{};
        std::wstring name{};
    };

    export using SetNames = std::vector<SetName>;

    // What an element page's grid holds, by the rules' places since its rows are rebuilt: the
    // selected cell where there is one, and the set's own rules whose rows are held beside it.
    export struct RuleSelection
    {
        struct Cell
        {
            std::size_t level{};  // 0 in the set's own rules, then each base up the chain
            std::size_t rule{};   // the place of the rule the cell stands in, in that set's list
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

    // What an element page calls after it has changed its set: where the edit stands, its name,
    // and what the grid held as it was made.
    export using OnRulesChanged =
        std::function<void(EditPhase, const Text& what, const RuleSelection& at)>;
    // The colour a rule of the list is applied to, which its value ramps are drawn from.
    export using OnGetListRuleBase = std::function<RuleBase(const ColorRule&, RuleChannel)>;
    // The theme's sets as the tree lists them, asked for as a page's rows are built.
    export using OnGetSetNames = std::function<SetNames()>;
    // What a page calls to take its set out of the theme - empty for a set the framework states.
    export using OnDeleteSet = std::function<void()>;
    // What a page calls once it has put its set into the theme, which moves every set: every
    // page's editors hold their rules by address, so every page is built again.
    export using OnSetsChanged = std::function<void()>;

    // What a page shows and what it calls, handed over whole.
    export struct PageBinding
    {
        ThemeRules* rules{};            // the theme's sets
        std::wstring set{};             // the name of the set the page shows
        const ElementRules* defaults{}; // what a reset puts back, null where there are none
        PaintChannels outputs{};        // the channels the set's rules may write
        const ThemeColors* colors{};    // what the hue editors read the palette from
        OnGetListRuleBase ruleBase{};
        OnGetSetNames setNames{};
        OnRulesChanged onRulesChanged{};
        OnDeleteSet onDeleteSet{};      // empty for a set the framework states
        OnSetsChanged onSetsChanged{};
    };

    // The grid's columns, as their tags name them.
    enum class RuleColumn : TagValue
    {
        ApplyTo,
        Hue,
        Saturation,
        Elevation
    };

    // One set's page: the three facts it states, its own rules, and under them the rules its
    // chain gives it, each base's in a group of its own.
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
        // Whether Delete has something to act on: own rules held, or the cell on a rule of a
        // base that still reaches the set.
        [[nodiscard]] bool canDelete() const;
    private:
        using RuleRows = std::vector<Grids::RowContainer*>;
        using Groups = std::vector<Grids::RowExpander*>;
        // The row of a rule a base gives the set: the base's level in the chain, the rule's
        // place in its list, and whether a set below has dropped it.
        struct InheritedRow
        {
            std::size_t level{};
            std::size_t rule{};
            Grids::RowBase* row{};
            bool dropped{};
        };
        using InheritedRows = std::vector<InheritedRow>;
        // Whether a set below a base has dropped a rule of it, and which.
        struct Dropping
        {
            bool dropped{};
            std::size_t level{}; // the level of the set that dropped it, 0 for this one
        };
    private:
        // The set, and nullptr while the theme states no set of that name.
        [[nodiscard]] ElementRules* set() const;
        // The set's own rules, and nullptr while the theme states no set of that name.
        [[nodiscard]] ColorRules* rules() const;
        // The set, made with no base where the theme states none - an edit is about to land.
        [[nodiscard]] ElementRules& statedSet();
        void addRule();
        void deleteSelectedRules();
        void deletePendingRules();
        void resetRules();
        void rulesMoved(const ItemsMoveEvent&);
        void addRow(std::size_t index);
        void bindEditors(Grids::RowContainer&);
        [[nodiscard]] bool isOwnRow(const Control&) const; // one of the set's own rules' rows
        // One expander per base of the chain, the nearest first, holding the base's rules.
        void addInheritedGroups();
        void addInheritedRow(Grids::GridBase& body, const ElementChain&, std::size_t level,
            std::size_t rule);
        // Which set of the chain below the level has dropped the rule, the lowest first.
        [[nodiscard]] Dropping droppingOf(const ElementChain&, std::size_t level,
            const ColorRule&) const;
        void dropRule(std::size_t level, std::size_t rule);
        void restoreRule(std::size_t level, std::size_t rule);
        // Fills the three lists with what the set may state and puts its facts on them.
        void showFacts();
        void basePicked();
        void standsOnPicked();
        void seedPicked();
        [[nodiscard]] Grids::Column& column(RuleColumn);
        [[nodiscard]] OnGetRuleBase ruleBaseOf(std::size_t index, RuleChannel) const;
        // The words a set reads by, as the tree names it, and its token where the tree does not.
        [[nodiscard]] std::wstring_view nameOf(std::wstring_view token) const;
        // What a change to one thing of the set is called.
        [[nodiscard]] Text changeStepName(std::wstring_view what) const;
        // What a change to one column of one rule is called - the rule named by its place.
        [[nodiscard]] Text ruleStepName(std::size_t index, std::wstring_view column) const;
        // A rule of a base named by its place and the base, as a drop or a restore is recorded.
        [[nodiscard]] Text inheritedRuleName(const ElementChain&, std::size_t level,
            std::size_t rule) const;
        void rulesChanged(EditPhase, const Text& what, const RuleSelection& at);
    private:
        const std::wstring m_name; // what the set is called, which every step of it starts with
        PageBinding m_binding{}; // rules null until bound
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
        SetNames m_names{}; // the sets as the tree names them, read as the rows are built
        TextItems m_baseItems{}; // none, then the sets the base may be
        TextItems m_standsOnItems{}; // none, then the sets the set may stand on
        TextItems m_seedItems{}; // none, then the four pigments
        std::vector<std::wstring> m_baseTokens{}; // the set each base item stands for
        std::vector<std::wstring> m_standsOnTokens{}; // the set each stands-on item stands for
        bool m_showingFacts{ false }; // set while showFacts puts the picks on, which is no edit

        Panel& m_topBar{ createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ) };

        Label& m_title{ m_topBar.createBody<Label>(
            VerticalTextAnchor::Center
        ) };

        StackPanel& m_tools{ m_topBar.createRightBar<StackPanel>(
            Orientation::Horizontal,
            FocusScope::Yes,
            NavigationReach::Yes,
            MouseFocus::Self,
            Spacing{ 4.0f }
        ) };

        ToolButton& m_addButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::PlusMark::draw },
            L"Add rule"
        ) };

        ToolButton& m_resetButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::ResetIcon::draw },
            L"Reset to defaults",
            NavigationReach::No,
            MouseFocus::Keep
        ) };

        // Words, icon and state come from the action; Keep leaves the focus in the grid.
        ToolButton& m_deleteButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            StdActions::del,
            NavigationReach::No,
            MouseFocus::Keep
        ) };

        // Shown on a set of the theme's own: a set the framework states stays.
        ToolButton& m_deleteSetButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::XMark::draw },
            L"Delete set"
        ) };

        StackPanel& m_body{ createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f },
            Spacing{ 8.0f }
        ) };

        // The three facts a set states beside its rules, each a framed field with its list. The
        // fields wrap where the page is narrower than the three, and stand packed: each is
        // aligned to the start, so the row hands its surplus to none of them.
        StackPanel& m_facts{ m_body.add<StackPanel>(
            Orientation::HorizontalWrap,
            FocusScope::Yes,
            NavigationReach::Yes,
            MouseFocus::Self,
            Spacing{ 12.0f }
        ) };

        StackPanel& m_baseFact{ m_facts.add<StackPanel>(
            Orientation::Horizontal,
            HorizontalAlign::Left,
            MetricsSet::Page,
            UiElement::Section,
            Padding{ 8.0f, 4.0f },
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
            MetricsSet::Page,
            UiElement::Section,
            Padding{ 8.0f, 4.0f },
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
            MetricsSet::Page,
            UiElement::Section,
            Padding{ 8.0f, 4.0f },
            Spacing{ 6.0f }
        ) };

        Label& m_seedLabel{ m_seedFact.add<Label>(
            Text{ ColoredText{ InkGrade::Muted, L"Seed" } },
            VerticalTextAnchor::Center
        ) };

        ComboBox& m_seedPick{ m_seedFact.add<ComboBox>(
            m_seedItems
        ) };

        // A row names its rule by the rule's place in the list, carried as the row's tag.
        Grids::Dt::Grid& m_grid{ m_body.add<Grids::Dt::Grid>(
            MetricsSet::Page,
            UiElement::Section,
            HorizontalAlign::Left,
            Grids::GridLines::Horizontal,
            SelectionMode::Multi,
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
        writePageTitle(m_title.text(), PageKind::Rules, title);
        m_addButton.onClick([this](ClickEvent&) {
            m_grid.requestNewItem();
        });
        m_grid.onNewItem([this](Grids::NewItemEvent&) {
            addRule();
        });
        m_grid.onItemsMove([this](ItemsMoveEvent& event) {
            rulesMoved(event);
        });
        // Only the set's own rows are carried or held: a base's rows and their groups keep their
        // places under them.
        m_grid.onCanMoveItem([this](CanMoveItemEvent& event) {
            if (!isOwnRow(event.item))
                event.canMove = false;
        });
        m_grid.onCanSelectItem([this](CanSelectItemEvent& event) {
            if (!isOwnRow(event.item))
                event.canSelect = false;
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingRules();
        });
        m_rebuildTimer.onTick([this](TimerEvent&) {
            rebuild();
            select(m_pendingSelection, TakeFocus::Yes);
        });
        // Delete reads the held rules and the selected cell, so a move of either asks again.
        m_grid.onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
        });
        m_grid.onSelectedCellChange([](Grids::SelectedCellChangeEvent&) {
            StdActions::del.invalidateState();
        });
        // The toolbar button and the Delete key land here alike, and the selection they act on is
        // this page's grid.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = canDelete() });
        });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::del)
                deleteSelectedRules();
        });
        m_resetButton.onClick([this](ClickEvent&) {
            resetRules();
        });
        // Offered only where a reset would change something.
        m_resetButton.onGetState([this](GetStateEvent& event) {
            const ElementRules* stated = set();
            event.state.enabled = stated && (m_binding.defaults
                ? *stated != *m_binding.defaults
                : !stated->rules.empty() || !stated->dropped.empty());
            event.stopPropagation();
        });
        m_deleteSetButton.onClick([this](ClickEvent&) {
            if (m_binding.onDeleteSet)
                m_binding.onDeleteSet();
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
        // A show enters the page at the grid, past the title, the tools and the facts.
        setCurrentItem(m_grid);
    }
}
