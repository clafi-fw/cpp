module SeeDocs_App.Ui.PageText;

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

namespace SeeDocs_App::Ui
{
    using namespace ClaFi;

    namespace
    {
        // Design units, as every measure a Text states is.
        constexpr float k_codeIndent = 18.0f;
        constexpr float k_bulletIndent = 18.0f;
        constexpr std::wstring_view k_bullet = L"• ";
        constexpr std::wstring_view k_noteBullet = L"- ";   // what a note writes an item behind
        constexpr std::wstring_view k_codeFence = L"    ";  // what a note sets a line of code in by

        // Each line stands under what the text holds already, and the prose ends with no line end.
        void writeBlock(Text& text, const Block& block, const float indent)
        {
            switch (block.kind)
            {
                case BlockKind::Paragraph:
                    text << ParagraphText{ ParaIndent{ indent }, textOf(block.runs) };
                    break;
                case BlockKind::SubHeading:
                    text << ParagraphText{
                        ParaIndent{ indent },
                        StyledText{ TextStyles::Section, textOf(block.runs) }
                    };
                    break;
                case BlockKind::Code:
                    for (const Runs& line : block.lines)
                    {
                        // A blank line of the block holds no words, so it is its line end alone.
                        if (line.empty())
                        {
                            text << k_endLine;
                            continue;
                        }
                        text << ParagraphText{
                            ParaIndent{ indent + k_codeIndent },
                            StyledText{ TextStyles::Code, textOf(line) }
                        };
                    }
                    break;
                case BlockKind::Bullets:
                    for (const Runs& item : block.items)
                    {
                        text << ParagraphText{
                            ParaIndent{ indent + k_bulletIndent },
                            k_bullet,
                            textOf(item)
                        };
                    }
                    break;
            }
        }

        // What a line of a hint's text stands for in a note.
        enum class ProseLine
        {
            Words,
            Item,
            Code
        };

        // A line all code behind its fence, as a hint shows a line of code: `    code`.
        [[nodiscard]] bool isFencedCode(const std::wstring_view line)
        {
            return line.size() > k_codeFence.size() + 2 && line.front() == k_codeMark
                && line.back() == k_codeMark && line.find(k_codeMark, 1) == line.size() - 1
                && line.substr(1).starts_with(k_codeFence);
        }

        // **The marked lines as a note's prose**: an item's bullet a note's dash, a line of code
        // set in by its fence alone, a paragraph a blank line from what stands round it.
        [[nodiscard]] std::wstring proseOf(const std::wstring_view marked)
        {
            std::wstring result;
            std::optional<ProseLine> previous;
            std::size_t start = 0;
            while (start <= marked.size())
            {
                std::size_t end = marked.find(k_endLine, start);
                if (end == std::wstring_view::npos)
                    end = marked.size();
                const std::wstring_view line = marked.substr(start, end - start);
                start = end + 1;
                ProseLine kind = ProseLine::Words;
                std::wstring said{ line };
                if (line.starts_with(k_bullet))
                {
                    kind = ProseLine::Item;
                    said = std::wstring{ k_noteBullet }
                        + std::wstring{ line.substr(k_bullet.size()) };
                }
                else if (isFencedCode(line))
                {
                    kind = ProseLine::Code;
                    said = std::wstring{ line.substr(1, line.size() - 2) };
                }
                if (said.find_first_not_of(L" \t") == std::wstring::npos)
                    continue;
                if (previous)
                {
                    const bool runsOn = kind == *previous && kind != ProseLine::Words;
                    result += runsOn ? L"\n" : L"\n\n";
                }
                result += said;
                previous = kind;
            }
            return result;
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
                ? Syntax::Languages::Pascal
                : Syntax::Languages::Cpp;
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

        // The runs as styled, each linked run inside its link, a colour given inside the link.
        void writeStyledRuns(Text& text, const Runs& runs, const std::optional<Color> color)
        {
            for (const Run& run : runs)
            {
                const auto writeLinked = [&text, &run](const auto& words){
                    if (run.link.empty())
                        text << words;
                    else
                        text << LinkText{ run.link, words };
                };
                const auto writeColored = [&writeLinked, &color](const auto& words){
                    if (color)
                        writeLinked(ColoredText{ *color, words });
                    else
                        writeLinked(words);
                };
                switch (run.style)
                {
                    case RunStyle::Plain:
                        writeColored(run.text);
                        break;
                    case RunStyle::Code:
                        writeColored(StyledText{ TextStyles::Code, run.text });
                        break;
                    case RunStyle::Path:
                        writeColored(StyledText{
                            TextStyles::Code,
                            ColoredText{ InkWell::spotInk(), run.text }
                        });
                        break;
                    case RunStyle::Muted:
                        writeColored(ColoredText{ InkGrade::Muted, run.text });
                        break;
                    case RunStyle::Bold:
                        writeColored(BoldText{ run.text });
                        break;
                }
            }
        }
    }

