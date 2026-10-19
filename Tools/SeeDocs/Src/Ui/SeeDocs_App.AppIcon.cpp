module SeeDocs_App.AppIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    namespace
    {
        using Corners = std::array<FloatPoint, 4>;
        using CornerRadii = std::array<float, 4>;
        using PointRun = std::span<const FloatPoint>;

        // WhatsClip's leaf, seen only through the glass: no two of its sides are quite parallel.
        struct Leaf
        {
            float left{};
            float top{};
            float right{};
            float bottom{};
            float radius{};
            Corners offsets{};
        };

        // A line of content across a leaf, from one fraction of its height to another.
        struct Line
        {
            float from{};
            float to{};
            Color color{};
            float inset{};   // how far the ends are pulled in from the leaf's sides
        };

        // A noodle: the points it passes through, joined by a centripetal Catmull-Rom spline.
        struct Strand
        {
            PointRun points;
            Color color{};
            float startWidth{};
        };

        struct Extent
        {
            float left{};
            float top{};
            float width{};
            float height{};
        };

        // One span of a strand, from one of its points to the next.
        struct Cubic
        {
            FloatPoint start{};
            FloatPoint control1{};
            FloatPoint control2{};
            FloatPoint end{};
        };

        // A point on a strand's centre line, the normal there, and how far along the strand it is.
        struct Sample
        {
            FloatPoint point{};
            FloatPoint normal{};
            float along{};
        };

        using Samples = std::vector<Sample>;

        // Every shape is built through this; through the lens it bends each point by the lens law.
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

        constexpr float k_epsilon{ 0.0001f };

        constexpr Color k_sky = 0xFF'A079C1; // { 0xFF4FA8D8 };
        constexpr Color k_periwinkle{ 0xFF7C8FE3 };
        constexpr Color k_amber = 0xFF'F189E1;// { 0xFFE5813C };
        constexpr Color k_paperBack{ 0xFFE9EEF6 };
        constexpr Color k_paperFront{ 0xFFF7FAFD };
        constexpr Color k_paperEdge{ 0xFF9DA9C0 };
        constexpr Color k_frameColor = 0xFF'38365C; //{ 0xFF3A3A44 };
        constexpr Color k_frameEdgeColor = 0xFF'CAC2D3; // { 0xFFF4F5F8 };
        constexpr Color k_rimShade{ 0x291E1432 };
        constexpr Color k_highlight{ 0xB8FFFFFF };

        constexpr Color k_StrandColor1 = 0xFF'B456F6; // FAA9D0;
        constexpr Color k_StrandColor2 = 0xFF'D46AF1;
        constexpr Color k_StrandColor3 = 0xFF'F189E1;
        constexpr Color k_StrandColor4 = 0xFF'B456F6; // Top Left


        // WhatsClip's box, tail included, so the glass lands on the same pixels in both marks.
        constexpr Extent k_markBox{ .left = 0.033f, .top = 0.055f, .width = 0.936f, .height = 0.8834f };

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

        constexpr float k_lineRadius{ 0.02f };

        constexpr std::array<Line, 2> k_backLines{
            Line{ .from = 0.20f, .to = 0.29f, .color = k_periwinkle, .inset = 0.14f },
            Line{ .from = 0.40f, .to = 0.49f, .color = k_sky, .inset = 0.14f }
        };

        constexpr std::array<Line, 3> k_frontLines{
            Line{ .from = 0.17f, .to = 0.27f, .color = k_sky, .inset = 0.12f },
            Line{ .from = 0.39f, .to = 0.49f, .color = k_periwinkle, .inset = 0.12f },
            Line{ .from = 0.61f, .to = 0.71f, .color = k_amber, .inset = 0.12f }
        };

        constexpr FloatPoint k_glassCenter{ 0.445f, 0.505f };
        constexpr float k_glassRadius{ 0.272f };
        constexpr float k_glassWidth{ 0.078f };
        constexpr FloatPoint k_tailEnd{ 0.278f, 0.907f };
        constexpr float k_tailWidth{ 0.070f };
        constexpr float k_paperEdgeWidth{ 0.028f };
        // The light hairline round the frame, which states its shape on a dark ground.
        constexpr float k_frameEdgeWidth{ 0.018f };
        constexpr float k_tailCorner{ 0.016f };   // keeps the flat end's corners off a razor

        // The glass, inside the ring.
        constexpr float k_lensRadius{ k_glassRadius - k_glassWidth * 0.5f };
        // At the lens's middle; it falls with the square of the radius to exactly 1 at the rim.
        constexpr float k_magnification{ 1.45f };
        constexpr int k_lensSteps{ 8 };

        constexpr float k_rimInset{ 0.012f };
        constexpr float k_rimWidth{ 0.024f };
        constexpr float k_highlightRadius{ k_lensRadius * 0.72f };
        constexpr float k_highlightWidth{ 0.030f };
        constexpr float k_highlightFromDegrees{ 194.4f };
        constexpr float k_highlightToDegrees{ 255.6f };
        constexpr int k_highlightSteps{ 10 };

        constexpr float k_strandWidth{ 0.032f };   // what a strand thins to from its line's width
        constexpr float k_taperLength{ 0.30f };
        // Spans laid at a time, the strands taking turns, so where two cross either may be on top.
        constexpr std::size_t k_spansPerStretch{ 2 };
        constexpr std::size_t k_samplesPerSpan{ 12 };
        // How far a stretch starts back inside the one before, so no seam shows where they meet.
        constexpr float k_seamReach{ k_strandWidth * 0.125f };

        constexpr auto k_skyRun = std::to_array<FloatPoint>({
            { 0.654f, 0.411f }, { 0.732f, 0.410f }, { 0.811f, 0.408f }, { 0.888f, 0.422f },
            { 0.916f, 0.444f }, { 0.916f, 0.470f }, { 0.857f, 0.524f }, { 0.786f, 0.563f },
            { 0.721f, 0.606f }, { 0.689f, 0.676f }, { 0.682f, 0.754f }, { 0.698f, 0.830f },
            { 0.727f, 0.865f }, { 0.753f, 0.864f }, { 0.774f, 0.838f }, { 0.777f, 0.761f },
            { 0.744f, 0.687f }, { 0.689f, 0.633f }, { 0.620f, 0.595f }, { 0.545f, 0.565f },
            { 0.473f, 0.533f }, { 0.408f, 0.485f }, { 0.366f, 0.420f }, { 0.361f, 0.341f },
            { 0.401f, 0.272f }, { 0.471f, 0.239f }, { 0.518f, 0.245f }, { 0.531f, 0.268f },
            { 0.520f, 0.297f }
        });

        constexpr auto k_periwinkleRun = std::to_array<FloatPoint>({
            { 0.670f, 0.546f }, { 0.749f, 0.544f }, { 0.829f, 0.551f }, { 0.882f, 0.596f },
            { 0.885f, 0.645f }, { 0.839f, 0.707f }, { 0.778f, 0.760f }, { 0.735f, 0.825f },
            { 0.713f, 0.901f }, { 0.689f, 0.911f }, { 0.665f, 0.901f }, { 0.637f, 0.828f },
            { 0.580f, 0.784f }, { 0.500f, 0.786f }, { 0.422f, 0.802f }, { 0.401f, 0.786f },
            { 0.399f, 0.760f }, { 0.438f, 0.728f }, { 0.517f, 0.722f }, { 0.567f, 0.769f },
            { 0.568f, 0.848f }, { 0.540f, 0.889f }, { 0.514f, 0.891f }, { 0.495f, 0.872f },
            { 0.501f, 0.795f }, { 0.494f, 0.716f }, { 0.449f, 0.653f }, { 0.376f, 0.627f },
            { 0.296f, 0.638f }, { 0.243f, 0.657f }
        });

        constexpr auto k_amberRun = std::to_array<FloatPoint>({
            { 0.588f, 0.683f }, { 0.669f, 0.681f }, { 0.747f, 0.688f }, { 0.825f, 0.699f },
            { 0.905f, 0.692f }, { 0.924f, 0.671f }, { 0.922f, 0.645f }, { 0.870f, 0.615f },
            { 0.792f, 0.615f }, { 0.723f, 0.654f }, { 0.684f, 0.722f }, { 0.669f, 0.798f },
            { 0.674f, 0.876f }, { 0.693f, 0.897f }, { 0.719f, 0.897f }, { 0.738f, 0.879f },
            { 0.734f, 0.799f }, { 0.718f, 0.720f }, { 0.698f, 0.644f }, { 0.676f, 0.569f },
            { 0.654f, 0.494f }, { 0.639f, 0.418f }, { 0.636f, 0.338f }, { 0.653f, 0.291f },
            { 0.679f, 0.283f }, { 0.701f, 0.297f }, { 0.708f, 0.373f }, { 0.679f, 0.445f },
            { 0.621f, 0.500f }, { 0.574f, 0.521f }
        });

        constexpr auto k_backSkyRun = std::to_array<FloatPoint>({
            { 0.256f, 0.376f }, { 0.175f, 0.374f }, { 0.095f, 0.363f }, { 0.062f, 0.340f },
            { 0.060f, 0.314f }, { 0.078f, 0.295f }, { 0.156f, 0.297f }, { 0.232f, 0.285f },
            { 0.270f, 0.233f }, { 0.264f, 0.154f }, { 0.230f, 0.105f }, { 0.204f, 0.104f },
            { 0.180f, 0.131f }, { 0.153f, 0.204f }, { 0.106f, 0.270f }, { 0.092f, 0.318f },
            { 0.127f, 0.387f }, { 0.158f, 0.461f }, { 0.187f, 0.537f }, { 0.234f, 0.598f },
            { 0.305f, 0.629f }, { 0.382f, 0.617f }, { 0.450f, 0.574f }, { 0.516f, 0.527f },
            { 0.590f, 0.505f }, { 0.663f, 0.529f }, { 0.701f, 0.599f }, { 0.687f, 0.689f }
        });

        // Each starts under the ring where a line of the paper meets the rim, as wide as that line.

        constexpr std::array<Strand, 4> k_strands{
            Strand{ .points = k_skyRun, .color = k_StrandColor1, .startWidth = 0.063f },
            Strand{ .points = k_periwinkleRun, .color = k_StrandColor2, .startWidth = 0.063f },
            Strand{ .points = k_amberRun, .color = k_StrandColor3, .startWidth = 0.063f },
            Strand{ .points = k_backSkyRun, .color = k_StrandColor4, .startWidth = 0.061f }
        };

        [[nodiscard]] FloatPoint mix(FloatPoint from, FloatPoint to, float amount);
        [[nodiscard]] FloatPoint towards(FloatPoint from, FloatPoint to, float distance);
        [[nodiscard]] FloatPoint lensPoint(FloatPoint);
        [[nodiscard]] Corners cornersOf(const Leaf&);
        [[nodiscard]] Corners lineCorners(const Corners& leaf, const Line&);
        [[nodiscard]] Corners tailQuad();
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds);
        [[nodiscard]] float knotSpacing(FloatPoint from, FloatPoint to);
        [[nodiscard]] FloatPoint tangentAt(FloatPoint before, FloatPoint at, FloatPoint after,
            float spanSpacing);
        [[nodiscard]] Cubic spanOf(PointRun points, std::size_t index);
        [[nodiscard]] FloatPoint pointOn(const Cubic&, float amount);
        [[nodiscard]] float widthAt(const Strand&, float along);
        [[nodiscard]] Samples samplesOf(const Strand&);
        [[nodiscard]] std::size_t sampleBack(const Samples&, std::size_t first, float reach);
        [[nodiscard]] std::size_t stretchCount(const Strand&);

        void addRoundedQuad(PathSink&, const Corners&, const CornerRadii& radii);
        void addCircle(PathSink&, FloatPoint center, float radius);
        void addArc(PathSink&, FloatPoint center, float radius, float fromDegrees, float toDegrees,
            int steps);
        void addOutline(PixelPath& path, const Samples&, const Strand&, std::size_t first,
            std::size_t last);

        void fillCircle(Canvas&, const Matrix3x2& transform, FloatPoint center, float radius,
            Color);
        void paintStretch(Canvas&, const Matrix3x2& transform, const Strand&, const Samples&,
            std::size_t stretch);
        void paintStrands(Canvas&, const Matrix3x2& transform);
        void fillLine(Canvas&, const Matrix3x2& transform, const Corners& leaf, const Line&);
        void drawLeafShape(Canvas&, const Matrix3x2& transform, const Leaf&, const PathDrawLayer&);
        void paintPaper(Canvas&, const Matrix3x2& transform);
        void paintGlass(Canvas&, const Matrix3x2& transform);
    }
}

