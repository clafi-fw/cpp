export module ClaFi.Core.TextEngine.Layout;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.BakedText;
import ClaFi.Core.TextEngine.Mono;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    export struct NativeLineMetrics
    {
        std::size_t textStart;
        std::size_t textLength;
        float width;
        float height;
        float baseline;
        bool isTrimmed;
    };

    export struct NativeParagraphMetrics
    {
        float width;
        float height;
    };

    export class INativeTextLayout
    {
    public:
        struct BuildParams
        {
            std::wstring_view text;
            float availableWidth;
            float scaleFactor;
            bool isEmpty;
            // Whether a line too long for availableWidth is broken or left as it is.
            bool wrap;

            std::vector<FamilySpan> families;
            std::vector<SizeSpan> sizes;
            std::vector<WeightSpan> weights;
            std::vector<StyleSpan> styles;
            std::vector<std::pair<std::size_t, BakedInlineObject>> inlineObjects;
        };
    public:
        virtual ~INativeTextLayout() = default;
    public:
        virtual void build(const BuildParams& params) = 0;
        // Where finished lines sit inside the box, which is not what they are shaped from - so it
        // is set on a built layout the way a colour is, and a rebuild states it again.
        virtual void setAlignment(TextAlign) = 0;
        // The box the finished lines are placed in. Never wider than the width they were built at,
        // so the lines it moves are the lines that were already there.
        virtual void setMaxWidth(float availableWidth) = 0;
        virtual NativeParagraphMetrics getMetrics() const = 0;
        // The lines of the last build, appended to the caller's pool rather than answered in a
        // vector of their own, so a document of many paragraphs allocates once for all of them.
        virtual void appendLineMetrics(std::vector<NativeLineMetrics>&) const = 0;
        virtual float getCharX(std::size_t localPos, bool trailing) const = 0;
        virtual std::size_t hitTestPoint(FloatPoint localPt, bool* isTrailing) const = 0;
        virtual FloatRect getCharRect(std::size_t localPos, bool trailing) const = 0;
        // Where the glyphs of a range put ink between two heights, one extent per glyph that has
        // any there, in the paragraph's own coordinates and in no particular order. What a link's
        // underline is cut around - see TextEngine-Types#links.
        virtual void appendInkAcross(TextRange localRange, float bandTop, float bandBottom,
            std::vector<Graphics::InkExtent>&) const = 0;

        // Everything a paragraph is drawn from is read, never kept: the spans point at buffers the
        // caller reuses for the next paragraph, so a backend that needs one past the call must copy
        // it.
        virtual void draw(
            ControlPaintContext&,
            const FloatRect& bounds,
            std::span<const ColorSpan> colors,
            std::span<const ScriptSpan> scripts,
            std::span<const TextRange> hitRanges,
            const TextRange& selectionRange,
            std::span<const float> localBaselinesToFade,
            float localCollapseBaseline,
            float collapseXOffset) = 0;
    };

    export std::unique_ptr<INativeTextLayout> createNativeTextLayout();

    // Colours drawn over a text's own, asked for by paragraph at draw time. See TextEngine-Types
    export class ColorOverlay
    {
    public:
        virtual ~ColorOverlay() = default;
        // The spans over one paragraph, in the paragraph's own coordinates, in order and not
        // overlapping. What the spans leave uncovered keeps the colour the text states.
        virtual void paragraphColors(std::size_t paragraph, std::wstring_view paragraphText,
            std::vector<ColorSpan>& out) const = 0;
    };

    // Where a layout's own origin stands in the caller's coordinates. A TextLayout holds every
    // position it answers relative to the top left of a box of its bounds, and the anchor is the
    // whole of what says where in the caller's rect that box was put - so this is the one
    // translation between the two spaces. Drawing and measuring both make it here, which is what
    // keeps a caret on the glyph it stands before.
    export [[nodiscard]] FloatPoint anchoredOrigin(const FloatRect& bounds, CalculatedDimensions,
        TextAnchor);

    // What a paragraph is drawn from, held across draws so that stating it allocates nothing. One
    // set for the application, the way the rasterizer's masks are.
    //
    // One set is enough for as long as nothing draws text inside a text draw. The place that could
    // is an in text icon: its paint function runs while the backend is drawing the run it sits in,
    // and baselines is still being read at that point - colors and hits are finished with before
    // the run is drawn, baselines is not. An icon that drew text would refill baselines under the
    // renderer reading it, so an icon paints shapes.
    //
    // colors, overlayColors, mergedColors and inkedColors are a chain because each step is stated
    // over what the step before it produced - the overlay is merged over the text's own colours,
    // and a selection is inked over whichever of those the paragraph is drawn from. Every sweep
    // reads one and writes the next, so none of them can be another.
    struct TextDrawBuffers
    {
        NoAllocFloatVector baselines;
        std::vector<TextRange> hits;
        std::vector<ColorSpan> colors;
        std::vector<ColorSpan> overlayColors;
        std::vector<ColorSpan> mergedColors;
        std::vector<ColorSpan> inkedColors;
        std::vector<ScriptSpan> scripts;
        // The ink under a line of the hovered link, which its underline is cut around. Filled and
        // spent inside one paragraph's draw.
        std::vector<Graphics::InkExtent> ink;
        // The selection bands standing past the last glyph of a row, in the coordinates the draw
        // was asked for. Those lie off the end of the row's text, which a row filling its box
        // puts outside the layout's own box, so they are held while the paragraphs are walked and
        // filled once that box is off the clip stack. Live for the whole of a draw, baselines
        // being the other one that is.
        std::vector<FloatRect> newlineBands;
    };

    TextDrawBuffers g_textDrawBuffers{};

    // One paragraph as laid out: shaped natively, or set on the cells of one monospace font,
    // which holds no native layout at all. See TextEngine-Types#mono-paragraph
    struct ParagraphLayoutState
    {
        // Null for a paragraph on cells, and for one whose layout was given back - see HeldLayouts.
        std::unique_ptr<INativeTextLayout> nativeLayout;
        // The font a paragraph on cells is laid out on, and null for one shaped natively.
        MonoFont* monoFont;
        FloatRect bounds;
        std::size_t textStart;
        std::size_t textLength;
        float indent;
        TextAlign alignment;
        // The lines as they came out of the last build, in the layout's pool - see
        // TextLayout::linesOf. Kept because the fit to the box is read off them and read again
        // whenever the height moves, which shapes nothing.
        std::size_t lineStart;
        std::size_t lineCount;
    };

    // The paragraphs of a layout holding their native layout. See TextEngine-Types
    class HeldLayouts
    {
    public:
        void clear();
        void add(std::size_t paragraph);
        // Paragraphs [first, first + replaced) became inserted others, the last taken as in view.
        void splice(std::size_t first, std::size_t replaced, std::size_t inserted);
        void beginDraw();
        void drawn(std::size_t paragraph);
        void endDraw();
        [[nodiscard]] bool drawing() const { return m_drawing; }
        [[nodiscard]] std::size_t count() const { return m_paragraphs.size(); }
        // The paragraphs past the budget, taken out farthest first - valid until the next call.
        [[nodiscard]] std::span<const std::size_t> trim(std::size_t budget);
    private:
        // How many paragraphs lie between this one and the drawn range.
        [[nodiscard]] std::size_t distance(std::size_t paragraph) const;
    private:
        std::vector<std::size_t> m_paragraphs{};
        std::vector<std::size_t> m_released{};   // what the last trim took out
        // The paragraphs the last draw showed - first past last before any draw has shown one.
        std::size_t m_drawnFirst{ k_maxSize };
        std::size_t m_drawnLast{ 0 };
        // The range a draw began over, which a draw showing nothing keeps.
        std::size_t m_keptFirst{ k_maxSize };
        std::size_t m_keptLast{ 0 };
        bool m_drawing{ false };
    };

    // What one edit did, in the coordinates of the text before it. See TextEngine-Types
    export struct TextEdit
    {
        TextRange replaced;
        std::size_t insertedLength{ 0 };
    };

    // Two edits as one, the second stated in the coordinates the first left behind and the answer
    // in the coordinates before either. Key repeat outruns the paint loop, so several edits reach
    // the text before anything asks the layout about it, and one record has to carry all of them.
    //
    // The answer is the HULL of the two, which can name text neither of them touched - a position
    // inside what the first edit inserted has no place in the coordinates before it, and the edge
    // of what that edit replaced stands for it. Widening is the safe direction: a hull too wide
    // re-shapes a paragraph that did not have to move, or is refused for crossing a boundary, and
    // both of those are answers the caller already handles.
    export [[nodiscard]] TextEdit mergedEdits(TextEdit first, TextEdit second);

    export class TextLayout
    {
    public:
        TextLayout() = default;
    public:
        void setEventPhase(EventPhase);
        void setText(const Text&);
        // Whether a text has been stated at all - a layout holding none has nothing to shape
        [[nodiscard]] bool isTextStated() const { return m_text != nullptr; }
        // The text after an edit, and what the edit was. A paragraph the edit did not reach keeps
        // the shaping it already has, which is the whole point: an ordinary key press re-shapes one
        // paragraph instead of the document.
        //
        // Answers false when it declined - an edit crossing a paragraph boundary, one that typed a
        // newline or took one out, or a layout with nothing shaped yet. The caller then states the
        // whole text with setText, and the layout is already invalidated for it.
        [[nodiscard]] bool applyTextEdit(const Text&, TextEdit);
        void setBoundsAndScale(MaxSize bounds, ScaleFactor);
        void setEditable(bool editable);
        void setWrap(bool wrap);
        // The width the lines are BROKEN at, when that is not the box they stand in. Zero, the
        // standing answer, breaks them at the box - which is what a control laying out its own
        // text asks for. A layout that has to repeat ANOTHER's line breaks states the width that
        // one was broken at, and is then free to stand in a box of any size: the box no longer
        // decides where a line ends, so a wider one shows what the narrower box cut instead of
        // breaking the text somewhere else. See TooltipLabel, which an over-text hint is drawn
        // from.
        void setBreakWidth(float);
        // Colours drawn over the text's own, or nothing. A draw-time question alone: the overlay
        // is asked about a paragraph as that paragraph is drawn, and the shaping knows nothing of
        // it - so stating one invalidates nothing, and a caller whose colours moved repaints.
        void setColorOverlay(const ColorOverlay*);
        // The link drawn underlined, or an empty range for none - the control holding the layout
        // says which, usually the one under the pointer. A draw-time question alone, the way the
        // overlay is: stating one invalidates nothing, and the caller repaints. A new text drops
        // it, since the range is stated in the text it was found in.
        void setHoveredLink(TextRange);
        [[nodiscard]] TextRange hoveredLink() const { return m_hoveredLink; }

        DrawTextResult draw(ControlPaintContext&, FloatPoint, const EditProps* = nullptr, TextRenderMode = TextRenderMode::Static);
        CalculatedDimensions calculatedDimensions();
        // Whether the box this layout stands in cuts the text: a line wider than the box, or a
        // line past its bottom. The same fit draw() reports as trimmed - see ensureVerticalFit -
        // so a caller that has to know before there is a paint asks here.
        [[nodiscard]] bool isTrimmed();

        CaretHit caretPos(FloatPoint);
        // The link whose glyph stands under a point, or nothing - a point past the end of a line,
        // or between two lines, names no glyph and so no link. In the coordinates caretPos takes.
        [[nodiscard]] std::optional<LinkHit> linkAt(FloatPoint);
        std::size_t rowStart(CaretHit);
        std::size_t rowEnd(CaretHit);
        // The line and column a position stands at - see TextLineColumn. A binary search over the
        // paragraphs, reading no text.
        [[nodiscard]] TextLineColumn lineColumnAt(std::size_t pos);
        CaretHit posOnNextRow(CaretHit, ScrollDirection, std::optional<float> targetX);
        FloatRect getCaretRect(CaretHit);

        // Whether this layout can answer about a box of that width without shaping again.
        [[nodiscard]] bool acceptsWidth(float boundsX) const;
        // Drops the shaping and everything read off it.
        void invalidate();
    private:
        using Lines = std::vector<NativeLineMetrics>;
        // The cursors a walk over the paragraphs carries - see advanceSpanCursor. Held together
        // because every one of them advances by the same paragraph start.
        struct SpanCursors
        {
            std::size_t families{ 0 };
            std::size_t sizes{ 0 };
            std::size_t weights{ 0 };
            std::size_t styles{ 0 };
            std::size_t scripts{ 0 };
            std::size_t inlineObjects{ 0 };
        };
    private:
        void ensureLayout();
        // The lines themselves, from the text, the width they are broken at and the scale.
        void ensureShaping();
        // Where those lines stand in the box: the collapse, the fades, and the size the text came
        // to. Runs on its own when only the height has moved.
        void ensureVerticalFit();
        // Tells the built lines which box they stand in, when that is not the one they were broken
        // at. Breaks nothing again - see acceptsWidth.
        void ensureBoxWidth();
        // The width a shaping breaks at: the stated one, or the box where none is stated.
        [[nodiscard]] float breakWidth() const;
        // The width the lines are PLACED in: the stated break width, or the box.
        [[nodiscard]] float placementWidth() const;
        // The paragraph's lines, out of the pool every paragraph's lines are kept in.
        [[nodiscard]] std::span<const NativeLineMetrics> linesOf(const ParagraphLayoutState&) const;
        [[nodiscard]] std::wstring_view paragraphText(const ParagraphLayoutState&) const;
        // The paragraph holding a position - the last one starting at or before it, which the
        // upper bound of the starts finds - or k_maxSize where the position lies past that
        // paragraph's end. A binary search, since the starts are in order.
        [[nodiscard]] std::size_t paragraphAt(std::size_t pos) const;
        // The paragraph a y falls in: the first whose bottom lies below it, and the last where
        // none does. A binary search, since the bottoms are in order.
        [[nodiscard]] std::size_t paragraphAtY(float y) const;
        // Where a paragraph on cells stands inside the placement box: its alignment, applied
        // as it is asked for rather than told to a native layout.
        [[nodiscard]] float monoOffset(const ParagraphLayoutState&) const;
        // A character's leading or trailing edge as a caret rect, in the paragraph's own
        // coordinates, whichever way the paragraph was laid out.
        [[nodiscard]] FloatRect charRect(const ParagraphLayoutState&, std::size_t localPos,
            bool trailing);
        // The hovered link's underline over one paragraph: a segment per line it reaches, under
        // the glyphs that line shows of it, in the colour its first one was drawn in. origin is
        // where the paragraph's top left was drawn.
        void drawLinkUnderline(ControlPaintContext&, const ParagraphLayoutState&, FloatPoint origin,
            std::span<const ColorSpan> colors);
        // Where the glyphs of a range put ink between two heights, in the paragraph's own
        // coordinates, whichever way the paragraph was laid out.
        void appendInkAcross(const ParagraphLayoutState&, TextRange localRange, float bandTop,
            float bandBottom, std::vector<Graphics::InkExtent>&);
        // The monospace font a paragraph is set in over its whole range, or null: a span of
        // family, size, weight or style ending inside it, a script, an inline object or a
        // character the font has no cell for all answer null, and the paragraph is shaped
        // natively. Asked with the cursors already advanced to the paragraph.
        [[nodiscard]] MonoFont* monoFontFor(const ParagraphStyle&, std::wstring_view slice,
            const SpanCursors&) const;
        // One paragraph, shaped and placed with its top at the given y, its lines appended to
        // the given pool. The cursors are advanced past what this paragraph consumed, so a walk
        // hands the same set to the next one and a caller shaping a single paragraph hands a
        // fresh set and pays one pass over the spans. boundsWidth is the width the lines are
        // BROKEN at, which is not always the box the layout currently holds: a paragraph shaped
        // on its own has to be broken the way the paragraphs around it were, or it is the one
        // line in the document that wraps differently.
        [[nodiscard]] ParagraphLayoutState shapeParagraph(const ParagraphStyle&, float top,
            float boundsWidth, SpanCursors&, Lines&);
        // The paragraph's native layout, built again first where it was given back.
        [[nodiscard]] INativeTextLayout& nativeOf(const ParagraphLayoutState&);
        // A native paragraph's layout as the shaping built it, told the box it was told since.
        [[nodiscard]] std::unique_ptr<INativeTextLayout> rebuiltNative(std::size_t paragraph);
        // The cursors a walk from the first paragraph would carry to pos, found by bisection.
        [[nodiscard]] SpanCursors cursorsAt(std::size_t pos) const;
        void releaseNatives(std::span<const std::size_t> paragraphs);
    private:
        const Text* m_text{ nullptr };
        BakedText m_bakedText;
        MaxSize m_bounds{ 0.0f, 0.0f };
        ScaleFactor m_scaleFactor{ 1.0f };
        bool m_editable{ false };
        bool m_wrap{ true };
        // Zero while the box the lines stand in is the width they are broken at - see
        // setBreakWidth.
        float m_breakWidth{ 0.0f };
        bool m_layoutValid{ false };
        bool m_verticalValid{ false };
        EventPhase m_eventPhase{ EventPhase::Calculate };
        const ColorOverlay* m_colorOverlay{ nullptr };
        TextRange m_hoveredLink{};

        std::vector<ParagraphLayoutState> m_paragraphs;
        // Every paragraph's lines, each naming its own run of them, so a paragraph holds no
        // vector and a document of a hundred thousand rows allocates once for all of them.
        Lines m_lines;
        // What the lines came to, before the box they were given is allowed to clamp it.
        float m_shapedWidth{ 0.0f };
        float m_shapedHeight{ 0.0f };
        // The width they were broken at, which is the other end of what acceptsWidth answers over.
        float m_builtBoundsX{ 0.0f };
        // The width the native layouts were last told about. They place their lines inside the box
        // they know of, so a box that narrowed onto lines that still fit has to say so.
        float m_boxWidthApplied{ -1.0f };
        float m_calcWidth{ 0.0f };
        float m_calcHeight{ 0.0f };
        std::vector<float> m_globalBaselinesToFade;
        float m_globalCollapseBaseline{ -9999.0f };
        float m_collapseXOffset{ 0.0f };
        // The paragraphs holding a native layout, while k_releaseNativeLayouts is on.
        HeldLayouts m_held;
        Lines m_rebuiltLines;   // the pool a rebuilt paragraph's lines are dropped into
    };

}
