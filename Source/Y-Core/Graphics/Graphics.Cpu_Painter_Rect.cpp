module;
#include <immintrin.h>
module ClaFi.Core.Graphics.Cpu_Painter_Rect;

import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    void RectPainter::paintSolid(const FloatRect& rect, const Brush& brush, float opacity, const Matrix3x2* brushTransform)
    {
        if (opacity <= 0.0f)
        {
            return;
        }

        FloatRect boundary = rect;
        boundary.intersectWith(m_pixelView.clippedBounds());
        if (boundary.empty())
        {
            return;
        }

        // Integer-snapped edges let the whole rect skip coverage maths entirely [CP]
        const bool isSnapped = (rect.left == std::floor(rect.left)) &&
            (rect.right == std::floor(rect.right)) &&
            (rect.top == std::floor(rect.top)) &&
            (rect.bottom == std::floor(rect.bottom));

        // ====================================================================
        // PATH A: SNAPPED SOLID COLOR BRUSH (aliased memory writes)
        // ====================================================================
        if (isSnapped && std::holds_alternative<SolidColor>(brush))
        {
            Color color = std::get<SolidColor>(brush).color;
            if (color.alpha == 0)
            {
                return;
            }

            int startX = static_cast<int>(boundary.left);
            int endX = static_cast<int>(boundary.right);
            int startY = static_cast<int>(boundary.top);
            int endY = static_cast<int>(boundary.bottom);
            int rowWidth = endX - startX;
            if (rowWidth <= 0 || endY <= startY)
            {
                return;
            }

            const float finalAlpha = (color.alpha / 255.0f) * opacity;
            std::intptr_t stride = m_pixelView.stride();

            if (finalAlpha >= 0.999f)
            {
                Color solidColor = color.fullyOpaque();
                Color* scanLine = m_pixelView.pixelAbs(static_cast<float>(startX), static_cast<float>(startY), false);
                PixelView::fillPixelLine(scanLine, rowWidth, solidColor);

                std::size_t lineSize = rowWidth * sizeof(Color);
                Color* endLine = scanLine + (endY - startY) * stride;
                for (Color* currentLine = scanLine + stride; currentLine != endLine; currentLine += stride)
                {
                    std::memcpy(currentLine, scanLine, lineSize);
                }

                return;
            }

            Color* scanLine = m_pixelView.pixelAbs(static_cast<float>(startX), static_cast<float>(startY), false);

            const __m256 vAlphaScale = _mm256_set1_ps(finalAlpha);
            const __m256 v256 = _mm256_set1_ps(256.0f);
            const __m256i vColorSimd = _mm256_set1_epi32(color.fullyOpaque().asUint());
            const __m256i vAlphaI = _mm256_cvtps_epi32(_mm256_mul_ps(vAlphaScale, v256));
            const __m256i vMinusOne = _mm256_set1_epi32(-1);

            for (int py = startY; py < endY; ++py)
            {
                Color* pixel = scanLine;

                for (int xOffset = 0; xOffset < rowWidth; xOffset += 8)
                {
                    int remaining = rowWidth - xOffset;
                    __m256i vMask = (remaining >= 8)
                        ? vMinusOne
                        : _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g_maskTableMiddlePtr - remaining));

                    __m256i vResult = _mm256_maskload_epi32(reinterpret_cast<const int*>(pixel), vMask);
                    vResult = Color::blend8(vResult, vColorSimd, vAlphaI);
                    _mm256_maskstore_epi32(reinterpret_cast<int*>(pixel), vMask, vResult);
                    pixel += 8;
                }

                scanLine += stride;
            }

            return;
        }

        // ====================================================================
        // PATH B: FRACTIONAL SOLID COLOR (sub-pixel anti-aliased blending)
        // ====================================================================
        if (std::holds_alternative<SolidColor>(brush))
        {
            Color color = std::get<SolidColor>(brush).color;
            if (color.alpha == 0)
            {
                return;
            }

            int startX = static_cast<int>(std::floor(boundary.left));
            int endX = static_cast<int>(std::ceil(boundary.right));
            int startY = static_cast<int>(std::floor(boundary.top));
            int endY = static_cast<int>(std::ceil(boundary.bottom));
            int rowWidth = endX - startX;
            if (rowWidth <= 0 || endY <= startY)
            {
                return;
            }

            const float baseAlpha = (color.alpha / 255.0f) * opacity;
            std::intptr_t stride = m_pixelView.stride();

            Color* scanLine = m_pixelView.pixelAbs(static_cast<float>(startX), static_cast<float>(startY), false);

            const __m256 vAlphaScale = _mm256_set1_ps(baseAlpha);
            const __m256 v256 = _mm256_set1_ps(256.0f);
            const __m256 vOne = _mm256_set1_ps(1.0f);
            const __m256 vZero = _mm256_setzero_ps();
            const __m256i vColorSimd = _mm256_set1_epi32(color.fullyOpaque().asUint());
            const __m256i vMinusOne = _mm256_set1_epi32(-1);
            const __m256 vSteps = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);

            const float rectLeft = rect.left;
            const float rectRight = rect.right;
            const float rectTop = rect.top;
            const float rectBottom = rect.bottom;

            const __m256 vLeft = _mm256_set1_ps(rectLeft);
            const __m256 vRight = _mm256_set1_ps(rectRight);

            for (int py = startY; py < endY; ++py)
            {
                float pyF = static_cast<float>(py);
                float coverageY = std::max(0.0f, std::min(pyF + 1.0f, rectBottom) - std::max(pyF, rectTop));

                if (coverageY <= 1e-6f)
                {
                    scanLine += stride;
                    continue;
                }

                Color* pixel = scanLine;
                const __m256 vCoverageY = _mm256_set1_ps(coverageY);

                for (int xOffset = 0; xOffset < rowWidth; xOffset += 8)
                {
                    int remaining = rowWidth - xOffset;
                    __m256i vMask = (remaining >= 8)
                        ? vMinusOne
                        : _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g_maskTableMiddlePtr - remaining));

                    __m256 vXStart = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(startX + xOffset)), vSteps);
                    __m256 vXEnd = _mm256_add_ps(vXStart, vOne);

                    __m256 vMin = _mm256_min_ps(vXEnd, vRight);
                    __m256 vMax = _mm256_max_ps(vXStart, vLeft);
                    __m256 vCoverageX = _mm256_max_ps(vZero, _mm256_sub_ps(vMin, vMax));

                    __m256 vAlphaF = _mm256_mul_ps(_mm256_mul_ps(vCoverageX, vCoverageY), vAlphaScale);
                    __m256i vAlphaI = _mm256_cvtps_epi32(_mm256_mul_ps(vAlphaF, v256));

                    __m256i vResult = _mm256_maskload_epi32(reinterpret_cast<const int*>(pixel), vMask);
                    vResult = Color::blend8(vResult, vColorSimd, vAlphaI);
                    _mm256_maskstore_epi32(reinterpret_cast<int*>(pixel), vMask, vResult);
                    pixel += 8;
                }

                scanLine += stride;
            }
        }
        // ====================================================================
        // PATH C: COMPLEX BRUSH (direct mask-free pixel compositing loop)
        // ====================================================================
        else
        {
            PixelView targetView = m_pixelView.subView(boundary);

            // This route writes every pixel and carries no coverage mask, so it reaches the shared
            // buffers only for the entry point - a solid brush stays the no-op it has always been
            // here, because it is handled by the fast path above.
            float glowSpreadScale = brushTransform ? brushTransform->getScaleFactor() : 1.0f;
            g_rasterBuffers.composite(targetView, brush, opacity, brushTransform, glowSpreadScale, false);
        }
    }

    void RectPainter::paintBorder(const FloatRect& rect, const Brush& brush, float borderWidth, float opacity, const Matrix3x2* brushTransform)
    {
        if (borderWidth <= 0.0f || opacity <= 0.0f)
        {
            return;
        }

        paintSolid({ rect.left, rect.top, rect.right, rect.top + borderWidth }, brush, opacity, brushTransform);
        paintSolid({ rect.left, rect.bottom - borderWidth, rect.right, rect.bottom }, brush, opacity, brushTransform);
        paintSolid({ rect.left, rect.top + borderWidth, rect.left + borderWidth, rect.bottom - borderWidth }, brush, opacity, brushTransform);
        paintSolid({ rect.right - borderWidth, rect.top + borderWidth, rect.right, rect.bottom - borderWidth }, brush, opacity, brushTransform);
    }
}
