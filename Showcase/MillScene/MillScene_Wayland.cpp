// **The whole of what this entry point knows about Wayland**: the two types the application is
// built out of. The showcase itself is in MillScene_App.Main, beside the Win32 entry point that
// names it the same way. A missing compositor is thrown from the platform's constructor, naming
// what was looked for, and ends the process as an uncaught exception does.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build
//   ./build/MillScene

import MillScene_App.Main;
import MillScene_App.Options;

import ClaFi.App.WindowApplication;
import ClaFi.Platform.Wayland;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

int main(int argumentCount, char** argumentList)
{
    using namespace ClaFi;

    // The arguments arrive as bytes; the showcase reads them as the text they stand for.
    std::vector<std::wstring> arguments{};
    for (int index = 1; index < argumentCount; ++index)
        arguments.push_back(fromUtf8(argumentList[index]));

    WaylandApplication app{
        WaylandPlatform::Params{},
        MillScene_App::millSceneApplicationProperties(),
        MillScene_App::createMillSceneConfigSchema(),
        MillScene_App::k_windowOptions,
        std::move(arguments)
    };

    return MillScene_App::runMillSceneShowcase(app);
}
