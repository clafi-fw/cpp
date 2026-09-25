export module ClaFi.Controls.Grids :Descriptor;

import :Columns;
import :Cell;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.RingBuffer;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.AppTheme_AnimationSlots;

namespace ClaFi::Controls::Grids
{
    export class RowExpanderHeader;
    export class Grid;
    export class GridBase;


    // How a grid lays its rows out.
    export enum class ViewMode
    {
        Sheet
    };

    // Which of a grid's inner lines are drawn, the frame around it aside. See Grids
    export enum class GridLines
    {
        Both,
        Vertical,
        Horizontal,
        None
    };

    struct CellHighlight
    {
        CellHighlight() = default;
        CellHighlight(const Control* r, const Column* c) : row{ r }, column{ c } {}
        const Control* row{};
        const Column* column{};
        float factor{};   // single axis: hover and select now live in separate rings
        bool is(const Control* r, const Column* c) const { return row == r && column == c; }
        bool vacant() const { return !row && !column; }
    };

    // The only reason HighlightChannel exists is to prevent false animations on other corners
    // when a state changes from a grid corner to the one that's diagonally opposite:
    // Because we do not have storage for each cell state, and a cell state calculates as RowState * ColumnState,
    // corners on other diagonal are falsely flashing midway animation, hence there is the RingBuffer to tame them
    class HighlightChannel
    {
    public:
        HighlightChannel(std::size_t capacity, const AnimationSlot& slot)
            :
            m_ring{ capacity },
            m_slot{ slot }
        {}
        RingBuffer<CellHighlight>& ring() { return m_ring; }
        const AnimationSlot& slot() { return m_slot; }
        // Mutable: a channel names the row so that it can move that row's state when the
        // selection leaves it - see GridBase::selectedRowChanged. It does not own the row.
        Control* row() const { return m_row; }
        // The controller is the grid's to name - null while the grid stands in no form, where
        // nothing of these cells can be running.
        void forgetRow(AnimationController*, const Control*);
        const Column* column() const { return m_column; }
        bool setCell(Control* row, const Column*);
        [[nodiscard]] float cellFactor(const Control& row, const Column&) const;
        CellHighlight& acquireCellHighlight(AnimationController*, const Control& row,
            const Column&);
    private:
        RingBuffer<CellHighlight> m_ring;
        const AnimationSlot& m_slot;
        Control* m_row{};
        const Column* m_column{};
    };

