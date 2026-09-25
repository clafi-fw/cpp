export module ClaFi.Icons.UndoIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::UndoIcon
{
    using namespace ::ClaFi::Graphics;

    // The shaft in local space, in a square of `size`: it rises out of the bottom right and
    // sweeps over to where the head sits. Redo is this same curve mirrored, which is why the
    // shape is stated here rather than inside paint().
    export void buildShaftPath(PixelPath& path, float size)
    {
        path.moveTo(size * 0.90f, size * 0.92f);
        path.cubicTo(
            { size * 0.98f, size * 0.46f },
            { size * 0.64f, size * 0.28f },
            { size * 0.20f, size * 0.30f });
    }

    // The head, an open chevron on the tip the shaft arrives at. Separate from the shaft because
    // it carries the accent colour: which way the arrow points is what tells undo from redo.
    export void buildHeadPath(PixelPath& path, float size)
    {
        path.moveTo(size * 0.40f, size * 0.12f);
        path.lineTo(size * 0.16f, size * 0.30f);
        path.lineTo(size * 0.42f, size * 0.50f);
    }

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        buildShaftPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) }, &transform);

        path.clear();
        buildHeadPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) }, &transform);
    }

}
