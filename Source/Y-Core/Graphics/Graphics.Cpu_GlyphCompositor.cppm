export module ClaFi.Core.Graphics.Cpu_GlyphCompositor;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{
    // Coverage for one glyph, rasterized at a whole pixel origin. See Graphics-Types
    export struct GlyphCoverage
    {
        int left{ 0 };
        int top{ 0 };
        int width{ 0 };
        int height{ 0 };
        int padStride{ 0 };
        std::vector<float> coverage{};

        // Sizes the padded buffer for ink of that size and clears it. left and top are the
        // caller's to state.
        void allocate(int inkWidth, int inkHeight);
        // The first ink pixel of row y, inside the border.
        [[nodiscard]] float* row(int y);
        [[nodiscard]] const float* row(int y) const;
        [[nodiscard]] bool hasInk() const { return width > 0 && height > 0; }
        // Where the ink stands across the rows between two heights, from its leftmost pixel to its
        // rightmost, as offsets from the origin - or nothing where none of it is there. The
        // heights are measured from the origin the way top is.
        [[nodiscard]] std::optional<InkExtent> inkAcross(float bandTop, float bandBottom) const;
    };

    // One glyph's coverage and where its ink stands on the surface. See Graphics-Types
    export struct PlacedGlyph
    {
        const GlyphCoverage* coverage;
        float x;
        float y;
    };

    // Draws one run of glyphs through a single coverage mask. See Graphics-Types
    export class GlyphCompositor
    {
    public:
        // Starts a run: forgets the placed glyphs and their bounds.
        void begin();
        // Places a glyph by its origin on the surface - the pen position with the glyph's own
        // offsets and any transform already applied. The bearings are added here. A coverage with
        // no ink places nothing.
        void place(const GlyphCoverage&, FloatPoint origin);
        [[nodiscard]] bool empty() const { return m_placed.empty(); }
        // The pixels the placed ink reaches, in surface coordinates. Meaningful once something is
        // placed.
        [[nodiscard]] const FloatRect& bounds() const { return m_bounds; }
        // Accumulates the placed glyphs into one mask over bounds() and composites the brush
        // through it, once, onto the backend's staging pixels for that rectangle.
        void composite(IBackend&, const Brush&);
    private:
        // Held across runs so a run does not allocate to lay its glyphs out.
        std::vector<PlacedGlyph> m_placed{};
        FloatRect m_bounds{};
    };
}
