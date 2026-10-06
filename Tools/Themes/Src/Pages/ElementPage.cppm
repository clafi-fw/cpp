export module Themes_App.ElementPage;

import Themes_App.ApplyToControl;
import Themes_App.RuleSlider;

import ClaFi.Icons.HueIcon;
import ClaFi.Icons.LuminosityIcon;
import ClaFi.Icons.ResetIcon;
import ClaFi.Icons.SaturationIcon;

import ClaFi.Controls.Button;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
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

    // What an element page's grid holds, by the rules' places since its rows are rebuilt: the
    // selected cell where there is one, and the rules whose rows are held beside it.
    export struct RuleSelection
    {
        struct Cell
        {
            std::size_t rule{}; // the place of the rule the cell stands in
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

    // What an element page calls after it has moved its rules: where the edit stands, its name,
    // and what the grid held as it was made.
    export using OnRulesChanged =
        std::function<void(EditPhase, const Text& what, const RuleSelection& at)>;
    // The colour a rule of the list is applied to, which its value ramps are drawn from.
    export using OnGetListRuleBase = std::function<RuleBase(const ColorRule&, RuleChannel)>;

    // One list of rules - an element's own or the shared ones - a grid row for each rule.
    export class ElementPage : public Panel
    {
    public:
        template<typename... Args>
        explicit ElementPage(const CreateParams&, std::wstring_view title, Args&&...);
    public:
        // Takes the list, its defaults, its channels, the palette, its ramps' base and edits.
        void bind(ColorRules&, const ColorRules& defaults, PaintChannels outputs,
            const ThemeColors&, OnGetListRuleBase, OnRulesChanged);
        // Builds a row for every rule in the list, in list order.
        void rebuild();
        // What the grid holds selected, as an edit made on it is recorded.
        [[nodiscard]] RuleSelection selection() const;
        // Puts a selection back on the rows as they stand; a cell past them lands on the last.
        void select(const RuleSelection&, TakeFocus);
    private:
        // The grid's columns, as their tags name them.
        enum class RuleColumn : TagValue
        {
            ApplyTo,
            Hue,
            Saturation,
            Elevation
        };
        using RuleRows = std::vector<Grids::RowContainer*>;
    private:
        void addRule();
        void deleteSelectedRules();
        void deletePendingRules();
        void resetRules();
        void addRow(std::size_t index);
        void bindEditors(Grids::RowContainer&);
        [[nodiscard]] Grids::Column& column(RuleColumn);
        [[nodiscard]] OnGetRuleBase ruleBaseOf(std::size_t index, RuleChannel) const;
        // What a change to one column of one rule is called - the rule named by its place.
        [[nodiscard]] Text ruleStepName(std::size_t index, std::wstring_view column) const;
        void rulesChanged(EditPhase, const Text& what, const RuleSelection& at);
    private:
        const std::wstring m_name; // what the list is called, which every step of it starts with
        ColorRules* m_rules{}; // the list the page shows, null until bound
        const ColorRules* m_defaults{}; // what a reset puts back, null until bound
        PaintChannels m_outputs{}; // the channels the list's rules may write
        const ThemeColors* m_colors{}; // what the hue editors read the palette from
        OnGetListRuleBase m_ruleBase{};
        OnRulesChanged m_onRulesChanged{};
        OnRuleChanged m_onCellEdit; // what the cells' editors call, held by address
        UiTimer m_deleteTimer{}; // deletes on the next tick, outside the event that asked
        RuleSelection m_pendingDelete{}; // what Delete was asked on, its rows still to go
        RuleRows m_ruleRows{}; // one per rule, the header aside

        Panel& m_topBar{ createTopBar<Panel>(
            Padding{ 12.0f, 8.0f }
        ) };

        Label& m_title{ m_topBar.createBody<Label>(
            VerticalTextAnchor::Center
        ) };

        Stack& m_tools{ m_topBar.createRightBar<Stack>(
            Orientation::Horizontal,
            Interactivity::ActiveContainer,
            Spacing{ 4.0f }
        ) };

        ToolButton& m_resetButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnPaintIcon{ Icons::ResetIcon::paint },
            L"Reset to defaults",
            Interactivity::MouseOnly
        ) };

        // Words, icon and state come from the action; mouse only keeps the focus in the grid.
        ToolButton& m_deleteButton{ m_tools.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            StdActions::del,
            Interactivity::MouseOnly
        ) };

        // A row names its rule by the rule's place in the list, carried as the row's tag.
        Grids::Dt::Grid& m_grid{ createBody<ScrollBox>(
            ScrollBars::Vertical
        ).createBody<Stack>(
            Orientation::Vertical,
            Padding{ 12.0f }
        ).add<Grids::Dt::Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal,
            SelectionMode::Multi,
            Grids::Dt::Columns{
                Grids::Dt::Column{ Tag{ RuleColumn::ApplyTo },
                    Text{ L"Apply to" },
                    Grids::ColumnWidthMode::Fill
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Hue },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::HueIcon::paint }, L" Hue" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Saturation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::SaturationIcon::paint },
                        L" Saturation" }
                },
                Grids::Dt::Column{ Tag{ RuleColumn::Elevation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::paint }, L" Elevation" }
                }
            },
            Grids::Dt::Header{},
            Grids::Dt::NewItem{ PlaceHolderText{ L"Add rule" } }
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
        m_title.text() << TextStyleId::SubTitle << title;
        m_grid.onNewItem([this](Grids::NewItemEvent&) {
            addRule();
        });
        m_deleteTimer.onTick([this](TimerEvent&) {
            deletePendingRules();
        });
        m_grid.onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
        });
        // The toolbar button and the Delete key land here alike, and the selection they act on is
        // this page's grid.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = !m_grid.selection().empty() });
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
            event.state.enabled = m_rules && *m_rules != *m_defaults;
            event.stopPropagation();
        });
    }
}
