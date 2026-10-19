module ClaFi.Icons.OpenChevron;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::OpenChevron
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        enum class Point
        {
            Up,
            Down
        };

        void paintPointing(PaintIconEvent& event, const Point point)
        {
            const FloatRect& iconRect = event.iconRect();
            const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
            const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
            // The arms run at 45 degrees, so the mark spans three quarters of the square.
            const float halfHeight = size * 0.19f;
            const float tipY = point == Point::Up ? -halfHeight : halfHeight;

            PixelPath path;
            path.moveTo(-2.0f * halfHeight, -tipY);
            path.lineTo(0.0f, tipY);
            path.lineTo(2.0f * halfHeight, -tipY);

            const Color color = event.textRgb(InkGrade::Strongest);
            const Matrix3x2 transform = Matrix3x2::Translation(event.iconCenter());
            event.canvas().drawPath(path, { PathDrawLayer::Stroke(color, strokeWidth) }, &transform);
        }
    }

    void paintUp(PaintIconEvent& event)
    {
        paintPointing(event, Point::Up);
    }

    void paintDown(PaintIconEvent& event)
    {
        paintPointing(event, Point::Down);
    }

}
