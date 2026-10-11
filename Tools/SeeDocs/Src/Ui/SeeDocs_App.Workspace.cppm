export module SeeDocs_App.Workspace;

import SeeDocs_App.Browser;
import SeeDocs_App.NewProject;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.App.Application;

import ClaFi.Core.Foundation;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    // **The projects SeeDocs has open**, and everything that follows from them: the surface read
    // out of each one's database, its notes, the window title that names the one shown, and the
    // record of them in the application's config - the projects to start on next time, the list
    // of those opened before, and where new ones were last stored. One per window, standing for
    // as long as the window does.
    export class Workspace
    {
    public:
        using Folders = std::vector<std::filesystem::path>;
    public:
        Workspace(ApplicationBase&, FormBase& form, SurfaceBrowser&);
        Workspace(const Workspace&) = delete;
        Workspace& operator=(const Workspace&) = delete;
    public:
        [[nodiscard]] bool anyOpen() const { return !m_projects.empty(); }
        // The projects opened before, most recent first, the ones open left out.
        [[nodiscard]] Folders recentFolders() const;
        // Opens the projects left open last time, the tabs on them standing as they were
        // restored, and the project the folder stated stands for in a new tab where it is not
        // one of them.
        void start(const std::filesystem::path& stated);
        // Opens the project the folder stands for in a new tab - a project open already gets a
        // new tab on its home - and for a folder that stands for none, offers to make one that
        // reads it. False where nothing was opened.
        bool openFolder(const std::filesystem::path& folder);
        // Asks for a project's folder in the desktop's own dialog and opens it.
        void browse();
        // Asks for a new project's name and where to store it in a window of its own, makes it,
        // and opens it on its options page. A source given is the first folder it reads. Asked
        // again with none while that window stands, it brings the window forward as it was left.
        void createNew(const std::filesystem::path& source = {}, InputStamp = {});
        [[nodiscard]] static ScanStamp scanned(const OpenProject&);   // when it was last scanned
        // Names the project, the folder's name for an empty one; answers the name it goes by.
        [[nodiscard]] std::wstring rename(OpenProject&, std::wstring_view name);
        // Scans the project's sources again and shows what was read back, the tabs standing.
        void rescan(OpenProject&);
        // Adds a folder of sources picked in the desktop's own dialog, and scans the project
        // again; false where none was added.
        bool addSource(OpenProject&);
        // Reads the folder in place of the source and scans again; the reason where it was refused.
        [[nodiscard]] std::wstring replaceSource(OpenProject&, const std::filesystem::path& source,
            const std::filesystem::path& folder);
        // Stops reading the folder of sources, and scans the project again.
        void removeSource(OpenProject&, const std::filesystem::path& source);
        // Whether the folders under the source are read too; scans the project again.
        void setSubfolders(OpenProject&, const std::filesystem::path& source, bool);
        // How the sources are read and how their comments give the hints - each scans the
        // project again so read.
        void setMode(OpenProject&, ScanMode);
        void setHints(OpenProject&, HintRules);
        // Whether a hint edited on a page is written into its source; the pages are built again.
        void setEdits(OpenProject&, HintEdits);
        // Closes the project and every tab on it once the click that asked has unwound - the
        // page it was pressed on may be one of those going. What the browser's pages ask for.
        void closeLater(OpenProject&);
    public:
        // What the config keeps the open projects' folders under, in the order they were opened.
        static constexpr std::wstring_view k_openNodeName = L"OpenProjects";
        // What the config keeps the folders opened before under, most recent first.
        static constexpr std::wstring_view k_recentNodeName = L"RecentProjects";
        // What the config keeps the folder a new project was last stored in under.
        static constexpr std::wstring_view k_locationNodeName = L"ProjectsLocation";
        static constexpr std::size_t k_recentLimit = 10;
        // Where a new project is offered to be stored before one has been: under the user's
        // documents.
        static constexpr std::wstring_view k_defaultLocation = L"Code Documentations";
        // What SeeDocs keeps of its own, beside the framework's schema.
        [[nodiscard]] static Dom::Dt::Section createConfigSchema();
    private:
        using Names = std::vector<std::wstring>;
        // Held by pointer: the browser shows each one by reference.
        using Projects = std::vector<std::unique_ptr<OpenProject>>;
        // What a change of the project's properties asks for once they are written.
        enum class Reread
        {
            Sources,   // a scan: what is read has changed
            Pages      // the pages built again over the surface as it stands
        };
    private:
        // Shows the project, its database scanned first where it has none or was read otherwise
        // than the project asks. False where it could not be, the reason written out.
        [[nodiscard]] bool openProject(Project, Text& reason, HomeTab);
        // The open project the folder stands for, by its own folder.
        [[nodiscard]] OpenProject* openFor(const std::filesystem::path& folder) const;
        void close(OpenProject&);
        // Asks whether to make a project that reads a folder that stands for none.
        [[nodiscard]] bool confirmMaking(const std::filesystem::path& folder);
        void refuse(std::wstring_view title, const Text& message);
        // Writes the properties as the edit leaves them, reads them back, and rereads as asked.
        void change(OpenProject&, const std::function<void(Project&)>& edit, Reread);
        // Writes the database of the project's sources, so that it has one to open.
        void scan(const Project&);
        void remember(const Project&);   // first among the recent, and among those open
        void forget(const std::filesystem::path& folder);
        [[nodiscard]] Names storedRecent() const;
        void storeRecent(const Names&);
        [[nodiscard]] Names storedOpen() const;
        void storeOpen();   // the projects open as they stand, in the order they were opened
        [[nodiscard]] std::filesystem::path storedLocation() const;
        void storeLocation(const std::filesystem::path&);
        // What the root shows while no project is open, the reasons first where there are any.
        [[nodiscard]] Text noProjectWords(const Text& reasons) const;
        void writeTitle();
    private:
        ApplicationBase& m_application;
        FormBase& m_form;
        SurfaceBrowser& m_browser;
        // What a question or a refusal is dropped under: the browser's app button.
        Control& m_asker;
        Projects m_projects{};
        const OpenProject* m_shown{ nullptr };   // the project the tab shown stands in
        OpenProject* m_closing{ nullptr };       // to close once the click has unwound
        // A folder stated at start that stands for no project; the offer to make one asks, and
        // asking waits for the window to be up.
        std::filesystem::path m_statedToMake{};
        UiTimer m_startTimer{};
        UiTimer m_closeTimer{};
        ScopedEventConnection m_projectShown{};
        ScopedEventConnection m_closeAsked{};
        // A project the new project dialog made, opened once the dialog is down.
        std::optional<Project> m_made{};
        UiTimer m_madeTimer{};
        // The last one asked for, standing or closed; let go when the next is asked for.
        std::unique_ptr<NewProjectDialog> m_newProject{};
    };
}
