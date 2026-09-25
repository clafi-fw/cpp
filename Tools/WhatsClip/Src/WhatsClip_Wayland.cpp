// THE WHOLE OF WHAT THIS ENTRY POINT KNOWS ABOUT WAYLAND: the two types the application is built
// out of. The viewer itself is in ClaFi.Tools.WhatsClip.Main, beside the Win32 entry point that
// names it the same way. A missing compositor is thrown from the platform's constructor, naming
// what was looked for, and ends the process as an uncaught exception does.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build
//   ./build/WhatsClip

import ClaFi.Tools.WhatsClip.Encodings;
import ClaFi.Tools.WhatsClip.Languages;
import ClaFi.Tools.WhatsClip.Main;
import ClaFi.Tools.WhatsClip.Page;

import ClaFi.App.Application;
import ClaFi.Platform.Wayland;

import ClaFi.Core.Foundation;

int main()
{
    using namespace ClaFi;

    // WATCHING RATHER THAN PASTING. Without this the window is told what is on the clipboard
    // only as it takes the keyboard focus, which for a viewer of the clipboard is the one moment
    // it is least worth being told - see WaylandPlatform::Params.
    WaylandApplication app{
        WaylandPlatform::Params{ .watchesClipboard = true },
        Tools::WhatsClip::applicationParams(),
        Dom::Dt::Section{}
    };

    // What the text pages read bytes in and colour text in. An application built over this
    // viewer appends its own here: an encoding with the claim that tells its bytes apart, a
    // language with its own detector.
    const Tools::WhatsClip::PickLists picks{
        Tools::WhatsClip::defaultEncodings(),
        Tools::WhatsClip::defaultLanguages()
    };

    return Tools::WhatsClip::runWhatsClip(app, picks);
}
