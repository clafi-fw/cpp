export module SeeDocs_App.Pages;

import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE DOCUMENTATION AS PAGES: what a chapter, a module or a type says, built out of the surface
// and the notes, and rendered by whoever shows it - the studio as a Text, a generator as a file.
// See the README beside the project.
namespace SeeDocs_App
{
    // How a run of a page's words is set.
    export enum class RunStyle
    {
        Plain,
        Code,      // a name, a type or a signature
        Muted,
        Bold,
        Missing    // what the surface ought to state and does not - a hint, a base
    };

    // A run of words, and the type whose page it links to where it names one.
    export struct Run
    {
        std::wstring text;
        RunStyle style{ RunStyle::Plain };
        std::wstring link;   // a type's qualified name, empty where the run links nowhere
    };

    export using Runs = std::vector<Run>;
    export using Cells = std::vector<Runs>;

    export enum class BlockKind
    {
        Title,
        Lead,          // the sentence under the title
        Meta,          // a line of facts under the lead
        Heading,
        SubHeading,
        Paragraph,
        Code,          // lines set as written
        Bullets,       // one entry per item
        Table,         // rows of cells, for an overview - short cells, a short hint last
        Entry,         // a member: its term as one row of cells, its hint under the term
        Source         // where a note's words come from
    };

    // One block of a page. The words are in runs, lines, items or rows as the kind says.
    export struct Block
    {
        BlockKind kind{ BlockKind::Paragraph };
        Runs runs;                  // every kind but Code, Bullets and Table; an Entry's hint
        Lines lines;                // Code
        std::vector<Runs> items;    // Bullets
        std::vector<Cells> rows;    // Table; an Entry's one term row
        std::size_t group{ 0 };     // Table, Entry: the blocks of one group share their columns
        std::size_t level{ 0 };     // how far the block stands in - a note under an entry is one in
    };

    export using Blocks = std::vector<Block>;

    export struct Page
    {
        std::wstring title;
        Blocks blocks;
    };

    export using TypeList = std::vector<const Type*>;

    // One module of a chapter: the types it exports, controls first.
    export struct ContentsModule
    {
        std::wstring name;        // the module's full name
        std::wstring shortName;   // what a reader sees
        TypeList types;
    };

    export using ContentsModules = std::vector<ContentsModule>;

    // One chapter: a folder under Source, and the modules whose types stand in it.
    export struct ContentsChapter
    {
        std::wstring category;
        std::wstring name;        // what a reader sees
        ContentsModules modules;
    };

    export using ContentsChapters = std::vector<ContentsChapter>;

    // The table of contents: the public types by chapter and module, in reading order.
    export [[nodiscard]] ContentsChapters contentsOf(const Surface&);

    // What a reader calls a folder under Source and a module.
    export [[nodiscard]] std::wstring chapterNameOf(std::wstring_view category);
    export [[nodiscard]] std::wstring moduleShortNameOf(std::wstring_view module);
    // The folder under Source a file of the surface stands in.
    export [[nodiscard]] std::wstring categoryOf(std::wstring_view relativeFile);

    export [[nodiscard]] Page chapterPage(const Surface&, Notes&, const ContentsChapter&);
    export [[nodiscard]] Page modulePage(const Surface&, Notes&, const ContentsChapter&,
        const ContentsModule&);
    export [[nodiscard]] Page typePage(const Surface&, Notes&, const Type&);
}
