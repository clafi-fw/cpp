export module ClaFi.App.CommandLine;

import ClaFi.StdLib;

namespace ClaFi
{
    // What the application accepts from the command line, and what this run asked for.
    export class CommandLine
    {
    public:
        // One option the application declares.
        struct Option
        {
            std::wstring_view name; // --scene
            std::wstring_view valueName; // <name>, empty for a flag
            std::wstring_view help;
        };
    public:
        CommandLine(std::vector<std::wstring> arguments, std::wstring_view programName,
            std::wstring_view version, std::wstring_view description,
            std::span<const Option> options);
        ~CommandLine() = default;
        CommandLine(const CommandLine&) = default;
        CommandLine& operator=(const CommandLine&) = default;
        CommandLine(CommandLine&&) noexcept = default;
        CommandLine& operator=(CommandLine&&) noexcept = default;
    public:
        [[nodiscard]] bool help() const { return m_help; }
        [[nodiscard]] bool version() const { return m_versionAsked; }
        [[nodiscard]] const std::optional<std::wstring>& error() const { return m_error; }
        [[nodiscard]] bool has(std::wstring_view option) const;
        [[nodiscard]] std::optional<std::wstring_view> value(std::wstring_view option) const;
        [[nodiscard]] std::vector<std::wstring_view> positionals() const;
        // How the arguments are written, for --help and for one that cannot be read.
        [[nodiscard]] std::wstring usage() const;
        [[nodiscard]] std::wstring versionLine() const;
    private:
        // What the option at an index took.
        struct Taken
        {
            bool present{ false };
            std::optional<std::wstring> value{};
        };
    private:
        [[nodiscard]] std::optional<std::size_t> optionIndex(std::wstring_view name) const;
        [[nodiscard]] std::wstring optionLabel(const Option&) const;
    private:
        std::vector<std::wstring> m_arguments;
        std::span<const Option> m_options;
        std::wstring m_programName;
        std::wstring m_version;
        std::wstring m_description;
        std::vector<Taken> m_taken;
        std::vector<std::size_t> m_positionals;
        bool m_help{ false };
        bool m_versionAsked{ false };
        std::optional<std::wstring> m_error{};
    };

    // The exit codes a console process answers with.
    export namespace ExitCode
    {
        constexpr int Ok = 0;
        constexpr int Failed = 1;
        constexpr int Misused = 2;
    }

    // Writes the text as UTF-8 to the process's standard output.
    export void writeToConsole(std::wstring_view text);
    // Writes the text as UTF-8 to the process's standard error.
    export void writeErrorToConsole(std::wstring_view text);
    // Answers help, version or a command line that could not be read, or nothing where the
    // project should go on.
    export [[nodiscard]] std::optional<int> answerCommandLine(const CommandLine&);
}
