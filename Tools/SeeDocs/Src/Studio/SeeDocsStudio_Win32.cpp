import SeeDocs_App.Studio.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Windows;

import ClaFi.StdLib;

// THE WHOLE OF WHAT THIS PROJECT KNOWS ABOUT WINDOWS: the entry point the operating system calls,
// the instance handle and the command line it hands over, and the two types the application is
// built out of. The studio itself is in SeeDocs_App.Studio.Main and names none of them.
namespace
{
    // The command line as the tree's folder: the one argument, its quotes taken off.
    [[nodiscard]] std::filesystem::path treeArgument(const wchar_t* commandLine)
    {
        std::wstring_view stated = commandLine ? commandLine : L"";
        while (stated.starts_with(L' '))
            stated.remove_prefix(1);
        while (stated.ends_with(L' '))
            stated.remove_suffix(1);
        if (stated.size() >= 2 && stated.starts_with(L'"') && stated.ends_with(L'"'))
            stated = stated.substr(1, stated.size() - 2);
        return std::filesystem::path{ stated };
    }
}

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t* commandLine, int)
{
    using namespace ClaFi;

    Win32Application app{
        Win32Platform::Params{ hInstance },
        SeeDocs_App::studioParams(),
        SeeDocs_App::createStudioConfigSchema()
    };

    return SeeDocs_App::runStudio(app, treeArgument(commandLine));
}
