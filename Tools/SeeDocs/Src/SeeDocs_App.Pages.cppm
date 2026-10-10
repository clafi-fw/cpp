export module SeeDocs_App.Pages;

import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE DOCUMENTATION AS PAGES: what a chapter, a module, a type or a method says, built out of the
// surface and the notes, and rendered by whoever shows it - SeeDocs as controls, a generator
// as a file. See the README beside the project.
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
        // A type's qualified name, a module's name, a chapter's name, or a link to a type's
        // member or to a module's function or constant (memberLink, exportLink); empty for none.
        std::wstring link;
    };

    export using Runs = std::vector<Run>;
    export using Cells = std::vector<Runs>;

    export constexpr std::wstring_view k_noHint = L"no hint";   // a missing hint, as a page says it

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
        std::vector<Runs> lines;    // Code
        std::vector<Runs> items;    // Bullets
    };

    export using Blocks = std::vector<Block>;

    // The words a note gives a declaration: where they come from, and the prose.
    export struct Excerpt
    {
        std::wstring source;   // the note's file and heading, and the mark of a match by name
        Reference from;        // the note and the section the words are read from
        bool byName{ false };  // the section is named after the declaration, no See reaching it
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
        bool hint{ false };   // the rows' hints stand in it
        std::size_t maxLines{ 0 };   // the most lines a cell shows, every line at zero
    };

    export using TableColumns = std::vector<TableColumn>;

    // A row of a table: its cells, the type it stands for where it does, what the first cell's
    // hint shows where it has one, the note the declaration's comment references, and the
    // comment the row's hint is read from.
    export struct TableRow
    {
        Cells cells;
        const Type* type{ nullptr };
        Runs hint;   // a method's signature, set as code
        SourceLanguage language{ SourceLanguage::Cpp };   // what the hint's code is written in
        std::optional<Excerpt> excerpt;
        std::optional<CommentSite> site;
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
        bool takesFootnotes{ false };   // a row may be given a footnote, set under the table
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

    // A declaration on a method's, a function's or a constant's page: its code, hint and note.
    export struct Signature
    {
        Runs code;         // code runs, a type the surface has a page for linked
        SourceLanguage language{ SourceLanguage::Cpp };   // what the code is written in
        Runs lead;         // the hint; none where it stands under the page's title
        std::optional<Excerpt> excerpt;   // where the comment references a note
        std::optional<CommentSite> site;  // the comment the hint is read from
    };

    export using Signatures = std::vector<Signature>;

    export enum class SectionKind
    {
        Note,
        Tree,
        Table,
        Signature   // declarations, under an access label the C++ way - public: - where one applies
    };

    // One section of a page: its heading, and the note, tree, table or signatures it holds as
    // the kind says.
    export struct Section
    {
        SectionKind kind{ SectionKind::Table };
        Runs heading;
        std::wstring hint;   // the heading's hint: what the section holds, counted; empty for none
        Excerpt excerpt;
        Branches branches;
        bool treeOpen{ false };   // whether the tree's nodes start open
        Table table;
        Signatures signatures;
    };

    export using Sections = std::vector<Section>;

    // The ways up from a type through its bases to the roots, a line each.
    export using Chains = std::vector<Runs>;

    // Where a footnote written new for a declaration on a page goes: a note, the section it
    // follows, and the name a heading that would repeat another's takes in front.
    export struct FootnoteHome
    {
        std::wstring note;
        std::wstring after;   // an anchor; empty for the note's end
        std::wstring owner;   // the type the page is about; empty on a chapter's or a module's
    };

    export struct Page
    {
        Runs scope;     // before the title, in its style - a method's type and ::
        std::wstring title;
        Runs badges;    // what the title is - the kind, control, template
        // Under the title, the C++ way: a colon before each base or an enum's underlying type, an
        // equals sign before what an alias stands for.
        std::vector<Runs> titleLines;
        Runs lead;      // the hint
        std::optional<CommentSite> leadSite;   // the comment the lead is read from
        Facts facts;
        Sections sections;
        std::optional<FootnoteHome> footnoteHome;   // none where no note takes one
    };

    export using TypeList = std::vector<const Type*>;
    export using FunctionList = std::vector<const FreeFunction*>;
    export using VariableList = std::vector<const Variable*>;

    // One module of a chapter: its types, controls first, its functions and constants as declared.
    export struct ContentsModule
    {
        std::wstring name;        // the module's full name, a header's path
        std::wstring shortName;   // what a reader sees: the last part, a unit's whole name
        bool header{ false };
        TypeList types;
        FunctionList functions;
        VariableList constants;
    };

    export using ContentsModules = std::vector<ContentsModule>;

    // One chapter: a folder under Source, and the modules whose exports stand in it.
    export struct ContentsChapter
    {
        std::wstring category;
        std::wstring name;        // what a reader sees
        ContentsModules modules;
    };

    export using ContentsChapters = std::vector<ContentsChapter>;

    // The table of contents: what the tree exports, by chapter and module, in reading order.
    export [[nodiscard]] ContentsChapters contentsOf(const Surface&);

    // What a row of the contents tree names.
    export enum class ContentsRowKind
    {
        Folder,
        Module,
        Type
    };

    // A row of the contents tree: what it names, the row it stands under, and the rows under it.
    export struct ContentsRow
    {
        ContentsRowKind kind{ ContentsRowKind::Folder };
        std::wstring_view folder;   // the category of the folder it stands in; a folder's own
        const ContentsModule* module{ nullptr };
        const Type* type{ nullptr };
        std::optional<std::size_t> parent{};   // none for a row at the top
        std::vector<std::size_t> children{};   // in reading order
    };

    export using ContentsRows = std::vector<ContentsRow>;
    export using RowIndexes = std::vector<std::size_t>;

    // THE CONTENTS AS A TREE: the folders, each holding the folders under it before its modules,
    // and each module the types it exports. Made once over the contents it views, which stand
    // for as long as it does, and read by every view of them.
    export class ContentsTree
    {
    public:
        ContentsTree() = default;
        explicit ContentsTree(const ContentsChapters&);
        ContentsTree(const ContentsTree&) = delete;
        ContentsTree& operator=(const ContentsTree&) = delete;
        ContentsTree(ContentsTree&&) = default;
        ContentsTree& operator=(ContentsTree&&) = default;
    public:
        [[nodiscard]] const ContentsRows& rows() const { return m_rows; }
        [[nodiscard]] const RowIndexes& top() const { return m_top; }   // the rows at the root
        [[nodiscard]] bool empty() const { return m_rows.empty(); }
        // The row of the nearest one named: the type, else the module, else the folder - the
        // root's empty folder names none.
        [[nodiscard]] std::optional<std::size_t> rowOf(std::wstring_view folder,
            const ContentsModule*, const Type*) const;
        [[nodiscard]] std::optional<std::size_t> folderRow(std::wstring_view category) const;
        [[nodiscard]] std::optional<std::size_t> moduleRow(std::wstring_view name) const;
    private:
        // The folder's row, made with every folder above it that is not there yet.
        std::size_t addFolder(std::wstring_view category);
        std::size_t addRow(ContentsRow);
    private:
        static constexpr wchar_t k_folderSeparator = L'/';   // between a category's folders

        ContentsRows m_rows{};
        RowIndexes m_top{};
        std::unordered_map<std::wstring, std::size_t> m_folders{};   // its keys are what rows view
        std::unordered_map<const void*, std::size_t> m_named{};      // a module's or a type's
        std::unordered_map<std::wstring_view, std::size_t> m_modules{};   // by the module's name
    };

    // What a reader calls a folder under Source and a module.
    export [[nodiscard]] std::wstring chapterNameOf(std::wstring_view category);
    export [[nodiscard]] std::wstring moduleShortNameOf(std::wstring_view module);
    // What a reader calls the last folder of a category - the folder's own name.
    export [[nodiscard]] std::wstring folderNameOf(std::wstring_view category);
    // The chapters whose folders stand under this one, in reading order; every chapter for the
    // root.
    export [[nodiscard]] ContentsChapters chaptersUnder(const ContentsChapters&,
        std::wstring_view category);
    // The folder under Source a file of the surface stands in.
    export [[nodiscard]] std::wstring categoryOf(std::wstring_view relativeFile);

    // The page of a folder: the chapters under it, each linked, with what every one holds.
    export [[nodiscard]] Page contentsPage(const ContentsChapters&, std::wstring_view title);
    // The page of the tree's root: the chapters under it as a folder's page has them, then what
    // the modules standing in the root itself export, by module.
    export [[nodiscard]] Page homePage(const Surface&, Notes&, const ContentsChapters&,
        std::wstring_view title);
    // A chapter's page: the chapters under its folder, then what its modules export, by module.
    export [[nodiscard]] Page chapterPage(const Surface&, Notes&, const ContentsChapters&,
        const ContentsChapter&);
    export [[nodiscard]] Page modulePage(const Surface&, Notes&, const ContentsChapter&,
        const ContentsModule&);
    // The page of a function the module exports, every overload of the name on it, or of a
    // constant it exports, the declaration as written.
    export [[nodiscard]] Page exportPage(const Surface&, Notes&, const ContentsChapter&,
        const ContentsModule&, std::wstring_view name);
    export [[nodiscard]] Page typePage(const Surface&, Notes&, const Type&);
    // The page of every method of the type with that name, a signature each.
    export [[nodiscard]] Page methodPage(const Surface&, Notes&, const Type&,
        std::wstring_view name);
    // The names the type's own methods stand under, one per name in declaration order: its
    // constructors and destructor in, a property's getter and setter out.
    export [[nodiscard]] std::vector<std::wstring_view> methodNames(const Type&);

    // The names a module's page lists after its types: its functions, one per name, then its
    // constants, each in the order declared.
    export [[nodiscard]] std::vector<std::wstring_view> exportNames(const ContentsModule&);

    // The link to a member's page: the type's qualified name, ::, the member's name.
    export [[nodiscard]] std::wstring memberLink(const Type&, std::wstring_view member);
    // The link to a function's or a constant's page: the module's name, ::, the export's name.
    export [[nodiscard]] std::wstring exportLink(const ContentsModule&, std::wstring_view name);

    // The two names a member link is made of, each a view into the link.
    export struct MemberLink
    {
        std::wstring_view type;     // a type's qualified name, or a module's name
        std::wstring_view member;
    };

    // A link cut at its last ::; nothing for one holding none.
    export [[nodiscard]] std::optional<MemberLink> readMemberLink(std::wstring_view link);
}
