module ClaFi.Controls.Grids;

import :Descriptor;
import :RowDivider;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Controls::Grids
{
    // RowDivider

    void RowDivider::paintSurface(PaintEvent& event)
    {
        // The band along the bottom of the row is the slot the grid line closing the divider
        // stands in, so the divider's own fill stops short of it. The line is drawn on what that
        // gap leaves showing - the surface the row sits on - the same as every other line of the
        // lattice is drawn on what it lies over.
        FloatRect fillRect = event.controlBounds();

        const Color surfaceRgb = event.surfaceRgb();
        if (surfaceRgb.alpha)
        {
            RoundedRectangleParts parts{
                .bounds = FloatRect::intersection(fillRect, event.viewport()),
                .radii = event.cornerRadii(),
                .sides = k_allRectSidesTrue
            };
            event.canvas().fillPartialRoundedRectangle(parts, surfaceRgb);
        }

        // The slot is kept whether or not a line stands in it, so a grid drawn without horizontal
        // lines shows the same band as one drawn with them.
        if (!descriptor().drawsHorizontalLines())
            return;

        // A rule that reaches nothing answers a transparent colour, and the slot is then left as
        // it is. Filling it with the divider's own colour instead would state a line the theme
        // did not ask for, and the gap is what a divider carrying no line looks like.
        const Hsl surfaceHsl = event.surfaceHsl();// event.parentEvent()->surfaceHsl();
        const Color lineColor = GridDescriptor::gridLineRgb(surfaceHsl, event.bakedColors(),
            event.lightness());
        if (lineColor.alpha)
        {
            FloatRect lineRect = fillRect;
            const float strokeWidth = descriptor().scaledBorderWidth() / 2;
            // Top
            lineRect.bottom = lineRect.top + strokeWidth;
            event.canvas().fillRectangle(lineRect, lineColor);
            // Bottom
            lineRect.bottom = fillRect.bottom;
            lineRect.top = lineRect.bottom - strokeWidth;
            event.canvas().fillRectangle(lineRect, lineColor);
        }
    }

}
