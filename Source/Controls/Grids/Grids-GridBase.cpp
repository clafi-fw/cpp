module ClaFi.Controls.Grids;

import :RowBase;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.StackPanelBase;
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
        for (ControlSpan::iterator it = firstChildInViewport(window); it != rows.end(); ++it)
        {
            Control& child = **it;
            // Visibility before the end test, the order traverseChildren uses: a hidden row
            // keeps whatever position it last had, and reading that as the end of the range
            // would hide every row after it.
            if (!child.visible())
                continue;
            if (child.isViewportEnd(window.right, window.bottom))
                break;
            if (!child.enabled(true))
                continue;
            // A held header is drawn on a line of its own, while the cells reported for it here
            // are the ones it was laid out with, which the scroll has carried away from where the
            // eye reads them. A key landing on one of those would select a cell nothing shows and
            // move the view nowhere, so the header rejoins the field when it comes back to rest.
            if (&child == m_header && isHeaderHeld())
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
        // The entry goes to the control the grid stands in, not to the grid. The pass an entry
        // opens runs inside its host's clip, and what a held header draws around itself - its
        // shadow - falls on whatever the grid stands on rather than stopping at the grid's edge,
        // which is exactly the grid's own width. A parent with no overlay list leaves the grid
        // hosting the entry itself, which costs the shadow the room outside the grid and nothing
        // else.
        //
        // Standard clipping either way: the header is clipped to where it is DRAWN rather than to
        // where it was laid out, because the traversal has already moved its bounds by
        // overlayChildOffset.
        m_headerHost = parent();
        if (!m_headerHost || !m_headerHost->addOverlayControl(*m_header, ClippingMode::Standard))
        {
            m_headerHost = this;
            addOverlayControl(*m_header, ClippingMode::Standard);
        }
        return *m_header;
    }

    ScaledDimensions GridBase::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result = StackView::calculateContent(event);
        result.x = m_descriptor.m_rootColumn.calculatedSubColumnsWidth2();
        return result;
    }

    void GridBase::currentItemChanged(CurrentItemChangeEvent& params)
    {
        StackView::currentItemChanged(params);
        m_descriptor.setSelectedRow(currentItem());
    }

    void GridBase::adjustPaint(AdjustPaintEvent& event)
    {
        StackView::adjustPaint(event);
        event.setColorRules(UiElement::Grid);
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
            return {};
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
        // the grid's parent rather than the grid - see addHeader.
        if (!event.overlayStage() || event.overlayHost() != m_headerHost)
            return;

        const float travel = overlayChildOffset(*m_header).y;
        if (travel != 0.0f)
        {
            const FloatRect rect = FloatRect::fromDimensions(
                event.contentPosition() + m_header->topLeft() + FloatPoint{ 0.0f, travel },
                m_header->dimensions());
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

    void GridBase::pressDown(PressDownEvent& event)
    {
        m_descriptor.beginCellSelection();
        StackView::pressDown(event);
        m_descriptor.endCellSelection();
    }

    void GridBase::doubleClick(DoubleClickEvent& event)
    {
        if (editSelectedCell())
        {
            // The walk reads this once this returns, so stopping it here is what keeps the
            // controls above from being handed a gesture on the far side of the editor's own
            // message loop - by then the user has had every chance to change what they are.
            event.stopPropagation();
            return;
        }
        StackView::doubleClick(event);
    }

    void GridBase::keyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
        case Keys::Left:
        case Keys::Right:
        case Keys::Up:
        case Keys::Down:
            if (moveCellSelection(event.key))
            {
                event.handled = true;
                return;
            }
            // Nothing that way in this grid. The key travels on: to the grid holding this one,
            // whose field is wider, and past it to the focus navigator, which leaves the grid.
            // Which band a row is entered at belongs to the move rather than to the row, so the
            // descriptor carries it to whichever row the navigator lands on.
            if (event.key == Keys::Up || event.key == Keys::Down)
                m_descriptor.setCellEntryEdge(event.key == Keys::Down
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd);
            break;
        case Keys::Home:
        case Keys::End:
            {
                const ScrollDirection edge = event.key == Keys::Home
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                const bool moved = event.modifiers.ctrl
                    ? moveCellToGridEdge(edge)
                    : moveCellToLineEdge(edge);
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
                const ScrollDirection direction = event.key == Keys::Next
                    ? ScrollDirection::ToEnd
                    : ScrollDirection::ToBegin;
                if (moveCellByPage(direction))
                {
                    event.handled = true;
                    return;
                }
                break;
            }
        // Both mean the same thing, and a cell answers them whenever its column opens an editor
        // at all - which every text column does, since EditorMode::ReadOnly is the default. So
        // Return reaches the panel underneath from a cell holding a control, and from a column
        // that says EditorMode::None, and nowhere else. Nothing is taken from it here:
        // Control::keyDown does nothing with Return, and the form's keyboard-click flag is a
        // visual pressed state rather than a click.
        case Keys::F2:
        case Keys::Return:
            {
                if (editSelectedCell())
                {
                    event.handled = true;
                    return;
                }
                break;
            }
        }
        StackView::keyDown(event);
    }

    bool GridBase::editSelectedCell()
    {
        if (!cellKeysApply())
            return false;

        FieldStops stops;
        const std::size_t sourceIndex = collectField(stops);
        // The selection is outside the collected window, so there is no rect to place an editor
        // over. Nothing else in this grid answers F2 either, so the key travels on.
        if (sourceIndex == k_maxSize)
            return false;

        const FieldStop& stop = stops[sourceIndex];
        // A stop with no column is the control a row without cells stands behind - an expander's
        // button - and that control answers its own keys.
        if (!stop.column || stop.column->editorMode() == EditorMode::None)
            return false;

        // Held as pointers rather than references: the row may be gone by the time the editor
        // returns, and the test below is an address comparison that must stay legal either way.
        RowBase* const row = stop.row;
        const Column* const column = stop.column;

        // ONLY A CELL DRAWN AS TEXT. A cell holding a control is that control's - its row hands
        // it the key, see RowContainer::keyDown - and a box laid over it would be showing a value
        // the cell does not draw. The column cannot answer this: which cells hold controls is the
        // ROW's, so a column of plain text with one checkbox in it is asked per cell.
        if (row->cellControl(*column))
            return false;

        // A read-only cell opens the same editor with its box refusing every change, which is how
        // a value too long for its column is read whole and copied out of. The sink is still
        // handed over: offerText is what declines to run it, in one place for every caller.
        const bool readOnly = column->editorMode() == EditorMode::ReadOnly;

        // The editor is placed over the TEXT it replaces, so what it is given is where that text
        // is drawn: the cell inset by the same padding and lead paintCell insets it by, in this
        // form's coordinates, which is the space a placement rect is stated in.
        FloatRect textRect = stop.rect;
        textRect.inflate(-m_descriptor.scaledCellMetrics().padding.toFloat());
        textRect.left += row->cellLead(*column).x;

        Text text;
        row->doGetCellText(*column, text);

        // Left, deliberately: paintCell lays a cell's text out left-anchored in that same rect,
        // and what moves it to the other end of the column is the TextAlign doGetCellText puts
        // at the head of the text. The editor takes both - the rect as its minimum width, and
        // the text - so it reproduces the same layout whichever way the column reads.
        const EditTarget target{
            *row,
            textRect,
            HorizontalTextAnchor::Left,
            // As wide as the grid can show and no wider. A value longer than its column is
            // worth seeing whole, which is half of why the editor is a window of its own; a
            // window wider than the grid it belongs to is not.
            { visibleRectInForm().width(), 0.0f },
            readOnly ? ReadOnly::Yes : ReadOnly::No
        };
        InPlaceEdit::run(target, text, [this, row, column](AcceptEditEvent& event){
            // This runs while the editor is up, on the stack of the loop it is pumping, so the
            // grid may have been worked on since the edit began.
            //
            // THE ROW IS THE TEST AND THE COLUMN IS NOT. What is being tested for is a deleted
            // row: it takes the selection with it, because RowBase's destructor tells the
            // descriptor, which drops the row it was holding. The selected COLUMN has usually
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

        // THE POINTER NEVER MOVED AND THE HOVER WENT ANYWAY. The editor is a window of its own
        // over the cell, so this form is told the mouse left it as the editor opens - and nothing
        // tells it otherwise when the editor goes, because a still pointer raises no move. The
        // cell the user is on would be left reading as though nothing were over it while the
        // pointer sits right on top of it.
        //
        // ONLY FOR AN ENDING THE KEYBOARD DROVE. An edit ended with the mouse ends WHERE THE
        // POINTER IS, which is somewhere the user has just chosen - putting the hover back on the
        // cell they were editing would be taking it away from them. The device answers this and
        // the EditResult does not: Escape is as much a keyboard ending as Return, and the cell
        // the user backed out of is still the cell they are on.
        //
        // The row is asked for again rather than trusted: the edit ran a loop of its own, and a
        // row deleted underneath it took the selection with it - see the sink above.
        if (Input::device() == InputDevice::Keyboard && m_descriptor.selectedRow() == row)
        {
            Input::setHoveredControl(row);
            // No hint. Nothing was pointed at, so there is nothing for a tooltip to have been
            // aimed at either - the same reason the keyboard's own cell entry passes false.
            row->setHoveredColumn(column, false);
        }
        return true;
    }

    bool GridBase::moveCellSelection(KeyCode key)
    {
        if (!cellKeysApply())
            return false;

        FieldStops stops;
        const std::size_t sourceIndex = collectField(stops);
        // The selection is out of view, so there is no position to move from. The navigator
        // takes over and lands on a row that is in view.
        if (sourceIndex == k_maxSize)
            return false;

        const FloatRect sourceRect = stops[sourceIndex].rect;
        const FloatPoint band = cellBand(sourceRect);
        const bool horizontal = key == Keys::Left || key == Keys::Right;
        const std::size_t bestIndex = searchField(
            stops,
            OrientedRect::orient(sourceRect, key),
            OrientedRect::orient(band, key),
            key,
            sourceIndex,
            FieldReach::Nearest
        );
        if (bestIndex == k_maxSize)
            // Left and Right are the field's own axis - the columns are what they address - so
            // the field root answers them whenever the selection is in its field, and a press at
            // the end of the line rests there. Letting one travel on hands it to the focus
            // navigator, whose source is the whole ROW: everything ahead of the row's centre
            // scores, so a press at the last column lands on whatever control stands further
            // right on some other row, an expander's button among them. Up and Down do travel
            // on, which is how a move leaves the grid at all.
            return horizontal && isFieldRoot();

        // The line is written before anything scrolls: a stop rect and the grid origin it is
        // measured against were read from one snapshot, and a scroll moves both.
        rememberCellLine(stops[bestIndex].rect, horizontal ? band.x : band.y, horizontal);
        focusStop(stops[bestIndex]);
        scrollStopIntoView(stops[bestIndex]);
        return true;
    }

    bool GridBase::moveCellToLineEdge(ScrollDirection edge)
    {
        if (!cellKeysApply())
            return false;
        // An edge is the edge of a whole line, and a line runs through rows a nested grid cannot
        // see: a group's span stands beside the rows it spans and belongs to the grid holding
        // the group. Only the outermost grid holds the whole field, so only it can answer.
        if (!isFieldRoot())
            return false;

        FieldStops stops;
        const std::size_t sourceIndex = collectField(stops);
        if (sourceIndex == k_maxSize)
            return false;

        const FloatRect sourceRect = stops[sourceIndex].rect;
        // The line these keys read runs through the stop the selection sits on. A step sideways
        // that found nothing on the line landed beside it, and an edge is measured from where
        // the selection is rather than from where the run started.
        const AxisSpan sourceSpan = { sourceRect.top, sourceRect.bottom };
        const float line = sourceSpan.clampInto(cellBand(sourceRect).y);
        const bool toBegin = edge == ScrollDirection::ToBegin;

        std::size_t bestIndex = sourceIndex;
        for (std::size_t i = 0; i != stops.size(); ++i)
        {
            const FloatRect& rect = stops[i].rect;
            if (!AxisSpan{ rect.top, rect.bottom }.contains(line))
                continue;
            const FloatRect& best = stops[bestIndex].rect;
            // Reading order: the stop furthest towards the edge, and the one highest up among
            // those a column hierarchy stacks at the same distance.
            const bool further = toBegin
                ? rect.left < best.left
                    || (rect.left == best.left && rect.top < best.top)
                : rect.right > best.right
                    || (rect.right == best.right && rect.bottom > best.bottom);
            if (further)
                bestIndex = i;
        }
        // The outermost grid answers these keys whenever the selection is in its field, so a
        // press at the edge of the line rests there. Letting it travel on hands it to the focus
        // navigator, which answers with the first or last row of the grid.
        if (bestIndex == sourceIndex)
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

    bool GridBase::moveCellByPage(ScrollDirection direction)
    {
        if (!cellKeysApply())
            return false;
        // A page crosses more rows than a nested grid holds, and the field it is measured over
        // is the whole one.
        if (!isFieldRoot())
            return false;

        // The page is the grid's own visible span, so what the user sees is what one press
        // crosses. The field is collected a page deep to match: the window otherwise reaches
        // only a fixed margin past the viewport, and a stop beyond that is one no press can
        // score, so the key would come to rest just under the edge of the screen however far it
        // asked to go.
        // The part of the grid's visible span the eye can actually read: the header stands over
        // the top of it, and a stop behind the header has been read no more than one off screen
        // has. That holds for both of the answers this rect gives - how far one press travels,
        // and which stop the selection is measured from when it has been scrolled out of sight.
        FloatRect viewport = visibleRectInForm();
        viewport.top += heldHeaderStrip();
        const float pageExtent = viewport.height();

        FieldStops stops;
        const std::size_t sourceIndex = collectField(stops, pageExtent);
        if (sourceIndex == k_maxSize)
            return false;

        const FloatRect sourceRect = stops[sourceIndex].rect;
        const FloatPoint band = cellBand(sourceRect);
        const bool forward = direction == ScrollDirection::ToEnd;
        const KeyCode key = forward ? Keys::Down : Keys::Up;
        const OrientedRect line = OrientedRect::orient(band, key);

        // Where the page being left behind ends. That is the selection: a press moves the
        // selection by a page, so the page it crosses starts where the selection stands, whether
        // the selection sits against the edge of the screen or in the middle of it. Scrolling can
        // carry the selection out of the viewport, and a page measured from there names a band
        // nothing reaches, so the last stop the viewport shows WHOLE stands in for it - a stop
        // half on screen is not one the eye has read.
        const AxisSpan viewportSpan = { viewport.top, viewport.bottom };
        std::size_t lastSeenIndex = sourceIndex;
        if (!viewportSpan.covers({ sourceRect.top, sourceRect.bottom }))
        {
            const std::size_t seenIndex = lastWholeInBand(stops, viewport, line, key);
            if (seenIndex != k_maxSize)
                lastSeenIndex = seenIndex;
        }

        // The next page begins on the stop after it, so nothing is crossed twice and nothing
        // between the two pages is stepped over.
        const std::size_t nextIndex = searchField(stops,
            OrientedRect::orient(stops[lastSeenIndex].rect, key),
            line, key, lastSeenIndex, FieldReach::Nearest);
        if (nextIndex == k_maxSize)
        {
            // The field ends inside this page, so the press comes to rest where a run of arrow
            // presses would: on the last stop this way.
            const std::size_t endIndex = searchField(stops, OrientedRect::orient(sourceRect, key),
                line, key, sourceIndex, FieldReach::Furthest);
            if (endIndex == k_maxSize)
                return true;
            rememberCellLine(stops[endIndex].rect, band.y, false);
            focusStop(stops[endIndex]);
            scrollStopIntoView(stops[endIndex]);
            return true;
        }

        // The page that stop opens, laid against the edge the press travelled from.
        const FloatRect& nextRect = stops[nextIndex].rect;
        FloatRect page = viewport;
        page.top = forward ? nextRect.top : nextRect.bottom - pageExtent;
        page.bottom = page.top + pageExtent;

        // The selection lands at the FAR edge of that page rather than at its start: the press
        // moves the view by a page and leaves the selection against the edge it travelled to,
        // which is where the next press measures its own page from.
        std::size_t bestIndex = lastWholeInBand(stops, page, line, key);
        if (bestIndex == k_maxSize)
            bestIndex = nextIndex;

        // Everything read out of the field is written back before anything scrolls: a stop rect,
        // the grid origin it is measured against and the page all come from one snapshot, and a
        // scroll moves all three.
        rememberCellLine(stops[bestIndex].rect, band.y, false);
        scrollPageIntoView(page);
        // The page is already on screen, so this settles the focus without moving the view: the
        // stop it lands on is one the page shows whole.
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

    std::size_t GridBase::collectField(FieldStops& stops, float lookAhead)
    {
        // The selection is a cell of a row, or the control a row that has no cells stands behind
        // - an expander button. Either can be the place a move starts from.
        const Control* sourceStop = m_descriptor.selectedRow();
        const Column* sourceColumn = m_descriptor.selectedColumn();
        std::size_t result = k_maxSize;

        // What is on screen is what a key can move to. A row further out than this is one the
        // walk would have to reach for and the eye cannot follow, and the count of rows a grid
        // may hold has no upper bound. The window is the grid's own visible part rather than the
        // whole form: a row scrolled out of the box holding the grid is as far out of reach as
        // one past the end of the grid.
        FloatRect window = visibleRectInForm();
        window.inflate(m_descriptor.scaler().scaled48 + lookAhead);
        traverseRows(window, [&](RowBase& row, FloatPoint rowOrigin){
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
            const RowStop stop = row.navigationStop();
            if (stop.control && stop.control->visible())
            {
                if (stop.control == sourceStop)
                    result = stops.size();
                stops.push_back({ .control = stop.control, .rect = stop.rect, .lane = stop.rect });
            }
        });
        return result;
    }

    std::size_t GridBase::searchField(const FieldStops& stops, const OrientedRect& source,
        const OrientedRect& line, KeyCode key, std::size_t sourceIndex, FieldReach reach) const
    {
        std::size_t result = k_maxSize;
        Score bestScore;
        for (std::size_t i = 0; i != stops.size(); ++i)
        {
            if (i == sourceIndex)
                continue;
            const OrientedRect candidate = OrientedRect::orient(stops[i].rect, key);
            Score score;
            score.primary = candidate.primary.start - source.primary.end;
            if (score.primary < 0.0f)
                continue;
            // Reversing the sign leaves the lane test and the tie-break where they are, and
            // turns the nearest stop ahead into the furthest one.
            if (reach == FieldReach::Furthest)
                score.primary = -score.primary;
            const OrientedRect lane = OrientedRect::orient(stops[i].lane, key);
            score.secondary = lane.secondary.distanceTo(line.secondary.start);
            score.inLane = score.secondary == 0.0f;
            if (score.isBetterThan(bestScore))
            {
                bestScore = score;
                result = i;
            }
        }
        return result;
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

    void GridBase::scrollPageIntoView(const FloatRect& pageInForm)
    {
        // A rect as tall as the viewport can only be shown by putting its leading edge against
        // the viewport's, which is what makes the page the press asked for the page displayed.
        FloatRect pageInGrid = pageInForm;
        pageInGrid.offset(-boundsInForm().topLeft());
        // Asked for by hand rather than through scrollChildIntoView, which a grid scrolling
        // itself does not pass through. The page is a strip shorter than the view for the same
        // reason - see moveCellByPage - so the two together still name a viewport's worth, and
        // the leading edge the scroll puts against the view's own is the header's bottom.
        pageInGrid.top -= heldHeaderStrip();
        scrollIntoView(pageInGrid);
    }

    std::size_t GridBase::lastWholeInBand(const FieldStops& stops, const FloatRect& within,
        const OrientedRect& line, KeyCode key) const
    {
        const AxisSpan withinSpan = { within.top, within.bottom };
        std::size_t result = k_maxSize;
        float bestReach = -k_maxFloat;
        for (std::size_t i = 0; i != stops.size(); ++i)
        {
            const FloatRect& rect = stops[i].rect;
            if (!withinSpan.covers({ rect.top, rect.bottom }))
                continue;
            const OrientedRect lane = OrientedRect::orient(stops[i].lane, key);
            if (lane.secondary.distanceTo(line.secondary.start) != 0.0f)
                continue;
            const OrientedRect candidate = OrientedRect::orient(rect, key);
            if (candidate.primary.end <= bestReach)
                continue;
            bestReach = candidate.primary.end;
            result = i;
        }
        return result;
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
            // A header at rest is the grid's first row, and an edge key reaches it. A held one
            // stands over the view instead, at a place no scroll brings the rest of the grid to,
            // so the edge of the grid is the first row under it.
            if (&row == m_header && isHeaderHeld())
                continue;
            RowBase* span = row.spanRow();
            GridBase* nested = row.subGrid();
            // A row comes before the span standing beside the rows it spans, and both come
            // before those rows. Read the other way round when the search runs from the end.
            const bool spanCounts = span && span->visible() && rowPaintsCells(*span);
            // The rows of a collapsed expander are hidden, so the end of the grid is the last
            // one still shown.
            const bool nestedCounts = nested && nested->visible();
            if (toBegin)
            {
                if (rowPaintsCells(row))
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
                if (rowPaintsCells(row))
                    return &row;
            }
        }
        return nullptr;
    }

    bool GridBase::rowPaintsCells(RowBase& row)
    {
        bool result = false;
        row.traverseCells([&](const RowCell&){
            result = true;
        });
        return result;
    }

    bool GridBase::isHeaderHeld() const
    {
        return m_header && m_header->isHeldInView();
    }

    void GridBase::selectedRowChanged(Control* from, Control* to)
    {
        // BOTH PATHS, WHOLE. A row's state belongs to the path it stands on - every row that
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

}
