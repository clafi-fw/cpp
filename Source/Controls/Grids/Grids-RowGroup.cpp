module ClaFi.Controls.Grids;

import :RowGroupBase;
import :Columns;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Icons.ExpanderMark;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Foundation;

namespace ClaFi::Controls::Grids
{
    // RowGroupSpan

    void RowGroupSpan::setExpanded(bool value)
    {
        if (!m_mark || m_expanded == value)
            return;
        m_expanded = value;
        const float newValue = value ? 1.0f : 0.0f;
        animate(AnimationSlots::expander, m_expandedFactor, newValue,
            [this](AnimateParams& params) {
                m_expandedFactor = params.value;
                m_mark->invalidate();
            }
        );
        ToggleExpandedEvent event{ m_expanded };
        emitEvent(event);
        // Opening is what makes the group taller, and the group is its span and its rows together.
        if (m_expanded)
            parent()->scrollIntoViewOnAlign();
    }

    void RowGroupSpan::hitTest(HitTestEvent& event) const
    {
        bool own = false;
        traverseCells([&](const RowCell& cell){
            if (cell.rect.contains(event.point))
                own = true;
        });
        if (!own)
            event.zone = HitTest::Transparent;
    }

    bool RowGroupSpan::hasBlank(const Column& column) const
    {
        if (!m_mark || m_expanded)
            return false;
        // A column no row fills has no width to show a blank in, and a column a span above fills
        // has that span's cell stretched over this group.
        if (column.calculatedWidth() == 0.0f)
            return false;
        return !parentAs<RowGroup>().outerSpanFills(column);
    }

    ScaledDimensions RowGroupSpan::cellLead(const Column& column) const
    {
        if (!m_mark || &column != markColumn())
            return {};
        // The cell's padding stands between the mark and what the cell holds, as it stands
        // between the cell's edge and the mark.
        const ScaledDimensions markDimensions = m_mark->dimensions();
        return {
            markDimensions.x + descriptor().scaledCellMetrics().padding.x,
            markDimensions.y,
        };
    }

