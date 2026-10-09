module SeeDocs_App.Cli;

import SeeDocs_App.Checks;
import SeeDocs_App.Database;
import SeeDocs_App.Project;
import SeeDocs_App.Scanner;
import SeeDocs_App.Surface;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_scanCommand = L"scan";
        constexpr std::wstring_view k_checkCommand = L"check";
        constexpr std::wstring_view k_outOption = L"--out";
        constexpr int k_ok = 0;
        constexpr int k_failed = 1;
        constexpr int k_misused = 2;

        constexpr std::wstring_view k_usage =
            L"SeeDocsCli - reads the ClaFi design surface out of the tree.\n"
            L"\n"
            L"    SeeDocsCli scan [tree] [--out file]   writes the tree's .seedocs/Surface.cfg\n"
            L"    SeeDocsCli check [tree]               reports deviations, exit code 1 on errors\n"
            L"\n"
            L"The tree is the folder of sources, read as it stands; the working directory when\n"
            L"left out. A project set to limited mode - Limited = true in .seedocs/Project.cfg -\n"
            L"is read as anyone's C++, headers included, and is not checked.\n";

        // The process's text goes out as UTF-8, whichever platform the console belongs to.
        void print(const std::wstring_view text)
        {
            const std::string utf8 = toUtf8(text);
            std::cout.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
            std::cout.flush();
        }

        // What the arguments after the command say. An empty output is the tree's own database.
        struct Options
        {
            std::filesystem::path tree;
            std::filesystem::path output;
            bool valid{ true };
        };

        [[nodiscard]] Options readOptions(const Arguments& arguments)
        {
            Options options;
            for (std::size_t i = 2; i < arguments.size(); ++i)
            {
                const std::wstring& argument = arguments[i];
                if (argument == k_outOption)
                {
                    if (i + 1 >= arguments.size())
                    {
                        options.valid = false;
                        break;
                    }
                    options.output = arguments[i + 1];
                    ++i;
                }
                else if (options.tree.empty())
                    options.tree = argument;
                else
                    options.valid = false;
            }
            // Absolute, so a bare file name has a folder the writer can make sure of.
            std::error_code error;
            options.tree = std::filesystem::absolute(options.tree, error);
            if (!options.output.empty())
                options.output = std::filesystem::absolute(options.output, error);
            return options;
        }

        [[nodiscard]] std::size_t classCount(const Surface& surface)
        {
            std::size_t count = 0;
            for (const Type& type : surface.types())
            {
                const bool nested = type.name.find(L"::") != std::wstring::npos;
                if (type.isPublic() && type.kind == TypeKind::Class && !nested)
                    ++count;
            }
            return count;
        }

        // A scan into the tree's own database makes the tree a project: the command names the
        // tree, which is the consent SeeDocs asks for in a dialog.
        // The tree is read as its project says; a tree that is no project is read in full.
        [[nodiscard]] ScanMode modeOf(const std::filesystem::path& tree)
        {
            const std::optional<Project> project = readProject(tree);
            return project ? project->mode : ScanMode::Full;
        }

        [[nodiscard]] int scan(const Options& options)
        {
            std::filesystem::path output = options.output;
            if (output.empty())
            {
                std::optional<Project> project = readProject(options.tree);
                if (!project)
                    project = createProject(options.tree, ScanMode::Full);
                if (!project)
                {
                    print(L"Cannot make " + dataFolderOf(options.tree).wstring() + L"\n");
                    return k_failed;
                }
                output = project->databasePath();
            }
            const Surface surface = scanTree(options.tree, modeOf(options.tree));
            writeDatabase(surface, output);
            print(std::to_wstring(classCount(surface)) + L" classes, "
                + std::to_wstring(surface.types().size()) + L" types, "
                + std::to_wstring(surface.functions().size()) + L" functions -> "
                + output.wstring() + L"\n");
            return k_ok;
        }

        // Limited mode holds a tree to nothing: what the checks ask is the routine's.
        [[nodiscard]] int check(const Options& options)
        {
            if (modeOf(options.tree) == ScanMode::Limited)
            {
                print(options.tree.wstring() + L" is read in limited mode, which checks nothing\n");
                return k_ok;
            }
            const Surface surface = scanTree(options.tree, ScanMode::Full);
            const Report report = checkSurface(surface, footnotesFolderOf(options.tree));
            print(formatReport(report, surface));
            print(L"\n" + std::to_wstring(classCount(surface)) + L" classes, "
                + std::to_wstring(report.errorCount()) + L" errors, "
                + std::to_wstring(report.warningCount()) + L" warnings\n");
            return report.errorCount() == 0 ? k_ok : k_failed;
        }
    }

    int run(const Arguments& arguments)
    {
        if (arguments.size() < 2)
        {
            print(k_usage);
            return k_misused;
        }
        const std::wstring& command = arguments[1];
        const Options options = readOptions(arguments);
        if (!options.valid || (command != k_scanCommand && command != k_checkCommand))
        {
            print(k_usage);
            return k_misused;
        }
        std::error_code error;
        if (!std::filesystem::is_directory(options.tree, error))
        {
            print(L"No such folder: " + options.tree.wstring() + L"\n");
            return k_misused;
        }
        if (command == k_scanCommand)
            return scan(options);
        return check(options);
    }
}
