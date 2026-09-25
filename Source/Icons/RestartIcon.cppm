export module ClaFi.Icons.RestartIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::RestartIcon
{
    using namespace ::ClaFi::Graphics;

    // The ring is most of a circle, run clockwise from one o'clock round to eleven, so the gap
    // stands at the top and the head lands beside it. Angles as the screen has them: degrees
    // from three o'clock, increasing clockwise.
    constexpr float k_startDegrees{ -50.0f };
    constexpr float k_sweepDegrees{ 300.0f };
    // The ring as a share of the icon's half size, leaving the head's outer barb inside the box.
    constexpr float k_radiusShare{ 0.68f };
    constexpr float k_headLength{ 0.5f };
    constexpr float k_headHalfWidth{ 0.35f };
    // The sweep is drawn in cubics, none longer than a quarter turn: a cubic holds a circle to a
    // hair at that length and drifts past it.
    constexpr int k_segments{ 4 };

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
    void buildRingPath(PixelPath& path, const FloatPoint center, const float radius)
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

    // The head, an open chevron on the tip the ring arrives at, pointing on along it. Separate
    // from the ring because it carries the accent colour.
    void buildHeadPath(PixelPath& path, const FloatPoint center, const float radius)
    {
        const float end = radiansOf(k_startDegrees + k_sweepDegrees);
        const FloatPoint tip = pointAt(center, radius, end);
        const FloatPoint back = -headingAt(end) * (radius * k_headLength);
        const FloatPoint aside = FloatPoint{ std::cos(end), std::sin(end) } * (radius * k_headHalfWidth);
        path.moveTo(tip + back + aside);
        path.lineTo(tip);
        path.lineTo(tip + back - aside);
    }

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const FloatPoint center{ size * 0.5f };
        const float radius = size * 0.5f * k_radiusShare;
        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        buildRingPath(path, center, radius);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) }, &transform);

        path.clear();
        buildHeadPath(path, center, radius);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) }, &transform);
    }
}
