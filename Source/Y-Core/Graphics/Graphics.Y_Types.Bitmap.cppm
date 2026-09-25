export module ClaFi.Core.Graphics.Types :Bitmap;

import :PixelView;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    using Vector = std::vector<Color, NoInitAllocator<Color>>;

    export class BitmapData : private Vector
    {
    public:
        using Vector::data;
        using Vector::resize;
        void growTo(std::size_t requiredPixels);
        void fill(Color, std::size_t);
    };

    export class Bitmap
    {
    public:
        Bitmap() = default;
        Bitmap(IntSize);
        Color* data() { return m_data.data(); }
        const Color* data() const { return m_data.data(); }
        std::intptr_t stride() const { return m_dimensions.x; }
        ScanLines scanLines();
        PixelView pixelView(const FloatPoint position, const FloatRect& clipRect);
        // A view for reading: the one handle a view has, on a bitmap nothing here draws into.
        PixelView pixelView(const FloatPoint position, const FloatRect& clipRect) const;
        PixelView pixelView(const FloatPoint position, FloatSize);
        PixelView pixelView(const FloatPoint position);
        PixelView pixelView();
        int width() const { return m_dimensions.x; }
        int height() const { return m_dimensions.y; }
        void resize(IntSize);
        bool empty() const { return !(m_dimensions.x && m_dimensions.y); }
        Color* pixel(int x, int y);
        inline void setPixel(int x, int y, Color value) { *pixel(x, y) = value; }
    private:
        IntSize m_dimensions{};
        BitmapData m_data{};
    };

}
