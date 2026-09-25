export module ClaFi.Tools.WhatsClip.Icon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip::AppIcon
{
    using namespace ::ClaFi::Graphics;

    // The application's own mark, stretched onto the rect it is given so it touches all four
    // sides. It takes no colour from the theme: this icon stands on a task bar and a title bar
    // the framework does not paint, so it carries its own palette and has to hold on a light
    // ground and a dark one alike.
    export void paint(Canvas&, const FloatRect& bounds);

    // The same mark shaped as a PaintIconFunc, so a control can take it as a property. It
    // reads the rect off the event and nothing else - there is no ink here to take from
    // the theme.
    export void paintIcon(PaintIconEvent&);
}

namespace ClaFi::Tools::WhatsClip::AppIcon
{
    namespace
    {
        constexpr float k_epsilon{ 0.0001f };

        // A leaf is a quadrilateral, not a rectangle. Each corner carries a fixed offset, so no
        // two of its sides are quite parallel. The offsets are constants because a shape that
        // takes new ones on every paint does not read as hand-drawn, it flickers.
        struct Leaf
        {
            float left;
            float top;
            float right;
            float bottom;
            float radius;
            std::array<FloatPoint, 4> offsets;
        };

        // A run across a leaf, from one fraction of its height to another. Inset pulls the ends
        // in from the leaf's sides, which is what turns a band into a line of content.
        struct Band
        {
            float from;
            float to;
            Color color;
            float inset;
            float topRadius;
            float bottomRadius;
        };

        enum class Rendering
        {
            Swatch,
            Paper
        };

        struct Extent
        {
            float left;
            float top;
            float width;
            float height;
        };

        constexpr Color k_sky{ 0xFF4FA8D8 };
        constexpr Color k_periwinkle{ 0xFF7C8FE3 };
        constexpr Color k_amber{ 0xFFE5813C };
        constexpr Color k_leafUpper{ 0xFFA3B9DA };
        constexpr Color k_leafLower{ 0xFF8CA4C9 };
        constexpr Color k_paperBack{ 0xFFE9EEF6 };
        constexpr Color k_paperFront{ 0xFFF7FAFD };
        constexpr Color k_paperEdge{ 0xFF9DA9C0 };
        constexpr Color k_frameColor{ 0xFF3A3A44 };
        constexpr Color k_frameEdgeColor{ 0xFFF4F5F8 };
        constexpr Color k_rimShade{ 0x291E1432 };
        constexpr Color k_highlight{ 0xB8FFFFFF };

        constexpr Leaf k_backLeaf{
            .left = 0.05f,
            .top = 0.07f,
            .right = 0.55f,
            .bottom = 0.75f,
            .radius = 0.085f,
            .offsets = { FloatPoint{ 0.008f, -0.005f }, FloatPoint{ -0.006f, 0.008f },
                         FloatPoint{ 0.005f, 0.006f }, FloatPoint{ -0.007f, -0.004f } }
        };

        constexpr Leaf k_frontLeaf{
            .left = 0.40f,
            .top = 0.27f,
            .right = 0.95f,
            .bottom = 0.90f,
            .radius = 0.095f,
            .offsets = { FloatPoint{ -0.007f, 0.006f }, FloatPoint{ 0.009f, -0.005f },
                         FloatPoint{ -0.005f, -0.008f }, FloatPoint{ 0.006f, 0.005f } }
        };

        constexpr std::array<Band, 2> k_backBands{
            Band{ .from = 0.00f, .to = 0.45f, .color = k_leafUpper, .topRadius = k_backLeaf.radius },
            Band{ .from = 0.45f, .to = 1.00f, .color = k_leafLower, .bottomRadius = k_backLeaf.radius }
        };

        constexpr std::array<Band, 3> k_frontBands{
            Band{ .from = 0.00f, .to = 0.34f, .color = k_sky, .topRadius = k_frontLeaf.radius },
            Band{ .from = 0.34f, .to = 0.67f, .color = k_periwinkle },
            Band{ .from = 0.67f, .to = 1.00f, .color = k_amber, .bottomRadius = k_frontLeaf.radius }
        };

