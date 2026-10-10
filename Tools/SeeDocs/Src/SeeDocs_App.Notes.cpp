module SeeDocs_App.Notes;

import SeeDocs_App.Surface;

import ClaFi.Documents.TextFile;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_readMeStem = L"README";
        constexpr wchar_t k_headingMark = L'#';
        constexpr std::size_t k_titleLevel = 1;

        [[nodiscard]] Lines splitLines(const std::wstring& text)
        {
            Lines lines;
            std::size_t start = 0;
            while (start <= text.size())
            {
                std::size_t end = text.find(L'\n', start);
                if (end == std::wstring::npos)
                    end = text.size();
                std::wstring_view line = std::wstring_view{ text }.substr(start, end - start);
                if (line.ends_with(L'\r'))
                    line.remove_suffix(1);
                lines.emplace_back(line);
                start = end + 1;
            }
            return lines;
        }

        void trimBlankLines(Lines& lines)
        {
            while (!lines.empty() && trimmed(lines.back()).empty())
                lines.pop_back();
            std::size_t leading = 0;
            while (leading < lines.size() && trimmed(lines[leading]).empty())
                ++leading;
            lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(leading));
        }
    }

    // A name the owner's own is the owner's: a type's footnote is headed by the type alone.
    std::wstring footnoteHeading(const std::wstring_view owner, const std::wstring_view name)
    {
        if (owner.empty() || owner == name)
            return std::wstring{ name };
        return std::wstring{ owner } + L" " + std::wstring{ name };
    }

    std::size_t headingLevel(const std::wstring_view line)
    {
        std::size_t level = 0;
        while (level < line.size() && line[level] == k_headingMark)
            ++level;
        const bool heading = level != 0 && (level == line.size() || line[level] == L' ');
        return heading ? level : 0;
    }

    std::size_t sectionLevel(const std::wstring_view line)
    {
        const std::size_t level = headingLevel(line);
        return level > k_titleLevel ? level : 0;
    }

    Notes::Notes(std::filesystem::path folder)
        :
        m_folder{ std::move(folder) }
    {
    }

    const Note* Notes::note(const std::wstring_view stem)
    {
        const auto found = m_read.find(std::wstring{ stem });
        if (found != m_read.end())
            return found->second ? &*found->second : nullptr;

        std::optional<Note>& slot = m_read[std::wstring{ stem }];
        const std::filesystem::path file =
            m_folder / (std::wstring{ stem } + std::wstring{ k_noteExtension });
        const std::optional<std::wstring> text = Documents::readTextFile(file);
        if (!text)
            return nullptr;
        slot = read(stem, *text);
        return &*slot;
    }

    const NoteSection* Notes::section(const std::wstring_view stem, const std::wstring_view anchor)
    {
        const Note* found = note(stem);
        if (!found)
            return nullptr;
        for (const NoteSection& section : found->sections)
        {
            if (section.anchor == anchor)
                return &section;
        }
        return nullptr;
    }

    bool Notes::exists(const std::wstring_view stem)
    {
        return note(stem) != nullptr;
    }

    bool Notes::reaches(const std::wstring_view stem, const std::wstring_view anchor)
    {
        return section(stem, anchor) != nullptr;
    }

    const Stems& Notes::stems()
    {
        if (m_stems)
            return *m_stems;
        m_stems.emplace();
        std::error_code error;
        for (const std::filesystem::directory_entry& entry :
            std::filesystem::directory_iterator{ m_folder, error })
        {
            if (!entry.is_regular_file(error) || entry.path().extension() != k_noteExtension)
                continue;
            const std::wstring stem = entry.path().stem().wstring();
            if (stem != k_readMeStem)
                m_stems->push_back(stem);
        }
        std::ranges::sort(*m_stems);
        return *m_stems;
    }

    NoteHits Notes::sectionsNamed(const std::wstring_view anchor)
    {
        NoteHits result;
        for (const std::wstring& stem : stems())
        {
            const Note* found = note(stem);
            if (!found)
                continue;
            for (const NoteSection& section : found->sections)
            {
                if (section.anchor == anchor)
                    result.push_back({ .note = found, .section = &section });
            }
        }
        return result;
    }

    void Notes::forget(const std::wstring_view stem)
    {
        m_read.erase(std::wstring{ stem });
        m_stems.reset();
    }

    void Notes::forgetAll()
    {
        m_read.clear();
        m_stems.reset();
    }

    // Every heading under the title opens a section; a section's lines run to the next heading
    // that is not nested under it, so a ## section carries its ### subsections, heading lines
    // included. The title opens none, as it would carry the whole note.
    Note Notes::read(const std::wstring_view stem, const std::wstring& text)
    {
        Note note{ .stem = std::wstring{ stem } };
        const Lines lines = splitLines(text);
        for (std::size_t i = 0; i != lines.size(); ++i)
        {
            const std::size_t level = sectionLevel(lines[i]);
            if (level == 0)
                continue;
            NoteSection section{
                .heading = std::wstring{ trimmed(std::wstring_view{ lines[i] }.substr(level)) },
                .level = level
            };
            section.anchor = slug(section.heading);
            for (std::size_t j = i + 1; j != lines.size(); ++j)
            {
                const std::size_t inner = headingLevel(lines[j]);
                if (inner != 0 && inner <= level)
                    break;
                section.lines.push_back(lines[j]);
            }
            trimBlankLines(section.lines);
            note.sections.push_back(std::move(section));
        }
        return note;
    }
}
