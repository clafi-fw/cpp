module ClaFi.Core.TextEngine.Layout;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.BakedText;
import ClaFi.Core.TextEngine.Mono;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // Whether a measured paragraph gives its native layout back. See TextEngine-Types#heldlayouts
    constexpr bool k_releaseNativeLayouts = false;
    // How many native layouts a layout holds between draws, when measured ones are given back.
    constexpr std::size_t k_heldLayoutBudget = 256;

    // Spans arrive in order and do not overlap, so a walk that visits the paragraphs in order
    // never has to look at a span an earlier paragraph has already left behind. The cursor is
    // what carries that across the walk: it names the first span that can still reach the
    // paragraph being asked about, which is what keeps the whole set to one pass instead of one
    // pass per paragraph. A caller with no walk to carry starts at zero.
    template <typename TSpan>
    static void advanceSpanCursor(const std::vector<TSpan>& globalSpans, std::size_t& cursor, std::size_t pStart)
    {
        while (cursor != globalSpans.size()
            && globalSpans[cursor].range.start + globalSpans[cursor].range.length <= pStart)
        {
            ++cursor;
        }
    }

    // Inline objects arrive in text order as well, and each one occupies a character of its own,
    // so the same walk applies with the object's position in place of a span's range: the cursor
    // names the first object that can still fall inside the paragraph being asked about.
    static void advanceInlineCursor(const std::vector<std::pair<std::size_t, BakedInlineObject>>& objects,
        std::size_t& cursor, std::size_t pStart)
    {
        while (cursor != objects.size() && objects[cursor].first < pStart)
            ++cursor;
    }

    // Where advanceSpanCursor would carry a cursor from zero, found by bisection over the ends.
    template <typename TSpan>
    static std::size_t spanCursorAt(const std::vector<TSpan>& globalSpans, std::size_t pStart)
    {
        const auto reached = std::partition_point(globalSpans.begin(), globalSpans.end(),
            [pStart](const TSpan& span){
                return span.range.start + span.range.length <= pStart;
            });
        return static_cast<std::size_t>(reached - globalSpans.begin());
    }

    // Where advanceInlineCursor would carry a cursor from zero, found by bisection.
    static std::size_t inlineCursorAt(
        const std::vector<std::pair<std::size_t, BakedInlineObject>>& objects, std::size_t pStart)
    {
        const auto reached = std::partition_point(objects.begin(), objects.end(),
            [pStart](const std::pair<std::size_t, BakedInlineObject>& object){
                return object.first < pStart;
            });
        return static_cast<std::size_t>(reached - objects.begin());
    }

    // How far the span the cursor names reaches into a paragraph: none of it, the whole of it,
    // or a part - which is a paragraph carrying more than one value, since the spans are
    // contiguous and a second one takes over where the first ends. The cursor names the first
    // span that can still reach the paragraph, so the span at its start is that one or none. A
    // span beginning exactly where an empty paragraph stands reaches none of it, the same answer
    // collectLocalSpans gives.
    enum class SpanReach
    {
        None,
        Whole,
        Part
    };

    template <typename TSpan>
    static SpanReach spanReach(const std::vector<TSpan>& spans, std::size_t cursor,
        std::size_t pStart, std::size_t pEnd)
    {
        if (cursor == spans.size() || spans[cursor].range.start >= pEnd)
            return SpanReach::None;
        if (spans[cursor].range.start > pStart)
            return SpanReach::Part;
        return spans[cursor].range.end() >= pEnd ? SpanReach::Whole : SpanReach::Part;
    }

    // Fills the caller's buffer rather than answering with one, so a paragraph after the first
    // reuses what the paragraph before it grew.
    template <typename TSpan>
    static void collectLocalSpans(const std::vector<TSpan>& globalSpans, std::size_t pStart,
        std::size_t pEnd, std::size_t cursor, std::vector<TSpan>& localSpans)
    {
        localSpans.clear();
        for (std::size_t i = cursor; i != globalSpans.size(); ++i)
        {
            const TSpan& span = globalSpans[i];
            std::size_t sStart = span.range.start;
            // Ordered, so the first span that starts past this paragraph ends the answer.
            if (sStart >= pEnd)
                break;

            std::size_t sEnd = sStart + span.range.length;
            if (sEnd > pStart)
            {
                std::size_t intersectStart = std::max(pStart, sStart);
                std::size_t intersectEnd = std::min(pEnd, sEnd);
                TSpan localSpan = span;
                localSpan.range.start = intersectStart - pStart;
                localSpan.range.length = intersectEnd - intersectStart;
                localSpans.push_back(localSpan);
            }
        }
    }

    // Selected text carries its own colour through the selection instead of being repainted in
    // one ink: the rule is applied to whatever each span already holds, so a grey run stays grey
    // under the band and an accent run stays accent. The part of the selection no span covers is
    // the default ink, and it is given a span of its own here - the renderer falls back to the
    // default colour for an uncovered range and knows nothing about the rule.
    //
    // Spans arrive in order and do not overlap, which is what lets one sweep both split them and
    // fill the gaps between them.
    // spans and result are read and written in step, so they must be different buffers.
    static void collectSelectionInk(std::span<const ColorSpan> spans,
        const TextRange& selection, const ControlPaintContext& theme,
        std::vector<ColorSpan>& result)
    {
        auto rgbOf = [&](const ColorDef& value) {
            Hsl hsl = value.index() == 0
                ? theme.inkHsl(std::get<0>(value))
                : Hsl{ std::get<1>(value) };
            // The band states which side of the theme it stands on and the ink over it is read
            // from that side, so the whole of that question is answered by the theme the control
            // was handed - a box inside an element carried across the theme moves its selected
            // ink the way it still has room to move.
            return theme.selectionInkHsl(hsl).toColor();
        };

        const std::size_t selectionStart = selection.start;
        const std::size_t selectionEnd = selectionStart + selection.length;
        const ColorDef defaultInk{ rgbOf(ColorDef{InkWell::textInk() }) };

        result.clear();
        result.reserve(spans.size() + 2);
        std::size_t filledTo = selectionStart;

        for (const ColorSpan& span : spans)
        {
            const std::size_t spanStart = span.range.start;
            const std::size_t spanEnd = spanStart + span.range.length;
            const std::size_t selectedStart = std::max(spanStart, selectionStart);
            const std::size_t selectedEnd = std::min(spanEnd, selectionEnd);
            if (selectedStart >= selectedEnd)
            {
                result.push_back(span);
                continue;
            }
            if (filledTo < selectedStart)
                result.push_back({ { filledTo, selectedStart - filledTo }, defaultInk });
            if (spanStart < selectedStart)
                result.push_back({ { spanStart, selectedStart - spanStart }, span.value });
            result.push_back({ { selectedStart, selectedEnd - selectedStart }, ColorDef{ rgbOf(span.value) } });
            filledTo = selectedEnd;
            if (selectedEnd < spanEnd)
                result.push_back({ { selectedEnd, spanEnd - selectedEnd }, span.value });
        }

        if (filledTo < selectionEnd)
            result.push_back({ { filledTo, selectionEnd - filledTo }, defaultInk });
    }

    // The overlay's spans laid over the paragraph's own: where an overlay span stands its colour
    // is drawn, and everywhere else the paragraph keeps what it states. Both arrive in order and
    // without overlap, so one sweep carries both, and a base span is cut wherever an overlay span
    // begins or ends inside it.
    // base and result are read and written in step, so they must be different buffers.
    static void mergeOverlayColors(std::span<const ColorSpan> base,
        std::span<const ColorSpan> overlay, std::vector<ColorSpan>& result)
    {
        result.clear();
        result.reserve(base.size() + overlay.size() * 2);

        std::size_t next = 0;
        for (const ColorSpan& span : base)
        {
            std::size_t at = span.range.start;
            const std::size_t end = span.range.end();
            while (at < end)
            {
                while (next != overlay.size() && overlay[next].range.end() <= at)
                    ++next;

                if (next == overlay.size() || overlay[next].range.start >= end)
                {
                    result.push_back({ { at, end - at }, span.value });
                    at = end;
                    break;
                }

                const ColorSpan& over = overlay[next];
                if (over.range.start > at)
                    result.push_back({ { at, over.range.start - at }, span.value });

                const std::size_t overStart = std::max(at, over.range.start);
                const std::size_t overEnd = std::min(end, over.range.end());
                result.push_back({ { overStart, overEnd - overStart }, over.value });
                at = overEnd;
            }
        }
    }

    static TextRange getLocalRange(const TextRange& globalRange, std::size_t pStart, std::size_t pEnd)
    {
        if (globalRange.start == static_cast<std::size_t>(-1) || globalRange.length == 0)
            return { static_cast<std::size_t>(-1), 0 };

        std::size_t gEnd = globalRange.start + globalRange.length;
        if (globalRange.start < pEnd && gEnd > pStart)
        {
            std::size_t intersectStart = std::max(pStart, globalRange.start);
            std::size_t intersectEnd = std::min(pEnd, gEnd);
            return { intersectStart - pStart, intersectEnd - intersectStart };
        }
        return { static_cast<std::size_t>(-1), 0 };
    }

    static void collectLocalRanges(const std::vector<TextRange>& globalRanges, std::size_t pStart,
        std::size_t pEnd, std::vector<TextRange>& localRanges)
    {
        localRanges.clear();
        for (const auto& r : globalRanges)
        {
            TextRange localRange = getLocalRange(r, pStart, pEnd);
            if (localRange.start != static_cast<std::size_t>(-1))
            {
                localRanges.push_back(localRange);
            }
        }
    }

    // What a selected newline shows. A newline carries no width of its own, so a row the
    // selection runs through would end flush with its last glyph, and a row with no text would
    // show nothing at all. The share is of the row's own height, which holds the band in
    // proportion across font sizes and scale factors with no measurement of its own.
    constexpr float k_newlineBandHeightShare = 0.25f;

    // How far under the baseline a link's underline stands, as a share of the room its line has
    // below the baseline. The font's own underline position is not read. See TextEngine-Types#links
    constexpr float k_underlineDescentShare = 0.3f;
    // How far the underline stands off a glyph it would cross, in its own thicknesses, on each
    // side of the ink. See TextEngine-Types#links
    constexpr float k_underlineInkGap = 1.0f;

    // How far a line may reach past its box and still fit it. The box a paint asks about is a
    // measured size read back out of a rect built from it, and a float sum can give it back a
    // ULP short - a strict test then finds a box exactly as tall as its text too small, and the
    // text collapses and fades. A tenth of a pixel is invisible either way.
    constexpr float k_fitTolerance = 0.1f;

    // Shaped as nothing: an empty paragraph is built from it, and it stands in for a line end.
    constexpr wchar_t k_zeroWidthSpace = 0x200B;

    // A character the native engine ends a line at by itself - a row is cut by '\n' alone.
    static bool isNativeLineEnd(const wchar_t character)
    {
        switch (character)
        {
            case L'\r':
            case L'\v':
            case L'\f':
            case 0x0085:
            case 0x2028:
            case 0x2029:
                return true;
            default:
                return false;
        }
    }

    // The slice for the native engine, its own line ends replaced one for one, positions kept.
    [[nodiscard]] static std::wstring_view withoutNativeLineEnds(const std::wstring_view slice,
        std::wstring& buffer)
    {
        if (std::ranges::none_of(slice, isNativeLineEnd))
            return slice;

        buffer.assign(slice);
        for (wchar_t& character : buffer)
        {
            if (isNativeLineEnd(character))
                character = k_zeroWidthSpace;
        }

        return buffer;
    }

    static bool isNewlineSelected(const TextRange& selection, std::size_t newlinePosition)
    {
        return selection.start <= newlinePosition
            && selection.start + selection.length > newlinePosition;
    }

    // Whether this paragraph shows the band standing for the newline the selection runs through
    // it by, which is the question of whether that newline is selected at all.
    //
    // WHICH newline that is, is the caret's. A selection runs from where it was opened toward the
    // caret, so a caret below this paragraph carries it out through the newline that ENDS the
    // paragraph, and a caret above carries it out through the newline that OPENS it - the one
    // ending the row above, at textStart - 1. EditProps::caretOnLeft is that direction, and the
    // whole of the mirror is this pair: the end swaps with the start, and the row with none below
    // it swaps with the row with none above. A paragraph's own range stops short of both
    // newlines - TextEngine::paragraphAt is where that split is stated - so neither position is
    // one the range covers.
    static bool isNewlineBandDrawn(const EditProps& editProps,
        const ParagraphLayoutState& paragraph, bool isFirstRow, bool isLastRow)
    {
        const TextRange& selection = editProps.selRange;
        if (selection.start == static_cast<std::size_t>(-1) || selection.length == 0)
            return false;

        if (editProps.caretOnLeft)
        {
            if (isFirstRow)
                return false;

            return isNewlineSelected(selection, paragraph.textStart - 1);
        }

        if (isLastRow)
            return false;

        return isNewlineSelected(selection, paragraph.textStart + paragraph.textLength);
    }

    // The one place a control's intent becomes raster settings. Kept above the platform so that a
    // backend is told what to do and never why, and kept in one function so the three cases cannot
    // drift apart - the whole failure this addresses is two of them disagreeing about a setting.
    static constexpr Graphics::TextRasterizationParams textRasterParams2(TextRenderMode mode)
    {
        switch (mode)
        {
            case TextRenderMode::Movable:
                return {
                    .antialias = Graphics::TextAntialiasingMode::Grayscale,
                    .gridFit = false,
                    .snapOrigins = false,
                    .enhancedContrast = false,
                    .rasterizationMode = Graphics::TextRasterizationMode::Cached,
                };
            case TextRenderMode::Moving:
                // Differs from Movable in the rasterizer and nothing else. Anything else differing
                // would be visible at the handover, which happens twice for every hover.
                return {
                    .antialias = Graphics::TextAntialiasingMode::Grayscale,
                    .gridFit = false,
                    .snapOrigins = false,
                    .enhancedContrast = false,
                    .rasterizationMode = Graphics::TextRasterizationMode::Outline,
                };
            case TextRenderMode::Static:
                break;
        }
        return {
            .antialias = Graphics::TextAntialiasingMode::Subpixel,
            .gridFit = true,
            .snapOrigins = true,
            .enhancedContrast = true,
            .rasterizationMode = Graphics::TextRasterizationMode::Cached,
        };
    }

    FloatPoint anchoredOrigin(const FloatRect& bounds, const CalculatedDimensions calculated,
        const TextAnchor anchor)
    {
        FloatPoint origin = bounds.topLeft();
        switch (anchor.vertical)
        {
            case VerticalTextAnchor::Center:
                origin.y += (bounds.height() - calculated.y) / 2.0f;
                break;
            case VerticalTextAnchor::Bottom:
                origin.y = bounds.bottom - calculated.y;
                break;
            case VerticalTextAnchor::Top:
                break;
        }
        // The offset carries the paragraph boxes as well as the glyphs, because the layout is drawn
        // from this point and each box reaches to the bounds width from it. Text naming its own
        // TextAlign is therefore placed by the alignment and moved by the anchor on top of it, and
        // wants Left here.
        switch (anchor.horizontal)
        {
            case HorizontalTextAnchor::Center:
                origin.x += (bounds.width() - calculated.x) / 2.0f;
                break;
            case HorizontalTextAnchor::Right:
                origin.x = bounds.right - calculated.x;
                break;
            case HorizontalTextAnchor::Left:
                break;
        }
        return origin;
    }

    TextEdit mergedEdits(TextEdit first, TextEdit second)
    {
        const std::size_t firstEnd = first.replaced.start + first.replaced.length;
        const std::ptrdiff_t firstDelta = static_cast<std::ptrdiff_t>(first.insertedLength)
            - static_cast<std::ptrdiff_t>(first.replaced.length);

        // A position in the text the first edit left behind, carried back to where it stood before
        // that edit. What the first edit inserted has no such place, so the far edge of what it
        // replaced stands for anything inside it.
        auto toBefore = [&](std::size_t position){
            if (position <= first.replaced.start)
                return position;
            if (position >= first.replaced.start + first.insertedLength)
                return static_cast<std::size_t>(static_cast<std::ptrdiff_t>(position) - firstDelta);

            return firstEnd;
        };

        const std::size_t start = std::min(first.replaced.start, toBefore(second.replaced.start));
        const std::size_t end = std::max(firstEnd,
            toBefore(second.replaced.start + second.replaced.length));

        const std::ptrdiff_t secondDelta = static_cast<std::ptrdiff_t>(second.insertedLength)
            - static_cast<std::ptrdiff_t>(second.replaced.length);

        // What the two did to the length is what the one has to do to it, whatever the hull came
        // to - which is the property the caller checks the record against.
        return {
            .replaced = { start, end - start },
            .insertedLength = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(end - start) + firstDelta + secondDelta),
        };
    }

    // HeldLayouts

    void HeldLayouts::clear()
    {
        m_paragraphs.clear();
        m_drawnFirst = k_maxSize;
        m_drawnLast = 0;
    }

    void HeldLayouts::add(std::size_t paragraph)
    {
        m_paragraphs.push_back(paragraph);
    }

    void HeldLayouts::splice(std::size_t first, std::size_t replaced, std::size_t inserted)
    {
        const std::size_t end = first + replaced;
        std::erase_if(m_paragraphs, [first, end](std::size_t paragraph){
            return paragraph >= first && paragraph < end;
        });
        for (std::size_t& paragraph : m_paragraphs)
        {
            if (paragraph >= end)
                paragraph = paragraph - replaced + inserted;
        }

        // Where an edit leaves the caret, which is where the next draw is likely to look.
        m_drawnFirst = inserted != 0 ? first + inserted - 1 : first;
        m_drawnLast = m_drawnFirst;
    }

    void HeldLayouts::beginDraw()
    {
        m_drawing = true;
        m_keptFirst = m_drawnFirst;
        m_keptLast = m_drawnLast;
        m_drawnFirst = k_maxSize;
        m_drawnLast = 0;
    }

    void HeldLayouts::drawn(std::size_t paragraph)
    {
        m_drawnFirst = std::min(m_drawnFirst, paragraph);
        m_drawnLast = std::max(m_drawnLast, paragraph);
    }

    void HeldLayouts::endDraw()
    {
        m_drawing = false;
        // A draw clipped out of the view entirely says nothing about where the view is.
        if (m_drawnFirst > m_drawnLast)
        {
            m_drawnFirst = m_keptFirst;
            m_drawnLast = m_keptLast;
        }
    }

    std::span<const std::size_t> HeldLayouts::trim(std::size_t budget)
    {
        m_released.clear();
        if (m_paragraphs.size() <= budget)
            return m_released;

        // The drawn range is kept whatever the budget says, so only what lies outside it competes.
        const auto covered = std::partition(m_paragraphs.begin(), m_paragraphs.end(),
            [this](std::size_t paragraph){
                return distance(paragraph) != 0;
            });
        const std::size_t outside = static_cast<std::size_t>(covered - m_paragraphs.begin());
        const std::size_t released = std::min(m_paragraphs.size() - budget, outside);
        std::nth_element(m_paragraphs.begin(), m_paragraphs.begin() + released, covered,
            [this](std::size_t left, std::size_t right){
                return distance(left) > distance(right);
            });

        m_released.assign(m_paragraphs.begin(), m_paragraphs.begin() + released);
        m_paragraphs.erase(m_paragraphs.begin(), m_paragraphs.begin() + released);
        return m_released;
    }

    std::size_t HeldLayouts::distance(std::size_t paragraph) const
    {
        // Before any draw the top of the text stands for the view.
        if (m_drawnFirst > m_drawnLast)
            return paragraph;
        if (paragraph < m_drawnFirst)
            return m_drawnFirst - paragraph;
        if (paragraph > m_drawnLast)
            return paragraph - m_drawnLast;
        return 0;
    }

    // TextLayout

    void TextLayout::setEventPhase(EventPhase eventPhase)
    {
        if (m_eventPhase != eventPhase)
        {
            m_eventPhase = eventPhase;
            invalidate();
        }
    }

    void TextLayout::setText(const Text& text)
    {
        m_text = &text;
        m_hoveredLink = {};
        invalidate();
    }

    // ONE paragraph re-shaped, and every paragraph after it moved. What makes that sound is that a
    // paragraph is an independent shaping: its own native layout, broken from its own slice of the
    // text. An edit that stays inside one of them cannot change what any other one says, only where
    // its characters start and where its lines sit.
    //
    // Everything this declines is a case where that stops being true, and the caller's setText is
    // the answer to all of them.
    bool TextLayout::applyTextEdit(const Text& text, TextEdit edit)
    {
        m_hoveredLink = {};
        // Nothing shaped is nothing to keep, and a first build is what setText is for.
        if (!m_layoutValid || m_paragraphs.empty() || edit.replaced.start == k_maxSize)
            return false;

        // The paragraphs the edit reaches, looked for in the list as it still stands - which is
        // stated in the coordinates of the text BEFORE the edit, the same ones the edit is. A
        // position at the very end of a paragraph belongs to it: the newline that ends it is not
        // part of its range, so an insertion there goes in front of that newline, and a deletion
        // that takes it reaches into the paragraph after. So the first paragraph is the first
        // whose end is not before the edit, and the last is the first whose end is not before
        // the edit's - the ends are in order, so each is a binary search.
        const std::size_t editEnd = edit.replaced.start + edit.replaced.length;
        auto endsBefore = [](const ParagraphLayoutState& paragraph, std::size_t position){
            return paragraph.textStart + paragraph.textLength < position;
        };
        const auto firstReached = std::lower_bound(m_paragraphs.begin(), m_paragraphs.end(),
            edit.replaced.start, endsBefore);
        if (firstReached == m_paragraphs.end())
            return false;
        const std::size_t firstIndex = static_cast<std::size_t>(
            firstReached - m_paragraphs.begin());

        const auto lastReached = std::lower_bound(firstReached, m_paragraphs.end(), editEnd,
            endsBefore);
        const std::size_t lastIndex = lastReached == m_paragraphs.end()
            ? m_paragraphs.size() - 1
            : static_cast<std::size_t>(lastReached - m_paragraphs.begin());

        // From here the text this was built from is gone whatever happens, so every refusal below
        // leaves the layout invalidated for the setText that follows it.
        m_text = &text;
        m_bakedText.rebuild(*m_text, m_editable);

        // What lies outside the slice did not change in number, so what the rebuild came to is
        // what says how many paragraphs the slice became: fewer where a newline went, more where
        // one arrived, the same where the edit was ordinary.
        const std::size_t leading = firstIndex;
        const std::size_t trailing = m_paragraphs.size() - lastIndex - 1;
        if (m_bakedText.paragraphs().size() <= leading + trailing)
        {
            invalidate();
            return false;
        }
        const std::size_t newSliceCount = m_bakedText.paragraphs().size() - leading - trailing;

        // A paragraph that is not leading has been told which box its lines sit in and a freshly
        // built one has not - see ensureBoxWidth. Bringing one into line with its neighbours is a
        // case this does not carry.
        if (m_boxWidthApplied != m_builtBoundsX)
        {
            for (std::size_t i = 0; i != newSliceCount; ++i)
            {
                if (m_bakedText.paragraphs()[leading + i].alignment == TextAlign::Left)
                    continue;

                invalidate();
                return false;
            }
        }

        const float top = m_paragraphs[firstIndex].bounds.top;
        const float oldBottom = m_paragraphs[lastIndex].bounds.bottom;

        // Broken at the width their neighbours were broken at, not at the box the layout holds
        // now - otherwise these are the only paragraphs in the document that wrap differently.
        // The slice's lines are shaped into a pool of their own and spliced into the layout's
        // below, where the old paragraphs' lines stood.
        SpanCursors cursors;
        std::vector<ParagraphLayoutState> slice;
        slice.reserve(newSliceCount);
        Lines sliceLines;
        float sliceBottom = top;
        for (std::size_t i = 0; i != newSliceCount; ++i)
        {
            ParagraphLayoutState state = shapeParagraph(m_bakedText.paragraphs()[leading + i],
                sliceBottom, m_builtBoundsX, cursors, sliceLines);
            sliceBottom = state.bounds.bottom;
            // A slice longer than the budget keeps the layouts at its end, where the edit ended.
            if constexpr (k_releaseNativeLayouts)
            {
                if (newSliceCount - i > k_heldLayoutBudget)
                    state.nativeLayout.reset();
            }
            slice.push_back(std::move(state));
        }

        // Assigned over where the count held, which is every edit that typed no newline and took
        // none out. Spliced where it moved, and moving the tail of the vector is what a newline
        // costs - a walk over pointers, against re-shaping the document.
        const std::size_t oldSliceCount = lastIndex - firstIndex + 1;
        const std::size_t oldLineStart = m_paragraphs[firstIndex].lineStart;
        const std::size_t oldLineEnd = m_paragraphs[lastIndex].lineStart
            + m_paragraphs[lastIndex].lineCount;
        for (ParagraphLayoutState& state : slice)
            state.lineStart += oldLineStart;
        if (newSliceCount == oldSliceCount)
        {
            for (std::size_t i = 0; i != newSliceCount; ++i)
                m_paragraphs[firstIndex + i] = std::move(slice[i]);
        }
        else
        {
            m_paragraphs.erase(m_paragraphs.begin() + firstIndex,
                m_paragraphs.begin() + lastIndex + 1);
            m_paragraphs.insert(m_paragraphs.begin() + firstIndex,
                std::make_move_iterator(slice.begin()), std::make_move_iterator(slice.end()));
        }

        if constexpr (k_releaseNativeLayouts)
        {
            m_held.splice(firstIndex, oldSliceCount, newSliceCount);
            for (std::size_t i = firstIndex; i != firstIndex + newSliceCount; ++i)
            {
                if (m_paragraphs[i].nativeLayout)
                    m_held.add(i);
            }
            releaseNatives(m_held.trim(k_heldLayoutBudget));
        }

        // The lines likewise: assigned over where the count held, which a re-shaped paragraph
        // that broke into the same number of rows leaves it, and spliced where it moved. Moving
        // the tail here is a copy of plain records, cheaper than the walk over pointers above.
        const std::ptrdiff_t lineDelta = static_cast<std::ptrdiff_t>(sliceLines.size())
            - static_cast<std::ptrdiff_t>(oldLineEnd - oldLineStart);
        if (lineDelta == 0)
        {
            std::copy(sliceLines.begin(), sliceLines.end(), m_lines.begin() + oldLineStart);
        }
        else
        {
            m_lines.erase(m_lines.begin() + oldLineStart, m_lines.begin() + oldLineEnd);
            m_lines.insert(m_lines.begin() + oldLineStart, sliceLines.begin(), sliceLines.end());
        }

        // Guards the walk below, which reads the two lists in step.
        if (m_paragraphs.size() != m_bakedText.paragraphs().size())
            unreachable("An incremental text edit left a paragraph count the text does not have");

        const float heightDelta = sliceBottom - oldBottom;
        const std::ptrdiff_t textDelta = static_cast<std::ptrdiff_t>(edit.insertedLength)
            - static_cast<std::ptrdiff_t>(edit.replaced.length);

        // Everything after keeps its shaping and moves: its characters begin later or earlier by
        // what the edit added or took away, its lines stand lower or higher by what the slice
        // came to, and its run of the pool starts later or earlier by the rows the slice gained
        // or lost.
        for (std::size_t i = firstIndex + newSliceCount; i != m_paragraphs.size(); ++i)
        {
            ParagraphLayoutState& paragraph = m_paragraphs[i];
            paragraph.textStart = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(paragraph.textStart) + textDelta);
            paragraph.lineStart = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(paragraph.lineStart) + lineDelta);
            paragraph.bounds.top += heightDelta;
            paragraph.bounds.bottom += heightDelta;

            // The shift is the only thing here that computes where a paragraph stands rather than
            // reading it off a shaping, so it is checked against the text the rebuild just
            // produced. Free: this is the walk the shift needs anyway.
            const ParagraphStyle& stated = m_bakedText.paragraphs()[i];
            if (paragraph.textStart != stated.range.start
                || paragraph.textLength != stated.range.length)
            {
                unreachable("An incremental text edit put a paragraph where the text has none");
            }
        }

        // The same check for the pool: the last paragraph's run has to end where the pool does.
        const ParagraphLayoutState& last = m_paragraphs.back();
        if (last.lineStart + last.lineCount != m_lines.size())
            unreachable("An incremental text edit left lines no paragraph names");

        // The widest paragraph can have been one of those re-shaped, and it can have narrowed.
        m_shapedWidth = 0.0f;
        for (const ParagraphLayoutState& paragraph : m_paragraphs)
            m_shapedWidth = std::max(m_shapedWidth, paragraph.bounds.right);

        m_shapedHeight += heightDelta;
        // The lines moved, so where they sit inside the box is read off them again. Nothing here
        // broke a line at a width the box has not already been told about.
        m_verticalValid = false;
        return true;
    }

    void TextLayout::setBoundsAndScale(MaxSize bounds, ScaleFactor scaleFactor)
    {
        // A scale is shaped from and a width usually is - but a width the lines already fit is
        // not, see acceptsWidth. A height never is: it leaves every glyph where it stands and
        // changes only what the fit to the box comes to, which is read off the lines again.
        if (m_scaleFactor != scaleFactor || !acceptsWidth(bounds.x))
            invalidate();
        else if (m_bounds != bounds)
            m_verticalValid = false;

        m_bounds = bounds;
        m_scaleFactor = scaleFactor;
    }

    void TextLayout::setEditable(bool editable)
    {
        if (m_editable != editable)
        {
            m_editable = editable;
            invalidate();
        }
    }

    void TextLayout::setWrap(bool wrap)
    {
        if (m_wrap != wrap)
        {
            m_wrap = wrap;
            invalidate();
        }
    }

    void TextLayout::setBreakWidth(float value)
    {
        if (m_breakWidth == value)
            return;

        m_breakWidth = value;
        invalidate();
    }

    void TextLayout::setColorOverlay(const ColorOverlay* overlay)
    {
        m_colorOverlay = overlay;
    }

    void TextLayout::setHoveredLink(TextRange link)
    {
        m_hoveredLink = link;
    }

    DrawTextResult TextLayout::draw(
        ControlPaintContext& controlContext,
        FloatPoint position,
        const EditProps* editProps,
        TextRenderMode textRenderMode)
    {
        auto nativeCanvas = controlContext.canvas().backend();
        if (!nativeCanvas)
            return {};

        ensureLayout();
        if constexpr (k_releaseNativeLayouts)
            m_held.beginDraw();
        DrawTextResult result{ .drawn = true, .trimmed = !m_globalBaselinesToFade.empty() };

        // Held for the whole run rather than per paragraph, because on Direct2D this is render
        // target state. Every run names its settings, including one wanting what the target already
        // carries - the backend is what knows that, and charges nothing for it.
        Graphics::ScopedTextRaster textRaster{ *nativeCanvas, textRasterParams2(textRenderMode) };

        // What is on screen, read before the layout narrows the clip to its own box. Control::paint
        // pushes the control's viewport already narrowed to the dirty rect, so this is the band a
        // glyph has to fall in to be seen by anyone. A paragraph outside it draws nothing, and
        // DirectWrite is told about lines rather than asked which ones matter - so a text taller
        // than its viewport spends the frame proving that, one glyph run at a time.
        const FloatRect& canvasClipBox = controlContext.canvas().clipBox();
        // Where a collapse stands, if there is one. EVERY PARAGRAPH IS DRAWN WHERE IT WAS LAID
        // OUT except the one this falls in: FadeTextRenderer lifts a run onto the collapse
        // baseline only while drawing that paragraph, and a paragraph below it is handed no
        // baseline at all - so it draws under the box and the clip takes it. That makes a
        // paragraph's own bounds the honest test for every one of them, and leaves exactly one
        // that has to be drawn whatever its bounds say.
        const float collapseBaseline = m_globalCollapseBaseline;

        controlContext.canvas().pushClip(FloatRect::fromDimensions(position, m_bounds));

        std::size_t colorCursor = 0;
        std::size_t scriptCursor = 0;
        TextDrawBuffers& buffers = g_textDrawBuffers;
        buffers.newlineBands.clear();

        // Which paragraph is being drawn, which is what an overlay is asked about.
        std::size_t paragraphIndex = static_cast<std::size_t>(-1);
        for (auto& paragraph : m_paragraphs)
        {
            ++paragraphIndex;
            advanceSpanCursor(m_bakedText.colors(), colorCursor, paragraph.textStart);
            advanceSpanCursor(m_bakedText.scripts(), scriptCursor, paragraph.textStart);

            const bool collapsesHere = collapseBaseline >= paragraph.bounds.top
                && collapseBaseline <= paragraph.bounds.bottom;
            if (!collapsesHere &&
                // is out of the clipBox
                (position.y + paragraph.bounds.bottom < canvasClipBox.top
                    || position.y + paragraph.bounds.top > canvasClipBox.bottom))
            {
                continue;
            }

            if constexpr (k_releaseNativeLayouts)
                m_held.drawn(paragraphIndex);

            buffers.baselines.clear();
            for (float gb : m_globalBaselinesToFade)
            {
                if (gb >= paragraph.bounds.top && gb <= paragraph.bounds.bottom)
                {
                    buffers.baselines.push_back(gb - paragraph.bounds.top);
                }
            }

            float localCollapseBaseline = -9999.0f;
            if (collapsesHere)
                localCollapseBaseline = collapseBaseline - paragraph.bounds.top;

            TextRange localSelection{};
            buffers.hits.clear();
            if (editProps)
            {
                localSelection = getLocalRange(editProps->selRange, paragraph.textStart, paragraph.textStart + paragraph.textLength);
                collectLocalRanges(editProps->hits, paragraph.textStart, paragraph.textStart + paragraph.textLength, buffers.hits);
            }

            // --- FIXED: Extend the right edge of the paragraph bounds to the container's right boundary ---
            FloatRect paraDrawBounds{
                position.x + paragraph.bounds.left,
                position.y + paragraph.bounds.top,
                position.x + m_bounds.x, // Extends to the container edge so fading aligns correctly
                position.y + paragraph.bounds.bottom
            };

            collectLocalSpans(m_bakedText.colors(), paragraph.textStart,
                paragraph.textStart + paragraph.textLength, colorCursor, buffers.colors);
            collectLocalSpans(m_bakedText.scripts(), paragraph.textStart,
                paragraph.textStart + paragraph.textLength, scriptCursor, buffers.scripts);
            std::span<const ColorSpan> drawnColors = buffers.colors;
            // Asked about the paragraphs that are drawn and no other, so what an overlay costs is
            // what is on screen rather than what the document holds.
            if (m_colorOverlay)
            {
                buffers.overlayColors.clear();
                m_colorOverlay->paragraphColors(paragraphIndex, paragraphText(paragraph),
                    buffers.overlayColors);
                if (!buffers.overlayColors.empty())
                {
                    mergeOverlayColors(buffers.colors, buffers.overlayColors, buffers.mergedColors);
                    drawnColors = buffers.mergedColors;
                }
            }
            // A flipped band carries the ink across the theme with it, so the run is re-inked on
            // an element that states a flip even where its own text rule says nothing.
            const BakedElement& rules = controlContext.bakedColors()
                .element(UiElement::SelectedText);
            if (localSelection.length and (rules.flip != 0.0f or !rules.text.changesNothing()))
            {
                collectSelectionInk(drawnColors, localSelection, controlContext, buffers.inkedColors);
                drawnColors = buffers.inkedColors;
            }

            // The newline the selection runs through this paragraph by is drawn from here rather
            // than by the layout: it is one position of the selection with no glyph and no width
            // behind it, so the band the layout draws stops at the last glyph and an empty row is
            // left with nothing.
            //
            // Where the band stands, in the paragraph's own coordinates: at the end of its last
            // line, or - with the caret above - at the start of its first. A paragraph with no
            // text has position zero for both of its ends. Either way the rect is the width of
            // nothing and carries the height of the line it stands on, which is what the band is
            // given its width and height from.
            //
            // The band is laid from that point into the row: at the end it covers the width past
            // the last glyph, and at the start it covers the cell of the first one. Laid outward
            // from the start instead it would hang in the room the control leaves around its
            // text, where it reads as a mark of its own rather than as one end of the band.
            if (editProps && isNewlineBandDrawn(*editProps, paragraph,
                &paragraph == &m_paragraphs.front(), &paragraph == &m_paragraphs.back()))
            {
                const bool atRowStart = editProps->caretOnLeft;
                const bool atStart = atRowStart || paragraph.textLength == 0;
                FloatRect newlineBand = atStart
                    ? charRect(paragraph, 0, false)
                    : charRect(paragraph, paragraph.textLength - 1, true);
                newlineBand.right = newlineBand.left
                    + newlineBand.height() * k_newlineBandHeightShare;
                newlineBand.offset(paraDrawBounds.topLeft());

                // A band at the start of a row lies on the cell of its first glyph, so it is
                // filled here, under the glyphs this paragraph is about to draw. One past the
                // last glyph lies off the end of the row's text, which a row filling its box puts
                // outside the clip pushed above, so that one is held for the pass following the
                // clip and nothing is drawn over it there.
                if (atRowStart)
                    controlContext.canvas().fillRectangle(newlineBand, controlContext.selectionRgb());
                else
                    buffers.newlineBands.push_back(newlineBand);
            }

            if (paragraph.monoFont)
            {
                MonoFont& font = *paragraph.monoFont;
                FloatPoint origin = paraDrawBounds.topLeft();
                const float offset = monoOffset(paragraph);
                origin.x += offset;
                // The collapse line is moved by the offset to make room for what is appended to
                // it, and no further left than the box, where the clip would take its start.
                if (collapsesHere)
                    origin.x += std::max(m_collapseXOffset, -offset);

                // The one row a paragraph on cells has fades when the fit named its baseline.
                bool fades = false;
                for (const float baseline : buffers.baselines)
                {
                    if (std::abs(baseline - font.baseline()) < 0.5f)
                    {
                        fades = true;
                        break;
                    }
                }

                font.draw(controlContext, paragraphText(paragraph), origin, {
                    .bounds = paraDrawBounds,
                    .clip = canvasClipBox,
                    .colors = drawnColors,
                    .hits = buffers.hits,
                    .selection = localSelection,
                    .fades = fades,
                });
            }
            else
            {
                nativeOf(paragraph).draw(controlContext, paraDrawBounds,
                    drawnColors,
                    buffers.scripts,
                    buffers.hits,
                    localSelection,
                    buffers.baselines,
                    localCollapseBaseline,
                    m_collapseXOffset);
            }

            // Over the glyphs rather than under them: a selection band is filled by the draw
            // above, and a line laid first would go under it.
            if (m_hoveredLink.length)
                drawLinkUnderline(controlContext, paragraph, paraDrawBounds.topLeft(), drawnColors);
        }

        // Whether a caret was ever placed is the range's START: a whole-text selection holds
        // k_maxSize as its LENGTH, so caretPos answers what an unplaced caret answers.
        if (editProps && editProps->caretVisible && editProps->selRange.start != k_maxSize)
        {
            FloatRect cRect = getCaretRect({ editProps->caretPos(), editProps->affinityTrailing });
            const float caretWidth = controlContext.scaledStrokeWidth(Thickness::Thin);
            // The caret stands after the last character, and a row that fills the width puts that
            // point ON the right edge of the bounds - which the clip pushed above is exclusive
            // of, so the whole caret falls outside it. Right aligned and justified rows reach that
            // edge by definition. Held one width inside instead, where the caret marks the same
            // insertion point and can be seen.
            float caretLeft = std::clamp(cRect.left, 0.0f, std::max(m_bounds.x - caretWidth, 0.0f));
            float caretX = position.x + caretLeft;

            FloatRect caretAbsoluteRect{
                caretX,
                position.y + cRect.top,
                caretX + caretWidth,
                position.y + cRect.bottom
            };

            controlContext.canvas().fillRectangle(caretAbsoluteRect, controlContext.indicatorRgb());
        }

        controlContext.canvas().popClip();

        // The bands standing past the last glyph of a row, filled with the layout's box off the
        // clip stack: a row that fills that box would otherwise have its band cut away at the
        // right edge. Nothing else is drawn where they lie - a band stands off the end of a row's
        // text, and the row it stands on is never the one the caret is in, since the newline the
        // caret's own row is bounded by is the one the selection stops at - so coming after the
        // glyphs costs them nothing.
        for (const FloatRect& newlineBand : buffers.newlineBands)
            controlContext.canvas().fillRectangle(newlineBand, controlContext.selectionRgb());

        // Trimmed once the draw is over: during it, a row it has not reached yet could go back.
        if constexpr (k_releaseNativeLayouts)
        {
            m_held.endDraw();
            releaseNatives(m_held.trim(k_heldLayoutBudget));
        }

        return result;
    }

    CalculatedDimensions TextLayout::calculatedDimensions()
    {
        ensureLayout();
        return { m_calcWidth, m_calcHeight };
    }

    bool TextLayout::isTrimmed()
    {
        ensureLayout();
        return !m_globalBaselinesToFade.empty();
    }

    CaretHit TextLayout::caretPos(FloatPoint pt)
    {
        ensureLayout();
        if (m_paragraphs.empty())
            return {};

        const ParagraphLayoutState& p = m_paragraphs[paragraphAtY(pt.y)];
        FloatPoint localPt = { pt.x - p.bounds.left, pt.y - p.bounds.top };
        bool isTrailing = false;
        std::size_t localPos = p.monoFont
            ? p.monoFont->hitTest(paragraphText(p), localPt.x - monoOffset(p), &isTrailing)
            : nativeOf(p).hitTestPoint(localPt, &isTrailing);

        // Map DWrite char index + trailing half to a Logical Insertion Point
        std::size_t offset = localPos + (isTrailing ? 1 : 0);

        // --- FIXED: Explicitly clamp the offset against the logical paragraph length ---
        // (Solves invalid logical index leak that caused jumping over empty lines and crashing off the end bounds)
        if (offset > p.textLength)
        {
            offset = p.textLength;
            if (p.textLength == 0)
            {
                isTrailing = false;
            }
        }

        std::size_t logicalPos = p.textStart + offset;
        return { logicalPos, isTrailing };
    }

    std::optional<LinkHit> TextLayout::linkAt(FloatPoint pt)
    {
        ensureLayout();
        const std::vector<LinkSpan>& links = m_bakedText.links();
        if (links.empty() || m_paragraphs.empty())
            return std::nullopt;

        // paragraphAtY answers the last paragraph for a point below all of them, which is a
        // point on no glyph.
        const ParagraphLayoutState& p = m_paragraphs[paragraphAtY(pt.y)];
        if (pt.y < p.bounds.top || pt.y >= p.bounds.bottom || p.textLength == 0)
            return std::nullopt;

        const FloatPoint localPt = { pt.x - p.bounds.left, pt.y - p.bounds.top };
        bool isTrailing = false;
        const std::size_t localPos = p.monoFont
            ? p.monoFont->hitTest(paragraphText(p), localPt.x - monoOffset(p), &isTrailing)
            : nativeOf(p).hitTestPoint(localPt, &isTrailing);
        if (localPos >= p.textLength)
            return std::nullopt;

        // The hit names the nearest character, which past the end of a line is its last one - so
        // the point is held to the box that character's glyph stands in.
        const FloatRect leading = charRect(p, localPos, false);
        const FloatRect trailing = charRect(p, localPos, true);
        const float left = std::min(leading.left, trailing.left);
        const float right = std::max(leading.left, trailing.left);
        if (localPt.x < left || localPt.x >= right || localPt.y < leading.top
            || localPt.y >= leading.bottom)
        {
            return std::nullopt;
        }

        const std::size_t pos = p.textStart + localPos;
        const auto above = std::upper_bound(links.begin(), links.end(), pos,
            [](std::size_t position, const LinkSpan& link){
                return position < link.range.start;
            });
        if (above == links.begin())
            return std::nullopt;

        const LinkSpan& link = *std::prev(above);
        if (pos >= link.range.end())
            return std::nullopt;

        return LinkHit{ link, pos };
    }

    std::size_t TextLayout::rowStart(CaretHit caretHit)
    {
        ensureLayout();
        const std::size_t index = paragraphAt(caretHit.pos);
        if (index == k_maxSize)
            return 0;

        const ParagraphLayoutState& p = m_paragraphs[index];
        const std::size_t pEnd = p.textStart + p.textLength;
        const std::span<const NativeLineMetrics> lines = linesOf(p);
        if (lines.empty())
            return p.textStart;

        // Map Logical index back to the underlying visual character index
        const std::size_t relPos = caretHit.pos - p.textStart;
        std::size_t testPos = relPos;
        if (caretHit.trailing && testPos > 0)
            testPos--;
        if (testPos >= p.textLength && p.textLength > 0)
            testPos = p.textLength - 1;

        for (const NativeLineMetrics& line : lines)
        {
            if (testPos >= line.textStart && testPos < line.textStart + line.textLength)
            {
                const std::size_t startPos = p.textStart + line.textStart;
                return startPos > pEnd ? pEnd : startPos;
            }
        }
        return p.textStart;
    }

    std::size_t TextLayout::rowEnd(CaretHit caretHit)
    {
        ensureLayout();
        const std::size_t index = paragraphAt(caretHit.pos);
        if (index == k_maxSize)
            return m_bakedText.plainText().length();

        const ParagraphLayoutState& p = m_paragraphs[index];
        const std::size_t pEnd = p.textStart + p.textLength;
        const std::span<const NativeLineMetrics> lines = linesOf(p);
        if (lines.empty())
            return p.textStart;

        // Map Logical index back to the underlying visual character index
        const std::size_t relPos = caretHit.pos - p.textStart;
        std::size_t testPos = relPos;
        if (caretHit.trailing && testPos > 0)
            testPos--;
        if (testPos >= p.textLength && p.textLength > 0)
            testPos = p.textLength - 1;

        for (const NativeLineMetrics& line : lines)
        {
            if (testPos >= line.textStart && testPos < line.textStart + line.textLength)
            {
                const std::size_t endPos = p.textStart + line.textStart + line.textLength;
                return endPos > pEnd ? pEnd : endPos;
            }
        }
        return pEnd;
    }

    TextLineColumn TextLayout::lineColumnAt(std::size_t pos)
    {
        ensureLayout();
        if (m_paragraphs.empty())
            return {};

        // The last paragraph starting at or before the position. A paragraph's range stops short
        // of the newline that ends it, so the position one past its last character is still that
        // paragraph's own end rather than the next one's start - which is what the upper bound of
        // the starts tells apart and a search for a containing range does not. The first
        // paragraph starts at zero, so the bound is never the beginning of the range.
        const auto above = std::upper_bound(m_paragraphs.begin(), m_paragraphs.end(), pos,
            [](std::size_t position, const ParagraphLayoutState& paragraph){
                return position < paragraph.textStart;
            });
        const std::size_t index = static_cast<std::size_t>(above - m_paragraphs.begin()) - 1;
        return {
            index + 1,
            pos - m_paragraphs[index].textStart + 1,
        };
    }

    CaretHit TextLayout::posOnNextRow(CaretHit caretHit, ScrollDirection dir, std::optional<float> targetX)
    {
        FloatRect cRect = getCaretRect(caretHit);
        FloatPoint targetPt;
        targetPt.x = targetX.has_value() ? targetX.value() : cRect.left;
        targetPt.y = (dir == ScrollDirection::ToBegin) ? cRect.top - 1.0f : cRect.bottom + 1.0f;
        return caretPos(targetPt);
    }

    FloatRect TextLayout::getCaretRect(CaretHit caretHit)
    {
        ensureLayout();
        // A whole-text selection names its end as k_maxSize, which stands for the end of the
        // text - the clamp TextBox::keyDown and caretLineColumn already make before measuring.
        caretHit.pos = std::min(caretHit.pos, m_bakedText.plainText().length());
        const std::size_t index = paragraphAt(caretHit.pos);
        if (index == k_maxSize)
            return { 0, 0, 0, 0 };

        const ParagraphLayoutState& p = m_paragraphs[index];
        const std::size_t relPos = caretHit.pos - p.textStart;

        std::size_t dwPos;
        bool dwTrailing;

        // Safely translate the Logical String index and affinity back into
        // DirectWrite valid Character Index and bounds boundary mapping.
        if (relPos == 0)
        {
            dwPos = 0;
            dwTrailing = false;
        }
        else if (relPos >= p.textLength)
        {
            dwPos = p.textLength > 0 ? p.textLength - 1 : 0;
            dwTrailing = p.textLength > 0;
        }
        else
        {
            if (caretHit.trailing)
            {
                dwPos = relPos - 1;
                dwTrailing = true;
            }
            else
            {
                dwPos = relPos;
                dwTrailing = false;
            }
        }

        FloatRect resultRect = charRect(p, dwPos, dwTrailing);
        resultRect.offset(p.bounds.topLeft());
        return resultRect;
    }

    // Lines broken at m_builtBoundsX came out no wider than m_shapedWidth, so every width between
    // the two breaks them the same: none has to split, because each already fits, and none can
    // join, because the breaker refused the next word at the wider of the two. A text too wide for
    // the box it was given puts m_shapedWidth above m_builtBoundsX and so accepts only the width
    // it was built at, which is what a run with nowhere to break should do. A layout holding no
    // shaping accepts anything, having nothing to lose.
    //
    // The lower end carries k_fitTolerance: a box the lines reach past by less than that holds
    // them by the fit test's own rule, and a box read back out of a rect built from the measured
    // width comes to exactly that - a ULP under the lines, which compared outright is a second
    // shaping of the whole text for nothing.
    bool TextLayout::acceptsWidth(float boundsX) const
    {
        if (!m_layoutValid)
            return true;
        // Lines broken at a STATED width are the lines this layout holds whatever box it is given:
        // a wider box shows a line the one they were broken at cut, standing where that box put
        // it.
        if (m_breakWidth > 0.0f)
            return true;
        // A text that is not broken to its box is not shaped from a width at all.
        if (!m_wrap)
            return true;
        if (boundsX == m_builtBoundsX)
            return true;
        return boundsX < m_builtBoundsX && boundsX + k_fitTolerance >= m_shapedWidth;
    }

    void TextLayout::invalidate()
    {
        m_layoutValid = false;
        m_verticalValid = false;
    }

    void TextLayout::ensureLayout()
    {
        ensureShaping();
        ensureBoxWidth();
        ensureVerticalFit();
    }

    // The lines themselves: what the text says, the width it is broken at, the scale it is drawn
    // at. Nothing here reads the height.
    void TextLayout::ensureShaping()
    {
        if (m_layoutValid && m_text)
            return;

        m_bakedText.rebuild(*m_text, m_editable);
        m_paragraphs.clear();
        m_lines.clear();
        m_held.clear();
        m_paragraphs.reserve(m_bakedText.paragraphs().size());

        float currentY = 0.0f;
        float maxW = 0.0f;
        const float breakAt = breakWidth();

        SpanCursors cursors;
        for (const auto& para : m_bakedText.paragraphs())
        {
            ParagraphLayoutState state = shapeParagraph(para, currentY, breakAt, cursors, m_lines);
            currentY = state.bounds.bottom;
            maxW = std::max(maxW, state.bounds.right);
            // The first stay built: all a short text has, and what a long one shows first.
            if constexpr (k_releaseNativeLayouts)
            {
                if (state.nativeLayout)
                {
                    if (m_held.count() < k_heldLayoutBudget)
                        m_held.add(m_paragraphs.size());
                    else
                        state.nativeLayout.reset();
                }
            }
            m_paragraphs.push_back(std::move(state));
        }

        m_shapedWidth = maxW;
        m_shapedHeight = currentY;
        m_builtBoundsX = breakAt;
        m_boxWidthApplied = breakAt;
        m_layoutValid = true;
        m_verticalValid = false;
    }

    // What the shaping comes to inside the box it was given: the line the text is cut off at, the
    // lines that fade, and the size the whole of it came to. Read off the lines rather than shaped
    // from, so a box of another height costs this walk and no glyph.
    void TextLayout::ensureVerticalFit()
    {
        if (m_verticalValid)
            return;

        m_globalBaselinesToFade.clear();
        m_globalCollapseBaseline = -9999.0f;
        m_collapseXOffset = 0.0f;

        bool hasCollapsed = false;
        float lastValidBaseline = -9999.0f;
        float appendedLinesWidth = 0.0f;
        TextAlign collapseAlignment = TextAlign::Left;
        bool isFirstParagraph = true;
        float constraintsY = m_bounds.y;

        for (const auto& p : m_paragraphs)
        {
            const std::span<const NativeLineMetrics> lines = linesOf(p);
            if (lines.empty())
                continue;

            float lineY = 0.0f;
            bool needsGap = !isFirstParagraph;
            isFirstParagraph = false;

            for (const NativeLineMetrics& line : lines)
            {
                float absoluteTop = p.bounds.top + lineY;
                // Not past the paragraph it stands in. A paragraph's height is what the shaping
                // said it was, and m_shapedHeight - the number a control's own height is taken
                // from - is the sum of those. The lines are added up here a second time, out of
                // the line metrics rather than the paragraph metrics, and the two totals need not
                // agree to the last bit: a hundredth of a pixel over on the last line is a text
                // declared not to fit a box that is exactly as tall as it is.
                float absoluteBottom = std::min(absoluteTop + line.height, p.bounds.bottom);
                float absoluteBaseline = absoluteTop + line.baseline;

                if (!hasCollapsed)
                {
                    if (absoluteBottom > constraintsY + k_fitTolerance)
                    {
                        hasCollapsed = true;
                        collapseAlignment = p.alignment;
                        if (lastValidBaseline > -9000.0f)
                        {
                            m_globalCollapseBaseline = lastValidBaseline;
                            if (needsGap)
                                appendedLinesWidth += line.height * 0.25f;

                            appendedLinesWidth += line.width;
                        }
                        else
                        {
                            m_globalCollapseBaseline = absoluteBaseline;
                        }
                        m_globalBaselinesToFade.push_back(m_globalCollapseBaseline);
                    }
                    else
                    {
                        lastValidBaseline = absoluteBaseline;
                        float availableW = std::max(1.0f, m_bounds.x - p.bounds.left);
                        if (line.isTrimmed || line.width > availableW + k_fitTolerance)
                        {
                            m_globalBaselinesToFade.push_back(absoluteBaseline);
                        }
                    }
                }
                else
                {
                    if (needsGap)
                        appendedLinesWidth += line.height * 0.25f;

                    appendedLinesWidth += line.width;
                }

                needsGap = false;
                lineY += line.height;
            }
        }

        if (hasCollapsed)
        {
            if (collapseAlignment == TextAlign::Center)
                m_collapseXOffset = -appendedLinesWidth / 2.0f;
            else if (collapseAlignment == TextAlign::Right)
                m_collapseXOffset = -appendedLinesWidth;
        }

        m_calcWidth = std::min(m_bounds.x, m_shapedWidth);
        m_calcHeight = std::min(m_bounds.y, m_shapedHeight);
        m_verticalValid = true;
    }

    // A native layout places its lines inside the box it was told about, so a right-aligned or
    // centred run keeps the offset the width it was built at gave it until this says otherwise.
    // Only the placement moves: the box is never wider than the lines were broken at, so none of
    // them can join and none has to split - see acceptsWidth.
    void TextLayout::ensureBoxWidth()
    {
        // A stated break width is the box the lines are PLACED in as well as broken at: a centred
        // paragraph stands where the text it repeats stands, and a wider box would centre it
        // somewhere else.
        if (m_breakWidth > 0.0f)
            return;
        if (m_boxWidthApplied == m_bounds.x)
            return;

        for (const ParagraphLayoutState& p : m_paragraphs)
        {
            // A leading line sits at the leading edge whatever the box comes to, so it has no
            // offset to recompute and nothing to be told. Telling it anyway is not free: every
            // setter discards the analysis the build paid for, and the GetMetrics inside
            // setMaxWidth then pays for it again - once per paragraph, for a placement that
            // cannot have moved.
            //
            // Nothing setMaxWidth writes is missed. The alignment it resolves again is LEADING for
            // a Left paragraph whatever the box, and the metrics cannot have moved, because
            // acceptsWidth admits only a width that re-breaks no line.
            if (p.alignment == TextAlign::Left)
                continue;
            // A paragraph on cells reads the box as it is asked - see monoOffset - and one whose
            // native layout was given back is told the box when it is built again.
            if (!p.nativeLayout)
                continue;

            p.nativeLayout->setMaxWidth(std::max(1.0f, m_bounds.x - p.indent));
        }

        m_boxWidthApplied = m_bounds.x;
    }

    float TextLayout::breakWidth() const
    {
        return m_breakWidth > 0.0f ? m_breakWidth : m_bounds.x;
    }

    float TextLayout::placementWidth() const
    {
        return m_breakWidth > 0.0f ? m_breakWidth : m_bounds.x;
    }

    std::span<const NativeLineMetrics> TextLayout::linesOf(const ParagraphLayoutState& p) const
    {
        return std::span<const NativeLineMetrics>{ m_lines }.subspan(p.lineStart, p.lineCount);
    }

    std::wstring_view TextLayout::paragraphText(const ParagraphLayoutState& p) const
    {
        return std::wstring_view{ m_bakedText.plainText() }.substr(p.textStart, p.textLength);
    }

    std::size_t TextLayout::paragraphAt(std::size_t pos) const
    {
        // The first paragraph starts at zero, so the bound is never the beginning of the range.
        const auto above = std::upper_bound(m_paragraphs.begin(), m_paragraphs.end(), pos,
            [](std::size_t position, const ParagraphLayoutState& paragraph){
                return position < paragraph.textStart;
            });
        const std::size_t index = static_cast<std::size_t>(above - m_paragraphs.begin()) - 1;
        const ParagraphLayoutState& p = m_paragraphs[index];
        return pos <= p.textStart + p.textLength ? index : k_maxSize;
    }

    std::size_t TextLayout::paragraphAtY(float y) const
    {
        const auto below = std::upper_bound(m_paragraphs.begin(), m_paragraphs.end(), y,
            [](float position, const ParagraphLayoutState& paragraph){
                return position < paragraph.bounds.bottom;
            });
        if (below == m_paragraphs.end())
            return m_paragraphs.size() - 1;
        return static_cast<std::size_t>(below - m_paragraphs.begin());
    }

    // The same rule the native layouts apply: a line wider than the box has no room left to be
    // moved in, and moving it anyway carries its start off the leading edge where the clip
    // takes it. Measured without the trailing whitespace, which is what a line is allowed to
    // overhang with.
    float TextLayout::monoOffset(const ParagraphLayoutState& p) const
    {
        if (p.alignment == TextAlign::Left)
            return 0.0f;

        const float box = std::max(1.0f, placementWidth() - p.indent);
        const float ink = linesOf(p).front().width;
        if (ink > box + 0.1f)
            return 0.0f;

        switch (p.alignment)
        {
            case TextAlign::Center:
                return (box - ink) / 2.0f;
            case TextAlign::Right:
                return box - ink;
            case TextAlign::Left:
            case TextAlign::Justified:
                break;
        }
        return 0.0f;
    }

    FloatRect TextLayout::charRect(const ParagraphLayoutState& p, std::size_t localPos,
        bool trailing)
    {
        if (!p.monoFont)
            return nativeOf(p).getCharRect(localPos, trailing);

        // The position after the last character is the trailing edge of that character, and an
        // empty paragraph has one edge, which is both of its ends.
        const std::wstring_view text = paragraphText(p);
        std::size_t edge = std::min(localPos, text.size());
        if (trailing && edge < text.size())
            ++edge;
        const float x = monoOffset(p) + p.monoFont->cellLeft(text, edge);
        return { x, 0.0f, x, p.monoFont->lineHeight() };
    }

    void TextLayout::drawLinkUnderline(ControlPaintContext& controlContext,
        const ParagraphLayoutState& paragraph, FloatPoint origin,
        std::span<const ColorSpan> colors)
    {
        const TextRange local = getLocalRange(m_hoveredLink, paragraph.textStart,
            paragraph.textStart + paragraph.textLength);
        if (!local.length)
            return;

        const std::wstring_view text = paragraphText(paragraph);
        const float thickness = controlContext.scaledStrokeWidth(Thickness::Thin);
        const float gap = thickness * k_underlineInkGap;
        const Color fallback = controlContext.textRgb(InkGrade::Strongest);
        std::vector<Graphics::InkExtent>& ink = g_textDrawBuffers.ink;
        std::size_t colorCursor = 0;
        for (const NativeLineMetrics& line : linesOf(paragraph))
        {
            // A line broken inside the link ends in the blank it broke at, which stands past the
            // last glyph the line shows - nothing is underlined there.
            std::size_t inkEnd = line.textStart + line.textLength;
            while (inkEnd > line.textStart && std::iswspace(text[inkEnd - 1]))
                --inkEnd;

            const std::size_t segmentStart = std::max(local.start, line.textStart);
            const std::size_t segmentEnd = std::min(local.end(), inkEnd);
            if (segmentEnd <= segmentStart)
                continue;

            const FloatRect leading = charRect(paragraph, segmentStart, false);
            const FloatRect trailing = charRect(paragraph, segmentEnd - 1, true);
            const float baseline = leading.top + line.baseline;
            const float top = std::round(origin.y + baseline
                + (line.height - line.baseline) * k_underlineDescentShare);

            while (colorCursor != colors.size() && colors[colorCursor].range.end() <= segmentStart)
                ++colorCursor;
            Color color = fallback;
            if (colorCursor != colors.size() && colors[colorCursor].range.start <= segmentStart)
            {
                const ColorDef& value = colors[colorCursor].value;
                color = value.index() == 0
                    ? controlContext.inkRgb(std::get<0>(value))
                    : std::get<1>(value);
            }

            // THE LINE IS CUT WHERE A GLYPH'S INK CROSSES IT, the way a browser draws an
            // underline: a descender stands clear of it by the gap on either side. A piece left
            // no longer than the line is thick would read as a dot between two descenders, and
            // is not drawn.
            const float bandTop = top - origin.y;
            ink.clear();
            appendInkAcross(paragraph, { segmentStart, segmentEnd - segmentStart }, bandTop,
                bandTop + thickness, ink);
            std::ranges::sort(ink, {}, &Graphics::InkExtent::left);

            const auto fillPiece = [&](float pieceLeft, float pieceRight){
                if (pieceRight - pieceLeft <= thickness)
                    return;
                controlContext.canvas().fillRectangle({
                    origin.x + pieceLeft,
                    top,
                    origin.x + pieceRight,
                    top + thickness,
                }, color);
            };
            const float right = std::max(leading.left, trailing.left);
            float from = std::min(leading.left, trailing.left);
            for (const Graphics::InkExtent& extent : ink)
            {
                const float cutLeft = extent.left - gap;
                const float cutRight = extent.right + gap;
                if (cutRight <= from)
                    continue;
                if (cutLeft >= right)
                    break;
                fillPiece(from, std::min(cutLeft, right));
                from = std::max(from, cutRight);
            }
            fillPiece(from, right);
        }
    }

    void TextLayout::appendInkAcross(const ParagraphLayoutState& paragraph, TextRange localRange,
        float bandTop, float bandBottom, std::vector<Graphics::InkExtent>& result)
    {
        if (!paragraph.monoFont)
        {
            nativeOf(paragraph).appendInkAcross(localRange, bandTop, bandBottom, result);
            return;
        }

        const std::size_t first = result.size();
        paragraph.monoFont->appendInkAcross(paragraphText(paragraph), localRange, bandTop,
            bandBottom, result);
        const float offset = monoOffset(paragraph);
        for (std::size_t i = first; i != result.size(); ++i)
        {
            result[i].left += offset;
            result[i].right += offset;
        }
    }

    MonoFont* TextLayout::monoFontFor(const ParagraphStyle& para, std::wstring_view slice,
        const SpanCursors& cursors) const
    {
        // Justified stretches its spaces to the box, which is not a cell.
        if (para.alignment == TextAlign::Justified)
            return nullptr;

        const std::size_t pStart = para.range.start;
        const std::size_t pEnd = pStart + para.range.length;

        const std::vector<std::pair<std::size_t, BakedInlineObject>>& objects
            = m_bakedText.inlineObjects();
        if (cursors.inlineObjects != objects.size() && objects[cursors.inlineObjects].first < pEnd)
            return nullptr;
        const std::vector<ScriptSpan>& scripts = m_bakedText.scripts();
        if (cursors.scripts != scripts.size() && scripts[cursors.scripts].range.start < pEnd)
            return nullptr;

        // The font a native build would read at the paragraph's first character, provided the
        // span stating each part of it reaches the paragraph's end.
        const TextStyle& body = k_textStyles[static_cast<std::size_t>(TextStyleId::Body)];
        std::wstring_view family = body.family;
        float size = body.size;
        FontWeight weight = body.weight;
        FontStyle style = body.style;

        const std::vector<FamilySpan>& families = m_bakedText.families();
        switch (spanReach(families, cursors.families, pStart, pEnd))
        {
            case SpanReach::Part:
                return nullptr;
            case SpanReach::Whole:
                family = families[cursors.families].value;
                break;
            case SpanReach::None:
                break;
        }
        const std::vector<SizeSpan>& sizes = m_bakedText.sizes();
        switch (spanReach(sizes, cursors.sizes, pStart, pEnd))
        {
            case SpanReach::Part:
                return nullptr;
            case SpanReach::Whole:
                size = sizes[cursors.sizes].value;
                break;
            case SpanReach::None:
                break;
        }
        const std::vector<WeightSpan>& weights = m_bakedText.weights();
        switch (spanReach(weights, cursors.weights, pStart, pEnd))
        {
            case SpanReach::Part:
                return nullptr;
            case SpanReach::Whole:
                weight = weights[cursors.weights].value;
                break;
            case SpanReach::None:
                break;
        }
        const std::vector<StyleSpan>& styles = m_bakedText.styles();
        switch (spanReach(styles, cursors.styles, pStart, pEnd))
        {
            case SpanReach::Part:
                return nullptr;
            case SpanReach::Whole:
                style = styles[cursors.styles].value;
                break;
            case SpanReach::None:
                break;
        }

        MonoFont* font = monoFont(family, size * m_scaleFactor, weight, style);
        if (!font || !font->covers(slice))
            return nullptr;
        return font;
    }

    ParagraphLayoutState TextLayout::shapeParagraph(const ParagraphStyle& para, float top,
        float boundsWidth, SpanCursors& cursors, Lines& lines)
    {
        const float paraIndent = para.indent * m_scaleFactor;
        const float availableWidth = std::max(1.0f, boundsWidth - paraIndent);

        const std::size_t pStart = para.range.start;
        const std::size_t pEnd = pStart + para.range.length;
        const std::wstring_view text = std::wstring_view{ m_bakedText.plainText() }
            .substr(pStart, para.range.length);

        advanceSpanCursor(m_bakedText.families(), cursors.families, pStart);
        advanceSpanCursor(m_bakedText.sizes(), cursors.sizes, pStart);
        advanceSpanCursor(m_bakedText.weights(), cursors.weights, pStart);
        advanceSpanCursor(m_bakedText.styles(), cursors.styles, pStart);
        advanceSpanCursor(m_bakedText.scripts(), cursors.scripts, pStart);
        // Taken through the cursor the walk carries. A sweep of the whole document's list per
        // paragraph is what a page of formatted text cannot afford: it costs paragraphs times
        // objects, and a written page carries one on most lines.
        advanceInlineCursor(m_bakedText.inlineObjects(), cursors.inlineObjects, pStart);

        // A paragraph set in one monospace font is one row of cells, measured here and holding
        // no native layout. A wrapping layout keeps it on cells while its ink fits the width,
        // since a line that fits is never broken; one that does not is broken natively. See
        // TextEngine-Types#mono-paragraph
        if (MonoFont* font = monoFontFor(para, text, cursors))
        {
            const MonoExtent extent = font->extent(text);
            if (!m_wrap || extent.inkWidth <= availableWidth)
            {
                lines.push_back({
                    .textStart = 0,
                    .textLength = para.range.length,
                    .width = extent.inkWidth,
                    .height = font->lineHeight(),
                    .baseline = font->baseline(),
                    .isTrimmed = false,
                });
                return {
                    .nativeLayout = nullptr,
                    .monoFont = font,
                    .bounds = {
                        paraIndent,
                        top,
                        paraIndent + extent.width,
                        top + font->lineHeight(),
                    },
                    .textStart = pStart,
                    .textLength = para.range.length,
                    .indent = paraIndent,
                    .alignment = para.alignment,
                    .lineStart = lines.size() - 1,
                    .lineCount = 1,
                };
            }
        }

        // Both builds below read the slice, so the copy it may stand on lives to the end.
        std::wstring standIns{};
        std::wstring_view slice;
        bool isEmpty = false;
        if (para.range.length == 0)
        {
            slice = std::wstring_view{ &k_zeroWidthSpace, 1 };
            isEmpty = true;
        }
        else
        {
            slice = withoutNativeLineEnds(text, standIns);
        }

        INativeTextLayout::BuildParams buildParams{
            .text = slice,
            .availableWidth = availableWidth,
            .scaleFactor = m_scaleFactor,
            .isEmpty = isEmpty,
            .wrap = m_wrap,
            .inlineObjects = {}
        };

        collectLocalSpans(m_bakedText.families(), pStart, pEnd, cursors.families, buildParams.families);
        collectLocalSpans(m_bakedText.sizes(), pStart, pEnd, cursors.sizes, buildParams.sizes);
        collectLocalSpans(m_bakedText.weights(), pStart, pEnd, cursors.weights, buildParams.weights);
        collectLocalSpans(m_bakedText.styles(), pStart, pEnd, cursors.styles, buildParams.styles);

        // The objects standing in this paragraph, from the cursor on.
        for (std::size_t i = cursors.inlineObjects; i != m_bakedText.inlineObjects().size(); ++i)
        {
            const std::pair<std::size_t, BakedInlineObject>& object = m_bakedText.inlineObjects()[i];
            // Ordered, so the first object standing past this paragraph ends the answer.
            if (object.first >= pEnd)
                break;

            BakedInlineObject localIcon = object.second;
            // A tab stop lands what follows it at a stated X, so it is measured as nothing and
            // given its width by the pass below. A flex space is measured at the width it states
            // and grown from there: that width is its floor, and the floor is what a line with
            // nothing to spare leaves standing.
            if (localIcon.isTabTo)
                localIcon.width = 0.0f;

            buildParams.inlineObjects.push_back({ object.first - pStart, localIcon });
        }

        // The paragraph's lines are the pool's from here on; a rebuild drops them and appends
        // again.
        const std::size_t lineStart = lines.size();
        auto nativeLayout = createNativeTextLayout();
        nativeLayout->build(buildParams);
        nativeLayout->appendLineMetrics(lines);
        bool needsRebuild = false;

        // --- RESOLVE TAB-STOPS AND FLEX-SPACES LINE-BY-LINE ---
        const std::span<const NativeLineMetrics> built = std::span<const NativeLineMetrics>{ lines }
            .subspan(lineStart);
        for (const NativeLineMetrics& line : built)
        {
            float accumulatedShift = 0.0f;
            std::vector<std::size_t> lineFillers;

            // Pass 1: Resolve TabTo Spacers and accumulate their shifts
            for (std::size_t i = 0; i < buildParams.inlineObjects.size(); ++i)
            {
                auto& [localIdx, bakedIcon] = buildParams.inlineObjects[i];
                if (localIdx >= line.textStart && localIdx < line.textStart + line.textLength)
                {
                    if (bakedIcon.isTabTo)
                    {
                        float hx = nativeLayout->getCharX(localIdx, false);
                        float spacedX = hx + accumulatedShift;

                        // The stop the tag was written with, which the copy above kept while it
                        // zeroed the width the resolve is about to state.
                        float targetX = (bakedIcon.tabTargetX * m_scaleFactor) - paraIndent;
                        float requiredWidth = std::max(0.0f, targetX - spacedX);

                        bakedIcon.width = requiredWidth / m_scaleFactor;
                        accumulatedShift += requiredWidth;
                        needsRebuild = true;
                    }
                    else if (bakedIcon.isFlexSpace)
                    {
                        lineFillers.push_back(i);
                    }
                }
            }

            // Pass 2: Distribute remaining space amongst FlexSpace Fillers
            if (!lineFillers.empty() && m_eventPhase == EventPhase::Paint)
            {
                float totalUsedWidth = line.width + accumulatedShift;
                float remainingSpace = availableWidth - totalUsedWidth;
                float spacePerFiller = remainingSpace / lineFillers.size() / m_scaleFactor;
                if (spacePerFiller > 0.01f)
                {
                    // ON TOP OF THE FLOOR each filler was measured at, which is why the share is
                    // added rather than stated: line.width above counted those floors, so what is
                    // shared out here is the room left beyond them.
                    for (std::size_t idx : lineFillers)
                    {
                        buildParams.inlineObjects[idx].second.width += spacePerFiller;
                    }
                    needsRebuild = true;
                }
            }
        }

        if (needsRebuild)
        {
            nativeLayout->build(buildParams);
            lines.resize(lineStart);
            nativeLayout->appendLineMetrics(lines);
        }

        // The size the paragraph came to, read BEFORE anything places its lines. Alignment says
        // where a line sits inside the box, not how much room the text needs, so the two must not
        // be read off one another - and reading them the other way round is not merely untidy. A
        // centred layout reports its width relative to a centred origin, and a measuring pass asks
        // at an effectively infinite width: at 2^31 a float's ULP is 256, so those widths come back
        // rounded to multiples of 64 and the paragraph measures NARROWER than its text. Measured
        // before alignment the origin is zero and there is no such loss.
        const NativeParagraphMetrics metrics = nativeLayout->getMetrics();

        // After the last build, which would have dropped it, and in EVERY phase: the cache hands
        // the layout a calculate pass built to the paint that follows, so a placement left to the
        // paint phase never reaches a text the align pass saw first. A Left paragraph costs
        // nothing here - the native layout answers an alignment it already applies without a
        // setter - and the size read above is the same wherever the lines sit.
        nativeLayout->setAlignment(para.alignment);
        return {
            .nativeLayout = std::move(nativeLayout),
            .monoFont = nullptr,
            .bounds = { paraIndent, top, paraIndent + metrics.width, top + metrics.height },
            .textStart = pStart,
            .textLength = para.range.length,
            .indent = paraIndent,
            .alignment = para.alignment,
            .lineStart = lineStart,
            .lineCount = lines.size() - lineStart,
        };
    }

    INativeTextLayout& TextLayout::nativeOf(const ParagraphLayoutState& paragraph)
    {
        const std::size_t index = static_cast<std::size_t>(&paragraph - m_paragraphs.data());
        ParagraphLayoutState& held = m_paragraphs[index];
        if constexpr (k_releaseNativeLayouts)
        {
            if (!held.nativeLayout)
            {
                // Room is made before it comes in, so the paragraph answered for is not the one
                // given back. A draw makes none until it is over.
                if (!m_held.drawing())
                    releaseNatives(m_held.trim(k_heldLayoutBudget - 1));
                held.nativeLayout = rebuiltNative(index);
                m_held.add(index);
            }
        }
        return *held.nativeLayout;
    }

    std::unique_ptr<INativeTextLayout> TextLayout::rebuiltNative(std::size_t paragraph)
    {
        const ParagraphStyle& style = m_bakedText.paragraphs()[paragraph];
        SpanCursors cursors = cursorsAt(style.range.start);
        m_rebuiltLines.clear();
        ParagraphLayoutState state = shapeParagraph(style, 0.0f, m_builtBoundsX, cursors,
            m_rebuiltLines);
        if (!state.nativeLayout)
            unreachable("A paragraph shaped natively came out on cells when it was built again");

        // The box ensureBoxWidth told the held layouts about since the text was shaped.
        if (m_boxWidthApplied != m_builtBoundsX && style.alignment != TextAlign::Left)
            state.nativeLayout->setMaxWidth(std::max(1.0f, m_boxWidthApplied - state.indent));
        return std::move(state.nativeLayout);
    }

    TextLayout::SpanCursors TextLayout::cursorsAt(std::size_t pos) const
    {
        return {
            .families = spanCursorAt(m_bakedText.families(), pos),
            .sizes = spanCursorAt(m_bakedText.sizes(), pos),
            .weights = spanCursorAt(m_bakedText.weights(), pos),
            .styles = spanCursorAt(m_bakedText.styles(), pos),
            .scripts = spanCursorAt(m_bakedText.scripts(), pos),
            .inlineObjects = inlineCursorAt(m_bakedText.inlineObjects(), pos),
        };
    }

    void TextLayout::releaseNatives(std::span<const std::size_t> paragraphs)
    {
        for (const std::size_t paragraph : paragraphs)
            m_paragraphs[paragraph].nativeLayout.reset();
    }

}