        constexpr float k_lineRadius{ 0.02f };

        constexpr std::array<Band, 2> k_backLines{
            Band{ .from = 0.20f, .to = 0.29f, .color = k_periwinkle, .inset = 0.14f,
                  .topRadius = k_lineRadius, .bottomRadius = k_lineRadius },
            Band{ .from = 0.40f, .to = 0.49f, .color = k_sky, .inset = 0.14f,
                  .topRadius = k_lineRadius, .bottomRadius = k_lineRadius }
        };

        constexpr std::array<Band, 3> k_frontLines{
            Band{ .from = 0.17f, .to = 0.27f, .color = k_sky, .inset = 0.12f,
                  .topRadius = k_lineRadius, .bottomRadius = k_lineRadius },
            Band{ .from = 0.39f, .to = 0.49f, .color = k_periwinkle, .inset = 0.12f,
                  .topRadius = k_lineRadius, .bottomRadius = k_lineRadius },
            Band{ .from = 0.61f, .to = 0.71f, .color = k_amber, .inset = 0.12f,
                  .topRadius = k_lineRadius, .bottomRadius = k_lineRadius }
        };

        constexpr std::array<Leaf, 2> k_leaves{ k_backLeaf, k_frontLeaf };

        constexpr FloatPoint k_glassCenter{ 0.445f, 0.505f };
        constexpr float k_glassRadius{ 0.272f };
        constexpr float k_glassWidth{ 0.078f };
        constexpr FloatPoint k_tailEnd{ 0.278f, 0.907f };
        constexpr float k_tailWidth{ 0.070f };
        constexpr float k_paperEdgeWidth{ 0.028f };
        // A hairline of the opposite value on each edge of the frame. Whichever side matches
        // the ground the mark stands on, the other side is what draws the shape.
        constexpr float k_frameEdgeWidth{ 0.018f };
        // The leaves carry no edge of their own. This one is the light value, so it states the
        // silhouette on a middle or dark ground and gives way on a light one.
        constexpr float k_leafEdgeWidth{ 0.020f };
        // The tail ends flat. A round cap on a tail this wide ends it in a bulb; the radius is
        // only what keeps the two far corners off a razor.
        constexpr float k_tailCorner{ 0.016f };

        // The glass, inside the ring.
        constexpr float k_lensRadius{ k_glassRadius - k_glassWidth * 0.5f };
        // Full magnification at the middle of the lens, falling off with the square of the radius
        // to exactly 1 at its rim - so the edge of the disc meets the world outside it without a
        // step, and the last of the content compresses into a thin band the way real glass does.
        constexpr float k_magnification{ 1.45f };
        constexpr int k_lensSteps{ 8 };

        constexpr float k_rimInset{ 0.012f };
        constexpr float k_rimWidth{ 0.024f };
        constexpr float k_highlightRadius{ k_lensRadius * 0.72f };
        constexpr float k_highlightWidth{ 0.030f };
        constexpr float k_highlightFromDegrees{ 194.4f };
        constexpr float k_highlightToDegrees{ 255.6f };
        constexpr int k_highlightSteps{ 10 };

        // Every shape is built through this. Left alone it hands the commands to the path as they
        // come; through the lens it flattens each curve and moves every point by the law above,
        // which is what bends the paper inside the glass.
        class PathSink
        {
        public:
            PathSink(PixelPath& path, bool throughLens);
            void moveTo(FloatPoint);
            void lineTo(FloatPoint);
            void quadTo(FloatPoint control, FloatPoint end);
            void close();
        private:
            void emit(FloatPoint);
            PixelPath& m_path;
            bool m_throughLens;
            FloatPoint m_current{};
            FloatPoint m_subpathStart{};
        };

        [[nodiscard]] FloatPoint mix(FloatPoint, FloatPoint, float amount);
        [[nodiscard]] FloatPoint towards(FloatPoint from, FloatPoint to, float distance);
        [[nodiscard]] FloatPoint lensPoint(FloatPoint);
        [[nodiscard]] std::array<FloatPoint, 4> cornersOf(const Leaf&);
        [[nodiscard]] std::array<FloatPoint, 4> quadOf(const std::array<FloatPoint, 4>& leaf, const Band&);
        [[nodiscard]] std::array<FloatPoint, 4> tailQuad();
        [[nodiscard]] Extent designExtent();
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds);

