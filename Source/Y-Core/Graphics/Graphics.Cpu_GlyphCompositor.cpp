module;
#include <immintrin.h>
module ClaFi.Core.Graphics.Cpu_GlyphCompositor;

import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{
    namespace
    {
        // Coverage a pixel needs to count as ink. A fainter fringe is antialiasing, and reads as
        // the gap around a glyph rather than as part of it.
        constexpr float k_inkThreshold = 0.2f;
    }

    void GlyphCoverage::allocate(int inkWidth, int inkHeight)
    {
        width = inkWidth;
        height = inkHeight;
        padStride = inkWidth + 2;
        coverage.assign(static_cast<std::size_t>(padStride) * (inkHeight + 2), 0.0f);
    }

    float* GlyphCoverage::row(int y)
    {
        return &coverage[static_cast<std::size_t>(y + 1) * padStride + 1];
    }

    const float* GlyphCoverage::row(int y) const
    {
        return &coverage[static_cast<std::size_t>(y + 1) * padStride + 1];
    }

    std::optional<InkExtent> GlyphCoverage::inkAcross(float bandTop, float bandBottom) const
    {
        // A row stands in the band where any of its height does.
        const int firstRow = std::max(0, static_cast<int>(std::floor(bandTop)) - top);
        const int endRow = std::min(height, static_cast<int>(std::ceil(bandBottom)) - top);

        int leftmost = width;
        int rightmost = -1;
        for (int y = firstRow; y < endRow; ++y)
        {
            const float* pixels = row(y);
            for (int x = 0; x < leftmost; ++x)
            {
                if (pixels[x] > k_inkThreshold)
                {
                    leftmost = x;
                    break;
                }
            }
            for (int x = width - 1; x > rightmost; --x)
            {
                if (pixels[x] > k_inkThreshold)
                {
                    rightmost = x;
                    break;
                }
            }
        }

        if (rightmost < leftmost)
            return std::nullopt;
        return InkExtent{
            static_cast<float>(left + leftmost),
            static_cast<float>(left + rightmost + 1),
        };
    }

    void GlyphCompositor::begin()
    {
        m_placed.clear();
        m_bounds = { 1e10f, 1e10f, -1e10f, -1e10f };
    }

    void GlyphCompositor::place(const GlyphCoverage& glyph, FloatPoint origin)
    {
        if (!glyph.hasInk())
            return;

        const float left = origin.x + static_cast<float>(glyph.left);
        const float top = origin.y + static_cast<float>(glyph.top);
        m_placed.push_back({ &glyph, left, top });

        m_bounds.left = std::min(m_bounds.left, left);
        m_bounds.top = std::min(m_bounds.top, top);
        m_bounds.right = std::max(m_bounds.right, left + static_cast<float>(glyph.width));
        m_bounds.bottom = std::max(m_bounds.bottom, top + static_cast<float>(glyph.height));
    }

    void GlyphCompositor::composite(IBackend& backend, const Brush& brush)
    {
        if (m_placed.empty())
            return;

        PixelView target = backend.stagingView(m_bounds);
        if (!target.data())
            return;

        // RasterBuffers::composite walks the surface a scanline at a time, from floor(top) to
        // ceil(bottom), and indexes the mask in those same rows. On a fractional origin that span
        // is a row taller and a column wider than the ink, so the mask is sized the same way.
        // prepareForEffect clears exactly what it allocates, and the resize behind it is
        // deliberately no-init, so a row past the end reads whatever the last run left and paints
        // it as coverage.
        const int maskLeft = static_cast<int>(std::floor(m_bounds.left));
        const int maskTop = static_cast<int>(std::floor(m_bounds.top));
        const int maskCols = static_cast<int>(std::ceil(m_bounds.right)) - maskLeft;
        const int maskRows = static_cast<int>(std::ceil(m_bounds.bottom)) - maskTop;
        g_rasterBuffers.prepareForEffect(maskCols, maskRows);

        const __m256 vOne = _mm256_set1_ps(1.0f);
        for (const PlacedGlyph& placed : m_placed)
        {
            const GlyphCoverage& glyph = *placed.coverage;
            const int padStride = glyph.padStride;

            const float fx = placed.x - std::floor(placed.x);
            const float fy = placed.y - std::floor(placed.y);
            const int baseCol = static_cast<int>(std::floor(placed.x)) - maskLeft;
            const int baseRow = static_cast<int>(std::floor(placed.y)) - maskTop;
            const bool exact = fx == 0.0f && fy == 0.0f;

            // Sampling at a fixed offset means every output pixel takes the same four weights -
            // x - fx always lands one source pixel back with the same remainder - so they are
            // computed once here rather than refloored per pixel.
            const float wRight = 1.0f - fx;
            const float wLower = 1.0f - fy;
            const float wUpperLeftF = fx * fy;
            const float wUpperRightF = wRight * fy;
            const float wLowerLeftF = fx * wLower;
            const float wLowerRightF = wRight * wLower;
            const __m256 vUpperLeft = _mm256_set1_ps(wUpperLeftF);
            const __m256 vUpperRight = _mm256_set1_ps(wUpperRightF);
            const __m256 vLowerLeft = _mm256_set1_ps(wLowerLeftF);
            const __m256 vLowerRight = _mm256_set1_ps(wLowerRightF);

            // An exact placement covers the ink only; a sampled one spreads one further.
            const int spanCols = exact ? glyph.width : glyph.width + 1;
            const int spanRows = exact ? glyph.height : glyph.height + 1;

            // Clipped once, so neither loop below tests a bound.
            const int yStart = std::max(0, -baseRow);
            const int yEnd = std::min(spanRows, maskRows - baseRow);
            const int xStart = std::max(0, -baseCol);
            const int xEnd = std::min(spanCols, maskCols - baseCol);

            for (int y = yStart; y < yEnd; ++y)
            {
                float* dst = &g_rasterBuffers.mask[static_cast<std::size_t>(baseRow + y) * g_rasterBuffers.m_maskStride + baseCol];

                // Coverage accumulates and saturates, so overlapping glyphs reach full ink where
                // they cross instead of compounding into a dark seam. The padded border guarantees
                // every load below is inside the buffer, including the one column the 2x2 tap
                // reads ahead.
                int x = xStart;
                if (exact)
                {
                    const float* src = &glyph.coverage[static_cast<std::size_t>(y + 1) * padStride + 1];
                    for (; x + 8 <= xEnd; x += 8)
                    {
                        __m256 acc = _mm256_add_ps(_mm256_loadu_ps(dst + x), _mm256_loadu_ps(src + x));
                        _mm256_storeu_ps(dst + x, _mm256_min_ps(vOne, acc));
                    }
                    for (; x < xEnd; ++x)
                        dst[x] = std::min(1.0f, dst[x] + src[x]);
                }
                else
                {
                    const float* upper = &glyph.coverage[static_cast<std::size_t>(y) * padStride];
                    const float* lower = upper + padStride;
                    for (; x + 8 <= xEnd; x += 8)
                    {
                        __m256 value = _mm256_mul_ps(_mm256_loadu_ps(upper + x), vUpperLeft);
                        value = _mm256_fmadd_ps(_mm256_loadu_ps(upper + x + 1), vUpperRight, value);
                        value = _mm256_fmadd_ps(_mm256_loadu_ps(lower + x), vLowerLeft, value);
                        value = _mm256_fmadd_ps(_mm256_loadu_ps(lower + x + 1), vLowerRight, value);
                        __m256 acc = _mm256_add_ps(_mm256_loadu_ps(dst + x), value);
                        _mm256_storeu_ps(dst + x, _mm256_min_ps(vOne, acc));
                    }
                    for (; x < xEnd; ++x)
                    {
                        const float value =
                            upper[x] * wUpperLeftF + upper[x + 1] * wUpperRightF +
                            lower[x] * wLowerLeftF + lower[x + 1] * wLowerRightF;
                        dst[x] = std::min(1.0f, dst[x] + value);
                    }
                }
            }
        }

        g_rasterBuffers.composite(target, brush, 1.0f);
    }
}
