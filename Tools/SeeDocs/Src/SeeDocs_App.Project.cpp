module SeeDocs_App.Project;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer: without it a node's set reads a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        namespace Keys
        {
            constexpr std::wstring_view name = L"Name";
        }

        using ProjectDocument = Dom::Document<Dom::FileFormat::ClaFi>;

        [[nodiscard]] Dom::Dt::Section projectLayout()
        {
            return Dom::Dt::Section{ Dom::Dt::Value{ Keys::name, std::wstring{} } };
        }

        // The folder's own name, with a trailing separator looked past.
        [[nodiscard]] std::wstring folderName(const std::filesystem::path& folder)
        {
            const std::filesystem::path normal = folder.lexically_normal();
            if (normal.has_filename())
                return normal.filename().wstring();
            return normal.parent_path().filename().wstring();
        }
    }

    // Project

    std::filesystem::path Project::dataFolder() const
    {
        return dataFolderOf(folder);
    }

    std::filesystem::path Project::databasePath() const
    {
        return dataFolder() / k_databaseFileName;
    }

    std::filesystem::path Project::footnotesFolder() const
    {
        return footnotesFolderOf(folder);
    }

    std::filesystem::path dataFolderOf(const std::filesystem::path& folder)
    {
        return folder / k_projectDataFolder;
    }

    std::filesystem::path footnotesFolderOf(const std::filesystem::path& folder)
    {
        return dataFolderOf(folder) / k_footnotesFolder;
    }

    bool holdsProject(const std::filesystem::path& folder)
    {
        std::error_code error;
        return std::filesystem::is_directory(dataFolderOf(folder), error);
    }

    // A data folder with no properties in it, or with no name in them, is still a project: the
    // folder is what makes one, and the name falls back to the folder's.
    std::optional<Project> readProject(const std::filesystem::path& folder)
    {
        if (!holdsProject(folder))
            return std::nullopt;

        Project project{ .folder = folder, .name = folderName(folder) };
        ProjectDocument document{
            dataFolderOf(folder) / k_projectFileName,
            Dom::AutoSave::No,
            projectLayout(),
            Dom::WriteDefaults::Yes
        };
        if (document.load())
        {
            const std::wstring stated = (document / Keys::name).get<std::wstring>();
            if (!stated.empty())
                project.name = stated;
        }
        return project;
    }

    std::optional<Project> createProject(const std::filesystem::path& folder)
    {
        std::error_code error;
        std::filesystem::create_directories(dataFolderOf(folder), error);
        if (error)
            return std::nullopt;

        Project project{ .folder = folder, .name = folderName(folder) };
        writeProject(project);
        return project;
    }

    void writeProject(const Project& project)
    {
        ProjectDocument document{
            project.dataFolder() / k_projectFileName,
            Dom::AutoSave::No,
            projectLayout(),
            Dom::WriteDefaults::Yes
        };
        (document / Keys::name).set(project.name);
        document.save();
    }
}
