module ClaFi.Controls.Grids;

import :Descriptor;
import :RowDivider;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Controls::Grids
{
    // RowDivider

    void RowDivider::paintSurface(PaintEvent& event)
    {
        // The band along the bottom of the row is the slot the line closing the divider stands
        // in, so the divider's own fill stops short of it. The line is the divider's own stroke,
        // the way every row of the grid draws its lines with its own.
        FloatRect fillRect = event.controlBounds();

        const Color surfaceRgb = event.surfaceRgb();
        if (surfaceRgb.alpha)
        {
            RoundedRectangleParts parts{
                .bounds = FloatRect::intersection(fillRect, event.viewport()),
                .radii = event.cornerRadii(),
                .sides = RectSides::all()
            };
            event.canvas().fillPartialRoundedRectangle(parts, surfaceRgb);
        }

        // The slot is kept whether or not a line stands in it, so a grid drawn without horizontal
        // lines shows the same band as one drawn with them.
        if (!descriptor().drawsHorizontalLines())
            return;

        // A stroke that reaches nothing is transparent, and the slot is then left as it is.
        // Filling it with the divider's own colour instead would state a line the theme did not
        // ask for, and the gap is what a divider carrying no line looks like. Raised off the
        // surface rather than read from strokeRgb(), which carries the focus ring.
        const Color lineColor = event.strokeRgbOn(event.surfaceHsl());
        // A line of odd width has its middle pixel on this row's last, which the bottom half stops
        // short of.
        const float shared = sharedLinePixel();
        const float strokeWidth = (descriptor().scaledBorderWidth() - shared) / 2.0f;
        if (lineColor.alpha && strokeWidth > 0.0f)
        {
            FloatRect lineRect = fillRect;
            // Top
            lineRect.bottom = lineRect.top + strokeWidth;
            event.canvas().fillRectangle(lineRect, lineColor);
            // Bottom
            lineRect.bottom = fillRect.bottom - shared;
            lineRect.top = lineRect.bottom - strokeWidth;
            event.canvas().fillRectangle(lineRect, lineColor);
        }
        const RectSides sides = { .top = true, .right = false, .bottom = true, .left = false };
        paintSharedLinePixels(event, fillRect, sides, lineColor, event.surfaceHsl().toColor());
    }

}
