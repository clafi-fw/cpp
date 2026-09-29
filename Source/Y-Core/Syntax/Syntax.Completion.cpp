module ClaFi.Core.Syntax.Completion;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    namespace
    {
        // In the enum's order.
        constexpr std::array<std::wstring_view, k_completionKindCount> k_kindNames = {
            L"keyword",
            L"type",
            L"class",
            L"function",
            L"procedure",
            L"property",
            L"field",
            L"constant",
            L"variable"
        };

        [[nodiscard]] wchar_t folded(const Language& language, const wchar_t value)
        {
            return language.ignoreCase ? static_cast<wchar_t>(std::towlower(value)) : value;
        }

        // Whether what ends right before `dot` is something a member is read off - a name that
        // is not a number, or a call's or an index's closing bracket.
        [[nodiscard]] bool memberSubjectBefore(const std::wstring_view line, const std::size_t dot,
            const Language& language)
        {
            if (dot == 0)
                return false;
            const wchar_t last = line[dot - 1];
            if (last == L')' || last == L']')
                return true;
            std::size_t start = dot;
            while (start != 0 && isNameChar(line[start - 1], language))
                --start;
            return start != dot && isNameStart(line[start], language);
        }
    }

    std::wstring_view completionKindName(const CompletionKind kind)
    {
        return k_kindNames[static_cast<std::size_t>(kind)];
    }

    std::optional<CompletionKind> completionKindOf(const std::wstring_view word)
    {
        const auto foldCase = [](const wchar_t value){
            return static_cast<wchar_t>(std::towlower(value));
        };
        for (std::size_t i = 0; i != k_kindNames.size(); ++i)
        {
            if (std::ranges::equal(k_kindNames[i], word, std::ranges::equal_to{}, std::identity{},
                foldCase))
                return static_cast<CompletionKind>(i);
        }
        return std::nullopt;
    }

    std::wstring_view CompletionPlace::typed(const std::wstring_view line) const
    {
        return line.substr(wordStart, caret - wordStart);
    }

    CompletionPlace completionPlace(const Language& language, const std::wstring_view line,
        std::size_t caret)
    {
        caret = std::min(caret, line.size());
        std::size_t start = caret;
        while (start != 0 && isNameChar(line[start - 1], language))
            --start;
        // A run opening on a digit is a number, and a number is not a name being typed.
        if (start != caret && !isNameStart(line[start], language))
            start = caret;
        CompletionPlace place = {
            .wordStart = start,
            .caret = caret,
        };
        place.member = start != 0 && line[start - 1] == L'.'
            && memberSubjectBefore(line, start - 1, language);
        return place;
    }

    bool completionMatches(const Language& language, const std::wstring_view name,
        const std::wstring_view typed)
    {
        if (name.size() < typed.size())
            return false;
        for (std::size_t i = 0; i != typed.size(); ++i)
        {
            if (folded(language, name[i]) != folded(language, typed[i]))
                return false;
        }
        return true;
    }

    bool completionNameLess(const Language& language, const std::wstring_view left,
        const std::wstring_view right)
    {
        const std::size_t common = std::min(left.size(), right.size());
        for (std::size_t i = 0; i != common; ++i)
        {
            const wchar_t leftChar = folded(language, left[i]);
            const wchar_t rightChar = folded(language, right[i]);
            if (leftChar != rightChar)
                return leftChar < rightChar;
        }
        return left.size() < right.size();
    }

    bool completionNamesEqual(const Language& language, const std::wstring_view left,
        const std::wstring_view right)
    {
        return left.size() == right.size() && completionMatches(language, left, right);
    }

    bool completionBlocked(const Tokens& lineTokens, const std::size_t caret)
    {
        for (const Token& token : lineTokens)
        {
            const bool blocking = token.kind == TokenKind::Comment
                || token.kind == TokenKind::String
                || token.kind == TokenKind::Escape;
            if (!blocking)
                continue;
            // Inside the token, or right at its end: a name typed against a closing quote or a
            // comment's end is not a name the source would read.
            if (token.range.start < caret && caret <= token.range.end())
                return true;
        }
        return false;
    }
}
