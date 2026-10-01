module ClaFi.Controls.Grids;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{


    // GridHeader

    void GridHeader::defaultGetCellText(const Cell& cell, Text& text)
    {
        text << cell.column.text();
    }

    void GridHeader::getCellText(const Column& column, Text& text)
    {
        descriptor().owner().getHeaderCellText({ *this, column }, text);
    }

    const TextFormat& GridHeader::cellTextFormat(const Column& column) const
    {
        return column.headerTextFormat();
    }

    void GridHeader::adjustPaint(AdjustPaintEvent& event)
    {
        RowBase::adjustPaint(event);
        event.setColorRules(UiElement::GridHeader);
    }

    void GridHeader::paintSurface(PaintEvent& event)
    {
        RowBase::paintSurface(event);
        doPaintColumns(event);
    }

    RoundedRectangleParts GridHeader::silhouette(const PaintEvent& gridEvent, const FloatRect& headerRect) const
    {
        // The corners the cells turn, from the same two answers paintOneCell reads: a grid rounds
        // the pair at the top of its first row, and leaves the pair at the bottom to whichever
        // row ends the grid. The header is that first row wherever the scroll has carried it.
        //
        // The radius is the cell frame's rather than the grid's. A cell lays its stroke inside
        // the grid's own frame, so the outer corner it turns is one border tighter.
        const float border = descriptor().scaledCellMetrics().border;
        const CornerRadii gridRadii = GridDescriptor::cornerRadiiOf(gridEvent);
        return {
            .bounds = headerRect,
            .radii = {
                std::max(0.0f, gridRadii[cornerIndex(Corner::TopLeft)] - border),
                std::max(0.0f, gridRadii[cornerIndex(Corner::TopRight)] - border),
                0.0f,
                0.0f
            }
        };
    }

}
