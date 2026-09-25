module ClaFi.Core.Graphics.ShadowPainter;

import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    void ShadowPainter::setDesign(const Design& value)
    {
        if (value == m_design)
            return;
        m_design = value;
        bake();
    }

    ShadowPainter::Reach ShadowPainter::reach() const
    {
        const ShadowParams& shadow = m_design.shadow;
        const float out = shadow.blur + shadow.spread;
        return {
            std::max(0.0f, out - shadow.offset.x),
            std::max(0.0f, out - shadow.offset.y),
            std::max(0.0f, out + shadow.offset.x),
            std::max(0.0f, out + shadow.offset.y)
        };
    }

    void ShadowPainter::paint(Canvas& canvas, const FloatRect& windowRect) const
    {
        const ShadowParams& shadow = m_design.shadow;
        if (windowRect.empty() || shadow.blur <= 0.0f || !shadow.color.alpha)
            return;

        const Extent extent = this->extent();
        const float corner = static_cast<float>(extent.corner);
        // A rectangle too small for its corners to stand apart is cast directly: a corner tile is
        // baked beside a straight edge, and two of them overlapping would each deny the other's
        // arc.
        if (windowRect.width() < 2.0f * corner || windowRect.height() < 2.0f * corner)
        {
            const RoundedRectangleParts silhouette{
                .bounds = windowRect,
                .radii = uniformCorners(m_design.radius)
            };
            canvas.castShadow(silhouette, shadow);
            return;
        }

        const FloatRect clip = canvas.clipBox();
        const float left = windowRect.left - static_cast<float>(extent.left);
        const float top = windowRect.top - static_cast<float>(extent.top);
        const float innerLeft = windowRect.left + corner;
        const float innerTop = windowRect.top + corner;
        const float innerRight = windowRect.right - corner;
        const float innerBottom = windowRect.bottom - corner;
        const float depthTop = static_cast<float>(extent.top + extent.corner);
        const float depthRight = static_cast<float>(extent.corner + extent.right);
        const float depthBottom = static_cast<float>(extent.corner + extent.bottom);
        const float depthLeft = static_cast<float>(extent.left + extent.corner);

        place(canvas, clip, Tile::TopLeft, { left, top });
        place(canvas, clip, Tile::TopRight, { innerRight, top });
        place(canvas, clip, Tile::BottomRight, { innerRight, innerBottom });
        place(canvas, clip, Tile::BottomLeft, { left, innerBottom });
        run(canvas, clip, Tile::Top, { innerLeft, top, innerRight, top + depthTop });
        run(canvas, clip, Tile::Right,
            { innerRight, innerTop, innerRight + depthRight, innerBottom });
        run(canvas, clip, Tile::Bottom,
            { innerLeft, innerBottom, innerRight, innerBottom + depthBottom });
        run(canvas, clip, Tile::Left, { left, innerTop, left + depthLeft, innerBottom });
    }

    ShadowPainter::Extent ShadowPainter::extent() const
    {
        const ShadowParams& shadow = m_design.shadow;
        const Reach reach = this->reach();
        // Along an edge the shadow is the same from the moment the nearest point of the
        // silhouette is straight across, which is past the arc and past wherever the offset and
        // the spread have carried the silhouette; the blur is the ramp's own length inward.
        const float carried = std::max(std::abs(shadow.offset.x), std::abs(shadow.offset.y));
        const float corner = m_design.radius + shadow.blur + shadow.spread + carried;
        return {
            static_cast<int>(std::ceil(reach.left)),
            static_cast<int>(std::ceil(reach.top)),
            static_cast<int>(std::ceil(reach.right)),
            static_cast<int>(std::ceil(reach.bottom)),
            static_cast<int>(std::ceil(corner))
        };
    }

    void ShadowPainter::bake()
    {
        const Extent extent = this->extent();
        // Two corner reaches and one edge tile along each side, so every tile is cut out of a run
        // of shadow that is straight where the tile will be drawn straight.
        const int side = 2 * extent.corner + k_edgeTileLength;
        const IntSize surface = {
            extent.left + side + extent.right,
            extent.top + side + extent.bottom
        };

        auto backend = std::make_unique<Cpu::CpuBackend>(surface);
        backend->setTransparentBase(true);
        Canvas canvas{ std::move(backend) };
        const IntRect whole = { 0, 0, surface.x, surface.y };
        canvas.beginPaint(nullptr, whole);
        const FloatRect rect = FloatRect::fromDimensions(
            { static_cast<float>(extent.left), static_cast<float>(extent.top) },
            { static_cast<float>(side), static_cast<float>(side) });
        const RoundedRectangleParts silhouette{
            .bounds = rect,
            .radii = uniformCorners(m_design.radius)
        };
        canvas.castShadow(silhouette, m_design.shadow);
        Bitmap* painted = nullptr;
        canvas.endPaint(nullptr, whole, painted);

        const int x1 = extent.left + extent.corner;
        const int x2 = x1 + k_edgeTileLength;
        const int y1 = extent.top + extent.corner;
        const int y2 = y1 + k_edgeTileLength;
        cut(Tile::TopLeft, *painted, { 0, 0, x1, y1 });
        cut(Tile::Top, *painted, { x1, 0, x2, y1 });
        cut(Tile::TopRight, *painted, { x2, 0, surface.x, y1 });
        cut(Tile::Right, *painted, { x2, y1, surface.x, y2 });
        cut(Tile::BottomRight, *painted, { x2, y2, surface.x, surface.y });
        cut(Tile::Bottom, *painted, { x1, y2, x2, surface.y });
        cut(Tile::BottomLeft, *painted, { 0, y2, x1, surface.y });
        cut(Tile::Left, *painted, { 0, y1, x1, y2 });
    }

    void ShadowPainter::cut(Tile which, const Bitmap& from, const IntRect& rect)
    {
        Bitmap& tile = m_tiles[static_cast<std::size_t>(which)];
        tile.resize(rect.dimensions());
        const FloatPoint size = {
            static_cast<float>(from.width()),
            static_cast<float>(from.height())
        };
        const PixelView source = from.pixelView({}, FloatRect::fromDimensions({}, size));
        // The source lands with rect's corner on the tile's origin, and the tile's own extent
        // bounds the copy.
        const FloatPoint origin = { static_cast<float>(-rect.left), static_cast<float>(-rect.top) };
        tile.pixelView().drawSurface(source, origin);
    }

    const Bitmap& ShadowPainter::tile(Tile which) const
    {
        return m_tiles[static_cast<std::size_t>(which)];
    }

    void ShadowPainter::place(Canvas& canvas, const FloatRect& clip, Tile which,
        FloatPoint position) const
    {
        const Bitmap& bitmap = tile(which);
        const FloatRect rect = FloatRect::fromDimensions(position,
            { static_cast<float>(bitmap.width()), static_cast<float>(bitmap.height()) });
        if (FloatRect::intersection(rect, clip).empty())
            return;
        canvas.drawBitmap(bitmap, position);
    }

    void ShadowPainter::run(Canvas& canvas, const FloatRect& clip, Tile which,
        const FloatRect& span) const
    {
        const FloatRect visible = FloatRect::intersection(span, clip);
        if (visible.empty())
            return;
        const Bitmap& bitmap = tile(which);
        const bool horizontal = which == Tile::Top || which == Tile::Bottom;
        const float length = static_cast<float>(k_edgeTileLength);
        // Pushed as the clip: the last tile runs into the corner's reach, and the tiles are
        // whole whatever the span's length is.
        canvas.pushClip(visible);
        if (horizontal)
        {
            const float skipped = std::floor((visible.left - span.left) / length) * length;
            for (float x = span.left + skipped; x < visible.right; x += length)
                canvas.drawBitmap(bitmap, { x, span.top });
        }
        else
        {
            const float skipped = std::floor((visible.top - span.top) / length) * length;
            for (float y = span.top + skipped; y < visible.bottom; y += length)
                canvas.drawBitmap(bitmap, { span.left, y });
        }
        canvas.popClip();
    }
}
