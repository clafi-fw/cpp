module ClaFi.Core.Graphics.Types;

import :PixelView;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // ========================================================================
    // Implementation: arcs
    // ========================================================================

    static constexpr float k_radiansPerDegree = std::numbers::pi_v<float> / 180.0f;
    static constexpr float k_quarterTurn = std::numbers::pi_v<float> * 0.5f;

    // In double: near radii that just span the ends, float rounding moves the centre visibly.
    std::optional<ArcEllipse> arcEllipse(FloatPoint start, FloatPoint end,
        FloatPoint radii, float rotation, ArcSize arcSize, ArcSweep arcSweep)
    {
        double radiusX = std::abs(static_cast<double>(radii.x));
        double radiusY = std::abs(static_cast<double>(radii.y));
        if (start == end || radiusX == 0.0 || radiusY == 0.0)
            return std::nullopt;

        const double angle = static_cast<double>(rotation) * std::numbers::pi / 180.0;
        const double cosAngle = std::cos(angle);
        const double sinAngle = std::sin(angle);

        // Half the chord from end to start, in the ellipse's own axes.
        const double chordX = (static_cast<double>(start.x) - end.x) * 0.5;
        const double chordY = (static_cast<double>(start.y) - end.y) * 0.5;
        const double x = cosAngle * chordX + sinAngle * chordY;
        const double y = cosAngle * chordY - sinAngle * chordX;

        const double reach = (x * x) / (radiusX * radiusX) + (y * y) / (radiusY * radiusY);
        if (reach > 1.0)
        {
            const double growth = std::sqrt(reach);
            radiusX *= growth;
            radiusY *= growth;
        }

        const double squaredX = radiusX * radiusX;
        const double squaredY = radiusY * radiusY;
        const double across = squaredX * y * y + squaredY * x * x;
        double offset = std::sqrt((std::max)(0.0, (squaredX * squaredY - across) / across));
        if ((arcSize == ArcSize::Large) == (arcSweep == ArcSweep::Clockwise))
            offset = -offset;
        const double centerX = offset * radiusX * y / radiusY;
        const double centerY = -offset * radiusY * x / radiusX;

        const double startAngle = std::atan2((y - centerY) / radiusY, (x - centerX) / radiusX);
        const double endAngle = std::atan2((-y - centerY) / radiusY, (-x - centerX) / radiusX);
        double sweepAngle = endAngle - startAngle;
        if (arcSweep == ArcSweep::Clockwise && sweepAngle < 0.0)
            sweepAngle += 2.0 * std::numbers::pi;
        else if (arcSweep == ArcSweep::CounterClockwise && sweepAngle > 0.0)
            sweepAngle -= 2.0 * std::numbers::pi;

        return ArcEllipse{
            .center = {
                static_cast<float>(cosAngle * centerX - sinAngle * centerY + (static_cast<double>(start.x) + end.x) * 0.5),
                static_cast<float>(sinAngle * centerX + cosAngle * centerY + (static_cast<double>(start.y) + end.y) * 0.5),
            },
            .radii = { static_cast<float>(radiusX), static_cast<float>(radiusY) },
            .rotation = static_cast<float>(angle),
            .startAngle = static_cast<float>(startAngle),
            .sweepAngle = static_cast<float>(sweepAngle),
        };
    }

    ArcCubics arcCubics(const ArcEllipse& arc, FloatPoint end)
    {
        const float cosAngle = std::cos(arc.rotation);
        const float sinAngle = std::sin(arc.rotation);
        auto pointAt = [&](float angle){
            const float x = arc.radii.x * std::cos(angle);
            const float y = arc.radii.y * std::sin(angle);
            return FloatPoint{
                arc.center.x + cosAngle * x - sinAngle * y,
                arc.center.y + sinAngle * x + cosAngle * y,
            };
        };
        auto tangentAt = [&](float angle){
            const float x = -arc.radii.x * std::sin(angle);
            const float y = arc.radii.y * std::cos(angle);
            return FloatPoint{ cosAngle * x - sinAngle * y, sinAngle * x + cosAngle * y };
        };

        // A sweep a hair over a whole number of quarter turns takes no extra piece.
        const float quarters = std::ceil(std::abs(arc.sweepAngle) / k_quarterTurn - 0.001f);

        ArcCubics result;
        result.count = (std::max)(std::size_t{ 1 }, static_cast<std::size_t>(quarters));
        const float step = arc.sweepAngle / static_cast<float>(result.count);
        const float handle = std::tan(step * 0.25f) * (4.0f / 3.0f);
        for (std::size_t i = 0; i < result.count; ++i)
        {
            const float from = arc.startAngle + step * static_cast<float>(i);
            const float to = from + step;
            const FloatPoint toPoint = i + 1 == result.count ? end : pointAt(to);
            result.segments[i] = {
                pointAt(from) + tangentAt(from) * handle,
                toPoint - tangentAt(to) * handle,
                toPoint,
            };
        }
        return result;
    }

    // The ellipse carried through the matrix's linear part, and its turn reversed by a mirror.
    static void transformArc(PathCommand& arc, const Matrix3x2& matrix)
    {
        const float angle = arc.p3.x * k_radiansPerDegree;
        const float cosAngle = std::cos(angle);
        const float sinAngle = std::sin(angle);
        const FloatPoint axisX = matrix.transformVector({ arc.p2.x * cosAngle, arc.p2.x * sinAngle });
        const FloatPoint axisY = matrix.transformVector({ -arc.p2.y * sinAngle, arc.p2.y * cosAngle });

        // The mapped ellipse's axes are the eigenvectors of this symmetric matrix. The shorter one
        // comes from the area, which a thin ellipse keeps where the smaller eigenvalue loses it.
        const float xx = axisX.x * axisX.x + axisY.x * axisY.x;
        const float xy = axisX.x * axisX.y + axisY.x * axisY.y;
        const float yy = axisX.y * axisX.y + axisY.y * axisY.y;
        const float mean = (xx + yy) * 0.5f;
        const float spread = std::hypot((xx - yy) * 0.5f, xy);
        const float determinant = matrix.a * matrix.d - matrix.b * matrix.c;
        const float longer = std::sqrt(mean + spread);
        const float shorter = longer > 0.0f ? std::abs(determinant * arc.p2.x * arc.p2.y) / longer : 0.0f;
        arc.p2 = { longer, shorter };
        arc.p3.x = std::atan2(2.0f * xy, xx - yy) * 0.5f / k_radiansPerDegree;

        if (determinant < 0.0f)
        {
            arc.arcSweep = arc.arcSweep == ArcSweep::Clockwise
                ? ArcSweep::CounterClockwise
                : ArcSweep::Clockwise;
        }
    }

    // ========================================================================
    // Implementation: PixelPath
    // ========================================================================

    void PixelPath::arcTo(FloatPoint radii, float rotation, ArcSize arcSize, ArcSweep arcSweep, FloatPoint end)
    {
        m_commands.push_back({ PathCommandType::ArcTo, end, radii, { rotation, 0.0f }, arcSize, arcSweep });
        m_currentPoint = end;
    }

    void PixelPath::arcBy(FloatPoint radii, float rotation, ArcSize arcSize, ArcSweep arcSweep, FloatPoint delta)
    {
        m_commands.push_back({ PathCommandType::ArcBy, delta, radii, { rotation, 0.0f }, arcSize, arcSweep });
        m_currentPoint = m_currentPoint + delta;
    }

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
        const float halfWidth = w * 0.5f;
        const float halfHeight = h * 0.5f;
        const float radius = std::min({ r, halfWidth, halfHeight });
        drawRoundedRect({ -halfWidth, -halfHeight, halfWidth, halfHeight }, radius, radius);
    }

    // Each radius is cut to half its own side, as SVG cuts a rect's.
    void PixelPath::drawRoundedRect(const FloatRect& rect, float radiusX, float radiusY)
    {
        const FloatPoint radii = {
            (std::max)(0.0f, (std::min)(radiusX, rect.width() * 0.5f)),
            (std::max)(0.0f, (std::min)(radiusY, rect.height() * 0.5f)),
        };

        clear();
        moveTo(rect.left + radii.x, rect.top);
        lineTo(rect.right - radii.x, rect.top);
        arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, { rect.right, rect.top + radii.y });
        lineTo(rect.right, rect.bottom - radii.y);
        arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, { rect.right - radii.x, rect.bottom });
        lineTo(rect.left + radii.x, rect.bottom);
        arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, { rect.left, rect.bottom - radii.y });
        lineTo(rect.left, rect.top + radii.y);
        arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, { rect.left + radii.x, rect.top });
        close();
    }

    // Two half turns, as an SVG path states an ellipse.
    void PixelPath::drawEllipse(FloatPoint center, float radiusX, float radiusY)
    {
        const FloatPoint radii = { radiusX, radiusY };
        const FloatPoint top = { center.x, center.y - radiusY };
        const FloatPoint bottom = { center.x, center.y + radiusY };

        clear();
        moveTo(top);
        arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, bottom);
        arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, top);
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

                case PathCommandType::ArcTo:
                    cmd.p1 = matrix.transform(cmd.p1);
                    transformArc(cmd, matrix);
                    currentPoint = cmd.p1;
                    break;

                case PathCommandType::ArcBy:
                    currentPoint = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                    cmd.p1 = matrix.transformVector(cmd.p1);
                    transformArc(cmd, matrix);
                    break;

                case PathCommandType::Close:
                    break;
            }
        }

        m_currentPoint = matrix.transform(m_currentPoint);
    }
}
