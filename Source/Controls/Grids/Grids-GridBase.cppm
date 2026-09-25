module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Grids :GridBase;

import :Cell;
import :Columns;
import :Descriptor;
import :RowBase;
import :Row;
import :RowContainer;
import :RowDivider;
import :GridHeader;
import :RowExpander;
import :RowGroup;

import ClaFi.Core.Foundation;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Events;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // A grid of rows sectioned by shared columns.
    export class GridBase : public StackView
    {
        friend GridDescriptor;
    public:
        using RowVisitor = std::function<void(RowBase&, FloatPoint originInForm)>;
    public:
        template<typename... Args>
        GridBase(const CreateParams&, GridDescriptor&, Args&&...);
    public:
        // A grid emits these as well as its rows do - see Grid::hasCellContent / getCellText.
        DECLARE_EVENT(CellContentEvent, OnCellContent, onCellContent)
        // The text one cell is about to be written with.
        DECLARE_EVENT(GetCellTextEvent, OnGetCellText, onGetCellText)
        // The text an in-place edit was left with, on its way back to the cell's source. See Grids
        DECLARE_EVENT(AcceptCellTextEvent, OnAcceptCellText, onAcceptCellText)
        // The hint for one cell, asked while the pointer rests on it. See Grids
        DECLARE_EVENT(GetCellTooltipEvent, OnGetCellTooltip, onGetCellTooltip)
    public:
        template<RowType Row, typename... Args>
        Row& add(Args&&... args) {
            Row& result = StackView::add<Row>(m_descriptor, std::forward<Args>(args)...);
            return result;
        }
        template<typename... Args>
        Row& addRow(Args&&... args) {
            return add<Row>(std::forward<Args>(args)...);
        }
        template<typename... Args>
        RowContainer& addControlRow(Args&&... args) {
            return add<RowContainer>(std::forward<Args>(args)...);
        }
        template<typename... Args>
        RowDivider& addDivider(Args&&... args) {
            return add<RowDivider>(std::forward<Args>(args)...);
        }
        template<typename... Args>
        RowExpander& addExpander(Args&&... args) {
            return add<RowExpander>(std::forward<Args>(args)...);
        }
        template<typename... Args>
        RowGroup& addGroup(Args&&... args) {
            return add<RowGroup>(std::forward<Args>(args)...);
        }
        template <std::invocable<RowContainer&> Func>
        void forEachRow(Func&& func);

        // Every row that can matter to a key press: the ones the window reaches, plus the
        // header and the rows of any group among them. The origin handed over is the row's own
        // top-left in form space, which is what makes one field out of the cells of rows that
        // sit at different depths.
        void traverseRows(const FloatRect& windowInForm, const RowVisitor&);

        void setViewMode(ViewMode value) { m_descriptor.setViewMode(value); }
        [[nodiscard]] GridLines gridLines() const { return m_descriptor.gridLines(); }
        void setGridLines(GridLines value) { m_descriptor.setGridLines(value); }
        // Columns are structure, not state. A const grid still hands them out mutable -
        // the same call GridDescriptor::columns() const makes. Narrowing to a const
        // reference here would only be cosmetic, since findByTag() and byTag() are const
        // members that return mutable columns anyway.
        ColumnCollection& columns() const { return m_descriptor.columns(); }
        GridDescriptor& descriptor() { return m_descriptor; }
        GridHeader& addHeader();
        // The header row, once one has been added - what a cell event's row is compared with to
        // tell a column's name cell from a data cell. nullptr before then.
        [[nodiscard]] const GridHeader* header() const { return m_header; }
        [[nodiscard]] Column* findColumnByTag(Tag tag) const { return columns().findByTag(tag); }
        [[nodiscard]] Column& columnByTag(Tag tag) const { return columns().byTag(tag); }
    protected:
        // Protecting internally used base props
        using StackView::setMetrics;
        using StackView::setSpacing;
        using StackView::setPadding;

        ScaledDimensions calculateContent(AlignEvent&) override;
        void currentItemChanged(CurrentItemChangeEvent&)  override;
        // Puts the theme's grid element on the grid, whose stroke is the grid's own border - the
        // outer line of the lattice: the cells leave their outermost strokes to it and draw only
        // the interior ones, in gridLine.
        void adjustPaint(AdjustPaintEvent&) override;
        // The header row, for as long as it heads a row - see Control::heldHeader.
        [[nodiscard]] const Control* heldHeader() const override;
        // Where the header is drawn: held on its rest line while a row it heads is still under it,
        // then leaving with the bottom edge of the last row - see Control::heldHeaderOffset. Zero
        // for every other child.
        [[nodiscard]] FloatPoint overlayChildOffset(const Control&) const override;
        // The header is an overlay control, so the standard pass leaves it alone and the overlay
        // pass reaches it here - the pass belonging to m_headerHost, which is normally the grid's
        // parent. The header has to be named rather than found: the ordered child range is
        // entered by the laid-out leading edge, and the view has scrolled past the header's
        // whenever the header is being held. What goes under a held header is drawn here for the
        // same reason it cannot be drawn by the header - see PaintEvent::paintHeldBackdrop.
        void paintChildren(PaintEvent&) override;
        // Every request to be seen that comes from inside this grid passes through here, and each
        // one asks for the strip a held header stands in as well as for itself. See
        // Control::heldHeaderStrip.
        void scrollChildIntoView(Control&, FloatRect) override;
        void pressDown(PressDownEvent&) override;
        // Answered HERE and not on the row. The double-click walk starts at the control the
        // pointer hit and climbs, so the grid sees every one of them, and the grid is what holds
        // the selected cell the edit is for. The first click of the pair has already put the
        // selection on the cell under the pointer.
        void doubleClick(DoubleClickEvent&) override;
        void keyDown(KeyDownEvent&) override;
    private:
        // A place a key can land: a cell of a row, or the control a row that has no cells of its
        // own stands behind. The origin is the row's own top left in form space, which is what
        // the rect was measured from.
        struct FieldStop
        {
            RowBase* row{};
            Column* column{};
            Control* control{};
            FloatPoint origin{};
            FloatRect rect{};
            FloatRect lane{}; // the rect widened over the blanks it stands for
        };
        using FieldStops = std::vector<FieldStop>;
        // Whether a search takes the first stop ahead of the source or the last one.
        enum class FieldReach
        {
            Nearest,
            Furthest
        };
    private:
        // Runs an in-place editor over the selected cell and gives the row what it comes back
        // with. False when there is nothing here to edit - no cell selected, a cell holding a
        // control, or a column whose EditorMode is None - which is what lets the key travel on.
        //
        // The editor runs a message loop, so this call lasts the whole edit and the user is free
        // to work in the form behind it meanwhile. The row and the column are therefore
        // re-checked against the selection before the accepted text is handed over. The GRID is
        // not re-checked and cannot be: this call is on the stack of the grid's own key walk, so
        // a grid that does not survive the edit has already taken the caller with it.
        bool editSelectedCell();
        // Moves the selected cell one step the way the key points, over the cells of every row
        // the window reaches. False when the field holds none that way, which is what lets the
        // key travel on to the grid that holds this one, and out of the grid altogether.
        bool moveCellSelection(KeyCode);
        // Home and End: the first or last stop of the line the selection sits on.
        bool moveCellToLineEdge(ScrollDirection edge);
        // Ctrl with Home or End: the first or last cell of the whole grid.
        bool moveCellToGridEdge(ScrollDirection edge);
        // PageUp and PageDown: one screenful along the field, keeping the line the run follows.
        bool moveCellByPage(ScrollDirection direction);
        // Whether the selection is what the keyboard is driving. A grid also carries controls
        // that answer their own keys - a control hosted in a cell - and those keys are theirs.
        [[nodiscard]] bool cellKeysApply() const;
        // Whether this grid is the one the whole field belongs to. A key that addresses more
        // than one row's worth of it is answered here and nowhere deeper: a nested grid sees
        // only its own rows, and an edge or a page measured over those is measured over part of
        // the answer.
        [[nodiscard]] bool isFieldRoot() const;
        // Every place a key can land, and where the selection stands among them - k_maxSize when
        // the selection is outside the window, which is when the navigator takes over.
        // lookAhead widens the window past the viewport edge margin. A key that reaches further
        // than one step must pass the distance it crosses, since nothing further out is
        // collected and no stop there can be scored.
        std::size_t collectField(FieldStops&, float lookAhead = 0.0f);
        // The stop a move from `source` lands on: the one ahead of it that `reach` names, with a
        // stop the line runs through taken over one merely nearer.
        [[nodiscard]] std::size_t searchField(const FieldStops&, const OrientedRect& source,
            const OrientedRect& line, KeyCode, std::size_t sourceIndex, FieldReach) const;
        // The point the two remembered lines cross, in form space. The first move of a run
        // adopts the position it starts from, so every later one measures against the same pair.
        FloatPoint cellBand(const FloatRect& sourceRect);
        // Records the line across the move and keeps the one along it: a run of Down holds its
        // column even where a row carries no cell at that x, which is what a pair of lines is
        // for. The written line enters the stop at the edge the move crossed, clamped into it
        // rather than centred on it - a cell spanning several rows would otherwise take the line
        // halfway down the group, and the step back out would land in the middle of it.
        void rememberCellLine(const FloatRect& targetRect, float band, bool horizontal);
        // Gives a stop the focus and records it as the selected cell. Showing it is the caller's
        // business, since a page press has a whole page to show and the stop is only where that
        // page ends.
        void focusStop(const FieldStop&);
        // The least scroll that shows the stop.
        void scrollStopIntoView(const FieldStop&);
        // A page-tall rect, which can only be shown by putting its leading edge against the
        // viewport's.
        void scrollPageIntoView(const FloatRect& pageInForm);
        // The stop furthest along the key's direction among those the line runs through and
        // `within` shows whole. A stop half on screen is not one the eye has read, so it neither
        // ends the page being left nor begins the one arriving.
        [[nodiscard]] std::size_t lastWholeInBand(const FieldStops&, const FloatRect& within,
            const OrientedRect& line, KeyCode) const;
        // The first or last row of this grid that paints a cell, in the order the eye reads
        // them. Structural rather than geometric: the far end of a grid is normally off screen,
        // where the walk that answers the arrow keys does not reach.
        RowBase* edgeRow(ScrollDirection edge);
        [[nodiscard]] bool rowPaintsCells(RowBase&);
        // Whether the header is being carried past where it was laid out. A held header stands
        // over the rows rather than among them: it is drawn on a line of its own, while the cells
        // it is laid out with stay where the scroll has taken them. So it takes part in the
        // keyboard field only while it is at rest, where the two are the same place - see
        // traverseRows and edgeRow.
        [[nodiscard]] bool isHeaderHeld() const;
        // BOTH PATHS OF A SELECTION MOVE. The rows in effect are the row the selected cell
        // stands in and every row that row stands inside, so a move changes the state of two
        // paths - and only below where they meet. The first control above the row being left
        // that also holds the row being entered is that meeting point: above it the answer is
        // the same before and after, and invalidating it would restart an animation that is
        // already where it belongs. Either row may be null, which is a path to nowhere.
        void selectedRowChanged(Control* from, Control* to);
    private:
        GridDescriptor& m_descriptor;
        // The header row, once one has been added.
        GridHeader* m_header{};
        // The container holding the overlay entry that names the header - the grid's parent where
        // that parent can hold one, and the grid itself where it cannot. It is what bounds what
        // the header draws, and it names the pass the header is painted in. See addHeader.
        Control* m_headerHost{};
    };


    //-------------------------------------------------------------------------


    // GridBase

    template<typename ...Args>
    GridBase::GridBase(const CreateParams& params, GridDescriptor& descriptor, Args&&... args)
        :
        StackView{
            params,
            Orientation::Vertical,
            Interactivity::None,
            std::forward<Args>(args)...,
            Spacing{ 0.0f },
            Padding{ 0.0f }
        },
        m_descriptor{ descriptor }
    {
    }

    template <std::invocable<RowContainer&> Func>
    void GridBase::forEachRow(Func&& func)
    {
        for (RowBase& row : controlsAs<RowBase>())
        {
            if (RowContainer* container = row.asRowContainer())
            {
                func(*container);
            }
            if (GridBase* nested = row.subGrid())
            {
                nested->forEachRow(func);
            }
        }
    }
}