        void addRoundedQuad(PathSink&, const std::array<FloatPoint, 4>&, const std::array<float, 4>& radii);
        void addCircle(PathSink&, FloatPoint center, float radius);
        void addArc(PathSink&, FloatPoint center, float radius, float fromDegrees, float toDegrees, int steps);

        void fillBand(Canvas&, const Matrix3x2&, bool throughLens, const std::array<FloatPoint, 4>& leaf, const Band&);
        void drawLeafShape(Canvas&, const Matrix3x2&, bool throughLens, const Leaf&, const PathDrawLayer&);
        void paintLeaves(Canvas&, const Matrix3x2&, Rendering);
        void paintGlass(Canvas&, const Matrix3x2&);
    }
}

//-----------------------------------------------------------------------------

namespace ClaFi::Tools::WhatsClip::AppIcon
{
    void paint(Canvas& canvas, const FloatRect& bounds)
    {
        const Matrix3x2 transform = fitTo(bounds);

        paintLeaves(canvas, transform, Rendering::Swatch);

        // What the glass shows is not a magnified copy of what is under it - it is the same
        // leaves drawn as paper, cut to the disc. The clip is the only thing that keeps the
        // second drawing inside the first.
        PixelPath lens;
        PathSink lensSink{ lens, false };
        addCircle(lensSink, k_glassCenter, k_lensRadius);
        canvas.pushClip(lens, &transform);
        paintLeaves(canvas, transform, Rendering::Paper);
        canvas.popClip();

        paintGlass(canvas, transform);
    }

    void paintIcon(PaintIconEvent& event)
    {
        paint(event.canvas(), event.iconRect());
    }

    namespace
    {
        PathSink::PathSink(PixelPath& path, const bool throughLens)
            :
            m_path{ path },
            m_throughLens{ throughLens }
        {
        }

        void PathSink::moveTo(const FloatPoint point)
        {
            m_current = point;
            m_subpathStart = point;
            m_path.moveTo(m_throughLens ? lensPoint(point) : point);
        }

        void PathSink::lineTo(const FloatPoint point)
        {
            if (!m_throughLens)
            {
                m_path.lineTo(point);
                m_current = point;
                return;
            }

            for (int step = 1; step <= k_lensSteps; ++step)
                emit(mix(m_current, point, static_cast<float>(step) / k_lensSteps));

            m_current = point;
        }

        void PathSink::quadTo(const FloatPoint control, const FloatPoint end)
        {
            if (!m_throughLens)
            {
                m_path.quadTo(control, end);
                m_current = end;
                return;
            }

            const FloatPoint start = m_current;
            for (int step = 1; step <= k_lensSteps; ++step)
            {
                const float amount = static_cast<float>(step) / k_lensSteps;
                emit(mix(mix(start, control, amount), mix(control, end, amount), amount));
            }

            m_current = end;
        }

        void PathSink::close()
        {
            m_path.close();
            m_current = m_subpathStart;
        }

        void PathSink::emit(const FloatPoint point)
        {
            m_path.lineTo(lensPoint(point));
        }

        FloatPoint mix(const FloatPoint from, const FloatPoint to, const float amount)
        {
            return { from.x + (to.x - from.x) * amount, from.y + (to.y - from.y) * amount };
        }

        FloatPoint towards(const FloatPoint from, const FloatPoint to, const float distance)
        {
            const float dx = to.x - from.x;
            const float dy = to.y - from.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            if (length < k_epsilon)
                return from;

            return { from.x + dx / length * distance, from.y + dy / length * distance };
        }

        FloatPoint lensPoint(const FloatPoint point)
        {
            const float dx = point.x - k_glassCenter.x;
            const float dy = point.y - k_glassCenter.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < k_epsilon)
                return point;

            const float reach = std::min(distance / k_lensRadius, 1.0f);
            const float factor = k_magnification - (k_magnification - 1.0f) * reach * reach;
            return { k_glassCenter.x + dx * factor, k_glassCenter.y + dy * factor };
        }

