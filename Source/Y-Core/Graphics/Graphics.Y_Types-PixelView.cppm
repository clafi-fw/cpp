export module ClaFi.Core.Graphics.Types :PixelView;

import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // The lines of whole pixels a view may address, and how they sit in memory. See Graphics-Types
    export struct ScanLines
    {
        Color* data{ nullptr };
        // Pixels from one line's start to the next. Wider than columns whenever the lines are
        // part of a larger allocation, which is what a subView addresses.
        std::intptr_t stride{ 0 };
        // Addressable pixels in a line, and addressable lines.
        int columns{ 0 };
        int rows{ 0 };
    };

    export class PixelView
    {
    public:
        PixelView(const FloatRect& bounds, const FloatRect& clip, const ScanLines&);
        PixelView subView(const FloatRect& subBox) const;
        // A view is a handle onto pixels it does not own, so const propagates to the handle and
        // not to what it addresses - a const view still paints.
        inline const ScanLines& scanLines() const { return m_scanLines; }
        inline Color* data() const { return m_scanLines.data; }
        inline Color* scanLineRel(int yRel) const { return m_scanLines.data + yRel * m_scanLines.stride; }
        inline Color* scanLineAbs(float yAbs) const {
            return m_scanLines.data + (static_cast<int>(std::floor(yAbs)) - static_cast<int>(std::floor(m_bounds.top))) * m_scanLines.stride;
        }
        inline Color* scanLineAbs(float xAbs, float yAbs) const {
            return m_scanLines.data + (static_cast<int>(std::floor(yAbs)) - static_cast<int>(std::floor(m_bounds.top))) * m_scanLines.stride
                + (static_cast<int>(std::floor(xAbs)) - static_cast<int>(std::floor(m_bounds.left)));
        }
        inline Color* pixelAbs(float xAbs, float yAbs, bool checkClippedBounds = true) const;
        inline Color* pixelAbs(FloatPoint, bool checkClippedBounds = true) const;
        inline std::intptr_t stride() const { return m_scanLines.stride; }
        inline int columns() const { return m_scanLines.columns; }
        inline int rows() const { return m_scanLines.rows; }
        // The whole pixel rectangle the scan lines back, in absolute coordinates. This is the
        // span every read and every write is clamped to.
        inline IntRect ownedPixels() const;
        // The safe area the render engine is allowed to modify
        inline const FloatRect& clipBox() const { return m_clipBox; }
        // The absolute memory boundaries of the requested UI element
        inline const FloatRect& bounds() const { return m_bounds; }
        inline const FloatRect clippedBounds() const { return FloatRect::intersection(m_bounds, m_clipBox); }

        inline float top() const { return m_bounds.top; }
        inline float left() const { return m_bounds.left; }
        inline float right() const { return m_bounds.right; }
        inline float bottom() const { return m_bounds.bottom; }
        inline float width() const { return m_bounds.width(); }
        inline float height() const { return m_bounds.height(); }
        inline FloatPoint topLeft() const { return m_bounds.topLeft(); }
        inline FloatPoint bottomRight() const { return m_bounds.bottomRight(); }
        inline FloatPoint topRight() const { return m_bounds.topRight(); }
        inline FloatPoint bottomLeft() const { return m_bounds.bottomLeft(); }
        inline FloatPoint center() const { return m_bounds.center(); }
        inline float centerX() const { return m_bounds.centerX(); }
        inline float centerY() const { return m_bounds.centerY(); }
        inline FloatPoint topCenter() const { return m_bounds.topCenter(); }
        inline FloatPoint bottomCenter() const { return m_bounds.bottomCenter(); }
        inline FloatPoint dimensions() const { return m_bounds.dimensions(); }
        static void fillPixelLine(Color* lineStart, std::size_t lineLength, Color);
        void fill(Color, Opacity = 1.0f) const;
        void copyFrom(const PixelView& source) const { drawSurface(source, source.topLeft()); }
        void drawSurface(const PixelView& source, const FloatPoint& destPos) const;
        // The same placement as drawSurface, with each source pixel mixed into what is already
        // there by this amount rather than replacing it: 0 leaves the destination as it stands, 1
        // is drawSurface. Both surfaces are read as ordinary pixels and the source's own alpha
        // takes no part - this mixes two images, it does not composite one over the other.
        void blendSurface(const PixelView& source, const FloatPoint& destPos, float amount) const;
    private:
        FloatRect m_bounds;
        FloatRect m_clipBox;
        ScanLines m_scanLines;
    };


    //-------------------------------------------------------------------------


    inline Color* PixelView::pixelAbs(float xAbs, float yAbs, bool checkClippedBounds) const
    {
        if (checkClippedBounds && !clippedBounds().contains(xAbs, yAbs))
            return nullptr;
        return scanLineAbs(xAbs, yAbs);
    }

    inline Color* PixelView::pixelAbs(FloatPoint pt, bool checkClippedBounds) const
    {
        return pixelAbs(pt.x, pt.y, checkClippedBounds);
    }

    inline IntRect PixelView::ownedPixels() const
    {
        const int left = static_cast<int>(std::floor(m_bounds.left));
        const int top = static_cast<int>(std::floor(m_bounds.top));
        return { left, top, left + m_scanLines.columns, top + m_scanLines.rows };
    }

}
