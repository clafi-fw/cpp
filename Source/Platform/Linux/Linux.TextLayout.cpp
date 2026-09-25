module;
#include "Linux.TextHeaders.h"
module ClaFi.Platform.Linux.TextLayout;

import ClaFi.Platform.Linux.Fonts;

import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.BakedText;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.Graphics.Cpu_GlyphCompositor;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Platform::Linux
{
    // The paragraph text is handed to HarfBuzz and libunibreak as UTF-32 in place, which is what a
    // wchar_t is on Linux. A position in the text is then a cluster value and a break index alike,
    // with nothing to translate.
    static_assert(sizeof(wchar_t) == 4);

    namespace
    {
        // A line wider than its box by less than this still counts as fitting it.
        constexpr float k_fitEpsilon = 0.01f;
        // How many destroyed layouts' buffers are kept for the layouts made after them.
        constexpr std::size_t k_maxSpareBuffers = 256;
        // A layout whose buffers grew past this many characters gives them to the allocator.
        constexpr std::size_t k_maxSpareCharacters = 1024;

        // One shaping buffer for the application. It is scratch for the length of a build: a
        // document is one layout per paragraph, and a buffer per layout would stand idle fifty
        // thousand times over on a page of source. One is enough while no build runs inside a
        // build.
        class ShapingBuffer
        {
        public:
            ShapingBuffer();
            ~ShapingBuffer();
            ShapingBuffer(const ShapingBuffer&) = delete;
            ShapingBuffer& operator=(const ShapingBuffer&) = delete;
        public:
            [[nodiscard]] hb_buffer_t* get() const { return m_buffer; }
        private:
            hb_buffer_t* m_buffer;
        };

        hb_buffer_t* shapingBuffer()
        {
            static ShapingBuffer s_buffer{};
            return s_buffer.get();
        }

        // The break class of every character of the paragraph being built, as libunibreak states
        // it: what may follow that character. Scratch of the build, like the shaping buffer.
        std::vector<char> g_breaks{};

        // What a line is allowed to end with and overhang the box by: the space, the ASCII
        // controls that separate lines, the line and paragraph separators and the ideographic
        // space. A no-break space is not among them, being a character a line keeps.
        bool isWhitespace(wchar_t character)
        {
            switch (character)
            {
                case L' ':
                case L'\t':
                case L'\n':
                case L'\r':
                case L'\v':
                case L'\f':
                case 0x2028:
                case 0x2029:
                case 0x3000:
                    return true;
                default:
                    return false;
            }
        }

        // A character that is set in the face of the text around it rather than in one of its own:
        // whitespace, combining marks, variation selectors and the invisible format characters. A
        // mark drawn from another face than its base would not sit on it, and a space is in every
        // face.
        bool followsText(wchar_t character)
        {
            if (isWhitespace(character))
                return true;
            const std::uint32_t code = static_cast<std::uint32_t>(character);
            return (code >= 0x0300 && code <= 0x036F)
                || (code >= 0x1AB0 && code <= 0x1AFF)
                || (code >= 0x1DC0 && code <= 0x1DFF)
                || (code >= 0x200B && code <= 0x200F)
                || (code >= 0x2060 && code <= 0x2064)
                || (code >= 0x20D0 && code <= 0x20FF)
                || (code >= 0xFE00 && code <= 0xFE0F)
                || (code >= 0xFE20 && code <= 0xFE2F)
                || code == 0xFEFF
                || (code >= 0xE0100 && code <= 0xE01EF);
        }

        // The value of the span standing at pos, or the fallback. The cursor is advanced past the
        // spans ending before pos, so a walk in text order pays one pass over the spans.
        template <typename Span, typename Value>
        Value walkSpanValue(std::span<const Span> spans, std::size_t pos, std::size_t& cursor, const Value& fallback)
        {
            while (cursor != spans.size() && spans[cursor].range.end() <= pos)
                ++cursor;
            if (cursor != spans.size() && spans[cursor].range.start <= pos)
                return spans[cursor].value;
            return fallback;
        }

        // The value in force at pos, searched rather than walked: the build asks once per segment,
        // and a segment boundary is every span edge, so this is segments times spans over a
        // paragraph carrying a handful of each.
        template <typename Span, typename Value>
        Value findSpanValue(const std::vector<Span>& spans, std::size_t pos, Value fallback)
        {
            for (const Span& span : spans)
            {
                if (span.range.start <= pos && pos < span.range.end())
                    return span.value;
            }
            return fallback;
        }

        // What an empty paragraph is styled with: the last value stated, which is the style in
        // force where the paragraph stands, or the body style with nothing stated.
        template <typename Span, typename Value>
        Value lastSpanValue(const std::vector<Span>& spans, Value fallback)
        {
            return spans.empty() ? fallback : spans.back().value;
        }

        // The value in force at the paragraph's first character, or what an empty paragraph is
        // styled with.
        template <typename Span, typename Value>
        Value leadingSpanValue(const std::vector<Span>& spans, bool isEmpty, Value fallback)
        {
            return isEmpty ? lastSpanValue(spans, fallback) : findSpanValue(spans, 0, fallback);
        }

        template <typename Span>
        void collectEdges(const std::vector<Span>& spans, std::vector<std::size_t>& edges)
        {
            for (const Span& span : spans)
            {
                edges.push_back(span.range.start);
                edges.push_back(span.range.end());
            }
        }

        Color colorOf(const ColorDef& value, const ControlPaintContext& context)
        {
            return value.index() == 0
                ? context.inkRgb(std::get<0>(value))
                : std::get<1>(value);
        }

        ShapingBuffer::ShapingBuffer()
            :
            m_buffer{ hb_buffer_create() }
        {
        }

        ShapingBuffer::~ShapingBuffer()
        {
            hb_buffer_destroy(m_buffer);
        }
    }


    //-------------------------------------------------------------------------


    void HarfBuzzLayout::build(const BuildParams& params)
    {
        m_textLength = params.text.size();
        m_scaleFactor = params.scaleFactor > 0.0f ? params.scaleFactor : 1.0f;
        m_availableWidth = params.availableWidth;
        m_maxWidth = params.availableWidth;
        // A new layout is aligned Left whatever the old one was aligned to; a rebuild states it
        // again.
        m_alignment = TextAlign::Left;

        m_glyphs.clear();
        m_items.clear();
        m_runs.clear();
        m_lines.clear();
        m_objects.clear();
        m_chars.assign(m_textLength, CharCell{});
        for (std::size_t i = 0; i != m_textLength; ++i)
        {
            m_chars[i].whitespace = isWhitespace(params.text[i]);
            m_chars[i].stretchable = params.text[i] == L' ';
            m_chars[i].tab = params.text[i] == L'\t';
        }
        m_tabInterval = k_tabStopSpaces * leadingSpaceAdvance(params);

        itemize(params);

        g_breaks.resize(m_textLength);
        set_linebreaks_utf32(reinterpret_cast<const utf32_t*>(params.text.data()), m_textLength, "en", g_breaks.data());

        breakLines(params);
        placeLines();
        alignLines();
    }

    void HarfBuzzLayout::setAlignment(TextAlign alignment)
    {
        m_alignment = alignment;
        alignLines();
    }

    void HarfBuzzLayout::setMaxWidth(float availableWidth)
    {
        m_maxWidth = availableWidth;
        alignLines();
    }

    NativeParagraphMetrics HarfBuzzLayout::getMetrics() const
    {
        return m_metrics;
    }

    void HarfBuzzLayout::appendLineMetrics(std::vector<NativeLineMetrics>& result) const
    {
        for (const Line& line : m_lines)
        {
            result.push_back({
                .textStart = line.textStart,
                .textLength = line.textLength,
                .width = line.width,
                .height = line.height,
                .baseline = line.baseline,
                .isTrimmed = false,
            });
        }
    }

    float HarfBuzzLayout::getCharX(std::size_t localPos, bool trailing) const
    {
        if (m_lines.empty())
            return 0.0f;
        return charEdge(localPos, trailing);
    }

    std::size_t HarfBuzzLayout::hitTestPoint(FloatPoint localPt, bool* isTrailing) const
    {
        *isTrailing = false;
        if (m_lines.empty())
            return 0;

        const Line& line = m_lines[lineAt(localPt.y)];
        const std::size_t lineEnd = line.textStart + line.textLength;
        const float x = localPt.x - line.offset;

        if (x < m_chars[line.textStart].left)
            return line.textStart;

        for (std::size_t i = line.textStart; i != lineEnd; ++i)
        {
            const CharCell& cell = m_chars[i];
            float width = cell.advance;
            if (cell.stretchable)
                width += line.stretch;
            if (x >= cell.left && x < cell.left + width)
            {
                *isTrailing = x - cell.left > width / 2.0f;
                return i;
            }
        }

        // Past the end of the line: the last character, on its trailing side.
        *isTrailing = true;
        return lineEnd - 1;
    }

    FloatRect HarfBuzzLayout::getCharRect(std::size_t localPos, bool trailing) const
    {
        if (m_lines.empty())
            return {};
        const Line& line = m_lines[lineOfPosition(localPos)];
        const float x = charEdge(localPos, trailing);
        return { x, line.top, x, line.top + line.height };
    }

    // Read off the coverage the glyphs are drawn from, at the pen positions draw places them at. A
    // script's shift is not applied: the spans that state it reach a layout only when it draws.
    void HarfBuzzLayout::appendInkAcross(TextRange localRange, float bandTop, float bandBottom,
        std::vector<Graphics::InkExtent>& result) const
    {
        const std::size_t end = localRange.end();
        for (const Line& line : m_lines)
        {
            if (line.textStart >= end || line.textStart + line.textLength <= localRange.start)
                continue;

            const float baselineY = line.top + line.baseline;
            const std::size_t runEnd = line.runStart + line.runCount;
            for (std::size_t runIndex = line.runStart; runIndex != runEnd; ++runIndex)
            {
                const Run& run = m_runs[runIndex];
                if (run.object != k_maxSize)
                    continue;

                const std::size_t glyphEnd = run.glyphStart + run.glyphCount;
                for (std::size_t glyphAt = run.glyphStart; glyphAt != glyphEnd; ++glyphAt)
                {
                    const Glyph& glyph = m_glyphs[glyphAt];
                    if (glyph.cluster < localRange.start || glyph.cluster >= end
                        || m_chars[glyph.cluster].tab)
                    {
                        continue;
                    }

                    const float originX = line.offset + glyph.x;
                    const float originY = baselineY - glyph.offsetY;
                    const std::optional<Graphics::InkExtent> ink = run.face->coverage(glyph.index,
                        run.emSize).inkAcross(bandTop - originY, bandBottom - originY);
                    if (ink.has_value())
                        result.push_back({ originX + ink->left, originX + ink->right });
                }
            }
        }
    }

    void HarfBuzzLayout::draw(
        ControlPaintContext& controlContext,
        const FloatRect& bounds,
        std::span<const ColorSpan> colors,
        std::span<const ScriptSpan> scripts,
        std::span<const TextRange> hitRanges,
        const TextRange& selectionRange,
        std::span<const float> localBaselinesToFade,
        float localCollapseBaseline,
        float collapseXOffset)
    {
        if (m_lines.empty())
            return;

        Graphics::Canvas& canvas = controlContext.canvas();
        Graphics::IBackend* backend = canvas.backend();
        if (!backend)
            return;

        // A band under a range of characters, one rectangle per line the range touches.
        auto fillRange = [&](const TextRange& range, Color color){
            if (range.length == 0 || range.start == k_maxSize)
                return;
            const std::size_t rangeEnd = std::min(range.end(), m_textLength);
            for (const Line& line : m_lines)
            {
                const std::size_t first = std::max(range.start, line.textStart);
                const std::size_t last = std::min(rangeEnd, line.textStart + line.textLength);
                if (first >= last)
                    continue;
                const float left = charEdge(first, false);
                const float right = charEdge(last - 1, true);
                canvas.fillRectangle({
                    bounds.left + left,
                    bounds.top + line.top,
                    bounds.left + right,
                    bounds.top + line.top + line.height,
                }, color);
            }
        };
        for (const TextRange& hit : hitRanges)
            fillRange(hit, controlContext.hit);
        fillRange(selectionRange, controlContext.selectionRgb());

        const bool snap = backend->snapTextOrigins();
        const float fadeWidth = controlContext.scaleF(30.0f);
        const Graphics::Matrix3x2& canvasTransform = canvas.transform();
        const bool transformed = !(canvasTransform == Graphics::Matrix3x2::identity());
        const Color defaultColor = controlContext.textRgb(InkGrade::Strongest);
        const ColorDef defaultColorDef{ std::in_place_type<Color>, defaultColor };
        const bool collapses = localCollapseBaseline > -9000.0f;

        // Where the lines lifted onto the collapse baseline continue from: the end of the collapse
        // line, then the end of each line appended after it.
        float collapseX = 0.0f;
        std::size_t colorCursor = 0;
        std::size_t scriptCursor = 0;

        for (const Line& line : m_lines)
        {
            const float lineBaseline = line.top + line.baseline;
            float drawBaseline = lineBaseline;
            float lineX = line.offset;

            if (collapses)
            {
                if (std::abs(lineBaseline - localCollapseBaseline) < 0.5f)
                {
                    // The collapse line is moved by the offset to make room for what is appended
                    // to it, and no further left than the box, where the clip would take its start.
                    lineX += std::max(collapseXOffset, -line.offset);
                    collapseX = lineX + line.fullWidth;
                }
                else if (lineBaseline > localCollapseBaseline + 0.5f)
                {
                    // A line below the collapse is drawn on the collapse baseline, after whatever
                    // stands there already. Its trailing whitespace is the gap to the next.
                    drawBaseline = localCollapseBaseline;
                    lineX = collapseX;
                    collapseX += line.fullWidth;
                }
            }

            bool fades = false;
            for (float baseline : localBaselinesToFade)
            {
                if (std::abs(drawBaseline - baseline) < 0.5f)
                {
                    fades = true;
                    break;
                }
            }

            // Snapping puts the baseline on a whole device pixel, which is what keeps still text
            // crisp. Under a scale it quantizes, and the backend says whether that trade is made.
            float baselineY = bounds.top + drawBaseline;
            if (snap)
                baselineY = std::round(baselineY);
            const float originX = bounds.left + lineX;

            for (std::size_t runIndex = line.runStart; runIndex != line.runStart + line.runCount; ++runIndex)
            {
                const Run& run = m_runs[runIndex];
                const float runX = originX + m_chars[run.textStart].left;
                if (runX >= bounds.right)
                    continue;

                if (run.object != k_maxSize)
                {
                    const BakedInlineObject& object = m_objects[run.object];
                    if (!object.paintLambda)
                        continue;
                    // A run has no way to reach the control it is inside, so an in text icon sees
                    // no state factors and is not faded: there is nothing here to fade it with.
                    const float top = baselineY - object.baseline * m_scaleFactor;
                    FloatRect rect = {
                        runX,
                        top,
                        runX + object.width * m_scaleFactor,
                        top + object.height * m_scaleFactor,
                    };
                    PaintIconEvent event{ controlContext, rect, object.tag, 0.0f };
                    object.paintLambda(event);
                    continue;
                }

                // Glyphs are gathered into one mask for as long as their colour and script shift
                // hold, and composited when either changes. A script stands off the baseline the
                // line placed it on, and only where it is drawn: which line this is and whether it
                // fades are questions about the line.
                bool open = false;
                Color pieceColor = defaultColor;
                float pieceShift = 0.0f;
                auto flush = [&](){
                    if (!open)
                        return;
                    Graphics::Brush brush = Graphics::SolidColor{ pieceColor };
                    if (fades)
                    {
                        brush = Graphics::LinearGradient{
                            .startPoint = { bounds.right - fadeWidth, 0.0f },
                            .endPoint = { bounds.right, 0.0f },
                            .stops = { { 0.0f, pieceColor }, { 1.0f, pieceColor.withOpacity(0.0f) } },
                        };
                    }
                    m_compositor.composite(*backend, brush);
                    open = false;
                };

                for (std::size_t glyphAt = run.glyphStart; glyphAt != run.glyphStart + run.glyphCount; ++glyphAt)
                {
                    const Glyph& glyph = m_glyphs[glyphAt];
                    // A tab is its advance and no ink: the glyph the face shaped for it is its
                    // .notdef.
                    if (m_chars[glyph.cluster].tab)
                        continue;
                    const Color color = colorOf(walkSpanValue(colors, glyph.cluster, colorCursor, defaultColorDef), controlContext);
                    const float shift = walkSpanValue(scripts, glyph.cluster, scriptCursor, 0.0f) * m_scaleFactor;
                    if (open && (color != pieceColor || shift != pieceShift))
                        flush();
                    if (!open)
                    {
                        m_compositor.begin();
                        pieceColor = color;
                        pieceShift = shift;
                        open = true;
                    }

                    FloatPoint origin = { originX + glyph.x, baselineY - glyph.offsetY - shift };
                    if (transformed)
                        origin = canvasTransform.transform(origin);
                    m_compositor.place(run.face->coverage(glyph.index, run.emSize), origin);
                }
                flush();
            }
        }
    }

    void HarfBuzzLayout::itemize(const BuildParams& params)
    {
        const TextStyle& body = k_textStyles[static_cast<std::size_t>(TextStyleId::Body)];

        // An empty paragraph is one placeholder character in the style in force where it stands.
        if (params.isEmpty)
        {
            const std::wstring family = lastSpanValue(params.families, std::wstring{ body.family });
            FaceChain& chain = fontSet().chain(family,
                lastSpanValue(params.weights, body.weight),
                lastSpanValue(params.styles, body.style));
            addTextItem(params, 0, m_textLength, chain.primary(),
                lastSpanValue(params.sizes, body.size) * m_scaleFactor);
            return;
        }

        // Every position a style changes at, and both sides of every inline object, cut the text
        // into segments; each segment is one item.
        std::vector<std::size_t> edges;
        edges.push_back(0);
        edges.push_back(m_textLength);
        collectEdges(params.families, edges);
        collectEdges(params.sizes, edges);
        collectEdges(params.weights, edges);
        collectEdges(params.styles, edges);
        for (const std::pair<std::size_t, BakedInlineObject>& object : params.inlineObjects)
        {
            edges.push_back(object.first);
            edges.push_back(object.first + 1);
        }
        std::sort(edges.begin(), edges.end());
        edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

        std::size_t objectCursor = 0;
        for (std::size_t edge = 0; edge + 1 < edges.size(); ++edge)
        {
            const std::size_t start = edges[edge];
            const std::size_t end = std::min(edges[edge + 1], m_textLength);
            if (start >= end)
                continue;

            while (objectCursor != params.inlineObjects.size() && params.inlineObjects[objectCursor].first < start)
                ++objectCursor;
            if (objectCursor != params.inlineObjects.size() && params.inlineObjects[objectCursor].first == start)
            {
                // The object stands in the one character its position names, and its size is the
                // whole of what the character measures.
                const BakedInlineObject& object = params.inlineObjects[objectCursor].second;
                Run item = {};
                item.textStart = start;
                item.textLength = end - start;
                item.glyphStart = m_glyphs.size();
                item.object = m_objects.size();
                m_objects.push_back(object);
                item.metrics = {
                    object.baseline * m_scaleFactor,
                    (object.height - object.baseline) * m_scaleFactor,
                    0.0f,
                };
                CharCell& cell = m_chars[start];
                cell.advance = object.width * m_scaleFactor;
                cell.clusterStart = true;
                cell.whitespace = false;
                cell.stretchable = false;
                m_items.push_back(item);
                continue;
            }

            const std::wstring family = findSpanValue(params.families, start, std::wstring{ body.family });
            FaceChain& chain = fontSet().chain(family,
                findSpanValue(params.weights, start, body.weight),
                findSpanValue(params.styles, start, body.style));
            const float emSize = findSpanValue(params.sizes, start, body.size) * m_scaleFactor;

            // The segment is cut again wherever the face changes. A character the matched face has
            // no glyph for is set in the first face behind it that has one; whitespace, marks and
            // the invisible characters take the face of the text they stand with.
            FontFace* runFace = nullptr;
            std::size_t runStart = start;
            for (std::size_t i = start; i != end; ++i)
            {
                FontFace* face = runFace;
                if (!face || !followsText(params.text[i]))
                {
                    FontFace* found = chain.faceFor(static_cast<char32_t>(params.text[i]));
                    face = found ? found : &chain.primary();
                }
                if (runFace && face != runFace)
                {
                    addTextItem(params, runStart, i, *runFace, emSize);
                    runStart = i;
                }
                runFace = face;
            }
            addTextItem(params, runStart, end, *runFace, emSize);
        }
    }

    void HarfBuzzLayout::addTextItem(const BuildParams& params, std::size_t textStart, std::size_t textEnd,
        FontFace& face, float emSize)
    {
        Run item = {};
        item.textStart = textStart;
        item.textLength = textEnd - textStart;
        item.face = &face;
        item.emSize = emSize;
        item.metrics = face.metrics(emSize);
        item.object = k_maxSize;
        shapeItem(params, item);
        m_items.push_back(item);
    }

    void HarfBuzzLayout::shapeItem(const BuildParams& params, Run& item)
    {
        hb_buffer_t* buffer = shapingBuffer();
        hb_buffer_clear_contents(buffer);
        // The whole paragraph is the context and the item is what is shaped, so a cluster is a
        // position in the paragraph and a glyph at an item edge sees the character beyond it.
        hb_buffer_add_utf32(buffer,
            reinterpret_cast<const std::uint32_t*>(params.text.data()),
            static_cast<int>(m_textLength),
            static_cast<unsigned int>(item.textStart),
            static_cast<int>(item.textLength));
        hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
        hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
        // Script and language from the text, with the direction stated above left as it is.
        hb_buffer_guess_segment_properties(buffer);

        hb_shape(item.face->hbFont(), buffer, nullptr, 0);

        unsigned int count = 0;
        const hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &count);
        const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &count);
        const float scale = item.emSize / item.face->unitsPerEm();

        item.glyphStart = m_glyphs.size();
        item.glyphCount = count;
        for (unsigned int i = 0; i != count; ++i)
        {
            m_glyphs.push_back({
                infos[i].codepoint,
                infos[i].cluster,
                static_cast<float>(positions[i].x_advance) * scale,
                static_cast<float>(positions[i].x_offset) * scale,
                static_cast<float>(positions[i].y_offset) * scale,
                0.0f,
            });
        }

        // A cluster's advance is shared evenly over the characters it covers. Clusters are
        // monotone, so the characters of one run from its first glyph's cluster to the next
        // cluster's start - or the item's end.
        const std::size_t itemEnd = item.textStart + item.textLength;
        std::size_t glyphAt = item.glyphStart;
        while (glyphAt != m_glyphs.size())
        {
            const std::size_t cluster = m_glyphs[glyphAt].cluster;
            float advance = 0.0f;
            while (glyphAt != m_glyphs.size() && m_glyphs[glyphAt].cluster == cluster)
            {
                advance += m_glyphs[glyphAt].advance;
                ++glyphAt;
            }
            const std::size_t clusterEnd = glyphAt != m_glyphs.size() ? m_glyphs[glyphAt].cluster : itemEnd;
            const float share = advance / static_cast<float>(clusterEnd - cluster);
            m_chars[cluster].clusterStart = true;
            for (std::size_t i = cluster; i != clusterEnd; ++i)
                m_chars[i].advance = share;
        }
    }

    void HarfBuzzLayout::breakLines(const BuildParams& params)
    {
        std::size_t lineStart = 0;
        while (lineStart < m_textLength)
        {
            std::size_t lineEnd = m_textLength;
            if (!params.wrap)
            {
                // One line however narrow the box, up to a break the text itself demands.
                for (std::size_t i = lineStart; i != m_textLength; ++i)
                {
                    if (g_breaks[i] == LINEBREAK_MUSTBREAK)
                    {
                        lineEnd = i + 1;
                        break;
                    }
                }
            }
            else
            {
                // Greedy, whole words: the line takes characters until one steps over the width,
                // then ends at the last opportunity seen. Whitespace is held back and counted only
                // once a character follows it, so a line may end in spaces that overhang the box.
                // With no opportunity yet the word is wider than the box, and it stays on this line
                // and overflows: the break comes at the first opportunity after it.
                float width = 0.0f;
                float pendingWhitespace = 0.0f;
                std::size_t lastOpportunity = k_maxSize;
                for (std::size_t i = lineStart; i != m_textLength; ++i)
                {
                    const CharCell& cell = m_chars[i];
                    const float advance = cell.tab ? tabAdvance(width + pendingWhitespace) : cell.advance;
                    if (cell.whitespace)
                    {
                        pendingWhitespace += advance;
                    }
                    else
                    {
                        width += pendingWhitespace + advance;
                        pendingWhitespace = 0.0f;
                    }

                    if (width > m_availableWidth + k_fitEpsilon && lastOpportunity != k_maxSize)
                    {
                        lineEnd = lastOpportunity + 1;
                        break;
                    }
                    if (g_breaks[i] == LINEBREAK_MUSTBREAK)
                    {
                        lineEnd = i + 1;
                        break;
                    }
                    // A break inside a cluster would cut a ligature, so an opportunity counts
                    // only where the next character starts a glyph of its own.
                    const bool nextStartsCluster = i + 1 >= m_textLength || m_chars[i + 1].clusterStart;
                    if (g_breaks[i] == LINEBREAK_ALLOWBREAK && nextStartsCluster)
                        lastOpportunity = i;
                }
            }

            addLine(lineStart, lineEnd);
            lineStart = lineEnd;
        }
    }

    void HarfBuzzLayout::addLine(std::size_t textStart, std::size_t textEnd)
    {
        Line line = {};
        line.textStart = textStart;
        line.textLength = textEnd - textStart;
        line.runStart = m_runs.size();
        line.top = m_lines.empty() ? 0.0f : m_lines.back().top + m_lines.back().height;

        float ascent = 0.0f;
        float descent = 0.0f;
        float lineGap = 0.0f;
        for (const Run& item : m_items)
        {
            const std::size_t itemEnd = item.textStart + item.textLength;
            const std::size_t runStart = std::max(item.textStart, textStart);
            const std::size_t runEnd = std::min(itemEnd, textEnd);
            if (runStart >= runEnd)
                continue;

            Run run = item;
            run.textStart = runStart;
            run.textLength = runEnd - runStart;

            // The item's glyphs standing in this line, found by cluster: clusters are monotone and
            // a line never cuts one.
            std::size_t first = item.glyphStart;
            const std::size_t itemGlyphEnd = item.glyphStart + item.glyphCount;
            while (first != itemGlyphEnd && m_glyphs[first].cluster < runStart)
                ++first;
            std::size_t last = first;
            while (last != itemGlyphEnd && m_glyphs[last].cluster < runEnd)
                ++last;
            run.glyphStart = first;
            run.glyphCount = last - first;

            ascent = std::max(ascent, run.metrics.ascent);
            descent = std::max(descent, run.metrics.descent);
            lineGap = std::max(lineGap, run.metrics.lineGap);
            m_runs.push_back(run);
        }
        line.runCount = m_runs.size() - line.runStart;
        line.height = ascent + descent + lineGap;
        line.baseline = ascent;

        std::size_t trailing = 0;
        while (trailing != line.textLength && m_chars[textEnd - 1 - trailing].whitespace)
            ++trailing;
        line.trailingWhitespace = trailing;
        for (std::size_t i = textEnd - trailing; i != textEnd; ++i)
            m_chars[i].stretchable = false;

        for (std::size_t i = textStart; i != textEnd - trailing; ++i)
        {
            if (m_chars[i].stretchable)
                ++line.stretchableSpaces;
        }

        m_lines.push_back(line);
    }

    void HarfBuzzLayout::placeLines()
    {
        float widest = 0.0f;
        float height = 0.0f;
        for (std::size_t lineIndex = 0; lineIndex != m_lines.size(); ++lineIndex)
        {
            Line& line = m_lines[lineIndex];
            const std::size_t lineEnd = line.textStart + line.textLength;
            const std::size_t inkEnd = lineEnd - line.trailingWhitespace;

            float pen = 0.0f;
            for (std::size_t i = line.textStart; i != lineEnd; ++i)
            {
                CharCell& cell = m_chars[i];
                cell.line = lineIndex;
                if (cell.tab)
                    cell.advance = tabAdvance(pen);
                cell.left = pen;
                pen += cell.advance;
                if (cell.stretchable)
                    pen += line.stretch;
            }
            line.fullWidth = pen;
            line.width = inkEnd != lineEnd ? m_chars[inkEnd].left : pen;

            // A glyph stands at its cluster's cell, after the glyphs of that cluster before it.
            for (std::size_t runIndex = line.runStart; runIndex != line.runStart + line.runCount; ++runIndex)
            {
                const Run& run = m_runs[runIndex];
                std::size_t cluster = k_maxSize;
                float withinCluster = 0.0f;
                for (std::size_t glyphAt = run.glyphStart; glyphAt != run.glyphStart + run.glyphCount; ++glyphAt)
                {
                    Glyph& glyph = m_glyphs[glyphAt];
                    if (glyph.cluster != cluster)
                    {
                        cluster = glyph.cluster;
                        withinCluster = 0.0f;
                    }
                    glyph.x = m_chars[cluster].left + withinCluster + glyph.offsetX;
                    withinCluster += glyph.advance;
                }
            }

            widest = std::max(widest, line.fullWidth);
            height += line.height;
        }
        m_metrics = { widest, height };
    }

    void HarfBuzzLayout::alignLines()
    {
        // A line wider than the box has no room left to be moved in, and moving it anyway carries
        // its start off the leading edge where the clip takes it. Measured without the trailing
        // whitespace, which is what a line is allowed to overhang with.
        TextAlign alignment = m_alignment;
        if (alignment != TextAlign::Left)
        {
            float widest = 0.0f;
            for (const Line& line : m_lines)
                widest = std::max(widest, line.width);
            if (widest > m_maxWidth + 0.1f)
                alignment = TextAlign::Left;
        }

        for (std::size_t lineIndex = 0; lineIndex != m_lines.size(); ++lineIndex)
        {
            Line& line = m_lines[lineIndex];
            line.offset = 0.0f;
            line.stretch = 0.0f;
            switch (alignment)
            {
                case TextAlign::Left:
                    break;
                case TextAlign::Center:
                    line.offset = (m_maxWidth - line.width) / 2.0f;
                    break;
                case TextAlign::Right:
                    line.offset = m_maxWidth - line.width;
                    break;
                case TextAlign::Justified:
                {
                    // Every line but the last is stretched to the box over its spaces.
                    const bool last = lineIndex + 1 == m_lines.size();
                    if (!last && line.stretchableSpaces != 0 && m_maxWidth > line.width)
                        line.stretch = (m_maxWidth - line.width) / static_cast<float>(line.stretchableSpaces);
                    break;
                }
            }
        }

        // Justified moved the cells, and the placed extent is read off the lines either way.
        placeLines();
    }

    float HarfBuzzLayout::tabAdvance(float pen) const
    {
        const float stop = (std::floor(pen / m_tabInterval) + 1.0f) * m_tabInterval;
        return stop - pen;
    }

    float HarfBuzzLayout::leadingSpaceAdvance(const BuildParams& params) const
    {
        const TextStyle& body = k_textStyles[static_cast<std::size_t>(TextStyleId::Body)];
        const std::wstring family = leadingSpanValue(params.families, params.isEmpty,
            std::wstring{ body.family });
        FaceChain& chain = fontSet().chain(family,
            leadingSpanValue(params.weights, params.isEmpty, body.weight),
            leadingSpanValue(params.styles, params.isEmpty, body.style));
        const float emSize = leadingSpanValue(params.sizes, params.isEmpty, body.size) * m_scaleFactor;
        return chain.primary().advance(U' ', emSize);
    }

    std::size_t HarfBuzzLayout::lineAt(float y) const
    {
        for (std::size_t lineIndex = 0; lineIndex != m_lines.size(); ++lineIndex)
        {
            if (y < m_lines[lineIndex].top + m_lines[lineIndex].height)
                return lineIndex;
        }
        return m_lines.size() - 1;
    }

    std::size_t HarfBuzzLayout::lineOfPosition(std::size_t localPos) const
    {
        if (localPos >= m_textLength)
            return m_lines.size() - 1;
        return m_chars[localPos].line;
    }

    float HarfBuzzLayout::charEdge(std::size_t localPos, bool trailing) const
    {
        // The position after the last character is the trailing edge of that character.
        if (localPos >= m_textLength)
        {
            localPos = m_textLength - 1;
            trailing = true;
        }
        const CharCell& cell = m_chars[localPos];
        const Line& line = m_lines[cell.line];
        float x = line.offset + cell.left;
        if (trailing)
        {
            x += cell.advance;
            if (cell.stretchable)
                x += line.stretch;
        }
        return x;
    }

    HarfBuzzLayout::SpareBuffers HarfBuzzLayout::s_spares{};

    HarfBuzzLayout::HarfBuzzLayout()
    {
        if (s_spares.buffers.empty())
            return;

        Buffers& spare = s_spares.buffers.back();
        m_glyphs = std::move(spare.glyphs);
        m_items = std::move(spare.items);
        m_runs = std::move(spare.runs);
        m_lines = std::move(spare.lines);
        m_chars = std::move(spare.chars);
        m_objects = std::move(spare.objects);
        m_compositor = std::move(spare.compositor);
        s_spares.buffers.pop_back();
    }

    bool HarfBuzzLayout::s_sparesOpen{ true };

    // Emptied before they are left, so the next layout finds capacity and nothing of this one.
    HarfBuzzLayout::~HarfBuzzLayout()
    {
        if (!s_sparesOpen || s_spares.buffers.size() == k_maxSpareBuffers)
            return;
        if (m_chars.capacity() > k_maxSpareCharacters || m_glyphs.capacity() > k_maxSpareCharacters)
            return;

        m_glyphs.clear();
        m_items.clear();
        m_runs.clear();
        m_lines.clear();
        m_chars.clear();
        m_objects.clear();
        s_spares.buffers.push_back({
            .glyphs = std::move(m_glyphs),
            .items = std::move(m_items),
            .runs = std::move(m_runs),
            .lines = std::move(m_lines),
            .chars = std::move(m_chars),
            .objects = std::move(m_objects),
            .compositor = std::move(m_compositor),
        });
    }

    HarfBuzzLayout::SpareBuffers::~SpareBuffers()
    {
        s_sparesOpen = false;
    }


    //-------------------------------------------------------------------------


    // FreeTypeMonoFont

    FreeTypeMonoFont::FreeTypeMonoFont(FontFace& face, float emSize)
        :
        m_face{ &face },
        m_emSize{ emSize },
        m_cellWidth{ face.advance(U' ', emSize) },
        m_metrics{ face.metrics(emSize) }
    {
    }

    MonoGlyph FreeTypeMonoFont::glyphOf(char32_t codePoint) const
    {
        const std::uint32_t glyph = m_face->glyphIndex(codePoint);
        // A glyph id past sixteen bits is outside what a MonoGlyph carries, and no face states one.
        if (glyph == 0 || glyph > std::numeric_limits<MonoGlyph>::max())
            return k_noMonoGlyph;
        // Compared as the same scaled float the cell is, from the same design advance, so two
        // equal advances compare equal.
        if (m_face->glyphAdvance(glyph, m_emSize) != m_cellWidth)
            return k_noMonoGlyph;
        return static_cast<MonoGlyph>(glyph);
    }

    void FreeTypeMonoFont::drawRun(ControlPaintContext& controlContext, FloatPoint baselineOrigin,
        std::span<const MonoGlyph> glyphs, const Graphics::Brush& brush)
    {
        Graphics::Canvas& canvas = controlContext.canvas();
        Graphics::IBackend* backend = canvas.backend();
        if (!backend || glyphs.empty())
            return;

        const Graphics::Matrix3x2& canvasTransform = canvas.transform();
        const bool transformed = !(canvasTransform == Graphics::Matrix3x2::identity());

        m_compositor.begin();
        float penX = baselineOrigin.x;
        for (const MonoGlyph glyph : glyphs)
        {
            FloatPoint origin = { penX, baselineOrigin.y };
            if (transformed)
                origin = canvasTransform.transform(origin);
            m_compositor.place(m_face->coverage(glyph, m_emSize), origin);
            penX += m_cellWidth;
        }
        m_compositor.composite(*backend, brush);
    }

    std::optional<Graphics::InkExtent> FreeTypeMonoFont::inkAcross(MonoGlyph glyph, float bandTop,
        float bandBottom)
    {
        return m_face->coverage(glyph, m_emSize).inkAcross(bandTop, bandBottom);
    }
}
