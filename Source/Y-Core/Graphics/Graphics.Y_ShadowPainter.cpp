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
        // A rectangle too small for its corners to stand apart is cast directly: a corner tile is
        // baked beside a straight edge, and two of them overlapping would each deny the other's
        // arc.
        if (rect.width() < 2.0f * corner || rect.height() < 2.0f * corner)
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

        place(canvas, clip, Tile::TopLeft, { left, top }, opacity);
        place(canvas, clip, Tile::TopRight, { innerRight, top }, opacity);
        place(canvas, clip, Tile::BottomRight, { innerRight, innerBottom }, opacity);
        place(canvas, clip, Tile::BottomLeft, { left, innerBottom }, opacity);
        run(canvas, clip, Tile::Top, { innerLeft, top, innerRight, top + depthTop }, opacity);
        run(canvas, clip, Tile::Right,
            { innerRight, innerTop, innerRight + depthRight, innerBottom }, opacity);
        run(canvas, clip, Tile::Bottom,
            { innerLeft, innerBottom, innerRight, innerBottom + depthBottom }, opacity);
        run(canvas, clip, Tile::Left, { left, innerTop, left + depthLeft, innerBottom }, opacity);
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
    void ShadowPainter::bake()
    {
        const HeldRasterClip heldClip{};
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
            .radii = m_design.radii
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
        FloatPoint position, const float opacity) const
    {
        const Bitmap& bitmap = tile(which);
        const FloatRect rect = FloatRect::fromDimensions(position,
            { static_cast<float>(bitmap.width()), static_cast<float>(bitmap.height()) });
        if (FloatRect::intersection(rect, clip).empty())
            return;
        canvas.drawBitmap(bitmap, position, opacity);
    }

    void ShadowPainter::run(Canvas& canvas, const FloatRect& clip, Tile which,
        const FloatRect& span, const float opacity) const
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
