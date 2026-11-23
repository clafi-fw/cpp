import TextEngine_App.Main;

import ClaFi.App.WindowApplication;
import ClaFi.Platform.Windows;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Cpu.Canvas;

import ClaFi.StdLib;

// **The whole of what this project knows about Windows**: the entry point the operating system
// calls, the instance handle it hands over, and the two types the application is built out of. The
// showcase itself is in TextEngine_App.Main and names neither.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, wchar_t* /*lpCmdLine*/, int /*nCmdShow*/)
{
    using namespace ClaFi;

    Win32Application app{
        { hInstance },
        TextEngine_App::textEngineAppParams(),
        Dom::Dt::Section{}
    };

    TextEngine_App::applyTextEngineDefaults(app);
    const std::unique_ptr<Form<TextEngine_App::MainForm>> form =
        app.createDialog<TextEngine_App::MainForm>();
    form->setConfigName(AppContext::k_mainFormName);
    return form->execute();
}
