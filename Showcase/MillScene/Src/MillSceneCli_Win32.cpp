import MillScene_App.Cli;
import MillScene_App.Options;

import ClaFi.Platform.Windows;

import ClaFi.StdLib;

// **The whole of what this console entry point knows about Windows**: the arguments the console
// subsystem hands over as wide strings. The application itself is in MillScene_App.Cli.
int wmain(int argumentCount, wchar_t** argumentList)
{
    std::vector<std::wstring> arguments{};
    for (int index = 1; index < argumentCount; ++index)
        arguments.emplace_back(argumentList[index]);

    ClaFi::Win32ConsoleApplication app{
        MillScene_App::millSceneProperties(L"MillSceneCli"),
        MillScene_App::k_renderOptions,
        std::move(arguments)
    };
    return MillScene_App::runMillSceneCli(app);
}
