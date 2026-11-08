module ClaFi.PathArt.SceneHelpers;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // How far a cubic's controls stand from the corner of a quarter ellipse.
    constexpr float k_quarterEllipseControl{ 0.5523f };

    namespace
    {
        // The brush placed the way `toCanvas` places the box it fills: moved, and scaled alike
        // both ways.
        [[nodiscard]] Brush brushOnCanvas(const Brush& brush, const Matrix3x2& toCanvas)
        {
            const float scale = toCanvas.a;
            const auto place = [&](FloatPoint point) {
                return FloatPoint{ point.x * scale + toCanvas.e, point.y * scale + toCanvas.f };
            };
            return std::visit([&](const auto& source) -> Brush {
                using Kind = std::decay_t<decltype(source)>;
                Kind placed = source;
                if constexpr (std::is_same_v<Kind, LinearGradient>)
                {
                    placed.startPoint = place(source.startPoint);
                    placed.endPoint = place(source.endPoint);
                }
                else if constexpr (std::is_same_v<Kind, RadialGradient>)
                {
                    placed.center = place(source.center);
                    placed.offset = source.offset * scale;
                    placed.radiusX = source.radiusX * scale;
                    placed.radiusY = source.radiusY * scale;
                }
                else if constexpr (std::is_same_v<Kind, PointGlow>)
                {
                    placed.lightPos = place(source.lightPos);
                    placed.lightSpread = source.lightSpread * scale;
                }
                return placed;
            }, brush);
        }
    }

    SceneFrame SceneFrame::FromBounds(Canvas& canvas, const FloatRect& bounds)
    {
        return {
            .canvas = canvas,
            .width = bounds.width(),
            .height = bounds.height(),
            .unit = std::min(bounds.width(), bounds.height()) / 600.0f,
            .toCanvas = Matrix3x2::Translation(bounds.topLeft())
        };
    }

    SceneFrame SceneFrame::within(const Matrix3x2& local) const
    {
        return {
            .canvas = canvas,
            .width = width,
            .height = height,
            .unit = unit,
            .toCanvas = toCanvas * local
        };
    }

    ScatteredShapes::ScatteredShapes(const SceneFrame& frame, const Brush& brush,
        const float strokeWidth, const StrokeCap cap)
        :
        m_frame{ frame },
        m_brush{ brush },
        m_strokeWidth{ strokeWidth },
        m_cap{ cap },
        m_oneByOne{ frame.canvas.backend() == nullptr
            || frame.canvas.backend()->type() == BackendType::Cpu }
    {
    }

    PixelPath& ScatteredShapes::next()
    {
        if (m_oneByOne)
            draw();
        return m_path;
    }

    void ScatteredShapes::draw()
    {
        if (m_path.isEmpty())
            return;
        if (m_strokeWidth > 0.0f)
            strokeShape(m_frame, m_path, m_brush, m_strokeWidth, m_cap);
        else
            fillShape(m_frame, m_path, m_brush);
        m_path.clear();
    }

    float pseudoRandom(const int index, const float shift)
    {
        const float value = std::abs(std::sin(static_cast<float>(index) * 12.9898f + shift))
            * 43758.5453f;
        return std::fmod(value, 1.0f);
    }

    float fractionalPart(const float value)
    {
        return value - std::floor(value);
    }

    float bump(const float value, const float center, const float width)
    {
        const float distance = (value - center) / width;
        return std::exp(-distance * distance);
    }

    FloatPoint rotated(const FloatPoint point, const float angle)
    {
        const float sine = std::sin(angle);
        const float cosine = std::cos(angle);
        return { point.x * cosine - point.y * sine, point.x * sine + point.y * cosine };
    }

    FloatPoint pointBetween(const FloatPoint from, const FloatPoint to, const float share)
    {
        return { std::lerp(from.x, to.x, share), std::lerp(from.y, to.y, share) };
    }

    Color colorBetween(const Color from, const Color to, const float share)
    {
        return Color::blend(from, to, std::clamp(share, 0.0f, 1.0f));
    }

    Color faded(Color color, const float share)
    {
        color.alpha = static_cast<ColorByte>(std::lround(color.alpha * share));
        return color;
    }

    float calculateSyncRotation(const float cycleTime, const int index, const float symmetryFactor)
    {
        const float spinDirection = index % 2 == 0 ? 1.0f : -1.0f;
        const float rotationsPerLoop = static_cast<float>(1 + index % 2);
        return cycleTime * (1.0f / symmetryFactor) * spinDirection * rotationsPerLoop
            + static_cast<float>(index);
    }

    Limb reachToward(const FloatPoint root, const FloatPoint target, const float upperLength,
        const float lowerLength, const BendToward bendToward)
    {
        FloatPoint toTarget = target - root;
        float distance = std::hypot(toTarget.x, toTarget.y);
        const float longest = (upperLength + lowerLength) * 0.998f;
        FloatPoint end = target;
        if (distance > longest)
        {
            end = root + toTarget * (longest / distance);
            distance = longest;
        }
        distance = std::max(distance, std::abs(upperLength - lowerLength) + 0.001f);

        // The law of cosines gives the angle the upper part leaves the line to the end at.
        const float cosine = (upperLength * upperLength + distance * distance
            - lowerLength * lowerLength) / (2.0f * upperLength * distance);
        const float bend = std::acos(std::clamp(cosine, -1.0f, 1.0f));
        const float toEnd = std::atan2(end.y - root.y, end.x - root.x);
        const auto jointAt = [&](float angle) {
            return root + FloatPoint{ std::cos(angle), std::sin(angle) } * upperLength;
        };
        const FloatPoint jointOne = jointAt(toEnd + bend);
        const FloatPoint jointTwo = jointAt(toEnd - bend);

        FloatPoint joint = jointOne;
        switch (bendToward)
        {
            case BendToward::Left:
                joint = jointOne.x < jointTwo.x ? jointOne : jointTwo;
                break;
            case BendToward::Right:
                joint = jointOne.x > jointTwo.x ? jointOne : jointTwo;
                break;
            case BendToward::Down:
                joint = jointOne.y > jointTwo.y ? jointOne : jointTwo;
                break;
        }
        return { joint, end };
    }

    // The calling thread takes indices as well, so a count of 1 runs on it alone.
    void spreadOverCores(const int count, const std::function<void(int)>& job)
    {
        std::atomic<int> nextIndex{ 0 };
        const auto work = [&]() {
            for (int index = nextIndex++; index < count; index = nextIndex++)
                job(index);
        };
        const int cores = static_cast<int>(std::thread::hardware_concurrency());
        const int helperCount = std::min(count, cores) - 1;
        std::vector<std::future<void>> helpers;
        for (int helper = 0; helper < helperCount; ++helper)
            helpers.push_back(std::async(std::launch::async, work));
        work();
        for (std::future<void>& helper : helpers)
            helper.get();
    }

    void addEllipse(PixelPath& path, const FloatPoint center, const float radiusX,
        const float radiusY)
    {
        const float controlX = radiusX * k_quarterEllipseControl;
        const float controlY = radiusY * k_quarterEllipseControl;
        const float left = center.x - radiusX;
        const float right = center.x + radiusX;
        const float top = center.y - radiusY;
        const float bottom = center.y + radiusY;
        path.moveTo(right, center.y);
        path.cubicTo({ right, center.y + controlY }, { center.x + controlX, bottom },
            { center.x, bottom });
        path.cubicTo({ center.x - controlX, bottom }, { left, center.y + controlY },
            { left, center.y });
        path.cubicTo({ left, center.y - controlY }, { center.x - controlX, top },
            { center.x, top });
        path.cubicTo({ center.x + controlX, top }, { right, center.y - controlY },
            { right, center.y });
        path.close();
    }

    void addSparkle(PixelPath& path, const FloatPoint center, const float radius)
    {
        const float inner = radius * 0.25f;
        path.moveTo(center.x, center.y - radius);
        path.lineTo(center.x + inner, center.y - inner);
        path.lineTo(center.x + radius, center.y);
        path.lineTo(center.x + inner, center.y + inner);
        path.lineTo(center.x, center.y + radius);
        path.lineTo(center.x - inner, center.y + inner);
        path.lineTo(center.x - radius, center.y);
        path.lineTo(center.x - inner, center.y - inner);
        path.close();
    }

    void fillShape(const SceneFrame& frame, const PixelPath& path, const Brush& brush,
        const Matrix3x2& local)
    {
        const Matrix3x2 toCanvas = frame.toCanvas * local;
        frame.canvas.fillPath(path, brush, &toCanvas);
    }

    void strokeShape(const SceneFrame& frame, const PixelPath& path, const Brush& brush,
        const float width, const StrokeCap cap, const Matrix3x2& local)
    {
        const Matrix3x2 toCanvas = frame.toCanvas * local;
        const PathDrawLayer stroke{
            .geometry{
                .mode = PathRenderMode::Stroke,
                .strokeWidth = width,
                .strokeCap = cap
            },
            .brush = brush
        };
        frame.canvas.drawPath(path, std::span<const PathDrawLayer>{ &stroke, 1 }, &toCanvas);
    }

    void drawSegment(const SceneFrame& frame, const FloatPoint from, const FloatPoint to,
        const Brush& brush, const float width, const StrokeCap cap)
    {
        PixelPath path;
        path.moveTo(from);
        path.lineTo(to);
        strokeShape(frame, path, brush, width, cap);
    }

    void fillDisc(const SceneFrame& frame, const FloatPoint center, const float radius,
        const Brush& brush)
    {
        if (radius <= 0.0f)
            return;
        PixelPath path;
        path.drawCircle(center, radius);
        fillShape(frame, path, brush);
    }

    void fillOval(const SceneFrame& frame, const FloatPoint center, const float radiusX,
        const float radiusY, const Brush& brush, const float angle)
    {
        if (radiusX <= 0.0f || radiusY <= 0.0f)
            return;
        PixelPath path;
        path.drawEllipse({}, radiusX, radiusY);
        fillShape(frame, path, brush, Matrix3x2::Translation(center) * Matrix3x2::Rotation(angle));
    }

    // Under a frame that only moves and scales, the box is a rectangle fill, which the CPU backend
    // paints without the path rasterizer and Direct2D without building a geometry.
    void fillBox(const SceneFrame& frame, const FloatRect& box, const Brush& brush)
    {
        const Matrix3x2& toCanvas = frame.toCanvas;
        const bool square = toCanvas.b == 0.0f && toCanvas.c == 0.0f && toCanvas.a == toCanvas.d;
        if (square && toCanvas.a > 0.0f)
        {
            const float scale = toCanvas.a;
            const FloatRect onCanvas = {
                box.left * scale + toCanvas.e,
                box.top * scale + toCanvas.f,
                box.right * scale + toCanvas.e,
                box.bottom * scale + toCanvas.f
            };
            frame.canvas.fillRectangle(onCanvas, brushOnCanvas(brush, toCanvas));
            return;
        }
        PixelPath path;
        path.moveTo(box.left, box.top);
        path.lineTo(box.right, box.top);
        path.lineTo(box.right, box.bottom);
        path.lineTo(box.left, box.bottom);
        path.close();
        fillShape(frame, path, brush);
    }

    void fillRoundedBox(const SceneFrame& frame, const FloatRect& box, const float radius,
        const Brush& brush)
    {
        PixelPath path;
        path.drawRoundedRect(box, radius, radius);
        fillShape(frame, path, brush);
    }

    void drawGlow(const SceneFrame& frame, const FloatPoint center, const float spread,
        const Color color, const float xRatio)
    {
        if (color.alpha == 0 || spread <= 0.0f)
            return;
        const float halfWidth = spread / xRatio;
        fillBox(frame,
            { center.x - halfWidth, center.y - spread, center.x + halfWidth, center.y + spread },
            PointGlow{
                .lightPos = center,
                .lightColor = color,
                .lightSpread = spread,
                .xRatio = xRatio,
                .shape = GlowShape::Circle
            });
    }

    void drawSparkle(const SceneFrame& frame, const FloatPoint center, const float radius,
        const Color color)
    {
        PixelPath path;
        addSparkle(path, center, radius);
        fillShape(frame, path, SolidColor{ color });
    }
}
