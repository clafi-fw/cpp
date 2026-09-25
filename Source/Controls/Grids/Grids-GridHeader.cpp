module ClaFi.Controls.Grids;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.AppTheme_Baked;
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

    void GridHeader::adjustPaint(AdjustPaintEvent& event)
    {
        RowBase::adjustPaint(event);
        // What establishes the element, not only the surface the cells are filled with: the ink
        // is what the cell text is drawn in and what the lines take their direction from, and a
        // control hosted in a header cell inherits the same pair every other child inherits. The
        // row's selection is left as it stands - a column name answers no pointer of its own.
        //
        // The row paints no surface of its own - the cells do - and a cell is filled from the
        // row's surface, so naming the surface rule here is the whole of what colours a header.
        // It states for the row the colour those cells arrive at, which is what everything
        // reading this row's surface needs to see.
        //
        // flip comes with them: it says which side of the theme the element stands on, so an ink
        // raised off that surface rises the way it leaves room for. An expander's header takes
        // the whole set and gets it that way; this row copies, so it has to name it.
        const BakedElement& header = event.bakedColors().element(UiElement::Header);
        const BakedElement& rowRules = event.colorRules();
        const BakedElement rules = {
            .flip = header.flip,
            .surface = header.surface,
            .active = rowRules.active,
            .text = header.text,
            .activeText = rowRules.activeText
        };
        event.setColorRules(rules);
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
