module;
#include <immintrin.h>
#include "Graphics.Cpu_Simd.h"
module ClaFi.Core.Graphics.Cpu_Painter_Rect;

import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{
    namespace
    {
        // How much of the pixel starting at `pixel` lies between `from` and `to`.
        float coverageOf(const int pixel, const float from, const float to)
        {
            const float start = static_cast<float>(pixel);
            return std::clamp(std::min(start + 1.0f, to) - std::max(start, from), 0.0f, 1.0f);
        }
    }

    void RectPainter::fill(const FloatRect& rect, const Brush& brush, const Matrix3x2* brushTransform)
    {
        FloatRect boundary = rect;
        boundary.intersectWith(m_pixelView.clippedBounds());
        if (boundary.empty())
        {
            return;
        }

        // The two solid routes below write pixels straight, and a path clip reaches a pixel only
        // through a coverage mask.
        if (g_rasterBuffers.hasActiveClip && std::holds_alternative<SolidColor>(brush))
        {
            fillCovered(boundary.roundedOut(), rect, brush, brushTransform);
            return;
        }

        // Integer-snapped edges let the whole rect skip coverage maths entirely [CP]
        const bool isSnapped = (rect.left == std::floor(rect.left)) &&
            (rect.right == std::floor(rect.right)) &&
            (rect.top == std::floor(rect.top)) &&
            (rect.bottom == std::floor(rect.bottom));

        // ====================================================================
        // **Path a: snapped solid color brush** (aliased memory writes)
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

            const float finalAlpha = color.alpha / 255.0f;
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
                    vResult = blend8(vResult, vColorSimd, vAlphaI);
                    _mm256_maskstore_epi32(reinterpret_cast<int*>(pixel), vMask, vResult);
                    pixel += 8;
                }

                scanLine += stride;
            }

            return;
        }

        // ====================================================================
        // **Path b: fractional solid color** (sub-pixel anti-aliased blending)
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

            const float baseAlpha = color.alpha / 255.0f;
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
                    vResult = blend8(vResult, vColorSimd, vAlphaI);
                    _mm256_maskstore_epi32(reinterpret_cast<int*>(pixel), vMask, vResult);
                    pixel += 8;
                }

                scanLine += stride;
            }
        }
        // ====================================================================
        // **Path c: complex brush** (direct mask-free pixel compositing loop)
        // ====================================================================
        else
        {
            // The pixels the rectangle covers whole take the brush without a coverage mask. Only
            // the rim an edge between pixels cuts goes through one, at most a pixel wide on each
            // side, so a large fill costs about what the mask-free composite alone costs. A rim
            // pixel filled whole would widen the rectangle by up to a pixel on that side.
            const IntRect pixels = boundary.roundedOut();
            const IntRect whole = {
                std::max(pixels.left, static_cast<int>(std::ceil(rect.left))),
                std::max(pixels.top, static_cast<int>(std::ceil(rect.top))),
                std::min(pixels.right, static_cast<int>(std::floor(rect.right))),
                std::min(pixels.bottom, static_cast<int>(std::floor(rect.bottom)))
            };
            if (whole.left >= whole.right || whole.top >= whole.bottom)
            {
                fillCovered(pixels, rect, brush, brushTransform);
                return;
            }

            // A solid brush never reaches here: compositing one needs the mask this skips.
            const float glowSpreadScale = brushTransform ? brushTransform->getScaleFactor() : 1.0f;
            g_rasterBuffers.composite(m_pixelView.subView(whole.toFloat()), brush, brushTransform,
                glowSpreadScale, false);
            fillCovered({ pixels.left, pixels.top, pixels.right, whole.top }, rect, brush,
                brushTransform);
            fillCovered({ pixels.left, whole.bottom, pixels.right, pixels.bottom }, rect, brush,
                brushTransform);
            fillCovered({ pixels.left, whole.top, whole.left, whole.bottom }, rect, brush,
                brushTransform);
            fillCovered({ whole.right, whole.top, pixels.right, whole.bottom }, rect, brush,
                brushTransform);
        }
    }

    // Every column between the first and the last lies inside the rectangle, so a row's coverage
    // is its own share of the height, cut at the two ends by their share of the width.
    void RectPainter::fillCovered(const IntRect& area, const FloatRect& rect, const Brush& brush,
        const Matrix3x2* brushTransform)
    {
        const int width = area.right - area.left;
        const int height = area.bottom - area.top;
        if (width <= 0 || height <= 0)
        {
            return;
        }

        RasterBuffers& buffers = g_rasterBuffers;
        buffers.prepareForEffect(width, height, false);
        const float leftCoverage = coverageOf(area.left, rect.left, rect.right);
        const float rightCoverage = coverageOf(area.right - 1, rect.left, rect.right);
        for (int y = area.top; y != area.bottom; ++y)
        {
            float* const row = buffers.mask.data()
                + static_cast<std::ptrdiff_t>(y - area.top) * buffers.m_maskStride;
            const float rowCoverage = coverageOf(y, rect.top, rect.bottom);
            std::fill(row, row + width, rowCoverage);
            row[0] = rowCoverage * leftCoverage;
            row[width - 1] = rowCoverage * rightCoverage;
        }

        const float glowSpreadScale = brushTransform ? brushTransform->getScaleFactor() : 1.0f;
        buffers.composite(m_pixelView.subView(area.toFloat()), brush, brushTransform,
            glowSpreadScale);
    }

    void RectPainter::stroke(const FloatRect& rect, const Brush& brush, float borderWidth, const Matrix3x2* brushTransform)
    {
        if (borderWidth <= 0.0f)
        {
            return;
        }

        fill({ rect.left, rect.top, rect.right, rect.top + borderWidth }, brush, brushTransform);
        fill({ rect.left, rect.bottom - borderWidth, rect.right, rect.bottom }, brush, brushTransform);
        fill({ rect.left, rect.top + borderWidth, rect.left + borderWidth, rect.bottom - borderWidth }, brush, brushTransform);
        fill({ rect.right - borderWidth, rect.top + borderWidth, rect.right, rect.bottom - borderWidth }, brush, brushTransform);
    }
}
