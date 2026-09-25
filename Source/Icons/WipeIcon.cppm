export module ClaFi.Icons.WipeIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::WipeIcon
{
    using namespace ::ClaFi::Graphics;

    // An eraser with a trail behind it. Drawn at the size of a toolbar mark, where a shape carries
    // about four strokes before it turns to mush - so the eraser is one block with one band across
    // it, the trail is two lines, and both reach the edges of the box rather than sitting inside a
    // margin of their own.
    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;

        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        // A strong grey holds the object, the accent marks what names the command - the division
        // DeleteIcon makes. Here the object is the eraser and the command is the trail it leaves:
        // an eraser standing still is a shape, an eraser with a trail is a wipe.
        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color accentColor = event.accentRgb(InkGrade::Strongest);

        // The eraser, tilted so it reads as travelling rather than resting, and the band across it
        // that separates the rubber from the holder.
        path.moveTo(size * 0.62f, size * 0.02f);
        path.lineTo(size * 1.00f, size * 0.18f);
        path.lineTo(size * 0.72f, size * 0.98f);
        path.lineTo(size * 0.34f, size * 0.82f);
        path.close();
        path.moveTo(size * 0.48f, size * 0.42f);
        path.lineTo(size * 0.86f, size * 0.58f);
        canvas.drawPath(path, { PathDrawLayer::stroke(bodyColor, strokeWidth) }, &transform);

        // The trail, running off the leading edge of the box and shortening away from the eraser,
        // which is what says the block has been travelling rather than merely standing there.
        path.clear();
        path.moveTo(0.0f, size * 0.30f);
        path.lineTo(size * 0.32f, size * 0.30f);
        path.moveTo(0.0f, size * 0.66f);
        path.lineTo(size * 0.24f, size * 0.66f);
        canvas.drawPath(path, { PathDrawLayer::stroke(accentColor, strokeWidth) }, &transform);
    }

}