    void RowGroupSpan::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        if (&event.control != m_mark)
        {
            RowContainer::adjustChildMetrics(event);
            return;
        }
        // The mark stands in a line of text the way a check box's mark does, and at its size.
        event.metrics = event.themeMetrics().checkMark;
    }

    void RowGroupSpan::alignContent(AlignEvent& event, ScaledPosition position,
        ScaledDimensions& dimensions)
    {
        RowContainer::alignContent(event, position, dimensions);
        if (!m_mark)
            return;
        const Column* column = markColumn();
        // A span that fills no cell has nowhere to put the mark, so the mark is given no room.
        if (!column)
        {
            alignControl(m_mark, event, position, {});
            return;
        }
        // Where the column puts a line of text: the cell as the walk reports it, less its lines
        // and its padding.
        FloatRect contentRect{};
        traverseCells(position, [&](const RowCell& cell){
            if (&cell.column == column)
                contentRect = cell.rect;
        });
        const ScaledCellMetrics& cellMetrics = descriptor().scaledCellMetrics();
        contentRect.right -= cellMetrics.border;
        contentRect.bottom -= cellMetrics.border;
        contentRect.inflate(-cellMetrics.padding.toFloat());
        const ScaledDimensions markDimensions = m_mark->dimensions();
        const TextAnchor anchor = { column->verticalTextAnchor(), HorizontalTextAnchor::Left };
        const ScaledPosition markPosition = anchoredOrigin(contentRect, markDimensions, anchor);
        alignControl(m_mark, event, markPosition, markDimensions);
    }

    void RowGroupSpan::keyDown(KeyDownEvent& event)
    {
        const bool opens = event.key == Keys::Right && !m_expanded;
        const bool closes = event.key == Keys::Left && m_expanded;
        // Every other press moves the selection as the grid moves it: Right from an open group's
        // mark steps into its rows, which is where a tree node's Right goes too.
        if (m_mark && (opens || closes)
            && descriptor().selectedRow() == this
            && descriptor().selectedColumn() == markColumn())
        {
            setExpanded(opens);
            event.handled = true;
            return;
        }
        RowContainer::keyDown(event);
    }

    void RowGroupSpan::doubleClick(DoubleClickEvent& event)
    {
        const bool onMark = event.control == m_mark;
        // The span takes a press only on its cells and its blanks, so no cell there is a blank.
        const bool onBlank = event.control == this && !columnAt(event.clickPos());
        if (!onMark && !onBlank)
        {
            RowContainer::doubleClick(event);
            return;
        }
        // On the mark the first press has turned the group already. Passed on, a double click
        // would reach the grid and open an editor over the selected cell.
        toggleExpanded();
        event.stopPropagation();
    }

    void RowGroupSpan::createMark()
    {
        // MouseOnly, as every control a row holds is: the row keeps the focus and takes the keys.
        m_mark = &addOwnControl<ExpanderButton>(Interactivity::MouseOnly);
        m_mark->connectEvent<PaintIconEvent>([this](PaintIconEvent& event) {
            Icons::ExpanderMark::paint(event, m_expandedFactor);
        });
        m_mark->onClick([this](ClickEvent&) {
            toggleExpanded();
        });
    }

    const Column* RowGroupSpan::markColumn() const
    {
        return firstFilledColumn(descriptor().columns());
    }

    const Column* RowGroupSpan::firstFilledColumn(const ColumnCollection& columns) const
    {
        for (const Column* column : columns)
        {
            if (hasContent(*column))
                return column;
            if (const ColumnCollection* subColumns = column->subColumns())
                if (const Column* found = firstFilledColumn(*subColumns))
                    return found;
        }
        return nullptr;
    }

    // RowGroup

    void RowGroup::preCalcColumn(Column& column)
    {
        RowGroupBase::preCalcColumn(column);
        span().preCalcColumn(column);
    }

    ScaledDimensions RowGroup::calculateContent(AlignEvent& event)
    {
        // Closed, the group is its span alone, which is what the base makes of a hidden body.
        if (!span().expanded())
            return RowGroupBase::calculateContent(event);
        // The body alone states the group's size. The span is drawn across its rows rather
        // than above them, and alignContent stretches it to their height, so a cell of the
        // span is as tall as the body whatever that cell holds.
        ScaledDimensions bodyDimensions = Control::calculateColumns(event, body().controls(), false);
        // The span may not have been sectioned yet (no column with content).
        auto& spanHeights = span().calculatedHeight();
        if (spanHeights.empty())
            spanHeights.push_back(bodyDimensions.y);
        else
            spanHeights[0] = bodyDimensions.y;
        return bodyDimensions;
    }

    void RowGroup::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& dimensions)
    {
        // A group overlaps its body rather than sitting above it: the two fill disjoint
        // sets of columns, so the group's own cells merge vertically across the body.
        // The base has just stacked them, so undo the stacking here.
        RowGroupBase::alignContent(event, position, dimensions);
        // A closed group is its span alone, which the stack already is.
        if (!span().expanded())
            return;
        // Read before setControlHeight() below overwrites it.
        const float stackedSpanHeight = span().height();
        setControlTop(body(), span().top());
        setControlHeight(span(), body().height());
        dimensions.y -= stackedSpanHeight;
    }

    void RowGroup::connectFolding()
    {
        span().connectEvent<ToggleExpandedEvent>([this](ToggleExpandedEvent& event) {
            m_controls[k_body]->setVisible(event.expanded());
        });
        m_controls[k_body]->setVisible(span().expanded());
    }

    bool RowGroup::outerSpanFills(const Column& column) const
    {
        // Every grid between this group and the root is the body of the row holding it, and of
        // those rows only a group answers spanRow(). The walk reads them and changes nothing.
        Control* grid = parent();
        while (grid != &descriptor().owner())
        {
            RowBase& holder = static_cast<RowBase&>(*grid->parent());
            if (RowBase* outerSpan = holder.spanRow())
            {
                // Its cell may stand in a column above this one, spanning that column's leaves.
                for (const Column* reached = &column; reached; reached = reached->parent())
                {
                    if (outerSpan->hasContent(*reached))
                        return true;
                }
            }
            grid = holder.parent();
        }
        return false;
    }

}
