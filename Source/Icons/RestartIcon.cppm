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
    // The sweep is drawn in cubics, none longer than a quarter turn: a cubic holds a circle to a
    // hair at that length and drifts past it.
    constexpr int k_segments{ 4 };

    // How large the ring and its head stand in the icon's square.
    export struct RingShape
    {
        float radiusShare{ 0.68f };     // of the half size, the head's outer barb inside the box
        float headLength{ 0.5f };       // of the radius, along the chord the head lies on
        float headHalfWidth{ 0.35f };   // of the radius, to each side of that chord
    };

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

    // The ring in local space, in a square of `size`, which ResetIcon draws mirrored. Each
    // segment's control points lie on the tangents at its ends, at the distance that keeps the
    // curve on the circle.
    export void buildRingPath(PixelPath& path, const float size, const RingShape& shape = {})
    {
        const FloatPoint center = { size * 0.5f };
        const float radius = size * 0.5f * shape.radiusShare;
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

    // The head, an open chevron on the tip the ring arrives at. It lies along the chord of the
    // ring it covers, so the ring runs in between its legs - laid on the tangent at the tip, its
    // inner leg would lie along the ring. Separate from the ring because it carries the accent.
    export void buildHeadPath(PixelPath& path, const float size, const RingShape& shape = {})
    {
        const FloatPoint center = { size * 0.5f };
        const float radius = size * 0.5f * shape.radiusShare;
        const float end = radiansOf(k_startDegrees + k_sweepDegrees);
        const FloatPoint tip = pointAt(center, radius, end);
        // Where the ring's tangent runs parallel to that chord - at the chord's middle.
        const float axis = end - std::asin(shape.headLength * 0.5f);
        const FloatPoint back = -headingAt(axis) * (radius * shape.headLength);
        const FloatPoint aside = FloatPoint{ std::cos(axis), std::sin(axis) }
            * (radius * shape.headHalfWidth);
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

        Matrix3x2 transform = Matrix3x2::Translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        buildRingPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::Stroke(event.textRgb(InkGrade::Strong), strokeWidth) }, &transform);

        path.clear();
        buildHeadPath(path, size);
        canvas.drawPath(path, { PathDrawLayer::Stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) }, &transform);
    }
}
