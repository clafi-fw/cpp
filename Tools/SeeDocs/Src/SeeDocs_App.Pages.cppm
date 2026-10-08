export module SeeDocs_App.Pages;

import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE DOCUMENTATION AS PAGES: what a chapter, a module or a type says, built out of the surface
// and the notes, and rendered by whoever shows it - the studio as controls, a generator as a
// file. See the README beside the project.
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

    // A run of words, and the page it links to where it names one.
    export struct Run
    {
        std::wstring text;
        RunStyle style{ RunStyle::Plain };
        std::wstring link;   // a type's qualified name or a module's name, empty for none
    };

    export using Runs = std::vector<Run>;
    export using Cells = std::vector<Runs>;

    // The prose of a note, as its lines read.
    export enum class BlockKind
    {
        Paragraph,
        SubHeading,
        Code,          // lines set as written
        Bullets        // one entry per item
    };

    // One block of prose. The words are in runs, lines or items as the kind says.
    export struct Block
    {
        BlockKind kind{ BlockKind::Paragraph };
        Runs runs;                  // Paragraph, SubHeading
        Lines lines;                // Code
        std::vector<Runs> items;    // Bullets
    };

    export using Blocks = std::vector<Block>;

    // The words a note gives a declaration: where they come from, and the prose.
    export struct Excerpt
    {
        Runs source;   // the note's file and heading, and the mark of a match by name
        Blocks blocks;
    };

    // One fact of a page's head: what it is called and what it says.
    export struct Fact
    {
        std::wstring label;
        Runs value;
    };

    export using Facts = std::vector<Fact>;

    // A column of a table: its name, and whether it takes the room the others leave.
    export struct TableColumn
    {
        std::wstring name;
        bool fills{ false };
    };

    export using TableColumns = std::vector<TableColumn>;

    // A row of a table: its cells, the type it stands for where it does, and the note the
    // declaration's comment references.
    export struct TableRow
    {
        Cells cells;
        const Type* type{ nullptr };
        std::optional<Excerpt> excerpt;
    };

    export using TableRows = std::vector<TableRow>;

    // Rows under one label, which heads them or stands beside them in the table's group column;
    // a group with no label is the table's plain run of rows.
    export struct TableGroup
    {
        Runs label;
        Runs hint;   // the label in full, where the label is short for it
        TableRows rows;
        std::vector<TableGroup> groups;   // under the rows, each headed by its own label
        [[nodiscard]] std::size_t rowCount() const;   // the rows under it at every level
    };

    export using TableGroups = std::vector<TableGroup>;

    export struct Table
    {
        TableColumns columns;
        std::wstring groupColumn;   // where a group's label stands beside its rows; empty for above
        bool header{ true };        // whether the columns are named over the rows
        TableGroups groups;
        [[nodiscard]] std::size_t rowCount() const;   // at every level
    };

    // A branch of a tree: its words, the type it names where the surface has one, and what
    // hangs under it.
    export struct Branch
    {
        Runs text;
        const Type* type{ nullptr };
        std::vector<Branch> children;
    };

    export using Branches = std::vector<Branch>;

    export enum class SectionKind
    {
        Note,
        Tree,
        Table
    };

    // One section of a page: its heading, and the note, tree or table it holds as the kind says.
    export struct Section
    {
        SectionKind kind{ SectionKind::Table };
        Runs heading;
        std::wstring count;   // what stands after the heading, muted; empty for nothing
        Excerpt excerpt;
        Branches branches;
        Table table;
    };

    export using Sections = std::vector<Section>;

    // The ways up from a type through its bases to the roots, a line each.
    export using Chains = std::vector<Runs>;

    export struct Page
    {
        std::wstring title;
        Runs badges;    // what the title is - the kind, control, template
        Chains bases;   // under the title, the C++ way: a colon before each base
        Runs lead;      // the hint
        Facts facts;
        Sections sections;
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
