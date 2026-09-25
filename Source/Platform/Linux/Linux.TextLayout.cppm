export module ClaFi.Platform.Linux.TextLayout;

import ClaFi.Platform.Linux.Fonts;

import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Mono;
import ClaFi.Core.TextEngine.BakedText;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.Graphics.Cpu_GlyphCompositor;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Platform::Linux
{
    // One paragraph shaped by HarfBuzz and broken into lines here. See Platform
    export class HarfBuzzLayout : public INativeTextLayout
    {
    public:
        // Starts on the buffers a destroyed layout left, where any are kept - see SpareBuffers.
        HarfBuzzLayout();
        ~HarfBuzzLayout() override;
    public:
        void build(const BuildParams& params) override;
        void setAlignment(TextAlign) override;
        void setMaxWidth(float availableWidth) override;
        [[nodiscard]] NativeParagraphMetrics getMetrics() const override;
        void appendLineMetrics(std::vector<NativeLineMetrics>&) const override;
        [[nodiscard]] float getCharX(std::size_t localPos, bool trailing) const override;
        [[nodiscard]] std::size_t hitTestPoint(FloatPoint localPt, bool* isTrailing) const override;
        [[nodiscard]] FloatRect getCharRect(std::size_t localPos, bool trailing) const override;
        void appendInkAcross(TextRange localRange, float bandTop, float bandBottom,
            std::vector<Graphics::InkExtent>&) const override;
        void draw(ControlPaintContext&, const FloatRect& bounds,
            std::span<const ColorSpan> colors,
            std::span<const ScriptSpan> scripts,
            std::span<const TextRange> hitRanges,
            const TextRange& selectionRange,
            std::span<const float> localBaselinesToFade,
            float localCollapseBaseline,
            float collapseXOffset) override;
    private:
        // One glyph as shaped, in pixels. cluster is the position in the paragraph text the glyph
        // starts at; x is where its pen stands on its line before the line's alignment offset, and
        // is written by placeLines.
        struct Glyph
        {
            std::uint32_t index;
            std::size_t cluster;
            float advance;
            float offsetX;
            // Positive upwards, as HarfBuzz states it.
            float offsetY;
            float x;
        };

        // A stretch of the paragraph in one face at one size, or one inline object. As shaped it is
        // an item, spanning whatever the text spans gave it; cut to a line it is a run, and an item
        // that breaks across lines is one run per line it touches.
        struct Run
        {
            std::size_t textStart;
            std::size_t textLength;
            std::size_t glyphStart;
            std::size_t glyphCount;
            // Null for an inline object, whose object then names it.
            FontFace* face;
            float emSize;
            FaceMetrics metrics;
            // Index into m_objects, or k_maxSize for text.
            std::size_t object;
        };

        // One character of the paragraph: the line it stands on and its cell on that line, before
        // the line's alignment offset. A ligature's advance is shared evenly among the characters
        // it covers, so every character has a cell and a caret can stand inside a ligature.
        struct CharCell
        {
            std::size_t line;
            float left;
            float advance;
            bool whitespace;
            // Where Justified puts its extra room: a space that is not trailing whitespace.
            bool stretchable;
            // Whether a glyph begins on this character. A line may only break where one does.
            bool clusterStart;
            // A tab's advance reaches the next tab stop from wherever the pen stands, so it is
            // written where the line is measured or placed, never by the shaping.
            bool tab;
        };

        struct Line
        {
            std::size_t textStart;
            std::size_t textLength;
            std::size_t runStart;
            std::size_t runCount;
            float top;
            float height;
            float baseline;
            // Without and with the trailing whitespace.
            float width;
            float fullWidth;
            std::size_t trailingWhitespace;
            std::size_t stretchableSpaces;
            // The alignment shift of the whole line inside the box, and the room Justified adds
            // after each stretchable space. Both are placement, restated without shaping again.
            float offset;
            float stretch;
        };

        using Glyphs = std::vector<Glyph>;
        using Runs = std::vector<Run>;
        using Lines = std::vector<Line>;
        using CharCells = std::vector<CharCell>;
        using Objects = std::vector<BakedInlineObject>;

        // Everything a build and a draw grow, handed from a destroyed layout to the next one made.
        struct Buffers
        {
            Glyphs glyphs;
            Runs items;
            Runs runs;
            Lines lines;
            CharCells chars;
            Objects objects;
            Graphics::Cpu::GlyphCompositor compositor;
        };

        // The buffers destroyed layouts left, for the layouts made after them. See Platform
        struct SpareBuffers
        {
            std::vector<Buffers> buffers{};
            ~SpareBuffers();
        };
    private:
        // Cuts the paragraph into items where a span changes, an object stands or the face a
        // character is found in changes, and shapes each.
        void itemize(const BuildParams&);
        void addTextItem(const BuildParams&, std::size_t textStart, std::size_t textEnd, FontFace&, float emSize);
        void shapeItem(const BuildParams&, Run& item);
        // Fills the lines greedily from the items, over the break opportunities libunibreak named.
        void breakLines(const BuildParams&);
        // Cuts the items into the runs of one line and measures the line.
        void addLine(std::size_t textStart, std::size_t textEnd);
        // Writes every glyph's pen position and every character's cell from the lines' stretch,
        // and the paragraph's extent from the lines.
        void placeLines();
        // The alignment offsets and the Justified stretch, from m_alignment and m_maxWidth.
        void alignLines();
        // The advance that takes a pen at this x to the next tab stop.
        [[nodiscard]] float tabAdvance(float pen) const;
        // The advance of a space in the font the paragraph starts in, at the size it is set at.
        [[nodiscard]] float leadingSpaceAdvance(const BuildParams&) const;

        [[nodiscard]] std::size_t lineAt(float y) const;
        [[nodiscard]] std::size_t lineOfPosition(std::size_t localPos) const;
        // The x of a character's leading or trailing edge, alignment included.
        [[nodiscard]] float charEdge(std::size_t localPos, bool trailing) const;
    private:
        static SpareBuffers s_spares;
        // False once s_spares is gone: layouts the text engine's cache holds are destroyed at exit.
        static bool s_sparesOpen;
        std::size_t m_textLength{ 0 };
        float m_scaleFactor{ 1.0f };
        // The width the lines were broken at, and the box they are placed in.
        float m_availableWidth{ 0.0f };
        float m_maxWidth{ 0.0f };
        TextAlign m_alignment{ TextAlign::Left };
        // The spacing of the tab stops, in pixels.
        float m_tabInterval{ 0.0f };
        Glyphs m_glyphs{};
        // The items as shaped, consumed by the line cutting of the same build.
        Runs m_items{};
        Runs m_runs{};
        Lines m_lines{};
        CharCells m_chars{};
        Objects m_objects{};
        NativeParagraphMetrics m_metrics{ 0.0f, 0.0f };
        Graphics::Cpu::GlyphCompositor m_compositor{};
    };

    // A monospace face fontconfig matched, drawn through the compositor. See Platform
    export class FreeTypeMonoFont : public INativeMonoFont
    {
    public:
        FreeTypeMonoFont(FontFace&, float emSize);
        ~FreeTypeMonoFont() override = default;
    public:
        [[nodiscard]] float cellWidth() const override { return m_cellWidth; }
        [[nodiscard]] float lineHeight() const override { return m_metrics.lineHeight(); }
        [[nodiscard]] float baseline() const override { return m_metrics.ascent; }
        [[nodiscard]] MonoGlyph glyphOf(char32_t) const override;
        void drawRun(ControlPaintContext&, FloatPoint baselineOrigin, std::span<const MonoGlyph>,
            const Graphics::Brush&) override;
        [[nodiscard]] std::optional<Graphics::InkExtent> inkAcross(MonoGlyph, float bandTop,
            float bandBottom) override;
    private:
        // The set's, opened for the life of the application - see FontSet::openFace.
        FontFace* m_face;
        float m_emSize;
        float m_cellWidth;
        FaceMetrics m_metrics;
        Graphics::Cpu::GlyphCompositor m_compositor{};
    };
}
