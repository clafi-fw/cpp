module;
#include <immintrin.h>
module ClaFi.Core.Graphics.Cpu_Painter_Circle;

import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    // A corner circle is a quarter arc: the distance field is folded so that only the quadrant
    // facing the corner contributes. A sign of zero means no fold, which is the full circle.
    struct CornerSigns
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    [[nodiscard]] static CornerSigns cornerSigns(Corner corner)
    {
        switch (corner)
        {
            case Corner::TopLeft:
                return { 1.0f, 1.0f };
            case Corner::TopRight:
                return { -1.0f, 1.0f };
            case Corner::BottomRight:
                return { -1.0f, -1.0f };
            case Corner::BottomLeft:
                return { 1.0f, -1.0f };
            default:
                return { 0.0f, 0.0f };
        }
    }

    void CirclePainter::initBoundary()
    {
        m_corner = Corner::None;
        m_boundingBox = {
            m_pivot.x - m_radius,
            m_pivot.y - m_radius,
            m_pivot.x + m_radius,
            m_pivot.y + m_radius,
        };
    }

    void CirclePainter::initBoundary(Corner corner)
    {
        m_corner = corner;

        float pivotX = m_pivot.x;
        float pivotY = m_pivot.y;
        float radius = m_radius;

        switch (corner)
        {
            case Corner::TopLeft:
                m_boundingBox = { pivotX - radius, pivotY - radius, pivotX, pivotY };
                break;
            case Corner::TopRight:
                m_boundingBox = { pivotX, pivotY - radius, pivotX + radius, pivotY };
                break;
            case Corner::BottomRight:
                m_boundingBox = { pivotX, pivotY, pivotX + radius, pivotY + radius };
                break;
            case Corner::BottomLeft:
                m_boundingBox = { pivotX - radius, pivotY, pivotX, pivotY + radius };
                break;
            default:
                break;
        }
    }

    void CirclePainter::paint(const Matrix3x2* brushTransform)
    {
        FloatRect boundary = m_boundingBox;
        boundary.intersectWith(m_pixelView.clippedBounds());
        if (boundary.empty())
        {
            return;
        }

        const SolidColor* solidBg = std::get_if<SolidColor>(&m_backGroundColor);
        const SolidColor* solidBorder = std::get_if<SolidColor>(&m_borderColor);

        bool canUseFastPath = true;

        if (!solidBg && m_backGroundColor.index() != 0)
        {
            canUseFastPath = false;
        }

        if (!solidBorder && m_borderColor.index() != 0)
        {
            canUseFastPath = false;
        }

        if (canUseFastPath)
        {
            paintDirectCircle(boundary, m_opacity);
            return;
        }

        int boundaryWidth = static_cast<int>(std::ceil(boundary.right)) - static_cast<int>(std::floor(boundary.left));
        int boundaryHeight = static_cast<int>(std::ceil(boundary.bottom)) - static_cast<int>(std::floor(boundary.top));

        bool hasBg = m_backGroundColor.index() != 0 || (solidBg && solidBg->color.alpha > 0);
        bool hasBorder = m_borderWidth > 0.0f && (m_borderColor.index() != 0 || (solidBorder && solidBorder->color.alpha > 0));

        if (hasBg)
        {
            g_rasterBuffers.prepareForEffect(boundaryWidth, boundaryHeight, true);
            rasterizeCircleMask(false, boundary, g_rasterBuffers.mask.data(), g_rasterBuffers.m_maskStride);
            compositeBrush(m_pixelView.subView(boundary), m_backGroundColor, m_opacity, brushTransform);
        }

        if (hasBorder)
        {
            g_rasterBuffers.prepareForEffect(boundaryWidth, boundaryHeight, true);
            rasterizeCircleMask(true, boundary, g_rasterBuffers.mask.data(), g_rasterBuffers.m_maskStride);
            compositeBrush(m_pixelView.subView(boundary), m_borderColor, m_opacity, brushTransform);
        }
    }

    void CirclePainter::paintRing(FloatPoint pivot, const Brush& borderBrush, float radius, float borderWidth, const Matrix3x2* brushTransform)
    {
        m_pivot = pivot;
        m_radius = radius;
        m_borderWidth = borderWidth;
        m_borderColor = borderBrush;
        m_backGroundColor = SolidColor{ Color{} };
        m_opacity = 1.0f;
        m_corner = Corner::None;

        initBoundary();
        paint(brushTransform);
    }

    void CirclePainter::paintSolid(FloatPoint pivot, const Brush& bgColor, float radius, const Matrix3x2* brushTransform)
    {
        m_pivot = pivot;
        m_radius = radius;
        m_borderWidth = 0.0f;
        m_borderColor = SolidColor{ Color{} };
        m_backGroundColor = bgColor;
        m_opacity = 1.0f;
        m_corner = Corner::None;

        initBoundary();
        paint(brushTransform);
    }

    void CirclePainter::paintDirectCircle(const FloatRect& boundary, float opacity)
    {
        int startX = static_cast<int>(std::floor(boundary.left));
        int endX = static_cast<int>(std::ceil(boundary.right));
        int startY = static_cast<int>(std::floor(boundary.top));
        int endY = static_cast<int>(std::ceil(boundary.bottom));
        int rowWidth = endX - startX;

        const FloatPoint adjustedPivot = m_pivot - FloatPoint{ 0.5f, 0.5f };

        const bool hasBg = std::holds_alternative<SolidColor>(m_backGroundColor) && std::get<SolidColor>(m_backGroundColor).color.alpha;
        const bool hasBorder = m_borderWidth > 0.0f && std::holds_alternative<SolidColor>(m_borderColor) && std::get<SolidColor>(m_borderColor).color.alpha;

        float bgRadius = m_radius - (hasBorder ? m_borderWidth - 0.5f : 0.0f);

        Color bgColor = hasBg ? std::get<SolidColor>(m_backGroundColor).color : Color{};
        Color borderColor = hasBorder ? std::get<SolidColor>(m_borderColor).color : Color{};

        const float bgAlphaScale = (bgColor.alpha / 255.0f) * opacity;
        const float borderAlphaScale = (borderColor.alpha / 255.0f) * opacity;

        const __m256i vBgColorSimd = _mm256_set1_epi32(bgColor.fullyOpaque().asUint());
        const __m256i vBorderColorSimd = _mm256_set1_epi32(borderColor.fullyOpaque().asUint());

        Color* scanLine = m_pixelView.pixelAbs(static_cast<float>(startX), static_cast<float>(startY), false);

        const __m256 vZero = _mm256_setzero_ps();
        const __m256 v05 = _mm256_set1_ps(0.5f);
        const __m256 vOne = _mm256_set1_ps(1.0f);
        const __m256i vMinusOne = _mm256_set1_epi32(-1);
        const __m256 v256 = _mm256_set1_ps(256.0f);
        const __m256 vSteps = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
        const __m256 vSignMask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));

        // Background Properties
        const __m256 vBgRadius = _mm256_set1_ps(bgRadius);
        const __m256 vBgAlphaScale = _mm256_set1_ps(bgAlphaScale);

        // Border Properties (Analytical Midpoint Stroke setup)
        const __m256 vRmid = _mm256_set1_ps(m_radius - m_borderWidth * 0.5f);
        const __m256 vHalfWidth = _mm256_set1_ps(m_borderWidth * 0.5f);
        const __m256 vBorderAlphaScale = _mm256_set1_ps(borderAlphaScale);

        const CornerSigns signs = cornerSigns(m_corner);
        const __m256 vSignX = _mm256_set1_ps(signs.x);
        const __m256 vSignY = _mm256_set1_ps(signs.y);

        const float radiusPlusOne = m_radius + 1.0f;
        const __m256 vRadiusPlusOneSquare = _mm256_set1_ps(radiusPlusOne * radiusPlusOne);

        for (int py = startY; py < endY; ++py)
        {
            Color* pixel = scanLine;
            float dyVal = static_cast<float>(py) - adjustedPivot.y;

            __m256 vDy = (signs.y == 0.0f)
                ? _mm256_and_ps(_mm256_set1_ps(dyVal), vSignMask)
                : _mm256_max_ps(vZero, _mm256_mul_ps(_mm256_set1_ps(dyVal), _mm256_sub_ps(vZero, vSignY)));

            const __m256 vY2 = _mm256_mul_ps(vDy, vDy);

            for (int xOffset = 0; xOffset < rowWidth; xOffset += 8)
            {
                int remaining = rowWidth - xOffset;
                __m256i vMask = (remaining >= 8) ? vMinusOne : _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g_maskTableMiddlePtr - remaining));

                __m256 vXDiff = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(startX + xOffset) - adjustedPivot.x), vSteps);

                __m256 vDx = (signs.x == 0.0f)
                    ? _mm256_and_ps(vXDiff, vSignMask)
                    : _mm256_max_ps(vZero, _mm256_mul_ps(vXDiff, _mm256_sub_ps(vZero, vSignX)));

                __m256 vDist2 = _mm256_fmadd_ps(vDx, vDx, vY2);

                if (_mm256_movemask_ps(_mm256_cmp_ps(vDist2, vRadiusPlusOneSquare, _CMP_GT_OQ)) != 0xFF)
                {
                    __m256 vDist = _mm256_sqrt_ps(vDist2);
                    __m256i vResult = _mm256_maskload_epi32(reinterpret_cast<const int*>(pixel), vMask);

                    if (hasBg)
                    {
                        __m256 alpha = _mm256_add_ps(_mm256_sub_ps(vBgRadius, vDist), v05);
                        __m256 vBgAlphaF = _mm256_max_ps(_mm256_min_ps(alpha, vOne), vZero);
                        vBgAlphaF = _mm256_mul_ps(vBgAlphaScale, vBgAlphaF);
                        __m256i vAlphaI = _mm256_cvtps_epi32(_mm256_mul_ps(vBgAlphaF, v256));
                        vResult = Color::blend8(vResult, vBgColorSimd, vAlphaI);
                    }

                    if (hasBorder)
                    {
                        __m256 vDistFromMid = _mm256_and_ps(_mm256_sub_ps(vDist, vRmid), vSignMask);
                        __m256 vRingAlphaF = _mm256_add_ps(_mm256_sub_ps(vHalfWidth, vDistFromMid), v05);
                        vRingAlphaF = _mm256_max_ps(_mm256_min_ps(vRingAlphaF, vOne), vZero);

                        vRingAlphaF = _mm256_mul_ps(vBorderAlphaScale, vRingAlphaF);
                        __m256i vAlphaI = _mm256_cvtps_epi32(_mm256_mul_ps(vRingAlphaF, v256));
                        vResult = Color::blend8(vResult, vBorderColorSimd, vAlphaI);
                    }

                    _mm256_maskstore_epi32(reinterpret_cast<int*>(pixel), vMask, vResult);
                }

                pixel += 8;
            }

            scanLine += m_pixelView.stride();
        }
    }

    void CirclePainter::rasterizeCircleMask(bool isRing, const FloatRect& bounds, float* mask, int stride)
    {
        int startX = static_cast<int>(std::floor(bounds.left));
        int endX = static_cast<int>(std::ceil(bounds.right));
        int startY = static_cast<int>(std::floor(bounds.top));
        int endY = static_cast<int>(std::ceil(bounds.bottom));
        int rowWidth = endX - startX;

        const FloatPoint adjustedPivot = m_pivot - FloatPoint{ 0.5f, 0.5f };

        float bgRadius = m_radius - (isRing ? m_borderWidth - 0.5f : 0.0f);

        const __m256 vZero = _mm256_setzero_ps();
        const __m256 v05 = _mm256_set1_ps(0.5f);
        const __m256 vOne = _mm256_set1_ps(1.0f);
        const __m256 vSteps = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
        const __m256 vSignMask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));

        const __m256 vBgRadius = _mm256_set1_ps(bgRadius);

        const __m256 vRmid = _mm256_set1_ps(m_radius - m_borderWidth * 0.5f);
        const __m256 vHalfWidth = _mm256_set1_ps(m_borderWidth * 0.5f);

        const CornerSigns signs = cornerSigns(m_corner);
        const __m256 vSignX = _mm256_set1_ps(signs.x);
        const __m256 vSignY = _mm256_set1_ps(signs.y);

        const float radiusPlusOne = m_radius + 1.0f;
        const __m256 vRadiusPlusOneSquare = _mm256_set1_ps(radiusPlusOne * radiusPlusOne);

        for (int py = startY; py < endY; ++py)
        {
            float* maskRow = &mask[(py - startY) * stride];
            float dyVal = static_cast<float>(py) - adjustedPivot.y;

            __m256 vDy = (signs.y == 0.0f)
                ? _mm256_and_ps(_mm256_set1_ps(dyVal), vSignMask)
                : _mm256_max_ps(vZero, _mm256_mul_ps(_mm256_set1_ps(dyVal), _mm256_sub_ps(vZero, vSignY)));

            const __m256 vY2 = _mm256_mul_ps(vDy, vDy);

            for (int xOffset = 0; xOffset < rowWidth; xOffset += 8)
            {
                int remaining = rowWidth - xOffset;
                __m256 vXDiff = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(startX + xOffset) - adjustedPivot.x), vSteps);

                __m256 vDx = (signs.x == 0.0f)
                    ? _mm256_and_ps(vXDiff, vSignMask)
                    : _mm256_max_ps(vZero, _mm256_mul_ps(vXDiff, _mm256_sub_ps(vZero, vSignX)));

                __m256 vDist2 = _mm256_fmadd_ps(vDx, vDx, vY2);

                __m256 vAlpha = vZero;

                if (_mm256_movemask_ps(_mm256_cmp_ps(vDist2, vRadiusPlusOneSquare, _CMP_GT_OQ)) != 0xFF)
                {
                    __m256 vDist = _mm256_sqrt_ps(vDist2);

                    if (!isRing)
                    {
                        __m256 alpha = _mm256_add_ps(_mm256_sub_ps(vBgRadius, vDist), v05);
                        vAlpha = _mm256_max_ps(_mm256_min_ps(alpha, vOne), vZero);
                    }
                    else
                    {
                        __m256 vDistFromMid = _mm256_and_ps(_mm256_sub_ps(vDist, vRmid), vSignMask);
                        vAlpha = _mm256_add_ps(_mm256_sub_ps(vHalfWidth, vDistFromMid), v05);
                        vAlpha = _mm256_max_ps(_mm256_min_ps(vAlpha, vOne), vZero);
                    }
                }

                if (remaining >= 8)
                {
                    _mm256_storeu_ps(maskRow + xOffset, vAlpha);
                }
                else
                {
                    _mm256_maskstore_ps(maskRow + xOffset, _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g_maskTableMiddlePtr - remaining)), vAlpha);
                }
            }
        }
    }

    void CirclePainter::compositeBrush(const PixelView& target, const Brush& brush, float opacity, const Matrix3x2* brushTransform)
    {
        float glowSpreadScale = brushTransform ? brushTransform->getScaleFactor() : 1.0f;
        g_rasterBuffers.composite(target, brush, opacity, brushTransform, glowSpreadScale);
    }
}
