export module ClaFi.Core.Graphics.Cpu_Rasterizer;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    struct RenderEdge {
        FloatPoint p1, p2;
        float dx, dy;
        float invDy;
        float invLenSq;
        float invLen;
        float len;
        float minY, maxY;
        bool isStartCap = false;
        bool isEndCap = false;
    };

    using EdgeIndices = std::vector<std::uint32_t>;

    export class RasterBuffers
    {
    public:
        void prepareForEffect(int width, int height, bool clearMask = true);
        void prepareEdgeBuckets(const FloatRect& bounds, float padding = 1.0f);
        void rasterizeFill(const FloatRect& bounds, float outset);
        void rasterizeStrokeSegment(const FloatRect& totalBox, const RenderEdge& edge, float radius, StrokeCap cap);
        void rasterizeStrokeMask(const FloatRect& bounds, float radius, StrokeCap cap);
        // `falloff` shapes the ramp from the shape's edge out to `radius`. The curve is the one
        // Graphics::falloffAt states; this computes it eight pixels at a time and so cannot call
        // it, which is why that function carries the note that the two move together.
        void rasterizeGlowMask(const FloatRect& bounds, float radius, StrokeCap cap, GlowFalloff falloff);
    public:
        // The one way to composite a brush. A painter hands the brush and the parameters below
        // rather than visiting the Brush itself, so the compositing rules are stated once.
        //
        // brushTransform places a gradient's endpoints and a glow's light position into the
        // target's coordinates; null leaves them as written. glowSpreadScale scales a glow's
        // radius, which is not always the transform's own scale - PixelPathPainter carries a
        // separate one.
        //
        // useOwnMask chooses between compositing through this instance's coverage mask and
        // writing every pixel of the target. A solid colour has no mask-free form, because
        // compositeSolid reads the mask directly, so a solid brush with useOwnMask false paints
        // nothing - which is what RectPainter's mask-free route did by carrying no solid branch.
        void composite(
            const PixelView& target,
            const Brush& brush,
            Opacity opacity,
            const Matrix3x2* brushTransform = nullptr,
            float glowSpreadScale = 1.0f,
            bool useOwnMask = true
        );
    public:
        NoAllocFloatVector mask;
        NoAllocFloatVector clipMask; // Backing allocation for setClipPath rasterization

        // Zero-Copy Pointer Optimization: Reference active mask via direct memory pointer [CP]
        const float* activeClipMaskPtr = nullptr;

        int m_clipStride{ 0 };
        bool hasActiveClip = false;
        FloatRect clipBox;
        std::vector<RenderEdge> m_edgeCache;
        int m_maskStride{ 0 };
        std::vector<BakedPathPoint> m_worldPoints;
        std::vector<BakedPathPoint> m_bakedPointsCache;
    private:
        void compositeSolid(const PixelView&, Color color, float opacity);
        static void buildGradientRamp(std::span<const GradientStop> stops, ColorAsUint* ramp);

        static void compositeGradient(
            const PixelView& target,
            FloatPoint startPoint,
            FloatPoint endPoint,
            std::span<const GradientStop> stops,
            Opacity opacity,
            const float* maskBase = nullptr,
            int maskStride = 0
        );
        static void compositePointGlow(
            const PixelView&,
            const PointGlow& p,
            const float* maskBase = nullptr,
            int maskStride = 0
        );
    private:
        static FloatRect calculateEdgeBounds(const RenderEdge& edge, float radius, FloatRect bounds);
        const EdgeIndices& edgeIndices(int y, int topOffset);
    private:
        // The ramp carries one entry per 1/256th of the gradient axis, plus the endpoint, so the
        // 0..256 position the loop already computes indexes it without a clamp.
        static constexpr int k_gradientRampLast = 256;
        // One bucket per scanline of the last prepareEdgeBuckets() bounds, holding the indices
        // into m_edgeCache whose padded vertical extent covers that scanline.
        std::vector<EdgeIndices> m_edgeBuckets;
    };

    export RasterBuffers g_rasterBuffers{};

    alignas(32) constexpr int g_maskTable2[16] = {
        -1, -1, -1, -1, -1, -1, -1, -1,
         0,  0,  0,  0,  0,  0,  0,  0
    };
    export const int* g_maskTableMiddlePtr = g_maskTable2 + 8;
}
