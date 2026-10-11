module SeeDocs_App.PageText;

import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        // Design units, as every measure a Text states is.
        constexpr float k_codeIndent = 18.0f;
        constexpr float k_bulletIndent = 18.0f;
        constexpr std::wstring_view k_bullet = L"• ";

        // Starts a line under what the text holds already, so the prose ends with no line end.
        void openLine(Text& text, bool& opened, const float indent)
        {
            if (opened)
                text << k_endLine;
            opened = true;
            text << ParaIndent{ indent };
        }

        void writeBlock(Text& text, const Block& block, const float indent, bool& opened)
        {
            switch (block.kind)
            {
                case BlockKind::Paragraph:
                    openLine(text, opened, indent);
                    writeRuns(text, block.runs);
                    break;
                case BlockKind::SubHeading:
                    openLine(text, opened, indent);
                    text << TextStyleId::Section;
                    writeRuns(text, block.runs);
                    text << PopTextStyle{};
                    break;
                case BlockKind::Code:
                    for (const Runs& line : block.lines)
                    {
                        openLine(text, opened, indent + k_codeIndent);
                        text << TextStyleId::Code;
                        writeRuns(text, line);
                        text << PopTextStyle{};
                    }
                    break;
                case BlockKind::Bullets:
                    for (const Runs& item : block.items)
                    {
                        openLine(text, opened, indent + k_bulletIndent);
                        text << k_bullet;
                        writeRuns(text, item);
                    }
                    break;
            }
        }

        // A token of a declaration with an ink of its own: where it stands in the whole text.
        struct InkedRange
        {
            std::size_t start{ 0 };
            std::size_t end{ 0 };
            Ink ink{};
        };

        using InkedRanges = std::vector<InkedRange>;

        // The declaration lexed in its language, a line at a time, the tokens drawn in an ink of
        // their own in the order they stand; a kind drawn in the text's ink is left out.
        [[nodiscard]] InkedRanges inkedTokens(const std::wstring& source,
            const SourceLanguage language)
        {
            const Syntax::Language& lexer = language == SourceLanguage::Pascal
                ? Syntax::Languages::pascal
                : Syntax::Languages::cpp;
            const Syntax::Inks inks = Syntax::defaultInks();
            const Ink textInk{};
            InkedRanges ranges;
            Syntax::StateStrings strings;
            Syntax::Tokens tokens;
            Syntax::LineState state{};
            std::size_t lineStart = 0;
            while (true)
            {
                const std::size_t newline = source.find(k_endLine, lineStart);
                const std::size_t lineEnd = newline == std::wstring::npos
                    ? source.size()
                    : newline;
                const std::wstring_view line =
                    std::wstring_view{ source }.substr(lineStart, lineEnd - lineStart);
                tokens.clear();
                state = Syntax::lexLine(lexer, line, state, strings, &tokens);
                for (const Syntax::Token& token : tokens)
                {
                    const Ink& ink = inks[token.kind];
                    if (ink == textInk)
                        continue;
                    const std::size_t start = lineStart + token.range.start;
                    ranges.push_back({
                        .start = start,
                        .end = start + token.range.length,
                        .ink = ink
                    });
                }
                if (newline == std::wstring::npos)
                    return ranges;
                lineStart = newline + 1;
            }
        }
    }

    // **Found words are set bold** over whatever the run's own style is, a link's included.
    void writeRuns(Text& text, const Runs& runs)
    {
        for (const Run& run : runs)
        {
            const bool linked = !run.link.empty();
            if (linked)
                text << PushLink{ run.link };
            if (run.found)
                text << TextOp::PushBold;
            switch (run.style)
            {
                case RunStyle::Plain:
                    text << run.text;
                    break;
                case RunStyle::Code:
                    text << TextStyleId::Code << run.text << PopTextStyle{};
                    break;
                case RunStyle::Muted:
                    text << InkGrade::Muted << run.text << PopColor{};
                    break;
                case RunStyle::Bold:
                    text << TextOp::PushBold << run.text << TextOp::PopBold;
                    break;
                case RunStyle::Missing:
                    text << InkWell::redInk(InkGrade::Muted) << TextOp::PushItalic << run.text
                        << TextOp::PopItalic << PopColor{};
                    break;
            }
            if (run.found)
                text << TextOp::PopBold;
            if (linked)
                text << PopLink{};
        }
    }

    Text textOf(const Runs& runs)
    {
        Text text;
        writeRuns(text, runs);
        return text;
    }

    // **A mark is written where the look changes between two characters**, not where a marker
    // stands: a run cut round a link closes its bold and opens it again at one place, and the
    // words read on as one bold span.
    std::wstring markdownOf(const Text& text)
    {
        const std::wstring& plain = text.plainText();
        const Text::Markers& markers = text.markers();
        std::wstring result;
        std::size_t boldDepth = 0;
        std::vector<TextStyleId> styles;
        bool boldOpen = false;
        bool codeOpen = false;
        std::size_t next = 0;
        for (std::size_t at = 0; at <= plain.size(); ++at)
        {
            for (; next < markers.size() && markers[next].first <= at; ++next)
            {
                const FormatItem& item = markers[next].second;
                if (const TextOp* op = std::get_if<TextOp>(&item))
                {
                    if (*op == TextOp::PushBold)
                        ++boldDepth;
                    else if (*op == TextOp::PopBold && boldDepth > 0)
                        --boldDepth;
                }
                else if (const PushTextStyle* push = std::get_if<PushTextStyle>(&item))
                {
                    styles.push_back(push->style);
                }
                else if (std::holds_alternative<PopTextStyle>(item) && !styles.empty())
                {
                    styles.pop_back();
                }
            }
            const bool atEnd = at == plain.size();
            const bool bold = !atEnd && boldDepth > 0;
            const bool code = !atEnd
                && std::ranges::find(styles, TextStyleId::Code) != styles.end();
            if (codeOpen && !code)
            {
                result += k_codeMark;
                codeOpen = false;
            }
            if (boldOpen != bold)
            {
                result += k_boldMark;
                boldOpen = bold;
            }
            if (code && !codeOpen)
            {
                result += k_codeMark;
                codeOpen = true;
            }
            if (!atEnd)
                result += plain[at];
        }
        return result;
    }

    void writeBlocks(Text& text, const Blocks& blocks, const float indent)
    {
        bool opened = !text.empty();
        for (const Block& block : blocks)
            writeBlock(text, block, indent, opened);
    }

    Text textOf(const Blocks& blocks)
    {
        Text text;
        writeBlocks(text, blocks);
        return text;
    }

    void writeFootnote(Text& text, const std::wstring_view anchor, const std::wstring_view mark,
        const Runs& name, const Excerpt& excerpt)
    {
        text << ParaIndent{ 0.0f } << PushAnchor{ std::wstring{ anchor } } << InkColor::Spot
            << mark << PopColor{} << L" ";
        writeRuns(text, name);
        text << PopAnchor{};
        writeBlocks(text, excerpt.blocks);
    }

    // The runs are the one declaration cut round its links, and the tokens are lexed over the
    // whole of it, so a token is clipped to the run it is written in and one reaching past the
    // run's end goes on in the next.
    void writeSignature(Text& text, const Runs& runs, const bool links,
        const SourceLanguage language)
    {
        std::wstring source;
        for (const Run& run : runs)
            source += run.text;
        const InkedRanges inked = inkedTokens(source, language);
        text << TextStyleId::Code;
        std::size_t at = 0;     // where the run starts in the source
        std::size_t next = 0;   // the first inked token not yet written out
        for (const Run& run : runs)
        {
            const std::size_t end = at + run.text.size();
            if (links && !run.link.empty())
            {
                text << PushLink{ run.link } << run.text << PopLink{};
                while (next < inked.size() && inked[next].end <= end)
                    ++next;
                at = end;
                continue;
            }
            std::size_t plain = at;
            while (next < inked.size() && inked[next].start < end)
            {
                const InkedRange& range = inked[next];
                const std::size_t start = std::max(range.start, at);
                const std::size_t stop = std::min(range.end, end);
                if (start > plain)
                    text << source.substr(plain, start - plain);
                text << range.ink << source.substr(start, stop - start) << PopColor{};
                plain = stop;
                if (range.end > end)
                    break;
                ++next;
            }
            if (plain < end)
                text << source.substr(plain, end - plain);
            at = end;
        }
        text << PopTextStyle{};
    }
}
