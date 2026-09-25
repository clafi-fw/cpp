module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Grids :RowBase;

import :Cell;
import :Columns;
import :Descriptor;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.Events;

namespace ClaFi::Controls::Grids
{
    export class RowContainer;

    // Per-column content state for ONE row, laid out in pre-order.
    // Built once per walk with exactly one hasContent() call per column, so a walk never
    // re-queries the row. `subtreeEnd` is the index just past this column's subtree, which
    // lets the walk step to the next sibling - or skip a merged subtree entirely - without
    // having to stay in lockstep with the build order.
    struct CellFlags
    {
        bool content{};
        bool contentBelow{};
        bool blank{};
        std::size_t subtreeEnd{};
    };
    using CellFlagsList = std::vector<CellFlags>;

    // One cell of one row, as the cell walk reports it. See Grids
    export struct RowCell
    {
        Column& column;
        FloatRect rect;
        std::size_t sectionIndex{};
        bool isFirstColumn{};
        bool blank{}; // painted, and nothing lands on it
    };

    // Where a key lands on a row that holds no cell to land on. See Grids
    export struct RowStop
    {
        Control* control{};
        FloatRect rect{};
    };

    // What every row of a grid is: cells against the columns, and a surface.
    export class RowBase : public LaneBase
    {
        friend Column;
        friend GridBase;
        friend class RowGroupBase;
        friend class RowGroup;
    public:
        template<typename... Args>
        RowBase(const CreateParams&, GridDescriptor&, Args&&...);
        ~RowBase() override;
    public:
        // The control one cell holds, for a cell that carries more than text.
        DECLARE_EVENT(CellContentEvent, OnCellContent, onCellContent)
        // The text one cell is about to be written with.
        DECLARE_EVENT(GetCellTextEvent, OnGetCellText, onGetCellText)
        // The text an in-place edit was left with, on its way back to the cell's source. See Grids
        DECLARE_EVENT(AcceptCellTextEvent, OnAcceptCellText, onAcceptCellText)
        // The hint for one cell, asked while the pointer rests on it. See Grids
        DECLARE_EVENT(GetCellTooltipEvent, OnGetCellTooltip, onGetCellTooltip)
    public:
        std::vector<float>& calculatedHeight() { return m_calculatedHeight; }
    protected:
        using CellVisitor = std::function<void(const RowCell&)>;
        using LaneVisitor = std::function<void(const RowCell&, const FloatRect& lane)>;
        virtual ScaledDimensions calculateCellContent(ScaledCellMetrics&, const Column&, float contentBoundsW);
        ScaledDimensions calculateCell(ScaledCellMetrics&, const Column&, float boundsW);
        // Room a row keeps at the start of a cell for a control of its own. See Grids#collapsible
        [[nodiscard]] virtual ScaledDimensions cellLead(const Column&) const { return {}; }
        ScaledDimensions calculateContent(AlignEvent&) override;
        //
        virtual bool hasContent(const Column& column) const;
        // Whether this row draws an empty cell in a leaf it has nothing in. See Grids#collapsible
        [[nodiscard]] virtual bool hasBlank(const Column&) const { return false; }
        // Whether this row draws both of its lines whatever GridLines the grid carries. Only the
        // header does: its cells are what states the columns, and a header that dropped its own
        // dividers would stop reading as a set of columns at all.
        [[nodiscard]] virtual bool alwaysDrawGridLines() const { return false; }
        //
        // Every cell this row paints, in pre-order, sub-columns ahead of the parent they
        // sit under. The one walk over a row's cells: painting, hit testing and keyboard
        // navigation all read it, so none of them can hold a different idea of where a cell
        // is or whether it exists. The column is handed out non-const from a const row
        // because the columns are the grid's structure, not the row's state.
        void traverseCells(const CellVisitor& visitor) const { traverseCells({ 0.0f, 0.0f }, visitor); }
        void traverseCells(FloatPoint origin, const CellVisitor&) const;
        // Every cell a key or a press lands on, with its lane. See Grids#collapsible
        void traverseLanes(FloatPoint origin, const LaneVisitor&) const;
        //
        virtual void getCellText(const Column&, Text&);
        // The hint for one cell, asked of this row's listeners and then of the grid - the pair
        // getCellText asks. Raised by getTooltip for the cell under the pointer.
        virtual void getCellTooltip(GetCellTooltipEvent&);
        // The other end of getCellText: an in-place edit of this cell is being committed, and
        // this is the text it was left with. It goes to whatever answers getCellText, so a row
        // reading a cell out of a record writes it back to the field it read.
        //
        // Answers with the reason the value was not taken, empty when it was. The editor is
        // still open while this runs, so a reason reaches the user and the edit survives.
        virtual std::wstring acceptCellText(const Column&, const Text&);
        virtual void paintCell(PaintEvent&, const FloatRect& cellRect, const Column&);
        // NO ROW PAINTS ITS OWN RECT. A row's surface reaches the screen through its cells, which
        // paintOneCell fills from it unconditionally - so a rect fill on top of that is the same
        // colour drawn twice, and drawn worse: the base fills controlBounds() with the ROW's
        // corner radius and rounded corners, where a cell frame snaps to the grid's edges,
        // subtracts the border bands its lines stand in, and rounds only the corners a first or
        // last cell actually turns.
        //
        // A row with no cells - a group, an expander - therefore paints nothing at all, and its
        // surface is a colour the rows inside it inherit and paint for it. See
        // RowGroupBase, and RowDivider, which is the one row that draws a fill of its own because
        // it has to stop short of the band its closing grid line stands in.
        void paintSurface(PaintEvent&) override {}
        void paintOneCell(PaintEvent&, const RowCell&, bool isFirstRow);
        void doPaintColumns(PaintEvent&);
        void paintText(PaintEvent&) override {}
        void getTooltip(GetTooltipEvent&) override;
        //
        void getControlState(GetStateEvent&) const override;
        void adjustPaint(AdjustPaintEvent&) override;
        void adjustChildPaint(AdjustPaintEvent&) override;
        //
        void mouseMove(MouseMoveEvent&) override;
        void hoverLeave() override;
        void nestedControlFocusing(FocusEvent&) override;
        void pressDown(PressDownEvent&) override;
        //
        Column* columnAt(PointInForm) const;
        Column* columnAt(PointInControl) const;
        void selectColumnUnderMouse();
        void selectCellOnKeyboardEntry();
        void selectColumn(const Column& value);
        void scrollCellIntoView(const FloatRect& cellRect);
        void setHoveredColumn(const Column* value, bool initiateHint = true);
        //
        virtual void preCalcColumn(Column& column);