//-----------------------------------------------------------------------------

namespace SeeDocs_App::AppIcon
{
    void paint(Canvas& canvas, const FloatRect& bounds, const Opacity opacity)
    {
        // One layer for the whole mark: its parts overlap, and faded one by one they would show.
        const ScopedCanvasOpacity layer{ canvas, opacity };
        const Matrix3x2 transform = fitTo(bounds);

        paintStrands(canvas, transform);

        // The glass shows paper where the strands run, kept inside the disc by the clip alone.
        PixelPath lens;
        PathSink lensSink{ lens, false };
        addCircle(lensSink, k_glassCenter, k_lensRadius);
        canvas.pushClip(lens, &transform);
        paintPaper(canvas, transform);
        canvas.popClip();

        paintGlass(canvas, transform);
    }

    void paintIcon(PaintIconEvent& event)
    {
        paint(event.canvas(), event.iconRect(), 1.0f - event.disabledAmount());
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
            return {
                from.x + (to.x - from.x) * amount,
                from.y + (to.y - from.y) * amount
            };
        }

        FloatPoint towards(const FloatPoint from, const FloatPoint to, const float distance)
        {
            const float dx = to.x - from.x;
            const float dy = to.y - from.y;
            const float length = std::hypot(dx, dy);
            if (length < k_epsilon)
                return from;

            return {
                from.x + dx / length * distance,
                from.y + dy / length * distance
            };
        }

