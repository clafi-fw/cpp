export module SeeDocs_App.Studio.Workspace;

import SeeDocs_App.Studio.Browser;
import SeeDocs_App.Notes;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.App.Application;

import ClaFi.Core.Foundation;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    // THE PROJECT THE STUDIO HAS OPEN, and everything that follows from it: the surface read out
    // of its database, the notes of its tree, the window title that names it, and the record of
    // it in the application's config - the project to start on next time, and the list of those
    // opened before. One per studio window, standing for as long as the window does.
    export class Workspace
    {
    public:
        using Folders = std::vector<std::filesystem::path>;
    public:
        Workspace(ApplicationBase&, FormBase& form, SurfaceBrowser&);
        Workspace(const Workspace&) = delete;
        Workspace& operator=(const Workspace&) = delete;
    public:
        // The project open, or null while none is.
        [[nodiscard]] const Project* project() const;
        // The projects opened before, most recent first, the one open left out.
        [[nodiscard]] Folders recentFolders() const;
        // Opens the project the studio starts on: the folder stated, else the project last left
        // open; nothing where there is neither. The browser's tabs were restored for the project
        // last left open, so they stand only where that is the one opened.
        void start(const std::filesystem::path& stated);
        // Opens the project in the folder, making one there where the user agrees to it. False
        // where nothing was opened, the user having been told why.
        bool openFolder(const std::filesystem::path& folder);
        // Asks for a folder in the desktop's own dialog and opens it.
        void browse();
    public:
        // What the config keeps the open project's folder under.
        static constexpr std::wstring_view k_projectNodeName = L"Project";
        // What the config keeps the folders opened before under, most recent first.
        static constexpr std::wstring_view k_recentNodeName = L"RecentProjects";
        static constexpr std::size_t k_recentLimit = 10;
        // What the studio keeps of its own, beside the framework's schema.
        [[nodiscard]] static Dom::Dt::Section createConfigSchema();
    private:
        using Names = std::vector<std::wstring>;
    private:
        // Shows the project, its database scanned first where it has none. False where it could
        // not be, the reason written out and the project open left as it was.
        [[nodiscard]] bool openProject(Project, Text& reason, OpenTabs);
        [[nodiscard]] bool confirmMaking(const std::filesystem::path& folder);
        void refuse(std::wstring_view title, const Text& message);
        // Writes the tree's database, so that the project has one to open.
        void scan(const Project&);
        void remember(const Project&);
        void forget(const std::filesystem::path& folder);
        [[nodiscard]] Names storedRecent() const;
        void storeRecent(const Names&);
        // Shows the studio's own words in place of a project, the reason first where there is one.
        void showNoProject(const Text& reason);
        // Shows the studio's own words for the project open, there being nothing to list in it.
        void showEmptyProject();
        void writeTitle();
    private:
        ApplicationBase& m_application;
        FormBase& m_form;
        SurfaceBrowser& m_browser;
        // What a question or a refusal is dropped under: the browser's app button.
        Control& m_asker;
        std::optional<Project> m_project{};
        std::optional<Surface> m_surface{};
        std::optional<Notes> m_notes{};
        // A folder stated at start that is not a project yet; making one asks, and asking waits
        // for the window to be up.
        std::filesystem::path m_statedToMake{};
        UiTimer m_startTimer{};
    };
}
