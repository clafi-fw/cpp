// **The whole of what this entry point knows about Wayland**: the two types the application is
// built out of. The showcase itself is in TextEngine_App.Main, beside the Win32 entry point that
// names it the same way. A missing compositor is thrown from the platform's constructor, naming
// what was looked for, and ends the process as an uncaught exception does.
//
//   cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
//         -DCMAKE_CXX_COMPILER=clang++-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++
//   cmake --build build
//   ./build/TextEngine

import TextEngine_App.Main;

import ClaFi.App.WindowApplication;
import ClaFi.Platform.Wayland;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;

import ClaFi.StdLib;

int main()
{
    using namespace ClaFi;

    WaylandApplication app{
        WaylandPlatform::Params{},
        TextEngine_App::textEngineAppParams(),
        Dom::Dt::Section{}
    };

    TextEngine_App::applyTextEngineDefaults(app);
    const std::unique_ptr<Form<TextEngine_App::MainForm>> form =
        app.createDialog<TextEngine_App::MainForm>();
    form->setConfigName(AppContext::k_mainFormName);
    return form->execute();
}
