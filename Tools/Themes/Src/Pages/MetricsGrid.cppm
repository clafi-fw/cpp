export module Themes_App.MetricsGrid;

import ClaFi.Controls.Button;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.ToolBarBox;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What the grid calls after it has moved a size: where the edit stands, and its name.
    export using OnMetricsChanged = std::function<void(EditPhase, const Text& what)>;
    // The words a style reads by, as the tree names it, for the style a size comes from.
    export using OnGetStyleName = std::function<std::wstring_view(std::wstring_view token)>;

    // One style's sizes under their tool bar, a row for each: the value its chain resolves to, and
    // which style states it. The bar and the grid stand in the scope of the ElementPage it is on.
    export class MetricsGrid : public ToolBarBox
    {
    public:
        template<typename... Args>
        explicit MetricsGrid(const CreateParams&, Args&&...);
    public:
        // Takes the theme's styles, the style the rows show, and what to call after an edit.
        void bind(ThemeStyles&, std::wstring_view style, OnGetStyleName, OnMetricsChanged);
        // Builds the rows for the style as it stands, after the theme moved under the grid.
        void refresh();
    private:
        // The grid's columns, as their tags name them.
        enum class SizeColumn : TagValue
        {
            Name,
            Value, // a size of one number, its cell spanning the two columns under it
            Across,
            Down,
            From
        };
        // What one row shows and edits.
        enum class SizeRow : TagValue
        {
            Border,
            Band,
            Radius,
            Padding,
            Spacing,
            MinSize,
            PreferredSize,
            Depth,
            GrowIn,
            ShadowBlur,
            ShadowOffset,
            ShadowSpread,
            ShadowOpacity
        };
        using SizeRows = std::initializer_list<SizeRow>;
        // Where a size's value comes from: the level up the chain, 0 for the style's own.
        struct Source
        {
            bool stated{};        // some style of the chain states it
            std::size_t level{};  // which one, where stated
        };
        // A row of one size, known by the control the grid selects.
        struct SizeRowEntry
        {
            Grids::RowBase* row{};
            SizeRow size{};
        };
        using SizeRowEntries = std::vector<SizeRowEntry>;
    private:
        // Whether the cell stands on a size this style states, which Delete puts back.
        [[nodiscard]] bool canClear() const;
        // Puts the size under the cell back to what the chain gives, and records the step.
        void clearCurrent();
        void addRows();
        // The weights are a fixed list, so the border's is picked from them.
        void addBorderRow();
        void addRow(Grids::GridBase&, SizeRow);
        // A group of rows under a header that names what they share.
        void addGroup(std::wstring_view name, SizeRows);
        void connectRow(Grids::Row&, SizeRow);
        void writeCell(Grids::GetCellTextEvent&, SizeRow);
        void writeEditorText(Grids::GetCellEditorTextEvent&, SizeRow);
        void accept(Grids::AcceptCellTextEvent&, SizeRow);
        void borderPicked();
        // The refusal for a typed number, empty where it reads as one the row may hold.
        [[nodiscard]] static std::wstring readNumber(SizeRow, std::wstring_view typed,
            float& value);
        // Writes a value into the style's own statement of the size, and records the step.
        void stateNumber(SizeRow, const Grids::Column&, float value);
        void sizeEdited(SizeRow);
        // Records the step now; the rows are built again on the next tick.
        void changed(const Text& what);
        // Puts the cell on a size's row, where the grid holds one.
        void selectRow(std::optional<SizeRow>);
        // The style's own statement of a size, the style made where the theme states none.
        [[nodiscard]] StatedMetrics& statedMetrics();
        // The number a cell shows, as the chain resolves it; null for a cell holding none.
        [[nodiscard]] std::optional<float> numberOf(SizeRow, const Grids::Column&) const;
        [[nodiscard]] FloatPoint pairOf(SizeRow, const ControlMetrics&) const;
        [[nodiscard]] Source sourceOf(SizeRow) const;
        [[nodiscard]] static bool states(const StatedMetrics&, SizeRow);
        static void clear(StatedMetrics&, SizeRow);
        [[nodiscard]] std::wstring_view nameOf(SizeRow) const;
        // The row the cell stands on, none while the cell is on no row of a size.
        [[nodiscard]] std::optional<SizeRow> currentRow() const;
        // Whether the style's chain reaches Any Window, whose sizes a window's shadow is among.
        [[nodiscard]] bool castsShadow() const;
        // Whether the style is square and sized by one number - an indicator, a scroll button.
        [[nodiscard]] bool hasSingleSize() const;
        // Whether a row holds two numbers, across and down, rather than one.
        [[nodiscard]] bool isPair(SizeRow) const;
        [[nodiscard]] bool hasCell(SizeRow, const Grids::Column&) const;
        [[nodiscard]] static bool isLength(SizeRow);
    private:
        ThemeStyles* m_styles{}; // the theme's styles, null until bound
        std::wstring m_style{}; // the name of the style the rows show
        OnGetStyleName m_styleName{};
        OnMetricsChanged m_onMetricsChanged{};
        std::vector<Grids::RowBase*> m_rows{}; // every row the grid holds outright, groups too
        SizeRowEntries m_sizeRows{}; // every row of a size, the groups' rows too
        ComboBox* m_borderPick{}; // the Border row's weight, built with the rows
        bool m_showingBorder{ false }; // set while the border's pick is put on, which is no edit
        UiTimer m_refreshTimer{}; // builds the rows again outside the pick or the editor that asked
        std::optional<SizeRow> m_pendingRow{}; // the row the cell goes back to after the rebuild

        // Words, icon and state come from the action; Keep leaves the focus in the grid.
        ToolButton& m_clearButton{ tools().add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            StdActions::del,
            NavigationReach::No,
            MouseFocus::Keep
        ) };

        Grids::Dt::Grid& m_grid{ createBody<Grids::Dt::Grid>(
            UiElement::Grid,
            HorizontalAlign::Left,
            FocusScope::No, // the page is the scope; the rows are its items
            NavigationReach::No,
            MouseFocus::Parent,
            Grids::Dt::Columns{
                Grids::Dt::Column{ Tag{ SizeColumn::Name },
                    Text{ L"Size" },
                    Grids::ColumnWidthMode::FitContent
                },
                Grids::Dt::Column{ Tag{ SizeColumn::Value },
                    Text{ L"Value" },
                    Grids::ColumnWidthMode::Fixed,
                    Grids::ColumnWidth{ 160.0f },
                    TextAlign::Right,
                    EditorMode::Editable,
                    Grids::Dt::Column{ Tag{ SizeColumn::Across },
                        Text{ L"Across" },
                        Grids::ColumnWidthMode::Fill,
                        TextAlign::Right,
                        EditorMode::Editable
                    },
                    Grids::Dt::Column{ Tag{ SizeColumn::Down },
                        Text{ L"Down" },
                        Grids::ColumnWidthMode::Fill,
                        TextAlign::Right,
                        EditorMode::Editable
                    }
                },
                Grids::Dt::Column{ Tag{ SizeColumn::From },
                    Text{ L"From" },
                    Grids::ColumnWidthMode::FitContent
                }
            },
            Grids::Dt::Header{}
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    MetricsGrid::MetricsGrid(const CreateParams& params, Args&&... args)
        :
        ToolBarBox{
            params,
            ToolsProps{
                FocusScope::No, // the page is the scope; the bar's buttons are its items
                NavigationReach::No,
                MouseFocus::Parent
            },
            std::forward<Args>(args)...
        }
    {
        m_refreshTimer.onTick([this](TimerEvent&) {
            refresh();
            selectRow(m_pendingRow);
        });
        // Delete is answered here, so the tool bar's button and the key in the grid reach one
        // handler, and a Delete presented elsewhere on the page never lands on a size.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = canClear() });
        });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::del)
                clearCurrent();
        });
        m_grid.onSelectedCellChange([](Grids::SelectedCellChangeEvent&) {
            StdActions::del.invalidateState();
        });
    }
}