    void writeRuns(Text& text, const Runs& runs)
    {
        writeStyledRuns(text, runs, std::nullopt);
    }

    void writeRuns(Text& text, const Runs& runs, const Color color)
    {
        writeStyledRuns(text, runs, color);
    }

    Text textOf(const Runs& runs)
    {
        Text text;
        writeRuns(text, runs);
        return text;
    }

    // **A mark is written where the look changes between two characters**, not where a run
    // starts: a run cut round a link is bold on both sides of the cut, and the words read on as
    // one bold span. A line end closes whatever is open, as a paragraph's end does in a note.
    std::wstring markdownOf(const Text& text)
    {
        const std::wstring& plain = text.plainText();
        std::wstring marked;
        bool boldOpen = false;
        bool codeOpen = false;
        // The marks standing before a character in that look, or before the end where both close.
        const auto markLook = [&marked, &boldOpen, &codeOpen](const bool bold, const bool code){
            if (codeOpen && !code)
            {
                marked += k_codeMark;
                codeOpen = false;
            }
            if (boldOpen != bold)
            {
                marked += k_boldMark;
                boldOpen = bold;
            }
            if (code && !codeOpen)
            {
                marked += k_codeMark;
                codeOpen = true;
            }
        };
        text.forEachRun([&plain, &marked, &markLook](const TextRunView& run){
            const bool bold = run.look.fontWeight == FontWeight::Bold;
            const bool code = run.look.style == &TextStyles::Code;
            for (std::size_t at = run.range.start; at != run.range.end(); ++at)
            {
                const bool atBreak = plain[at] == k_endLine;
                markLook(bold && !atBreak, code && !atBreak);
                marked += plain[at];
            }
        });
        markLook(false, false);
        return proseOf(marked);
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
        text << AnchorText{
            std::wstring{ anchor },
            ColoredText{ InkColor::Spot, mark },
            L" ",
            textOf(name)
        };
        writeBlocks(text, excerpt.blocks);
    }

    // The holder is written as plain words: the link it takes is to the footnote, not to the
    // page its name links to elsewhere. An empty holder is the declaration above.
    void writeShownAt(Text& text, const std::wstring_view anchor, const std::wstring_view mark,
        const Runs& name, const Runs& holder, const std::wstring_view holderLink)
    {
        if (!name.empty())
        {
            text << AnchorText{
                std::wstring{ anchor },
                ColoredText{ InkColor::Spot, mark },
                L" ",
                textOf(name)
            };
        }
        const std::wstring_view lead = name.empty() ? L"See " : L" - see ";
        text << ColoredText{ InkGrade::Muted, lead };
        if (holder.empty())
        {
            text << ColoredText{ InkGrade::Muted, L"the note above" };
            return;
        }
        Runs unlinked = holder;
        for (Run& run : unlinked)
            run.link.clear();
        if (holderLink.empty())
        {
            writeRuns(text, unlinked);
            return;
        }
        text << LinkText{ std::wstring{ holderLink }, textOf(unlinked) };
    }

    // The runs are the one declaration cut round its links, and the tokens are lexed over the
    // whole of it, so a token is clipped to the run it is written in and one reaching past the
    // run's end goes on in the next. A linked token keeps its ink, since a colour given inside a
    // link wins over the link's accent.
    void writeSignature(Text& text, const Runs& runs, const bool links,
        const SourceLanguage language)
    {
        std::wstring source;
        for (const Run& run : runs)
            source += run.text;
        const InkedRanges inked = inkedTokens(source, language);
        Text code{};
        std::size_t at = 0;     // where the run starts in the source
        std::size_t next = 0;   // the first inked token not yet written out
        for (const Run& run : runs)
        {
            const std::size_t end = at + run.text.size();
            Text words{};
            std::size_t plain = at;
            while (next < inked.size() && inked[next].start < end)
            {
                const InkedRange& range = inked[next];
                const std::size_t start = std::max(range.start, at);
                const std::size_t stop = std::min(range.end, end);
                if (start > plain)
                    words << source.substr(plain, start - plain);
                words << ColoredText{ range.ink, source.substr(start, stop - start) };
                plain = stop;
                if (range.end > end)
                    break;
                ++next;
            }
            if (plain < end)
                words << source.substr(plain, end - plain);
            if (links && !run.link.empty())
                code << LinkText{ run.link, words };
            else
                code << words;
            at = end;
        }
        text << StyledText{ TextStyles::Code, code };
    }
}
