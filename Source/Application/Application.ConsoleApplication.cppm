export module ClaFi.App.ConsoleApplication;

export import ClaFi.App.ApplicationProperties;
export import ClaFi.App.CommandLine;

import ClaFi.Core.System.Animation;

import ClaFi.StdLib;

namespace ClaFi
{
    // A process with no window: what the application is, what its command line asked for, and
    // the console it answers on.
    export class ConsoleApplication
    {
    public:
        ConsoleApplication(const ApplicationProperties&, std::span<const CommandLine::Option>,
            std::vector<std::wstring> arguments);
        virtual ~ConsoleApplication() = default;
        ConsoleApplication(const ConsoleApplication&) = delete;
        ConsoleApplication& operator=(const ConsoleApplication&) = delete;
    public:
        [[nodiscard]] const CommandLine& commandLine() const { return m_commandLine; }
        [[nodiscard]] const ApplicationProperties& properties() const { return m_properties; }
    private:
        ApplicationProperties m_properties;
        CommandLine m_commandLine;
    };
}
