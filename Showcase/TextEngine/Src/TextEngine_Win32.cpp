import TextEngine_App.Main;

import ClaFi.App.Application;
import ClaFi.Platform.Windows;

import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Cpu.Canvas;

// THE WHOLE OF WHAT THIS PROJECT KNOWS ABOUT WINDOWS: the entry point the operating system calls,
// the instance handle it hands over, and the two types the application is built out of. The
// showcase itself is in TextEngine_App.Main and names neither.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, wchar_t* /*lpCmdLine*/, int /*nCmdShow*/)
{
    using namespace ClaFi;

    Win32Application app{
        { hInstance },
        TextEngine_App::textEngineAppParams(),
        Dom::Dt::Section{}
    };

    return TextEngine_App::runTextEngineShowcase(app);
}
