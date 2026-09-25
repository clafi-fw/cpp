import ThisApp.Main;

import ClaFi.Platform.Windows;

// THE WHOLE OF WHAT THIS PROJECT KNOWS ABOUT WINDOWS: the entry point the operating system calls,
// the instance handle it hands over, and the two types the application is built out of. The
// application itself is in ThisApp.Main and names neither.
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, wchar_t* /*lpCmdLine*/, int /*nCmdShow*/)
{
    using namespace ClaFi;

    ThisApp::ThemesApplication<Win32Platform, Direct2DBackend> app{ { hInstance } };

    return app.run();
}
