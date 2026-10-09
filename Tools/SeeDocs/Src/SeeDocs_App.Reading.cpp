module SeeDocs_App.Reading;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        // A doc comment line that opens with a command rather than words.
        [[nodiscard]] bool opensWithCommand(const std::wstring_view line)
        {
            if (line.size() < 2 || (line.front() != L'@' && line.front() != L'\\'))
                return false;
            const wchar_t next = line[1];
            return std::iswalpha(next) != 0 || next == L'{' || next == L'}';
        }

        [[nodiscard]] bool isBriefCommand(const std::wstring_view line)
        {
            const std::wstring_view word = line.substr(0, line.find(L' '));
            return word == L"@brief" || word == L"\\brief";
        }

        [[nodiscard]] std::wstring_view trimmedText(std::wstring_view text)
        {
            while (!text.empty() && (text.front() == L' ' || text.front() == L'\t'))
                text.remove_prefix(1);
            while (!text.empty() && (text.back() == L' ' || text.back() == L'\t'))
                text.remove_suffix(1);
            return text;
        }

        // Up to the first full stop that ends a sentence - one a blank follows, or the text's end.
        [[nodiscard]] std::wstring firstSentence(const std::wstring_view text)
        {
            const std::size_t stop = text.find(L". ");
            if (stop == std::wstring_view::npos)
                return std::wstring{ trimmedText(text) };
            return std::wstring{ trimmedText(text.substr(0, stop + 1)) };
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

    std::wstring briefOf(const std::span<const std::wstring> lines)
    {
        const auto ends = [](const std::wstring& line) {
            return line.empty() || opensWithCommand(line);
        };
        const auto brief = std::ranges::find_if(lines, [](const std::wstring& line) {
            return isBriefCommand(line);
        });
        std::size_t at = 0;
        std::wstring paragraph;
        if (brief != lines.end())
        {
            const std::wstring_view first = *brief;
            paragraph = trimmedText(first.substr(std::min(first.find(L' '), first.size())));
            at = static_cast<std::size_t>(brief - lines.begin()) + 1;
        }
        else
        {
            while (at < lines.size() && ends(lines[at]))
                ++at;
        }
        for (; at < lines.size() && !ends(lines[at]); ++at)
        {
            if (!paragraph.empty())
                paragraph += L' ';
            paragraph += lines[at];
        }
        return brief != lines.end() ? paragraph : firstSentence(paragraph);
    }
}
