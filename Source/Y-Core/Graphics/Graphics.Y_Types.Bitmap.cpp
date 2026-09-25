module ClaFi.Core.Graphics.Types;

import :PixelView;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // BitmapData

    void BitmapData::growTo(std::size_t requiredPixels)
    {
        if (Vector::size() < requiredPixels)
        {
            resize(requiredPixels);
        }
    }

    void BitmapData::fill(Color color, std::size_t count)
    {
        std::fill_n(data(), count, color);
    }

    // Bitmap

    Bitmap::Bitmap(IntSize dimensions)
    {
        resize(dimensions);
    }

    // resize() allocates width * height, so both counts are exact and neither grows when a
    // view places the bitmap between two device pixels.
    ScanLines Bitmap::scanLines()
    {
        return {
            .data = data(),
            .stride = stride(),
            .columns = m_dimensions.x,
            .rows = m_dimensions.y,
        };
    }

    PixelView Bitmap::pixelView(const FloatPoint position, const FloatRect& clipRect)
    {
        FloatRect rect = FloatRect::fromDimensions(position, m_dimensions.toFloat());
        return PixelView{ rect, clipRect, scanLines() };
    }

    PixelView Bitmap::pixelView(const FloatPoint position, const FloatRect& clipRect) const
    {
        const FloatRect rect = FloatRect::fromDimensions(position, m_dimensions.toFloat());
        const ScanLines lines = {
            .data = const_cast<Color*>(data()),
            .stride = stride(),
            .columns = m_dimensions.x,
            .rows = m_dimensions.y,
        };
        return PixelView{ rect, clipRect, lines };
    }

    PixelView Bitmap::pixelView(const FloatPoint position, FloatSize dimensions)
    {
        return pixelView(position, FloatRect::fromDimensions(position, dimensions));
    }

    PixelView Bitmap::pixelView(const FloatPoint position)
    {
        return pixelView(position, m_dimensions.toFloat());
    }

    PixelView Bitmap::pixelView()
    {
        return pixelView({}, FloatRect::fromDimensions({}, m_dimensions.toFloat()));
    }

    void Bitmap::resize(IntSize dimensions)
    {
        if (dimensions == m_dimensions)
        {
            return;
        }

        dimensions.x = std::max(dimensions.x, 0);
        dimensions.y = std::max(dimensions.y, 0);
        m_data.resize(static_cast<std::size_t>(dimensions.x * dimensions.y));
        m_dimensions = dimensions;
    }

    Color* Bitmap::pixel(int x, int y)
    {
        return m_data.data() + stride() * y + x;
    }
}
