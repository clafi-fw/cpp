module ClaFi.App.ConsoleApplication;

import ClaFi.Core.System.Animation;

import ClaFi.StdLib;

namespace ClaFi
{
    ConsoleApplication::ConsoleApplication(const ApplicationProperties& properties,
        const std::span<const CommandLine::Option> options, std::vector<std::wstring> arguments)
        :
        m_properties{ properties },
        m_commandLine{ std::move(arguments), properties.name.plainText(), properties.version,
            properties.description.plainText(), options }
    {
        // A console process has no loop to drive timers, so nothing it builds arms one.
        AnimationController::usePlatformTimer(false);
    }
}
