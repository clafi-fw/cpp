export module ClaFi.Icons.DeleteIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::DeleteIcon
{
    using namespace ::ClaFi::Graphics;

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        const Color bodyColor = event.textRgb(InkGrade::Strong);
        // The lid and the handle are drawn apart from the body so that they can be told apart, and
        // they are the part of a bin that moves.
        //
        // TODO: should these carry the accent? Accent marks the object being acted on, and what is
        // acted on here is the thing going in rather than the lid.
        const Color accentColor = bodyColor;

        // Bin body, tapered, reaching the bottom edge.
        path.moveTo(size * 0.15f, size * 0.12f);
        path.lineTo(size * 0.85f, size * 0.12f);
        path.lineTo(size * 0.78f, size * 1.00f);
        path.lineTo(size * 0.22f, size * 1.00f);
        path.close();

        // The ribs down the front.
        path.moveTo(size * 0.40f, size * 0.30f);
        path.lineTo(size * 0.42f, size * 0.80f);
        path.moveTo(size * 0.60f, size * 0.30f);
        path.lineTo(size * 0.58f, size * 0.80f);
        canvas.drawPath(path, { PathDrawLayer::stroke(bodyColor, strokeWidth) }, &transform);

        // Handle, on the top edge.
        path.clear();
        path.moveTo(size * 0.35f, size * 0.12f);
        path.lineTo(size * 0.35f, 0.0f);
        path.lineTo(size * 0.65f, 0.0f);
        path.lineTo(size * 0.65f, size * 0.12f);

        // Lid, wider than the bin it sits on.
        path.moveTo(size * 0.05f, size * 0.12f);
        path.lineTo(size * 0.95f, size * 0.12f);
        canvas.drawPath(path, { PathDrawLayer::stroke(accentColor, strokeWidth) }, &transform);
    }

}
