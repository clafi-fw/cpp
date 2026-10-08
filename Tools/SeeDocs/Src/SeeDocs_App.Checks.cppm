export module SeeDocs_App.Checks;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE DECLARATION ROUTINE, held to. Every rule the routine states is read off the surface: the
// one-line comment and where it sits, the binds a property needs, the three spellings of an
// event, the references into the footnotes. See the README beside the project.
namespace SeeDocs_App
{
    export enum class Level
    {
        Warning,
        Error
    };

    export struct Entry
    {
        Level level{ Level::Warning };
        Place place;
        std::wstring text;
    };

    export using Entries = std::vector<Entry>;

    // What a check of the tree found, in file and line order once sorted.
    export class Report
    {
    public:
        void add(Level, Place, std::wstring text);
        [[nodiscard]] const Entries& entries() const { return m_entries; }
        [[nodiscard]] std::size_t errorCount() const;
        [[nodiscard]] std::size_t warningCount() const { return m_entries.size() - errorCount(); }
        // Orders the entries by the file they name and the line in it.
        void sort(const Surface&);
    private:
        Entries m_entries;
    };

    // Runs every rule over the surface, reading the footnotes in that folder for the references.
    export [[nodiscard]] Report checkSurface(const Surface&,
        const std::filesystem::path& footnotes);

    // The report as lines of `file:line: level: text`, one per entry.
    export [[nodiscard]] std::wstring formatReport(const Report&, const Surface&);

    // The most columns a declaration line and its comment may take.
    export constexpr std::size_t k_maxColumns = 100;
}
