export module ClaFi.Icons.CopyIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::CopyIcon
{
    using namespace ::ClaFi::Graphics;

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        // A strong grey holds the object, the accent marks what names the command. Here that is
        // the front sheet: it is the copy, and the sheet behind it is what was copied.
        Color bodyColor = event.textRgb(InkGrade::Strong);
        Color accentColor = event.accentRgb(InkGrade::Strongest);

        float radius = size * 0.12f;
        // Where the front sheet starts, and how far the back one reaches. The front sheet is
        // opaque to the eye only because the back one stops at its edge, so these two numbers
        // are what keep the sheets from drawing through each other.
        float front = size * 0.28f;
        float back = size * 0.72f;

        // The back sheet, open at both ends: the outline stops where the front sheet covers it.
        path.moveTo(back, front);
        path.lineTo(back, radius);
        path.quadTo({ back, 0.0f }, { back - radius, 0.0f });
        path.lineTo(radius, 0.0f);
        path.quadTo({ 0.0f, 0.0f }, { 0.0f, radius });
        path.lineTo(0.0f, back - radius);
        path.quadTo({ 0.0f, back }, { radius, back });
        path.lineTo(front, back);
        canvas.drawPath(path, { PathDrawLayer::stroke(bodyColor, strokeWidth) }, &transform);

        // The front sheet, whole.
        path.clear();
        path.moveTo(front + radius, front);
        path.lineTo(size - radius, front);
        path.quadTo({ size, front }, { size, front + radius });
        path.lineTo(size, size - radius);
        path.quadTo({ size, size }, { size - radius, size });
        path.lineTo(front + radius, size);
        path.quadTo({ front, size }, { front, size - radius });
        path.lineTo(front, front + radius);
        path.quadTo({ front, front }, { front + radius, front });
        path.close();
        canvas.drawPath(path, { PathDrawLayer::stroke(accentColor, strokeWidth) }, &transform);
    }

}
