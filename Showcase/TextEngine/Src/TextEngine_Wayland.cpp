// THE WHOLE OF WHAT THIS ENTRY POINT KNOWS ABOUT WAYLAND: the two types the application is built
// out of. The showcase itself is in ClaFi.Showcase.TextEngine.Main, beside the Win32 entry point
// that names it the same way. A missing compositor is thrown from the platform's constructor,
// naming what was looked for, and ends the process as an uncaught exception does.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build
//   ./build/TextEngine

import ClaFi.Showcase.TextEngine.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Wayland;

import ClaFi.Core.Foundation;

int main()
{
    using namespace ClaFi;

    WaylandApplication app{
        WaylandPlatform::Params{},
        Showcase::TextEngine::textEngineAppParams(),
        Dom::Dt::Section{}
    };

    return Showcase::TextEngine::runTextEngineShowcase(app);
}
