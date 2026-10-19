export module ClaFi.Core.Graphics.ShadowPainter;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // A rounded rectangle's shadow as baked tiles, cut to the rectangle's shape. See Graphics-Types
    export class ShadowPainter
    {
    public:
        // What the tiles are baked from, in real pixels. Rectangles of one design share the look.
        struct Design
        {
            ShadowParams shadow{};
            CornerRadii radii{};   // the corners of the rectangle that casts it
            [[nodiscard]] bool operator==(const Design&) const = default;
        };
        // How far the shadow reaches past the rectangle on each side, in real pixels.
        struct Reach
        {
            float left{ 0.0f };
            float top{ 0.0f };
            float right{ 0.0f };
            float bottom{ 0.0f };
        };
    public:
        // Bakes the tiles when the design moved, and nothing when it did not.
        void setDesign(const Design&);
        [[nodiscard]] const Design& design() const { return m_design; }
        [[nodiscard]] Reach reach() const;
        // Draws the shadow around rect at opacity, inside the canvas's clip and nowhere else.
        void paint(Canvas&, const FloatRect& rect, float opacity = 1.0f) const;
    private:
        enum class Tile
        {
            TopLeft,
            Top,
            TopRight,
            Right,
            BottomRight,
            Bottom,
            BottomLeft,
            Left
        };
        // The three tiles of a run along a rectangle whose other side is too short for two corner
        // tiles to stand apart: the ends and the piece repeated between, cut across that side.
        struct Strip
        {
            Bitmap head{};
            Bitmap piece{};
            Bitmap tail{};
        };
        using Strips = std::unordered_map<int, Strip>;   // by the short side's length in pixels
        // The reach on each side in whole pixels, and how far a corner tile reaches into the
        // rectangle: past the arc, and past the ramp's inward half.
        struct Extent
        {
            int left{ 0 };
            int top{ 0 };
            int right{ 0 };
            int bottom{ 0 };
            int corner{ 0 };
        };
    private:
        [[nodiscard]] Extent extent() const;
        // The shadow cast around a rectangle of this size on a CPU canvas, the reach around it.
        [[nodiscard]] Bitmap cast(IntSize rectangle) const;
        // Casts the shadow once around a small rectangle and cuts the eight tiles out of it.
        void bake();
        static void cut(Bitmap& into, const Bitmap& from, const IntRect&);
        [[nodiscard]] Bitmap& tile(Tile);
        [[nodiscard]] const Bitmap& tile(Tile) const;
        // The strip for a short side of this length, baked on the first ask and kept.
        [[nodiscard]] const Strip& strip(bool horizontal, int shortSide) const;
        void paintStrip(Canvas&, const FloatRect& clip, const FloatRect& placed, bool horizontal,
            float opacity) const;
        // One tile at position, when the clip reaches it.
        void place(Canvas&, const FloatRect& clip, const Bitmap&, FloatPoint position,
            float opacity) const;
        // The tile repeated along span, cut to it and to the clip.
        void run(Canvas&, const FloatRect& clip, const Bitmap&, const FloatRect& span,
            bool horizontal, float opacity) const;
    private:
        static constexpr std::size_t k_tileCount = 8;
        using Tiles = std::array<Bitmap, k_tileCount>;
        // An edge is as many of these as fit, the last cut by the clip.
        static constexpr int k_edgeTileLength = 64;
        // A short window being dragged bakes a strip per step; a map this full is emptied.
        static constexpr std::size_t k_maxStrips = 8;
        Design m_design{};
        Tiles m_tiles{};
        // mutable: a strip baked on a miss is a record of work done, not state of the design.
        mutable Strips m_shortStrips{};    // along a rectangle shorter than two corners
        mutable Strips m_narrowStrips{};   // down a rectangle narrower than two corners
    };
}
