// THE WHOLE OF WHAT THIS ENTRY POINT KNOWS ABOUT WAYLAND: the two types the application is built
// out of. The application itself is in ThisApp.Main, beside the Win32 entry point that names it
// the same way. A missing compositor is thrown from the platform's constructor, naming what was
// looked for, and ends the process as an uncaught exception does.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build
//   ./build/Themes

import ThisApp.Main;

import ClaFi.Platform.Wayland;

int main()
{
    using namespace ClaFi;

    // No GPU backend is named, because this platform has none.
    ThisApp::ThemesApplication<WaylandPlatform> app{ WaylandPlatform::Params{} };

    return app.run();
}
