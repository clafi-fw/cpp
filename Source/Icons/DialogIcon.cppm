export module ClaFi.Icons.DialogIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::DialogIcon
{
    using namespace ::ClaFi::Graphics;

    // A box with an arrow leaving its top right corner: a command that opens a window of its own.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    // The box, as fractions of the icon's extent. It stops short of the top and the right so the
    // arrow has somewhere to go: an arrow ending on the box's own corner reads as part of the box
    // rather than as something leaving it.
    constexpr float k_boxTop = 0.13f;
    constexpr float k_boxRight = 0.86f;

    // How far back each of the box's corners is cut, as a fraction of its width.
    constexpr float k_cornerRadius = 0.12f;

    // Where the box breaks for the arrow - along its top edge, and down its right one. Both are
    // fractions of the side they sit on.
    constexpr float k_breakX = 0.46f;
    constexpr float k_breakY = 0.50f;

    // Where the arrow's tail stands inside the box, and how long each leg of its head is.
    constexpr float k_tailX = 0.46f;
    constexpr float k_tailY = 0.58f;
    constexpr float k_headLength = 0.33f;

    // How thick the outline is drawn, as a fraction of the extent. Proportional rather than a
    // hairline, so the mark keeps the weight of the text it rides in at every size.
    constexpr float k_strokeShare = 0.075f;

    void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        // A stroke lies inside its bounds, so the shape is built half a stroke in and that much
        // smaller rather than pre-inset at each point.
        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const float boxTop = size * k_boxTop;
        const float boxRight = size * k_boxRight;
        const float radius = boxRight * k_cornerRadius;

        PixelPath path;

        // The box, from the break in its top edge and round the long way to the break in its
        // right one.
        path.moveTo(boxRight * k_breakX, boxTop);
        path.lineTo(radius, boxTop);
        path.quadTo({ 0.0f, boxTop }, { 0.0f, boxTop + radius });
        path.lineTo(0.0f, size - radius);
        path.quadTo({ 0.0f, size }, { radius, size });
        path.lineTo(boxRight - radius, size);
        path.quadTo({ boxRight, size }, { boxRight, size - radius });
        path.lineTo(boxRight, boxTop + (size - boxTop) * k_breakY);

        const Matrix3x2 transform = Matrix3x2::translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });

        event.canvas().drawPath(
            path,
            { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) },
            &transform
        );

        // The arrow: a diagonal out of the box and a head at its far end, both reaching the top
        // right corner of the extent.
        path.clear();
        path.moveTo(size * k_tailX, size * k_tailY);
        path.lineTo(size, 0.0f);
        path.moveTo(size - size * k_headLength, 0.0f);
        path.lineTo(size, 0.0f);
        path.lineTo(size, size * k_headLength);

        event.canvas().drawPath(
            path,
            { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) },
            &transform
        );
    }

}
