import SeeDocs_App.Main;

import ClaFi.Platform.Windows;

import ClaFi.StdLib;

// **The whole of what this project knows about Windows**: the entry point the operating system
// calls, the instance handle and the command line it hands over, and the two types the application
// is built out of. SeeDocs itself is in SeeDocs_App.Main and names none of them.
namespace
{
    // The command line as a project's folder: the one argument, its quotes taken off.
    [[nodiscard]] std::filesystem::path folderArgument(const wchar_t* commandLine)
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

    SeeDocs_App::Ui::SeeDocsApplication<Win32Platform, Direct2DBackend> app{ { hInstance } };

    return app.run(folderArgument(commandLine));
}
