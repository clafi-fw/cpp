module ClaFi.Controls.Grids;

import :RowBase;
import :RowNewItem;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.StackBase;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{

    // GridBase

    void GridBase::traverseRows(const FloatRect& windowInForm, const RowVisitor& visitor)
    {
        ControlSpan rows = controls();
        if (rows.empty())
            return;
        // Every child of a grid shares one content origin, so the window is converted once and
        // the rows are then compared in the space their own topLeft is measured in.
        const FloatPoint contentOrigin = rows.front()->parentContentOrigin();
        FloatRect window = windowInForm;
        window.offset(-contentOrigin);
        const TraversalOrder order = traversalOrder();
        for (ControlSpan::iterator it = firstChildInViewport(window); it != rows.end(); ++it)
        {
            Control& child = **it;
            // Visibility before the end test, the order traverseChildren uses: a hidden row
            // keeps whatever position it last had, and reading that as the end of the range
            // would hide every row after it.
            if (!child.visible())
                continue;
            if (child.isViewportEnd(order, window))
                break;
            if (!child.enabled(true))
                continue;
            // Every child of a grid is a row.
            RowBase& row = static_cast<RowBase&>(child);
            visitor(row, contentOrigin + child.topLeft());
            // A group's span stands beside the rows it holds rather than above them, so it
            // belongs to the same field: reaching it is a step sideways out of a child row.
            if (RowBase* span = row.spanRow())
                if (span->visible())
                    visitor(*span, span->parentContentOrigin() + span->topLeft());
            // A collapsed expander keeps its rows, hidden. They paint nothing and no click can
            // reach them, so no key may either.
            if (GridBase* nested = row.subGrid())
                if (nested->visible())
                    nested->traverseRows(windowInForm, visitor);
        }
    }

    GridHeader& GridBase::addHeader()
    {
        m_header = &add<GridHeader>(*this);
        // The entry goes to the host Control::heldHeaderHost names, not to the grid. The pass an
        // entry opens runs inside its host's clip, and what a held header draws around itself -
        // its shadow - falls on what the grid stands on rather than stopping at the grid's edge,
        // which is exactly the grid's own width. A host with no overlay list leaves the grid
        // hosting the entry itself, which costs the shadow the room outside the grid and nothing
        // else.
        //
        // Standard clipping either way: the header is clipped to where it is **drawn** rather than
        // to where it was laid out, because the traversal has already moved its bounds by
        // overlayChildOffset.
        m_headerHost = heldHeaderHost();
        if (!m_headerHost || !m_headerHost->addOverlayControl(*m_header, ClippingMode::Standard))
        {
            m_headerHost = this;
            addOverlayControl(*m_header, ClippingMode::Standard);
        }
        return *m_header;
    }

    RowNewItem& GridBase::addNewItemRow(PlaceHolderText placeHolderText)
    {
        m_newItemRow = &add<RowNewItem>(std::move(placeHolderText));
        return *m_newItemRow;
    }

    // The row a handler added is entered at its leading cell, since a new item is filled in from
    // its start. It has not been laid out yet, so the reveal waits for the pass.
    void GridBase::requestNewItem()
    {
        m_rowAdded = nullptr;
        NewItemEvent event{ *this };
        emitEvent(event);
        if (!m_rowAdded)
            return;
        // Every child of a grid is a row.
        RowBase& row = static_cast<RowBase&>(*m_rowAdded);
        m_rowAdded = nullptr;
        const Column* leading = nullptr;
        row.traverseLanes({ 0.0f, 0.0f }, [&](const RowCell& cell, const FloatRect&){
            if (!leading)
                leading = &cell.column;
        });
        if (leading)
            row.selectColumn(*leading);
        row.setFocus();
        row.scrollIntoViewOnAlign();
    }

    // The focus is left to the caller: a pick moves it, a restore of a grid off screen does not.
    void GridBase::selectCell(RowBase& row, const Column* column)
    {
        row.makeCurrentItem();
        if (column)
            row.selectColumn(*column);
        else
            m_descriptor.setSelectedCell(&row, nullptr);
    }

    ScaledSize GridBase::calculateContent(AlignEvent& event)
    {
        ScaledSize result = StackView::calculateContent(event);
        result.x = m_descriptor.m_rootColumn.calculatedSubColumnsWidth2();
        return result;
    }

    void GridBase::currentItemChanged(CurrentItemChangeEvent& params)
    {
        StackView::currentItemChanged(params);
        m_descriptor.setSelectedRow(currentItem());
    }

    // Only a grid that is not a container of its own. One that follows the user keeps its selection
    // whatever a container above does with the row.
    void GridBase::nestedCurrentItemChange(Control*)
    {
        if (interactivity() != Interactivity::ActiveContainer && isFieldRoot())
            m_descriptor.setSelectedCell(nullptr, nullptr);
    }

    void GridBase::adjustPaint(AdjustPaintEvent& event)
    {
        StackView::adjustPaint(event);
        event.setColorRules(UiElement::Grid);
    }

    void GridBase::adjustViewPort(AdjustViewportEvent& event) const
    {
        if (this == &m_descriptor.owner())
            return;
        const float shared = m_descriptor.sharedLinePixel();
        event.viewport.top -= shared;
        event.viewport.bottom += shared;
    }

    const Control* GridBase::heldHeader() const
    {
        if (!m_header || !m_header->visible())
            return nullptr;
        // A header on its own heads nothing, so there is nothing for it to stay in front of.
        if (controls().size() < 2)
            return nullptr;
        return m_header;
    }

    FloatPoint GridBase::overlayChildOffset(const Control& child) const
    {
        if (&child != heldHeader())
            return StackView::overlayChildOffset(child);
        // Every row of a grid shares one content origin, so the last row's bottom is measured in
        // the space the header's own topLeft is.
        return heldHeaderOffset(*m_header, controls().back()->bottom());
    }

    void GridBase::paintChildren(PaintEvent& event)
    {
        StackView::paintChildren(event);
        // Named children are not filtered the way a ranged-over child is, so the visibility test
        // that traverseChildren makes belongs here.
        if (!m_header || !m_header->visible())
            return;
        // The pass belonging to whichever container holds the header's entry, which is normally
        // the box's body or the box rather than the grid - see addHeader.
        if (!event.overlayStage() || event.overlayHost() != m_headerHost)
            return;

        const float travel = overlayChildOffset(*m_header).y;
        if (travel != 0.0f)
        {
            const FloatRect rect = FloatRect::FromDimensions(
                event.contentPosition() + m_header->topLeft() + FloatPoint{ 0.0f, travel },
                m_header->scaledSize());
            event.paintHeldBackdrop(m_header->silhouette(event, rect), travel);
        }
        event.paintChild(*m_header);
    }

    // Stated as a taller request rather than as a shorter viewport: the viewport belongs to the
    // box doing the scrolling, and that box knows nothing of what is held over it. The rect
    // arrives in this grid's own content space, which is the space the header's own strip is
    // measured in, so the two are added without conversion.
    //
    // Every child but the header. The strip is the room the header takes off the top of the view,
    // and the header is what stands in it: a header asking for its own height above itself names a
    // rect no part of it reaches.
    void GridBase::scrollChildIntoView(Control& control, FloatRect controlRect)
    {
        if (&control != m_header)
            controlRect.top -= heldHeaderStrip();
        StackView::scrollChildIntoView(control, controlRect);
    }

    void GridBase::nestedPressDown(PressDownEvent& event)
    {
        m_descriptor.beginCellSelection();
        StackView::nestedPressDown(event);
        m_descriptor.endCellSelection();
    }

    // The click that picks a cell never edits it, and a press that became a drag raises no click.
    void GridBase::nestedClick(NestedClickEvent& event)
    {
        if (clickEditsCell(event) && editSelectedCell(event.scaler()))
        {
            // The walk reads this once this returns, so stopping it here is what keeps the
            // controls above from being handed a gesture on the far side of the editor's own
            // message loop - by then the user has had every chance to change what they are.
            event.stopPropagation();
            return;
        }
        StackView::nestedClick(event);
    }

    bool GridBase::clickEditsCell(const NestedClickEvent& event) const
    {
        if (!Input::mouse().active())
            return false;
        const CellPress& press = m_descriptor.lastPress();
        if (press.kind != CellPressKind::Act || !press.column || press.row != &event.clickedControl)
            return false;
        const RowBase& row = static_cast<const RowBase&>(event.clickedControl);
        return row.laneColumnAt(event.clickPos()) == press.column;
    }

    void GridBase::nestedKeyDown(KeyDownEvent& event)
    {
        // Held with Alt, a key carries on to the shortcut scopes - Alt+Left is a browser's Back.
        if (event.modifiers.alt)
        {
            StackView::nestedKeyDown(event);
            return;
        }
        switch (event.key)
        {
        case Keys::Left:
        case Keys::Right:
        case Keys::Up:
        case Keys::Down:
            if (moveCellSelection(event))
            {
                event.handled = true;
                return;
            }
            // Nothing that way in the field, or this grid is not the root that answers for the
            // whole of it. The key travels on: to the root, and past it to the focus navigator,
            // which leaves the grid. Which band a row is entered at belongs to the move rather than
            // to the row, so the descriptor carries it to whichever row the navigator lands on.
            if (event.key == Keys::Up || event.key == Keys::Down)
                m_descriptor.setCellEntryEdge(event.key == Keys::Down
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd);
            break;
        case Keys::Home:
        case Keys::End:
            {
                // The ends of the line are the row's wherever the grid stands. Under Ctrl the keys
                // address a container, as the page keys do: a grid standing in one leaves them to
                // it, and the selection leaves the grid with them.
                if (event.modifiers.ctrl && interactivity() != Interactivity::ActiveContainer)
                    break;
                const ScrollDirection edge = event.key == Keys::Home
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                const bool moved = event.modifiers.ctrl
                    ? moveCellToGridEdge(edge)
                    : moveCellToLineEdge(event.scaler(), edge);
                if (moved)
                {
                    event.handled = true;
                    return;
                }
                break;
            }
        case Keys::Prior:
        case Keys::Next:
            {
                // The page keys address a container, and a grid standing in one leaves them to it.
                if (interactivity() != Interactivity::ActiveContainer)
                    break;
                const ScrollDirection direction = event.key == Keys::Next
                    ? ScrollDirection::ToEnd
                    : ScrollDirection::ToBegin;
                if (moveCellByPage(event.scaler(), direction))
                {
                    event.handled = true;
                    return;
                }
                break;
            }
        // Return follows the selected cell's link first, and both keys open the editor otherwise.
        // A cell answers them whenever its column opens an editor at all - which every text column
        // does, since EditorMode::ReadOnly is the default. So Return reaches the panel underneath
        // from a cell holding a control, and from a column that says EditorMode::None and holds no
        // link, and nowhere else. Nothing is taken from it here: Control::nestedKeyDown does
        // nothing with Return, and the form's keyboard-click flag is a visual pressed state rather
        // than a click.
        case Keys::F2:
        case Keys::Return:
            {
                if (event.key == Keys::Return && followSelectedCellLink(event))
                    return;
                if (editSelectedCell(event.scaler()))
                {
                    event.handled = true;
                    return;
                }
                break;
            }
        }
        StackView::nestedKeyDown(event);
    }

    // An arrow press reaches the focus navigator only once the field root has found nothing that
    // way in the whole field - a row this stack would step to is one the field already passed
    // over. The steps a page press is made of are the navigator's own, and walk the rows like any
    // stack's.
    Control* GridBase::navigationNeighbour(const Control& child, const NavigationStep& step)
    {
        if (step.source == NavigationSource::Key)
            return nullptr;
        return StackView::navigationNeighbour(child, step);
    }

    void GridBase::controlAdded(Control& control)
    {
        StackView::controlAdded(control);
        m_rowAdded = &control;
        if (m_newItemRow && &control != m_newItemRow)
            moveControl(*m_newItemRow, controls().size());
    }

    bool GridBase::defaultCanSelectItem(Control& value)
    {
        return &value != m_newItemRow && StackView::defaultCanSelectItem(value);
    }

    bool GridBase::defaultCanMoveItem(Control& value)
    {
        return &value != m_header && &value != m_newItemRow
            && StackView::defaultCanMoveItem(value);
    }

    bool GridBase::editSelectedCell(const Scaler& scaler)
    {
        if (!cellKeysApply())
            return false;

        FieldStops stops;
        const std::size_t sourceIndex = collectField(scaler, stops);
        // The selection is outside the collected window, so there is no rect to place an editor
        // over. Nothing else in this grid answers F2 either, so the key travels on.
        if (sourceIndex == k_maxSize)
            return false;

        const FieldStop& stop = stops[sourceIndex];
        // A stop with no column is the control a row without cells stands behind - an expander's
        // header - and that control answers its own keys.
        if (!stop.column || stop.column->editorMode() == EditorMode::None)
            return false;

        // Held as pointers rather than references: the row may be gone by the time the editor
        // returns, and the test below is an address comparison that must stay legal either way.
        RowBase* const row = stop.row;
        const Column* const column = stop.column;

        // **Only a cell drawn as text.** A cell holding a control is that control's - its row hands
        // it the key, see RowContainer::nestedKeyDown - and a box laid over it would be showing a
        // value the cell does not draw. The column cannot answer this: which cells hold controls is
        // the **row's**, so a column of plain text with one check box in it is asked per cell.
        if (row->cellControl(*column))
            return false;

        // A read-only cell opens the same editor with its box refusing every change, which is how
        // a value too long for its column is read whole and copied out of. The sink is still
        // handed over: offerText is what declines to run it, in one place for every caller.
        const bool readOnly = column->editorMode() == EditorMode::ReadOnly;

        // The editor is placed over the **text** it replaces, so what it is given is where that
        // text is drawn: the cell inside its lines, inset by the same padding and lead paintCell
        // insets it by, in this form's coordinates, which is the space a placement rect is stated
        // in.
        FloatRect textRect = m_descriptor.cellInnerRect(stop.rect);
        textRect.inflate(-m_descriptor.scaledCellMetrics().padding);
        textRect.left += row->cellLead(*column).x;

        // A cell may show more than its value - a mark, words beside it - so a source that
        // states the value apart is asked first.
        GetCellEditorTextEvent editorText{ *row, *column };
        row->getCellEditorText(editorText);
        Text text;
        if (editorText.text.has_value())
            text = std::move(*editorText.text);
        else
            row->doGetCellText(*column, text);

        // None, deliberately: paintCell lays a cell's text out across that same rect, and what
        // moves it to the other end of the column is the alignment in the column's format. The
        // editor takes both - the rect as its minimum width, and the format - so it reproduces
        // the same layout whichever way the column reads.
        const EditTarget target{
            .control = *row,
            .textRect = textRect,
            .textAnchor = HorizontalTextAnchor::None,
            // As wide as the grid can show and no wider. A value longer than its column is
            // worth seeing whole, which is half of why the editor is a window of its own; a
            // window wider than the grid it belongs to is not.
            .maxTextSize = { visibleRectInForm().width(), 0.0f },
            .readOnly = readOnly ? ReadOnly::Yes : ReadOnly::No,
            .suggester = editorText.suggester,
            .textFormat = &row->cellTextFormat(*column),
        };
        InPlaceEdit::run(target, text, [this, row, column](AcceptEditEvent& event){
            // This runs while the editor is up, on the stack of the loop it is pumping, so the
            // grid may have been worked on since the edit began.
            //
            // **The row is the test and the column is not.** What is being tested for is a deleted
            // row: it takes the selection with it, because RowBase's destructor tells the
            // descriptor, which drops the row it was holding. The selected **column** has usually
            // moved by the time this runs and means nothing here - the commonest way to end an
            // edit with the mouse is to click another cell, and the press that does it moves
            // the selection before the release closes the editor. The value belongs to the cell
            // the edit started on, which is what the captured column names.
            if (m_descriptor.selectedRow() != row)
                return;

            const std::wstring reason = row->acceptCellText(*column, event.text);
            if (!reason.empty())
            {
                event.refuse(reason);
                return;
            }
            // The value the cell reads has changed, and a column sized to its content is sized
            // to the old one until a pass says otherwise.
            invalidateFormAlign();
        });

        // **The pointer never moved and the hover went anyway.** The editor is a window of its own
        // over the cell, so this form is told the mouse left it as the editor opens - and nothing
        // tells it otherwise when the editor goes, because a still pointer raises no move. So the
        // form hit-tests the pointer where it last saw it, which puts the hover back on whatever
        // the pointer is on.
        //
        // **Only for an ending the keyboard drove.** An edit ended with the mouse ends with a
        // pointer event of its own, which has hit-tested where the pointer is. The mouse controller
        // answers this and the EditResult does not: Escape is as much a keyboard ending as Return.
        if (!Input::mouse().active())
            form().mouseTick(false);
        return true;
    }

    // The field is collected as editSelectedCell collects it, so the two keys agree on which cell
    // the selection names.
    bool GridBase::followSelectedCellLink(const KeyDownEvent& event)
    {
        if (!cellKeysApply())
            return false;
        FieldStops stops;
        const std::size_t sourceIndex = collectField(event.scaler(), stops);
        if (sourceIndex == k_maxSize)
            return false;
        const FieldStop& stop = stops[sourceIndex];
        if (!stop.row || !stop.column)
            return false;
        if (stop.column->editorMode() == EditorMode::Editable && !event.modifiers.ctrl)
            return false;
        return stop.row->followSelectedCellLink(event);
    }

    // **The field root answers every arrow**, over the whole field. A nested grid holds part of
    // it, and the line a run keeps wins over distance - so the nearest stop on the line inside a
    // sub-grid is not the nearest on it in the field, which may stand past the sub-grid's rows.
    bool GridBase::moveCellSelection(const KeyDownEvent& event)
    {
        if (!cellKeysApply() || !isFieldRoot())
            return false;

        FieldStops stops;
        std::size_t sourceIndex = collectField(event.scaler(), stops);
        // Scrolled out of view, the selection is still where a key moves from: the field is read
        // around it instead, and the stop landed on is scrolled into view.
        if (sourceIndex == k_maxSize)
        {
            stops.clear();
            sourceIndex = collectFieldAroundSelection(stops);
            if (sourceIndex == k_maxSize)
                return false;
        }

        const FieldTable table{ stops };
        const FloatRect sourceRect = stops[sourceIndex].rect;
        const FloatPoint band = cellBand(sourceRect);
        const bool horizontal = event.key == Keys::Left || event.key == Keys::Right;
        const bool forward = event.key == Keys::Right || event.key == Keys::Down;
        const std::size_t line = horizontal ? table.bandAt(band.y) : table.columnAt(band.x);
        const std::size_t bestIndex = stopAlong(table, stops, sourceIndex, line, !horizontal,
            forward, FieldReach::Nearest);
        if (bestIndex == k_maxSize)
            // Left and Right are the field's own axis, the columns being what they address, so a
            // press at the end of a line rests there rather than leaving the grid sideways. Up
            // and Down travel on, which is how a move leaves the grid at all.
            return horizontal;

        // The line is written before anything scrolls: a stop rect and the grid origin it is
        // measured against were read from one snapshot, and a scroll moves both.
        rememberCellLine(stops[bestIndex].rect, horizontal ? band.x : band.y, horizontal);
        focusStop(stops[bestIndex]);
        scrollStopIntoView(stops[bestIndex]);
        return true;
    }

    bool GridBase::moveCellToLineEdge(const Scaler& scaler, ScrollDirection edge)
    {
        if (!cellKeysApply())
            return false;
        // An edge is the edge of a whole line, and a line runs through rows a nested grid cannot
        // see: a group's span stands beside the rows it spans and belongs to the grid holding
        // the group. Only the outermost grid holds the whole field, so only it can answer.
        if (!isFieldRoot())
            return false;

        FieldStops stops;
        const std::size_t sourceIndex = collectField(scaler, stops);
        if (sourceIndex == k_maxSize)
            return false;

        const FieldTable table{ stops };
        const FloatRect sourceRect = stops[sourceIndex].rect;
        // The line these keys read runs through the stop the selection sits on. A step sideways
        // that found nothing on the line landed beside it, and an edge is measured from where
        // the selection is rather than from where the run started.
        const AxisSpan sourceSpan = { sourceRect.top, sourceRect.bottom };
        const float line = sourceSpan.clampInto(cellBand(sourceRect).y);
        const bool toBegin = edge == ScrollDirection::ToBegin;
        const std::size_t bestIndex = table.endStop(table.bandAt(line), toBegin);
        // The outermost grid answers these keys whenever the selection is in its field, so a
        // press at the edge of the line rests there. Letting it travel on hands it to the focus
        // navigator, which answers with an end of the container the grid is or stands in.
        if (bestIndex == sourceIndex || bestIndex == k_maxSize)
            return true;

        const FloatRect& targetRect = stops[bestIndex].rect;
        rememberCellLine(targetRect, toBegin ? targetRect.left : targetRect.right, true);
        focusStop(stops[bestIndex]);
        scrollStopIntoView(stops[bestIndex]);
        return true;
    }

    bool GridBase::moveCellToGridEdge(ScrollDirection edge)
    {
        if (!cellKeysApply())
            return false;
        // The whole grid is the outermost grid's business, so a sub-grid lets the key travel on
        // rather than answering with the end of its own few rows.
        if (!isFieldRoot())
            return false;

        RowBase* row = edgeRow(edge);
        if (!row)
            return true;
        const FloatPoint rowOrigin = row->parentContentOrigin() + row->topLeft();

        const bool toBegin = edge == ScrollDirection::ToBegin;
        Column* targetColumn = nullptr;
        FloatRect targetRect{};
        row->traverseCells(rowOrigin, [&](const RowCell& cell){
            if (cell.blank)
                return;
            // Reading order: the cell furthest towards the edge, and the one highest up among
            // those a column hierarchy stacks at the same distance.
            const bool further = toBegin
                ? cell.rect.left < targetRect.left
                    || (cell.rect.left == targetRect.left && cell.rect.top < targetRect.top)
                : cell.rect.right > targetRect.right
                    || (cell.rect.right == targetRect.right && cell.rect.bottom > targetRect.bottom);
            if (!targetColumn || further)
            {
                targetColumn = &cell.column;
                targetRect = cell.rect;
            }
        });
        if (!targetColumn)
            return true;

        // The band is read before the move writes to it: an unset line is seeded from the cell
        // landed on, and a set one is carried in and clamped there.
        const FloatPoint band = cellBand(targetRect);
        // The key names an edge, so the line across the move goes to that edge. The line along it
        // is carried into the cell, exactly as a step would carry it.
        rememberCellLine(targetRect, toBegin ? targetRect.left : targetRect.right, true);
        rememberCellLine(targetRect, band.y, false);

        const FieldStop target = {
            .row = row,
            .column = targetColumn,
            .origin = rowOrigin,
            .rect = targetRect,
        };
        focusStop(target);
        scrollStopIntoView(target);
        return true;
    }

    bool GridBase::moveCellByPage(const Scaler& scaler, ScrollDirection direction)
    {
        if (!cellKeysApply())
            return false;
        // A page crosses more rows than a nested grid holds, and the field it is measured over
        // is the whole one.
        if (!isFieldRoot())
            return false;

        // The page is the grid's own visible span, so what the user sees is what one press
        // crosses - read where the view comes to rest, since a press can arrive while the one
        // before it is still carrying the view. The field is collected a page deep to match: the
        // window otherwise reaches only a fixed margin past the viewport, and a stop beyond that
        // is one no press can score, so the key would come to rest just under the edge of the
        // screen however far it asked to go.
        FloatRect viewport = visibleRectInForm();
        viewport.offset(viewTravelRemaining());

        FieldStops stops;
        const std::size_t sourceIndex = collectField(scaler, stops, viewport.height());
        if (sourceIndex == k_maxSize)
            return false;

        const FieldTable table{ stops };
        const FloatRect sourceRect = stops[sourceIndex].rect;
        const FloatPoint band = cellBand(sourceRect);
        const bool forward = direction == ScrollDirection::ToEnd;
        const std::size_t column = table.columnAt(band.x);

        // Where the page being left behind ends. That is the selection: a press moves the
        // selection by a page, so the page it crosses starts where the selection stands, whether
        // the selection sits against the edge of the screen or in the middle of it. Scrolling can
        // carry the selection out of the viewport or behind a held header, and a page measured
        // from there names a band nothing reaches, so the last stop the viewport shows whole
        // stands in for it.
        std::size_t lastSeenIndex = sourceIndex;
        if (!showsWhole(viewport, stops[sourceIndex]))
        {
            const std::size_t seenIndex = lastWholeInColumn(table, stops, viewport, column,
                forward);
            if (seenIndex != k_maxSize)
                lastSeenIndex = seenIndex;
        }

        // The next page begins on the stop after it, so nothing is crossed twice and nothing
        // between the two pages is stepped over.
        const std::size_t nextIndex = stopAlong(table, stops, lastSeenIndex, column, true, forward,
            FieldReach::Nearest);
        if (nextIndex == k_maxSize)
        {
            // The field ends inside this page, so the press comes to rest where a run of arrow
            // presses would: on the last stop this way.
            const std::size_t endIndex = stopAlong(table, stops, sourceIndex, column, true, forward,
                FieldReach::Furthest);
            if (endIndex == k_maxSize)
                return true;
            rememberCellLine(stops[endIndex].rect, band.y, false);
            focusStop(stops[endIndex]);
            scrollStopIntoView(stops[endIndex]);
            return true;
        }

        // The view the press arrives at, a viewport's height from that stop. Going down, the
        // stop's row stands at the top, under every header held over it and the margin the box
        // keeps clear of its client edge; going up, it ends at the bottom edge.
        const FieldStop& next = stops[nextIndex];
        FloatRect arriving = viewport;
        arriving.top = forward
            ? next.rect.top - next.row->heldStackStrip() - scaler.scaled8
            : next.rect.bottom - viewport.height();
        arriving.bottom = arriving.top + viewport.height();

        // The selection lands at the **far** edge of that view rather than at its start: the press
        // moves the view by a page and leaves the selection against the edge it travelled to,
        // which is where the next press measures its own page from.
        std::size_t bestIndex = lastWholeInColumn(table, stops, arriving, column, forward);
        if (bestIndex == k_maxSize)
            bestIndex = nextIndex;

        // Everything read out of the field is written back before anything scrolls: a stop rect,
        // the grid origin it is measured against and the view all come from one snapshot, and a
        // scroll moves all three.
        rememberCellLine(stops[bestIndex].rect, band.y, false);
        scrollViewToStop(next, arriving);
        // The view is on its way, so this settles the focus without moving it: the stop it lands
        // on is one that view shows whole.
        focusStop(stops[bestIndex]);
        return true;
    }

    bool GridBase::cellKeysApply() const
    {
        const Control* sourceStop = m_descriptor.selectedRow();
        if (!sourceStop)
            return false;
        // isFocused() is the test to use rather than Input::focusedControl(): the focus is held
        // by the container that answers for the row, and only focusDelegate() names the row.
        return sourceStop->isFocused();
    }

    bool GridBase::isFieldRoot() const
    {
        return this == &m_descriptor.owner();
    }

    // **What is on screen is what a key can move to.** A row further out than this is one the walk
    // would have to reach for and the eye cannot follow, and the count of rows a grid may hold has
    // no upper bound. The window is the grid's own visible part rather than the whole form: a row
    // scrolled out of the box holding the grid is as far out of reach as one past the end of the
    // grid. A key that reaches further than one step passes the distance it crosses as lookAhead,
    // since nothing further out is collected and no stop there can be landed on.
    std::size_t GridBase::collectField(const Scaler& scaler, FieldStops& stops, float lookAhead)
    {
        FloatRect window = visibleRectInForm();
        window.inflate(scaler.scaled48 + lookAhead);
        return collectFieldIn(window, stops);
    }

    std::size_t GridBase::collectFieldAroundSelection(FieldStops& stops)
    {
        const Control* sourceStop = m_descriptor.selectedRow();
        if (!sourceStop)
            return k_maxSize;
        const FloatRect viewport = visibleRectInForm();
        FloatRect window = sourceStop->boundsInForm();
        window.inflate(viewport.width(), viewport.height());
        return collectFieldIn(window, stops);
    }

    std::size_t GridBase::collectFieldIn(const FloatRect& window, FieldStops& stops)
    {
        // The selection is a cell of a row, or the control a row that has no cells stands behind
        // - an expander's header. Either can be the place a move starts from.
        const Control* sourceStop = m_descriptor.selectedRow();
        const Column* sourceColumn = m_descriptor.selectedColumn();
        std::size_t result = k_maxSize;
        traverseRows(window, [&](RowBase& row, FloatPoint rowOrigin){
            // **A cell is a stop only in a row the user can be on.** The focus never rests on a
            // control of Interactivity::None, and the field reads that the way the navigator
            // and a mouse press do. A grid's header is one: it paints cells, and a key passes
            // them over.
            if (row.canTakeFocus())
            {
                row.traverseLanes(rowOrigin, [&](const RowCell& cell, const FloatRect& lane){
                    if (sourceColumn && &row == sourceStop && &cell.column == sourceColumn)
                        result = stops.size();
                    stops.push_back({
                        .row = &row,
                        .column = &cell.column,
                        .origin = rowOrigin,
                        .rect = cell.rect,
                        .lane = lane,
                    });
                });
            }
            const RowStop stop = row.navigationStop();
            if (stop.control && stop.control->visible())
            {
                if (stop.control == sourceStop)
                    result = stops.size();
                stops.push_back({
                    .row = &row,
                    .control = stop.control,
                    .origin = rowOrigin,
                    .rect = stop.rect,
                    .lane = stop.rect,
                });
            }
        });
        return result;
    }

    // **On the line first, however far**: a run of Down keeps its column across rows that hold
    // nothing in it. Only where nothing ahead stands on the line does the step take the nearest
    // place ahead holding anything, at the line nearest the kept one - and a step back along the
    // kept line still finds the source, since nothing between the two stands on it. A stop is
    // ahead only when it starts past the whole of the source, so a span standing beside the
    // source is never a step down from it.
    std::size_t GridBase::stopAlong(const FieldTable& table, const FieldStops& stops,
        const std::size_t sourceIndex, const std::size_t line, const bool vertical,
        const bool forward, const FieldReach reach)
    {
        const auto spanOf = [&](const std::size_t index) {
            return vertical ? table.bandsOf(stops[index]) : table.columnsOf(stops[index]);
        };
        const auto cellAt = [&](const std::size_t along, const std::size_t across) {
            return vertical ? table.stopAt(along, across) : table.stopAt(across, along);
        };
        const IndexSpan source = spanOf(sourceIndex);
        const std::size_t count = vertical ? table.bands() : table.columns();
        const std::size_t lines = vertical ? table.columns() : table.bands();
        // The places ahead of the source, first included and end not.
        const std::size_t first = forward ? source.last + 1 : 0;
        const std::size_t end = forward ? count : source.first;
        if (first >= end)
            return k_maxSize;
        const bool fromFirst = forward == (reach == FieldReach::Nearest);
        const auto placeAt = [&](const std::size_t step) {
            return fromFirst ? first + step : end - 1 - step;
        };
        const auto isAhead = [&](const std::size_t index) {
            const IndexSpan span = spanOf(index);
            return forward ? span.first > source.last : span.last < source.first;
        };

        for (std::size_t step = 0; step != end - first; ++step)
        {
            const std::size_t stop = cellAt(placeAt(step), line);
            if (stop != k_maxSize && isAhead(stop))
                return stop;
        }
        for (std::size_t step = 0; step != end - first; ++step)
        {
            std::size_t result = k_maxSize;
            std::size_t nearest = k_maxSize;
            for (std::size_t across = 0; across != lines; ++across)
            {
                const std::size_t stop = cellAt(placeAt(step), across);
                if (stop == k_maxSize || !isAhead(stop))
                    continue;
                const std::size_t distance = across > line ? across - line : line - across;
                if (distance < nearest)
                {
                    nearest = distance;
                    result = stop;
                }
            }
            if (result != k_maxSize)
                return result;
        }
        return k_maxSize;
    }

    FloatPoint GridBase::cellBand(const FloatRect& sourceRect)
    {
        const FloatPoint gridOrigin = m_descriptor.owner().boundsInForm().topLeft();
        FloatPoint result = sourceRect.center();
        if (m_descriptor.desiredCellX() == k_maxFloat)
            m_descriptor.setDesiredCellX(result.x - gridOrigin.x);
        else
            result.x = m_descriptor.desiredCellX() + gridOrigin.x;
        if (m_descriptor.desiredCellY() == k_maxFloat)
            m_descriptor.setDesiredCellY(result.y - gridOrigin.y);
        else
            result.y = m_descriptor.desiredCellY() + gridOrigin.y;
        return result;
    }

    void GridBase::rememberCellLine(const FloatRect& targetRect, float band, bool horizontal)
    {
        // Both lines live in the owning grid's space, since rows sit at different depths and the
        // grid scrolls under them.
        const FloatPoint gridOrigin = m_descriptor.owner().boundsInForm().topLeft();
        if (horizontal)
        {
            const AxisSpan targetSpan = { targetRect.left, targetRect.right };
            m_descriptor.setDesiredCellX(targetSpan.clampInto(band) - gridOrigin.x);
        }
        else
        {
            const AxisSpan targetSpan = { targetRect.top, targetRect.bottom };
            m_descriptor.setDesiredCellY(targetSpan.clampInto(band) - gridOrigin.y);
        }
    }

    void GridBase::focusStop(const FieldStop& stop)
    {
        if (stop.control)
        {
            // No column: the stop is not a cell, so the grid shows no selected cell while the
            // focus rests here, and the control paints its own state. Recorded before the focus
            // moves, so that the grid's own item selection - which rewrites the row half as the
            // focus settles - finds the column already cleared.
            m_descriptor.setSelectedCell(stop.control, nullptr);
            stop.control->setFocus();
            return;
        }
        stop.row->selectColumn(*stop.column);
        stop.row->setFocus();
    }

    void GridBase::scrollStopIntoView(const FieldStop& stop)
    {
        if (stop.control)
        {
            stop.control->scrollIntoView();
            return;
        }
        // The cell, not the row: a row wider than the viewport counts as visible the moment any
        // part of it is, so scrolling the row leaves a cell at the far end off screen.
        FloatRect cellRect = stop.rect;
        cellRect.offset(-stop.origin);
        stop.row->scrollCellIntoView(cellRect);
    }

    // What the row asks for starts under the whole stack and the margin the box keeps clear of
    // its client edge, and each holder on the way up puts its own strip back. The box is handed
    // the view less that margin - taller than its client area, which it can only show by putting
    // the leading edge against its own.
    void GridBase::scrollViewToStop(const FieldStop& stop, const FloatRect& viewInForm)
    {
        FloatRect rect = viewInForm;
        rect.top += stop.row->heldStackStrip() + scaler().scaled8;
        rect.offset(-stop.origin);
        stop.row->scrollIntoView(rect);
    }

    bool GridBase::showsWhole(const FloatRect& within, const FieldStop& stop)
    {
        const AxisSpan shown = { within.top + stop.row->heldStackStrip(), within.bottom };
        return shown.covers({ stop.rect.top, stop.rect.bottom });
    }

    std::size_t GridBase::lastWholeInColumn(const FieldTable& table, const FieldStops& stops,
        const FloatRect& within, const std::size_t column, const bool forward)
    {
        const std::size_t bands = table.bands();
        for (std::size_t i = 0; i != bands; ++i)
        {
            const std::size_t stop = table.stopAt(forward ? bands - 1 - i : i, column);
            if (stop != k_maxSize && showsWhole(within, stops[stop]))
                return stop;
        }
        return k_maxSize;
    }

    RowBase* GridBase::edgeRow(ScrollDirection edge)
    {
        const bool toBegin = edge == ScrollDirection::ToBegin;
        ControlSpan rows = controls();
        for (std::size_t i = 0; i != rows.size(); ++i)
        {
            // Every child of a grid is a row.
            RowBase& row = static_cast<RowBase&>(*rows[toBegin ? i : rows.size() - 1 - i]);
            if (!row.visible() || !row.enabled(true))
                continue;
            RowBase* span = row.spanRow();
            GridBase* nested = row.subGrid();
            // A row comes before the span standing beside the rows it spans, and both come
            // before those rows. Read the other way round when the search runs from the end.
            const bool spanCounts = span && span->visible() && rowHoldsCellStops(*span);
            // The rows of a collapsed expander are hidden, so the end of the grid is the last
            // one still shown.
            const bool nestedCounts = nested && nested->visible();
            if (toBegin)
            {
                if (rowHoldsCellStops(row))
                    return &row;
                if (spanCounts)
                    return span;
                if (nestedCounts)
                    if (RowBase* result = nested->edgeRow(edge))
                        return result;
            }
            else
            {
                if (nestedCounts)
                    if (RowBase* result = nested->edgeRow(edge))
                        return result;
                if (spanCounts)
                    return span;
                if (rowHoldsCellStops(row))
                    return &row;
            }
        }
        return nullptr;
    }

    // The rule collectFieldIn states, read off the structure: a cell is a stop only in a row the
    // user can be on, and a row holds one only where it paints a cell.
    bool GridBase::rowHoldsCellStops(RowBase& row)
    {
        if (!row.canTakeFocus())
            return false;
        bool result = false;
        row.traverseCells([&](const RowCell&){
            result = true;
        });
        return result;
    }

    void GridBase::selectedRowChanged(Control* from, Control* to)
    {
        // **Both paths, whole.** A row's state belongs to the path it stands on - every row that
        // holds the selected cell inside it is in effect - so both paths are told, from the row
        // to the grid. The grid is where they end: a control above it holds the same answer
        // whatever the grid's selection does.
        //
        // Not stopped where the two paths meet. Stopping there rests on the meeting point's
        // answer being unchanged, which is true - and on it having been right in the first
        // place, which is a claim about every earlier move rather than about this one. A control
        // whose answer has not changed recomputes it and settles on the same value, and an
        // animation already heading there is left alone - see the restart guard - so telling it
        // twice costs nothing and telling it never is unrecoverable.
        invalidateStateUp(from, this, InvalidateEvent::None);
        invalidateStateUp(to, this, InvalidateEvent::None);
    }

    // GridBase::FieldTable

    // **The edges the stops share are the lines of the field.** Cells tile it, so a stop's top is
    // the bottom of the one above, and each edge that meets another is one line. Edges within the
    // tolerance are one: a group's span is as tall as its rows by a sum of its own.
    GridBase::FieldTable::FieldTable(const FieldStops& stops)
    {
        for (const FieldStop& stop : stops)
        {
            bandEdges.push_back(stop.rect.top);
            bandEdges.push_back(stop.rect.bottom);
            columnEdges.push_back(stop.lane.left);
            columnEdges.push_back(stop.lane.right);
        }
        mergeEdges(bandEdges);
        mergeEdges(columnEdges);
        cells.assign(bands() * columns(), k_maxSize);
        if (cells.empty())
            return;
        for (std::size_t index = 0; index != stops.size(); ++index)
        {
            const IndexSpan stopBands = bandsOf(stops[index]);
            const IndexSpan stopColumns = columnsOf(stops[index]);
            for (std::size_t band = stopBands.first; band <= stopBands.last; ++band)
            {
                for (std::size_t column = stopColumns.first; column <= stopColumns.last; ++column)
                {
                    std::size_t& cell = cells[band * columns() + column];
                    if (cell == k_maxSize)
                        cell = index;
                }
            }
        }
    }

    std::size_t GridBase::FieldTable::bands() const
    {
        return bandEdges.size() < 2 ? 0 : bandEdges.size() - 1;
    }

    std::size_t GridBase::FieldTable::columns() const
    {
        return columnEdges.size() < 2 ? 0 : columnEdges.size() - 1;
    }

    std::size_t GridBase::FieldTable::stopAt(const std::size_t band, const std::size_t column) const
    {
        if (band >= bands() || column >= columns())
            return k_maxSize;
        return cells[band * columns() + column];
    }

    std::size_t GridBase::FieldTable::bandAt(const float y) const
    {
        return spanAt(bandEdges, y);
    }

    std::size_t GridBase::FieldTable::columnAt(const float x) const
    {
        return spanAt(columnEdges, x);
    }

    GridBase::IndexSpan GridBase::FieldTable::bandsOf(const FieldStop& stop) const
    {
        return spansBetween(bandEdges, stop.rect.top, stop.rect.bottom);
    }

    // The lane, not the rect: a blank is answered for by the cell nearest it.
    GridBase::IndexSpan GridBase::FieldTable::columnsOf(const FieldStop& stop) const
    {
        return spansBetween(columnEdges, stop.lane.left, stop.lane.right);
    }

    std::size_t GridBase::FieldTable::endStop(const std::size_t band, const bool atStart) const
    {
        const std::size_t count = columns();
        for (std::size_t i = 0; i != count; ++i)
        {
            const std::size_t stop = stopAt(band, atStart ? i : count - 1 - i);
            if (stop != k_maxSize)
                return stop;
        }
        return k_maxSize;
    }

    std::size_t GridBase::FieldTable::edgeAt(const Edges& edges, const float value)
    {
        const auto found = std::ranges::lower_bound(edges, value - k_edgeTolerance);
        return static_cast<std::size_t>(found - edges.begin());
    }

    // A value past either end is held by the span at that end.
    std::size_t GridBase::FieldTable::spanAt(const Edges& edges, const float value)
    {
        if (edges.size() < 2)
            return 0;
        const auto above = std::ranges::upper_bound(edges, value);
        const std::size_t index = static_cast<std::size_t>(above - edges.begin());
        return index == 0 ? 0 : std::min(index - 1, edges.size() - 2);
    }

    GridBase::IndexSpan GridBase::FieldTable::spansBetween(const Edges& edges, const float from,
        const float to)
    {
        if (edges.size() < 2)
            return {};
        const std::size_t spans = edges.size() - 1;
        const std::size_t first = std::min(edgeAt(edges, from), spans - 1);
        const std::size_t end = std::min(edgeAt(edges, to), spans);
        return { first, end > first ? end - 1 : first };
    }

    void GridBase::FieldTable::mergeEdges(Edges& edges)
    {
        std::ranges::sort(edges);
        Edges merged;
        for (const float edge : edges)
        {
            if (merged.empty() || edge - merged.back() > k_edgeTolerance)
                merged.push_back(edge);
        }
        edges = std::move(merged);
    }

}
