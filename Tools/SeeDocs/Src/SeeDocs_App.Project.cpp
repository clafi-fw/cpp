module SeeDocs_App.Project;

import SeeDocs_App.Surface;

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
            constexpr std::wstring_view sources = L"Sources";
            constexpr std::wstring_view path = L"Path";
            constexpr std::wstring_view subfolders = L"Subfolders";
            constexpr std::wstring_view limited = L"Limited";
            constexpr std::wstring_view looseHints = L"LooseHints";
            constexpr std::wstring_view editHints = L"EditHints";
        }

        constexpr std::wstring_view k_parentStep = L"..";

        using ProjectDocument = Dom::Document<Dom::FileFormat::ClaFi>;
        using Sections = Dom::Sequence<Dom::Section>;

        [[nodiscard]] Dom::Dt::Section projectLayout()
        {
            using namespace Dom::Dt;
            return Section{
                Value{ Keys::name, std::wstring{} },
                Sequence{
                    Keys::sources,
                    Section{
                        Value{ Keys::path, std::wstring{} },
                        Value{ Keys::subfolders, true }
                    }
                },
                Value{ Keys::limited, false },
                Value{ Keys::looseHints, false },
                Value{ Keys::editHints, false }
            };
        }

        // One spelling for a folder: absolute, normal, and without a trailing separator.
        [[nodiscard]] std::filesystem::path cleanFolder(const std::filesystem::path& folder)
        {
            std::error_code error;
            std::filesystem::path result = std::filesystem::absolute(folder, error);
            if (error)
                result = folder;
            result = result.lexically_normal();
            if (!result.has_filename())
                result = result.parent_path();
            return result;
        }

        [[nodiscard]] bool isTreeProject(const std::filesystem::path& folder)
        {
            return folder.filename().wstring() == k_treeProjectFolder;
        }

        // The name a project goes by where it states none: its folder's, or for a tree's project
        // folder, the tree's.
        [[nodiscard]] std::wstring nameOfFolder(const std::filesystem::path& folder)
        {
            if (isTreeProject(folder))
                return folder.parent_path().filename().wstring();
            return folder.filename().wstring();
        }

        // How many folders deep a path stands under the root of its volume.
        [[nodiscard]] std::size_t depthOf(const std::filesystem::path& path)
        {
            std::size_t depth = 0;
            for (const std::filesystem::path& part : path.relative_path())
            {
                if (!part.empty())
                    ++depth;
            }
            return depth;
        }

        // A SOURCE IS WRITTEN RELATIVE TO THE PROJECT while the two share a folder under the
        // root of their volume - a tree's project and its tree, a project kept beside the
        // sources - so that they move together, and a project kept in a repository names no
        // place outside it. Any other is written as it stands.
        [[nodiscard]] std::wstring storedPath(const std::filesystem::path& source,
            const std::filesystem::path& folder)
        {
            const std::filesystem::path relative = source.lexically_relative(folder);
            std::size_t climbs = 0;
            for (const std::filesystem::path& part : relative)
            {
                if (part.wstring() == k_parentStep)
                    ++climbs;
            }
            if (relative.empty() || climbs >= depthOf(folder))
                return source.generic_wstring();
            return relative.generic_wstring();
        }

        // A source as it was written, read against the project's folder.
        [[nodiscard]] std::filesystem::path sourcePath(const std::wstring& stored,
            const std::filesystem::path& folder)
        {
            const std::filesystem::path written{ stored };
            return cleanFolder(written.is_absolute() ? written : folder / written);
        }
    }

    // Project

    std::filesystem::path Project::databasePath() const
    {
        return folder / k_databaseFileName;
    }

    std::filesystem::path Project::footnotesFolder() const
    {
        return folder / k_footnotesFolder;
    }

    SourceRoots Project::sourceRoots() const
    {
        SourceRoots roots;
        for (const SourceFolder& source : sources)
            roots.push_back(source.path);
        return roots;
    }

    std::wstring Project::folderName() const
    {
        return nameOfFolder(folder);
    }

    bool Project::writesHints() const
    {
        return edits == HintEdits::Direct && hints == HintRules::Strict;
    }

    std::optional<std::filesystem::path> projectFolderOf(const std::filesystem::path& named)
    {
        const std::filesystem::path folder = cleanFolder(named);
        std::error_code error;
        if (std::filesystem::is_regular_file(folder / k_projectFileName, error))
            return folder;
        if (isTreeProject(folder) && std::filesystem::is_directory(folder, error))
            return folder;
        const std::filesystem::path inTree = folder / k_treeProjectFolder;
        if (std::filesystem::is_directory(inTree, error))
            return inTree;
        return std::nullopt;
    }

    // A TREE'S PROJECT FOLDER THAT NAMES NO SOURCES READS ITS TREE, with the folders under it -
    // which is what one made before projects named their sources read. One that states nothing
    // of its hints either, as such a one does not, has the hints its read made of them: loose
    // and only read for a third-party library, strict and written for a tree of the routine's.
    // Nothing is written for it here; the next change of its properties writes them so. A folder
    // with no properties in it, or with no name in them, is a project still, named after its
    // folder.
    std::optional<Project> readProject(const std::filesystem::path& named)
    {
        const std::optional<std::filesystem::path> found = projectFolderOf(named);
        if (!found)
            return std::nullopt;

        const std::filesystem::path& folder = *found;
        Project project{ .folder = folder, .name = nameOfFolder(folder) };
        bool hintsStated = false;
        ProjectDocument document{
            folder / k_projectFileName,
            Dom::AutoSave::No,
            projectLayout(),
            Dom::WriteDefaults::Yes
        };
        if (document.load())
        {
            const std::wstring stated = (document / Keys::name).get<std::wstring>();
            if (!stated.empty())
                project.name = stated;
            for (const Dom::Section& entry : (document / Keys::sources).as<Sections>())
            {
                const std::wstring stored = (entry / Keys::path).get<std::wstring>();
                if (stored.empty())
                    continue;
                project.sources.push_back({
                    .path = sourcePath(stored, folder),
                    .subfolders = (entry / Keys::subfolders).get<bool>()
                });
            }
            if ((document / Keys::limited).get<bool>())
                project.mode = ScanMode::Limited;
            if ((document / Keys::looseHints).get<bool>())
                project.hints = HintRules::Loose;
            if ((document / Keys::editHints).get<bool>())
                project.edits = HintEdits::Direct;
            hintsStated = project.hints != HintRules::Strict || project.edits != HintEdits::None;
        }

        if (isTreeProject(folder) && project.sources.empty())
        {
            project.sources.push_back({ .path = folder.parent_path() });
            if (!hintsStated)
            {
                const bool limited = project.mode == ScanMode::Limited;
                project.hints = limited ? HintRules::Loose : HintRules::Strict;
                project.edits = limited ? HintEdits::None : HintEdits::Direct;
            }
        }
        return project;
    }

    // What was written is what is answered, read back.
    std::optional<Project> createProject(const std::filesystem::path& named,
        const std::wstring_view name)
    {
        const std::filesystem::path folder = cleanFolder(named);
        std::error_code error;
        std::filesystem::create_directories(folder, error);
        if (error)
            return std::nullopt;

        Project project{
            .folder = folder,
            .name = std::wstring{ name },
            .mode = ScanMode::Limited,
            .hints = HintRules::Loose
        };
        writeProject(project);
        return readProject(folder);
    }

    // A name that is the folder's own is not written, so that the project goes on following its
    // folder's name.
    void writeProject(const Project& project)
    {
        ProjectDocument document{
            project.folder / k_projectFileName,
            Dom::AutoSave::No,
            projectLayout(),
            Dom::WriteDefaults::Yes
        };
        const bool folderNamed = project.name == project.folderName();
        (document / Keys::name).set(folderNamed ? std::wstring{} : project.name);
        Sections& sources = (document / Keys::sources).as<Sections>();
        for (const SourceFolder& source : project.sources)
        {
            Dom::Section& entry = sources.add();
            (entry / Keys::path).set(storedPath(source.path, project.folder));
            (entry / Keys::subfolders).set(source.subfolders);
        }
        (document / Keys::limited).set(project.mode == ScanMode::Limited);
        (document / Keys::looseHints).set(project.hints == HintRules::Loose);
        (document / Keys::editHints).set(project.edits == HintEdits::Direct);
        document.save();
    }
}
