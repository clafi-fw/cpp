export module ClaFi.Icons.RedoIcon;

import ClaFi.Icons.UndoIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::RedoIcon
{
    using namespace ::ClaFi::Graphics;

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        // Redo is undo's arrow read the other way round, so the curve has one statement and this
        // icon reflects it across the vertical centre of its own square. The reflection is the
        // right operand: a point meets that one first, and the placement second.
        Matrix3x2 mirror = { -1.0f, 0.0f, 0.0f, 1.0f, size, 0.0f };
        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f }) * mirror;
        Canvas& canvas = event.canvas();
        PixelPath path;

        UndoIcon::buildShaftPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) }, &transform);

        path.clear();
        UndoIcon::buildHeadPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) }, &transform);
    }

}
