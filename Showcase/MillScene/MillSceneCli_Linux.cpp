import MillScene_App.Cli;
import MillScene_App.Options;

import ClaFi.Platform.Linux.ConsoleApplication;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

// **The whole of what this console entry point knows about Linux**: the arguments arrive as UTF-8
// and are widened once, here. The application itself is in MillScene_App.Cli.
int main(int argumentCount, char** argumentList)
{
    std::vector<std::wstring> arguments{};
    for (int index = 1; index < argumentCount; ++index)
        arguments.push_back(ClaFi::fromUtf8(argumentList[index]));

    ClaFi::LinuxConsoleApplication app{
        MillScene_App::millSceneProperties(L"MillSceneCli"),
        MillScene_App::k_renderOptions,
        std::move(arguments)
    };
    return MillScene_App::runMillSceneCli(app);
}