        std::array<FloatPoint, 4> cornersOf(const Leaf& leaf)
        {
            const std::array<FloatPoint, 4> base{
                FloatPoint{ leaf.left, leaf.top },
                FloatPoint{ leaf.right, leaf.top },
                FloatPoint{ leaf.right, leaf.bottom },
                FloatPoint{ leaf.left, leaf.bottom }
            };

            std::array<FloatPoint, 4> result{};
            for (std::size_t index = 0; index < base.size(); ++index)
                result[index] = { base[index].x + leaf.offsets[index].x, base[index].y + leaf.offsets[index].y };

            return result;
        }

        std::array<FloatPoint, 4> quadOf(const std::array<FloatPoint, 4>& leaf, const Band& band)
        {
            const FloatPoint topLeft = mix(leaf[0], leaf[3], band.from);
            const FloatPoint topRight = mix(leaf[1], leaf[2], band.from);
            const FloatPoint bottomRight = mix(leaf[1], leaf[2], band.to);
            const FloatPoint bottomLeft = mix(leaf[0], leaf[3], band.to);

            if (band.inset < k_epsilon)
                return { topLeft, topRight, bottomRight, bottomLeft };

            return {
                mix(topLeft, topRight, band.inset),
                mix(topRight, topLeft, band.inset),
                mix(bottomRight, bottomLeft, band.inset),
                mix(bottomLeft, bottomRight, band.inset)
            };
        }

        // Everything drawn, measured in design space: both leaves' offset corners and the hairline
        // on them, the ring's outer edge and the round cap on the end of the tail. It is what the
        // mark is stretched from, and it is settled at compile time because none of it moves.
        Extent designExtent()
        {
            constexpr float outer = k_glassRadius + k_glassWidth * 0.5f + k_frameEdgeWidth;
            float left = k_glassCenter.x - outer;
            float top = k_glassCenter.y - outer;
            float right = k_glassCenter.x + outer;
            float bottom = k_glassCenter.y + outer;

            const auto take = [&left, &top, &right, &bottom](const float x, const float y)
            {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x);
                bottom = std::max(bottom, y);
            };

            for (const Leaf& leaf : k_leaves)
            {
                const std::array<FloatPoint, 4> base{
                    FloatPoint{ leaf.left, leaf.top },
                    FloatPoint{ leaf.right, leaf.top },
                    FloatPoint{ leaf.right, leaf.bottom },
                    FloatPoint{ leaf.left, leaf.bottom }
                };
                for (std::size_t index = 0; index < base.size(); ++index)
                {
                    const float cornerX = base[index].x + leaf.offsets[index].x;
                    const float cornerY = base[index].y + leaf.offsets[index].y;
                    take(cornerX - k_leafEdgeWidth * 0.5f, cornerY - k_leafEdgeWidth * 0.5f);
                    take(cornerX + k_leafEdgeWidth * 0.5f, cornerY + k_leafEdgeWidth * 0.5f);
                }
            }

            for (const FloatPoint& corner : tailQuad())
            {
                take(corner.x - k_frameEdgeWidth, corner.y - k_frameEdgeWidth);
                take(corner.x + k_frameEdgeWidth, corner.y + k_frameEdgeWidth);
            }

