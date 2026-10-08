module SeeDocs_App.Studio.PageText;

import SeeDocs_App.Pages;

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
        constexpr float k_footnoteGap = 6.0f;   // the room under a footnote, as an empty line
        constexpr std::wstring_view k_bullet = L"• ";

        void writeBlock(Text& text, const Block& block, const float indent)
        {
            switch (block.kind)
            {
                case BlockKind::Paragraph:
                    text << ParaIndent{ indent };
                    writeRuns(text, block.runs);
                    text << k_endLine;
                    break;
                case BlockKind::SubHeading:
                    text << ParaIndent{ indent } << TextStyleId::Section;
                    writeRuns(text, block.runs);
                    text << PopTextStyle{} << k_endLine;
                    break;
                case BlockKind::Code:
                    for (const Runs& line : block.lines)
                    {
                        text << ParaIndent{ indent + k_codeIndent } << TextStyleId::Code;
                        writeRuns(text, line);
                        text << PopTextStyle{} << k_endLine;
                    }
                    break;
                case BlockKind::Bullets:
                    for (const Runs& item : block.items)
                    {
                        text << ParaIndent{ indent + k_bulletIndent } << k_bullet;
                        writeRuns(text, item);
                        text << k_endLine;
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

        // The declaration lexed as C++, a line at a time, the tokens drawn in an ink of their
        // own in the order they stand; a kind drawn in the text's ink is left out.
        [[nodiscard]] InkedRanges inkedTokens(const std::wstring& source)
        {
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
                state = Syntax::lexLine(Syntax::Languages::cpp, line, state, strings, &tokens);
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

    void writeRuns(Text& text, const Runs& runs)
    {
        for (const Run& run : runs)
        {
            const bool linked = !run.link.empty();
            if (linked)
                text << PushLink{ run.link };
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

    void writeBlocks(Text& text, const Blocks& blocks, const float indent)
    {
        for (const Block& block : blocks)
            writeBlock(text, block, indent);
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
        text << ParaIndent{ 0.0f } << PushAnchor{ std::wstring{ anchor } } << InkGrade::Muted
            << mark << PopColor{} << L" ";
        writeRuns(text, name);
        text << PopAnchor{} << k_endLine;
        writeBlocks(text, excerpt.blocks);
        text << PushFontSize{ k_footnoteGap } << k_endLine << PopFontSize{};
    }

    // The runs are the one declaration cut round its links, and the tokens are lexed over the
    // whole of it, so a token is clipped to the run it is written in and one reaching past the
    // run's end goes on in the next.
    void writeSignature(Text& text, const Runs& runs, const bool links)
    {
        std::wstring source;
        for (const Run& run : runs)
            source += run.text;
        const InkedRanges inked = inkedTokens(source);
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