        FloatPoint lensPoint(const FloatPoint point)
        {
            const float dx = point.x - k_glassCenter.x;
            const float dy = point.y - k_glassCenter.y;
            const float distance = std::hypot(dx, dy);
            if (distance < k_epsilon)
                return point;

            const float reach = std::min(distance / k_lensRadius, 1.0f);
            const float factor = k_magnification - (k_magnification - 1.0f) * reach * reach;
            return {
                k_glassCenter.x + dx * factor,
                k_glassCenter.y + dy * factor
            };
        }

        Corners cornersOf(const Leaf& leaf)
        {
            const Corners base = {
                FloatPoint{ leaf.left, leaf.top },
                FloatPoint{ leaf.right, leaf.top },
                FloatPoint{ leaf.right, leaf.bottom },
                FloatPoint{ leaf.left, leaf.bottom }
            };

            Corners result = {};
            for (std::size_t index = 0; index < base.size(); ++index)
            {
                result[index] = {
                    base[index].x + leaf.offsets[index].x,
                    base[index].y + leaf.offsets[index].y
                };
            }

            return result;
        }

        Corners lineCorners(const Corners& leaf, const Line& line)
        {
            const FloatPoint topLeft = mix(leaf[0], leaf[3], line.from);
            const FloatPoint topRight = mix(leaf[1], leaf[2], line.from);
            const FloatPoint bottomRight = mix(leaf[1], leaf[2], line.to);
            const FloatPoint bottomLeft = mix(leaf[0], leaf[3], line.to);

            return {
                mix(topLeft, topRight, line.inset),
                mix(topRight, topLeft, line.inset),
                mix(bottomRight, bottomLeft, line.inset),
                mix(bottomLeft, bottomRight, line.inset)
            };
        }

