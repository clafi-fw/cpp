export module ClaFi.Icons.CutIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::CutIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // A ring added to whatever the path already holds. PixelPath::drawCircle begins a fresh
        // path, so the blades would go with it.
        void addRing(PixelPath& path, FloatPoint center, float radius)
        {
            float controlOffset = radius * 0.4142f;
            float diagonal = radius * 0.7071f;

            path.moveTo(center.x, center.y - radius);
            path.quadTo({ center.x + controlOffset, center.y - radius }, { center.x + diagonal, center.y - diagonal });
            path.quadTo({ center.x + radius, center.y - controlOffset }, { center.x + radius, center.y });
            path.quadTo({ center.x + radius, center.y + controlOffset }, { center.x + diagonal, center.y + diagonal });
            path.quadTo({ center.x + controlOffset, center.y + radius }, { center.x, center.y + radius });
            path.quadTo({ center.x - controlOffset, center.y + radius }, { center.x - diagonal, center.y + diagonal });
            path.quadTo({ center.x - radius, center.y + controlOffset }, { center.x - radius, center.y });
            path.quadTo({ center.x - radius, center.y - controlOffset }, { center.x - diagonal, center.y - diagonal });
            path.quadTo({ center.x - controlOffset, center.y - radius }, { center.x, center.y - radius });
            path.close();
        }
    }

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        // A strong grey holds the object, the accent marks what names the command. Here that is
        // the blades: the handles are what any pair of scissors has, the crossing edges are the cut.
        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color accentColor = event.accentRgb(InkGrade::Strongest);

        // The handles. The radius and the blade ends agree: each end sits on the ring it meets.
        addRing(path, { size * 0.33f, size * 0.85f }, size * 0.15f);
        addRing(path, { size * 0.67f, size * 0.85f }, size * 0.15f);
        canvas.drawPath(path, { PathDrawLayer::stroke(bodyColor, strokeWidth) }, &transform);

        // The two blades. Each runs from a tip at the top into the ring diagonally opposite it,
        // so they cross on the icon's centre line at 0.46 of its height.
        path.clear();
        path.moveTo(size * 0.28f, size * 0.02f);
        path.lineTo(size * 0.62f, size * 0.70f);
        path.moveTo(size * 0.72f, size * 0.02f);
        path.lineTo(size * 0.38f, size * 0.70f);
        canvas.drawPath(path, { PathDrawLayer::stroke(accentColor, strokeWidth) }, &transform);
    }

}
