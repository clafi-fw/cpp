module;
#include <immintrin.h>
module ClaFi.Core.Graphics.Cpu_Rasterizer;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{


    struct SimdEnv
    {
        const __m256 vZero = _mm256_setzero_ps();
        const __m256 vOne = _mm256_set1_ps(1.0f);
        const __m256 v05 = _mm256_set1_ps(0.5f);
        const __m256 v256 = _mm256_set1_ps(256.0f);
        const __m256 vSteps = _mm256_setr_ps(0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f);
        const __m256i vZeroi = _mm256_setzero_si256();

        [[nodiscard]] __m256i loadMask(int remaining) const;
    };

    // Holds precalculated row invariants for distance field calculations
    struct RowSdfEdge
    {
        __m256 vP1x;
        __m256 vDx;
        __m256 vDy;
        __m256 vDyDy;     // Precalculated dy * e.dy as a broadcasted vector
        __m256 vDyScalar; // Precalculated dy as a broadcasted vector
        __m256 vInvLenSq;
    };

    // Number of active edges a single scanline can carry. Edges past this cap are ignored
    // for that row, so the bound has to exceed the worst-case crossing count of any real path.
    constexpr int k_maxRowEdges = 512;

    [[nodiscard]] static __m256 calcEdgeDistSq(__m256 vPx, __m256 vPy, const RenderEdge& e)
    {
        const __m256 vZero = _mm256_setzero_ps();
        const __m256 vOne = _mm256_set1_ps(1.0f);

        __m256 vP1x = _mm256_set1_ps(e.p1.x);
        __m256 vP1y = _mm256_set1_ps(e.p1.y);
        __m256 vDx = _mm256_set1_ps(e.dx);
        __m256 vDy = _mm256_set1_ps(e.dy);

        __m256 dx = _mm256_sub_ps(vPx, vP1x);
        __m256 dy = _mm256_sub_ps(vPy, vP1y);

        __m256 t = _mm256_min_ps(vOne, _mm256_max_ps(vZero,
            _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(dx, vDx), _mm256_mul_ps(dy, vDy)), _mm256_set1_ps(e.invLenSq))));

        __m256 offsetX = _mm256_sub_ps(dx, _mm256_mul_ps(t, vDx));
        __m256 offsetY = _mm256_sub_ps(dy, _mm256_mul_ps(t, vDy));

        return _mm256_add_ps(_mm256_mul_ps(offsetX, offsetX), _mm256_mul_ps(offsetY, offsetY));
    }

    [[nodiscard]] static __m256 calcStrokeEdgeDistSq(__m256 vPx, __m256 vPy, const RenderEdge& e, StrokeCap cap, float radius)
    {
        const __m256 vZero = _mm256_setzero_ps();
        const __m256 vOne = _mm256_set1_ps(1.0f);

        __m256 vDx = _mm256_set1_ps(e.dx);
        __m256 vDy = _mm256_set1_ps(e.dy);
        __m256 dx = _mm256_sub_ps(vPx, _mm256_set1_ps(e.p1.x));
        __m256 dy = _mm256_sub_ps(vPy, _mm256_set1_ps(e.p1.y));

        __m256 dot = _mm256_add_ps(_mm256_mul_ps(dx, vDx), _mm256_mul_ps(dy, vDy));
        __m256 tUnclamped = _mm256_mul_ps(dot, _mm256_set1_ps(e.invLenSq));
        __m256 tClamped = _mm256_min_ps(vOne, _mm256_max_ps(vZero, tUnclamped));

        __m256 clampedX = _mm256_sub_ps(dx, _mm256_mul_ps(tClamped, vDx));
        __m256 clampedY = _mm256_sub_ps(dy, _mm256_mul_ps(tClamped, vDy));
        __m256 finalDistSq = _mm256_add_ps(_mm256_mul_ps(clampedX, clampedX), _mm256_mul_ps(clampedY, clampedY));

        if (e.isStartCap)
        {
            __m256 startMask = _mm256_cmp_ps(vZero, tUnclamped, 30);
            if (_mm256_movemask_ps(startMask) != 0)
            {
                __m256 perpX = _mm256_sub_ps(dx, _mm256_mul_ps(tUnclamped, vDx));
                __m256 perpY = _mm256_sub_ps(dy, _mm256_mul_ps(tUnclamped, vDy));
                __m256 perpDistSq = _mm256_add_ps(_mm256_mul_ps(perpX, perpX), _mm256_mul_ps(perpY, perpY));

                __m256 perpDist = _mm256_sqrt_ps(perpDistSq);
                __m256 longDist = _mm256_mul_ps(_mm256_sub_ps(vZero, dot), _mm256_set1_ps(e.invLen));
                if (cap == StrokeCap::Square)
                {
                    longDist = _mm256_sub_ps(longDist, _mm256_set1_ps(radius));
                }

                __m256 dist = _mm256_max_ps(perpDist, longDist);
                __m256 distSq = _mm256_mul_ps(dist, dist);
                finalDistSq = _mm256_blendv_ps(finalDistSq, distSq, startMask);
            }
        }

        if (e.isEndCap)
        {
            __m256 endMask = _mm256_cmp_ps(tUnclamped, vOne, 30);
            if (_mm256_movemask_ps(endMask) != 0)
            {
                __m256 perpX = _mm256_sub_ps(dx, _mm256_mul_ps(tUnclamped, vDx));
                __m256 perpY = _mm256_sub_ps(dy, _mm256_mul_ps(tUnclamped, vDy));
                __m256 perpDistSq = _mm256_add_ps(_mm256_mul_ps(perpX, perpX), _mm256_mul_ps(perpY, perpY));

                __m256 perpDist = _mm256_sqrt_ps(perpDistSq);
                __m256 longDist = _mm256_sub_ps(_mm256_mul_ps(dot, _mm256_set1_ps(e.invLen)), _mm256_set1_ps(e.len));
                if (cap == StrokeCap::Square)
                {
                    longDist = _mm256_sub_ps(longDist, _mm256_set1_ps(radius));
                }

                __m256 dist = _mm256_max_ps(perpDist, longDist);
                __m256 distSq = _mm256_mul_ps(dist, dist);
                finalDistSq = _mm256_blendv_ps(finalDistSq, distSq, endMask);
            }
        }

        return finalDistSq;
    }

    [[nodiscard]] static __m256 calcSdfAlpha(__m256 minEdgeDistSq, __m256i vInsideMask, float outset, const SimdEnv& env)
    {
        __m256 dist = _mm256_sqrt_ps(minEdgeDistSq);
        __m256 signedDist = _mm256_blendv_ps(_mm256_sub_ps(env.vZero, dist), dist, _mm256_castsi256_ps(vInsideMask));
        return _mm256_min_ps(env.vOne, _mm256_max_ps(env.vZero, _mm256_add_ps(signedDist, _mm256_set1_ps(0.5f + outset))));
    }

    static void blendAndStoreBlock(
        Color* dest,
        __m256i vLoadMask,
        __m256i vSrcColor,
        __m256i vAlphaI,
        const SimdEnv& env,
        bool useSolidFastPath = false)
    {
        if (_mm256_movemask_epi8(_mm256_cmpeq_epi32(vAlphaI, env.vZeroi)) == 0xFFFFFFFF)
        {
            return;
        }

        if (useSolidFastPath && _mm256_movemask_epi8(_mm256_cmpgt_epi32(vAlphaI, _mm256_set1_epi32(254))) == 0xFFFFFFFF)
        {
            _mm256_maskstore_epi32(reinterpret_cast<int*>(dest), vLoadMask, vSrcColor);
            return;
        }

        __m256i vDest = _mm256_maskload_epi32(reinterpret_cast<int*>(dest), vLoadMask);
        _mm256_maskstore_epi32(reinterpret_cast<int*>(dest), vLoadMask, Color::blend8(vDest, vSrcColor, vAlphaI));
    }

    [[nodiscard]] static __m256 calcGlowDist(__m256 vDx, __m256 vDyNorm, __m256 vDySq, __m256 vDyQuad, GlowShape shape)
    {
        if (shape == GlowShape::Circle)
        {
            __m256 square = _mm256_add_ps(_mm256_mul_ps(vDx, vDx), vDySq);
            return _mm256_mul_ps(square, _mm256_rsqrt_ps(square));
        }

        if (shape == GlowShape::SoftRectangle)
        {
            __m256 vDxSq = _mm256_mul_ps(vDx, vDx);
            __m256 square = _mm256_add_ps(_mm256_mul_ps(vDxSq, vDxSq), vDyQuad);
            square = _mm256_mul_ps(square, _mm256_rsqrt_ps(square));
            square = _mm256_sqrt_ps(square);
            return _mm256_mul_ps(square, _mm256_rsqrt_ps(square));
        }

        return _mm256_max_ps(vDx, vDyNorm);
    }

    __m256i SimdEnv::loadMask(int remaining) const
    {
        if (remaining >= 8)
        {
            return _mm256_set1_epi32(-1);
        }
        return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g_maskTableMiddlePtr - remaining));
    }

    void RasterBuffers::prepareForEffect(int width, int height, bool clearMask)
    {
        m_maskStride = (width + 7) & ~7;
        std::size_t needed = static_cast<std::size_t>(m_maskStride) * height + 8;
        if (mask.size() < needed)
        {
            // NoAllocFloatVector skips value-initialisation, so the grown tail is indeterminate
            // until either the clearMask fill below or a full-coverage rasterize pass writes it.
            mask.resize(needed);
        }

        if (clearMask)
        {
            std::fill(mask.begin(), mask.begin() + needed, 0.0f);
        }
    }

    void RasterBuffers::prepareEdgeBuckets(const FloatRect& bounds, float padding)
    {
        int top = static_cast<int>(std::floor(bounds.top));
        int bottom = static_cast<int>(std::ceil(bounds.bottom));
        int height = bottom - top;

        // An inverted rect would otherwise reach resize() with a negative count, which converts
        // to a size_t large enough to throw.
        if (height < 0)
        {
            return;
        }

        if (m_edgeBuckets.size() < static_cast<std::size_t>(height) + 1)
        {
            m_edgeBuckets.resize(static_cast<std::size_t>(height) + 1);
        }

        for (int i = 0; i <= height; ++i)
        {
            m_edgeBuckets[i].clear();
        }

        for (std::uint32_t edgeIndex = 0; edgeIndex < m_edgeCache.size(); ++edgeIndex)
        {
            const RenderEdge& edge = m_edgeCache[edgeIndex];

            int startY = static_cast<int>(std::floor(edge.minY - padding)) - top;
            int endY = static_cast<int>(std::ceil(edge.maxY + padding));

            startY = std::max(startY, 0);
            endY = std::min(endY, bottom) - top;

            for (int y = startY; y < endY; ++y)
            {
                if (y <= height)
                {
                    m_edgeBuckets[y].push_back(edgeIndex);
                }
            }
        }
    }

    void RasterBuffers::rasterizeFill(const FloatRect& bounds, float outset)
    {
        SimdEnv env;
        int stride = m_maskStride;

        int startY = static_cast<int>(std::floor(bounds.top));
        int endY = static_cast<int>(std::ceil(bounds.bottom));
        int startX = static_cast<int>(std::floor(bounds.left));
        int endX = static_cast<int>(std::ceil(bounds.right));
        int width = endX - startX;

        // Stack-allocated row invariants, aligned to the AVX 32-byte boundary.
        alignas(32) __m256 crossingXInts[k_maxRowEdges];
        alignas(32) RowSdfEdge preppedEdges[k_maxRowEdges];

        for (int y = startY; y < endY; ++y)
        {
            float py = static_cast<float>(y) + 0.5f;
            float* maskRow = &mask[(y - startY) * stride];
            const EdgeIndices& activeEdges = edgeIndices(y, startY);

            int numCrossing = 0;
            int numSdf = 0;

            // 1. Pre-pass: hoist every row invariant out of the horizontal loop.
            for (std::uint32_t idx : activeEdges)
            {
                const RenderEdge& e = m_edgeCache[idx];

                // Crossing check and xInt calculation, once per row per edge.
                if (py >= e.minY && py < e.maxY)
                {
                    if (numCrossing < k_maxRowEdges)
                    {
                        float xInt = e.p1.x + (py - e.p1.y) * e.dx * e.invDy;
                        crossingXInts[numCrossing++] = _mm256_set1_ps(xInt);
                    }
                }

                // Pre-broadcast distance parameters, once per row per edge.
                if (numSdf < k_maxRowEdges)
                {
                    float dy = py - e.p1.y;
                    RowSdfEdge& prepped = preppedEdges[numSdf++];
                    prepped.vP1x = _mm256_set1_ps(e.p1.x);
                    prepped.vDx = _mm256_set1_ps(e.dx);
                    prepped.vDy = _mm256_set1_ps(e.dy);
                    prepped.vDyDy = _mm256_set1_ps(dy * e.dy);
                    prepped.vDyScalar = _mm256_set1_ps(dy);
                    prepped.vInvLenSq = _mm256_set1_ps(e.invLenSq);
                }
            }

            // 2. Horizontal loop: no branches, no divisions.
            for (int xOffset = 0; xOffset < width; xOffset += 8)
            {
                __m256 vPx = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(startX + xOffset)), env.vSteps);
                __m256i vInsideMask = env.vZeroi;
                __m256 minEdgeDistSq = _mm256_set1_ps(1e10f);

                // Inside/outside winding rule, using the pre-broadcast xInt values.
                for (int i = 0; i < numCrossing; ++i)
                {
                    vInsideMask = _mm256_xor_si256(
                        vInsideMask,
                        _mm256_castps_si256(_mm256_cmp_ps(vPx, crossingXInts[i], 30))
                    );
                }

                // Distance field, using the pre-broadcast row constants.
                for (int i = 0; i < numSdf; ++i)
                {
                    const RowSdfEdge& prepped = preppedEdges[i];

                    __m256 dx = _mm256_sub_ps(vPx, prepped.vP1x);

                    // t = clamp((dx * vDx + vDyDy) * vInvLenSq, 0.0, 1.0)
                    __m256 t = _mm256_min_ps(env.vOne, _mm256_max_ps(env.vZero,
                        _mm256_mul_ps(
                            _mm256_add_ps(_mm256_mul_ps(dx, prepped.vDx), prepped.vDyDy),
                            prepped.vInvLenSq
                        )
                    ));

                    __m256 offsetX = _mm256_sub_ps(dx, _mm256_mul_ps(t, prepped.vDx));
                    __m256 offsetY = _mm256_sub_ps(prepped.vDyScalar, _mm256_mul_ps(t, prepped.vDy));

                    __m256 distSq = _mm256_add_ps(_mm256_mul_ps(offsetX, offsetX), _mm256_mul_ps(offsetY, offsetY));
                    minEdgeDistSq = _mm256_min_ps(minEdgeDistSq, distSq);
                }

                __m256 alpha = calcSdfAlpha(minEdgeDistSq, vInsideMask, outset, env);

                int remaining = width - xOffset;
                if (remaining >= 8)
                {
                    _mm256_storeu_ps(maskRow + xOffset, alpha);
                }
                else
                {
                    _mm256_maskstore_ps(maskRow + xOffset, env.loadMask(remaining), alpha);
                }
            }
        }
    }

    void RasterBuffers::rasterizeStrokeSegment(const FloatRect& totalBox, const RenderEdge& edge, float radius, StrokeCap cap)
    {
        SimdEnv env;
        const __m256 vRadius = _mm256_set1_ps(radius);

        FloatRect segBox = calculateEdgeBounds(edge, radius, totalBox);
        if (segBox.empty())
        {
            return;
        }

        int startY = static_cast<int>(std::floor(segBox.top));
        int endY = static_cast<int>(std::ceil(segBox.bottom));
        int startX = static_cast<int>(std::floor(segBox.left));
        int endX = static_cast<int>(std::ceil(segBox.right));

        int totalTop = static_cast<int>(std::floor(totalBox.top));
        int totalLeft = static_cast<int>(std::floor(totalBox.left));

        bool isStandard = (cap == StrokeCap::Round) || (!edge.isStartCap && !edge.isEndCap);

        for (int y = startY; y < endY; ++y)
        {
            __m256 vPy = _mm256_set1_ps(static_cast<float>(y) + 0.5f);
            float* maskPtr = &mask[(y - totalTop) * m_maskStride + (startX - totalLeft)];

            for (int x = startX; x < endX; x += 8)
            {
                __m256i vLoadMask = env.loadMask(endX - x);
                __m256 vPx = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(x)), env.vSteps);

                __m256 distSq = isStandard ? calcEdgeDistSq(vPx, vPy, edge) : calcStrokeEdgeDistSq(vPx, vPy, edge, cap, radius);
                __m256 newAlpha = _mm256_min_ps(env.vOne, _mm256_max_ps(env.vZero,
                    _mm256_add_ps(_mm256_sub_ps(vRadius, _mm256_sqrt_ps(distSq)), env.v05)));

                _mm256_maskstore_ps(maskPtr, vLoadMask, _mm256_max_ps(_mm256_maskload_ps(maskPtr, vLoadMask), newAlpha));
                maskPtr += 8;
            }
        }
    }

    void RasterBuffers::rasterizeStrokeMask(const FloatRect& bounds, float radius, StrokeCap cap)
    {
        for (const RenderEdge& edge : m_edgeCache)
        {
            rasterizeStrokeSegment(bounds, edge, radius, cap);
        }
    }

    void RasterBuffers::rasterizeGlowMask(const FloatRect& bounds, float radius, StrokeCap cap, GlowFalloff falloff)
    {
        if (radius <= 0.0f)
        {
            return;
        }

        SimdEnv env;
        const __m256 vInvRadius = _mm256_set1_ps(1.0f / radius);
        const bool squared = falloff == GlowFalloff::Smooth;

        int totalTop = static_cast<int>(std::floor(bounds.top));
        int totalLeft = static_cast<int>(std::floor(bounds.left));

        for (const RenderEdge& edge : m_edgeCache)
        {
            FloatRect segBox = calculateEdgeBounds(edge, radius, bounds);
            if (segBox.empty())
            {
                continue;
            }

            int startY = static_cast<int>(std::floor(segBox.top));
            int endY = static_cast<int>(std::ceil(segBox.bottom));
            int startX = static_cast<int>(std::floor(segBox.left));
            int endX = static_cast<int>(std::ceil(segBox.right));

            bool isStandard = (cap == StrokeCap::Round) || (!edge.isStartCap && !edge.isEndCap);

            for (int y = startY; y < endY; ++y)
            {
                __m256 vPy = _mm256_set1_ps(static_cast<float>(y) + 0.5f);
                float* maskPtr = &mask[(y - totalTop) * m_maskStride + (startX - totalLeft)];

                for (int x = startX; x < endX; x += 8)
                {
                    __m256i vLoadMask = env.loadMask(endX - x);
                    __m256 vPx = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(x)), env.vSteps);

                    __m256 distSq = isStandard ? calcEdgeDistSq(vPx, vPy, edge) : calcStrokeEdgeDistSq(vPx, vPy, edge, cap, 0.0f);
                    __m256 dist = _mm256_sqrt_ps(distSq);
                    __m256 glowAlpha = _mm256_max_ps(env.vZero, _mm256_sub_ps(env.vOne, _mm256_mul_ps(dist, vInvRadius)));
                    // Squaring the linear ramp is the whole of the smooth curve: steep where the
                    // shape is and level where it ends, at the cost of one multiply. The branch
                    // is on a value that holds for the entire mask, so it predicts perfectly and
                    // costs nothing the ramp does not already spend.
                    if (squared)
                    {
                        glowAlpha = _mm256_mul_ps(glowAlpha, glowAlpha);
                    }

                    __m256 oldAlpha = _mm256_maskload_ps(maskPtr, vLoadMask);
                    _mm256_maskstore_ps(maskPtr, vLoadMask, _mm256_max_ps(oldAlpha, glowAlpha));
                    maskPtr += 8;
                }
            }
        }
    }

    void RasterBuffers::composite(const PixelView& target, const Brush& brush, Opacity opacity,
        const Matrix3x2* brushTransform, float glowSpreadScale, bool useOwnMask)
    {
        const float* maskBase = useOwnMask ? mask.data() : nullptr;
        int maskStride = useOwnMask ? m_maskStride : 0;

        std::visit([&](const auto& brushArg){
            using T = std::decay_t<decltype(brushArg)>;

            if constexpr (std::is_same_v<T, SolidColor>)
            {
                if (!useOwnMask)
                    return;
                compositeSolid(target, brushArg.color, opacity);
            }
            else if constexpr (std::is_same_v<T, LinearGradient>)
            {
                FloatPoint worldStart = brushTransform ? brushTransform->transform(brushArg.startPoint) : brushArg.startPoint;
                FloatPoint worldEnd = brushTransform ? brushTransform->transform(brushArg.endPoint) : brushArg.endPoint;
                compositeGradient(target, worldStart, worldEnd, brushArg.stops, opacity, maskBase, maskStride);
            }
            else if constexpr (std::is_same_v<T, PointGlow>)
            {
                PointGlow glowParams = brushArg;
                if (brushTransform)
                {
                    glowParams.lightPos = brushTransform->transform(glowParams.lightPos);
                }
                glowParams.lightSpread *= glowSpreadScale;
                glowParams.opacity *= opacity;
                compositePointGlow(target, glowParams, maskBase, maskStride);
            }
            // RadialGradient has no CPU path yet, and had none at any of the call sites either.
        }, brush);
    }

    void RasterBuffers::compositeSolid(const PixelView& surface, Color color, float opacity)
    {
        SimdEnv env;
        const __m256 vBaseAlpha = _mm256_set1_ps((color.alpha / 255.0f) * opacity);
        const __m256i vColorSimd = _mm256_set1_epi32(color.fullyOpaque().asUint());
        const bool useClip = hasActiveClip;

        FloatRect activeRect = surface.clippedBounds();
        if (activeRect.empty())
        {
            return;
        }

        int startY = static_cast<int>(std::floor(activeRect.top));
        int endY = static_cast<int>(std::ceil(activeRect.bottom));
        int startX = static_cast<int>(std::floor(activeRect.left));
        int endX = static_cast<int>(std::ceil(activeRect.right));
        int width = endX - startX;

        int geomTop = static_cast<int>(std::floor(surface.top()));
        int geomLeft = static_cast<int>(std::floor(surface.left()));

        // --- HOIST LOOP INVARIANTS --- [CP]
        const float* clipBaseRow = nullptr;
        int clipYOffset = 0;
        int clipMaxHeight = 0;
        int clipLeft = 0;

        if (useClip && activeClipMaskPtr)
        {
            clipYOffset = -static_cast<int>(std::floor(clipBox.top));
            clipMaxHeight = static_cast<int>(std::ceil(clipBox.bottom)) - static_cast<int>(std::floor(clipBox.top));
            clipLeft = static_cast<int>(std::floor(clipBox.left));
            clipBaseRow = activeClipMaskPtr;
        }

        for (int y = startY; y < endY; ++y)
        {
            float* maskRow = &mask[(y - geomTop) * m_maskStride + (startX - geomLeft)];
            const float* clipRow = nullptr;

            // Zero float-to-int conversions inside the row loop [CP]
            if (clipBaseRow)
            {
                int clipY = y + clipYOffset;
                if (clipY >= 0 && clipY < clipMaxHeight)
                {
                    clipRow = &clipBaseRow[clipY * m_clipStride];
                }
            }

            Color* pixelRow = surface.scanLineAbs(static_cast<float>(startX), static_cast<float>(y));

            for (int xOffset = 0; xOffset < width; xOffset += 8)
            {
                __m256i vLoadMask = env.loadMask(width - xOffset);
                __m256 maskAlpha = _mm256_maskload_ps(maskRow, vLoadMask);

                if (clipRow)
                {
                    int clipX = (startX + xOffset) - clipLeft;
                    maskAlpha = _mm256_mul_ps(maskAlpha, _mm256_maskload_ps(const_cast<float*>(clipRow + clipX), vLoadMask));
                }
                else if (useClip)
                {
                    maskAlpha = env.vZero;
                }

                __m256i vAlphaI = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_mul_ps(maskAlpha, vBaseAlpha), env.v256));
                blendAndStoreBlock(pixelRow, vLoadMask, vColorSimd, vAlphaI, env);

                pixelRow += 8;
                maskRow += 8;
            }
        }
    }

    // Walks the stop list once into a colour ramp that the gradient loop then indexes with the
    // position it already computes. Stop positions are honoured, which the two-colour interpolation
    // this replaces could not do - it took the first and last stop and spread them evenly, so a
    // three stop gradient lost its middle colour entirely.
    //
    // Positions are taken to ascend. Direct2D's own stop collection requires the same, so a brush
    // that renders correctly on the GPU backend renders the same way here.
    void RasterBuffers::buildGradientRamp(std::span<const GradientStop> stops, ColorAsUint* ramp)
    {
        if (stops.empty())
        {
            for (int i = 0; i <= k_gradientRampLast; ++i)
            {
                ramp[i] = Color{}.asUint();
            }
            return;
        }

        std::size_t segment = 0;
        for (int i = 0; i <= k_gradientRampLast; ++i)
        {
            float position = static_cast<float>(i) / static_cast<float>(k_gradientRampLast);
            while (segment + 1 < stops.size() && stops[segment + 1].position < position)
            {
                ++segment;
            }

            const GradientStop& from = stops[segment];
            if (segment + 1 == stops.size())
            {
                ramp[i] = from.color.asUint();
                continue;
            }

            // Before the first stop and after the last one the clamp holds that stop's colour,
            // which is how a gradient extends past its own axis.
            const GradientStop& to = stops[segment + 1];
            float span = to.position - from.position;
            float k = span > 1e-6f ? (position - from.position) / span : 0.0f;
            k = std::min(1.0f, std::max(0.0f, k));
            ramp[i] = Color::blend(from.color, to.color, k).asUint();
        }
    }

    void RasterBuffers::compositeGradient(const PixelView& target, FloatPoint startPoint,
        FloatPoint endPoint, std::span<const GradientStop> stops, Opacity opacity,
        const float* maskBase, int maskStride)
    {
        FloatPoint axis = {
            endPoint.x - startPoint.x,
            endPoint.y - startPoint.y,
        };
        float lenSq = axis.x * axis.x + axis.y * axis.y;
        float invLenSq = (lenSq > 1e-6f) ? (1.0f / lenSq) : 0.0f;

        const FloatRect activeRect = target.clippedBounds();
        if (activeRect.empty())
        {
            return;
        }

        const __m256 vP1x = _mm256_set1_ps(startPoint.x);
        const __m256 vP1y = _mm256_set1_ps(startPoint.y);
        const __m256 vDx = _mm256_set1_ps(axis.x);
        const __m256 vDy = _mm256_set1_ps(axis.y);
        const __m256 vInvLenSq = _mm256_set1_ps(invLenSq);
        const __m256 vSteps = _mm256_setr_ps(0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f);

        const __m256 vZero = _mm256_setzero_ps();
        const __m256 vOne = _mm256_set1_ps(1.0f);
        const __m256 v256 = _mm256_set1_ps(256.0f);

        const __m256 vAlphaNorm = _mm256_set1_ps(256.0f / 255.0f);
        const __m256 vGlobalOpacity = _mm256_set1_ps(opacity);
        const __m256i vOpaqueMask = _mm256_set1_epi32(0xFF000000);

        // A kilobyte on the stack, built once per call. The inner loop reads it with a single
        // gather, so it blends no stop colours per pixel.
        ColorAsUint ramp[k_gradientRampLast + 1];
        buildGradientRamp(stops, ramp);

        RasterBuffers& buffers = g_rasterBuffers;
        const bool useClip = buffers.hasActiveClip;
        const FloatRect& clipBounds = buffers.clipBox;
        const int clipStride = buffers.m_clipStride;

        int startY = static_cast<int>(std::floor(activeRect.top));
        int endY = static_cast<int>(std::ceil(activeRect.bottom));
        int startX = static_cast<int>(std::floor(activeRect.left));
        int endX = static_cast<int>(std::ceil(activeRect.right));
        int width = endX - startX;

        int geomTop = static_cast<int>(std::floor(target.top()));
        int geomLeft = static_cast<int>(std::floor(target.left()));

        // --- HOIST LOOP INVARIANTS --- [CP]
        const float* clipBaseRow = nullptr;
        int clipYOffset = 0;
        int clipMaxHeight = 0;
        int clipLeft = 0;

        if (useClip && buffers.activeClipMaskPtr)
        {
            clipYOffset = -static_cast<int>(std::floor(clipBounds.top));
            clipMaxHeight = static_cast<int>(std::ceil(clipBounds.bottom)) - static_cast<int>(std::floor(clipBounds.top));
            clipLeft = static_cast<int>(std::floor(clipBounds.left));
            clipBaseRow = buffers.activeClipMaskPtr;
        }

        for (int y = startY; y < endY; ++y)
        {
            float py = static_cast<float>(y) + 0.5f;
            __m256 vRy = _mm256_sub_ps(_mm256_set1_ps(py), vP1y);

            Color* pixelRow = target.scanLineAbs(static_cast<float>(startX), static_cast<float>(y));
            const float* maskRow = maskBase ? &maskBase[(y - geomTop) * maskStride + (startX - geomLeft)] : nullptr;
            const float* clipRow = nullptr;

            if (clipBaseRow)
            {
                int clipY = y + clipYOffset;
                if (clipY >= 0 && clipY < clipMaxHeight)
                {
                    clipRow = &clipBaseRow[clipY * clipStride];
                }
            }

            for (int xOffset = 0; xOffset < width; xOffset += 8)
            {
                int remaining = width - xOffset;
                __m256i vLoadMask = (remaining >= 8) ? _mm256_set1_epi32(-1) :
                    _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g_maskTableMiddlePtr - remaining));

                __m256 vPx = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(startX + xOffset)), vSteps);
                __m256 vRx = _mm256_sub_ps(vPx, vP1x);
                __m256 vDot = _mm256_add_ps(_mm256_mul_ps(vRx, vDx), _mm256_mul_ps(vRy, vDy));
                __m256 vT = _mm256_min_ps(vOne, _mm256_max_ps(vZero, _mm256_mul_ps(vDot, vInvLenSq)));
                __m256i vTI = _mm256_cvtps_epi32(_mm256_mul_ps(vT, v256));
                __m256i vSrcColors = _mm256_i32gather_epi32(reinterpret_cast<const int*>(ramp), vTI, 4);

                __m256 vPathAlpha = maskRow ? _mm256_maskload_ps(const_cast<float*>(maskRow), vLoadMask) : vOne;

                if (clipRow)
                {
                    int clipX = (startX + xOffset) - clipLeft;
                    __m256 vClipAlpha = _mm256_maskload_ps(const_cast<float*>(clipRow + clipX), vLoadMask);
                    vPathAlpha = _mm256_mul_ps(vPathAlpha, vClipAlpha);
                }
                else if (useClip)
                {
                    vPathAlpha = vZero;
                }

                __m256 vColorAlphaF = _mm256_cvtepi32_ps(_mm256_srli_epi32(vSrcColors, 24));
                __m256i vWeightI = _mm256_cvtps_epi32(_mm256_mul_ps(vColorAlphaF,
                    _mm256_mul_ps(vAlphaNorm,
                        _mm256_mul_ps(vPathAlpha, vGlobalOpacity))));

                if (_mm256_movemask_epi8(_mm256_cmpeq_epi32(vWeightI, _mm256_setzero_si256())) != 0xFFFFFFFF)
                {
                    __m256i vDest = _mm256_maskload_epi32(reinterpret_cast<const int*>(pixelRow), vLoadMask);
                    __m256i vBlendColors = _mm256_or_si256(vSrcColors, vOpaqueMask);
                    _mm256_maskstore_epi32(reinterpret_cast<int*>(pixelRow), vLoadMask, Color::blend8(vDest, vBlendColors, vWeightI));
                }

                pixelRow += 8;
                if (maskRow)
                {
                    maskRow += 8;
                }
            }
        }
    }

    void RasterBuffers::compositePointGlow(const PixelView& view, const PointGlow& p, const float* maskBase, int maskStride)
    {
        if (p.lightSpread <= 0.0f || p.opacity <= 0.0f)
        {
            return;
        }

        float radiusX = p.lightSpread / (p.xRatio > 0.0f ? p.xRatio : 1.0f);
        float radiusY = p.lightSpread;
        FloatRect glowBox = {
            p.lightPos.x - radiusX,
            p.lightPos.y - radiusY,
            p.lightPos.x + radiusX,
            p.lightPos.y + radiusY,
        };

        FloatRect activeBounds = view.clippedBounds();
        activeBounds.intersectWith(glowBox);
        if (activeBounds.empty())
        {
            return;
        }

        SimdEnv env;
        const __m256 vLightX = _mm256_set1_ps(p.lightPos.x);
        const __m256 vFactor = _mm256_set1_ps(p.opacity * (p.lightColor.alpha / 255.0f));
        const __m256i vColor = _mm256_set1_epi32(p.lightColor.fullyOpaque().asUint());

        // --- HOIST LOOP MULTIPLIER --- [CP]
        // Folds the x-ratio and the reciprocal spread into one broadcast, so the inner loop
        // needs a single multiply rather than a divide plus a multiply.
        const __m256 vXRatioSpread = _mm256_set1_ps((p.xRatio > 0.0f ? p.xRatio : 1.0f) / p.lightSpread);

        RasterBuffers& buffers = g_rasterBuffers;

        int startY = static_cast<int>(std::floor(activeBounds.top));
        int endY = static_cast<int>(std::ceil(activeBounds.bottom));
        int startX = static_cast<int>(std::floor(activeBounds.left));
        int endX = static_cast<int>(std::ceil(activeBounds.right));

        int geomTop = static_cast<int>(std::floor(view.top()));
        int geomLeft = static_cast<int>(std::floor(view.left()));

        const float* clipBaseRow = nullptr;
        int clipYOffset = 0;
        int clipMaxHeight = 0;
        int clipLeft = 0;

        if (buffers.hasActiveClip && buffers.activeClipMaskPtr)
        {
            clipYOffset = -static_cast<int>(std::floor(buffers.clipBox.top));
            clipMaxHeight = static_cast<int>(std::ceil(buffers.clipBox.bottom)) - static_cast<int>(std::floor(buffers.clipBox.top));
            clipLeft = static_cast<int>(std::floor(buffers.clipBox.left));
            clipBaseRow = buffers.activeClipMaskPtr;
        }

        for (int y = startY; y < endY; ++y)
        {
            float py = static_cast<float>(y) + 0.5f;
            float dyVal = std::abs(py - p.lightPos.y);
            float dyNormVal = dyVal / p.lightSpread;

            // --- EARLY-ROW REJECTION --- [CP]
            // Rows outside the vertical radius contribute nothing.
            if (dyNormVal >= 1.0f)
            {
                continue;
            }

            __m256 vDyNorm = _mm256_set1_ps(dyNormVal);
            __m256 vDySq = _mm256_mul_ps(vDyNorm, vDyNorm);
            __m256 vDyQuad = _mm256_mul_ps(vDySq, vDySq);

            Color* pixelRow = view.scanLineAbs(static_cast<float>(startX), static_cast<float>(y));
            const float* maskRow = maskBase ? &maskBase[(y - geomTop) * maskStride + (startX - geomLeft)] : nullptr;
            const float* clipRow = nullptr;

            if (clipBaseRow)
            {
                int clipY = y + clipYOffset;
                if (clipY >= 0 && clipY < clipMaxHeight)
                {
                    clipRow = &clipBaseRow[clipY * buffers.m_clipStride];
                }
            }

            for (int x = startX; x < endX; x += 8)
            {
                __m256i vLoadMask = env.loadMask(endX - x);

                __m256 vPx = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(x)), env.vSteps);
                __m256 vDx = _mm256_mul_ps(mm256_abs_ps(_mm256_sub_ps(vPx, vLightX)), vXRatioSpread);

                __m256 vDist = calcGlowDist(vDx, vDyNorm, vDySq, vDyQuad, p.shape);
                __m256 vIntensity = _mm256_mul_ps(_mm256_max_ps(env.vZero, _mm256_sub_ps(env.vOne, vDist)), vFactor);

                if (maskRow)
                {
                    __m256 vPathAlpha = _mm256_maskload_ps(const_cast<float*>(maskRow), vLoadMask);
                    if (clipRow)
                    {
                        int clipX = x - clipLeft;
                        vPathAlpha = _mm256_mul_ps(vPathAlpha, _mm256_maskload_ps(const_cast<float*>(clipRow + clipX), vLoadMask));
                    }
                    else if (buffers.hasActiveClip)
                    {
                        vPathAlpha = env.vZero;
                    }
                    vIntensity = _mm256_mul_ps(vIntensity, vPathAlpha);
                }

                __m256i vAlphaI = _mm256_cvtps_epi32(_mm256_mul_ps(vIntensity, env.v256));
                blendAndStoreBlock(pixelRow, vLoadMask, vColor, vAlphaI, env);

                pixelRow += 8;
                if (maskRow)
                {
                    maskRow += 8;
                }
            }
        }
    }

    FloatRect RasterBuffers::calculateEdgeBounds(const RenderEdge& edge, float radius, FloatRect bounds)
    {
        FloatRect segBox = {
            std::min(edge.p1.x, edge.p2.x) - radius,
            std::min(edge.p1.y, edge.p2.y) - radius,
            std::max(edge.p1.x, edge.p2.x) + radius,
            std::max(edge.p1.y, edge.p2.y) + radius,
        };
        segBox.intersectWith(bounds);
        return segBox;
    }

    const EdgeIndices& RasterBuffers::edgeIndices(int y, int topOffset)
    {
        return m_edgeBuckets[y - topOffset];
    }

}