        Corners tailQuad()
        {
            const float dx = k_tailEnd.x - k_glassCenter.x;
            const float dy = k_tailEnd.y - k_glassCenter.y;
            const float length = std::hypot(dx, dy);
            const FloatPoint along = { dx / length, dy / length };
            const FloatPoint across = { -along.y, along.x };
            const float half = k_tailWidth * 0.5f;
            // The near end sits inside the ring, which is drawn over the joint.
            const FloatPoint near = {
                k_glassCenter.x + along.x * k_lensRadius,
                k_glassCenter.y + along.y * k_lensRadius
            };

            return {
                FloatPoint{ near.x + across.x * half, near.y + across.y * half },
                FloatPoint{ k_tailEnd.x + across.x * half, k_tailEnd.y + across.y * half },
                FloatPoint{ k_tailEnd.x - across.x * half, k_tailEnd.y - across.y * half },
                FloatPoint{ near.x - across.x * half, near.y - across.y * half }
            };
        }

        Matrix3x2 fitTo(const FloatRect& bounds)
        {
            const float scaleX = bounds.width() / k_markBox.width;
            const float scaleY = bounds.height() / k_markBox.height;
            const FloatPoint origin = bounds.topLeft();

            return {
                scaleX, 0.0f,
                0.0f, scaleY,
                origin.x - k_markBox.left * scaleX,
                origin.y - k_markBox.top * scaleY
            };
        }

