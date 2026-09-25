module ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
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

    void PixelView::fill(Color color, Opacity opacity) const
    {
        // Rounding the clip out covers the edge pixels it only partly reaches. Meeting that
        // against the owned pixels is what keeps the expansion from writing past the
        // allocation when the clip sits between two device pixels.
        IntRect area = clippedBounds().roundedOut();
        if (!area.intersectWith(ownedPixels()))
        {
            return;
        }

        color.alpha = static_cast<ColorByte>(color.alpha * opacity);

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
}
