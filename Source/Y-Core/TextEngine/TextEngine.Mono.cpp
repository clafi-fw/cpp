module ClaFi.Core.TextEngine.Mono;

import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        struct CodeRange
        {
            char32_t first;
            char32_t last;
        };

        // What a cmap lookup alone cannot lay out, so a paragraph carrying one of these is shaped
        // natively whatever the face has for it: the controls and the format characters, which
        // draw nothing or break lines; combining marks, which stand on the character before them;
        // the right to left blocks, whose runs are reversed and, for Arabic, joined; and the
        // scripts that reorder or stack - Indic, Thai, Lao, Tibetan, Myanmar, Khmer. Refusing one
        // costs the native path and nothing else, so the list errs toward refusing.
        constexpr std::array<CodeRange, 15> k_refusedRanges = { {
            { 0x0000, 0x001F },
            { 0x007F, 0x009F },
            { 0x0300, 0x036F },
            { 0x0590, 0x08FF },
            { 0x0900, 0x109F },
            { 0x1780, 0x17FF },
            { 0x1AB0, 0x1AFF },
            { 0x1DC0, 0x1DFF },
            { 0x200B, 0x200F },
            { 0x2028, 0x202E },
            { 0x2060, 0x206F },
            { 0x20D0, 0x20FF },
            { 0xD800, 0xDFFF },
            { 0xFB1D, 0xFDFF },
            { 0xFE00, 0xFEFF },
        } };

        // A code point past the plane the table covers. A supplementary character is two code
        // units on Windows, which the surrogate range above refuses one at a time.
        constexpr char32_t k_firstRefusedPlane = 0x10000;

        bool isRefused(char32_t code)
        {
            if (code >= k_firstRefusedPlane)
                return true;
            for (const CodeRange& range : k_refusedRanges)
            {
                if (code >= range.first && code <= range.last)
                    return true;
            }
            return false;
        }

        // What a line may end with and not count toward its ink. Every other whitespace the
        // native layouts know is refused above or wider than a cell.
        bool isWhitespace(wchar_t character)
        {
            return character == L' ' || character == L'\t';
        }

        // The stop a tab reaches from a column: the next multiple of k_tabStopSpaces, which is a
        // whole one however the column stands, so a tab is never a cell of nothing.
        std::size_t nextTabStop(std::size_t column)
        {
            constexpr std::size_t stop = static_cast<std::size_t>(k_tabStopSpaces);
            return (column / stop + 1) * stop;
        }

        // The column after the character.
        std::size_t columnAfter(std::size_t column, wchar_t character)
        {
            return character == L'\t' ? nextTabStop(column) : column + 1;
        }

        Color colorOf(const ColorDef& value, const ControlPaintContext& context)
        {
            return value.index() == 0
                ? context.inkRgb(std::get<0>(value))
                : std::get<1>(value);
        }

        // The colour standing at pos, the cursor advanced past the spans ending before it, so a
        // walk in text order pays one pass over the spans.
        Color colorAt(std::span<const ColorSpan> colors, std::size_t pos, std::size_t& cursor,
            Color fallback, const ControlPaintContext& context)
        {
            while (cursor != colors.size() && colors[cursor].range.end() <= pos)
                ++cursor;
            if (cursor != colors.size() && colors[cursor].range.start <= pos)
                return colorOf(colors[cursor].value, context);
            return fallback;
        }

        // The glyphs of the run being drawn. One buffer for the application, refilled per run:
        // a mono run paints no icon, so nothing draws text inside one.
        MonoGlyphs g_runGlyphs{};

        struct MonoFontEntry
        {
            MonoFontRequest request;
            // Null where the platform answered no monospace face, kept so the request is not
            // resolved again.
            std::unique_ptr<MonoFont> font;
        };

        // A handful of entries - the fonts and sizes a form draws mono text in - searched rather
        // than hashed, because the search compares views and a key would copy the family name
        // for every paragraph that asks.
        std::vector<MonoFontEntry> g_monoFonts{};
    }

    //-------------------------------------------------------------------------


    MonoFont* monoFont(std::wstring_view family, float size, FontWeight weight, FontStyle style)
    {
        for (const MonoFontEntry& entry : g_monoFonts)
        {
            if (entry.request.family == family && entry.request.size == size
                && entry.request.weight == weight && entry.request.style == style)
            {
                return entry.font.get();
            }
        }

        MonoFontEntry entry = { { std::wstring{ family }, size, weight, style }, nullptr };
        std::unique_ptr<INativeMonoFont> native = createNativeMonoFont(entry.request);
        if (native)
            entry.font = std::make_unique<MonoFont>(std::move(native));
        g_monoFonts.push_back(std::move(entry));
        return g_monoFonts.back().font.get();
    }


    //-------------------------------------------------------------------------


    // MonoFont

    MonoFont::MonoFont(std::unique_ptr<INativeMonoFont> native)
        :
        m_native{ std::move(native) },
        m_cellWidth{ m_native->cellWidth() },
        m_lineHeight{ m_native->lineHeight() },
        m_baseline{ m_native->baseline() }
    {
    }

    MonoGlyph MonoFont::glyphOf(wchar_t character)
    {
        const char32_t code = static_cast<char32_t>(character);
        if (code < m_ascii.size())
        {
            if (!m_asciiAsked.test(code))
            {
                m_ascii[code] = resolve(code);
                m_asciiAsked.set(code);
            }
            return m_ascii[code];
        }

        OtherGlyphs::const_iterator known = m_others.find(code);
        if (known != m_others.end())
            return known->second;

        const MonoGlyph glyph = resolve(code);
        m_others.emplace(code, glyph);
        return glyph;
    }

    bool MonoFont::covers(std::wstring_view text)
    {
        for (const wchar_t character : text)
        {
            if (character != L'\t' && glyphOf(character) == k_noMonoGlyph)
                return false;
        }
        return true;
    }

    MonoExtent MonoFont::extent(std::wstring_view text)
    {
        std::size_t column = 0;
        std::size_t inkColumn = 0;
        for (const wchar_t character : text)
        {
            column = columnAfter(column, character);
            if (!isWhitespace(character))
                inkColumn = column;
        }
        return {
            static_cast<float>(column) * m_cellWidth,
            static_cast<float>(inkColumn) * m_cellWidth,
        };
    }

    float MonoFont::cellLeft(std::wstring_view text, std::size_t pos)
    {
        const std::size_t end = std::min(pos, text.size());
        std::size_t column = 0;
        for (std::size_t i = 0; i != end; ++i)
            column = columnAfter(column, text[i]);
        return static_cast<float>(column) * m_cellWidth;
    }

    std::size_t MonoFont::hitTest(std::wstring_view text, float x, bool* isTrailing)
    {
        *isTrailing = false;
        if (text.empty() || x < 0.0f)
            return 0;

        std::size_t column = 0;
        for (std::size_t i = 0; i != text.size(); ++i)
        {
            const std::size_t next = columnAfter(column, text[i]);
            const float left = static_cast<float>(column) * m_cellWidth;
            const float right = static_cast<float>(next) * m_cellWidth;
            if (x < right)
            {
                *isTrailing = x - left > (right - left) / 2.0f;
                return i;
            }
            column = next;
        }

        // Past the end of the line: the last character, on its trailing side.
        *isTrailing = true;
        return text.size() - 1;
    }

    void MonoFont::appendInkAcross(std::wstring_view text, TextRange range, float bandTop,
        float bandBottom, std::vector<Graphics::InkExtent>& result)
    {
        const std::size_t end = std::min(range.end(), text.size());
        std::size_t column = 0;
        for (std::size_t i = 0; i != end; ++i)
        {
            const wchar_t character = text[i];
            const std::size_t next = columnAfter(column, character);
            if (i >= range.start && character != L'\t')
            {
                const std::optional<Graphics::InkExtent> ink = m_native->inkAcross(
                    glyphOf(character), bandTop - m_baseline, bandBottom - m_baseline);
                if (ink.has_value())
                {
                    const float left = static_cast<float>(column) * m_cellWidth;
                    result.push_back({ left + ink->left, left + ink->right });
                }
            }
            column = next;
        }
    }

    void MonoFont::draw(ControlPaintContext& context, std::wstring_view text, FloatPoint origin,
        const MonoLineDraw& params)
    {
        Graphics::Canvas& canvas = context.canvas();
        Graphics::IBackend* backend = canvas.backend();
        if (!backend)
            return;

        // A band under a range of characters, from the leading edge of its first to the trailing
        // edge of its last.
        auto fillRange = [&](const TextRange& range, Color color){
            if (range.length == 0 || range.start == k_maxSize)
                return;
            const std::size_t end = std::min(range.end(), text.size());
            if (range.start >= end)
                return;
            canvas.fillRectangle({
                origin.x + cellLeft(text, range.start),
                origin.y,
                origin.x + cellLeft(text, end),
                origin.y + m_lineHeight,
            }, color);
        };
        for (const TextRange& hit : params.hits)
            fillRange(hit, context.hit);
        fillRange(params.selection, context.selectionRgb());

        // Snapping puts the baseline on a whole device pixel, which is what keeps still text
        // crisp. Under a scale it quantizes, and the backend says whether that trade is made.
        float baselineY = origin.y + m_baseline;
        if (backend->snapTextOrigins())
            baselineY = std::round(baselineY);

        const Color defaultColor = context.textRgb(InkGrade::Strongest);
        const float fadeWidth = context.scaleF(30.0f);
        std::size_t colorCursor = 0;

        // Glyphs are gathered into one run for as long as their colour holds and no tab breaks
        // the cells, and drawn when either changes. A run standing past the box or ending before
        // what is on screen is dropped whole.
        MonoGlyphs& glyphs = g_runGlyphs;
        glyphs.clear();
        Color runColor = defaultColor;
        float runLeft = 0.0f;
        auto flush = [&](float runRight){
            if (glyphs.empty())
                return;
            const float left = origin.x + runLeft;
            if (left < params.bounds.right && origin.x + runRight > params.clip.left)
            {
                Graphics::Brush brush = Graphics::SolidColor{ runColor };
                if (params.fades)
                {
                    brush = Graphics::LinearGradient{
                        .startPoint = { params.bounds.right - fadeWidth, 0.0f },
                        .endPoint = { params.bounds.right, 0.0f },
                        .stops = { { 0.0f, runColor }, { 1.0f, runColor.withOpacity(0.0f) } },
                    };
                }
                m_native->drawRun(context, { left, baselineY }, glyphs, brush);
            }
            glyphs.clear();
        };

        std::size_t column = 0;
        for (std::size_t i = 0; i != text.size(); ++i)
        {
            const wchar_t character = text[i];
            const float left = static_cast<float>(column) * m_cellWidth;
            if (character == L'\t')
            {
                flush(left);
                column = nextTabStop(column);
                continue;
            }

            const Color color = colorAt(params.colors, i, colorCursor, defaultColor, context);
            if (!glyphs.empty() && color != runColor)
                flush(left);
            if (glyphs.empty())
            {
                runColor = color;
                runLeft = left;
            }
            glyphs.push_back(glyphOf(character));
            ++column;
        }
        flush(static_cast<float>(column) * m_cellWidth);
    }

    MonoGlyph MonoFont::resolve(char32_t code)
    {
        if (isRefused(code))
            return k_noMonoGlyph;
        return m_native->glyphOf(code);
    }
}