        float knotSpacing(const FloatPoint from, const FloatPoint to)
        {
            return std::max(std::sqrt(std::hypot(to.x - from.x, to.y - from.y)), k_epsilon);
        }

        // The spline's tangent at a point, in the units of the span that starts or ends there.
        FloatPoint tangentAt(const FloatPoint before, const FloatPoint at, const FloatPoint after,
            const float spanSpacing)
        {
            const float into = knotSpacing(before, at);
            const float outOf = knotSpacing(at, after);
            const FloatPoint tangent = (at - before) * (1.0f / into)
                - (after - before) * (1.0f / (into + outOf))
                + (after - at) * (1.0f / outOf);

            return tangent * spanSpacing;
        }

        Cubic spanOf(const PointRun points, const std::size_t index)
        {
            const FloatPoint start = points[index];
            const FloatPoint end = points[index + 1];
            // An end has no neighbour beyond it, so it takes the next point mirrored through it.
            const FloatPoint before = index > 0 ? points[index - 1] : start * 2.0f - end;
            const FloatPoint after = index + 2 < points.size() ? points[index + 2] : end * 2.0f - start;
            const float spacing = knotSpacing(start, end);

            return {
                .start = start,
                .control1 = start + tangentAt(before, start, end, spacing) * (1.0f / 3.0f),
                .control2 = end - tangentAt(start, end, after, spacing) * (1.0f / 3.0f),
                .end = end
            };
        }

        FloatPoint pointOn(const Cubic& span, const float amount)
        {
            const float rest = 1.0f - amount;
            const float startShare = rest * rest * rest;
            const float control1Share = 3.0f * rest * rest * amount;
            const float control2Share = 3.0f * rest * amount * amount;
            const float endShare = amount * amount * amount;

            return {
                span.start.x * startShare + span.control1.x * control1Share
                    + span.control2.x * control2Share + span.end.x * endShare,
                span.start.y * startShare + span.control1.y * control1Share
                    + span.control2.y * control2Share + span.end.y * endShare
            };
        }

        float widthAt(const Strand& strand, const float along)
        {
            const float amount = std::clamp(along / k_taperLength, 0.0f, 1.0f);
            const float eased = amount * amount * (3.0f - 2.0f * amount);
            return strand.startWidth + (k_strandWidth - strand.startWidth) * eased;
        }

