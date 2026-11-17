// **The whole of what this entry point knows about Wayland**: the two types the application is
// built out of. The showcase itself is in MillScene_App.Main, beside the Win32 entry point that
// names it the same way. A missing compositor is thrown from the platform's constructor, naming
// what was looked for, and ends the process as an uncaught exception does.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build
//   ./build/MillScene
//
// A render draws through the CPU backend and opens no window at all, so it needs no compositor.

import MillScene_App.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Wayland;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

int main(int argumentCount, char** argumentList)
{
    using namespace ClaFi;

    // The arguments arrive as bytes; the showcase reads them as the text they stand for.
    std::vector<std::wstring> owned{};
    for (int index = 1; index < argumentCount; ++index)
        owned.push_back(fromUtf8(argumentList[index]));
    std::vector<std::wstring_view> arguments{};
    for (const std::wstring& text : owned)
        arguments.emplace_back(text);

    std::wstring error{};
    const std::optional<MillScene_App::LaunchOptions> options =
        MillScene_App::parseLaunchOptions(arguments, error);
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

    WaylandApplication app{
        WaylandPlatform::Params{},
        MillScene_App::millSceneAppParams(),
        MillScene_App::createMillSceneConfigSchema()
    };

    return MillScene_App::runMillSceneShowcase(app, *options);
}
