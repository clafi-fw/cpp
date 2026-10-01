module ClaFi.Core.Syntax.Completion;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.TextEngine.Types;

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
            L"variable",
            L"key",
            L"value"
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

        // Whether a character of the line stands in a comment or a string.
        [[nodiscard]] bool literalAt(const Tokens& lineTokens, const std::size_t pos)
        {
            for (const Token& token : lineTokens)
            {
                const bool literal = token.kind == TokenKind::Comment
                    || token.kind == TokenKind::String
                    || token.kind == TokenKind::Escape;
                if (literal && token.range.start <= pos && pos < token.range.end())
                    return true;
            }
            return false;
        }

        // Where the bracket at `closer` opens, a bracket inside a literal counting for none -
        // nothing where the line holds no opener for it.
        [[nodiscard]] std::optional<std::size_t> openerOf(const std::wstring_view line,
            const Tokens& lineTokens, const std::size_t closer)
        {
            if (literalAt(lineTokens, closer))
                return std::nullopt;
            std::size_t depth = 0;
            for (std::size_t pos = closer + 1; pos != 0;)
            {
                --pos;
                if (literalAt(lineTokens, pos))
                    continue;
                const wchar_t value = line[pos];
                if (value == L')' || value == L']')
                {
                    ++depth;
                }
                else if (value == L'(' || value == L'[')
                {
                    --depth;
                    if (depth == 0)
                        return pos;
                }
            }
            return std::nullopt;
        }

        // The class of that name, under the language's case rule - none for no name.
        [[nodiscard]] const CompletionEntry* classNamed(const Language& language,
            const CompletionEntries& entries, const std::wstring_view name)
        {
            if (name.empty())
                return nullptr;
            for (const CompletionEntry& entry : entries)
            {
                if (entry.kind == CompletionKind::Class
                    && completionNamesEqual(language, entry.name, name))
                    return &entry;
            }
            return nullptr;
        }

        // The type a link answers. A routine and a property answer theirs whatever brackets
        // follow - a call, an indexed property's element - and anything else only with none,
        // since what indexing it answers is stated nowhere.
        [[nodiscard]] std::wstring_view typeOfLink(const CompletionEntry& entry,
            const bool bracketed)
        {
            const bool answersType = entry.kind == CompletionKind::Function
                || entry.kind == CompletionKind::Procedure
                || entry.kind == CompletionKind::Property;
            if (bracketed && !answersType)
                return {};
            return entry.type;
        }

        // The class and its parents, the class first and each once.
        [[nodiscard]] CompletionClasses lineageOf(const Language& language,
            const CompletionEntries& entries, const CompletionEntry& type)
        {
            CompletionClasses lineage{ &type };
            while (true)
            {
                const CompletionEntry* parent =
                    classNamed(language, entries, lineage.back()->parent);
                // A class named again would walk the same classes for ever.
                if (!parent || std::ranges::find(lineage, parent) != lineage.end())
                    return lineage;
                lineage.push_back(parent);
            }
        }

        // The member of that name the lineage states nearest its first class.
        [[nodiscard]] const CompletionEntry* memberNamed(const Language& language,
            const CompletionClasses& lineage, const std::wstring_view name)
        {
            for (const CompletionEntry* owner : lineage)
            {
                for (const CompletionEntries* members : { &owner->methods, &owner->properties })
                {
                    for (const CompletionEntry& member : *members)
                    {
                        if (completionNamesEqual(language, member.name, name))
                            return &member;
                    }
                }
            }
            return nullptr;
        }

        // The declaration of that name in force at the caret, the innermost of several.
        [[nodiscard]] const Declaration* declarationNamed(const Language& language,
            const Declarations& declarations, const std::wstring_view name,
            const std::size_t caret)
        {
            const Declaration* found = nullptr;
            for (const Declaration& declaration : declarations)
            {
                const TextRange& scope = declaration.scope;
                const bool inForce = scope.start <= caret && caret <= scope.end();
                if (!inForce || !completionNamesEqual(language, declaration.entry.name, name))
                    continue;
                if (!found || declaration.depth > found->depth)
                    found = &declaration;
            }
            return found;
        }

        // The links ending right before `end` on the line, root first: a name, a call's or an
        // index's brackets after it stepped over, and a dot before it linking the name before
        // that. Empty where what ends there is not a name.
        [[nodiscard]] CompletionSubject subjectEndingAt(const Language& language,
            const std::wstring_view line, const Tokens& lineTokens, const std::size_t end)
        {
            CompletionSubject subject;
            std::size_t linkEnd = end;
            while (true)
            {
                // A call's or an index's brackets stand between a link's name and its dot.
                CompletionLink link;
                std::size_t nameEnd = linkEnd;
                while (nameEnd != 0 && (line[nameEnd - 1] == L')' || line[nameEnd - 1] == L']'))
                {
                    const std::optional<std::size_t> opener =
                        openerOf(line, lineTokens, nameEnd - 1);
                    if (!opener)
                        return {};
                    nameEnd = *opener;
                    link.bracketed = true;
                }
                std::size_t start = nameEnd;
                while (start != 0 && isNameChar(line[start - 1], language))
                    --start;
                if (start == nameEnd || !isNameStart(line[start], language))
                    return {};
                link.name = line.substr(start, nameEnd - start);
                subject.push_back(link);
                // A dot before the name links it to the name before that; a range's two dots do
                // not.
                if (start == 0 || line[start - 1] != L'.')
                    break;
                if (start >= 2 && line[start - 2] == L'.')
                    break;
                linkEnd = start - 1;
            }
            std::ranges::reverse(subject);
            return subject;
        }

        // Whether the word stands in one of the language's tables, under its case rule.
        [[nodiscard]] bool listedWord(const Language& language, const Words table,
            const std::wstring_view word)
        {
            return language.ignoreCase ? isListedNoCase(table, word) : isListed(table, word);
        }

        // Whether the word before the name starting there opens a routine's header, so that
        // the name's brackets open no call.
        [[nodiscard]] bool headerBefore(const Language& language, const std::wstring_view line,
            const std::size_t nameStart)
        {
            std::size_t end = nameStart;
            while (end != 0 && isBlank(line[end - 1]))
                --end;
            std::size_t start = end;
            while (start != 0 && isNameChar(line[start - 1], language))
                --start;
            if (start == end)
                return false;
            return listedWord(language, language.routineWords, line.substr(start, end - start));
        }

        // The class the subject's root stands for: a name the text declares, by its type, else
        // one the host offers - a class naming itself, cast or not.
        [[nodiscard]] const CompletionEntry* rootClass(const Language& language,
            const CompletionEntries& entries, const Declarations& declarations,
            const CompletionLink& root, const std::size_t caret)
        {
            const Declaration* declared =
                declarationNamed(language, declarations, root.name, caret);
            if (declared)
                return classNamed(language, entries, typeOfLink(declared->entry, root.bracketed));
            for (const CompletionEntry& entry : entries)
            {
                if (!completionNamesEqual(language, entry.name, root.name))
                    continue;
                if (entry.kind == CompletionKind::Class)
                    return &entry;
                return classNamed(language, entries, typeOfLink(entry, root.bracketed));
            }
            return nullptr;
        }

        // What a title line spaces its parts with, a no-break space among them.
        constexpr std::wstring_view k_titleBlanks = L" \t\u00A0";

        [[nodiscard]] std::size_t pastTitleBlanks(const std::wstring_view line, std::size_t pos,
            const std::size_t end)
        {
            while (pos < end && k_titleBlanks.find(line[pos]) != std::wstring_view::npos)
                ++pos;
            return pos;
        }

        [[nodiscard]] std::size_t beforeTitleBlanks(const std::wstring_view line,
            const std::size_t start, std::size_t end)
        {
            while (end > start && k_titleBlanks.find(line[end - 1]) != std::wstring_view::npos)
                --end;
            return end;
        }

        // Whether the line holds nothing but comments and blanks, as a title block's lines do.
        [[nodiscard]] bool commentsOnly(const SourceLines& lines, const std::size_t line)
        {
            const std::wstring_view text = lines.text(line);
            std::size_t pos = 0;
            for (const Token& token : lines.tokens(line))
            {
                const bool blanksBefore =
                    pastTitleBlanks(text, pos, token.range.start) == token.range.start;
                if (!blanksBefore || token.kind != TokenKind::Comment)
                    return false;
                pos = token.range.end();
            }
            return pastTitleBlanks(text, pos, text.size()) == text.size();
        }

        // The key a line of the title block states - empty for a line that states none.
        [[nodiscard]] std::wstring_view titleKeyOf(const Language& language,
            const TitleBlock& block, const std::wstring_view line)
        {
            if (!line.starts_with(block.prefix))
                return {};
            const std::size_t start = block.prefix.size();
            return line.substr(start, endOfName(line, start, language) - start);
        }

        [[nodiscard]] const TitleKey* titleKeyNamed(const Language& language,
            const TitleBlock& block, const std::wstring_view name)
        {
            for (const TitleKey& key : block.keys)
            {
                if (completionNamesEqual(language, key.entry.name, name))
                    return &key;
            }
            return nullptr;
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

    CompletionSubject completionSubject(const Language& language, const std::wstring_view line,
        const Tokens& lineTokens, const CompletionPlace& place)
    {
        if (!place.member)
            return {};
        return subjectEndingAt(language, line, lineTokens, place.wordStart - 1);
    }

    CompletionClasses completionMemberClasses(const Language& language,
        const CompletionEntries& entries, const Declarations& declarations,
        const CompletionSubject& subject, const std::size_t caret)
    {
        if (subject.empty())
            return {};
        const CompletionEntry* current =
            rootClass(language, entries, declarations, subject.front(), caret);
        // Every link after the root is a member of the class the link before it answers.
        for (const CompletionLink& link : subject | std::views::drop(1))
        {
            if (!current)
                return {};
            const CompletionEntry* member =
                memberNamed(language, lineageOf(language, entries, *current), link.name);
            current = member
                ? classNamed(language, entries, typeOfLink(*member, link.bracketed))
                : nullptr;
        }
        if (!current)
            return {};
        return lineageOf(language, entries, *current);
    }

    std::optional<CompletionCall> completionCall(const Language& language,
        const SourceLines& lines, const std::size_t line, const std::size_t caret)
    {
        if (line >= lines.count())
            return std::nullopt;
        std::size_t depth = 0;
        std::size_t commas = 0;
        std::size_t at = line;
        std::wstring_view text = lines.text(at);
        std::size_t pos = std::min(caret, text.size());
        while (true)
        {
            const Tokens& tokens = lines.tokens(at);
            while (pos != 0)
            {
                --pos;
                if (literalAt(tokens, pos))
                    continue;
                const wchar_t value = text[pos];
                if (value == L')' || value == L']')
                {
                    ++depth;
                }
                else if (value == L'(' || value == L'[')
                {
                    if (depth != 0)
                    {
                        --depth;
                        continue;
                    }
                    // The bracket left open. A name before it is what is called; anything else
                    // - an operator, a keyword such as if or not - groups an expression, whose
                    // commas are its own and count for nothing.
                    CompletionSubject callee = subjectEndingAt(language, text, tokens, pos);
                    const bool keyword = callee.size() == 1
                        && listedWord(language, language.keywords, callee.front().name);
                    if (callee.empty() || keyword)
                    {
                        commas = 0;
                        continue;
                    }
                    const std::size_t nameStart =
                        static_cast<std::size_t>(callee.front().name.data() - text.data());
                    if (headerBefore(language, text, nameStart))
                        return std::nullopt;
                    return CompletionCall{
                        .line = at,
                        .bracket = pos,
                        .argument = commas,
                        .callee = std::move(callee),
                    };
                }
                else if (depth == 0 && value == L',')
                {
                    ++commas;
                }
                else if (depth == 0 && (value == L';' || value == L'{' || value == L'}'))
                {
                    // The statement's start, with no bracket left open before it.
                    return std::nullopt;
                }
            }
            if (at == 0)
                return std::nullopt;
            --at;
            text = lines.text(at);
            pos = text.size();
        }
    }

    const CompletionEntry* completionCallee(const Language& language,
        const CompletionEntries& entries, const Declarations& declarations,
        const CompletionSubject& callee, const std::size_t caret)
    {
        if (callee.empty())
            return nullptr;
        const std::wstring_view name = callee.back().name;
        if (callee.size() == 1)
        {
            const Declaration* declared = declarationNamed(language, declarations, name, caret);
            if (declared)
                return &declared->entry;
            for (const CompletionEntry& entry : entries)
            {
                if (completionNamesEqual(language, entry.name, name))
                    return &entry;
            }
            return nullptr;
        }
        // A member: the links before it are its subject, and it is read off those classes.
        const CompletionSubject subject(callee.begin(), callee.end() - 1);
        const CompletionClasses classes =
            completionMemberClasses(language, entries, declarations, subject, caret);
        if (classes.empty())
            return nullptr;
        return memberNamed(language, classes, name);
    }

    std::wstring_view TitlePlace::typed(const std::wstring_view line) const
    {
        return line.substr(wordStart, caret - wordStart);
    }

    std::optional<TitlePlace> completionTitlePlace(const Language& language,
        const TitleBlock& block, const SourceLines& lines, const std::size_t line,
        std::size_t caret)
    {
        if (block.prefix.empty() || line >= lines.count())
            return std::nullopt;
        const std::wstring_view text = lines.text(line);
        caret = std::min(caret, text.size());
        if (!text.starts_with(block.prefix) || caret < block.prefix.size())
            return std::nullopt;
        // The block runs to the first line of source; a key's line past that states nothing.
        std::size_t blockEnd = 0;
        while (blockEnd != lines.count() && commentsOnly(lines, blockEnd))
            ++blockEnd;
        if (line >= blockEnd)
            return std::nullopt;

        CompletionNames stated;
        const std::size_t keyStart = block.prefix.size();
        const std::size_t keyEnd = endOfName(text, keyStart, language);
        if (caret <= keyEnd)
        {
            for (std::size_t other = 0; other != blockEnd; ++other)
            {
                const std::wstring_view key = titleKeyOf(language, block, lines.text(other));
                if (other != line && !key.empty())
                    stated.push_back(key);
            }
            return TitlePlace{
                .wordStart = keyStart,
                .caret = caret,
                .stated = std::move(stated),
            };
        }

        // A value follows the equals sign, and only a key that lists its values offers them.
        const std::size_t sign = pastTitleBlanks(text, keyEnd, caret);
        if (sign == caret || text[sign] != L'=')
            return std::nullopt;
        const TitleKey* key =
            titleKeyNamed(language, block, text.substr(keyStart, keyEnd - keyStart));
        if (!key || key->values.empty())
            return std::nullopt;
        // A list's value is the one between the commas around the caret; the rest are stated.
        std::size_t valueStart = sign + 1;
        if (key->list)
        {
            for (std::size_t itemStart = sign + 1; itemStart <= text.size();)
            {
                std::size_t itemEnd = text.find(L',', itemStart);
                if (itemEnd == std::wstring_view::npos)
                    itemEnd = text.size();
                if (itemStart <= caret && caret <= itemEnd)
                {
                    valueStart = itemStart;
                }
                else
                {
                    const std::size_t from = pastTitleBlanks(text, itemStart, itemEnd);
                    const std::size_t to = beforeTitleBlanks(text, from, itemEnd);
                    if (from != to)
                        stated.push_back(text.substr(from, to - from));
                }
                itemStart = itemEnd + 1;
            }
        }
        return TitlePlace{
            .wordStart = pastTitleBlanks(text, valueStart, caret),
            .caret = caret,
            .key = key,
            .stated = std::move(stated),
        };
    }
}
