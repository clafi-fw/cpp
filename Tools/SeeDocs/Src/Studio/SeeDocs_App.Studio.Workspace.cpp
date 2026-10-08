module SeeDocs_App.Studio.Workspace;

import SeeDocs_App.Studio.SurfaceView;
import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Project;
import SeeDocs_App.Scanner;
import SeeDocs_App.Surface;

import ClaFi.Controls.MessageDialog;

import ClaFi.App.Application;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer, which is what makes the config's nodes readable as strings.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
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
        using RecentNode = Dom::Sequence<std::wstring>;

        constexpr std::wstring_view k_titleSeparator = L" - ";

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

    Workspace::Workspace(ApplicationBase& application, FormBase& form, RichControl& title,
        Control& asker, SurfaceView& view)
        :
        m_application{ application },
        m_form{ form },
        m_title{ title },
        m_asker{ asker },
        m_view{ view }
    {
        m_startTimer.onTick([this](TimerEvent&) {
            openFolder(std::exchange(m_statedToMake, {}));
        });
    }

    const Project* Workspace::project() const
    {
        return m_project ? &*m_project : nullptr;
    }

    Workspace::Folders Workspace::recentFolders() const
    {
        Folders result;
        const std::wstring open = m_project ? m_project->folder.wstring() : std::wstring{};
        for (const std::wstring& name : storedRecent())
        {
            if (name != open)
                result.emplace_back(name);
        }
        return result;
    }

    // A project that stands already opens before the window is up, so the first frame shows it.
    // A folder that is not one yet is opened once the window is up, since making one asks. Nothing
    // is guessed from where the studio stands: the tree is the user's to name.
    void Workspace::start(const std::filesystem::path& stated)
    {
        Text reason;
        if (!stated.empty())
        {
            const std::filesystem::path folder = cleanFolder(stated);
            std::optional<Project> project = readProject(folder);
            if (!project)
            {
                m_statedToMake = folder;
                m_startTimer.start(MilliSeconds{ 0u });
                showNoProject(reason);
                return;
            }
            if (!openProject(std::move(*project), reason))
                showNoProject(reason);
            return;
        }

        const std::wstring stored =
            (m_application.config() / k_projectNodeName).get<std::wstring>();
        if (!stored.empty())
        {
            if (std::optional<Project> project = readProject(stored))
            {
                if (!openProject(std::move(*project), reason))
                    showNoProject(reason);
                return;
            }
        }

        showNoProject(reason);
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

        std::optional<Project> project = readProject(folder);
        if (!project)
        {
            if (!confirmMaking(folder))
                return false;
            project = createProject(folder);
            if (!project)
            {
                Text message;
                message << L"The folder could not be made:";
                writeFolder(message, dataFolderOf(folder));
                refuse(L"Cannot make the project", message);
                return false;
            }
        }

        Text reason;
        if (openProject(std::move(*project), reason))
            return true;
        refuse(L"Cannot open the project", reason);
        return false;
    }

    void Workspace::browse()
    {
        std::filesystem::path start;
        if (m_project)
            start = m_project->folder;
        else
            start = std::filesystem::path{ Platform::documentsPath() };

        const std::optional<std::filesystem::path> picked =
            Platform::pickFolder(&m_form, L"Open a project", start);
        if (picked)
            openFolder(*picked);
    }

    Dom::Dt::Section Workspace::createConfigSchema()
    {
        return Dom::Dt::Section{
            Dom::Dt::Value{ k_projectNodeName, std::wstring{} },
            Dom::Dt::Sequence{ k_recentNodeName, std::wstring{} }
        };
    }

    // THE DATABASE IS READ BEFORE ANYTHING IS LET GO OF, so a project that cannot be opened leaves
    // the one open as it was. A project with no database is scanned first; what is shown is
    // always what was read back from the file, one path for a fresh scan and an old one alike.
    bool Workspace::openProject(Project project, Text& reason)
    {
        ScopedWaitCursor wait{};
        std::optional<Surface> surface = readDatabase(project.databasePath());
        if (!surface)
        {
            scan(project);
            surface = readDatabase(project.databasePath());
        }
        if (!surface)
        {
            reason << L"The database could not be written to";
            writeFolder(reason, project.databasePath());
            return false;
        }

        m_surface = std::move(surface);
        m_notes.emplace(project.footnotesFolder());
        m_project = std::move(project);
        if (!m_view.bind(*m_surface, *m_notes))
            showEmptyProject();
        remember(*m_project);
        writeTitles();
        return true;
    }

    // Asked before the folder is made, because the folder is what is being agreed to.
    bool Workspace::confirmMaking(const std::filesystem::path& folder)
    {
        Text message;
        message << m_application.name();
        message << L" will make a folder in the tree, for its database and its properties:";
        writeFolder(message, dataFolderOf(folder));
        message << k_endLine << k_endLine << L"The tree is scanned into it at once.";

        MessageDialog dialog{ m_asker, L"New project", message, MessageIcon::Question };
        dialog.add(DialogAnswer::Ok);
        dialog.add(DialogAnswer::Cancel);
        return dialog.execute() == DialogAnswer::Ok;
    }

    void Workspace::refuse(const std::wstring_view title, const Text& message)
    {
        MessageDialog dialog{ m_asker, title, message, MessageIcon::Warning };
        dialog.add(DialogAnswer::Ok);
        dialog.execute();
    }

    void Workspace::scan(const Project& project)
    {
        writeDatabase(scanTree(project.folder), project.databasePath());
    }

    void Workspace::remember(const Project& project)
    {
        const std::wstring folder = project.folder.wstring();
        (m_application.config() / k_projectNodeName).set(folder);

        Names recent = storedRecent();
        std::erase(recent, folder);
        recent.insert(recent.begin(), folder);
        if (recent.size() > k_recentLimit)
            recent.resize(k_recentLimit);
        storeRecent(recent);
    }

    void Workspace::forget(const std::filesystem::path& folder)
    {
        const std::wstring name = folder.wstring();
        Dom::DomNodeBase& open = m_application.config() / k_projectNodeName;
        if (open.get<std::wstring>() == name)
            open.set(std::wstring{});

        Names recent = storedRecent();
        if (std::erase(recent, name) > 0)
            storeRecent(recent);
    }

    Workspace::Names Workspace::storedRecent() const
    {
        Names result;
        const RecentNode& node = (m_application.config() / k_recentNodeName).as<RecentNode>();
        for (const std::wstring& name : node.get())
            result.push_back(name);
        return result;
    }

    void Workspace::storeRecent(const Names& names)
    {
        RecentNode& node = (m_application.config() / k_recentNodeName).as<RecentNode>();
        node.clear();
        for (const std::wstring& name : names)
            node.add().set(name);
    }

    // The view lets go of the surface and the notes before they are dropped.
    void Workspace::showNoProject(const Text& reason)
    {
        Text text;
        text << TextStyleId::Title << m_application.name() << PopTextStyle{};
        text << k_endLine << k_endLine;
        if (!reason.empty())
            text << reason << k_endLine << k_endLine;
        text << L"No project is open. The Open page of the application menu opens one - "
            L"the folder of the sources to document." << k_endLine;
        m_view.showText(text);

        m_project.reset();
        m_surface.reset();
        m_notes.reset();
        writeTitles();
    }

    // The project is open all the same, named in the titles and remembered; only its page is words.
    void Workspace::showEmptyProject()
    {
        Text text;
        text << TextStyleId::Title << m_project->name << PopTextStyle{};
        text << k_endLine << k_endLine;
        text << L"There is nothing to document: no source in the folder exports a type."
            << k_endLine << L"The folder read is:";
        writeFolder(text, m_project->folder);
        m_view.showText(text);
    }

    void Workspace::writeTitles()
    {
        const Text& appName = m_application.name();
        Text title;
        std::wstring windowTitle;
        if (m_project)
        {
            title << m_project->name << k_titleSeparator;
            windowTitle.append(m_project->name).append(k_titleSeparator);
        }
        title << appName;
        windowTitle.append(appName.plainText());
        m_title.text() = title;
        m_form.setWindowTitle(windowTitle);
    }
}
