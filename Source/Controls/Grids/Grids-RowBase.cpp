module;
#include "../Y-Core/System/Switches.h"

module ClaFi.Controls.Grids;

import :Columns;
import :RowBase;

import ClaFi.Controls.StackView;

import ClaFi.Diagnostic.Log;

import ClaFi.Core.System.Utils;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.Fit;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;

import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    namespace
    {
        // A moving cell's text, shaped into a layout that lives for the one call and is never
        // put in the cache - see MovingText. Wrapped, as the engine's own drawText shapes a
        // cell; nothing here is editable. The text has to outlive the layout, which names it.
        void shapeAlone(TextLayout& layout, const FormContext& formContext, const Text& text,
            const TextFormat& format, MaxSize bounds, EventPhase phase)
        {
            layout.setEventPhase(phase);
            layout.setFormat(&format);
            layout.setText(text);
            layout.setWrap(true);
            layout.setBoundsAndScale(bounds, formContext.scaleFactor());
        }

        // Whether a link is written anywhere in the text, so that a cell without one costs the
        // pointer no layout lookup.
        [[nodiscard]] bool hasLink(const Text& text)
        {
            for (const auto& marker : text.markers())
            {
                if (std::holds_alternative<PushLink>(marker.second))
                    return true;
            }
            return false;
        }

        // Scratch buffer for the per-row cell flags, kept here rather than in RowBase so that
        // a grid holding a few thousand rows carries one vector rather than one per row.
        CellFlagsList g_cellFlags;
        bool g_cellFlagsInUse{};

        // Lends the scratch to the outermost cell walk. A walk started from inside another
        // one - a visitor that hit-tests, a nested grid painting a row within a row - takes a
        // buffer of its own, so it cannot overwrite the flags the outer walk is still stepping
        // through. Nesting is the rare case and pays for itself.
        class CellFlagsScope
        {
        public:
            CellFlagsScope()
                :
                m_isOuter{ !g_cellFlagsInUse }
            {
                g_cellFlagsInUse = true;
            }
            ~CellFlagsScope()
            {
                if (m_isOuter)
                    g_cellFlagsInUse = false;
            }
            [[nodiscard]] CellFlagsList& list() { return m_isOuter ? g_cellFlags : m_nestedFlags; }
        private:
            bool m_isOuter;
            CellFlagsList m_nestedFlags;
        };
    }

    // RowBase

    RowBase::~RowBase()
    {
        // Highlights outlive the cursor by up to a full fade, so a row can die mid-fade.
        m_descriptor.forgetRow(this);
    }

    ScaledDimensions RowBase::calculateCellContent(ScaledCellMetrics& cellMetrics, const Column& column, float contentBoundsW)
    {
        Text cellText{};
        doGetCellText(column, cellText);
        const TextFormat& format = cellTextFormat(column);
        MaxSize maxSize = { static_cast<float>(contentBoundsW), k_maxFloat };
        if (column.movingText() == MovingText::Yes)
        {
            TextLayout layout;
            shapeAlone(layout, cellMetrics.formContext(), cellText, format, maxSize,
                EventPhase::Calculate);
            return layout.calculatedDimensions();
        }
        ScaledDimensions result = textEngine().calculateText(cellMetrics.formContext(), cellText,
            maxSize, false, true, &format);
        return result;
    }

    ScaledDimensions RowBase::calculateCell(ScaledCellMetrics& cellMetrics, const Column& column, float boundsW)
    {
        ScaledPadding fullPadding = cellMetrics.padding * 2.0f;
        fullPadding += cellMetrics.border;
        // The lead is room the content starts after, so the cell grows by it as it does by padding.
        const ScaledDimensions lead = cellLead(column);
        fullPadding.x += lead.x;
        if (boundsW <= fullPadding.x)
            return fullPadding;
        if (boundsW != k_maxFloat)
            boundsW -= fullPadding.x;
        ScaledDimensions contentDimensions = calculateCellContent(cellMetrics, column, boundsW);
        contentDimensions.y = std::max(contentDimensions.y, lead.y);
        return contentDimensions + fullPadding;
    }

    ScaledDimensions RowBase::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result;

        // calculate width
        result.x = m_descriptor.rootColumn().finalWidth() - event.padding.x * 2.0f; // ***

        // calculate height
        m_calculatedHeight.clear();

        calculateSectionHeight(m_descriptor.rootColumn(), 0);

        result.y = 0;
        for (float h : m_calculatedHeight)
            result.y += h;

        //if (result.y)
        //{
        //    result.y -= event.padding.y * 2; // ***
        //    if (event.padding.y)
        //        if (bool b = scaledPadding().y) debugBeep();
        //}
        // *** The painting routine only draws the cells borders correctly if the *row* paddings are zero.
        // So instead of that we just set them to 0 in the RowBase constructor.

        return result;
    }

    bool RowBase::hasContent(const Column& column) const
    {
        if (&column == &m_descriptor.m_rootColumn)
        {
            return false;
        }
        CellContentEvent event{ *this, column };
        emitEvent(event);
        if (!event.propagationStopped())
        {
            m_descriptor.m_owner.hasCellContent(event);
        }
        return event.hasContent;
    }

    void RowBase::traverseCells(FloatPoint origin, const CellVisitor& visitor) const
    {
        // One hasContent() call per column. Everything below reads the result positionally
        // instead of asking the row again.
        CellFlagsScope cellFlags;
        cellFlags.list().clear();
        buildCellFlags(m_descriptor.rootColumn(), false, cellFlags.list());
        traverseCell(m_descriptor.rootColumn(), origin, 0, 0, true, cellFlags.list(), visitor);
    }

    void RowBase::traverseLanes(FloatPoint origin, const LaneVisitor& visitor) const
    {
        std::vector<RowCell> cells;
        std::vector<FloatRect> lanes;
        std::vector<FloatRect> blanks;
        traverseCells(origin, [&](const RowCell& cell){
            if (cell.blank)
            {
                blanks.push_back(cell.rect);
                return;
            }
            cells.push_back(cell);
            lanes.push_back(cell.rect);
        });
        if (cells.empty())
            return;
        // A blank still closes its row, so the cell nearest to it answers for it.
        for (const FloatRect& blank : blanks)
        {
            const float blankCenter = blank.centerX();
            std::size_t nearest = 0;
            float nearestDistance = k_maxFloat;
            for (std::size_t i = 0; i != cells.size(); ++i)
            {
                const AxisSpan cellSpan = { cells[i].rect.left, cells[i].rect.right };
                const float distance = cellSpan.distanceTo(blankCenter);
                if (distance < nearestDistance)
                {
                    nearestDistance = distance;
                    nearest = i;
                }
            }
            FloatRect& lane = lanes[nearest];
            lane.left = std::min(lane.left, blank.left);
            lane.right = std::max(lane.right, blank.right);
        }
        for (std::size_t i = 0; i != cells.size(); ++i)
            visitor(cells[i], lanes[i]);
    }

    void RowBase::getCellText(const Column& column, Text& text)
    {
        GetCellTextEvent event{ *this, column, text };
        emitEvent(event);
        if (!event.propagationStopped())
        {
            m_descriptor.m_owner.getCellText(event);
        }
    }

    const TextFormat& RowBase::cellTextFormat(const Column& column) const
    {
        return column.cellTextFormat();
    }

    void RowBase::getCellHint(GetCellHintEvent& event)
    {
        emitEvent(event);
        if (!event.propagationStopped())
        {
            m_descriptor.m_owner.getCellHint(event);
        }
    }

    void RowBase::cellLinkClick(CellLinkClickEvent& event)
    {
        emitEvent(event);
        if (!event.propagationStopped())
            m_descriptor.m_owner.cellLinkClick(event);
    }

    void RowBase::cellLinkContextPopup(CellLinkContextPopupEvent& event)
    {
        emitEvent(event);
        if (!event.popup.propagationStopped())
            m_descriptor.m_owner.cellLinkContextPopup(event);
    }

    CursorShape RowBase::cursor() const
    {
        return m_pointsAtLink ? CursorShape::Hand : LaneBase::cursor();
    }

    FloatRect RowBase::contextMenuAnchor() const
    {
        const Column* column = m_descriptor.selectedColumn();
        if (column && m_descriptor.selectedRow() == this)
        {
            const FloatRect cellRect = cellRectInForm(*column);
            if (!cellRect.empty())
                return cellRect;
        }
        return LaneBase::contextMenuAnchor();
    }

    std::wstring RowBase::acceptCellText(const Column& column, const Text& text)
    {
        AcceptCellTextEvent event{ *this, column, text };
        emitEvent(event);
        if (!event.propagationStopped())
        {
            m_descriptor.m_owner.acceptCellText(event);
        }
        return event.reason();
    }

    void RowBase::paintCell(PaintEvent& paintEvent, const FloatRect& cellRect, const Column& column)
    {
        FloatRect textBounds = cellRect.toFloat();
        const ScaledCellMetrics& cellMetrics = descriptor().scaledCellMetrics();
        textBounds.inflate(-cellMetrics.padding.toFloat());
        textBounds.left += cellLead(column).x;
        Text text;
        doGetCellText(column, text);
        const TextFormat& format = cellTextFormat(column);
        const TextAnchor anchor = { column.verticalTextAnchor(), HorizontalTextAnchor::None };
        if (column.movingText() == MovingText::Yes)
        {
            if (text.plainText().empty())
                return;
            TextLayout layout;
            shapeAlone(layout, paintEvent.formContext(), text, format, textBounds.dimensions(),
                EventPhase::Paint);
#if CLAFI_TEXT_MOVING
            layout.draw(
                paintEvent.controlContext(),
                anchoredOrigin(textBounds, layout.calculatedDimensions(), anchor),
                nullptr,
                textRenderMode()
            );
#else
            layout.draw(
                paintEvent.controlContext(),
                anchoredOrigin(textBounds, layout.calculatedDimensions(), anchor)
            );
#endif
            return;
        }
        textEngine().drawText(
            paintEvent.controlContext(),
            textBounds,
            text,
            anchor,
            nullptr,
#if CLAFI_TEXT_MOVING
            textRenderMode(),
#endif
            true,
            &format
        );
    }

    void RowBase::paintOneCell(PaintEvent& event, const RowCell& cell, bool isFirstRow)
    {
        FloatRect cellRect = cell.rect;
        const ScaledCellMetrics& cellLt = m_descriptor.scaledCellMetrics();
        const PaintEvent& gridEvent = m_descriptor.gridPaintEvent();
        RoundedRectangleParts cellFrame;
        cellFrame.bounds = cellRect;
        const CornerRadii gridRadii = GridDescriptor::cornerRadiiOf(gridEvent);

        const float snapThreshhold = gridEvent.radius();
        bool snapToRight = gridEvent.right() - cellRect.right <= snapThreshhold;
        // bottom snapThreshhold MUST be less than a separator size,
        // in case there's one at the very bottom, so it's a < and NOT <=
        bool snapToBottom = gridEvent.bottom() - cellRect.bottom < snapThreshhold;

        bool isfirstVisibleSection = isFirstRow
            // will do for now
            && !cell.sectionIndex;

        // A cell at a corner of the grid turns the grid's corner there.
        cellFrame.radii = {
            cell.isFirstColumn && isfirstVisibleSection ? gridRadii.topLeft : 0.0f,
            snapToRight && isfirstVisibleSection ? gridRadii.topRight : 0.0f,
            snapToRight && snapToBottom ? gridRadii.bottomRight : 0.0f,
            snapToBottom && cell.isFirstColumn ? gridRadii.bottomLeft : 0.0f
        };

        if (snapToRight)
        {
            cellFrame.bounds.right = gridEvent.right();
            cellFrame.bounds.right -= cellLt.border;
        }
        if (snapToBottom)
        {
            cellFrame.bounds.bottom -= cellLt.border;
        }
        // One border tighter than the grid's corner: the cell's stroke lies inside the grid's frame.
        
        cellFrame.radii.topLeft = std::max(0.0f, cellFrame.radii.topLeft - cellLt.border);
        cellFrame.radii.topRight = std::max(0.0f, cellFrame.radii.topRight - cellLt.border);
        cellFrame.radii.bottomRight = std::max(0.0f, cellFrame.radii.bottomRight - cellLt.border);
        cellFrame.radii.bottomLeft = std::max(0.0f, cellFrame.radii.bottomLeft - cellLt.border);

        const bool alwaysDrawLines = alwaysDrawGridLines();
        const bool drawsVertical = (alwaysDrawLines || m_descriptor.drawsVerticalLines()) && !snapToRight;
        const bool drawsHorizontal = (alwaysDrawLines || m_descriptor.drawsHorizontalLines()) && !snapToBottom;
        cellFrame.sides = { drawsHorizontal, drawsVertical, drawsHorizontal, drawsVertical };
        // A ROW PAINTS NO SURFACE OF ITS OWN, SO ITS CELLS ARE IT, and the fill is
        // unconditional. What a cell stands on is the row's surface: what the row inherited from
        // whatever holds it, with the row's own rules over that - GridRow's Selected rule among
        // them.
        //
        // Unconditional because the colour a row inherits is as much a colour it has to paint as
        // one it states itself. A row standing in a group that holds the selection states nothing
        // of its own, and a fill made conditional on the ROW having moved the surface would drop
        // exactly that case.
        const Hsl bgHsl = event.surfaceHsl();
        event.canvas().fillPartialRoundedRectangle(cellFrame, bgHsl.toColor());
        // Each side drawn is half a line, inside the bounds, and the cell across the edge draws
        // the other half. Each half is raised off its own cell's surface, which is what lets a
        // line between two differently coloured cells follow both colours.
        if (drawsVertical || drawsHorizontal)
            event.canvas().drawPartialRoundedRectangle(cellFrame, event.strokeRgbOn(bgHsl),
                cellLt.border / 2.0f);

        // A blank is the surface and the lines alone: nothing is written in it, and nothing
        // lands on it for a highlight to show.
        if (cell.blank)
            return;

        // Per-cell state. Not rowFactor * columnFactor: two independent axes
        // cannot name a cell, and their product lights the corners the cursor
        // never visited just as brightly as the ones it did.
        const float hotFactor = m_descriptor.cellHoveredFactor(*this, cell.column);
        const float selFactor = m_descriptor.cellSelectedFactor(*this, cell.column);

        // The mode is read before the control is looked up: one is a field read, the
        // other a map lookup. A frame around a control would state twice what the
        // control already states itself, which is what Control mode avoids.
        const CellHighlightMode highlightMode = cell.column.cellHighlightMode();
        Control* hostedControl = highlightMode == CellHighlightMode::Control
            ? cellControl(cell.column)
            : nullptr;
        if (hostedControl)
        {
            // Set without invalidating: these factors only move while the grid
            // animates the cell, and every step of that animation invalidates the
            // grid already. The control's paint event samples them when it is built,
            // which happens in paintChildren() - after this surface pass.
            hostedControl->setStateFactor(VisualStateIndex::Hovered, hotFactor, false);
            hostedControl->setStateFactor(VisualStateIndex::Selected, selFactor, false);
            // Focus belongs to the row, selection to the cell, and the control shows
            // it only where the two meet: the focus travels to whichever cell the
            // selection moves to, and leaves the row entirely when the row loses it.
            // The control itself is MouseOnly, so this is the only focus it ever has.
            hostedControl->setStateFactor(VisualStateIndex::Focused,
                                          event.focusedFactor() * selFactor, false);
        }
        // A Control cell with no control in this row - the header cell, or a plain
        // text cell in the same column - has no better cue to defer to, so it lands
        // here with the Grid ones.
        else if (highlightMode != CellHighlightMode::None)
        {
            // highlight inner rect (hovered/selected)
            cellFrame.sides = RectSides::all();
            float hlOpacity = StateFactors::compose(hotFactor * 0.25f, selFactor * 0.5f);
            if (hlOpacity)
            {
                Color borderColor = Color(event.textRgb(InkGrade::Strongest), event.indicatorRgb(), hotFactor);

                // The focus belongs to the row and the selection to the cell, so the ring
                // thickens only where the two meet. A cell the pointer is merely on wears
                // the line's own width, whichever cell in the row holds the selection.
                const float strokeWidth = cellLt.border
                    + event.scaleF(event.focusedFactor() * selFactor);
                cellFrame.bounds.inflate(-event.scaleF(event.pressedFactor()));
                event.canvas().drawPartialRoundedRectangle(cellFrame, borderColor.withOpacity(hlOpacity), strokeWidth);
            }
        }
        // Paint cell content;
        paintCell(event, m_descriptor.cellInnerRect(cellRect), cell.column);
    }

    void RowBase::doPaintColumns(PaintEvent& event)
    {
        // isFirstRow is a property of the row, so the walk carries no trace of it.
        const bool isFirstRow = parent() == &descriptor().owner() && isFirstInParent();
        traverseCells(event.topLeft(), [&](const RowCell& cell){
            paintOneCell(event, cell, isFirstRow);
        });
    }

    // A CELL SAYS WHAT ITS COLUMN IS TOO NARROW TO SHOW, which is the over-text hint
    // Control::nestedGetHint offers for a control's own text - see that one for the shape of the
    // answer. A row draws its cells rather than holding a control per cell, so the question is
    // asked of the row about one of its cells, and every part of the answer is worked out here
    // rather than read off what a paint recorded.
    void RowBase::nestedGetHint(GetHintEvent& event)
    {
        // ONLY THE ROW THE HOVER NAMES ANSWERS. The walk that asks this climbs from the control
        // under the pointer, so a row holding the hovered one is asked in its turn - and the
        // cells it would answer about are not the ones the pointer stands on.
        if (m_descriptor.hoveredRow() != this)
            return;
        const Column* hoveredColumn = m_descriptor.hoveredColumn();
        if (!hoveredColumn)
            return;

        // Where the cell is now, in the space a placement rect is stated in. A column with no
        // cell in this row - one whose sub-columns carry the content - is not the cell under the
        // pointer, which columnAt named off a rect.
        const FloatRect cellRect = cellRectInForm(*hoveredColumn);
        if (cellRect.empty())
            return;

        // THE CELL'S OWN HINT FIRST. Whoever answers for the cell's text may answer for its hint
        // the same way, and the anchor is set ahead so a placement against the cell needs
        // nothing more. Only a cell nobody spoke for goes on to repeat the words its column cut.
        event.anchorRect = cellRect;
        GetCellHintEvent cellEvent{ *this, *hoveredColumn, event };
        getCellHint(cellEvent);
        if (!event.text.empty())
            return;

        // The fit and the placement are both stated over the box the text is drawn in, so the
        // hint stands on the words it repeats and appears exactly when they are cut.
        const FloatRect textBounds = cellTextBounds(*hoveredColumn, cellRect);
        if (textBounds.empty())
            return;

        Text cellText;
        doGetCellText(*hoveredColumn, cellText);
        CalculatedDimensions drawn{};
        const bool trimmed = isCellTextTrimmed(event.formContext(), *hoveredColumn, cellText,
            textBounds.dimensions(), drawn);
        if (!trimmed)
            return;

        // Gathered into the hint's own buffer rather than copied into it: a Text holding an
        // lvalue item list shares that list by reference, and this one's would be the local
        // above. Reached only once a hint is going up, so the second gather costs a hover.
        doGetCellText(*hoveredColumn, event.text);
        event.format = cellTextFormat(*hoveredColumn);
        // THE HINT STANDS ON THE GLYPHS, NOT ON THE BOX THEY WERE GIVEN. The column's vertical
        // anchor moves the block inside that box - paintCell states the same anchor - and the
        // placement lands the hint's own first glyph on this rect's top left. The width is the
        // box's, which is the width the lines were broken at.
        const TextAnchor anchor = {
            hoveredColumn->verticalTextAnchor(),
            HorizontalTextAnchor::None,
        };
        event.anchorRect = FloatRect::fromDimensions(
            anchoredOrigin(textBounds, drawn, anchor),
            { textBounds.width(), drawn.y }
        );
        event.placement = FormPlacement::OverText;
        // A cell's text is broken to its box - see shapeAlone.
        event.wordWrap = true;
    }

    void RowBase::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        // WHAT A GRID ROW IS SELECTED BY IS THE CELL SELECTION - the row holding the selected
        // cell, and every row that row is nested in, a group row included - and the grid's own
        // selection where it holds several rows. Never the container's current item.
        //
        // THE WALK ENDS HERE, and it has to. `Control::state()` runs leaf to root and the last
        // writer wins; a grid is an ActiveContainer, so `StackBase::getControlState` above
        // answers for its items with `m_currentItem == item` and would replace this. That answer
        // is true of the one row the user last landed on and false of every row that merely holds
        // the cell inside it, which is exactly the set this rule exists to name.
        //
        // Written before the base runs, so a handler on the row still has the last word.
        event.state.selected = containsNested(m_descriptor.selectedRow())
            or m_descriptor.owner().selection().contains(this);
        LaneBase::getControlState(event);
        event.stopPropagation();
    }

    void RowBase::adjustPaint(AdjustPaintEvent& event)
    {
        // Groups and sections wear it too. See Grids#gridrow
        event.setColorRules(UiElement::GridRow);
    }

    void RowBase::mouseMove(MouseMoveEvent& event)
    {
        Column* columnToHover = columnAt(event.posOnControl);
        setHoveredColumn(columnToHover);
        m_pointsAtLink = columnToHover && linkAt(*columnToHover, event.posOnForm).has_value();
    }

    void RowBase::hoverLeave()
    {
        LaneBase::hoverLeave();
        m_pointsAtLink = false;
        // A move names the cell it lands on, so the hover follows the pointer for as long as
        // the pointer stays on the grid. Leaving it is the one way out that no move reports,
        // and the cell has to be given up here or it stays lit with the pointer elsewhere.
        //
        // The leave walks up the parent chain, so a row that still holds the pointer is told
        // its child was left. Only the row the hover names may clear it.
        if (m_descriptor.hoveredRow() == this)
            m_descriptor.setHoveredCell(nullptr, nullptr, false);
    }

    void RowBase::nestedControlFocusing(FocusEvent& event)
    {
        m_descriptor.beginCellSelection();
        // Recorded by the row the focus lands on, before the selection moves under the press.
        if (event.control == this)
            recordPress(event.modifiers);
        selectColumnUnderMouse();
        // Only when the row itself is the control being focused. The event passes through every
        // row on the way up, and a row holding controls in its cells must not answer for the
        // one of them the focus actually landed on.
        if (event.control == this)
            selectCellOnKeyboardEntry();
        // The control standing in for this row has taken the focus - clicked, or reached by the
        // focus navigator rather than by a key the grid answered. The grid holds no cell while
        // the focus rests there, and recording that is what lets the next key move on from it
        // the same way it would had a key put the focus there.
        else if (const RowStop stop = navigationStop(); stop.control && event.control == stop.control)
            m_descriptor.setSelectedCell(stop.control, nullptr);
        Control::nestedControlFocusing(event);
        m_descriptor.endCellSelection();
    }

    void RowBase::nestedPressDown(PressDownEvent& event)
    {
        m_descriptor.beginCellSelection();
        selectColumnUnderMouse();
        Control::nestedPressDown(event);
        m_descriptor.endCellSelection();
    }

    // Only a pointer click lands on a link - one a key made has no point to land on. The press
    // has already picked the cell, and the click is the link's alone.
    void RowBase::nestedClick(ClickEvent& event)
    {
        if (event.control == this && Input::mouse().active())
        {
            const PointInForm clickPos = event.clickPos();
            if (const Column* column = columnAt(clickPos))
            {
                if (std::optional<std::wstring> target = linkAt(*column, clickPos))
                {
                    event.stopPropagation();
                    CellLinkClickEvent linkEvent{ *this, *column, std::move(*target),
                        event.stamp };
                    cellLinkClick(linkEvent);
                    return;
                }
            }
        }
        LaneBase::nestedClick(event);
    }

    // A LINK IS WHAT THE MENU IS ABOUT where one stands under the pointer or, from the keyboard,
    // in the selected cell. Only the row the request was raised on answers - it passes through
    // every row on the way up. A handler that has shown the link's menu stops the request; one
    // that has not leaves it to go on up as from anywhere else on the row.
    void RowBase::nestedContextPopup(ContextPopupEvent& event)
    {
        if (event.control == this)
        {
            if (std::optional<MenuLink> link = menuLink(event))
            {
                CellLinkContextPopupEvent linkEvent{ *this, *link->column, std::move(link->target),
                    event };
                cellLinkContextPopup(linkEvent);
                if (event.propagationStopped())
                    return;
            }
        }
        LaneBase::nestedContextPopup(event);
    }

    Column* RowBase::columnAt(PointInForm mousePosition) const
    {
        return columnAt(mousePosition, boundsInForm().topLeft());
    }

    Column* RowBase::columnAt(PointInControl mousePosition) const
    {
        return columnAt(mousePosition, { 0, 0 });
    }

    void RowBase::selectColumnUnderMouse()
    {
        if (!Input::mouse().active())
            return;
        const PointInForm mousePosition = form().mouseDownPos();
        if (const Column* columnToSelect = laneColumnAt(mousePosition))
        {
            selectColumn(*columnToSelect);
            // The click names both lines a later move keeps, the same as the keys do.
            const FloatPoint gridOrigin = m_descriptor.owner().boundsInForm().topLeft();
            m_descriptor.setDesiredCellX(mousePosition.x - gridOrigin.x);
            m_descriptor.setDesiredCellY(mousePosition.y - gridOrigin.y);
        }
    }

    CellPressKind RowBase::pressOn(const Column& column) const
    {
        // A key reaches a cell's control only while its cell is the selected one.
        if (!Input::mouse().active())
            return CellPressKind::Act;
        const CellPress& press = m_descriptor.lastPress();
        // A press recorded on some other cell did not move the selection onto this one.
        if (press.row != this || press.column != &column)
            return CellPressKind::Act;
        return press.kind;
    }

    void RowBase::selectCellOnKeyboardEntry()
    {
        if (Input::mouse().active())
            return;
        // A cell of this row already selected is where the user left it, and the grid returning
        // to this row returns to that cell.
        if (m_descriptor.selectedRow() == this && m_descriptor.selectedColumn())
            return;
        FloatRect cellRect{};
        if (Column* columnToSelect = cellAtEntry(cellRect))
        {
            selectColumn(*columnToSelect);
            scrollCellIntoView(cellRect);
        }
    }

    void RowBase::selectColumn(const Column& value)
    {
        m_descriptor.setSelectedCell(this, &value);
        invalidate();
    }

    void RowBase::scrollCellIntoView(const FloatRect& cellRect)
    {
        // The cell, not the row: a row wider than the viewport counts as visible the moment any
        // part of it is, so scrolling the row leaves a cell at the far end off screen.
        scrollIntoView(cellRect);
    }

    void RowBase::setHoveredColumn(const Column* value, bool initiateHint)
    {
        // The row half of the cell identity. This is the only place both halves are
        // known, which is why hover routes through the row rather than the descriptor.
        descriptor().setHoveredCell(this, value, initiateHint);
    }

    void RowBase::preCalcColumn(Column& column)
    {
        if (!hasContent(column))
            return;
        ScaledCellMetrics& cellMetrics = m_descriptor.scaledCellMetrics();
        float w = calculateCell(cellMetrics, column, k_maxFloat).x;
        column.addNeededWidth(w);
    }

    Column* RowBase::columnAt(ScaledPosition mousePosition, ScaledPosition topLeft) const
    {
        const FloatPoint point = {
            mousePosition.x - topLeft.x,
            mousePosition.y - topLeft.y,
        };
        Column* result = nullptr;
        traverseCells([&](const RowCell& cell){
            if (!result && !cell.blank && cell.rect.contains(point))
                result = &cell.column;
        });
        return result;
    }

    const Column* RowBase::laneColumnAt(const PointInForm position) const
    {
        const Column* result = nullptr;
        traverseLanes(boundsInForm().topLeft(), [&](const RowCell& cell, const FloatRect& lane){
            if (!result && lane.contains(position))
                result = &cell.column;
        });
        return result;
    }

    // The user is on a cell while it is the selected one and, in a view holding a selection,
    // while its row is held as well - StackBase reads a press on anything else as a pick.
    void RowBase::recordPress(const KeyModifiers modifiers)
    {
        if (!Input::mouse().active())
            return;
        const Column* column = laneColumnAt(form().mouseDownPos());
        Grid& grid = m_descriptor.owner();
        const bool multiSelect = grid.selectionMode() == SelectionMode::Multi;
        if (multiSelect && (modifiers.ctrl || modifiers.shift))
        {
            m_descriptor.recordPress(*this, column, CellPressKind::Select);
            return;
        }
        const bool onSelectedCell = column
            && m_descriptor.selectedRow() == this
            && m_descriptor.selectedColumn() == column;
        const bool rowHeld = !multiSelect || grid.selection().contains(this);
        const CellPressKind kind = onSelectedCell && rowHeld
            ? CellPressKind::Act
            : CellPressKind::Pick;
        m_descriptor.recordPress(*this, column, kind);
    }

    void RowBase::traverseCell(Column& column, FloatPoint position, std::size_t sectionIndex,
        std::size_t flagsIndex, bool isFirstColumn, const CellFlagsList& cellFlags,
        const CellVisitor& visitor) const
    {
        // buildCellFlags filled one slot per column of the same tree in the same pre-order,
        // so every index this descent forms addresses the column it is standing on.
        const bool hasColContent = cellFlags[flagsIndex].content;
        const bool contentBelow = cellFlags[flagsIndex].contentBelow;

        float h = 0.0f;
        if (sectionIndex < m_calculatedHeight.size())
            h = m_calculatedHeight.at(sectionIndex);

        // A column whose entire subtree is empty for this row merges downwards over the
        // remaining sections, exactly like a leaf. Its sub-columns tile its x-range, so the
        // merged region stays a rectangle - no L-shaped cells.
        if (column.hasSubColumns() && contentBelow)
        {
            FloatPoint subPosition = position;
            std::size_t subSectionIndex = sectionIndex;
            if (hasColContent)
            {
                subPosition.y += h;
                ++subSectionIndex;
            }
            // pre-order: the first sub-column sits right after its parent
            std::size_t subFlagsIndex = flagsIndex + 1;
            bool isFirstSubColumn = isFirstColumn;
            for (Column* subColumn : *column.subColumns())
            {
                traverseCell(*subColumn, subPosition, subSectionIndex, subFlagsIndex,
                    isFirstSubColumn, cellFlags, visitor);
                subPosition.x += subColumn->calculatedWidth();
                // Step over this column's subtree to land on the next sibling's slot.
                subFlagsIndex = cellFlags[subFlagsIndex].subtreeEnd;
                isFirstSubColumn = false;
            }
        }
        else
            for (std::size_t level = sectionIndex + 1; level < m_calculatedHeight.size(); ++level)
                h += m_calculatedHeight.at(level);

        if (hasColContent)
            visitor({
                .column{ column },
                .rect{ position.x, position.y, position.x + column.calculatedWidth(), position.y + h },
                .sectionIndex = sectionIndex,
                .isFirstColumn = isFirstColumn,
                .blank = cellFlags[flagsIndex].blank,
            });
    }

    Column* RowBase::cellAtEntry(FloatRect& cellRect) const
    {
        const float desiredCellX = m_descriptor.desiredCellX();
        // With no band to return to the row is entered at its leading edge.
        const float bandX = desiredCellX == k_maxFloat ? 0.0f : rowLocalX(desiredCellX);
        const bool fromTop = m_descriptor.cellEntryEdge() == ScrollDirection::ToBegin;

        Column* result = nullptr;
        Score bestScore;
        traverseCells([&](const RowCell& cell){
            if (cell.blank)
                return;
            // x decides and the band breaks the tie, not the other way round: a band may hold
            // no cell at that x at all, and a cell in the next one down is still the cell the
            // move was aiming at.
            Score score;
            score.primary = AxisSpan{ cell.rect.left, cell.rect.right }.distanceTo(bandX);
            score.secondary = fromTop ? cell.rect.top : -cell.rect.bottom;
            if (score.isBetterThan(bestScore))
            {
                bestScore = score;
                result = &cell.column;
                cellRect = cell.rect;
            }
        });
        return result;
    }

    float RowBase::rowLocalX(float gridX) const
    {
        return gridX + m_descriptor.owner().boundsInForm().left - boundsInForm().left;
    }

    void RowBase::calculateSectionHeight(const Column& column, std::size_t sectionIndex)
    {
        ScaledCellMetrics& cellLt = m_descriptor.scaledCellMetrics();

        auto setSectionHeight = [this](std::size_t sectionIndex, float currentH) {
            while (m_calculatedHeight.size() <= sectionIndex)
                m_calculatedHeight.push_back(0);
            m_calculatedHeight[sectionIndex] = std::max(currentH, m_calculatedHeight[sectionIndex]);
        };

        bool hasColContent = hasContent(column);
        if (hasColContent)
        {
            float cellW = column.calculatedWidth();
            float cellH = calculateCell(cellLt, column, cellW).y;
            setSectionHeight(sectionIndex, cellH);
            ++sectionIndex;
        }

        if (column.subColumns())
            for (const Column* subColumn : *column.subColumns())
                calculateSectionHeight(*subColumn, sectionIndex);
    }

    bool RowBase::buildCellFlags(const Column& column, bool covered, CellFlagsList& list) const
    {
        const std::size_t index = list.size();
        list.push_back({});
        // The one and only hasContent() call for this column in this pass.
        const bool content = hasContent(column);
        bool contentBelow = false;
        if (ColumnCollection* subColumns = column.subColumns())
            for (const Column* subColumn : *subColumns)
            {
                const bool subTreeHasContent = buildCellFlags(*subColumn, covered || content, list);
                contentBelow = contentBelow || subTreeHasContent;
            }
        // A blank stands in a leaf, and only where no cell of this row covers it already.
        const bool blank = !content && !covered && !column.hasSubColumns() && hasBlank(column);
        // list may have reallocated during the recursion, so re-address by index.
        CellFlags& flags = list[index];
        flags.content = content || blank;
        flags.blank = blank;
        flags.contentBelow = contentBelow;
        flags.subtreeEnd = list.size();
        return flags.content || contentBelow;
    }

    void RowBase::doGetCellText(const Column& column, Text& text)
    {
        text.clear();
        getCellText(column, text);
    }

    bool RowBase::isCellTextTrimmed(const FormContext& formContext, const Column& column,
        const Text& text, MaxSize bounds, CalculatedDimensions& drawn)
    {
        const TextFormat& format = cellTextFormat(column);
        if (column.movingText() == MovingText::No)
        {
            drawn = textEngine().calculateText(formContext, text, bounds, false, true, &format);
            return textEngine().isTextTrimmed(formContext, text, bounds, true, &format);
        }

        // A moving cell's layout is never put in the cache, so the block is read off the one
        // shaped here. Asking the engine to measure it would file a text that changes every
        // frame - see shapeAlone and MovingText.
        TextLayout layout;
        shapeAlone(layout, formContext, text, format, bounds, EventPhase::Paint);
        drawn = layout.calculatedDimensions();
        return layout.isTrimmed();
    }

    // The origin is the row's own place in the form, which is the space the paint walks the
    // cells in.
    FloatRect RowBase::cellRectInForm(const Column& column) const
    {
        FloatRect cellRect;
        traverseCells(boundsInForm().topLeft(), [&](const RowCell& cell) {
            if (&cell.column == &column)
                cellRect = cell.rect;
        });
        return cellRect;
    }

    // The cell inside its lines, then the padding and the lead.
    FloatRect RowBase::cellTextBounds(const Column& column, const FloatRect& cellRect) const
    {
        const ScaledCellMetrics& cellMetrics = m_descriptor.scaledCellMetrics();
        FloatRect textBounds = m_descriptor.cellInnerRect(cellRect);
        textBounds.inflate(-cellMetrics.padding.toFloat());
        textBounds.left += cellLead(column).x;
        return textBounds;
    }

    // Asked the way the cell is drawn - a moving column's cell off a layout of its own, any
    // other through the cache - and the point measured from where the column's vertical anchor
    // put the block, as paintCell puts it.
    std::optional<std::wstring> RowBase::linkAt(const Column& column, const PointInForm point)
    {
        const FloatRect cellRect = cellRectInForm(column);
        if (cellRect.empty())
            return std::nullopt;
        const FloatRect textBounds = cellTextBounds(column, cellRect);
        if (textBounds.empty())
            return std::nullopt;
        Text cellText;
        doGetCellText(column, cellText);
        if (!hasLink(cellText))
            return std::nullopt;

        const FormContext& formContext = this->formContext();
        const TextFormat& format = cellTextFormat(column);
        const MaxSize bounds = textBounds.dimensions();
        const TextAnchor anchor = { column.verticalTextAnchor(), HorizontalTextAnchor::None };
        std::optional<LinkHit> hit;
        if (column.movingText() == MovingText::No)
        {
            const CalculatedDimensions drawn = textEngine().calculateText(formContext, cellText,
                bounds, false, true, &format);
            const FloatPoint origin = anchoredOrigin(textBounds, drawn, anchor);
            hit = textEngine().linkAt(formContext, cellText, bounds, point - origin, true,
                &format);
        }
        else
        {
            TextLayout layout;
            shapeAlone(layout, formContext, cellText, format, bounds, EventPhase::Paint);
            const FloatPoint origin = anchoredOrigin(textBounds, layout.calculatedDimensions(),
                anchor);
            hit = layout.linkAt(point - origin);
        }
        if (!hit.has_value())
            return std::nullopt;
        return std::wstring{ hit->link.value };
    }

    // The keyboard names a cell and no point in it, so the cell's first link stands for the cell.
    std::optional<RowBase::MenuLink> RowBase::menuLink(const ContextPopupEvent& event)
    {
        if (event.mousePos)
        {
            const PointInForm point = *event.mousePos;
            const Column* column = columnAt(point);
            if (!column)
                return std::nullopt;
            std::optional<std::wstring> target = linkAt(*column, point);
            if (!target.has_value())
                return std::nullopt;
            return MenuLink{ column, std::move(*target) };
        }
        const Column* column = m_descriptor.selectedColumn();
        if (!column || m_descriptor.selectedRow() != this)
            return std::nullopt;
        Text cellText;
        doGetCellText(*column, cellText);
        for (const Text::Marker& marker : cellText.markers())
        {
            if (const PushLink* link = std::get_if<PushLink>(&marker.second))
                return MenuLink{ column, link->target };
        }
        return std::nullopt;
    }

}