        // The control this row shows in `column`, or nullptr when the cell is plain
        // text. A row that holds no controls leaves this alone, which is what keeps its
        // cells framed by the grid whatever the column asked for.
        virtual Control* cellControl(const Column&) { return nullptr; }

        // Do we need them both? May be return span?
        virtual RowContainer* asRowContainer() { return nullptr; }
        virtual GridBase* subGrid() { return nullptr; }
        // Where a move lands on this row when it holds no cells of its own. Empty for a row
        // whose cells already answer.
        virtual RowStop navigationStop() { return {}; }
        // The row this one holds beside its own rows, whose cells span them - a group's span.
        // Its cells belong to the same field as those rows: they name what the rows have in
        // common. Every other row answers nullptr, an expander too - its header is a panel.
        virtual RowBase* spanRow() { return nullptr; }

    private:
        Column* columnAt(ScaledPosition mousePosition, ScaledPosition topLeft) const;
        // The column whose cell holds `control` and gives it a colour, or nullptr. The colour
        // is tested first: it is a field read where cellControl is a lookup, so a grid whose
        // columns carry no colour - the common case - never makes the second.
        //
        // The whole tree, not the top level: a cell belongs to the leaf column it stands in,
        // and a grid that groups its columns keeps every leaf under one - Saturation's
        // Operation and Value are sub-columns of a Saturation group, and it is the leaf that
        // paintOneCell reads the colour from.
        const Column* coloredColumnOfCell(const Control&, const PaintEvent&);
        const Column* coloredColumnOfCell(const ColumnCollection&, const Control&, const PaintEvent&);
        void traverseCell(Column&, FloatPoint position, std::size_t sectionIndex, std::size_t flagsIndex,
            bool isFirstColumn, const CellFlagsList&, const CellVisitor&) const;
        // The cell the keyboard lands on when it arrives at this row.
        Column* cellAtEntry(FloatRect& cellRect) const;
        // Cells are addressed in the grid's own space when a position has to outlive the row
        // holding it: rows sit at different depths and the grid scrolls under them, so that
        // space is the one that holds still.
        float rowLocalX(float gridX) const;
        void calculateSectionHeight(const Column&, std::size_t sectionIndex);
        // Fills `list` in pre-order, one hasContent() call per column. Returns true when `column`
        // or any of its descendants holds a cell, and `covered` says whether a column above does.
        bool buildCellFlags(const Column&, bool covered, CellFlagsList&) const;
        void doGetCellText(const Column&, Text&);
        // Whether the box cuts this cell's text, asked the way the cell is drawn - a moving
        // column's cell off a layout of its own, any other through the cache. `drawn` is the
        // block that box came to, which is what the column's vertical anchor places inside it.
        [[nodiscard]] bool isCellTextTrimmed(const FormContext&, const Column&, const Text&,
            MaxSize bounds, CalculatedDimensions& drawn);
    private:
        std::vector<float> m_calculatedHeight;
    };

    export template<typename T>
        concept RowType = std::is_base_of_v<RowBase, T>;


    //-------------------------------------------------------------------------


    // RowBase

    template<typename ...Args>
    RowBase::RowBase(const CreateParams& params, GridDescriptor& descriptor, Args&&... args)
        :
        LaneBase{ params, descriptor, std::forward<Args>(args)... }
    {
    }

}
