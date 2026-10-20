export module SeeDocs_App.Project;

import SeeDocs_App.Surface;

import ClaFi.Core.System.Serialization;

import ClaFi.StdLib;

// **A project is a folder of its own.** Its properties, the database the scanner writes and the
// footnotes the pages show stand in it; the sources it documents stand wherever they are - any
// number of folders, each read as it stands. See README
namespace SeeDocs_App
{
    export constexpr std::wstring_view k_projectFileName = L"Project.cfg";
    export constexpr std::wstring_view k_databaseFileName = L"Surface.cfg";
    // What the last scan read of each file, so that the next reads only what changed.
    export constexpr std::wstring_view k_scanCacheFileName = L"Scan.cache";
    // The footnotes' folder, under the project's own.
    export constexpr std::wstring_view k_footnotesFolder = L"footnotes";

    // What becomes of a hint edited on a page. See README
    export enum class HintEdits
    {
        None,     // the sources are only read
        Direct    // it is written into its source file at once, with no undo
    };

    // A folder of sources as the properties write it, and whether the folders under it are read.
    export struct ProjectSource
    {
        std::wstring written;   // as typed, read as a folder only where one is needed
        bool subfolders{ true };
    };

    export using ProjectSources = std::vector<ProjectSource>;

    // What an item of a project's sidebar is. See README
    export enum class SidebarKind
    {
        Category,     // holds items of its own, which its page lists
        Divider,      // a line between the items either side of it
        AllTheApis,   // the folders the sources read, as the scan lists them
        Content       // a page of the project's own
    };

    // The kinds as a project's properties spell them.
    export constexpr auto enumNames(SidebarKind)
    {
        return std::array{ L"Category", L"Divider", L"AllTheApis", L"Content" };
    }

    export struct SidebarItem;
    export using SidebarItems = std::vector<SidebarItem>;

    // An item of a project's sidebar: its kind, its name, which a url names it by, and the items
    // under it where it is a category.
    export struct SidebarItem
    {
        SidebarKind kind{ SidebarKind::Content };
        std::wstring name;    // empty on a divider
        SidebarItems items;   // a category's
    };

    // How a project's properties write an item: the items under it the same way, as deep as
    // they go.
    export constexpr auto serializedFields(const SidebarItem&)
    {
        return std::make_tuple(
            ClaFi::SerializedField{ L"Kind", &SidebarItem::kind },
            ClaFi::SerializedField{ L"Name", &SidebarItem::name },
            ClaFi::SerializedField{ L"Items", &SidebarItem::items }
        );
    }

    // What a project's sidebar lists while its properties list nothing: the folders the
    // sources read, as one item.
    export [[nodiscard]] SidebarItems defaultSidebar();
    // The first item of the kind at any depth, read in order; null where there is none.
    export [[nodiscard]] const SidebarItem* firstOfKind(const SidebarItems&, SidebarKind);

    // A project: its folder, what it is called, the folders it reads and how it reads them, and
    // the sidebar its tree and its home list.
    export struct Project
    {
        std::filesystem::path folder;
        std::wstring name;
        ProjectSources sources;   // in the order their roots count them
        ScanMode mode{ ScanMode::AnyCppAndDelphi };
        HintRules hints{ HintRules::Strict };
        HintEdits edits{ HintEdits::None };
        SidebarItems sidebar{ defaultSidebar() };

        [[nodiscard]] std::filesystem::path databasePath() const;
        [[nodiscard]] std::filesystem::path scanCachePath() const;
        [[nodiscard]] std::filesystem::path footnotesFolder() const;
        [[nodiscard]] SourceFolders sourceFolders() const;   // the sources, each read as a folder
        [[nodiscard]] SourceRoots sourceRoots() const;       // the sources' folders, a root each
        // A folder picked as the properties write it: relative while the two share one. See README
        [[nodiscard]] std::wstring writtenSource(const std::filesystem::path& picked) const;
        // The folder a source written or typed names, read against the project's folder. See README
        [[nodiscard]] std::filesystem::path readSource(std::wstring_view written) const;
        [[nodiscard]] std::wstring folderName() const;   // the name it goes by where it states none
        // Whether a hint edited on a page is written: hints edited directly, read by the strict
        // rules - the comment a hint is written into is the one a strict read takes it from.
        [[nodiscard]] bool writesHints() const;
    };

    // Reads the project whose properties the folder holds; nothing where it holds none.
    export [[nodiscard]] std::optional<Project> readProject(const std::filesystem::path& folder);
    // Makes the folder and the properties in it, the project reading no sources yet and set to
    // read them as any C++ and Delphi, every recognizable comment a hint. Nothing where the
    // folder could not be made.
    export [[nodiscard]] std::optional<Project> createProject(const std::filesystem::path& folder,
        std::wstring_view name);
    // Writes the project's properties into its folder.
    export void writeProject(const Project&);
}
