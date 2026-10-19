module SeeDocs_App.Project;

import SeeDocs_App.Database;
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
            constexpr std::wstring_view Name = L"Name";
            constexpr std::wstring_view Sources = L"Sources";
            constexpr std::wstring_view Path = L"Path";
            constexpr std::wstring_view Subfolders = L"Subfolders";
            constexpr std::wstring_view ReadAs = L"ReadAs";
            constexpr std::wstring_view LooseHints = L"LooseHints";
            constexpr std::wstring_view EditHints = L"EditHints";
        }

        constexpr std::wstring_view k_parentStep = L"..";

        using ProjectDocument = Dom::Document<Dom::FileFormat::ClaFi>;
        using Sections = Dom::Sequence<Dom::Section>;

        [[nodiscard]] Dom::Dt::Section projectLayout()
        {
            using namespace Dom::Dt;
            return Section{
                Value{ Keys::Name, std::wstring{} },
                Sequence{
                    Keys::Sources,
                    Section{
                        Value{ Keys::Path, std::wstring{} },
                        Value{ Keys::Subfolders, true }
                    }
                },
                Value{ Keys::ReadAs, std::wstring{} },
                Value{ Keys::LooseHints, false },
                Value{ Keys::EditHints, false }
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

        // **A folder picked is written relative to the project** while the two share a folder
        // under the root of their volume - a project kept inside its sources or beside them - so
        // that they move together, and a project kept in a repository names no place outside it.
        // Any other is written as it stands.
        [[nodiscard]] std::wstring storedPath(const std::filesystem::path& picked,
            const std::filesystem::path& folder)
        {
            const std::filesystem::path relative = picked.lexically_relative(folder);
            std::size_t climbs = 0;
            for (const std::filesystem::path& part : relative)
            {
                if (part.wstring() == k_parentStep)
                    ++climbs;
            }
            std::filesystem::path written = relative.empty() || climbs >= depthOf(folder)
                ? picked
                : relative;
            return written.make_preferred().wstring();
        }

        // **A backslash parts folders on every desktop**, so a source typed on Windows names the
        // same folder where the project is read on Linux.
        [[nodiscard]] std::filesystem::path sourcePath(const std::wstring_view written,
            const std::filesystem::path& folder)
        {
            std::wstring separated{ written };
            std::ranges::replace(separated, L'\\', L'/');
            const std::filesystem::path path{ separated };
            return cleanFolder(path.is_absolute() ? path : folder / path);
        }
    }

    // Project

    std::filesystem::path Project::databasePath() const
    {
        return folder / k_databaseFileName;
    }

    std::filesystem::path Project::scanCachePath() const
    {
        return folder / k_scanCacheFileName;
    }

    std::filesystem::path Project::footnotesFolder() const
    {
        return folder / k_footnotesFolder;
    }

    SourceFolders Project::sourceFolders() const
    {
        SourceFolders folders;
        for (const ProjectSource& source : sources)
        {
            folders.push_back({
                .path = readSource(source.written),
                .subfolders = source.subfolders
            });
        }
        return folders;
    }

    SourceRoots Project::sourceRoots() const
    {
        SourceRoots roots;
        for (const ProjectSource& source : sources)
            roots.push_back(readSource(source.written));
        return roots;
    }

    std::wstring Project::writtenSource(const std::filesystem::path& picked) const
    {
        return storedPath(cleanFolder(picked), folder);
    }

    std::filesystem::path Project::readSource(const std::wstring_view written) const
    {
        return sourcePath(written, folder);
    }

    std::wstring Project::folderName() const
    {
        return folder.filename().wstring();
    }

    bool Project::writesHints() const
    {
        return edits == HintEdits::Direct && hints == HintRules::Strict;
    }

    // **A project reads what its properties name** and nothing besides: one kept inside the tree
    // it reads names the tree as `..`, as any other source. Properties that state no name leave
    // the project named after its folder.
    std::optional<Project> readProject(const std::filesystem::path& named)
    {
        const std::filesystem::path folder = cleanFolder(named);
        std::error_code error;
        if (!std::filesystem::is_regular_file(folder / k_projectFileName, error))
            return std::nullopt;

        Project project{ .folder = folder, .name = folder.filename().wstring() };
        ProjectDocument document{
            folder / k_projectFileName,
            Dom::AutoSave::No,
            projectLayout(),
            Dom::WriteDefaults::Yes
        };
        if (document.load())
        {
            const std::wstring stated = (document / Keys::Name).get<std::wstring>();
            if (!stated.empty())
                project.name = stated;
            for (const Dom::Section& entry : (document / Keys::Sources).as<Sections>())
            {
                std::wstring written = (entry / Keys::Path).get<std::wstring>();
                if (written.empty())
                    continue;
                project.sources.push_back({
                    .written = std::move(written),
                    .subfolders = (entry / Keys::Subfolders).get<bool>()
                });
            }
            project.mode = scanModeOf((document / Keys::ReadAs).get<std::wstring>())
                .value_or(ScanMode::AnyCppAndDelphi);
            if ((document / Keys::LooseHints).get<bool>())
                project.hints = HintRules::Loose;
            if ((document / Keys::EditHints).get<bool>())
                project.edits = HintEdits::Direct;
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
        (document / Keys::Name).set(folderNamed ? std::wstring{} : project.name);
        Sections& sources = (document / Keys::Sources).as<Sections>();
        for (const ProjectSource& source : project.sources)
        {
            Dom::Section& entry = sources.add();
            (entry / Keys::Path).set(source.written);
            (entry / Keys::Subfolders).set(source.subfolders);
        }
        (document / Keys::ReadAs).set(std::wstring{ modeWord(project.mode) });
        (document / Keys::LooseHints).set(project.hints == HintRules::Loose);
        (document / Keys::EditHints).set(project.edits == HintEdits::Direct);
        document.save();
    }
}