        Samples samplesOf(const Strand& strand)
        {
            const std::size_t spanCount = strand.points.size() - 1;
            Samples samples;
            samples.reserve(spanCount * k_samplesPerSpan + 1);
            for (std::size_t index = 0; index < spanCount; ++index)
            {
                const Cubic span = spanOf(strand.points, index);
                for (std::size_t step = 0; step < k_samplesPerSpan; ++step)
                {
                    const float amount = static_cast<float>(step) / k_samplesPerSpan;
                    samples.push_back(Sample{ .point = pointOn(span, amount) });
                }
            }
            samples.push_back(Sample{ .point = strand.points.back() });

            // A normal is taken across the sample's two neighbours.
            for (std::size_t index = 0; index < samples.size(); ++index)
            {
                const FloatPoint before = samples[index > 0 ? index - 1 : index].point;
                const FloatPoint after = samples[std::min(index + 1, samples.size() - 1)].point;
                const float length = std::max(std::hypot(after.x - before.x, after.y - before.y), k_epsilon);
                samples[index].normal = { (before.y - after.y) / length, (after.x - before.x) / length };

                if (index > 0)
                {
                    const FloatPoint previous = samples[index - 1].point;
                    const FloatPoint current = samples[index].point;
                    const float step = std::hypot(current.x - previous.x, current.y - previous.y);
                    samples[index].along = samples[index - 1].along + step;
                }
            }

            return samples;
        }

        // The sample the reach given lies back from first, or the strand's start.
        std::size_t sampleBack(const Samples& samples, const std::size_t first, const float reach)
        {
            std::size_t index = first;
            while (index > 0 && samples[index].along > samples[first].along - reach)
                --index;

            return index;
        }

        std::size_t stretchCount(const Strand& strand)
        {
            const std::size_t spanCount = strand.points.size() - 1;
            return (spanCount + k_spansPerStretch - 1) / k_spansPerStretch;
        }

        void addRoundedQuad(PathSink& sink, const Corners& points, const CornerRadii& radii)
        {
            bool started = false;
            for (std::size_t index = 0; index < points.size(); ++index)
            {
                const FloatPoint previous = points[(index + 3) % points.size()];
                const FloatPoint current = points[index];
                const FloatPoint next = points[(index + 1) % points.size()];

                const float toPrevious = std::hypot(current.x - previous.x, current.y - previous.y);
                const float toNext = std::hypot(next.x - current.x, next.y - current.y);
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
                const FloatPoint point = {
                    center.x + std::cos(radians) * radius,
                    center.y + std::sin(radians) * radius
                };
                if (step == 0)
                    sink.moveTo(point);
                else
                    sink.lineTo(point);
            }
        }

        // One side forward and the other back, each at half the width there.
        void addOutline(PixelPath& path, const Samples& samples, const Strand& strand,
            const std::size_t first, const std::size_t last)
        {
            for (std::size_t index = first; index <= last; ++index)
            {
                const Sample& sample = samples[index];
                const float half = widthAt(strand, sample.along) * 0.5f;
                const FloatPoint side = sample.point + sample.normal * half;
                if (index == first)
                    path.moveTo(side);
                else
                    path.lineTo(side);
            }

            for (std::size_t index = last + 1; index-- > first;)
            {
                const Sample& sample = samples[index];
                const float half = widthAt(strand, sample.along) * 0.5f;
                path.lineTo(sample.point - sample.normal * half);
            }

            path.close();
        }

        void fillCircle(Canvas& canvas, const Matrix3x2& transform, const FloatPoint center,
            const float radius, const Color color)
        {
            PixelPath path;
            PathSink sink{ path, false };
            addCircle(sink, center, radius);
            canvas.drawPath(path, { PathDrawLayer::Fill(color) }, &transform);
        }

        void paintStretch(Canvas& canvas, const Matrix3x2& transform, const Strand& strand,
            const Samples& samples, const std::size_t stretch)
        {
            const std::size_t spanCount = strand.points.size() - 1;
            const std::size_t lastSpan = std::min((stretch + 1) * k_spansPerStretch, spanCount);
            const std::size_t first = stretch * k_spansPerStretch * k_samplesPerSpan;
            const std::size_t last = lastSpan * k_samplesPerSpan;

            PixelPath path;
            addOutline(path, samples, strand, sampleBack(samples, first, k_seamReach), last);
            canvas.drawPath(path, { PathDrawLayer::Fill(strand.color) }, &transform);

            if (lastSpan == spanCount)
            {
                const float endRadius = widthAt(strand, samples.back().along) * 0.5f;
                fillCircle(canvas, transform, samples.back().point, endRadius, strand.color);
            }
        }

