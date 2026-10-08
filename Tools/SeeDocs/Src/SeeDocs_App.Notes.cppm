export module SeeDocs_App.Notes;

import ClaFi.StdLib;

// THE FOOTNOTES, each read on the first ask and cut into its sections. A section is a
// heading and the lines under it, up to the next heading of its own level or a higher one, and a
// reference reaches it by the slug of its heading. See the README beside the project.
namespace SeeDocs_App
{
    export using Lines = std::vector<std::wstring>;
    export using Stems = std::vector<std::wstring>;

    // One heading of a note and the lines under it.
    export struct NoteSection
    {
        std::wstring heading;    // as written, the # marks taken off
        std::wstring anchor;     // the heading's slug, which a reference names
        std::size_t level{ 0 };  // how many # marks the heading carries
        Lines lines;             // up to the next heading of this level or a higher one
    };

    export using NoteSections = std::vector<NoteSection>;

    // A note: its stem, and its sections in the order they are written.
    export struct Note
    {
        std::wstring stem;
        NoteSections sections;
    };

    // A section found by its anchor, and the note it stands in.
    export struct NoteHit
    {
        const Note* note{ nullptr };
        const NoteSection* section{ nullptr };
    };

    export using NoteHits = std::vector<NoteHit>;

    // How many marks a heading line opens with; none for a line that is no heading.
    export [[nodiscard]] std::size_t headingLevel(std::wstring_view line);

    // The folder of notes. A note is read once, on the first ask for it.
    export class Notes
    {
    public:
        explicit Notes(std::filesystem::path folder);
        Notes(const Notes&) = delete;
        Notes& operator=(const Notes&) = delete;
    public:
        [[nodiscard]] const std::filesystem::path& folder() const { return m_folder; }
        // The note of that stem, or null where the folder holds no such file.
        [[nodiscard]] const Note* note(std::wstring_view stem);
        // The section a reference reaches, or null where the note or the anchor is missing.
        [[nodiscard]] const NoteSection* section(std::wstring_view stem, std::wstring_view anchor);
        [[nodiscard]] bool exists(std::wstring_view stem);
        [[nodiscard]] bool reaches(std::wstring_view stem, std::wstring_view anchor);
        // The stems of every note in the folder, in name order.
        [[nodiscard]] const Stems& stems();
        // Every section of every note whose anchor is the one named, in the folder's order.
        [[nodiscard]] NoteHits sectionsNamed(std::wstring_view anchor);
    private:
        [[nodiscard]] static Note read(std::wstring_view stem, const std::wstring& text);
    private:
        std::filesystem::path m_folder;
        std::map<std::wstring, std::optional<Note>> m_read;   // nothing where the file is missing
        std::optional<Stems> m_stems;
    };
}
