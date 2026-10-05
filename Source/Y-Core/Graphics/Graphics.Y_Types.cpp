module ClaFi.Core.Graphics.Types;

import :PixelView;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // ========================================================================
    // Implementation: PixelPath
    // ========================================================================

    // A quarter circle as one cubic: each handle's length as a share of the radius.
    static constexpr float k_quarterArcHandle = 0.5522847f;

    void PixelPath::addRoundedPolygon(std::span<const FloatPoint> points, float radius)
    {
        const std::size_t count = points.size();
        if (count < 3ull)
            return;

        auto lengthBetween = [](FloatPoint first, FloatPoint second){
            const FloatPoint delta = second - first;
            return std::sqrt(delta.x * delta.x + delta.y * delta.y);
        };

        // A point on the edge running from a corner toward another, the stated distance along it.
        auto pointAlong = [&lengthBetween](FloatPoint corner, FloatPoint other, float distance){
            const float length = lengthBetween(corner, other);
            if (length <= 0.0f)
                return corner;
            const float step = distance / length;
            return FloatPoint{ corner.x + (other.x - corner.x) * step,
                corner.y + (other.y - corner.y) * step };
        };

        auto cutOf = [&](std::size_t index){
            const float before = lengthBetween(points[index], points[(index + count - 1ull) % count]);
            const float after = lengthBetween(points[index], points[(index + 1ull) % count]);
            return (std::min)(radius, (std::min)(before, after) * 0.5f);
        };

        // Each turn of the walk draws the edge into one corner and the arc around it, so the walk
        // starts where the first corner's arc will end.
        moveTo(pointAlong(points[0ull], points[1ull], cutOf(0ull)));
        for (std::size_t index = 0ull; index != count; ++index)
        {
            const std::size_t at = (index + 1ull) % count;
            const FloatPoint& corner = points[at];
            const float cut = cutOf(at);
            lineTo(pointAlong(corner, points[index], cut));
            quadTo(corner, pointAlong(corner, points[(index + 2ull) % count], cut));
        }
        close();
    }

    void PixelPath::drawRoundedRect(float w, float h, float r)
    {
        const float right = w * 0.5f;
        const float bottom = h * 0.5f;
        const float left = -right;
        const float top = -bottom;
        r = std::min({ r, right, bottom });
        // How far from its corner each handle of the corner's arc lies.
        const float reach = r * (1.0f - k_quarterArcHandle);

        clear();
        moveTo(left + r, top);
        lineTo(right - r, top);
        cubicTo({ right - reach, top }, { right, top + reach }, { right, top + r });
        lineTo(right, bottom - r);
        cubicTo({ right, bottom - reach }, { right - reach, bottom }, { right - r, bottom });
        lineTo(left + r, bottom);
        cubicTo({ left + reach, bottom }, { left, bottom - reach }, { left, bottom - r });
        lineTo(left, top + r);
        cubicTo({ left, top + reach }, { left + reach, top }, { left + r, top });
        close();
    }

    // A cubic a quarter turn, each off the circle by 0.03 percent of the radius at most.
    void PixelPath::drawCircle(FloatPoint center, float radius)
    {
        const float handle = radius * k_quarterArcHandle;
        const float left = center.x - radius;
        const float top = center.y - radius;
        const float right = center.x + radius;
        const float bottom = center.y + radius;

        clear();
        moveTo(center.x, top);
        cubicTo({ center.x + handle, top }, { right, center.y - handle }, { right, center.y });
        cubicTo({ right, center.y + handle }, { center.x + handle, bottom }, { center.x, bottom });
        cubicTo({ center.x - handle, bottom }, { left, center.y + handle }, { left, center.y });
        cubicTo({ left, center.y - handle }, { center.x - handle, top }, { center.x, top });
        close();
    }

    // Axis-aligned commands do not survive a general matrix - a horizontal line stops being
    // horizontal under a rotation - so each one is resolved against the running point and rewritten
    // as its unconstrained equivalent before being mapped.
    void PixelPath::transform(const Matrix3x2& matrix)
    {
        FloatPoint currentPoint = { 0.0f, 0.0f };

        for (auto& cmd : m_commands)
        {
            switch (cmd.type)
            {
                case PathCommandType::MoveTo:
                    cmd.p1 = matrix.transform(cmd.p1);
                    currentPoint = cmd.p1;
                    break;

                case PathCommandType::MoveBy:
                    currentPoint = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                    cmd.p1 = matrix.transformVector(cmd.p1);
                    break;

                case PathCommandType::LineTo:
                    cmd.p1 = matrix.transform(cmd.p1);
                    currentPoint = cmd.p1;
                    break;

                case PathCommandType::LineBy:
                    currentPoint = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                    cmd.p1 = matrix.transformVector(cmd.p1);
                    break;

                case PathCommandType::HLineTo:
                {
                    FloatPoint absolute = { cmd.p1.x, currentPoint.y };
                    cmd.type = PathCommandType::LineTo;
                    cmd.p1 = matrix.transform(absolute);
                    currentPoint = cmd.p1;
                    break;
                }

                case PathCommandType::HLineBy:
                {
                    FloatPoint relative = { cmd.p1.x, 0.0f };
                    currentPoint = { currentPoint.x + relative.x, currentPoint.y };
                    cmd.type = PathCommandType::LineBy;
                    cmd.p1 = matrix.transformVector(relative);
                    break;
                }

                case PathCommandType::VLineTo:
                {
                    FloatPoint absolute = { currentPoint.x, cmd.p1.y };
                    cmd.type = PathCommandType::LineTo;
                    cmd.p1 = matrix.transform(absolute);
                    currentPoint = cmd.p1;
                    break;
                }

                case PathCommandType::VLineBy:
                {
                    FloatPoint relative = { 0.0f, cmd.p1.y };
                    currentPoint = { currentPoint.x, currentPoint.y + relative.y };
                    cmd.type = PathCommandType::LineBy;
                    cmd.p1 = matrix.transformVector(relative);
                    break;
                }

                case PathCommandType::QuadTo:
                    cmd.p1 = matrix.transform(cmd.p1);
                    cmd.p2 = matrix.transform(cmd.p2);
                    currentPoint = cmd.p2;
                    break;

                case PathCommandType::QuadBy:
                    currentPoint = { currentPoint.x + cmd.p2.x, currentPoint.y + cmd.p2.y };
                    cmd.p1 = matrix.transformVector(cmd.p1);
                    cmd.p2 = matrix.transformVector(cmd.p2);
                    break;

                case PathCommandType::CubicTo:
                    cmd.p1 = matrix.transform(cmd.p1);
                    cmd.p2 = matrix.transform(cmd.p2);
                    cmd.p3 = matrix.transform(cmd.p3);
                    currentPoint = cmd.p3;
                    break;

                case PathCommandType::CubicBy:
                    currentPoint = { currentPoint.x + cmd.p3.x, currentPoint.y + cmd.p3.y };
                    cmd.p1 = matrix.transformVector(cmd.p1);
                    cmd.p2 = matrix.transformVector(cmd.p2);
                    cmd.p3 = matrix.transformVector(cmd.p3);
                    break;

                case PathCommandType::Close:
                    break;
            }
        }

        m_currentPoint = matrix.transform(m_currentPoint);
    }
}
