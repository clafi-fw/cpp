module ClaFi.Icons.OpenChevron;

import ClaFi.Core.Context.DrawIconEvent;
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
            Down,
            Right
        };

        void drawPointing(DrawIconEvent& event, const Point point)
        {
            const FloatRect& iconRect = event.iconRect();
            const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
            const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
            // The arms run at 45 degrees, so the mark spans three quarters of the square.
            const float halfHeight = size * 0.19f;
            const float tip = point == Point::Up ? -halfHeight : halfHeight;

            PixelPath path;
            if (point == Point::Right)
            {
                path.moveTo(-tip, -2.0f * halfHeight);
                path.lineTo(tip, 0.0f);
                path.lineTo(-tip, 2.0f * halfHeight);
            }
            else
            {
                path.moveTo(-2.0f * halfHeight, -tip);
                path.lineTo(0.0f, tip);
                path.lineTo(2.0f * halfHeight, -tip);
            }

            const Color color = event.textRgb(InkGrade::Strongest);
            const Matrix3x2 transform = Matrix3x2::Translation(event.iconCenter());
            event.canvas().drawPath(path, { PathDrawLayer::Stroke(color, strokeWidth) }, &transform);
        }
    }

    void drawUp(DrawIconEvent& event)
    {
        drawPointing(event, Point::Up);
    }

    void drawDown(DrawIconEvent& event)
    {
        drawPointing(event, Point::Down);
    }

    void drawRight(DrawIconEvent& event)
    {
        drawPointing(event, Point::Right);
    }

}
