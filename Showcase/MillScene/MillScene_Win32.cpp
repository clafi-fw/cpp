import ClaFi.Showcase.MillScene.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Windows;

import ClaFi.Core.Foundation;

// THE WHOLE OF WHAT THIS PROJECT KNOWS ABOUT WINDOWS: the entry point the operating system calls,
// the instance handle it hands over, and the type the application is built out of. The showcase
// itself is in ClaFi.Showcase.MillScene.Main and names neither.
//
// DIRECT2D COMES WITH Win32Application, beside the CPU backend, and GPU acceleration in Settings
// picks which of the two draws the scene.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, wchar_t* /*lpCmdLine*/, int /*nCmdShow*/)
{
    using namespace ClaFi;

    Win32Application app{
        { .appInstance = hInstance },
        Showcase::millSceneAppParams(),
        Showcase::createMillSceneConfigSchema()
    };

    return Showcase::runMillSceneShowcase(app);
}
