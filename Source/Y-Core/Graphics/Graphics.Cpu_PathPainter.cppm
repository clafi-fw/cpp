export module ClaFi.Core.Graphics.Cpu_PathPainter;

import ClaFi.StdLib;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Graphics::Cpu
{
    // How finely a curve is broken into segments.
    export enum class CurveQuality {
        Low,
        Medium,
        High,
        VeryHigh
    };

    export class BakedPixelPath {
    public:
        // Automatically hook into the global scratchpad cache [CP]
        BakedPixelPath() : m_points{ g_rasterBuffers.m_bakedPointsCache } {
            m_points.clear();
        }

        void moveTo(FloatPoint point) { m_points.emplace_back(point, BakedPathCommand::Move); }
        void moveTo(float x, float y) { m_points.emplace_back(x, y, BakedPathCommand::Move); }
        void lineTo(FloatPoint p) { m_points.emplace_back(p, BakedPathCommand::Line); }
        void lineTo(float x, float y) { m_points.emplace_back(x, y, BakedPathCommand::Line); }
        void quadTo(FloatPoint ctrl, FloatPoint end, CurveQuality quality = CurveQuality::Medium, float scaleHint = 1.0f);
        void cubicTo(FloatPoint ctrl1, FloatPoint ctrl2, FloatPoint end, CurveQuality quality = CurveQuality::Medium, float scaleHint = 1.0f);
        void close() { m_closed = true; }

        // keeps m_points capacity alive!
        void clear() { m_points.clear(); m_closed = false; }

        void transform(const Matrix3x2& matrix);
        void bake(const PixelPath& path, CurveQuality quality = CurveQuality::Medium, float scaleHint = 1.0f);

        bool isClosed() const { return m_closed; }
        bool isEmpty() const { return m_points.empty(); }
        const std::vector<BakedPathPoint>& points() const { return m_points; }

    private:
        void flattenQuadraticBezier(FloatPoint start, FloatPoint ctrl, FloatPoint end, float tolSq, int depth);
        void flattenCubicBezier(FloatPoint start, FloatPoint ctrl1, FloatPoint ctrl2, FloatPoint end, float tolSq, int depth);
        FloatPoint currentPoint() const;

        std::vector<BakedPathPoint>& m_points; // Pointer Reference [CP]
        bool m_closed = false;
    };

    export class PixelPathPainter
    {
    public:
        explicit PixelPathPainter(const PixelView& surface) : m_surface{ surface } {}

        void drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform = nullptr, Opacity globalOpacity = 1.0f);
        void drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2& transform, Opacity globalOpacity = 1.0f);

        void drawPath(const PixelPath& path, std::initializer_list<PathDrawLayer> layers, const Matrix3x2* transform = nullptr, Opacity globalOpacity = 1.0f)
        {
            drawPath(path, std::span<const PathDrawLayer>{ layers.begin(), layers.end() }, transform, globalOpacity);
        }

        void drawPath(const PixelPath& path, std::initializer_list<PathDrawLayer> layers, const Matrix3x2& transform, Opacity globalOpacity = 1.0f)
        {
            drawPath(path, std::span<const PathDrawLayer>{ layers.begin(), layers.end() }, & transform, globalOpacity);
        }

        // Single-layer convenience overloads (zero allocation) [CP]
        void drawPath(const PixelPath& path, const PathDrawLayer& layer, const Matrix3x2* transform = nullptr, Opacity globalOpacity = 1.0f)
        {
            drawPath(path, std::span<const PathDrawLayer>{ &layer, 1 }, transform, globalOpacity);
        }

        void drawPath(const PixelPath& path, const PathDrawLayer& layer, const Matrix3x2& transform, Opacity globalOpacity = 1.0f)
        {
            drawPath(path, std::span<const PathDrawLayer>{ &layer, 1 }, & transform, globalOpacity);
        }

        void setClipPath(const PixelPath& path, const Matrix3x2* transform = nullptr);
        void clearClip() { m_rasterBuffers.hasActiveClip = false; }
    private:
        FloatRect prepareWorldPointsAndBounds(const BakedPixelPath& path, const Matrix3x2* transform, float padding);
        void prepareEdges(const std::vector<BakedPathPoint>& pts, bool pathIsClosed, bool forceClose);
        void addEdge(FloatPoint p1, FloatPoint p2);

    private:
        PixelView m_surface;
        RasterBuffers& m_rasterBuffers{ g_rasterBuffers };
    };

    // ========================================================================
    // Implementation: BakedPixelPath
    // ========================================================================

    void BakedPixelPath::cubicTo(FloatPoint ctrl1, FloatPoint ctrl2, FloatPoint end, CurveQuality quality, float scaleHint)
    {
        if (m_points.empty()) moveTo({ 0.0f, 0.0f });
        BakedPathPoint start = m_points.back();

        float tolerance = 0.5f;
        switch (quality) {
        case CurveQuality::Low:      tolerance = 2.0f; break;
        case CurveQuality::Medium:   tolerance = 0.5f; break;
        case CurveQuality::High:     tolerance = 0.1f; break;
        case CurveQuality::VeryHigh: tolerance = 0.01f; break;
        }

        float effectiveScale = std::max(0.001f, std::abs(scaleHint));
        tolerance /= effectiveScale;

        flattenCubicBezier(start.coord, ctrl1, ctrl2, end, tolerance * tolerance, 0);
    }

    void BakedPixelPath::transform(const Matrix3x2& matrix)
    {
        for (BakedPathPoint& pt : m_points)
            pt.coord = matrix.transform(pt.coord);
    }

    void BakedPixelPath::bake(const PixelPath& path, CurveQuality quality, float scaleHint)
    {
        clear();
        FloatPoint cp{ 0.0f, 0.0f };
        FloatPoint subpathStart{ 0.0f, 0.0f };

        for (const auto& cmd : path.commands()) {
            switch (cmd.type) {
            case PathCommandType::MoveTo:
                moveTo(cmd.p1);
                cp = cmd.p1;
                subpathStart = cmd.p1;
                break;
            case PathCommandType::MoveBy: {
                FloatPoint absPt = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                moveTo(absPt);
                cp = absPt;
                subpathStart = absPt;
                break;
            }
            case PathCommandType::LineTo:
                lineTo(cmd.p1);
                cp = cmd.p1;
                break;
            case PathCommandType::LineBy: {
                FloatPoint absPt = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                lineTo(absPt);
                cp = absPt;
                break;
            }
            case PathCommandType::HLineTo: {
                FloatPoint absPt = { cmd.p1.x, cp.y };
                lineTo(absPt);
                cp = absPt;
                break;
            }
            case PathCommandType::HLineBy: {
                FloatPoint absPt = { cp.x + cmd.p1.x, cp.y };
                lineTo(absPt);
                cp = absPt;
                break;
            }
            case PathCommandType::VLineTo: {
                FloatPoint absPt = { cp.x, cmd.p1.y };
                lineTo(absPt);
                cp = absPt;
                break;
            }
            case PathCommandType::VLineBy: {
                FloatPoint absPt = { cp.x, cp.y + cmd.p1.y };
                lineTo(absPt);
                cp = absPt;
                break;
            }
            case PathCommandType::QuadTo:
                quadTo(cmd.p1, cmd.p2, quality, scaleHint);
                cp = cmd.p2;
                break;
            case PathCommandType::QuadBy: {
                FloatPoint ctrl = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                FloatPoint end = { cp.x + cmd.p2.x, cp.y + cmd.p2.y };
                quadTo(ctrl, end, quality, scaleHint);
                cp = end;
                break;
            }
            case PathCommandType::CubicTo:
                cubicTo(cmd.p1, cmd.p2, cmd.p3, quality, scaleHint);
                cp = cmd.p3;
                break;
            case PathCommandType::CubicBy: {
                FloatPoint ctrl1 = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                FloatPoint ctrl2 = { cp.x + cmd.p2.x, cp.y + cmd.p2.y };
                FloatPoint end = { cp.x + cmd.p3.x, cp.y + cmd.p3.y };
                cubicTo(ctrl1, ctrl2, end, quality, scaleHint);
                cp = end;
                break;
            }
            case PathCommandType::Close:
                close();
                cp = subpathStart;
                break;
            }
        }
    }

    struct QuadStackNode {
        FloatPoint start;
        FloatPoint ctrl;
        FloatPoint end;
        int depth;
    };

    void BakedPixelPath::flattenQuadraticBezier(FloatPoint start, FloatPoint ctrl, FloatPoint end, float tolSq, int depth)
    {
        // A stack of 32 is physically impossible to overflow given our depth limit of 12
        QuadStackNode stack[32];
        int stackPtr = 0;

        stack[stackPtr++] = { start, ctrl, end, depth };

        while (stackPtr > 0) {
            QuadStackNode node = stack[--stackPtr];

            if (node.depth > 12) {
                lineTo(node.end);
                continue;
            }

            float dx = node.start.x - 2.0f * node.ctrl.x + node.end.x;
            float dy = node.start.y - 2.0f * node.ctrl.y + node.end.y;
            float distSq = dx * dx + dy * dy;

            if (distSq <= 16.0f * tolSq) {
                lineTo(node.end);
                continue;
            }

            FloatPoint p01 = { (node.start.x + node.ctrl.x) * 0.5f, (node.start.y + node.ctrl.y) * 0.5f };
            FloatPoint p12 = { (node.ctrl.x + node.end.x) * 0.5f, (node.ctrl.y + node.end.y) * 0.5f };
            FloatPoint p012 = { (p01.x + p12.x) * 0.5f, (p01.y + p12.y) * 0.5f };

            // Push the right sub-curve first, then the left sub-curve.
            // This guarantees LIFO processing order, keeping the path segments generated
            // sequentially from left to right (identical to recursive order).
            stack[stackPtr++] = { p012, p12, node.end, node.depth + 1 };
            stack[stackPtr++] = { node.start, p01, p012, node.depth + 1 };
        }
    }

    struct CubicStackNode {
        FloatPoint p0, p1, p2, p3;
        int depth;
    };

    void BakedPixelPath::quadTo(FloatPoint ctrl, FloatPoint end, CurveQuality quality, float scaleHint) {
        if (m_points.empty()) moveTo({ 0,0 });
        BakedPathPoint start = m_points.back();

        float tolerance = 0.5f;
        switch (quality) {
        case CurveQuality::Low:      tolerance = 2.0f; break;
        case CurveQuality::Medium:   tolerance = 0.5f; break;
        case CurveQuality::High:     tolerance = 0.1f; break;
        case CurveQuality::VeryHigh: tolerance = 0.01f; break;
        }

        float effectiveScale = std::max(0.001f, std::abs(scaleHint));
        tolerance /= effectiveScale;

        flattenQuadraticBezier(start.coord, ctrl, end, tolerance * tolerance, 0);
    }

    void BakedPixelPath::flattenCubicBezier(FloatPoint p0, FloatPoint p1, FloatPoint p2, FloatPoint p3, float tolSq, int depth)
    {
        CubicStackNode stack[32];
        int stackPtr = 0;

        stack[stackPtr++] = { p0, p1, p2, p3, depth };

        while (stackPtr > 0) {
            CubicStackNode node = stack[--stackPtr];

            if (node.depth > 12) {
                lineTo(node.p3);
                continue;
            }

            float dx = node.p3.x - node.p0.x;
            float dy = node.p3.y - node.p0.y;
            float d2 = dx * dx + dy * dy;

            bool isFlat = false;
            if (d2 == 0.0f) {
                float d1sq = (node.p1.x - node.p0.x) * (node.p1.x - node.p0.x) + (node.p1.y - node.p0.y) * (node.p1.y - node.p0.y);
                float d2sq = (node.p2.x - node.p0.x) * (node.p2.x - node.p0.x) + (node.p2.y - node.p0.y) * (node.p2.y - node.p0.y);
                if (std::max(d1sq, d2sq) <= tolSq) {
                    isFlat = true;
                }
            }
            else {
                float cross1 = std::abs((node.p1.x - node.p0.x) * dy - (node.p1.y - node.p0.y) * dx);
                float cross2 = std::abs((node.p2.x - node.p0.x) * dy - (node.p2.y - node.p0.y) * dx);
                float maxCross = std::max(cross1, cross2);

                if (maxCross * maxCross <= tolSq * d2) {
                    isFlat = true;
                }
            }

            if (isFlat) {
                lineTo(node.p3);
                continue;
            }

            FloatPoint p01 = { (node.p0.x + node.p1.x) * 0.5f, (node.p0.y + node.p1.y) * 0.5f };
            FloatPoint p12 = { (node.p1.x + node.p2.x) * 0.5f, (node.p1.y + node.p2.y) * 0.5f };
            FloatPoint p23 = { (node.p2.x + node.p3.x) * 0.5f, (node.p2.y + node.p3.y) * 0.5f };

            FloatPoint p012 = { (p01.x + p12.x) * 0.5f, (p01.y + p12.y) * 0.5f };
            FloatPoint p123 = { (p12.x + p23.x) * 0.5f, (p12.y + p23.y) * 0.5f };

            FloatPoint p0123 = { (p012.x + p123.x) * 0.5f, (p012.y + p123.y) * 0.5f };

            // Push right then left (LIFO sequence)
            stack[stackPtr++] = { p0123, p123, p23, node.p3, node.depth + 1 };
            stack[stackPtr++] = { node.p0, p01, p012, p0123, node.depth + 1 };
        }
    }

    FloatPoint BakedPixelPath::currentPoint() const
    {
        return m_points.empty() ? FloatPoint{ 0.0f, 0.0f } : m_points.back().coord;
    }


    // --- Implementation of PixelPathPainter ---

    void PixelPathPainter::drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform, Opacity globalOpacity)
    {
        if (path.isEmpty() || layers.empty()) return;

        float scaleFactor = transform ? transform->getScaleFactor() : 1.0f;
        BakedPixelPath bakedPath;
        bakedPath.bake(path, CurveQuality::Medium, scaleFactor);

        float maxPadding = 0.5f;
        for (const auto& layer : layers) {
            if (layer.geometry.mode == PathRenderMode::Stroke) {
                maxPadding = std::max(maxPadding, layer.geometry.strokeWidth * scaleFactor * 0.5f);
            }
            else if (layer.geometry.mode == PathRenderMode::OuterGlow || layer.geometry.mode == PathRenderMode::Shadow) {
                maxPadding = std::max(maxPadding, layer.geometry.strokeWidth * scaleFactor);
            }
            if (const auto* glow = std::get_if<PointGlow>(&layer.brush)) {
                maxPadding = std::max(maxPadding, glow->lightSpread * scaleFactor);
            }
        }

        FloatRect bounds = prepareWorldPointsAndBounds(bakedPath, transform, maxPadding);
        if (bounds.empty()) return;

        PixelView effectSurface = m_surface.subView(bounds);
        int bWidth = static_cast<int>(std::ceil(bounds.right)) - static_cast<int>(std::floor(bounds.left));
        int bHeight = static_cast<int>(std::ceil(bounds.bottom)) - static_cast<int>(std::floor(bounds.top));

        for (const auto& layer : layers)
        {
            if (layer.geometry.mode == PathRenderMode::Fill)
            {
                // Fills completely overwrite the mask. Bypasses standard clearing entirely!
                m_rasterBuffers.prepareForEffect(bWidth, bHeight, false);
                prepareEdges(m_rasterBuffers.m_worldPoints, bakedPath.isClosed(), true);
                m_rasterBuffers.prepareEdgeBuckets(bounds, maxPadding);
                m_rasterBuffers.rasterizeFill(bounds, 0.0f);
            }
            else if (layer.geometry.mode == PathRenderMode::Stroke)
            {
                // Stroke accumulation requires a zero-cleared mask
                m_rasterBuffers.prepareForEffect(bWidth, bHeight, true);
                prepareEdges(m_rasterBuffers.m_worldPoints, bakedPath.isClosed(), false);
                m_rasterBuffers.prepareEdgeBuckets(bounds, maxPadding);
                float radius = (layer.geometry.strokeWidth * scaleFactor) * 0.5f;
                m_rasterBuffers.rasterizeStrokeMask(bounds, radius, layer.geometry.strokeCap);
            }
            else if (layer.geometry.mode == PathRenderMode::OuterGlow)
            {
                // Glow accumulation requires a zero-cleared mask
                m_rasterBuffers.prepareForEffect(bWidth, bHeight, true);
                prepareEdges(m_rasterBuffers.m_worldPoints, bakedPath.isClosed(), false);
                m_rasterBuffers.prepareEdgeBuckets(bounds, maxPadding);
                float radius = layer.geometry.strokeWidth * scaleFactor;
                m_rasterBuffers.rasterizeGlowMask(bounds, radius, layer.geometry.strokeCap, layer.geometry.falloff);
            }
            else if (layer.geometry.mode == PathRenderMode::Shadow)
            {
                // The fill writes every pixel of the mask, and the glow keeps the greater of what
                // it finds and its own ramp: solid inside, the ramp outside, the shape's own edge
                // between them. One set of edges serves both, closed, since a shadow is of what
                // the shape covers.
                m_rasterBuffers.prepareForEffect(bWidth, bHeight, false);
                prepareEdges(m_rasterBuffers.m_worldPoints, bakedPath.isClosed(), true);
                m_rasterBuffers.prepareEdgeBuckets(bounds, maxPadding);
                m_rasterBuffers.rasterizeFill(bounds, 0.0f);
                float radius = layer.geometry.strokeWidth * scaleFactor;
                m_rasterBuffers.rasterizeGlowMask(bounds, radius, StrokeCap::Round, layer.geometry.falloff);
            }

            // The glow's spread follows this painter's own scale factor rather than the
            // transform's, which is why it is passed separately.
            m_rasterBuffers.composite(effectSurface, layer.brush, globalOpacity, transform, scaleFactor);
        }
    }

    void PixelPathPainter::drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2& transform, Opacity globalOpacity)
    {
        drawPath(path, layers, &transform, globalOpacity);
    }

    void PixelPathPainter::setClipPath(const PixelPath& path, const Matrix3x2* transform)
    {
        BakedPixelPath bakedPath;
        float scaleFactor = transform ? transform->getScaleFactor() : 1.0f;
        bakedPath.bake(path, CurveQuality::Medium, scaleFactor);

        FloatRect bounds = prepareWorldPointsAndBounds(bakedPath, transform, 0.5f);
        if (bounds.empty()) return;
        int bWidth = static_cast<int>(std::ceil(bounds.right)) - static_cast<int>(std::floor(bounds.left));
        int bHeight = static_cast<int>(std::ceil(bounds.bottom)) - static_cast<int>(std::floor(bounds.top));

        prepareEdges(m_rasterBuffers.m_worldPoints, bakedPath.isClosed(), true);
        // Clipping masks are fills. Skip clearing!
        m_rasterBuffers.prepareForEffect(bWidth, bHeight, false);
        m_rasterBuffers.m_clipStride = m_rasterBuffers.m_maskStride;

        m_rasterBuffers.clipMask.resize(m_rasterBuffers.mask.size()); // Resize with no-init
        std::swap(m_rasterBuffers.mask, m_rasterBuffers.clipMask);
        m_rasterBuffers.prepareEdgeBuckets(bounds);
        m_rasterBuffers.rasterizeFill(bounds, 0.0f);
        std::swap(m_rasterBuffers.mask, m_rasterBuffers.clipMask);

        m_rasterBuffers.hasActiveClip = true;
        m_rasterBuffers.clipBox = bounds;
    }

    FloatRect PixelPathPainter::prepareWorldPointsAndBounds(const BakedPixelPath& path, const Matrix3x2* transform, float padding)
    {
        m_rasterBuffers.m_worldPoints.clear();
        m_rasterBuffers.m_worldPoints.reserve(path.points().size());

        float minX = 1e10f, maxX = -1e10f, minY = 1e10f, maxY = -1e10f;
        for (const auto& p : path.points()) {
            FloatPoint pt = transform ? transform->transform(p.coord) : p.coord;
            m_rasterBuffers.m_worldPoints.emplace_back(pt, p.command);
            minX = std::min(minX, pt.x); maxX = std::max(maxX, pt.x);
            minY = std::min(minY, pt.y); maxY = std::max(maxY, pt.y);
        }

        FloatRect result = { minX - padding, minY - padding, maxX + padding, maxY + padding };
        result.intersectWith(m_surface.clippedBounds());
        return result;
    }

    void PixelPathPainter::prepareEdges(const std::vector<BakedPathPoint>& pts, bool pathIsClosed, bool forceClose)
    {
        m_rasterBuffers.m_edgeCache.clear();
        if (pts.size() < 2) return;

        std::size_t subPathStart = 0;
        bool hasEdges = false;

        for (std::size_t i = 1; i < pts.size(); ++i) {
            if (pts[i].command == BakedPathCommand::Move) {
                if (hasEdges) {
                    if (forceClose || pathIsClosed) addEdge(pts[i - 1].coord, pts[subPathStart].coord);
                    else m_rasterBuffers.m_edgeCache.back().isEndCap = true;
                }
                subPathStart = i;
                hasEdges = false;
                continue;
            }

            std::size_t oldSize = m_rasterBuffers.m_edgeCache.size();
            addEdge(pts[i - 1].coord, pts[i].coord);
            if (m_rasterBuffers.m_edgeCache.size() > oldSize) {
                if (!hasEdges) m_rasterBuffers.m_edgeCache.back().isStartCap = !(forceClose || pathIsClosed);
                hasEdges = true;
            }
        }

        if (hasEdges) {
            if (forceClose || pathIsClosed) addEdge(pts.back().coord, pts[subPathStart].coord);
            else m_rasterBuffers.m_edgeCache.back().isEndCap = true;
        }
    }

    void PixelPathPainter::addEdge(FloatPoint p1, FloatPoint p2)
    {
        if (p1 == p2) return;
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float lenSq = dx * dx + dy * dy;
        float len = std::sqrt(lenSq);
        m_rasterBuffers.m_edgeCache.push_back({
            p1, p2, dx, dy,
            (std::abs(dy) > 1e-6f) ? (1.0f / dy) : 0.0f,
            (lenSq > 0.0f) ? (1.0f / lenSq) : 0.0f,
            (len > 0.0f) ? (1.0f / len) : 0.0f,
            len,
            std::min(p1.y, p2.y), std::max(p1.y, p2.y),
            false, false
        });
    }

}
