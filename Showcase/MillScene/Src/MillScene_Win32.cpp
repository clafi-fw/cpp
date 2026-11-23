#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

import MillScene_App.Main;

import ClaFi.Platform.Windows;

import ClaFi.StdLib;

// **The whole of what this project knows about Windows**: the entry point the operating system
// calls, the instance handle it hands over, and the type the application is built out of. The
// showcase itself is in MillScene_App.Main and names neither.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, wchar_t* /*lpCmdLine*/, int /*nCmdShow*/)
{
    using namespace ClaFi;

    // The arguments as the operating system split them, quoted the way it quotes them: a theme
    // named "Clear Day" arrives as one argument.
    int argumentCount = 0;
    wchar_t** argumentList = ::CommandLineToArgvW(::GetCommandLineW(), &argumentCount);
    std::vector<std::wstring> arguments{};
    for (int index = 1; index < argumentCount; ++index)
        arguments.emplace_back(argumentList[index]);
    if (argumentList)
        ::LocalFree(argumentList);

    MillScene_App::MillSceneApplication<Win32Platform, Direct2DBackend> app{
        { .appInstance = hInstance },
        std::move(arguments)
    };

    return app.run();
}
