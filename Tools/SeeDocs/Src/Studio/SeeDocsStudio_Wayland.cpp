import SeeDocs_App.Studio.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Wayland;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

// THE WHOLE OF WHAT THIS ENTRY POINT KNOWS ABOUT WAYLAND: the two types the application is built
// out of, and the arguments arriving as UTF-8. The studio itself is in SeeDocs_App.Studio.Main,
// beside the Win32 entry point that names it the same way.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build --target SeeDocsStudio
//   ./build/SeeDocsStudio .

int main(int argc, char** argv)
{
    using namespace ClaFi;

    WaylandApplication app{
        WaylandPlatform::Params{},
        SeeDocs_App::studioParams(),
        Dom::Dt::Section{}
    };

    std::filesystem::path stated;
    if (argc > 1)
        stated = std::filesystem::path{ fromUtf8(argv[1]) };

    return SeeDocs_App::runStudio(app, SeeDocs_App::findTree(stated));
}
