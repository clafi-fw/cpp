module;
#include <immintrin.h>
module ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    namespace
    {
        // Eight premultiplied pixels laid over eight others, each faded to scale 256ths - the
        // arithmetic compositeSurface does for one pixel, eight at a time.
        void compositeEight(Color* destination, const Color* source, std::uint32_t scale);
    }

    //-------------------------------------------------------------------------

    PixelView::PixelView(const FloatRect& bounds, const FloatRect& clip, const ScanLines& scanLines)
        :
        m_bounds{ bounds },
        m_clipBox{ clip },
        m_scanLines{ scanLines }
    {
    }

    PixelView PixelView::subView(const FloatRect& subBox) const
    {
        // The sub view starts part way into the same allocation, so what remains below and to
        // the right of its first pixel is all it may touch - its own bounds say nothing about
        // that.
        const IntRect owned = ownedPixels();
        const int columnOffset = std::clamp(static_cast<int>(std::floor(subBox.left)) - owned.left, 0, m_scanLines.columns);
        const int rowOffset = std::clamp(static_cast<int>(std::floor(subBox.top)) - owned.top, 0, m_scanLines.rows);
        return PixelView{
            subBox,
            clippedBounds(),
            {
                .data = scanLineAbs(subBox.left, subBox.top),
                .stride = m_scanLines.stride,
                .columns = m_scanLines.columns - columnOffset,
                .rows = m_scanLines.rows - rowOffset,
            }
        };
    }

    void PixelView::fillPixelLine(Color* lineStart, std::size_t lineLength, Color lineColor)
    {
#if 1
        std::fill_n(lineStart, lineLength, lineColor);
#else
        // preparing the first segment of 64 pixels length
        Color* currentPix = lineStart;
        std::size_t bufferLengthInPixels = (std::min)(64ull, lineLength);
        const Color* segmentEnd = currentPix + bufferLengthInPixels;
        while (currentPix < segmentEnd)
        {
            *currentPix = lineColor;
            ++currentPix;
        }
        // propagating prepared 64 pixels segment along the line
        segmentEnd = lineStart + lineLength - bufferLengthInPixels;
        std::size_t buffBytesNum = bufferLengthInPixels * sizeof(Color);
        while (currentPix < segmentEnd)
        {
            std::memcpy(currentPix, lineStart, buffBytesNum);
            currentPix += bufferLengthInPixels;
        }
        // fill remaining tail
        segmentEnd += bufferLengthInPixels;
        while (currentPix < segmentEnd)
        {
            *currentPix = lineColor;
            ++currentPix;
        }
#endif
    }

    void PixelView::fill(Color color) const
    {
        // Rounding the clip out covers the edge pixels it only partly reaches. Meeting that
        // against the owned pixels is what keeps the expansion from writing past the
        // allocation when the clip sits between two device pixels.
        IntRect area = clippedBounds().roundedOut();
        if (!area.intersectWith(ownedPixels()))
        {
            return;
        }

        int width = area.width();
        Color* firstLinePtr = scanLineAbs(static_cast<float>(area.left), static_cast<float>(area.top));
        PixelView::fillPixelLine(firstLinePtr, width, color);

        std::size_t lineSize = width * sizeof(Color);
        const std::intptr_t stride = m_scanLines.stride;
        Color* endLine = firstLinePtr + area.height() * stride;
        for (Color* currentLine = firstLinePtr + stride; currentLine != endLine; currentLine += stride)
        {
            std::memcpy(currentLine, firstLinePtr, lineSize);
        }
    }

    void PixelView::drawSurface(const PixelView& source, const FloatPoint& destPos) const
    {
        // A whole pixel offset, so a surface landing between two device pixels is placed on
        // one of them and each row stays a straight memcpy.
        const int offsetX = static_cast<int>(std::round(destPos.x - source.left()));
        const int offsetY = static_cast<int>(std::round(destPos.y - source.top()));

        // Each side contributes the pixels it owns. Taking the span from the bounds instead
        // lets a surface sitting at a fractional position claim one row and one column more
        // than its allocation holds, and the last row copy then runs off the end of it.
        IntRect area = source.ownedPixels();
        area.offset(offsetX, offsetY);
        if (!area.intersectWith(ownedPixels()))
        {
            return;
        }
        if (!area.intersectWith(clippedBounds().roundedOut()))
        {
            return;
        }

        const std::size_t bytesPerRow = static_cast<std::size_t>(area.width()) * sizeof(Color);
        const float srcX = static_cast<float>(area.left - offsetX);

        for (int y = area.top; y < area.bottom; ++y)
        {
            Color* destRow = scanLineAbs(static_cast<float>(area.left), static_cast<float>(y));
            const Color* srcRow = source.scanLineAbs(srcX, static_cast<float>(y - offsetY));
            std::memcpy(destRow, srcRow, bytesPerRow);
        }
    }

    void PixelView::blendSurface(const PixelView& source, const FloatPoint& destPos,
        float amount) const
    {
        if (amount <= 0.0f)
        {
            return;
        }

        // Placed by the same whole pixel offset as drawSurface, and bounded by the same three
        // rectangles: what the source owns, what this view owns, and what the clip allows.
        const int offsetX = static_cast<int>(std::round(destPos.x - source.left()));
        const int offsetY = static_cast<int>(std::round(destPos.y - source.top()));

        IntRect area = source.ownedPixels();
        area.offset(offsetX, offsetY);
        if (!area.intersectWith(ownedPixels()))
        {
            return;
        }
        if (!area.intersectWith(clippedBounds().roundedOut()))
        {
            return;
        }

        const int width = area.width();
        const float srcX = static_cast<float>(area.left - offsetX);
        // Taken in 1/256ths so the whole row runs in integers, which is the range
        // Color::blend_fixed states. An amount of 1 lands on 256, where the destination keeps
        // nothing and the source arrives exactly.
        const int mix = static_cast<int>(std::clamp(amount, 0.0f, 1.0f) * 256.0f);

        for (int y = area.top; y < area.bottom; ++y)
        {
            Color* destRow = scanLineAbs(static_cast<float>(area.left), static_cast<float>(y));
            const Color* srcRow = source.scanLineAbs(srcX, static_cast<float>(y - offsetY));
            for (int x = 0; x < width; ++x)
            {
                destRow[x].blend_fixed(srcRow[x], mix);
            }
        }
    }

    void PixelView::compositeSurface(const PixelView& source, const FloatPoint& destPos,
        const float opacity) const
    {
        if (opacity <= 0.0f)
        {
            return;
        }

        // Placed and bounded as drawSurface and blendSurface are.
        const int offsetX = static_cast<int>(std::round(destPos.x - source.left()));
        const int offsetY = static_cast<int>(std::round(destPos.y - source.top()));

        IntRect area = source.ownedPixels();
        area.offset(offsetX, offsetY);
        if (!area.intersectWith(ownedPixels()))
        {
            return;
        }
        if (!area.intersectWith(clippedBounds().roundedOut()))
        {
            return;
        }

        const int width = area.width();
        const float srcX = static_cast<float>(area.left - offsetX);
        // In 1/256ths, as blendSurface takes its amount. A premultiplied pixel is faded by
        // scaling all four channels alike.
        const std::uint32_t scale =
            static_cast<std::uint32_t>(std::clamp(opacity, 0.0f, 1.0f) * 256.0f);
        auto faded = [scale](const ColorByte channel) {
            return (static_cast<std::uint32_t>(channel) * scale) >> 8;
        };
        // **Source over, premultiplied**: the source as it stands, and what was there kept by
        // what the source leaves uncovered. Held at 255 for a source that is not premultiplied.
        auto over = [](const std::uint32_t source, const ColorByte destination,
            const std::uint32_t uncovered) {
            const std::uint32_t kept = (destination * uncovered + 127) / 255;
            return static_cast<ColorByte>(std::min<std::uint32_t>(source + kept, 255));
        };
        auto compositePixel = [&](Color& destination, const Color& pixel) {
            const std::uint32_t alpha = faded(pixel.alpha);
            if (alpha == 0)
                return;
            if (alpha == 255)
            {
                destination = pixel;
                return;
            }
            const std::uint32_t uncovered = 255 - alpha;
            destination.red = over(faded(pixel.red), destination.red, uncovered);
            destination.green = over(faded(pixel.green), destination.green, uncovered);
            destination.blue = over(faded(pixel.blue), destination.blue, uncovered);
            destination.alpha = over(alpha, destination.alpha, uncovered);
        };

        // Eight pixels at a time, and the ones left over at the end of a row one at a time.
        for (int y = area.top; y < area.bottom; ++y)
        {
            Color* destRow = scanLineAbs(static_cast<float>(area.left), static_cast<float>(y));
            const Color* srcRow = source.scanLineAbs(srcX, static_cast<float>(y - offsetY));
            int x = 0;
            for (; x + 8 <= width; x += 8)
                compositeEight(destRow + x, srcRow + x, scale);
            for (; x < width; ++x)
                compositePixel(destRow[x], srcRow[x]);
        }
    }

    namespace
    {
        // **Eight clear pixels leave what is there, and eight opaque ones at full opacity replace
        // it,** both found from the alphas alone; only a mixed eight is worked through. There every
        // channel is widened to 16 bits: faded = channel * scale / 256, then the source over what
        // is there as compositeSurface states it, the division by 255 done as a multiply by 0x8081
        // and a shift of 23, exact for every value the product reaches.
        void compositeEight(Color* destination, const Color* source, const std::uint32_t scale)
        {
            const __m256i alphaBits = _mm256_set1_epi32(std::bit_cast<int>(0xFF000000u));
            const __m256i pixels = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source));
            if (_mm256_testz_si256(pixels, alphaBits))
                return;
            __m256i* const target = reinterpret_cast<__m256i*>(destination);
            if (scale == 256)
            {
                const __m256i alphas = _mm256_and_si256(pixels, alphaBits);
                if (_mm256_movemask_epi8(_mm256_cmpeq_epi32(alphas, alphaBits)) == -1)
                {
                    _mm256_storeu_si256(target, pixels);
                    return;
                }
            }

            const __m256i under = _mm256_loadu_si256(target);
            const __m256i zero = _mm256_setzero_si256();
            const __m256i scale16 = _mm256_set1_epi16(static_cast<short>(scale));
            const __m256i full16 = _mm256_set1_epi16(255);
            const __m256i rounding16 = _mm256_set1_epi16(127);
            const __m256i reciprocal16 =
                _mm256_set1_epi16(std::bit_cast<short>(std::uint16_t{ 0x8081 }));
            const auto over = [&](const __m256i source16, const __m256i under16) {
                const __m256i faded = _mm256_srli_epi16(_mm256_mullo_epi16(source16, scale16), 8);
                const __m256i fadedAlpha = _mm256_shufflehi_epi16(
                    _mm256_shufflelo_epi16(faded, 0xFF), 0xFF);
                const __m256i uncovered = _mm256_sub_epi16(full16, fadedAlpha);
                const __m256i product = _mm256_add_epi16(_mm256_mullo_epi16(under16, uncovered),
                    rounding16);
                const __m256i kept = _mm256_srli_epi16(
                    _mm256_mulhi_epu16(product, reciprocal16), 7);
                return _mm256_min_epu16(_mm256_add_epi16(faded, kept), full16);
            };
            const __m256i blended = _mm256_packus_epi16(
                over(_mm256_unpacklo_epi8(pixels, zero), _mm256_unpacklo_epi8(under, zero)),
                over(_mm256_unpackhi_epi8(pixels, zero), _mm256_unpackhi_epi8(under, zero)));

            // A pixel whose alpha fades to nothing is left as it was, whatever its colour holds.
            const __m256i scale32 = _mm256_set1_epi32(static_cast<int>(scale));
            const __m256i fadedAlpha = _mm256_srli_epi32(
                _mm256_mullo_epi32(_mm256_srli_epi32(pixels, 24), scale32), 8);
            const __m256i untouched = _mm256_cmpeq_epi32(fadedAlpha, zero);
            _mm256_storeu_si256(target, _mm256_blendv_epi8(blended, under, untouched));
        }
    }
}
