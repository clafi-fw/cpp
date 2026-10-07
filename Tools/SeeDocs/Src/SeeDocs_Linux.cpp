import SeeDocs_App.Main;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

// THE WHOLE OF WHAT THIS ENTRY POINT KNOWS ABOUT LINUX: the arguments arrive as UTF-8 and are
// widened once, here. The commands are in SeeDocs_App.Main, beside the Win32 entry point that
// names them the same way.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build --target SeeDocs
//   ./build/SeeDocs check .

int main(int argc, char** argv)
{
    SeeDocs_App::Arguments arguments;
    for (int i = 0; i != argc; ++i)
        arguments.push_back(ClaFi::fromUtf8(argv[i]));

    return SeeDocs_App::run(arguments);
}
