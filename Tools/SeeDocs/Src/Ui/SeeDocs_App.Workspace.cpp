module SeeDocs_App.Workspace;

import SeeDocs_App.Browser;
import SeeDocs_App.NewProject;
import SeeDocs_App.FileWriter;
import SeeDocs_App.Database;
import SeeDocs_App.Project;
import SeeDocs_App.Scanner;
import SeeDocs_App.ScanCache;
import SeeDocs_App.Surface;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.MessageDialog;

import ClaFi.App.Application;
import ClaFi.App.Notices;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer, which is what makes the config's nodes readable as strings.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        using FoldersNode = Dom::Sequence<std::wstring>;

        constexpr std::wstring_view k_titleSeparator = L" - ";

        // The running program's size and last write, which tell one build of it from another;
        // empty where the platform cannot name the program.
        [[nodiscard]] std::wstring programStamp()
        {
            const std::filesystem::path program{ Platform::executablePath() };
            if (program.empty())
                return {};
            std::error_code sizeError;
            std::error_code timeError;
            const std::uintmax_t size = std::filesystem::file_size(program, sizeError);
            const std::filesystem::file_time_type written =
                std::filesystem::last_write_time(program, timeError);
            if (sizeError || timeError)
                return {};
            const std::int64_t ticks =
                static_cast<std::int64_t>(written.time_since_epoch().count());
            return std::to_wstring(size) + L":" + std::to_wstring(ticks);
        }

        using DatabasePtr = std::shared_ptr<const Database>;

        // Hands the database to the writer, which lets it go once it is written.
        void writeDatabaseLater(FileWriter& writer, const Project& project, DatabasePtr database)
        {
            writer.write(project.databasePath(), [database](const std::filesystem::path& file) {
                return database->save(file);
            });
        }

        void writeCacheLater(FileWriter& writer, const Project& project, ScanCache cache)
        {
            const std::shared_ptr<const ScanCache> kept =
                std::make_shared<const ScanCache>(std::move(cache));
            writer.write(project.scanCachePath(), [kept](const std::filesystem::path& file) {
                return writeScanCache(*kept, file);
            });
        }

        // One spelling for a folder wherever it is compared or stored: absolute, normal, and
        // without a trailing separator.
        [[nodiscard]] std::filesystem::path cleanFolder(const std::filesystem::path& folder)
        {
            std::error_code error;
            std::filesystem::path result =
                std::filesystem::absolute(folder, error).lexically_normal();
            if (!result.has_filename())
                result = result.parent_path();
            return result;
        }

        // Why the project cannot read the folder as a source, the one it replaces aside; empty
        // where it can.
        [[nodiscard]] std::wstring sourceRefusal(const Project& project,
            const std::filesystem::path& folder, const std::wstring_view replaced)
        {
            std::error_code error;
            if (!std::filesystem::is_directory(folder, error))
                return L"There is no such folder.";
            const bool known = std::ranges::any_of(project.sources,
                [&project, &folder, replaced](const ProjectSource& entry) {
                    return entry.written != replaced && project.readSource(entry.written) == folder;
                });
            if (known)
                return L"The project reads this folder already.";
            return {};
        }

        // A path as a paragraph of its own, in spot ink, the way a question names a folder.
        void writeFolder(Text& text, const std::filesystem::path& folder)
        {
            text << VSpace{ 8.0f } << k_endLine;
            text << ParaIndent{ 12.0f };
            text << PushThemeColor{ InkWell::spotInk() };
            text << folder.wstring();
            text << PopColor{};
        }
    }

    // Workspace

    Workspace::Workspace(ApplicationBase& application, FormBase& form, SurfaceBrowser& browser)
        :
        m_application{ application },
        m_form{ form },
        m_browser{ browser },
        m_asker{ browser.appButton() }
    {
        m_startTimer.onTick([this](TimerEvent&) {
            openFolder(std::exchange(m_statedToMake, {}));
        });
        m_closeTimer.onTick([this](TimerEvent&) {
            if (OpenProject* closing = std::exchange(m_closing, nullptr))
                close(*closing);
        });
        m_madeTimer.onTick([this](TimerEvent&) {
            std::optional<Project> made = std::exchange(m_made, std::nullopt);
            if (made)
                openProject(std::move(*made), HomeTab::Options);
        });
        // The window title names the project the tab shown stands in.
        m_projectShown = m_browser.onProjectShown([this](ProjectShownEvent& event) {
            m_shown = event.project;
            writeTitle();
        });
        m_closeAsked = m_browser.onCloseProject([this](CloseProjectEvent& event) {
            closeLater(event.project);
        });
        m_orderAsked = m_browser.onOrderProjects([this](OrderProjectsEvent& event) {
            orderProjects(event.projects);
        });
        m_rescanAsked = m_browser.onRescanProject([this](RescanProjectEvent& event) {
            rescan(event.project);
        });
        m_surfaceEdited = m_browser.onSurfaceEdited([this](SurfaceEditedEvent& event) {
            saveDatabase(event.project);
        });
        m_writeFailed = m_writer.onWriteFailed([this](WriteFailedEvent& event) {
            noteUnwritten(event.file);
        });
    }

    Workspace::Folders Workspace::recentFolders() const
    {
        Folders result;
        for (const std::wstring& name : storedRecent())
        {
            if (!openFor(name))
                result.emplace_back(name);
        }
        return result;
    }

    // Projects that stand already open before the window is up, so the first frame shows them. A
    // folder that stands for no project is offered one once the window is up, since the offer
    // asks. A project left open whose properties cannot be read now is left out of those open.
    // Nothing is guessed from where SeeDocs stands: the project is the user's to name.
    void Workspace::start(const std::filesystem::path& stated)
    {
        for (const std::wstring& name : storedOpen())
        {
            std::optional<Project> project = readProject(name);
            if (project && !openFor(project->folder))
                openProject(std::move(*project), HomeTab::None);
        }

        if (!stated.empty())
        {
            const std::filesystem::path folder = cleanFolder(stated);
            std::optional<Project> project = readProject(folder);
            if (!project)
            {
                m_statedToMake = folder;
                m_startTimer.start(MilliSeconds{ 0u });
            }
            else if (!openFor(project->folder))
            {
                openProject(std::move(*project), HomeTab::Open);
            }
        }

        storeOpen();
        m_browser.showWords(noProjectWords());
    }

    bool Workspace::openFolder(const std::filesystem::path& stated)
    {
        const std::filesystem::path folder = cleanFolder(stated);
        std::error_code error;
        if (!std::filesystem::is_directory(folder, error))
        {
            forget(folder);
            Text message;
            message << L"There is no such folder any more:";
            writeFolder(message, folder);
            refuse(L"Folder not found", message);
            return false;
        }

        if (OpenProject* open = openFor(folder))
        {
            m_browser.openHome(*open);
            remember(open->project);
            return true;
        }

        std::optional<Project> project = readProject(folder);
        if (!project)
        {
            if (confirmMaking(folder))
                createNew(folder);
            return false;
        }

        openProject(std::move(*project), HomeTab::Open);
        return true;
    }

    void Workspace::browse()
    {
        std::filesystem::path start;
        if (m_shown)
            start = m_shown->project.folder;
        else
            start = std::filesystem::path{ Platform::documentsPath() };

        const std::optional<std::filesystem::path> picked =
            Platform::pickFolder(&m_form, L"Open a project", start);
        if (picked)
            openFolder(*picked);
    }

    // **The project is made while the dialog stands**, so a folder that cannot be made is said
    // there, with the name and the place still on screen; it opens once the dialog is down, on a
    // timer, since the click that made it is the dialog's and the dialog closes after it. A source
    // given is written in before it opens, so that its first scan reads it. A source asked for
    // while a dialog stands is a new question, and the dialog standing makes way for it. Where a
    // project is stored is kept whichever way the dialog closes.
    void Workspace::createNew(const std::filesystem::path& source, const InputStamp stamp)
    {
        if (m_newProject && m_newProject->visible() && source.empty())
        {
            m_newProject->showOrActivate(stamp);
            return;
        }
        if (m_newProject && m_newProject->visible())
            m_newProject->close();

        const std::wstring name = source.empty()
            ? std::wstring{}
            : cleanFolder(source).filename().wstring();
        m_newProject = std::make_unique<NewProjectDialog>(
            m_form,
            storedLocation(),
            name,
            [this, source](const std::filesystem::path& folder, const std::wstring_view stated) {
                std::optional<Project> made = createProject(folder, stated);
                if (!made)
                    return std::wstring{ L"The folder could not be made." };
                if (!source.empty())
                {
                    made->sources.push_back({ .written = made->writtenSource(source) });
                    writeProject(*made);
                    if (std::optional<Project> read = readProject(made->folder))
                        made = std::move(read);
                }
                m_made = std::move(made);
                m_madeTimer.start(MilliSeconds{ 0u });
                return std::wstring{};
            }
        );
        NewProjectDialog& dialog = *m_newProject;
        dialog.onClose([this, &dialog](FormCloseEvent&) {
            storeLocation(dialog.location());
        });
        dialog.showOrActivate(stamp);
    }

    ScanStamp Workspace::scanned(const OpenProject& open)
    {
        return open.surface.scanned();
    }

    // **What is shown is what was read back**, so a name the file did not take does not stand. The
    // name the project already goes by is no edit: an empty name kept in the file is what lets
    // the project follow its folder's name, and writing that name out would end it.
    std::wstring Workspace::rename(OpenProject& open, const std::wstring_view name)
    {
        const std::wstring stated{ trimmed(name) };
        if (stated == open.project.name)
            return open.project.name;
        Project renamed = open.project;
        renamed.name = stated;
        writeProject(renamed);
        if (const std::optional<Project> read = readProject(open.project.folder))
            open.project.name = read->name;
        m_browser.renameProject(open);
        writeTitle();
        return open.project.name;
    }

    // **The surface shown is the database as scanned**, read off the document the file is written
    // from; a write that fails says so in a notice. See README#saving
    void Workspace::rescan(OpenProject& open)
    {
        ScopedWaitCursor wait{};
        open.unwritten.withdraw();
        open.surface = scan(open.project);
        open.notes.forgetAll();
        m_browser.rebindProject(open);
    }

    std::wstring Workspace::addSource(OpenProject& open, const std::wstring_view written)
    {
        std::wstring refusal = sourceRefusal(open.project, open.project.readSource(written), {});
        if (!refusal.empty())
            return refusal;
        change(open, [written](Project& project) {
            project.sources.push_back({ .written = std::wstring{ written } });
        }, Reread::Sources);
        return {};
    }

    // **The words written take the source's place** in the order the roots count them, and keep
    // whether the folders under it are read. The same words again are no change, and the same
    // folder spelled another way is written without a scan.
    std::wstring Workspace::replaceSource(OpenProject& open, const std::wstring_view source,
        const std::wstring_view written)
    {
        if (written == source)
            return {};
        const std::filesystem::path folder = open.project.readSource(written);
        std::wstring refusal = sourceRefusal(open.project, folder, source);
        if (!refusal.empty())
            return refusal;
        const Reread reread = folder == open.project.readSource(source)
            ? Reread::Pages
            : Reread::Sources;
        change(open, [source, written](Project& project) {
            for (ProjectSource& entry : project.sources)
            {
                if (entry.written == source)
                    entry.written = written;
            }
        }, reread);
        return {};
    }

    void Workspace::removeSource(OpenProject& open, const std::wstring_view source)
    {
        change(open, [source](Project& project) {
            std::erase_if(project.sources, [source](const ProjectSource& entry) {
                return entry.written == source;
            });
        }, Reread::Sources);
    }

    void Workspace::setSubfolders(OpenProject& open, const std::wstring_view source,
        const bool subfolders)
    {
        change(open, [source, subfolders](Project& project) {
            for (ProjectSource& entry : project.sources)
            {
                if (entry.written == source)
                    entry.subfolders = subfolders;
            }
        }, Reread::Sources);
    }

    void Workspace::orderSources(OpenProject& open,
        const std::span<const std::wstring_view> sources)
    {
        change(open, [sources](Project& project) {
            ProjectSources ordered{};
            ordered.reserve(project.sources.size());
            for (const std::wstring_view source : sources)
            {
                const ProjectSources::iterator found = std::ranges::find_if(project.sources,
                    [source](const ProjectSource& entry) {
                        return entry.written == source;
                    });
                if (found == project.sources.end())
                    continue;
                ordered.push_back(std::move(*found));
                project.sources.erase(found);
            }
            for (ProjectSource& entry : project.sources)
                ordered.push_back(std::move(entry));
            project.sources = std::move(ordered);
        }, Reread::Sources);
    }

    void Workspace::setMode(OpenProject& open, const ScanMode mode)
    {
        if (open.project.mode == mode)
            return;
        change(open, [mode](Project& project) {
            project.mode = mode;
        }, Reread::Sources);
    }

    void Workspace::setHints(OpenProject& open, const HintRules hints)
    {
        if (open.project.hints == hints)
            return;
        change(open, [hints](Project& project) {
            project.hints = hints;
        }, Reread::Sources);
    }

    void Workspace::setEdits(OpenProject& open, const HintEdits edits)
    {
        if (open.project.edits == edits)
            return;
        change(open, [edits](Project& project) {
            project.edits = edits;
        }, Reread::Pages);
    }

    void Workspace::closeLater(OpenProject& open)
    {
        m_closing = &open;
        m_closeTimer.start(MilliSeconds{ 0u });
    }

    Dom::Dt::Section Workspace::createConfigSchema()
    {
        return Dom::Dt::Section{
            Dom::Dt::Sequence{ k_openNodeName, std::wstring{} },
            Dom::Dt::Sequence{ k_recentNodeName, std::wstring{} },
            Dom::Dt::Value{ k_locationNodeName, std::wstring{} }
        };
    }

    // **The database is read before anything is shown**, once the writes asked for are done. A
    // project with no database is scanned first, and so is one whose database was read otherwise
    // than the project now asks; what is shown is always the database, read off the document a
    // fresh scan writes and off the file otherwise, by the one reader.
    void Workspace::openProject(Project project, const HomeTab homeTab)
    {
        ScopedWaitCursor wait{};
        m_writer.waitIdle();
        std::optional<Surface> surface = readDatabase(project.databasePath());
        if (surface && (surface->mode() != project.mode || surface->hintRules() != project.hints))
            surface.reset();
        if (!surface)
            surface = scan(project);

        m_projects.push_back(std::make_unique<OpenProject>(std::move(project),
            std::move(*surface)));
        OpenProject& open = *m_projects.back();
        remember(open.project);
        m_browser.addProject(open, homeTab);
    }

    OpenProject* Workspace::openFor(const std::filesystem::path& folder) const
    {
        const std::filesystem::path projectFolder = cleanFolder(folder);
        for (const std::unique_ptr<OpenProject>& open : m_projects)
        {
            if (open->project.folder == projectFolder)
                return open.get();
        }
        return nullptr;
    }

    void Workspace::orderProjects(const std::span<OpenProject* const> order)
    {
        Projects ordered{};
        ordered.reserve(m_projects.size());
        for (OpenProject* open : order)
        {
            const Projects::iterator found = std::ranges::find_if(m_projects,
                [open](const std::unique_ptr<OpenProject>& project) {
                    return project.get() == open;
                });
            if (found == m_projects.end())
                continue;
            ordered.push_back(std::move(*found));
            m_projects.erase(found);
        }
        for (std::unique_ptr<OpenProject>& project : m_projects)
            ordered.push_back(std::move(project));
        m_projects = std::move(ordered);
        storeOpen();
        m_browser.orderProjects(order);
    }

    // The browser lets go of the project before it is dropped.
    void Workspace::close(OpenProject& open)
    {
        m_browser.removeProject(open);
        std::erase_if(m_projects, [&open](const std::unique_ptr<OpenProject>& project) {
            return project.get() == &open;
        });
        storeOpen();
    }

    bool Workspace::confirmMaking(const std::filesystem::path& folder)
    {
        Text message;
        message << L"No project is stored in this folder:";
        writeFolder(message, folder);
        message << k_endLine << k_endLine << L"Make a new project that reads it? The project is "
            L"stored in a folder of its own, wherever you choose, and the sources are left as "
            L"they are.";

        MessageDialog dialog{ m_asker, L"New project", message, MessageIcon::Question };
        dialog.add(DialogAnswer::Yes);
        dialog.add(DialogAnswer::No);
        return dialog.execute() == DialogAnswer::Yes;
    }

    void Workspace::refuse(const std::wstring_view title, const Text& message)
    {
        MessageDialog dialog{ m_asker, title, message, MessageIcon::Warning };
        dialog.add(DialogAnswer::Ok);
        dialog.execute();
    }

    // **Written before anything is read again**, so a scan that fails leaves the project set as
    // asked all the same; what stands afterwards is what was read back.
    void Workspace::change(OpenProject& open, const std::function<void(Project&)>& edit,
        const Reread reread)
    {
        Project changed = open.project;
        edit(changed);
        writeProject(changed);
        if (std::optional<Project> read = readProject(open.project.folder))
            open.project = std::move(*read);
        if (reread == Reread::Sources)
            rescan(open);
        else
            m_browser.rebindProject(open);
    }

    // **Only the files changed since the last scan are read**, by the cache the last write left.
    // See README#scan-cache
    Surface Workspace::scan(const Project& project)
    {
        const std::wstring build = programStamp();
        m_writer.waitIdle();
        ScanCache cache = build.empty()
            ? ScanCache{}
            : readScanCache(project.scanCachePath(), build);
        const DatabasePtr database = std::make_shared<const Database>(
            scanSources(project.sourceFolders(), project.mode, project.hints, cache));
        Surface surface = database->surface();
        writeDatabaseLater(m_writer, project, database);
        if (!build.empty())
        {
            cache.build = build;
            writeCacheLater(m_writer, project, std::move(cache));
        }
        return surface;
    }

    void Workspace::saveDatabase(OpenProject& open)
    {
        open.unwritten.withdraw();
        writeDatabaseLater(m_writer, open.project, std::make_shared<const Database>(open.surface));
    }

    // A cache that could not be written costs the next scan its speed, and says nothing.
    void Workspace::noteUnwritten(const std::filesystem::path& file)
    {
        const Projects::iterator found = std::ranges::find_if(m_projects,
            [&file](const std::unique_ptr<OpenProject>& open) {
                return open->project.databasePath() == file;
            });
        if (found == m_projects.end())
            return;
        OpenProject& open = **found;
        Text message;
        message << L"The database could not be written to ";
        message << PushThemeColor{ InkWell::spotInk() } << file.wstring() << PopColor{};
        message << L". The pages show the last scan, and the file holds the one before it.";
        open.unwritten = notices().post(Notice{
            .icon = MessageIcon::Error,
            .title = L"Save database: " + open.project.name,
            .message = std::move(message),
            .answers = {
                NoticeAnswer{
                    .caption = L"Try again",
                    .run = [this, &open](ClickEvent&) {
                        saveDatabase(open);
                    }
                }
            }
        });
    }

    void Workspace::remember(const Project& project)
    {
        const std::wstring folder = project.folder.wstring();
        Names recent = storedRecent();
        std::erase(recent, folder);
        recent.insert(recent.begin(), folder);
        if (recent.size() > k_recentLimit)
            recent.resize(k_recentLimit);
        storeRecent(recent);
        storeOpen();
    }

    void Workspace::forget(const std::filesystem::path& folder)
    {
        Names recent = storedRecent();
        if (std::erase(recent, folder.wstring()) > 0)
            storeRecent(recent);
    }

    Workspace::Names Workspace::storedRecent() const
    {
        Names result;
        const FoldersNode& node = (m_application.config() / k_recentNodeName).as<FoldersNode>();
        for (const std::wstring& name : node.get())
            result.push_back(name);
        return result;
    }

    void Workspace::storeRecent(const Names& names)
    {
        FoldersNode& node = (m_application.config() / k_recentNodeName).as<FoldersNode>();
        node.clear();
        for (const std::wstring& name : names)
            node.add().set(name);
    }

    Workspace::Names Workspace::storedOpen() const
    {
        Names result;
        const FoldersNode& node = (m_application.config() / k_openNodeName).as<FoldersNode>();
        for (const std::wstring& name : node.get())
            result.push_back(name);
        return result;
    }

    void Workspace::storeOpen()
    {
        FoldersNode& node = (m_application.config() / k_openNodeName).as<FoldersNode>();
        node.clear();
        for (const std::unique_ptr<OpenProject>& open : m_projects)
            node.add().set(open->project.folder.wstring());
    }

    std::filesystem::path Workspace::storedLocation() const
    {
        const std::wstring stored =
            (m_application.config() / k_locationNodeName).get<std::wstring>();
        if (!stored.empty())
            return std::filesystem::path{ stored };
        const std::wstring documents = Platform::documentsPath();
        if (documents.empty())
            return {};
        return cleanFolder(std::filesystem::path{ documents } / k_defaultLocation);
    }

    void Workspace::storeLocation(const std::filesystem::path& location)
    {
        if (!location.empty())
            (m_application.config() / k_locationNodeName).set(location.wstring());
    }

    Text Workspace::noProjectWords() const
    {
        Text text;
        text << TextStyle::Title << m_application.name() << PopTextStyle{};
        text << k_endLine << k_endLine;
        text << L"No project is open. The Open page of the application menu opens one, or "
            L"makes a new one - a folder of its own, reading the folders of sources it is "
            L"given." << k_endLine;
        return text;
    }

    // The browser's own bar carries the application's name; the project shown goes to the window.
    void Workspace::writeTitle()
    {
        std::wstring title;
        if (m_shown)
            title.append(m_shown->project.name).append(k_titleSeparator);
        title.append(m_application.name().plainText());
        m_form.setWindowTitle(title);
    }
}
