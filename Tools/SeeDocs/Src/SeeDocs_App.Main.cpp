module SeeDocs_App.Main;

import SeeDocs_App.Checks;
import SeeDocs_App.Database;
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
        constexpr std::wstring_view k_defaultOutput = L"Tools/SeeDocs/Surface.cfg";
        constexpr int k_ok = 0;
        constexpr int k_failed = 1;
        constexpr int k_misused = 2;

        constexpr std::wstring_view k_usage =
            L"SeeDocs - reads the ClaFi design surface out of the tree.\n"
            L"\n"
            L"    seedocs scan [tree] [--out file]    writes Tools/SeeDocs/Surface.cfg\n"
            L"    seedocs check [tree]                reports deviations, exit code 1 on errors\n"
            L"\n"
            L"The tree is the folder holding Source; the working directory when left out.\n";

        // The process's text goes out as UTF-8, whichever platform the console belongs to.
        void print(const std::wstring_view text)
        {
            const std::string utf8 = toUtf8(text);
            std::cout.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
            std::cout.flush();
        }

        // What the arguments after the command say.
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
            if (options.output.empty())
                options.output = options.tree / k_defaultOutput;
            options.output = std::filesystem::absolute(options.output, error);
            return options;
        }

        [[nodiscard]] bool treeFits(const std::filesystem::path& tree)
        {
            std::error_code error;
            return std::filesystem::is_directory(tree / k_sourceFolder, error);
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

        [[nodiscard]] int scan(const Options& options)
        {
            const Surface surface = scanTree(options.tree);
            writeDatabase(surface, options.output);
            print(std::to_wstring(classCount(surface)) + L" classes, "
                + std::to_wstring(surface.types().size()) + L" types, "
                + std::to_wstring(surface.functions().size()) + L" functions -> "
                + options.output.wstring() + L"\n");
            return k_ok;
        }

        [[nodiscard]] int check(const Options& options)
        {
            const Surface surface = scanTree(options.tree);
            const Report report = checkSurface(surface, options.tree);
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
        if (!treeFits(options.tree))
        {
            print(L"No Source folder under " + options.tree.wstring() + L"\n");
            return k_misused;
        }
        if (command == k_scanCommand)
            return scan(options);
        return check(options);
    }
}
