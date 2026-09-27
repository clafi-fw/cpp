export module ClaFi.Icons.ResetIcon;

import ClaFi.Icons.RestartIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::ResetIcon
{
    using namespace ::ClaFi::Graphics;

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        // Reset is the restart ring run back the other way, so this icon reflects that ring across
        // the vertical centre of its own square. The reflection is the right operand: a point
        // meets that one first, and the placement second.
        const Matrix3x2 mirror = { -1.0f, 0.0f, 0.0f, 1.0f, size, 0.0f };
        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f }) * mirror;
        Canvas& canvas = event.canvas();
        PixelPath path;

        RestartIcon::buildRingPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) }, &transform);

        path.clear();
        RestartIcon::buildHeadPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) }, &transform);
    }
}
