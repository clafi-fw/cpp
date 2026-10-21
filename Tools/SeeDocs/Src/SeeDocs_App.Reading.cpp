module SeeDocs_App.Reading;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        constexpr std::size_t k_tabColumns = 4;
        constexpr std::size_t k_separatorRun = 3;   // the marks in a row a separator takes
        constexpr std::wstring_view k_separatorMarks = L"-=*~#_+";
        constexpr std::wstring_view k_todo = L"TODO";
        constexpr std::wstring_view k_codeFence = L"    ";   // what sets a line of code in
        constexpr std::wstring_view k_lineComment = L"//";
        constexpr std::wstring_view k_cppBlock = L"/*";
        constexpr std::wstring_view k_cppBlockEnd = L"*/";
        constexpr std::wstring_view k_pascalParen = L"(*";
        constexpr std::wstring_view k_pascalParenEnd = L"*)";
        constexpr std::wstring_view k_pascalBrace = L"{";
        constexpr std::wstring_view k_pascalBraceEnd = L"}";
        constexpr std::wstring_view k_summaryOpener = L"<summary>";
        constexpr std::wstring_view k_summaryCloser = L"</summary>";
        constexpr auto k_briefCommands = std::to_array<std::wstring_view>({
            L"@brief", L"\\brief"
        });
        // The tags of a C# or Delphi doc comment, each of which starts a part of the comment.
        constexpr auto k_docTags = std::to_array<std::wstring_view>({
            L"summary", L"remarks", L"returns", L"param", L"typeparam", L"exception", L"example",
            L"code", L"para", L"value", L"see", L"seealso", L"list", L"item", L"note"
        });

        [[nodiscard]] bool isBlankChar(const wchar_t value)
        {
            return value == L' ' || value == L'\t';
        }


        [[nodiscard]] std::wstring_view trimmedText(std::wstring_view text)
        {
            while (!text.empty() && isBlankChar(text.front()))
                text.remove_prefix(1);
            while (!text.empty() && isBlankChar(text.back()))
                text.remove_suffix(1);
            return text;
        }

        [[nodiscard]] std::wstring_view rightTrimmed(std::wstring_view text)
        {
            while (!text.empty() && (isBlankChar(text.back()) || text.back() == L'\r'))
                text.remove_suffix(1);
            return text;
        }

        [[nodiscard]] bool startsWithLowered(const std::wstring_view text,
            const std::wstring_view prefix)
        {
            if (text.size() < prefix.size())
                return false;
            for (std::size_t i = 0; i != prefix.size(); ++i)
            {
                if (std::towlower(text[i]) != std::towlower(prefix[i]))
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool endsWithLowered(const std::wstring_view text,
            const std::wstring_view suffix)
        {
            return text.size() >= suffix.size()
                && startsWithLowered(text.substr(text.size() - suffix.size()), suffix);
        }

        // What a comment opening at a place of a line opens with, and what closes it.
        struct Opener
        {
            std::size_t length{ 0 };      // the marks, those a doc comment adds included
            std::wstring_view closer;     // empty for a comment running to the line's end
        };

        // The marks a doc comment adds after the opening ones: stars, then ! and < by loose rules.
        [[nodiscard]] std::size_t docMarksEnd(const std::wstring_view text, std::size_t at,
            const std::wstring_view closer, const wchar_t repeated, const HintRules rules)
        {
            while (at < text.size() && text[at] == repeated
                && (closer.empty() || !text.substr(at).starts_with(closer)))
            {
                ++at;
            }
            if (rules != HintRules::Loose)
                return at;
            if (at < text.size() && text[at] == L'!')
                ++at;
            if (at < text.size() && text[at] == L'<')
                ++at;
            return at;
        }

        [[nodiscard]] std::optional<Opener> openerAt(const std::wstring_view line,
            const std::size_t at, const SourceLanguage language, const HintRules rules)
        {
            const std::wstring_view rest = line.substr(std::min(at, line.size()));
            if (rest.starts_with(k_lineComment))
            {
                return Opener{
                    .length = docMarksEnd(rest, k_lineComment.size(), {}, L'/', rules),
                    .closer = {}
                };
            }
            if (language == SourceLanguage::Cpp)
            {
                if (!rest.starts_with(k_cppBlock))
                    return std::nullopt;
                return Opener{
                    .length = docMarksEnd(rest, k_cppBlock.size(), k_cppBlockEnd, L'*', rules),
                    .closer = k_cppBlockEnd
                };
            }
            if (rest.starts_with(k_pascalParen))
            {
                return Opener{
                    .length = docMarksEnd(rest, k_pascalParen.size(), k_pascalParenEnd, L'*',
                        rules),
                    .closer = k_pascalParenEnd
                };
            }
            if (rest.starts_with(k_pascalBrace))
            {
                return Opener{
                    .length = docMarksEnd(rest, k_pascalBrace.size(), k_pascalBraceEnd, L'*',
                        rules),
                    .closer = k_pascalBraceEnd
                };
            }
            return std::nullopt;
        }

        // Where the marks closing a block start in a text: at the closer, or at the stars and the
        // blanks standing right before it - the words end there.
        [[nodiscard]] std::size_t tailStart(const std::wstring_view text, std::size_t close)
        {
            while (close != 0 && text[close - 1] == L'*')
                --close;
            while (close != 0 && isBlankChar(text[close - 1]))
                --close;
            return close;
        }

        // Moves what the words open with into the lead and what they end with into the tail,
        // where the rules read those as marks: a brief's command, a summary's tags.
        void takeMarks(CommentLine& line, const HintRules rules)
        {
            const std::size_t blanks = leadingBlanks(line.words);
            const std::wstring_view words = std::wstring_view{ line.words }.substr(blanks);
            for (const std::wstring_view command : k_briefCommands)
            {
                const bool strictOne = command.front() == L'@';
                if (!words.starts_with(command) || (rules == HintRules::Strict && !strictOne))
                    continue;
                const std::size_t after = command.size();
                if (after != words.size() && !isBlankChar(words[after]))
                    continue;
                const std::size_t cut = blanks + after
                    + leadingBlanks(words.substr(std::min(after, words.size())));
                line.lead += line.words.substr(0, cut);
                line.words.erase(0, cut);
                line.opensParagraph = true;
                return;
            }
            if (rules != HintRules::Loose)
                return;
            if (startsWithLowered(words, k_summaryOpener))
            {
                const std::size_t cut = blanks + k_summaryOpener.size()
                    + leadingBlanks(words.substr(k_summaryOpener.size()));
                line.lead += line.words.substr(0, cut);
                line.words.erase(0, cut);
                line.opensParagraph = true;
            }
            const std::wstring_view left = rightTrimmed(line.words);
            if (endsWithLowered(left, k_summaryCloser))
            {
                const std::size_t cut = tailStart(left, left.size() - k_summaryCloser.size());
                line.tail.insert(0, line.words.substr(cut));
                line.words.erase(cut);
            }
        }

        // The words a line says by the rules, or that it says none.
        void settleWords(CommentLine& line, const HintRules rules)
        {
            if (!line.speaks)
                return;
            const std::wstring_view said = trimmedText(line.words);
            if (said.starts_with(k_todo))
            {
                line.speaks = false;
                line.opensParagraph = false;
                return;
            }
            takeMarks(line, rules);
            if (rules == HintRules::Loose)
            {
                std::wstring cleaned = withoutSeparators(line.words);
                const bool separator = trimmedText(cleaned).empty()
                    && !trimmedText(line.words).empty();
                if (separator)
                {
                    line.speaks = false;
                    line.opensParagraph = true;
                    return;
                }
                line.words = std::move(cleaned);
            }
            if (opensPart(trimmedText(line.words), rules))
                line.opensParagraph = true;
            if (trimmedText(line.words).empty() && !line.tail.empty())
            {
                line.speaks = false;
                line.opensParagraph = true;
            }
        }

        // A block's lines no star dresses are set in by what they share, so code keeps its own.
        void setInBare(CommentLines& lines, const std::span<const std::size_t> bare)
        {
            std::size_t shared = std::wstring::npos;
            for (const std::size_t index : bare)
            {
                const std::wstring& words = lines[index].words;
                if (!trimmedText(words).empty())
                    shared = std::min(shared, leadingBlanks(words));
            }
            if (shared == std::wstring::npos)
                shared = 0;
            for (const std::size_t index : bare)
            {
                CommentLine& line = lines[index];
                const std::size_t cut = std::min(shared, leadingBlanks(line.words));
                line.lead = line.words.substr(0, cut);
                line.words.erase(0, cut);
            }
        }
    }

    // Branches

    void Branches::take(const std::wstring_view word, const std::wstring_view condition)
    {
        const bool zero = condition == L"0";
        if (word == L"if" || word == L"ifdef" || word == L"ifndef")
        {
            const bool outer = reading();
            const bool read = outer && !(word == L"if" && zero);
            m_frames.push_back({ .outerReading = outer, .reading = read, .taken = read });
            return;
        }
        if (m_frames.empty())
            return;
        Frame& frame = m_frames.back();
        if (word == L"elif" || word == L"elifdef" || word == L"elifndef")
        {
            frame.reading = frame.outerReading && !frame.taken && !(word == L"elif" && zero);
            frame.taken = frame.taken || frame.reading;
        }
        else if (word == L"else")
        {
            frame.reading = frame.outerReading && !frame.taken;
            frame.taken = true;
        }
        else if (word == L"endif")
        {
            m_frames.pop_back();
        }
    }

    bool Branches::reading() const
    {
        return m_frames.empty() || m_frames.back().reading;
    }

    // **A comment opens on the first line at the column given**; on the others it opens where the
    // line does, or carries on the block a line before left open. A line comment's marks are its
    // slashes, and by loose rules the ! and the < a doc comment adds; a block's are its opener,
    // the stars after it and those dressing a line inside it, and its closer.
    CommentLines cutComment(const std::span<const std::wstring_view> lines, const std::size_t start,
        const SourceLanguage language, const HintRules rules)
    {
        CommentLines result;
        std::wstring_view closer;          // what closes the block a line ended inside
        std::vector<std::size_t> bare;     // the lines of the open block no star dresses
        for (std::size_t i = 0; i != lines.size(); ++i)
        {
            const std::wstring_view line = rightTrimmed(lines[i]);
            CommentLine entry;
            if (closer.empty())
            {
                const std::size_t at = i == 0 ? start : leadingBlanks(line);
                const std::optional<Opener> opener = openerAt(line, at, language, rules);
                if (!opener)
                {
                    entry.lead = std::wstring{ line };
                    entry.speaks = false;
                    entry.opensParagraph = true;
                    result.push_back(std::move(entry));
                    continue;
                }
                std::size_t wordsAt = std::min(at + opener->length, line.size());
                if (wordsAt < line.size() && isBlankChar(line[wordsAt]))
                    ++wordsAt;
                const std::wstring_view body = line.substr(wordsAt);
                entry.lead = std::wstring{ line.substr(0, wordsAt) };
                entry.words = std::wstring{ body };
                if (!opener->closer.empty())
                {
                    entry.opensParagraph = !result.empty();
                    const std::size_t close = body.find(opener->closer);
                    if (close == std::wstring_view::npos)
                    {
                        closer = opener->closer;
                        bare.clear();
                        if (trimmedText(body).empty())
                        {
                            entry.speaks = false;
                            entry.opensParagraph = true;
                        }
                    }
                    else
                    {
                        const std::size_t cut = tailStart(body, close);
                        entry.words = std::wstring{ body.substr(0, cut) };
                        entry.tail = std::wstring{ body.substr(cut) };
                    }
                }
            }
            else
            {
                const std::size_t close = line.find(closer);
                const std::size_t end = close == std::wstring_view::npos
                    ? line.size()
                    : tailStart(line, close);
                const std::wstring_view inside = line.substr(0, end);
                const std::size_t at = leadingBlanks(inside);
                if (at < inside.size() && inside[at] == L'*')
                {
                    std::size_t wordsAt = at;
                    while (wordsAt < inside.size() && inside[wordsAt] == L'*')
                        ++wordsAt;
                    if (wordsAt < inside.size() && isBlankChar(inside[wordsAt]))
                        ++wordsAt;
                    entry.lead = std::wstring{ inside.substr(0, wordsAt) };
                    entry.words = std::wstring{ inside.substr(wordsAt) };
                }
                else
                {
                    bare.push_back(result.size());
                    entry.words = std::wstring{ inside };
                }
                entry.tail = std::wstring{ line.substr(end) };
                if (close != std::wstring_view::npos)
                {
                    closer = {};
                    result.push_back(std::move(entry));
                    setInBare(result, bare);
                    bare.clear();
                    continue;
                }
            }
            result.push_back(std::move(entry));
        }
        setInBare(result, bare);
        for (CommentLine& entry : result)
            settleWords(entry, rules);
        // A TODO runs on over the lines under it to the end of its paragraph, or of the block it
        // stands in, saying nothing.
        bool todo = false;
        for (CommentLine& entry : result)
        {
            if (!entry.speaks && !entry.opensParagraph)
            {
                todo = entry.tail.empty();
                continue;
            }
            const bool ends = entry.opensParagraph || !entry.speaks
                || trimmedText(entry.words).empty();
            if (todo && !ends)
            {
                entry.speaks = false;
                todo = entry.tail.empty();
            }
            else
            {
                todo = false;
            }
        }
        return result;
    }

    // A line that says nothing parts paragraphs only where it marks a part: a TODO is skipped.
    // Code - a line set in by four blanks - stands apart from the words round it, as a comment
    // shows it with no blank line between.
    CommentTextLines textLinesOf(const CommentLines& lines)
    {
        CommentTextLines result;
        bool parted = false;
        bool afterCode = false;
        for (std::size_t i = 0; i != lines.size(); ++i)
        {
            const CommentLine& line = lines[i];
            if (!line.speaks || trimmedText(line.words).empty())
            {
                parted = parted || line.speaks || line.opensParagraph;
                continue;
            }
            const bool code = std::wstring_view{ line.words }.starts_with(k_codeFence);
            const bool apart = parted || line.opensParagraph || code != afterCode;
            if (!result.empty() && apart)
                result.push_back({ .words = {}, .line = std::wstring::npos });
            parted = false;
            afterCode = code;
            result.push_back({ .words = line.words, .line = i });
        }
        return result;
    }

    std::wstring commentText(const CommentLines& lines)
    {
        std::wstring result;
        for (const CommentTextLine& line : textLinesOf(lines))
        {
            if (!result.empty() || line.line == std::wstring::npos)
                result += L'\n';
            result += line.words;
        }
        return result;
    }

    // A doc command is @ or \\ and a letter or a brace; a doc tag opens with < and a tag's name,
    // or </ and one.
    bool opensPart(const std::wstring_view words, const HintRules rules)
    {
        if (words.size() >= 2 && (words.front() == L'@' || words.front() == L'\\'))
        {
            const wchar_t next = words[1];
            if (std::iswalpha(next) != 0 || next == L'{' || next == L'}')
                return true;
        }
        if (rules != HintRules::Loose || !words.starts_with(L'<'))
            return false;
        std::wstring_view tag = words.substr(1);
        if (tag.starts_with(L'/'))
            tag.remove_prefix(1);
        return std::ranges::any_of(k_docTags, [tag](const std::wstring_view name) {
            if (!startsWithLowered(tag, name))
                return false;
            const wchar_t after = tag.size() > name.size() ? tag[name.size()] : L'>';
            return after == L'>' || after == L' ' || after == L'/';
        });
    }

    // The blanks before the mark stay: they part it from the dressing, and so the words from it.
    std::wstring_view leadWithoutBrief(const std::wstring_view lead)
    {
        const std::wstring_view said = rightTrimmed(lead);
        for (const std::wstring_view command : k_briefCommands)
        {
            if (said.ends_with(command))
                return lead.substr(0, said.size() - command.size());
        }
        if (endsWithLowered(said, k_summaryOpener))
            return lead.substr(0, said.size() - k_summaryOpener.size());
        return lead;
    }

    std::size_t leadingBlanks(const std::wstring_view text)
    {
        std::size_t count = 0;
        while (count < text.size() && isBlankChar(text[count]))
            ++count;
        return count;
    }

    std::size_t columnOf(const std::wstring_view line, const std::size_t index)
    {
        std::size_t column = 0;
        const std::size_t end = std::min(index, line.size());
        for (std::size_t i = 0; i != end; ++i)
        {
            if (line[i] == L'\t')
                column += k_tabColumns - column % k_tabColumns;
            else
                ++column;
        }
        return column;
    }

    std::size_t indentOf(const std::wstring_view line)
    {
        std::size_t first = 0;
        while (first < line.size() && isBlankChar(line[first]))
            ++first;
        return columnOf(line, first);
    }

    // The blanks around a separator fold into one, which stands only between words.
    std::wstring withoutSeparators(const std::wstring_view words)
    {
        std::wstring result;
        bool cut = false;
        std::size_t i = 0;
        while (i < words.size())
        {
            const wchar_t current = words[i];
            std::size_t end = i + 1;
            while (end < words.size() && words[end] == current)
                ++end;
            const bool separator = end - i >= k_separatorRun
                && k_separatorMarks.find(current) != std::wstring_view::npos;
            if (separator)
            {
                cut = true;
            }
            else if (!cut || !isBlankChar(current))
            {
                if (cut)
                {
                    while (!result.empty() && isBlankChar(result.back()))
                        result.pop_back();
                    if (!result.empty())
                        result += L' ';
                    cut = false;
                }
                result.append(words.substr(i, end - i));
            }
            i = end;
        }
        while (!result.empty() && isBlankChar(result.back()))
            result.pop_back();
        return result;
    }
}
