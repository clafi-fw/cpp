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
import :RowNewItem;

import ClaFi.Core.Foundation;
import ClaFi.Controls.Base.StackBase;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Core.TextEngine.Text;
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
        // The value an in-place editor opens on for one cell, where it is not what the cell shows.
        DECLARE_EVENT(GetCellEditorTextEvent, OnGetCellEditorText, onGetCellEditorText)
        // The text an in-place edit was left with, on its way back to the cell's source. See Grids
        DECLARE_EVENT(AcceptCellTextEvent, OnAcceptCellText, onAcceptCellText)
        // The tooltip for one cell, asked while the pointer rests on it. See Grids
        DECLARE_EVENT(GetCellTooltipEvent, OnGetCellTooltip, onGetCellTooltip)
        // A link in one cell's text was clicked, and no row listener took it. See Grids
        DECLARE_EVENT(CellLinkClickEvent, OnCellLinkClick, onCellLinkClick)
        // The menu for a link in one cell is asked for, and no row listener showed one. See Grids
        DECLARE_EVENT(CellLinkContextPopupEvent, OnCellLinkContextPopup, onCellLinkContextPopup)
        // Raised when the new-item row asks for an item. See Grids
        DECLARE_EVENT(NewItemEvent, OnNewItem, onNewItem)
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
        // Every expander row, at every depth, each before the ones in its body.
        template <std::invocable<RowExpander&> Func>
        void forEachExpander(Func&& func);

        // Every row that can matter to a key press: the ones the window reaches, and the rows
        // of any group among them. The origin handed over is the row's own top-left in form
        // space, which is what makes one field out of the cells of rows that sit at different
        // depths.
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
        // The row kept after the last one, whose cell asks for an item.
        RowNewItem& addNewItemRow(PlaceHolderText);
        // Raises NewItemEvent, then selects the leading cell of the row a handler added.
        void requestNewItem();
        // Puts the selection on one cell from code, as a pick does. See Grids#selection-from-code
        void selectCell(RowBase&, const Column*);
        [[nodiscard]] Column* findColumnByTag(Tag tag) const { return columns().findByTag(tag); }
        [[nodiscard]] Column& columnByTag(Tag tag) const { return columns().byTag(tag); }
    protected:
        // Protecting internally used base props
        using StackView::setMetrics;
        using StackView::setSpacing;
        using StackView::setPadding;

        ScaledSize calculateContent(AlignEvent&) override;
        void currentItemChanged(CurrentItemChangeEvent&)  override;
        // A grid standing in a container drops its selected cell when the container's item leaves.
        void nestedCurrentItemChange(Control* currentItem) override;
        // Puts the theme's grid element on the grid, whose stroke is the grid's own border - the
        // outer line of the lattice: the cells leave their outermost strokes to it and draw only
        // the interior ones, each in its row's stroke.
        void adjustPaint(AdjustPaintEvent&) override;
        // A nested grid lets its rows reach the line pixels they share with the rows around it.
        void adjustViewPort(AdjustViewportEvent&) const override;
        // The header row, for as long as it heads a row - see Control::heldHeader.
        [[nodiscard]] const Control* heldHeader() const override;
        // Where the header is drawn: held on its rest line while a row it heads is still under it,
        // then leaving with the bottom edge of the last row - see Control::heldHeaderOffset. Zero
        // for every other child.
        [[nodiscard]] FloatPoint overlayChildOffset(const Control&) const override;
        // The header is an overlay control, so the standard pass leaves it alone and the overlay
        // pass reaches it here - the pass belonging to m_headerHost, which is normally the box's
        // body or the box. The header has to be named rather than found: the ordered child range is
        // entered by the laid-out leading edge, and the view has scrolled past the header's
        // whenever the header is being held. What goes under a held header is drawn here for the
        // same reason it cannot be drawn by the header - see PaintEvent::paintHeldBackdrop.
        void paintChildren(PaintEvent&) override;
        // Every request to be seen that comes from inside this grid passes through here, and each
        // one asks for the strip a held header stands in as well as for itself. See
        // Control::heldHeaderStrip.
        void scrollChildIntoView(Control&, FloatRect) override;
        void nestedPressDown(PressDownEvent&) override;
        // Answered **here** and not on the row. The click walk starts at the control the pointer
        // hit and climbs, so the grid sees every one of them, and the grid is what holds the
        // selected cell the edit is for. See Grids#editing-a-cell
        void nestedClick(NestedClickEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;
        // An arrow reaching here has already been walked over the field, and leaves the grid.
        [[nodiscard]] Control* navigationNeighbour(const Control& child,
            const NavigationStep&) override;
        // Puts a row added after the new-item row ahead of it, and notes it for requestNewItem.
        void controlAdded(Control&) override;
        // The new-item row is a command rather than an item, so no selection holds it.
        bool defaultCanSelectItem(Control&) override;
        // The header and the new-item row keep their places.
        bool defaultCanMoveItem(Control&) override;
    private:
        // A place a key can land: a cell of a row, or the control a row that has no cells of its
        // own stands behind. The origin is the row's own top left in form space, which is what
        // the rect was measured from.
        struct FieldStop
        {
            RowBase* row{}; // for a control stop, the row the control stands in
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
        // A run of table indices, both ends included.
        struct IndexSpan
        {
            std::size_t first{};
            std::size_t last{};
        };
        using Edges = std::vector<float>;
        // The field's stops on the lines their edges share - bands down, columns across.
        struct FieldTable
        {
            explicit FieldTable(const FieldStops&);
            Edges bandEdges{};                  // every distinct top and bottom, ascending
            Edges columnEdges{};                // every distinct left and right of a lane
            std::vector<std::size_t> cells{};   // the stop on each band and column, band by band
            [[nodiscard]] std::size_t bands() const;
            [[nodiscard]] std::size_t columns() const;
            // The stop covering a band and a column, k_maxSize where none does.
            [[nodiscard]] std::size_t stopAt(std::size_t band, std::size_t column) const;
            // The band holding y, or the nearest one.
            [[nodiscard]] std::size_t bandAt(float y) const;
            // The column holding x, or the nearest one.
            [[nodiscard]] std::size_t columnAt(float x) const;
            [[nodiscard]] IndexSpan bandsOf(const FieldStop&) const;
            [[nodiscard]] IndexSpan columnsOf(const FieldStop&) const;
            // The first or the last stop of a band, in column order.
            [[nodiscard]] std::size_t endStop(std::size_t band, bool atStart) const;
            // The line a value is on.
            [[nodiscard]] static std::size_t edgeAt(const Edges&, float value);
            // The span holding a value, or the one at the end it is past.
            [[nodiscard]] static std::size_t spanAt(const Edges&, float value);
            // The spans from the line one value is on to the line the other is on.
            [[nodiscard]] static IndexSpan spansBetween(const Edges&, float from, float to);
            static void mergeEdges(Edges&);   // sorted, and edges within the tolerance made one
        };
    private:
        // Runs an in-place editor over the selected cell and gives the row what it comes back
        // with. False when there is nothing here to edit - no cell selected, a cell holding a
        // control, or a column whose EditorMode is None - which is what lets the key travel on.
        //
        // The editor runs a message loop, so this call lasts the whole edit and the user is free
        // to work in the form behind it meanwhile. The row and the column are therefore
        // re-checked against the selection before the accepted text is handed over. The **grid** is
        // not re-checked and cannot be: this call is on the stack of the grid's own key walk, so
        // a grid that does not survive the edit has already taken the caller with it.
        bool editSelectedCell(const Scaler& scaler);
        // Return follows the selected cell's link where the column opens no editor on it, and
        // Ctrl+Return where it does. False where there is no link to follow, so the key goes on.
        bool followSelectedCellLink(const KeyDownEvent& event);
        // Whether this click is the one that opens the selected cell's editor: a mouse click,
        // released on the cell the press landed on, that press having found the user on it.
        [[nodiscard]] bool clickEditsCell(const NestedClickEvent&) const;
        // One step of the selected cell, answered by the field root; false lets Up and Down leave.
        bool moveCellSelection(const KeyDownEvent& event);
        // Home and End: the first or last stop of the line the selection sits on.
        bool moveCellToLineEdge(const Scaler&, ScrollDirection edge);
        // Ctrl with Home or End: the first or last cell of the whole grid.
        bool moveCellToGridEdge(ScrollDirection edge);
        // PageUp and PageDown: one screenful along the field, keeping the line the run follows.
        bool moveCellByPage(const Scaler&, ScrollDirection direction);
        // Whether the selection is what the keyboard is driving. A grid also carries controls
        // that answer their own keys - a control hosted in a cell - and those keys are theirs.
        [[nodiscard]] bool cellKeysApply() const;
        // Whether this grid is the one the whole field belongs to. A key that addresses more
        // than one row's worth of it is answered here and nowhere deeper: a nested grid sees
        // only its own rows, and an edge or a page measured over those is measured over part of
        // the answer.
        [[nodiscard]] bool isFieldRoot() const;
        // Every place a key can land in the view and lookAhead past it, and the selection's index.
        std::size_t collectField(const Scaler& scaler, FieldStops&, float lookAhead = 0.0f);
        // The same a viewport each way around the selection, for one scrolled out of view.
        std::size_t collectFieldAroundSelection(FieldStops&);
        // Every place a key can land inside window, and the selection's index, k_maxSize off it.
        std::size_t collectFieldIn(const FloatRect& window, FieldStops&);
        // The stop a step from the source lands on, along bands or columns and keeping `line`.
        [[nodiscard]] static std::size_t stopAlong(const FieldTable&, const FieldStops&,
            std::size_t sourceIndex, std::size_t line, bool vertical, bool forward, FieldReach);
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
        // Scrolls until the view stands where `viewInForm` says, a viewport-tall rect. Asked for
        // by the stop's row, so every header held over that row is cleared on the way up.
        void scrollViewToStop(const FieldStop&, const FloatRect& viewInForm);
        // Whether `within` - the viewport, where it stands or where a press will leave it - shows
        // the stop whole: inside it, and under every header held over the stop's row. A stop half
        // on screen or behind a header is not one the eye has read.
        [[nodiscard]] static bool showsWhole(const FloatRect& within, const FieldStop&);
        // The stop furthest down or up a column that `within` shows whole - see showsWhole.
        [[nodiscard]] static std::size_t lastWholeInColumn(const FieldTable&, const FieldStops&,
            const FloatRect& within, std::size_t column, bool forward);
        // The first or last row of this grid holding a cell a key lands on, in the order the eye
        // reads them. Structural rather than geometric: the far end of a grid is normally off
        // screen, where the walk that answers the arrow keys does not reach.
        RowBase* edgeRow(ScrollDirection edge);
        [[nodiscard]] bool rowHoldsCellStops(RowBase&);
        // **Both paths of a selection move.** The rows in effect are the row the selected cell
        // stands in and every row that row stands inside, so a move changes the state of two
        // paths - and only below where they meet. The first control above the row being left
        // that also holds the row being entered is that meeting point: above it the answer is
        // the same before and after, and invalidating it would restart an animation that is
        // already where it belongs. Either row may be null, which is a path to nowhere.
        void selectedRowChanged(Control* from, Control* to);
    private:
        static constexpr float k_edgeTolerance{ 0.5f };   // edges nearer than this are one line
        GridDescriptor& m_descriptor;
        // The header row, once one has been added.
        GridHeader* m_header{};
        // The container holding the overlay entry that names the header - the host
        // Control::heldHeaderHost names where it can hold one, and the grid itself where it
        // cannot. It is what bounds what the header draws, and it names the pass the header is
        // painted in. See addHeader.
        Control* m_headerHost{};
        RowNewItem* m_newItemRow{}; // null until addNewItemRow
        Control* m_rowAdded{}; // the last row added, which requestNewItem selects
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

    template <std::invocable<RowExpander&> Func>
    void GridBase::forEachExpander(Func&& func)
    {
        for (RowBase& row : controlsAs<RowBase>())
        {
            if (RowExpander* expander = row.asRowExpander())
                func(*expander);
            if (GridBase* nested = row.subGrid())
                nested->forEachExpander(func);
        }
    }
}