    export class GridDescriptor
    {
        //
        friend Column;
        friend ColumnCollection;
        friend RowBase;
        friend GridBase;
        friend Grid;
        friend RowExpanderHeader;
    public:
        GridDescriptor(Grid& owner, ViewMode, GridLines);
        GridDescriptor(GridDescriptor& other) = delete;
    public:
        Grid& owner() { return m_owner; }
        const Grid& owner() const { return m_owner; }
        Column& rootColumn() { return m_rootColumn; }
        ColumnCollection& columns() const { return m_columns; }
        const ScaledCellMetrics& scaledCellMetrics() const { return m_scaledCellMetrics; }
        ScaledCellMetrics& scaledCellMetrics() { return m_scaledCellMetrics; }
        ScaledPadding scaledCellPadding() const { return m_scaledCellMetrics.padding; }
        float scaledBorderWidth() const { return m_scaledCellMetrics.border; }
        const ControlMetrics& designCellMetrics() const { return m_designCellMetrics; }
        Thickness designBorder() const { return m_designCellMetrics.border; }
        float scrollBarWidthAndSpacing() const { return m_scrollBarWidthAndSpacing; }
        ViewMode viewMode() const { return m_viewMode; }
        void setViewMode(ViewMode value) { m_viewMode = value; }
        [[nodiscard]] GridLines gridLines() const { return m_gridLines; }
        void setGridLines(GridLines);
        // Asked rather than compared against the enum: three unrelated painters read this, and a
        // cell reads both of them for every cell it draws.
        [[nodiscard]] bool drawsVerticalLines() const;
        [[nodiscard]] bool drawsHorizontalLines() const;
        const Scaler& scaler() const { return m_scaledCellMetrics.scaler(); }
        float calculateGrid(float rightPadding, float boundsW);
        const Column* hoveredColumn() const { return m_hoverChannel.column(); }
        [[nodiscard]] const Control* hoveredRow() const { return m_hoverChannel.row(); }
        bool isColumnHovered(const Column&) const;
        const Column* selectedColumn() const { return m_selectChannel.column(); }
        const Control* selectedRow() const { return m_selectChannel.row(); }
        // Per-cell state, replacing the row-factor * column-factor product.
        [[nodiscard]] float cellHoveredFactor(const Control&, const Column&) const;
        [[nodiscard]] float cellSelectedFactor(const Control&, const Column&) const;
        // The colour of a line of the lattice, drawn on the given surface. A line belongs to what
        // it is drawn on, so the surface is passed in: a cell hands over its own surface, so a
        // cell that carries a colour of its own carries the lines beside it with it. The grid's
        // frame is the same rule applied to the grid's own surface - see GridBase::adjustPaint.
        //
        // The alpha carries how much of the rule reached the surface, so a rule that changes
        // nothing answers a fully transparent colour: a caller that has left a gap for the line
        // must test it and leave that gap open rather than close it with something else.
        // The direction is the caller's to state: a line is seen against the surface it is drawn
        // on, so it rises the way whatever carries that surface still has room to rise, which is
        // PaintEvent::contrastSign of the control drawing it.
        [[nodiscard]] static Color gridLineRgb(Hsl surface, const BakedColors&, Lightness);
        // The corners the grid's corner cells turn: what the grid paints at each corner, except
        // that a corner the scroll has cut out of view keeps at least the grid's own radius - the
        // row that turns it may still be in view, held there, and the header at the top of the
        // view is that row.
        [[nodiscard]] static CornerRadii cornerRadiiOf(const PaintEvent& gridEvent);
    private:
        const PaintEvent& gridPaintEvent() const { return *m_gridPaintEvent; } // use only during the paint stage
        void setPaintGridEvent(const PaintEvent& value) { m_gridPaintEvent = &value; }
        // The line a cell move keeps returning to, in the owning grid's own space: a vertical
        // move keeps the x, a horizontal move keeps the y, and each writes the other. A row
        // that does not carry the column, or a cell tall enough to span several rows, therefore
        // costs nothing on the way back. k_maxFloat means no move has set one yet.
        [[nodiscard]] float desiredCellX() const { return m_desiredCellX; }
        void setDesiredCellX(float value) { m_desiredCellX = value; }
        [[nodiscard]] float desiredCellY() const { return m_desiredCellY; }
        void setDesiredCellY(float value) { m_desiredCellY = value; }
        // Which band of a row the keyboard enters it at: ToBegin its first section, ToEnd its
        // last. A vertical key leaving a row records it for the row that receives the focus.
        [[nodiscard]] ScrollDirection cellEntryEdge() const { return m_cellEntryEdge; }
        void setCellEntryEdge(ScrollDirection value) { m_cellEntryEdge = value; }
        //
        void setHoveredCell(Control* row, const Column*, bool initiateHint = true);
        void setSelectedCell(Control* row, const Column*);
        void beginCellSelection();
        void endCellSelection();
        void selectedCellChanged();
        void forgetRow(const Control*);
        //
        bool setHighlightedCell(HighlightChannel&, Control* row, const Column*) const;
        void animateCell(HighlightChannel&, CellHighlight&, float to) const;

        void onCellAnimated(AnimateParams&) const;
        void setSelectedRow(Control*);
    private:
        //
        static constexpr std::size_t k_maxHoverHighlights{ 16 };
        static constexpr std::size_t k_maxSelectHighlights{ 8 };   // current + a few fading-out
        Grid& m_owner;
        ViewMode m_viewMode;
        GridLines m_gridLines;
        Column m_rootColumn{ *this, nullptr, L"Root menu", ShowInHeader::No, ColumnWidthMode::Fill };
        ColumnCollection& m_columns{ m_rootColumn.createSubColumns() };
        const ControlMetrics& m_designCellMetrics;
        ScaledCellMetrics m_scaledCellMetrics;
        float m_lastBoundaryWidth{}; // for which the columns were calculated
        // TODO: why does the descriptor carry scroll bar width?
        float m_scrollBarWidthAndSpacing{};
        HighlightChannel m_hoverChannel{ k_maxHoverHighlights,  AnimationSlots::hovered };
        HighlightChannel m_selectChannel{ k_maxSelectHighlights, AnimationSlots::selected };
        const OnAnimate m_onCellAnimate;
        //
        float m_desiredCellX{ k_maxFloat };
        float m_desiredCellY{ k_maxFloat };
        ScrollDirection m_cellEntryEdge{ ScrollDirection::ToBegin };
        //
        // begin/endCellSelection routine
        int m_selectionCnt{};
        bool m_selectionChanged{};
        //
        const PaintEvent* m_gridPaintEvent{}; // valid only during the paint stage.
    };

}
