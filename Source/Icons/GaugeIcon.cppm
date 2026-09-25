export module ClaFi.Icons.GaugeIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::GaugeIcon
{
    using namespace ::ClaFi::Graphics;

    // The dial is the top half of a circle, stopped a little short at each end so the two tips
    // read as ends rather than as a bar. Angles as the screen has them: degrees from three
    // o'clock, increasing clockwise.
    constexpr float k_startDegrees{ 185.0f };
    constexpr float k_sweepDegrees{ 170.0f };
    // Where the needle stands. High, and short of the end: a reading, not a fault.
    constexpr float k_needleDegrees{ 317.0f };
    // The dial as a share of the icon's half size, and the pivot's drop below the icon's own
    // centre as a share of its size - which is what leaves the dial the top of the box and the
    // needle the middle.
    constexpr float k_radiusShare{ 0.75f };
    constexpr float k_pivotDropShare{ 0.15f };
    // The marks under the dial, the needle and the pivot, as shares of the radius.
    constexpr float k_tickShare{ 0.22f };
    constexpr float k_needleShare{ 0.68f };
    constexpr float k_pivotShare{ 0.18f };
    // Two cubics for the sweep: a cubic holds a circle to a hair up to a quarter turn and drifts
    // past it.
    constexpr int k_segments{ 2 };
    // The ends of the scale and its middle, which is also where the dial is highest.
    constexpr float k_tickDegrees[]{ 180.0f, 270.0f, 360.0f };

    [[nodiscard]] float radiansOf(const float degrees)
    {
        return degrees * std::numbers::pi_v<float> / 180.0f;
    }

    [[nodiscard]] FloatPoint pointAt(const FloatPoint center, const float radius, const float radians)
    {
        return center + FloatPoint{ std::cos(radians), std::sin(radians) } * radius;
    }

    // Where a clockwise sweep is heading at an angle, as a unit vector.
    [[nodiscard]] FloatPoint headingAt(const float radians)
    {
        return { -std::sin(radians), std::cos(radians) };
    }

    // Each segment's control points lie on the tangents at its ends, at the distance that keeps
    // the curve on the circle.
    void buildDialPath(PixelPath& path, const FloatPoint center, const float radius)
    {
        const float step = radiansOf(k_sweepDegrees) / k_segments;
        const float handle = 4.0f / 3.0f * std::tan(step / 4.0f) * radius;
        float angle = radiansOf(k_startDegrees);
        path.moveTo(pointAt(center, radius, angle));
        for (int i = 0; i < k_segments; ++i)
        {
            const float next = angle + step;
            const FloatPoint from = pointAt(center, radius, angle);
            const FloatPoint to = pointAt(center, radius, next);
            path.cubicTo(
                from + headingAt(angle) * handle,
                to - headingAt(next) * handle,
                to);
            angle = next;
        }
    }

    // The scale's marks, hung inside the dial so that the dial stays the icon's outline.
    void buildTicksPath(PixelPath& path, const FloatPoint center, const float radius)
    {
        for (const float degrees : k_tickDegrees)
        {
            const float angle = radiansOf(degrees);
            path.moveTo(pointAt(center, radius, angle));
            path.lineTo(pointAt(center, radius * (1.0f - k_tickShare), angle));
        }
    }

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const FloatPoint pivot = event.iconCenter() + FloatPoint{ 0.0f, size * k_pivotDropShare };
        const float radius = size * 0.5f * k_radiusShare;
        Canvas& canvas = event.canvas();
        PixelPath path;

        buildDialPath(path, pivot, radius);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) });

        path.clear();
        buildTicksPath(path, pivot, radius);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Muted), strokeWidth) });

        const Color needleColor = event.accentRgb(InkGrade::Strongest);
        const FloatPoint tip = pointAt(pivot, radius * k_needleShare, radiansOf(k_needleDegrees));
        canvas.drawLine(pivot, tip, needleColor, strokeWidth);
        canvas.fillCircle(pivot, radius * k_pivotShare, needleColor);
    }
}
