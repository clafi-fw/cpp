export module SeeDocs_App.Articles;

import SeeDocs_App.Project;

import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

// **An article is a page of a project's own**, which a Content item of the sidebar shows: a title,
// a lead and blocks. Its file stands under the project's Articles folder, in a folder for each
// category on the way. See README#articles
namespace SeeDocs_App::Articles
{
    using ClaFi::Text;

    export constexpr std::wstring_view k_fileExtension = L".cfg";

    export using Texts = std::vector<Text>;

    // A paragraph of prose.
    export struct Paragraph
    {
        Text text;

        bool operator==(const Paragraph&) const = default;
    };

    // A heading over the blocks after it, at one of three levels under the article's title.
    export struct Heading
    {
        std::size_t level{ 1 };
        std::wstring anchor;   // what a link reaches it by, kept when its words change
        Text text;

        bool operator==(const Heading&) const = default;
    };

    // What an item of a list stands behind.
    export enum class ListKind
    {
        Bullet,
        Numbered
    };

    export constexpr auto enumNames(ListKind)
    {
        return std::array{ L"Bullet", L"Numbered" };
    }

    // An item of a list. A list is a run of items standing together.
    export struct ListItem
    {
        ListKind kind{ ListKind::Bullet };
        Text text;

        bool operator==(const ListItem&) const = default;
    };

    // Code as it is written, coloured and linked in the language it names when shown.
    export struct CodeBlock
    {
        std::wstring language;   // a Syntax language's name, such as C++; plain text when empty
        std::wstring code;

        bool operator==(const CodeBlock&) const = default;
    };

    // A row of a table: a cell under each column, or one group cell across them all.
    export struct TableRow
    {
        Texts cells;
        bool isGroup{ false };

        bool operator==(const TableRow&) const = default;
    };

    export using TableRows = std::vector<TableRow>;

    // A table: the header's cells, one to a column, and the rows under it.
    export struct Table
    {
        Texts header;
        TableRows rows;

        bool operator==(const Table&) const = default;
    };

    // A picture from a PNG or SVG file of the project's.
    export struct Figure
    {
        std::wstring file;          // relative to the article's own file
        std::wstring description;   // what it shows, for a reader who cannot see it

        bool operator==(const Figure&) const = default;
    };

    // A list of what to do beside a list of what not to.
    export struct DoDont
    {
        Texts doItems;
        Texts dontItems;

        bool operator==(const DoDont&) const = default;
    };

    // What a callout holds for the reader: a fact worth knowing, or a trap.
    export enum class CalloutKind
    {
        Info,
        Warning
    };

    export constexpr auto enumNames(CalloutKind)
    {
        return std::array{ L"Info", L"Warning" };
    }

    export using CalloutBlock = std::variant<Paragraph, ListItem, CodeBlock>;
    export using CalloutBlocks = std::vector<CalloutBlock>;

    // A box of a few blocks, set off from the prose round it.
    export struct Callout
    {
        CalloutKind kind{ CalloutKind::Info };
        CalloutBlocks blocks;

        bool operator==(const Callout&) const = default;
    };

    export using FoldBlock = std::variant<Paragraph, Heading, ListItem, CodeBlock, Table, Figure,
        DoDont>;
    export using FoldBlocks = std::vector<FoldBlock>;

    // Blocks shown under a summary when it is opened.
    export struct Fold
    {
        Text summary;
        FoldBlocks blocks;

        bool operator==(const Fold&) const = default;
    };

    export using Block = std::variant<Paragraph, Heading, ListItem, CodeBlock, Table, Figure,
        DoDont, Callout, Fold>;
    export using Blocks = std::vector<Block>;

    // An article: its title, the lead under the title, and its blocks.
    export struct Article
    {
        std::wstring title;
        Text lead;
        Blocks blocks;

        bool operator==(const Article&) const = default;
    };

    // Where the article of the Content item the names lead to stands.
    export [[nodiscard]] std::filesystem::path pathOf(const Project&, const SidebarNames&);
    // Reads the article in the file; nothing where there is no such file.
    export [[nodiscard]] std::optional<Article> read(const std::filesystem::path&);
    // Writes the article to the file, making the folders on the way; false where the file could
    // not be written.
    export [[nodiscard]] bool write(const Article&, const std::filesystem::path&);
    // Moves the article of a renamed Content item, or a renamed category's folder, to where the
    // new names reach; nothing moves where something stands already. See README#names-and-files
    export void followRename(const Project&, const SidebarNames& before,
        const SidebarNames& after, SidebarKind);
    // The articles in the project's folder that no Content item of its sidebar reaches.
    export [[nodiscard]] std::vector<std::filesystem::path> unreached(const Project&);
}