            return { left, top, right - left, bottom - top };
        }

        Matrix3x2 fitTo(const FloatRect& bounds)
        {
            // Settled once. Nothing in the drawing moves, so the bounds it is stretched from do not.
            static const Extent s_design = designExtent();

            const float scaleX = bounds.width() / s_design.width;
            const float scaleY = bounds.height() / s_design.height;
            const FloatPoint origin = bounds.topLeft();

            return {
                scaleX, 0.0f,
                0.0f, scaleY,
                origin.x - s_design.left * scaleX,
                origin.y - s_design.top * scaleY
            };
        }

        void addRoundedQuad(PathSink& sink, const std::array<FloatPoint, 4>& points, const std::array<float, 4>& radii)
        {
            bool started = false;
            for (std::size_t index = 0; index < points.size(); ++index)
            {
                const FloatPoint previous = points[(index + 3) % points.size()];
                const FloatPoint current = points[index];
                const FloatPoint next = points[(index + 1) % points.size()];

                const float toPrevious = std::sqrt((current.x - previous.x) * (current.x - previous.x)
                    + (current.y - previous.y) * (current.y - previous.y));
                const float toNext = std::sqrt((next.x - current.x) * (next.x - current.x)
                    + (next.y - current.y) * (next.y - current.y));
                const float radius = std::min({ radii[index], toPrevious * 0.5f, toNext * 0.5f });

                const FloatPoint entry = towards(current, previous, radius);
                const FloatPoint exit = towards(current, next, radius);

                if (started)
                    sink.lineTo(entry);
                else
                {
                    sink.moveTo(entry);
                    started = true;
                }

                if (radius > k_epsilon)
                    sink.quadTo(current, exit);
            }

            sink.close();
        }

        std::array<FloatPoint, 4> tailQuad()
        {
            const float dx = k_tailEnd.x - k_glassCenter.x;
            const float dy = k_tailEnd.y - k_glassCenter.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            const FloatPoint along{ dx / length, dy / length };
            const FloatPoint across{ -along.y, along.x };
            const float half = k_tailWidth * 0.5f;
            // The near end sits inside the ring, which is drawn over the joint.
            const FloatPoint near{
                k_glassCenter.x + along.x * (k_glassRadius - k_glassWidth * 0.5f),
                k_glassCenter.y + along.y * (k_glassRadius - k_glassWidth * 0.5f)
            };

            return {
                FloatPoint{ near.x + across.x * half, near.y + across.y * half },
                FloatPoint{ k_tailEnd.x + across.x * half, k_tailEnd.y + across.y * half },
                FloatPoint{ k_tailEnd.x - across.x * half, k_tailEnd.y - across.y * half },
                FloatPoint{ near.x - across.x * half, near.y - across.y * half }
            };
        }

        // Four quads, as the handle rings in Icons/CutIcon are built.
        void addCircle(PathSink& sink, const FloatPoint center, const float radius)
        {
            const float control = radius * 0.4142f;
            const float diagonal = radius * 0.7071f;

            sink.moveTo({ center.x, center.y - radius });
            sink.quadTo({ center.x + control, center.y - radius }, { center.x + diagonal, center.y - diagonal });
            sink.quadTo({ center.x + radius, center.y - control }, { center.x + radius, center.y });
            sink.quadTo({ center.x + radius, center.y + control }, { center.x + diagonal, center.y + diagonal });
            sink.quadTo({ center.x + control, center.y + radius }, { center.x, center.y + radius });
            sink.quadTo({ center.x - control, center.y + radius }, { center.x - diagonal, center.y + diagonal });
            sink.quadTo({ center.x - radius, center.y + control }, { center.x - radius, center.y });
            sink.quadTo({ center.x - radius, center.y - control }, { center.x - diagonal, center.y - diagonal });
            sink.quadTo({ center.x - control, center.y - radius }, { center.x, center.y - radius });
            sink.close();
        }

        void addArc(PathSink& sink, const FloatPoint center, const float radius,
            const float fromDegrees, const float toDegrees, const int steps)
        {
            constexpr float k_radiansPerDegree = std::numbers::pi_v<float> / 180.0f;

            for (int step = 0; step <= steps; ++step)
            {
                const float degrees = fromDegrees + (toDegrees - fromDegrees) * (static_cast<float>(step) / steps);
                const float radians = degrees * k_radiansPerDegree;
                const FloatPoint point{ center.x + std::cos(radians) * radius, center.y + std::sin(radians) * radius };
                if (step == 0)
                    sink.moveTo(point);
                else
                    sink.lineTo(point);
            }
        }

        void fillBand(Canvas& canvas, const Matrix3x2& transform, const bool throughLens,
            const std::array<FloatPoint, 4>& leaf, const Band& band)
        {
            PixelPath path;
            PathSink sink{ path, throughLens };
            const std::array<float, 4> radii{ band.topRadius, band.topRadius, band.bottomRadius, band.bottomRadius };
            addRoundedQuad(sink, quadOf(leaf, band), radii);
            canvas.drawPath(path, { PathDrawLayer::fill(band.color) }, &transform);
        }

        void drawLeafShape(Canvas& canvas, const Matrix3x2& transform, const bool throughLens,
            const Leaf& leaf, const PathDrawLayer& layer)
        {
            PixelPath path;
            PathSink sink{ path, throughLens };
            addRoundedQuad(sink, cornersOf(leaf), { leaf.radius, leaf.radius, leaf.radius, leaf.radius });
            canvas.drawPath(path, { layer }, &transform);
        }

        void paintLeaves(Canvas& canvas, const Matrix3x2& transform, const Rendering rendering)
        {
            const std::array<FloatPoint, 4> back = cornersOf(k_backLeaf);
            const std::array<FloatPoint, 4> front = cornersOf(k_frontLeaf);

            if (rendering == Rendering::Swatch)
            {
                // A leaf takes its edge before the next covers it, so the front one draws the seam.
                for (const Band& band : k_backBands)
                    fillBand(canvas, transform, false, back, band);

                drawLeafShape(canvas, transform, false, k_backLeaf, PathDrawLayer::stroke(k_frameEdgeColor, k_leafEdgeWidth));

                for (const Band& band : k_frontBands)
                    fillBand(canvas, transform, false, front, band);

                drawLeafShape(canvas, transform, false, k_frontLeaf, PathDrawLayer::stroke(k_frameEdgeColor, k_leafEdgeWidth));

                return;
            }

            // The back leaf and its content go down before the front leaf covers them. A line
            // drawn after would run across the sheet standing over it.
            drawLeafShape(canvas, transform, true, k_backLeaf, PathDrawLayer::fill(k_paperBack));
            for (const Band& band : k_backLines)
                fillBand(canvas, transform, true, back, band);

            drawLeafShape(canvas, transform, true, k_backLeaf, PathDrawLayer::stroke(k_paperEdge, k_paperEdgeWidth));
            drawLeafShape(canvas, transform, true, k_frontLeaf, PathDrawLayer::fill(k_paperFront));
            drawLeafShape(canvas, transform, true, k_frontLeaf, PathDrawLayer::stroke(k_paperEdge, k_paperEdgeWidth));

            for (const Band& band : k_frontLines)
                fillBand(canvas, transform, true, front, band);
        }

        void paintGlass(Canvas& canvas, const Matrix3x2& transform)
        {
            PixelPath path;
            PathSink sink{ path, false };

            addCircle(sink, k_glassCenter, k_lensRadius - k_rimInset);
            canvas.drawPath(path, { PathDrawLayer::stroke(k_rimShade, k_rimWidth) }, &transform);

            path.clear();
            addArc(sink, k_glassCenter, k_highlightRadius,
                k_highlightFromDegrees, k_highlightToDegrees, k_highlightSteps);
            canvas.drawPath(path, { PathDrawLayer::stroke(k_highlight, k_highlightWidth) }, &transform);

            PixelPath ring;
            PathSink ringSink{ ring, false };
            addCircle(ringSink, k_glassCenter, k_glassRadius);

            PixelPath tail;
            PathSink tailSink{ tail, false };
            addRoundedQuad(tailSink, tailQuad(), { 0.0f, k_tailCorner, k_tailCorner, 0.0f });

            // BOTH EDGES BEFORE EITHER FILL. Where the ring and the tail meet, the fill covers the
            // edge under it and only the fringe outside the two shapes is left showing.
            canvas.drawPath(ring,
                { PathDrawLayer::stroke(k_frameEdgeColor, k_glassWidth + k_frameEdgeWidth * 2.0f) }, &transform);
            canvas.drawPath(tail,
                { PathDrawLayer::stroke(k_frameEdgeColor, k_frameEdgeWidth * 2.0f) }, &transform);
            canvas.drawPath(tail, { PathDrawLayer::fill(k_frameColor) }, &transform);
            canvas.drawPath(ring, { PathDrawLayer::stroke(k_frameColor, k_glassWidth) }, &transform);
        }
    }
}
