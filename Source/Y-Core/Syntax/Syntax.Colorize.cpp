module ClaFi.Core.Syntax.Colorize;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    void appendColorized(Text& out, const std::wstring_view source, const Language& language,
        const Inks& inks)
    {
        // A kind with no ink of its own is drawn in the text's, and a run in that ink is plain: it
        // takes no marker pair, and most of a source file is that - the names, the punctuation and
        // the blanks between the tokens that mean something.
        const Ink textInk{};

        StateStrings strings;
        Tokens tokens;
        LineState state{};

        // Where the plain run waiting to be written begins. Held rather than written per token,
        // so the plain text between two coloured runs costs one append instead of one per
        // character.
        std::size_t plainStart = 0;

        // The coloured run still open, waiting for the token after it to say whether it goes on:
        // two tokens in one ink standing side by side are one run and one marker pair.
        std::size_t runStart = 0;
        std::size_t runEnd = 0;
        Ink runInk{};
        bool runOpen = false;

        auto closeRun = [&](){
            if (!runOpen)
                return;
            out << source.substr(plainStart, runStart - plainStart);
            out << runInk;
            out << source.substr(runStart, runEnd - runStart);
            out << PopColor{};
            plainStart = runEnd;
            runOpen = false;
        };

        std::size_t lineStart = 0;
        for (;;)
        {
            const std::size_t newline = source.find(L'\n', lineStart);
            const std::size_t lineEnd = newline == std::wstring_view::npos
                ? source.size()
                : newline;
            const std::wstring_view line = source.substr(lineStart, lineEnd - lineStart);

            tokens.clear();
            state = lexLine(language, line, state, strings, &tokens);
            for (const Token& token : tokens)
            {
                const Ink& ink = inks[token.kind];
                if (ink == textInk)
                    continue;

                const std::size_t start = lineStart + token.range.start;
                const std::size_t end = start + token.range.length;
                if (runOpen && ink == runInk && start == runEnd)
                {
                    runEnd = end;
                    continue;
                }
                closeRun();
                runStart = start;
                runEnd = end;
                runInk = ink;
                runOpen = true;
            }

            if (newline == std::wstring_view::npos)
                break;
            lineStart = newline + 1;
        }

        closeRun();
        out << source.substr(plainStart);
    }

    Text colorize(const std::wstring_view source, const Language& language, const Inks& inks)
    {
        Text result;
        appendColorized(result, source, language, inks);
        return result;
    }
}
