module ClaFi.Core.Syntax.Languages;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    namespace
    {
        // Emits from `start` to the closer on this line, or to the line's end with the mode kept
        // for the line after. The search begins at index, which is past the opener when there is
        // one on this line.
        void takeToCloser(Scan& scan, const std::size_t start, const std::wstring_view closer,
            const TokenKind kind, const std::uint8_t mode)
        {
            const std::size_t close = scan.line.find(closer, scan.index);
            if (close == std::wstring_view::npos)
            {
                scan.emit(start, scan.line.size(), kind);
                scan.index = scan.line.size();
                scan.state.mode = mode;
                return;
            }
            const std::size_t end = close + closer.size();
            scan.emit(start, end, kind);
            scan.index = end;
            scan.state.mode = Mode::normal;
        }

        void takeToLineEnd(Scan& scan, const TokenKind kind)
        {
            scan.emit(scan.index, scan.line.size(), kind);
            scan.index = scan.line.size();
        }


        //---------------------------------------------------------------------
        // Reading a whole text, which is what a detector is handed


        constexpr std::wstring_view k_blanks = L" \t\r\n";

        // The text without the blanks at either end.
        [[nodiscard]] std::wstring_view trimmed(const std::wstring_view text)
        {
            const std::size_t first = text.find_first_not_of(k_blanks);
            if (first == std::wstring_view::npos)
                return {};

            const std::size_t last = text.find_last_not_of(k_blanks);
            return text.substr(first, last - first + 1);
        }

        // The line starting at `start`, its blanks trimmed, and `start` moved to the next one.
        [[nodiscard]] std::wstring_view lineFrom(const std::wstring_view text, std::size_t& start)
        {
            std::size_t lineEnd = text.find(L'\n', start);
            if (lineEnd == std::wstring_view::npos)
                lineEnd = text.size();

            const std::wstring_view line = trimmed(text.substr(start, lineEnd - start));
            start = lineEnd + 1;
            return line;
        }

        // Whether the text opens with one of the words.
        [[nodiscard]] bool opensWithAny(const std::wstring_view text, const Words words)
        {
            for (const std::wstring_view word : words)
            {
                if (text.starts_with(word))
                    return true;
            }
            return false;
        }

        // Whether the text opens with the word, whatever the case it is spelled in.
        [[nodiscard]] bool opensWithNoCase(const std::wstring_view text,
            const std::wstring_view lowerWord)
        {
            if (text.size() < lowerWord.size())
                return false;

            for (std::size_t i = 0; i != lowerWord.size(); ++i)
            {
                if (static_cast<wchar_t>(std::towlower(text[i])) != lowerWord[i])
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool endsWithNoCase(const std::wstring_view text,
            const std::wstring_view lowerWord)
        {
            return text.size() >= lowerWord.size()
                && opensWithNoCase(text.substr(text.size() - lowerWord.size()), lowerWord);
        }

        // Whether the text opens with one of the words whole - the word, and then no letter,
        // digit or underscore - whatever the case it is spelled in.
        [[nodiscard]] bool opensWithAnyWordNoCase(const std::wstring_view text,
            const Words lowerWords)
        {
            for (const std::wstring_view word : lowerWords)
            {
                if (!opensWithNoCase(text, word))
                    continue;
                if (text.size() == word.size())
                    return true;

                const wchar_t after = text[word.size()];
                if (std::iswalnum(after) == 0 && after != L'_')
                    return true;
            }
            return false;
        }


        //---------------------------------------------------------------------
        // C++


        namespace CppMode
        {
            constexpr std::uint8_t rawString = Mode::firstLanguageMode;
        }

        // What a C++ source spells and no other language here does: a directive, a module
        // declaration, the standard namespace.
        constexpr auto k_cppSignatures = std::to_array<std::wstring_view>({
            L"#include", L"#pragma", L"#define", L"#if", L"#endif", L"export module"
        });

        // The shape of a source file, which other languages spell as well.
        constexpr auto k_cppLineOpeners = std::to_array<std::wstring_view>({
            L"import ", L"module ", L"namespace ", L"template<", L"template <"
        });

        // The statements that open with a bracketed condition. Sorted.
        constexpr auto k_cppConditionWords = std::to_array<std::wstring_view>({
            L"for", L"if", L"switch", L"while"
        });
        static_assert(std::ranges::is_sorted(k_cppConditionWords));

        // What Pascal puts after a bracketed condition, where C++ has the line's end, a brace or
        // a statement. Lower case, as Pascal reads them in any case.
        constexpr auto k_conditionContinuers = std::to_array<std::wstring_view>({
            L"do", L"then"
        });

        // The words a line of C++ opens with, less the ones a line of Pascal opens with as well -
        // break, class, new, delete and the like. A line opening with one of them or with a type
        // of the table, and closed like a line of source, is the language's shape. Sorted.
        constexpr auto k_cppLineWords = std::to_array<std::wstring_view>({
            L"auto", L"catch", L"co_await", L"co_return", L"co_yield", L"concept", L"const",
            L"consteval", L"constexpr", L"constinit", L"decltype", L"explicit", L"extern",
            L"friend", L"inline", L"mutable", L"return", L"static", L"static_assert", L"this",
            L"thread_local", L"throw", L"typedef", L"typename", L"using", L"virtual", L"volatile"
        });
        static_assert(std::ranges::is_sorted(k_cppLineWords));

        // The words that open a type's own declaration, which the lines after close. No other
        // language here spells them. Sorted.
        constexpr auto k_cppDeclarers = std::to_array<std::wstring_view>({
            L"enum", L"struct", L"union"
        });
        static_assert(std::ranges::is_sorted(k_cppDeclarers));

        // The words that open a label, which a colon closes: a switch's cases, a class's access
        // sections. Sorted.
        constexpr auto k_cppLabelWords = std::to_array<std::wstring_view>({
            L"case", L"default", L"private", L"protected", L"public"
        });
        static_assert(std::ranges::is_sorted(k_cppLabelWords));

        // What a line of source closes with: a terminator, a brace either way, a bracket.
        constexpr std::wstring_view k_cppLineClosers = L";{})";

        // A letter, a digit or an underscore, which is what a C++ name is made of.
        [[nodiscard]] bool isCppNameChar(const wchar_t value)
        {
            return std::iswalnum(value) != 0 || value == L'_';
        }

        // The name a line opens with, whole, or nothing where it opens with anything else.
        [[nodiscard]] std::wstring_view leadingWord(const std::wstring_view line)
        {
            std::size_t end = 0;
            while (end != line.size() && isCppNameChar(line[end]))
                ++end;
            return line.substr(0, end);
        }

        // The line without a comment ending it, its blanks trimmed.
        [[nodiscard]] std::wstring_view withoutLineComment(const std::wstring_view line)
        {
            return trimmed(line.substr(0, line.find(L"//")));
        }

        // One past the bracket matching the one at `open`, or npos where the line ends first.
        [[nodiscard]] std::size_t pastMatchingBracket(const std::wstring_view line,
            const std::size_t open)
        {
            int depth = 0;
            for (std::size_t i = open; i != line.size(); ++i)
            {
                if (line[i] == L'(')
                    ++depth;
                if (line[i] == L')')
                    --depth;
                if (depth == 0)
                    return i + 1;
            }
            return std::wstring_view::npos;
        }

        // Whether the line, its comment taken off, closes like a line of source.
        [[nodiscard]] bool closesLikeSource(const std::wstring_view code)
        {
            return !code.empty() && k_cppLineClosers.find(code.back()) != std::wstring_view::npos;
        }

        // Whether the line, its comment taken off, is a statement opening with a bracketed
        // condition - if (x), for (;;), while (x), switch (x) - closed like a line of source.
        // Pascal spells the first three as well and puts then or do after the bracket, where C++
        // has the line's end, a brace or a statement.
        [[nodiscard]] bool isCppConditionLine(const std::wstring_view code)
        {
            const std::wstring_view word = leadingWord(code);
            if (!isListed(k_cppConditionWords, word) || !closesLikeSource(code))
                return false;

            const std::size_t open = code.find_first_not_of(L" \t", word.size());
            if (open == std::wstring_view::npos || code[open] != L'(')
                return false;

            const std::size_t close = pastMatchingBracket(code, open);
            if (close == std::wstring_view::npos)
                return false;

            const std::wstring_view after = trimmed(code.substr(close));
            return !opensWithAnyWordNoCase(after, k_conditionContinuers);
        }

        // Whether the line, its comment taken off, opens a type's declaration, is a label, or
        // opens with a word of the language's and closes like a line of source.
        [[nodiscard]] bool isCppStatementLine(const std::wstring_view code)
        {
            const std::wstring_view word = leadingWord(code);
            if (isListed(k_cppDeclarers, word))
                return true;
            if (isListed(k_cppLabelWords, word))
                return code.ends_with(L':');

            return (isListed(k_cppLineWords, word) || isListed(CppTables::types, word))
                && closesLikeSource(code);
        }

        // Whether the token joins two names anywhere in the text, as :: and -> do in an
        // expression: a name character or one of `closers` before it, and a name after it.
        [[nodiscard]] bool joinsNames(const std::wstring_view text, const std::wstring_view token,
            const std::wstring_view closers)
        {
            std::size_t at = text.find(token);
            while (at != std::wstring_view::npos && at + token.size() < text.size())
            {
                const wchar_t before = at == 0 ? L' ' : text[at - 1];
                const wchar_t after = text[at + token.size()];
                const bool fromValue = isCppNameChar(before)
                    || closers.find(before) != std::wstring_view::npos;
                if (fromValue && isCppNameChar(after) && !isDigit(after))
                    return true;

                at = text.find(token, at + token.size());
            }
            return false;
        }

        // What a reading of the lines finds of the language's shape.
        struct CppSigns
        {
            bool signature{ false };      // a directive or a module declaration opening a line
            bool opener{ false };         // a namespace, template, import or module line
            bool statement{ false };      // a line shaped like a statement, a declaration, a label
            std::size_t codeLines{ 0 };   // the lines with anything but a comment on them
            std::size_t closedLines{ 0 }; // those among them closed like a line of source
        };

        // Whether most lines close the way a line of source does, which prose about the code -
        // a paragraph naming a member through its class - does not.
        [[nodiscard]] bool isSourceShaped(const CppSigns& signs)
        {
            return signs.codeLines > 0 && signs.closedLines * 2 >= signs.codeLines;
        }

        [[nodiscard]] CppSigns readCppSigns(const std::wstring_view text)
        {
            CppSigns signs;
            std::size_t start = 0;
            while (start < text.size())
            {
                const std::wstring_view line = lineFrom(text, start);
                const std::wstring_view code = withoutLineComment(line);
                if (code.empty())
                    continue;

                ++signs.codeLines;
                if (closesLikeSource(code))
                    ++signs.closedLines;
                if (opensWithAny(code, k_cppSignatures))
                    signs.signature = true;
                if (opensWithAny(code, k_cppLineOpeners))
                    signs.opener = true;
                if (isCppConditionLine(code) || isCppStatementLine(code)
                    || code.starts_with(L"[["))
                {
                    signs.statement = true;
                }
            }
            return signs;
        }

        // The standard's own bound on a raw string's delimiter.
        constexpr std::size_t k_rawDelimiterMax = 16;

        // Where a literal's quote stands past the prefix at index - L, u, U, u8, and R or a
        // prefix and R for a raw string.
        struct LiteralPrefix
        {
            std::size_t quote;
            bool raw;
        };

        // The prefix at index with a quote after it, or nothing where what stands there is a
        // name that happens to begin with one of those letters.
        [[nodiscard]] std::optional<LiteralPrefix> literalPrefixAt(const Scan& scan)
        {
            std::size_t at = scan.index;
            const wchar_t first = scan.line[at];
            if (first == L'u' && scan.peek() == L'8')
                at += 2;
            else if (first == L'L' || first == L'u' || first == L'U')
                at += 1;

            bool raw = false;
            if (at < scan.line.size() && scan.line[at] == L'R')
            {
                raw = true;
                ++at;
            }
            if (at == scan.index || at >= scan.line.size())
                return std::nullopt;

            const wchar_t quote = scan.line[at];
            if (quote == L'"' || (!raw && quote == L'\''))
                return LiteralPrefix{ at, raw };
            return std::nullopt;
        }

        // The raw string from `start` to its closer - a parenthesis, the delimiter and a quote -
        // on this line, or to the line's end with the delimiter carried to the line after by id.
        void continueRawString(Scan& scan, const std::size_t start,
            const std::wstring_view delimiter, const std::uint16_t delimiterId)
        {
            std::size_t at = scan.index;
            for (;;)
            {
                at = scan.line.find(L')', at);
                if (at == std::wstring_view::npos)
                    break;

                const std::wstring_view rest = scan.line.substr(at + 1);
                if (rest.starts_with(delimiter) && rest.size() > delimiter.size()
                    && rest[delimiter.size()] == L'"')
                {
                    const std::size_t end = at + 1 + delimiter.size() + 1;
                    scan.emit(start, end, TokenKind::String);
                    scan.index = end;
                    scan.state.mode = Mode::normal;
                    scan.state.text = 0;
                    return;
                }
                ++at;
            }

            scan.emit(start, scan.line.size(), TokenKind::String);
            scan.index = scan.line.size();
            scan.state.mode = CppMode::rawString;
            scan.state.text = delimiterId;
        }

        void takeRawString(Scan& scan, const std::size_t quote)
        {
            const std::size_t open = scan.line.find(L'(', quote + 1);
            if (open == std::wstring_view::npos || open - quote - 1 > k_rawDelimiterMax)
            {
                // Not a raw string after all, so an ordinary one: R"( is what opens one.
                scan.takeQuoted(scan.index, quote, L'"', L'\\');
                return;
            }

            const std::size_t start = scan.index;
            const std::wstring_view delimiter = scan.line.substr(quote + 1, open - quote - 1);
            scan.index = open + 1;
            continueRawString(scan, start, delimiter, scan.strings.idOf(delimiter));
        }

        // The hash and the directive's name as one run, and the path of an include after it as a
        // string. Whatever else the line carries is source, and is read as source.
        void takeDirective(Scan& scan)
        {
            const std::size_t start = scan.index;
            const std::size_t wordStart = scan.pastBlanks(start + 1);
            const std::size_t wordEnd = endOfName(scan.line, wordStart, scan.language);
            scan.emit(start, wordEnd, TokenKind::Directive);
            scan.index = wordEnd;

            const std::wstring_view word = scan.line.substr(wordStart, wordEnd - wordStart);
            if (word != L"include" && word != L"import")
                return;

            const std::size_t pathStart = scan.pastBlanks(wordEnd);
            if (pathStart == scan.line.size() || scan.line[pathStart] != L'<')
                return;

            const std::size_t close = scan.line.find(L'>', pathStart);
            const std::size_t end = close == std::wstring_view::npos ? scan.line.size() : close + 1;
            scan.emit(pathStart, end, TokenKind::String);
            scan.index = end;
        }

        void takeAttribute(Scan& scan)
        {
            const std::size_t close = scan.line.find(L"]]", scan.index + 2);
            const std::size_t end = close == std::wstring_view::npos ? scan.line.size() : close + 2;
            scan.emit(scan.index, end, TokenKind::Attribute);
            scan.index = end;
        }
    }

    bool cppHook(Scan& scan)
    {
        if (scan.state.mode == CppMode::rawString)
        {
            const std::uint16_t delimiterId = scan.state.text;
            continueRawString(scan, scan.index, scan.strings.stringOf(delimiterId), delimiterId);
            return true;
        }

        const wchar_t current = scan.current();
        if (current == L'#' && scan.atLineStart())
        {
            takeDirective(scan);
            return true;
        }
        if (current == L'[' && scan.peek() == L'[')
        {
            takeAttribute(scan);
            return true;
        }
        if (current == L'L' || current == L'u' || current == L'U' || current == L'R')
        {
            const std::optional<LiteralPrefix> prefix = literalPrefixAt(scan);
            if (!prefix)
                return false;

            if (prefix->raw)
                takeRawString(scan, prefix->quote);
            else
                scan.takeQuoted(scan.index, prefix->quote, scan.line[prefix->quote], L'\\');
            return true;
        }
        return false;
    }

    // A directive, a module declaration or the standard namespace: no other language here spells
    // them. The shape of a source - a namespace or a template line, a bracketed condition, a
    // declaration, a label, or a qualified name or an arrow among lines closed like source - is
    // one a text of another language could share.
    Claim cppDetector(const std::wstring_view text)
    {
        const CppSigns signs = readCppSigns(text);
        if (signs.signature || text.find(L"std::") != std::wstring_view::npos)
            return Claim::Certain;

        if (signs.opener || signs.statement)
            return Claim::Likely;

        const bool joins = joinsNames(text, L"::", L"") || joinsNames(text, L"->", L")]");
        return (isSourceShaped(signs) && joins) ? Claim::Likely : Claim::None;
    }


    //-------------------------------------------------------------------------
    // Pascal


    namespace
    {
        namespace PascalMode
        {
            constexpr std::uint8_t braceDirective = Mode::firstLanguageMode;
            constexpr std::uint8_t parenDirective = Mode::firstLanguageMode + 1;
        }

        // A directive that is an everyday name as well - a property's accessors, a method's
        // message - and a keyword only past a declaration's signature. Sorted, and lower case.
        constexpr auto k_pascalContextual = std::to_array<std::wstring_view>({
            L"default", L"dispid", L"implements", L"index", L"message", L"name", L"nodefault",
            L"read", L"stored", L"write"
        });
        static_assert(std::ranges::is_sorted(k_pascalContextual));

        // The words a declaration line opens with. Sorted, and lower case.
        constexpr auto k_pascalDeclarers = std::to_array<std::wstring_view>({
            L"class", L"constructor", L"destructor", L"function", L"operator", L"procedure",
            L"property"
        });
        static_assert(std::ranges::is_sorted(k_pascalDeclarers));

        // The header of a source file: one of these, a name, and a semicolon closing the line.
        constexpr auto k_pascalHeaders = std::to_array<std::wstring_view>({
            L"library", L"program", L"unit"
        });

        constexpr auto k_pascalBlockWords = std::to_array<std::wstring_view>({
            L"begin", L"end"
        });

        constexpr auto k_pascalRoutineWords = std::to_array<std::wstring_view>({
            L"function", L"procedure"
        });

        using DigitTest = bool (*)(wchar_t);

        [[nodiscard]] bool isHexDigit(const wchar_t value)
        {
            return std::iswxdigit(value) != 0;
        }

        [[nodiscard]] bool isBinaryDigit(const wchar_t value)
        {
            return value == L'0' || value == L'1';
        }

        // From index to the end of the digits the test accepts from `digits` on, as one token.
        void takeDigits(Scan& scan, const std::size_t digits, const DigitTest isDigitOf,
            const TokenKind kind)
        {
            std::size_t end = digits;
            while (end != scan.line.size() && isDigitOf(scan.line[end]))
                ++end;

            scan.emit(scan.index, end, kind);
            scan.index = end;
        }

        // A string: quoted with single quotes, closed on its own line, and carrying a quote as
        // two - drawn as an escape, the way a backslash and what it escapes are elsewhere.
        void takePascalString(Scan& scan)
        {
            std::size_t runStart = scan.index;
            std::size_t at = scan.index + 1;
            while (at < scan.line.size())
            {
                if (scan.line[at] != L'\'')
                {
                    ++at;
                    continue;
                }
                if (at + 1 < scan.line.size() && scan.line[at + 1] == L'\'')
                {
                    scan.emit(runStart, at, TokenKind::String);
                    scan.emit(at, at + 2, TokenKind::Escape);
                    at += 2;
                    runStart = at;
                    continue;
                }
                ++at;
                break;
            }
            scan.emit(runStart, at, TokenKind::String);
            scan.index = at;
        }

        // A character by its code - #13, #$0D - which a string may abut on either side.
        void takeCharCode(Scan& scan)
        {
            if (scan.peek() == L'$')
                takeDigits(scan, scan.index + 2, isHexDigit, TokenKind::Escape);
            else
                takeDigits(scan, scan.index + 1, isDigit, TokenKind::Escape);
        }

        // Whether the line opens with a word a declaration does.
        [[nodiscard]] bool opensDeclaration(const Scan& scan)
        {
            const std::size_t start = scan.pastBlanks(0);
            const std::size_t end = endOfName(scan.line, start, scan.language);
            return isListedNoCase(k_pascalDeclarers, scan.line.substr(start, end - start));
        }

        // Whether a colon or a semicolon outside every bracket stands before index on the line -
        // past a routine's parameters and a property's own name, where the directives begin.
        [[nodiscard]] bool pastSignature(const Scan& scan)
        {
            int depth = 0;
            for (std::size_t i = 0; i != scan.index; ++i)
            {
                switch (scan.line[i])
                {
                    case L'(':
                    case L'[':
                        ++depth;
                        break;
                    case L')':
                    case L']':
                        --depth;
                        break;
                    case L':':
                    case L';':
                        if (depth == 0)
                            return true;
                        break;
                    default:
                        break;
                }
            }
            return false;
        }

        // A directive that is also a name, as a keyword where it stands as a directive. Anywhere
        // else it is left to the tables, which list it nowhere.
        [[nodiscard]] bool takeContextualDirective(Scan& scan)
        {
            const std::size_t end = endOfName(scan.line, scan.index, scan.language);
            const std::wstring_view word = scan.line.substr(scan.index, end - scan.index);
            if (!isListedNoCase(k_pascalContextual, word))
                return false;
            if (!opensDeclaration(scan) || !pastSignature(scan))
                return false;

            scan.emit(scan.index, end, TokenKind::Keyword);
            scan.index = end;
            return true;
        }

        // What a reading of the lines finds of the language's shape.
        struct PascalSigns
        {
            bool header{ false };      // unit, program or library: the word, a name, a semicolon
            bool directive{ false };   // a line opening a compiler directive
            bool block{ false };       // a line opening with begin or end
            bool routine{ false };     // a line opening with procedure or function
        };

        [[nodiscard]] PascalSigns readPascalSigns(const std::wstring_view text)
        {
            PascalSigns signs;
            std::size_t start = 0;
            while (start < text.size())
            {
                const std::wstring_view line = lineFrom(text, start);
                if (line.empty())
                    continue;

                if (line.back() == L';' && opensWithAnyWordNoCase(line, k_pascalHeaders))
                    signs.header = true;
                if (line.starts_with(L"{$"))
                    signs.directive = true;
                if (opensWithAnyWordNoCase(line, k_pascalBlockWords))
                    signs.block = true;
                if (opensWithAnyWordNoCase(line, k_pascalRoutineWords))
                    signs.routine = true;
            }
            return signs;
        }
    }

    bool pascalHook(Scan& scan)
    {
        switch (scan.state.mode)
        {
            case PascalMode::braceDirective:
                takeToCloser(scan, scan.index, L"}", TokenKind::Directive,
                    PascalMode::braceDirective);
                return true;
            case PascalMode::parenDirective:
                takeToCloser(scan, scan.index, L"*)", TokenKind::Directive,
                    PascalMode::parenDirective);
                return true;
            default:
                break;
        }

        const wchar_t current = scan.current();
        const wchar_t next = scan.peek();
        if (current == L'\'')
        {
            takePascalString(scan);
            return true;
        }
        if (current == L'#' && (isDigit(next) || next == L'$'))
        {
            takeCharCode(scan);
            return true;
        }
        if (current == L'$' && isHexDigit(next))
        {
            takeDigits(scan, scan.index + 1, isHexDigit, TokenKind::Number);
            return true;
        }
        if (current == L'%' && isBinaryDigit(next))
        {
            takeDigits(scan, scan.index + 1, isBinaryDigit, TokenKind::Number);
            return true;
        }

        // A compiler directive is a comment whose first character is a dollar, closed the way the
        // comment is. The comments without one are the table's.
        const std::size_t start = scan.index;
        if (current == L'{' && next == L'$')
        {
            scan.index += 2;
            takeToCloser(scan, start, L"}", TokenKind::Directive, PascalMode::braceDirective);
            return true;
        }
        if (current == L'(' && next == L'*' && scan.peek(2) == L'$')
        {
            scan.index += 3;
            takeToCloser(scan, start, L"*)", TokenKind::Directive, PascalMode::parenDirective);
            return true;
        }

        // An ampersand makes a name of a reserved word, and the name is as plain as any.
        if (current == L'&' && isNameStart(next, scan.language))
        {
            scan.index = endOfName(scan.line, start + 1, scan.language);
            return true;
        }
        if (isNameStart(current, scan.language))
            return takeContextualDirective(scan);
        return false;
    }

    // A file's header, a compiler directive, or a program closed with `end.` past a begin: no
    // other language here spells them. A block with an assignment or a routine in it is the
    // shape of a script, which a text of another language could share.
    Claim pascalDetector(const std::wstring_view text)
    {
        const PascalSigns signs = readPascalSigns(text);
        const bool closesProgram = signs.block && endsWithNoCase(trimmed(text), L"end.");
        if (signs.header || signs.directive || closesProgram)
            return Claim::Certain;

        const bool assigns = text.find(L":=") != std::wstring_view::npos;
        if (signs.block && (signs.routine || assigns))
            return Claim::Likely;
        return Claim::None;
    }


    //-------------------------------------------------------------------------
    // ClaFi


    namespace
    {
        // The characters that say what a line is on their own: the hook's cases, and what the
        // detector knows a line by.
        constexpr std::wstring_view k_claFiMarkers = L";@]|+=";

        // A key and its equals sign: a name, blanks if any, then `=`. A value closed with a
        // semicolon is a statement of another language: the format opens a comment line with
        // one and closes no value with it.
        [[nodiscard]] bool isClaFiKeyLine(const std::wstring_view line)
        {
            if (line.empty())
                return false;
            if (std::iswalpha(line.front()) == 0 && line.front() != L'_')
                return false;

            std::size_t index = 1;
            while (index < line.size() && (std::iswalnum(line[index]) != 0 || line[index] == L'_'
                || line[index] == L'.' || line[index] == L'-'))
            {
                ++index;
            }

            index = line.find_first_not_of(L" \t", index);
            if (index == std::wstring_view::npos || line[index] != L'=')
                return false;
            return !line.ends_with(L';');
        }

        // Whether a trimmed line is one the format reads: a comment, a directive, a close, a
        // continuation, a sequence item, a bare value, or a key.
        [[nodiscard]] bool isClaFiLine(const std::wstring_view line)
        {
            if (line.empty())
                return false;

            return k_claFiMarkers.find(line.front()) != std::wstring_view::npos
                || isClaFiKeyLine(line);
        }

        // Whether the first two lines with anything on them are ones the format reads, one of
        // them a key - the shape a section travels in without its header.
        [[nodiscard]] bool opensAsClaFi(const std::wstring_view text)
        {
            std::size_t seen = 0;
            bool keySeen = false;
            std::size_t start = 0;
            while (start < text.size() && seen < 2)
            {
                const std::wstring_view line = lineFrom(text, start);
                if (line.empty())
                    continue;

                if (!isClaFiLine(line))
                    return false;

                keySeen = keySeen || isClaFiKeyLine(line);
                ++seen;
            }

            return seen == 2 && keySeen;
        }

        // What stands after the marker or the equals sign, trimmed the way the reader trims it:
        // a bracket opening a section, a number, or a string - quoted with its bookends, or the
        // bare literal the format leaves to the application, which is a string all the same.
        void takeClaFiValue(Scan& scan, const std::size_t from)
        {
            const std::size_t start = scan.pastBlanks(from);
            std::size_t end = scan.line.size();
            while (end > start && isBlank(scan.line[end - 1]))
                --end;

            scan.index = scan.line.size();
            if (end == start)
                return;

            const std::wstring_view value = scan.line.substr(start, end - start);
            if (value == L"[")
            {
                scan.emit(start, end, TokenKind::Punctuation);
                return;
            }
            if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
            {
                scan.emit(start, end, TokenKind::String);
                return;
            }

            const bool signedNumber = (value.front() == L'-' || value.front() == L'+')
                && value.size() > 1 && isDigit(value[1]);
            if (isDigit(value.front()) || signedNumber)
            {
                const std::size_t digits = signedNumber ? start + 1 : start;
                if (endOfNumber(scan.line, digits) == end)
                {
                    scan.emit(start, end, TokenKind::Number);
                    return;
                }
            }
            scan.emit(start, end, TokenKind::String);
        }
    }

    // The first character of a line says what the line is, so the hook is asked at the line's
    // first character and takes the whole line from there.
    bool claFiHook(Scan& scan)
    {
        if (!scan.atLineStart())
            return false;

        const wchar_t current = scan.current();
        switch (current)
        {
            case L';':
                takeToLineEnd(scan, TokenKind::Comment);
                return true;
            case L'@':
                takeToLineEnd(scan, TokenKind::Directive);
                return true;
            case L']':
                scan.emit(scan.index, scan.index + 1, TokenKind::Punctuation);
                scan.index = scan.line.size();
                return true;
            case L'|':
            case L'+':
            case L'=':
                scan.emit(scan.index, scan.index + 1, TokenKind::Operator);
                takeClaFiValue(scan, scan.index + 1);
                return true;
            default:
                break;
        }

        // A key, its equals sign and its value. A line with no equals sign says nothing the
        // format knows, and is left plain whole.
        const std::size_t equals = scan.line.find(L'=', scan.index);
        if (equals == std::wstring_view::npos)
        {
            scan.index = scan.line.size();
            return true;
        }

        std::size_t keyEnd = equals;
        while (keyEnd > scan.index && isBlank(scan.line[keyEnd - 1]))
            --keyEnd;

        scan.emit(scan.index, keyEnd, TokenKind::Property);
        scan.emit(equals, equals + 1, TokenKind::Operator);
        takeClaFiValue(scan, equals + 1);
        return true;
    }

    // The header is the format's own word. A section without one - the shape the framework's rich
    // text travels in - is told by its first lines, which any settings file could share.
    Claim claFiDetector(const std::wstring_view text)
    {
        if (trimmed(text).starts_with(L"@ClaFi"))
            return Claim::Certain;
        if (opensAsClaFi(text))
            return Claim::Likely;
        return Claim::None;
    }


    //-------------------------------------------------------------------------
    // XML


    namespace
    {
        namespace XmlMode
        {
            constexpr std::uint8_t tag = Mode::firstLanguageMode;
            constexpr std::uint8_t cData = Mode::firstLanguageMode + 1;
            constexpr std::uint8_t instruction = Mode::firstLanguageMode + 2;
        }

        // The longest entity a document is likely to spell, past which an ampersand is text.
        constexpr std::size_t k_entityMax = 10;

        // One step inside a tag: its closer, an attribute, its equals sign, its value.
        void continueTag(Scan& scan)
        {
            const wchar_t current = scan.current();
            if (current == L'>')
            {
                scan.emit(scan.index, scan.index + 1, TokenKind::Punctuation);
                ++scan.index;
                scan.state.mode = Mode::normal;
                return;
            }
            if ((current == L'/' || current == L'?') && scan.peek() == L'>')
            {
                scan.emit(scan.index, scan.index + 2, TokenKind::Punctuation);
                scan.index += 2;
                scan.state.mode = Mode::normal;
                return;
            }
            if (current == L'=')
            {
                scan.emit(scan.index, scan.index + 1, TokenKind::Operator);
                ++scan.index;
                return;
            }
            if (current == L'"' || current == L'\'')
            {
                scan.takeQuoted(scan.index, scan.index, current, L'\0');
                return;
            }
            if (isNameStart(current, scan.language))
            {
                const std::size_t end = endOfName(scan.line, scan.index, scan.language);
                scan.emit(scan.index, end, TokenKind::Attribute);
                scan.index = end;
                return;
            }
            ++scan.index;
        }
    }

    bool xmlHook(Scan& scan)
    {
        switch (scan.state.mode)
        {
            case XmlMode::tag:
                continueTag(scan);
                return true;
            case XmlMode::cData:
                takeToCloser(scan, scan.index, L"]]>", TokenKind::String, XmlMode::cData);
                return true;
            case XmlMode::instruction:
                takeToCloser(scan, scan.index, L"?>", TokenKind::Directive, XmlMode::instruction);
                return true;
            default:
                break;
        }

        const wchar_t current = scan.current();
        if (current == L'<')
        {
            // The comment rule's, and it stands in the table.
            if (scan.startsWith(L"<!--"))
                return false;

            const std::size_t start = scan.index;
            if (scan.startsWith(L"<![CDATA["))
            {
                scan.index += 9;
                takeToCloser(scan, start, L"]]>", TokenKind::String, XmlMode::cData);
                return true;
            }
            if (scan.startsWith(L"<?"))
            {
                scan.index += 2;
                takeToCloser(scan, start, L"?>", TokenKind::Directive, XmlMode::instruction);
                return true;
            }
            if (scan.startsWith(L"<!"))
            {
                const std::size_t close = scan.line.find(L'>', start);
                const std::size_t end = close == std::wstring_view::npos
                    ? scan.line.size()
                    : close + 1;
                scan.emit(start, end, TokenKind::Directive);
                scan.index = end;
                return true;
            }

            // An element: its bracket, with the slash of a closing one, and then its name. What
            // follows inside the brackets is the tag mode's, on this line or the next.
            const std::size_t nameStart = scan.peek() == L'/' ? start + 2 : start + 1;
            scan.emit(start, nameStart, TokenKind::Punctuation);
            const std::size_t nameEnd = endOfName(scan.line, nameStart, scan.language);
            scan.emit(nameStart, nameEnd, TokenKind::Tag);
            scan.index = nameEnd;
            scan.state.mode = XmlMode::tag;
            return true;
        }
        if (current == L'&')
        {
            const std::size_t close = scan.line.find(L';', scan.index);
            if (close != std::wstring_view::npos && close - scan.index <= k_entityMax)
            {
                scan.emit(scan.index, close + 1, TokenKind::Escape);
                scan.index = close + 1;
                return true;
            }
        }
        return false;
    }

    Claim xmlDetector(const std::wstring_view text)
    {
        const std::wstring_view body = trimmed(text);
        if (body.starts_with(L"<?xml") || opensWithNoCase(body, L"<!doctype"))
            return Claim::Certain;

        // A tag or a comment: the bracket, a name character or the bang, and a close somewhere
        // after.
        const bool opensAsMarkup = body.size() > 2 && body.front() == L'<'
            && (std::iswalpha(body[1]) != 0 || body[1] == L'!')
            && body.find(L'>') != std::wstring_view::npos;
        return opensAsMarkup ? Claim::Likely : Claim::None;
    }


    //-------------------------------------------------------------------------
    // JSON


    namespace
    {
        // Whether an object opens with a key: the brace, a quoted name, a colon, blanks between.
        [[nodiscard]] bool opensWithKey(const std::wstring_view body)
        {
            std::size_t at = body.find_first_not_of(k_blanks, 1);
            if (at == std::wstring_view::npos || body[at] != L'"')
                return false;

            const std::size_t close = body.find(L'"', at + 1);
            if (close == std::wstring_view::npos)
                return false;

            at = body.find_first_not_of(k_blanks, close + 1);
            return at != std::wstring_view::npos && body[at] == L':';
        }

        // Whether nothing but blanks stands between the opening bracket and the closing one.
        [[nodiscard]] bool isEmptyBracketed(const std::wstring_view body)
        {
            return body.find_first_not_of(k_blanks, 1) == body.size() - 1;
        }

        // Whether an array opens with a value: a string, a number, an object, an array. A word
        // is left out - a bracketed word names a section in a settings file as often as it
        // spells true, false or null.
        [[nodiscard]] bool opensWithValue(const std::wstring_view body)
        {
            const std::size_t at = body.find_first_not_of(k_blanks, 1);
            if (at == std::wstring_view::npos)
                return false;

            const wchar_t value = body[at];
            return value == L'"' || value == L'-' || isDigit(value) || value == L'{'
                || value == L'[';
        }
    }

    // An object or an array whole: the opening bracket first and its close last. A key after
    // the brace is the format's own spelling. An empty object, or an array with a value first,
    // is its shape; a brace with anything else after it opens a block of source, not an object.
    Claim jsonDetector(const std::wstring_view text)
    {
        const std::wstring_view body = trimmed(text);
        if (body.empty())
            return Claim::None;

        const wchar_t first = body.front();
        const wchar_t last = body.back();
        if (first == L'{' && last == L'}')
        {
            if (opensWithKey(body))
                return Claim::Certain;
            return isEmptyBracketed(body) ? Claim::Likely : Claim::None;
        }
        if (first == L'[' && last == L']')
            return (isEmptyBracketed(body) || opensWithValue(body)) ? Claim::Likely : Claim::None;
        return Claim::None;
    }


    //-------------------------------------------------------------------------
    // Text


    Claim textDetector(std::wstring_view)
    {
        return Claim::Possible;
    }
}
