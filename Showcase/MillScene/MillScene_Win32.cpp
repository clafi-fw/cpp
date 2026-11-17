#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <cstdio>
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

import MillScene_App.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Windows;

import ClaFi.Core.Foundation;

import ClaFi.StdLib;

// **The whole of what this project knows about Windows**: the entry point the operating system
// calls, the instance handle it hands over, and the type the application is built out of. The
// showcase itself is in MillScene_App.Main and names neither.
//
// **Direct2D comes with** Win32Application, beside the CPU backend, and GPU acceleration in
// Settings picks which of the two draws the scene. A render draws through the CPU backend and
// opens no window, so it is decided before the application is built.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, wchar_t* /*lpCmdLine*/, int /*nCmdShow*/)
{
    using namespace ClaFi;

    // The arguments as the operating system split them, quoted the way it quotes them: a theme
    // named "Clear Day" arrives as one argument.
    int argumentCount = 0;
    wchar_t** argumentList = ::CommandLineToArgvW(::GetCommandLineW(), &argumentCount);
    std::vector<std::wstring_view> arguments{};
    for (int index = 1; index < argumentCount; ++index)
        arguments.emplace_back(argumentList[index]);

    std::wstring error{};
    const std::optional<MillScene_App::LaunchOptions> options =
        MillScene_App::parseLaunchOptions(arguments, error);
    if (argumentList)
        ::LocalFree(argumentList);

    if (!options)
    {
        MillScene_App::reportLaunchError(error);
        return 2;
    }
    if (options->help)
    {
        MillScene_App::reportLaunchUsage();
        return 0;
    }

    Win32Application app{
        { .appInstance = hInstance },
        MillScene_App::millSceneAppParams(),
        MillScene_App::createMillSceneConfigSchema()
    };

    return MillScene_App::runMillSceneShowcase(app, *options);
}
