export module ClaFi.Core.TextEngine.Mono;

import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // A glyph of a face, by the index its cmap answers. Zero is the missing glyph in every
    // OpenType face, so it stands for no glyph here.
    export using MonoGlyph = std::uint16_t;
    export constexpr MonoGlyph k_noMonoGlyph = 0;
    export using MonoGlyphs = std::vector<MonoGlyph>;

    // One face at one size, the size in the units a layout works in. See TextEngine-Types
    export struct MonoFontRequest
    {
        std::wstring family;
        float size;
        FontWeight weight;
        FontStyle style;
    };

    // A monospace face at one size, as the platform answers about it. See TextEngine-Types
    export class INativeMonoFont
    {
    public:
        virtual ~INativeMonoFont() = default;
    public:
        [[nodiscard]] virtual float cellWidth() const = 0;
        [[nodiscard]] virtual float lineHeight() const = 0;
        [[nodiscard]] virtual float baseline() const = 0;
        // The face's glyph for the code point where that glyph is one cell wide, else none.
        [[nodiscard]] virtual MonoGlyph glyphOf(char32_t) const = 0;
        // One run of glyphs a cell apart, the first standing at the origin.
        virtual void drawRun(ControlPaintContext&, FloatPoint baselineOrigin,
            std::span<const MonoGlyph>, const Graphics::Brush&) = 0;
        // Where one glyph puts ink between two heights, as offsets from its baseline origin, or
        // nothing where it puts none there.
        [[nodiscard]] virtual std::optional<Graphics::InkExtent> inkAcross(MonoGlyph,
            float bandTop, float bandBottom) = 0;
    };

    // The platform's font for a request, or null where the request is not a monospace face.
    export [[nodiscard]] std::unique_ptr<INativeMonoFont> createNativeMonoFont(
        const MonoFontRequest&);

    // What a line of cells comes to, with and without its trailing whitespace.
    export struct MonoExtent
    {
        float width;
        float inkWidth;
    };

    // What one line is drawn with, over and above its text and its origin.
    export struct MonoLineDraw
    {
        // The box the line stands in; a fade runs out over its right edge.
        FloatRect bounds;
        // What is on screen: a run ending left of it or starting past the box is not drawn.
        FloatRect clip;
        std::span<const ColorSpan> colors;
        std::span<const TextRange> hits;
        TextRange selection;
        bool fades;
    };

    // A monospace font with the glyph of every code point asked so far. See TextEngine-Types
    export class MonoFont
    {
    public:
        explicit MonoFont(std::unique_ptr<INativeMonoFont>);
    public:
        [[nodiscard]] float cellWidth() const { return m_cellWidth; }
        [[nodiscard]] float lineHeight() const { return m_lineHeight; }
        [[nodiscard]] float baseline() const { return m_baseline; }
        // The glyph the character is drawn as, or none. A tab is no glyph and a cell count.
        [[nodiscard]] MonoGlyph glyphOf(wchar_t);
        // Whether every character of the text has a cell: a tab, or a glyph of this font.
        [[nodiscard]] bool covers(std::wstring_view);
        [[nodiscard]] MonoExtent extent(std::wstring_view);
        // The leading edge of the character at pos, in the line's own coordinates. The text's
        // length names the end of the line.
        [[nodiscard]] float cellLeft(std::wstring_view, std::size_t pos);
        // The character standing under x, and whether x fell in the trailing half of its cell.
        [[nodiscard]] std::size_t hitTest(std::wstring_view, float x, bool* isTrailing);
        // Where the characters of a range put ink between two heights, one extent per glyph that
        // has any there. The heights are measured from the top of the line, and x from its start.
        void appendInkAcross(std::wstring_view, TextRange, float bandTop, float bandBottom,
            std::vector<Graphics::InkExtent>&);
        // The line's bands and glyphs, the origin being the top left of its first cell.
        void draw(ControlPaintContext&, std::wstring_view, FloatPoint origin, const MonoLineDraw&);
    private:
        using OtherGlyphs = std::unordered_map<char32_t, MonoGlyph>;
    private:
        [[nodiscard]] MonoGlyph resolve(char32_t);
    private:
        std::unique_ptr<INativeMonoFont> m_native;
        float m_cellWidth;
        float m_lineHeight;
        float m_baseline;
        // ASCII by table, every other code point by map. An entry not asked yet is told from one
        // answered none by m_asciiAsked.
        std::array<MonoGlyph, 128> m_ascii{};
        std::bitset<128> m_asciiAsked{};
        OtherGlyphs m_others{};
    };

    // The font for a request, or null where the platform has no monospace face for it. The one
    // table for the application: a request is resolved once and answered from then on.
    export [[nodiscard]] MonoFont* monoFont(std::wstring_view family, float size, FontWeight,
        FontStyle);
}
