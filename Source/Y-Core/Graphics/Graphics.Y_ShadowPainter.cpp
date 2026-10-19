module ClaFi.Core.Graphics.ShadowPainter;

import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    namespace
    {
        // The CPU rasterizer's clip as it stands, put back when this goes.
        class HeldRasterClip
        {
        public:
            HeldRasterClip() = default;
            HeldRasterClip(const HeldRasterClip&) = delete;
            HeldRasterClip& operator=(const HeldRasterClip&) = delete;
            ~HeldRasterClip();
        private:
            bool m_active{ Cpu::g_rasterBuffers.hasActiveClip };
            const float* m_mask{ Cpu::g_rasterBuffers.activeClipMaskPtr };
            FloatRect m_box{ Cpu::g_rasterBuffers.clipBox };
            int m_stride{ Cpu::g_rasterBuffers.m_clipStride };
        };
    }

    // ShadowPainter

    void ShadowPainter::setDesign(const Design& value)
    {
        if (value == m_design)
            return;
        m_design = value;
        bake();
        m_shortStrips.clear();
        m_narrowStrips.clear();
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

    // **Baked once and drawn at any opacity**, so a shadow fading in costs one bake for the design
    // rather than one for each step of the fade. **The tiles go on whole pixels**, where each lands
    // whatever position it is given: around an edge between two pixels, the run along it and the
    // corner past it would both draw the column the edge falls in.
    void ShadowPainter::paint(Canvas& canvas, const FloatRect& rect, const float opacity) const
    {
        const ShadowParams& shadow = m_design.shadow;
        if (rect.empty() || opacity <= 0.0f || shadow.blur <= 0.0f || !shadow.color.alpha)
            return;

        const Extent extent = this->extent();
        const float corner = static_cast<float>(extent.corner);
        const bool shortHeight = rect.height() < 2.0f * corner;
        const bool narrowWidth = rect.width() < 2.0f * corner;
        // A corner tile is baked beside a straight edge, so two of them overlapping would each
        // deny the other's arc. A rectangle whose corners stand apart on one axis takes a strip
        // baked at its length on the other; one too small on both is cast directly.
        if (shortHeight && narrowWidth)
        {
            const RoundedRectangleParts silhouette{
                .bounds = rect,
                .radii = m_design.radii
            };
            ShadowParams faded = shadow;
            faded.color.alpha = static_cast<ColorByte>(std::lround(
                static_cast<float>(shadow.color.alpha) * std::min(opacity, 1.0f)));
            canvas.castShadow(silhouette, faded);
            return;
        }

        const FloatRect placed = rect.rounded().toFloat();
        const FloatRect clip = canvas.clipBox();
        if (shortHeight || narrowWidth)
        {
            paintStrip(canvas, clip, placed, shortHeight, opacity);
            return;
        }

        const float left = placed.left - static_cast<float>(extent.left);
        const float top = placed.top - static_cast<float>(extent.top);
        const float innerLeft = placed.left + corner;
        const float innerTop = placed.top + corner;
        const float innerRight = placed.right - corner;
        const float innerBottom = placed.bottom - corner;
        const float depthTop = static_cast<float>(extent.top + extent.corner);
        const float depthRight = static_cast<float>(extent.corner + extent.right);
        const float depthBottom = static_cast<float>(extent.corner + extent.bottom);
        const float depthLeft = static_cast<float>(extent.left + extent.corner);

        place(canvas, clip, tile(Tile::TopLeft), { left, top }, opacity);
        place(canvas, clip, tile(Tile::TopRight), { innerRight, top }, opacity);
        place(canvas, clip, tile(Tile::BottomRight), { innerRight, innerBottom }, opacity);
        place(canvas, clip, tile(Tile::BottomLeft), { left, innerBottom }, opacity);
        run(canvas, clip, tile(Tile::Top),
            { innerLeft, top, innerRight, top + depthTop }, true, opacity);
        run(canvas, clip, tile(Tile::Right),
            { innerRight, innerTop, innerRight + depthRight, innerBottom }, false, opacity);
        run(canvas, clip, tile(Tile::Bottom),
            { innerLeft, innerBottom, innerRight, innerBottom + depthBottom }, true, opacity);
        run(canvas, clip, tile(Tile::Left),
            { left, innerTop, left + depthLeft, innerBottom }, false, opacity);
    }

    ShadowPainter::Extent ShadowPainter::extent() const
    {
        const ShadowParams& shadow = m_design.shadow;
        const Reach reach = this->reach();
        // Along an edge the shadow is the same from the moment the nearest point of the
        // silhouette is straight across, which is past the arc and past wherever the offset and
        // the spread have carried the silhouette; the blur is the ramp's own length inward. Every
        // corner tile is as large as the roundest corner needs.
        const CornerRadii& radii = m_design.radii;
        const float radius = std::max({ radii.topLeft, radii.topRight, radii.bottomRight,
            radii.bottomLeft });
        const float carried = std::max(std::abs(shadow.offset.x), std::abs(shadow.offset.y));
        const float corner = radius + shadow.blur + shadow.spread + carried;
        return {
            static_cast<int>(std::ceil(reach.left)),
            static_cast<int>(std::ceil(reach.top)),
            static_cast<int>(std::ceil(reach.right)),
            static_cast<int>(std::ceil(reach.bottom)),
            static_cast<int>(std::ceil(corner))
        };
    }

    // **The CPU rasterizer's clip is one state for every canvas**, and a canvas states its own on
    // beginPaint and endPaint. A bake runs in the middle of a paint as often as not - see
    // FormBase::shadowPainter - so the clip that paint stated is held across it.
    Bitmap ShadowPainter::cast(const IntSize rectangle) const
    {
        const HeldRasterClip heldClip{};
        const Extent extent = this->extent();
        const IntSize surface = {
            extent.left + rectangle.x + extent.right,
            extent.top + rectangle.y + extent.bottom
        };

        auto backend = std::make_unique<Cpu::CpuBackend>(surface);
        backend->setTransparentBase(true);
        Canvas canvas{ std::move(backend) };
        const IntRect whole = { 0, 0, surface.x, surface.y };
        canvas.beginPaint(nullptr, whole);
        const FloatRect rect = FloatRect::fromDimensions(
            { static_cast<float>(extent.left), static_cast<float>(extent.top) },
            { static_cast<float>(rectangle.x), static_cast<float>(rectangle.y) });
        const RoundedRectangleParts silhouette{
            .bounds = rect,
            .radii = m_design.radii
        };
        canvas.castShadow(silhouette, m_design.shadow);
        Bitmap* painted = nullptr;
        canvas.endPaint(nullptr, whole, painted);
        return *painted;
    }

    void ShadowPainter::bake()
    {
        const Extent extent = this->extent();
        // Two corner reaches and one edge tile along each side, so every tile is cut out of a run
        // of shadow that is straight where the tile will be drawn straight.
        const int side = 2 * extent.corner + k_edgeTileLength;
        const Bitmap painted = cast({ side, side });
        const int x1 = extent.left + extent.corner;
        const int x2 = x1 + k_edgeTileLength;
        const int y1 = extent.top + extent.corner;
        const int y2 = y1 + k_edgeTileLength;
        const int right = painted.width();
        const int bottom = painted.height();
        cut(tile(Tile::TopLeft), painted, { 0, 0, x1, y1 });
        cut(tile(Tile::Top), painted, { x1, 0, x2, y1 });
        cut(tile(Tile::TopRight), painted, { x2, 0, right, y1 });
        cut(tile(Tile::Right), painted, { x2, y1, right, y2 });
        cut(tile(Tile::BottomRight), painted, { x2, y2, right, bottom });
        cut(tile(Tile::Bottom), painted, { x1, y2, x2, bottom });
        cut(tile(Tile::BottomLeft), painted, { 0, y2, x1, bottom });
        cut(tile(Tile::Left), painted, { 0, y1, x1, y2 });
    }

    void ShadowPainter::cut(Bitmap& into, const Bitmap& from, const IntRect& rect)
    {
        into.resize(rect.dimensions());
        const FloatPoint size = {
            static_cast<float>(from.width()),
            static_cast<float>(from.height())
        };
        const PixelView source = from.pixelView({}, FloatRect::fromDimensions({}, size));
        // The source lands with rect's corner on the tile's origin, and the tile's own extent
        // bounds the copy.
        const FloatPoint origin = { static_cast<float>(-rect.left), static_cast<float>(-rect.top) };
        into.pixelView().drawSurface(source, origin);
    }

    Bitmap& ShadowPainter::tile(Tile which)
    {
        return m_tiles[static_cast<std::size_t>(which)];
    }

    const Bitmap& ShadowPainter::tile(Tile which) const
    {
        return m_tiles[static_cast<std::size_t>(which)];
    }

    // The shadow is cast around a rectangle of the short side's length and one straight run - two
    // corner reaches and an edge tile - along the other, and cut across the short side whole, so
    // both arcs and whatever the two reach of each other are in every tile.
    const ShadowPainter::Strip& ShadowPainter::strip(const bool horizontal,
        const int shortSide) const
    {
        Strips& strips = horizontal ? m_shortStrips : m_narrowStrips;
        const Strips::iterator found = strips.find(shortSide);
        if (found != strips.end())
            return found->second;
        if (strips.size() >= k_maxStrips)
            strips.clear();

        const Extent extent = this->extent();
        const int side = 2 * extent.corner + k_edgeTileLength;
        const IntSize rectangle = horizontal
            ? IntSize{ side, shortSide }
            : IntSize{ shortSide, side };
        const Bitmap painted = cast(rectangle);
        const int right = painted.width();
        const int bottom = painted.height();
        const int first = (horizontal ? extent.left : extent.top) + extent.corner;
        const int second = first + k_edgeTileLength;
        Strip& strip = strips[shortSide];
        if (horizontal)
        {
            cut(strip.head, painted, { 0, 0, first, bottom });
            cut(strip.piece, painted, { first, 0, second, bottom });
            cut(strip.tail, painted, { second, 0, right, bottom });
        }
        else
        {
            cut(strip.head, painted, { 0, 0, right, first });
            cut(strip.piece, painted, { 0, first, right, second });
            cut(strip.tail, painted, { 0, second, right, bottom });
        }
        return strip;
    }

    void ShadowPainter::paintStrip(Canvas& canvas, const FloatRect& clip, const FloatRect& placed,
        const bool horizontal, const float opacity) const
    {
        const Extent extent = this->extent();
        const int shortSide = static_cast<int>(horizontal ? placed.height() : placed.width());
        const Strip& strip = this->strip(horizontal, shortSide);
        const float corner = static_cast<float>(extent.corner);
        const float left = placed.left - static_cast<float>(extent.left);
        const float top = placed.top - static_cast<float>(extent.top);
        place(canvas, clip, strip.head, { left, top }, opacity);
        if (horizontal)
        {
            const float innerLeft = placed.left + corner;
            const float innerRight = placed.right - corner;
            const float depth = static_cast<float>(strip.piece.height());
            place(canvas, clip, strip.tail, { innerRight, top }, opacity);
            run(canvas, clip, strip.piece, { innerLeft, top, innerRight, top + depth }, true,
                opacity);
        }
        else
        {
            const float innerTop = placed.top + corner;
            const float innerBottom = placed.bottom - corner;
            const float depth = static_cast<float>(strip.piece.width());
            place(canvas, clip, strip.tail, { left, innerBottom }, opacity);
            run(canvas, clip, strip.piece, { left, innerTop, left + depth, innerBottom }, false,
                opacity);
        }
    }

    void ShadowPainter::place(Canvas& canvas, const FloatRect& clip, const Bitmap& bitmap,
        FloatPoint position, const float opacity) const
    {
        const FloatRect rect = FloatRect::fromDimensions(position,
            { static_cast<float>(bitmap.width()), static_cast<float>(bitmap.height()) });
        if (FloatRect::intersection(rect, clip).empty())
            return;
        canvas.drawBitmap(bitmap, position, opacity);
    }

    void ShadowPainter::run(Canvas& canvas, const FloatRect& clip, const Bitmap& bitmap,
        const FloatRect& span, const bool horizontal, const float opacity) const
    {
        const FloatRect visible = FloatRect::intersection(span, clip);
        if (visible.empty())
            return;
        const float length = static_cast<float>(k_edgeTileLength);
        // Pushed as the clip: the last tile runs into the corner's reach, and the tiles are
        // whole whatever the span's length is.
        canvas.pushClip(visible);
        if (horizontal)
        {
            const float skipped = std::floor((visible.left - span.left) / length) * length;
            for (float x = span.left + skipped; x < visible.right; x += length)
                canvas.drawBitmap(bitmap, { x, span.top }, opacity);
        }
        else
        {
            const float skipped = std::floor((visible.top - span.top) / length) * length;
            for (float y = span.top + skipped; y < visible.bottom; y += length)
                canvas.drawBitmap(bitmap, { span.left, y }, opacity);
        }
        canvas.popClip();
    }

    namespace
    {
        // HeldRasterClip

        HeldRasterClip::~HeldRasterClip()
        {
            Cpu::g_rasterBuffers.hasActiveClip = m_active;
            Cpu::g_rasterBuffers.activeClipMaskPtr = m_mask;
            Cpu::g_rasterBuffers.clipBox = m_box;
            Cpu::g_rasterBuffers.m_clipStride = m_stride;
        }
    }
}
