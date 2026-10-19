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
        constexpr std::wstring_view k_timeOption = L"--time";
        constexpr std::wstring_view k_totalLabel = L"total";
        constexpr int k_ok = 0;
        constexpr int k_failed = 1;
        constexpr int k_misused = 2;

        constexpr std::wstring_view k_usage =
            L"SeeDocsCli - reads the design surface out of a project's sources.\n"
            L"\n"
            L"    SeeDocsCli scan [folder] [--out file] [--time] "
            L"writes the project's Surface.cfg\n"
            L"    SeeDocsCli check [folder]                      "
            L"reports deviations, exit code 1 on errors\n"
            L"\n"
            L"The folder is a project's own, holding its Project.cfg; the working\n"
            L"directory when left out. A project reads the folders of sources its Project.cfg\n"
            L"names, as it says; one with ReadAs = AnyCppAndDelphi is not\n"
            L"checked. A scan with --out takes any other folder too, read as ClaFi modules; one\n"
            L"with --time prints how long each step of the scan took, then the write of the\n"
            L"database and the read of it SeeDocs shows a scan from.\n";

        // The process's text goes out as UTF-8, whichever platform the console belongs to.
        void print(const std::wstring_view text)
        {
            const std::string utf8 = toUtf8(text);
            std::cout.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
            std::cout.flush();
        }

        // What the arguments after the command say. An empty output is the project's database.
        struct Options
        {
            std::filesystem::path folder;
            std::filesystem::path output;
            bool timed{ false };
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
                else if (argument == k_timeOption)
                    options.timed = true;
                else if (options.folder.empty())
                    options.folder = argument;
                else
                    options.valid = false;
            }
            // Absolute, so a bare file name has a folder the writer can make sure of.
            std::error_code error;
            options.folder = std::filesystem::absolute(options.folder, error).lexically_normal();
            if (!options.output.empty())
                options.output = std::filesystem::absolute(options.output, error);
            return options;
        }

        [[nodiscard]] double seconds(const std::chrono::steady_clock::duration took)
        {
            return std::chrono::duration<double>{ took }.count();
        }

        // A line per step with the seconds aligned, and their total last.
        [[nodiscard]] std::wstring stepsText(const ScanSteps& steps)
        {
            std::size_t width = k_totalLabel.size();
            for (const ScanStep& step : steps)
                width = std::max(width, step.name.size());
            std::wstring text;
            std::chrono::steady_clock::duration total{};
            for (const ScanStep& step : steps)
            {
                text += std::format(L"{:<{}}  {:>8.3f} s\n", step.name, width, seconds(step.took));
                total += step.took;
            }
            text += std::format(L"{:<{}}  {:>8.3f} s\n", k_totalLabel, width, seconds(total));
            return text;
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

        // The project the folder stands for; for a folder that stands for none, one reading it
        // as ClaFi modules, with no folder of its own.
        [[nodiscard]] Project projectOf(const std::filesystem::path& folder)
        {
            if (std::optional<Project> project = readProject(folder))
                return std::move(*project);
            return Project{
                .sources = { { .written = folder.wstring() } },
                .mode = ScanMode::ClaFiModules
            };
        }

        [[nodiscard]] int scan(const Options& options)
        {
            const Project project = projectOf(options.folder);
            std::filesystem::path output = options.output;
            if (output.empty() && project.folder.empty())
            {
                print(L"No project stands for " + options.folder.wstring()
                    + L" - name a project's folder, or the file to write with --out\n");
                return k_misused;
            }
            if (output.empty())
                output = project.databasePath();
            ScanSteps steps;
            ScanSteps* timedSteps = options.timed ? &steps : nullptr;
            const Surface surface = scanSources(project.sourceFolders(), project.mode,
                project.hints, timedSteps);
            const Database database{ surface, timedSteps };
            if (!database.save(output))
            {
                print(L"The database could not be written to " + output.wstring() + L"\n");
                return k_failed;
            }
            print(std::to_wstring(classCount(surface)) + L" classes, "
                + std::to_wstring(surface.types().size()) + L" types, "
                + std::to_wstring(surface.functions().size()) + L" functions -> "
                + output.wstring() + L"\n");
            if (options.timed)
            {
                const Surface shown = database.surface();
                print(stepsText(steps));
            }
            return k_ok;
        }

        // A project reading any C++ is held to nothing: what the checks ask is the routine's. The
        // footnotes the references reach are a project's, so a folder that is none is refused.
        [[nodiscard]] int check(const Options& options)
        {
            const Project project = projectOf(options.folder);
            if (project.folder.empty())
            {
                print(L"No project stands for " + options.folder.wstring() + L"\n");
                return k_misused;
            }
            if (project.mode == ScanMode::AnyCppAndDelphi)
            {
                print(project.name + L" reads any C++ and Delphi, which checks nothing\n");
                return k_ok;
            }
            const Surface surface = scanSources(project.sourceFolders(), ScanMode::ClaFiModules,
                project.hints);
            const Report report = checkSurface(surface, project.footnotesFolder());
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
        if (!std::filesystem::is_directory(options.folder, error))
        {
            print(L"No such folder: " + options.folder.wstring() + L"\n");
            return k_misused;
        }
        if (command == k_scanCommand)
            return scan(options);
        return check(options);
    }
}
