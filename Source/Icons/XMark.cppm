export module ClaFi.Icons.XMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::XMark
{
    using namespace ::ClaFi::Graphics;

    // The path is built at the icon rect's own size and placed by the transform below. The canvas
    // transform is composed by the backend, which scales the cross and its line width together.
    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float lineWidth = event.scaledStrokeWidth(Thickness::Thin);
        const Color color = event.textRgb(InkGrade::Strongest);
        PixelPath path;
        // Safety padding
        FloatRect markRect = { 0.0f, 0.0f, iconRect.width(), iconRect.height() };
        markRect.inflate(-lineWidth / 2.0f);
        path.moveTo(markRect.topLeft());
        path.lineTo(markRect.bottomRight());
        path.moveTo(markRect.bottomLeft());
        path.lineTo(markRect.topRight());
        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft());
        event.canvas().drawPath(path, { PathDrawLayer::stroke(color, lineWidth) }, &transform);
    }
}