        void paintStrands(Canvas& canvas, const Matrix3x2& transform)
        {
            std::array<Samples, k_strands.size()> samples;
            std::size_t stretches = 0;
            for (std::size_t index = 0; index < k_strands.size(); ++index)
            {
                samples[index] = samplesOf(k_strands[index]);
                stretches = std::max(stretches, stretchCount(k_strands[index]));
            }

            for (std::size_t stretch = 0; stretch < stretches; ++stretch)
            {
                for (std::size_t index = 0; index < k_strands.size(); ++index)
                {
                    if (stretch < stretchCount(k_strands[index]))
                        paintStretch(canvas, transform, k_strands[index], samples[index], stretch);
                }
            }
        }

        void fillLine(Canvas& canvas, const Matrix3x2& transform, const Corners& leaf,
            const Line& line)
        {
            const CornerRadii radii = { k_lineRadius, k_lineRadius, k_lineRadius, k_lineRadius };
            PixelPath path;
            PathSink sink{ path, true };
            addRoundedQuad(sink, lineCorners(leaf, line), radii);
            canvas.drawPath(path, { PathDrawLayer::Fill(line.color) }, &transform);
        }

        void drawLeafShape(Canvas& canvas, const Matrix3x2& transform, const Leaf& leaf,
            const PathDrawLayer& layer)
        {
            const CornerRadii radii = { leaf.radius, leaf.radius, leaf.radius, leaf.radius };
            PixelPath path;
            PathSink sink{ path, true };
            addRoundedQuad(sink, cornersOf(leaf), radii);
            canvas.drawPath(path, { layer }, &transform);
        }

        void paintPaper(Canvas& canvas, const Matrix3x2& transform)
        {
            const Corners back = cornersOf(k_backLeaf);
            const Corners front = cornersOf(k_frontLeaf);

            // The back leaf's lines go down before the front leaf covers them.
            drawLeafShape(canvas, transform, k_backLeaf, PathDrawLayer::Fill(k_paperBack));
            for (const Line& line : k_backLines)
                fillLine(canvas, transform, back, line);

            drawLeafShape(canvas, transform, k_backLeaf, PathDrawLayer::Stroke(k_paperEdge, k_paperEdgeWidth));
            drawLeafShape(canvas, transform, k_frontLeaf, PathDrawLayer::Fill(k_paperFront));
            drawLeafShape(canvas, transform, k_frontLeaf, PathDrawLayer::Stroke(k_paperEdge, k_paperEdgeWidth));

            for (const Line& line : k_frontLines)
                fillLine(canvas, transform, front, line);
        }

        void paintGlass(Canvas& canvas, const Matrix3x2& transform)
        {
            PixelPath path;
            PathSink sink{ path, false };

            addCircle(sink, k_glassCenter, k_lensRadius - k_rimInset);
            canvas.drawPath(path, { PathDrawLayer::Stroke(k_rimShade, k_rimWidth) }, &transform);

            path.clear();
            addArc(sink, k_glassCenter, k_highlightRadius,
                k_highlightFromDegrees, k_highlightToDegrees, k_highlightSteps);
            canvas.drawPath(path, { PathDrawLayer::Stroke(k_highlight, k_highlightWidth) }, &transform);

            PixelPath ring;
            PathSink ringSink{ ring, false };
            addCircle(ringSink, k_glassCenter, k_glassRadius);

            PixelPath tail;
            PathSink tailSink{ tail, false };
            addRoundedQuad(tailSink, tailQuad(), { 0.0f, k_tailCorner, k_tailCorner, 0.0f });

            // Both edges before either fill, so only the fringe outside ring and tail shows.
            canvas.drawPath(ring,
                { PathDrawLayer::Stroke(k_frameEdgeColor, k_glassWidth + k_frameEdgeWidth * 2.0f) }, &transform);
            canvas.drawPath(tail,
                { PathDrawLayer::Stroke(k_frameEdgeColor, k_frameEdgeWidth * 2.0f) }, &transform);
            canvas.drawPath(tail, { PathDrawLayer::Fill(k_frameColor) }, &transform);
            canvas.drawPath(ring, { PathDrawLayer::Stroke(k_frameColor, k_glassWidth) }, &transform);
        }
    }
}
