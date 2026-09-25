module ClaFi.Core.Syntax.Lexer;

import ClaFi.Core.Syntax.Types;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    namespace
    {
        [[nodiscard]] bool holds(std::wstring_view set, wchar_t value)
        {
            return set.find(value) != std::wstring_view::npos;
        }

        // The rest of a block comment left open by the line before, or the whole line where the
        // close is not on it either.
        void continueBlockComment(Scan& scan)
        {
            const CommentRule& rule = scan.language.comments[scan.state.aux];
            const std::size_t close = scan.line.find(rule.close, scan.index);
            if (close == std::wstring_view::npos)
            {
                scan.emit(scan.index, scan.line.size(), TokenKind::Comment);
                scan.index = scan.line.size();
                return;
            }
            const std::size_t end = close + rule.close.size();
            scan.emit(scan.index, end, TokenKind::Comment);
            scan.index = end;
            scan.state.mode = Mode::normal;
            scan.state.aux = 0;
        }

        [[nodiscard]] bool takeComment(Scan& scan)
        {
            const std::span<const CommentRule> rules = scan.language.comments;
            for (std::size_t i = 0; i != rules.size(); ++i)
            {
                const CommentRule& rule = rules[i];
                if (!scan.startsWith(rule.open))
                    continue;

                if (rule.close.empty())
                {
                    scan.emit(scan.index, scan.line.size(), TokenKind::Comment);
                    scan.index = scan.line.size();
                    return true;
                }

                // Searched past the opener, so an opener whose tail spells the closer - /*/ - is
                // still open.
                const std::size_t close = scan.line.find(rule.close, scan.index + rule.open.size());
                if (close == std::wstring_view::npos)
                {
                    scan.emit(scan.index, scan.line.size(), TokenKind::Comment);
                    scan.index = scan.line.size();
                    scan.state.mode = Mode::blockComment;
                    scan.state.aux = static_cast<std::uint8_t>(i);
                    return true;
                }

                const std::size_t end = close + rule.close.size();
                scan.emit(scan.index, end, TokenKind::Comment);
                scan.index = end;
                return true;
            }
            return false;
        }

        [[nodiscard]] bool takeString(Scan& scan)
        {
            const wchar_t current = scan.current();
            for (const StringRule& rule : scan.language.strings)
            {
                if (current != rule.open)
                    continue;

                const std::size_t firstToken = scan.tokens ? scan.tokens->size() : 0;
                scan.takeQuoted(scan.index, scan.index, rule.close, rule.escape);

                // A key is a string with a colon after it, and the string's own runs are what
                // carry the answer - the escapes inside it stay escapes.
                if (scan.language.keyBeforeColon && scan.tokens
                    && scan.peek(scan.pastBlanks(scan.index) - scan.index) == L':')
                {
                    for (std::size_t i = firstToken; i != scan.tokens->size(); ++i)
                    {
                        Token& token = (*scan.tokens)[i];
                        if (token.kind == TokenKind::String)
                            token.kind = TokenKind::Property;
                    }
                }
                return true;
            }
            return false;
        }

        [[nodiscard]] bool startsNumber(const Scan& scan)
        {
            const wchar_t current = scan.current();
            return isDigit(current) || (current == L'.' && isDigit(scan.peek()));
        }

        void takeNumber(Scan& scan)
        {
            const std::size_t end = endOfNumber(scan.line, scan.index);
            scan.emit(scan.index, end, TokenKind::Number);
            scan.index = end;
        }

        [[nodiscard]] wchar_t foldCase(const wchar_t value)
        {
            return static_cast<wchar_t>(std::towlower(value));
        }

        // Delphi's prefixes: a class or record, an interface, an exception, a pointer type.
        [[nodiscard]] bool isPrefixedCapital(const std::wstring_view word)
        {
            return word.size() > 1 && holds(L"TIEP", word.front()) && std::iswupper(word[1]) != 0;
        }

        void takeName(Scan& scan)
        {
            const Language& language = scan.language;
            const std::size_t end = endOfName(scan.line, scan.index, language);
            const std::wstring_view word = scan.line.substr(scan.index, end - scan.index);
            const auto listed = [&](const Words words){
                return language.ignoreCase ? isListedNoCase(words, word) : isListed(words, word);
            };

            // A prefixed type standing before a parenthesis is a cast, so that rule outranks the
            // call rule; a leading capital alone says less, and does not.
            TokenKind kind = TokenKind::Plain;
            if (listed(language.keywords))
                kind = TokenKind::Keyword;
            else if (listed(language.types))
                kind = TokenKind::Type;
            else if (listed(language.constants))
                kind = TokenKind::Constant;
            else if (language.typeRule == TypeRule::PrefixedCapital && isPrefixedCapital(word))
                kind = TokenKind::Type;
            else if (language.callIsFunction && scan.line.substr(scan.pastBlanks(end)).starts_with(L'('))
                kind = TokenKind::Function;
            else if (language.typeRule == TypeRule::LeadingCapital && std::iswupper(word.front()))
                kind = TokenKind::Type;

            // A name nothing claims joins the plain run, which is what leaves it without a token.
            if (kind != TokenKind::Plain)
                scan.emit(scan.index, end, kind);
            scan.index = end;
        }

        // A run of operator characters is one token: -> and <<= read as one and colouring them
        // apart from each other changes nothing on screen.
        void takeOperators(Scan& scan)
        {
            std::size_t end = scan.index;
            while (end != scan.line.size() && holds(scan.language.operators, scan.line[end]))
                ++end;

            scan.emit(scan.index, end, TokenKind::Operator);
            scan.index = end;
        }

        // Hands the position to the language's hook and holds it to its contract: a hook that
        // answers true has moved past what it took, or the walk would never end.
        [[nodiscard]] bool takeByHook(Scan& scan)
        {
            const Hook hook = scan.language.hook;
            if (!hook)
                return false;

            const std::size_t before = scan.index;
            if (!hook(scan))
                return false;

            if (scan.index == before)
                unreachable("A syntax hook claimed a position without moving past it.");
            return true;
        }
    }

    LineState lexLine(const Language& language, const std::wstring_view line, const LineState state,
        StateStrings& strings, Tokens* tokens)
    {
        Scan scan{ language, line, strings, tokens, 0, state };
        while (scan.index < line.size())
        {
            if (scan.state.mode == Mode::blockComment)
            {
                continueBlockComment(scan);
                continue;
            }
            if (scan.state.mode >= Mode::firstLanguageMode)
            {
                if (!takeByHook(scan))
                    unreachable("A line stands in a language mode its hook does not carry.");
                continue;
            }

            const wchar_t current = scan.current();
            if (isBlank(current))
            {
                ++scan.index;
                continue;
            }
            if (takeByHook(scan))
                continue;
            if (takeComment(scan))
                continue;
            if (takeString(scan))
                continue;
            if (language.numbers && startsNumber(scan))
            {
                takeNumber(scan);
                continue;
            }
            if (isNameStart(current, language))
            {
                takeName(scan);
                continue;
            }
            if (holds(language.operators, current))
            {
                takeOperators(scan);
                continue;
            }
            if (holds(language.punctuation, current))
            {
                scan.emit(scan.index, scan.index + 1, TokenKind::Punctuation);
                ++scan.index;
                continue;
            }
            ++scan.index;
        }
        return scan.state;
    }

    Language detect(const std::span<const Language> languages, const std::wstring_view text)
    {
        const Language* found = nullptr;
        Claim surest = Claim::None;
        for (const Language& language : languages)
        {
            if (!language.detect)
                continue;

            const Claim claim = language.detect(text);
            if (claim <= surest)
                continue;

            found = &language;
            surest = claim;
            if (surest == Claim::Certain)
                break;
        }

        if (!found)
            return {};
        return *found;
    }

    bool isListed(const Words words, const std::wstring_view word)
    {
        return std::ranges::binary_search(words, word);
    }

    // A lower-case table sorted by code unit is sorted under the folded comparison as well, which
    // is what lets the bisection read it with either.
    bool isListedNoCase(const Words words, const std::wstring_view word)
    {
        const auto less = [](const std::wstring_view left, const std::wstring_view right){
            return std::ranges::lexicographical_compare(left, right, std::ranges::less{}, foldCase,
                foldCase);
        };
        return std::ranges::binary_search(words, word, less);
    }

    bool isBlank(const wchar_t value)
    {
        return value == L' ' || value == L'\t';
    }

    bool isDigit(const wchar_t value)
    {
        return value >= L'0' && value <= L'9';
    }

    bool isNameStart(const wchar_t value, const Language& language)
    {
        return value == L'_' || std::iswalpha(value) != 0 || holds(language.nameChars, value);
    }

    bool isNameChar(const wchar_t value, const Language& language)
    {
        return value == L'_' || std::iswalnum(value) != 0 || holds(language.nameChars, value);
    }

    std::size_t endOfName(const std::wstring_view line, const std::size_t start,
        const Language& language)
    {
        std::size_t end = start;
        while (end != line.size() && isNameChar(line[end], language))
            ++end;
        return end;
    }

    std::size_t endOfNumber(const std::wstring_view line, const std::size_t start)
    {
        // A sign belongs to the number after an exponent letter alone, and which letter that is
        // depends on the radix: 0xE+1 is a hexadecimal E followed by a plus.
        const bool hexadecimal = line[start] == L'0' && (line.substr(start + 1).starts_with(L'x')
            || line.substr(start + 1).starts_with(L'X'));
        std::size_t end = start;
        while (end != line.size())
        {
            const wchar_t current = line[end];
            if (current == L'.' && end + 1 != line.size() && line[end + 1] == L'.')
                break;
            if (std::iswalnum(current) != 0 || current == L'.' || current == L'\'')
            {
                ++end;
                continue;
            }
            if ((current == L'+' || current == L'-') && end != start)
            {
                const wchar_t before = line[end - 1];
                const bool exponent = hexadecimal
                    ? (before == L'p' || before == L'P')
                    : (before == L'e' || before == L'E');
                if (exponent)
                {
                    ++end;
                    continue;
                }
            }
            break;
        }
        return end;
    }


    //-------------------------------------------------------------------------


    std::uint16_t StateStrings::idOf(const std::wstring_view value)
    {
        for (std::size_t i = 0; i != m_strings.size(); ++i)
        {
            if (m_strings[i] == value)
                return static_cast<std::uint16_t>(i + 1);
        }
        m_strings.emplace_back(value);
        return static_cast<std::uint16_t>(m_strings.size());
    }

    std::wstring_view StateStrings::stringOf(const std::uint16_t id) const
    {
        if (id == 0 || id > m_strings.size())
            unreachable("A line state names a string its table does not hold.");
        return m_strings[id - 1];
    }

    bool Scan::atLineStart() const
    {
        return line.find_first_not_of(L" \t") == index;
    }

    bool Scan::startsWith(const std::wstring_view prefix) const
    {
        return line.substr(index).starts_with(prefix);
    }

    wchar_t Scan::peek(const std::size_t ahead) const
    {
        const std::size_t at = index + ahead;
        return at < line.size() ? line[at] : L'\0';
    }

    std::size_t Scan::pastBlanks(const std::size_t from) const
    {
        std::size_t at = from;
        while (at < line.size() && isBlank(line[at]))
            ++at;
        return at;
    }

    void Scan::emit(const std::size_t start, const std::size_t end, const TokenKind kind)
    {
        if (tokens && end > start)
            tokens->push_back({ { start, end - start }, kind });
    }

    void Scan::takeQuoted(const std::size_t start, const std::size_t quote, const wchar_t close,
        const wchar_t escape)
    {
        std::size_t runStart = start;
        std::size_t at = quote + 1;
        while (at < line.size())
        {
            const wchar_t current = line[at];
            if (escape != L'\0' && current == escape && at + 1 < line.size())
            {
                emit(runStart, at, TokenKind::String);
                emit(at, at + 2, TokenKind::Escape);
                at += 2;
                runStart = at;
                continue;
            }
            ++at;
            if (current == close)
                break;
        }
        emit(runStart, at, TokenKind::String);
        index = at;
    }
}
