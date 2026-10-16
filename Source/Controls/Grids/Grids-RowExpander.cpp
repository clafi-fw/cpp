module ClaFi.Controls.Grids;

import :RowExpander;
import :RowGroupBase;
import :Columns;
import :Descriptor;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // RowExpanderHeader

    // The overlay a held header is painted in measures its corners against the view, which knows
    // nothing of the grid's, so the grid's corners are turned here. A corner the view cuts off
    // keeps the square it has, and one a held header is carried away from keeps the view's.
    void RowExpanderHeader::adjustPaint(AdjustPaintEvent& event)
    {
        ExpanderHeader::adjustPaint(event);
        const GridDescriptor& descriptor = parentAs<RowExpander>().descriptor();
        const CornerRadii gridRadii = GridDescriptor::cornerRadiiOf(descriptor.gridPaintEvent());
        // One border tighter than the grid's corner: a cell's stroke lies inside the grid's frame.
        const float border = descriptor.scaledCellMetrics().border;

        const PaintEvent& own = event.paintEvent();
        const FloatRect bounds = own.controlBounds();
        const FloatRect visible = own.viewport();
        using CornerPoints = std::array<FloatPoint, k_cornersNum>;
        const CornerPoints ownCorners = {
            bounds.topLeft(),
            bounds.topRight(),
            bounds.bottomRight(),
            bounds.bottomLeft(),
        };
        const CornerPoints shownCorners = {
            visible.topLeft(),
            visible.topRight(),
            visible.bottomRight(),
            visible.bottomLeft(),
        };
        const RectSides onFrame = sidesOnGridFrame(own);
        const std::array<bool, k_cornersNum> atGridCorner = {
            onFrame.top && onFrame.left,
            onFrame.top && onFrame.right,
            onFrame.bottom && onFrame.right,
            onFrame.bottom && onFrame.left,
        };

        CornerRadii radii = event.cornerRadii();
        for (std::size_t i = 0; i < k_cornersNum; ++i)
        {
            if (atGridCorner[i] && shownCorners[i] == ownCorners[i])
                radii.set(i, std::max(radii.get(i), gridRadii.get(i) - border));
        }
        event.setCornerRadii(radii);
    }

    void RowExpanderHeader::paintSurface(PaintEvent& event)
    {
        ExpanderHeader::paintSurface(event);
        GridDescriptor& descriptor = parentAs<RowExpander>().descriptor();
        // A side on the grid's frame is the frame's to draw, as it is a cell's.
        if (!descriptor.drawsHorizontalLines() || sidesOnGridFrame(event).bottom)
            return;
        FloatRect bottomBorderRect = event.controlBounds();
        bottomBorderRect.top = bottomBorderRect.bottom - descriptor.scaledBorderWidth()
            - event.scale(1.0f - expandedFactor());
        // Raised off the surface rather than read from strokeRgb(), which carries the focus ring.
        event.canvas().fillRectangle(bottomBorderRect, event.strokeRgbOn(event.surfaceHsl()));
    }

    // Against the grid's bounds, with the grid's radius for the reach a cell ending a row snaps
    // by - see RowBase::paintOneCell. A held header carried down from the grid's top stands on no
    // top side.
    RectSides RowExpanderHeader::sidesOnGridFrame(const PaintEvent& event) const
    {
        const PaintEvent& gridEvent = parentAs<RowExpander>().descriptor().gridPaintEvent();
        const FloatRect grid = gridEvent.controlBounds();
        const FloatRect bounds = event.controlBounds();
        const float reach = gridEvent.radius();
        return {
            .top = bounds.top - grid.top <= reach,
            .right = grid.right - bounds.right <= reach,
            .bottom = grid.bottom - bounds.bottom < reach,
            .left = bounds.left - grid.left <= reach
        };
    }

    // RowExpander

    void RowExpander::getControlState(GetStateEvent& event) const
    {
        // The base is what ends the walk, so StackBase above still does not get to answer
        // with its current item. Only the selection is declined.
        RowGroupBase::getControlState(event);
        if (&event.control == this)
            event.state.selected = false;
    }

    ChildSearch RowExpander::childSearch(const Control& child) const
    {
        if (&child == m_controls[k_body].get())
            return ChildSearch::Revealed;
        return RowGroupBase::childSearch(child);
    }

    void RowExpander::revealChild(Control&)
    {
        header().setExpanded(true);
    }

    // A section shut over the selected cell leaves its header standing for it: the stop the next
    // key moves on from, and the item the container holds.
    void RowExpander::bodyToggled(ToggleExpandedEvent& event)
    {
        Control& body = *m_controls[k_body];
        body.setVisible(event.expanded());
        if (event.expanded() || !body.containsNested(m_descriptor.selectedRow()))
            return;
        selectNavigationStop();
        header().makeCurrentItem();
    }

}
