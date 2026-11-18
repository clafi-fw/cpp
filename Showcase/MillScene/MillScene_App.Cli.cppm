export module MillScene_App.Cli;

export import MillScene_App.Options;

import MillScene_App.Render;

import ClaFi.App.CommandLine;
import ClaFi.App.ConsoleApplication;

import ClaFi.StdLib;

namespace MillScene_App
{
    // Reads the parsed command line, renders what it names, and answers the exit code.
    export [[nodiscard]] int runMillSceneCli(::ClaFi::ConsoleApplication&);

    int runMillSceneCli(ClaFi::ConsoleApplication& application)
    {
        if (const std::optional<int> answer = ClaFi::answerCommandLine(application.commandLine()))
            return *answer;

        std::wstring error{};
        const std::optional<RenderOptions> options =
            renderOptionsOf(application.commandLine(), error);
        if (!options)
        {
            ClaFi::writeErrorToConsole(error);
            ClaFi::writeErrorToConsole(L"\n");
            ClaFi::writeErrorToConsole(application.commandLine().usage());
            return ClaFi::ExitCode::Misused;
        }
        return renderLaunchFrames(*options);
    }
}
