module;
#include <cstdio>

module ClaFi.App.CommandLine;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    CommandLine::CommandLine(std::vector<std::wstring> arguments, const std::wstring_view programName,
        const std::wstring_view version, const std::wstring_view description,
        const std::span<const Option> options)
        :
        m_arguments{ std::move(arguments) },
        m_options{ options },
        m_programName{ programName },
        m_version{ version },
        m_description{ description },
        m_taken(options.size())
    {
        bool positionalOnly = false;
        for (std::size_t at = 0; at != m_arguments.size(); ++at)
        {
            const std::wstring& argument = m_arguments[at];
            if (positionalOnly)
            {
                m_positionals.push_back(at);
                continue;
            }
            if (argument == L"--")
            {
                positionalOnly = true;
                continue;
            }
            if (argument == L"--help" || argument == L"-h")
            {
                m_help = true;
                continue;
            }
            if (argument == L"--version" || argument == L"-v")
            {
                m_versionAsked = true;
                continue;
            }
            if (argument.empty() || argument.front() != L'-' || argument == L"-")
            {
                m_positionals.push_back(at);
                continue;
            }

            const std::size_t equals = argument.find(L'=');
            const std::wstring name = equals == std::wstring::npos
                ? argument
                : argument.substr(0, equals);
            const std::optional<std::size_t> index = optionIndex(name);
            if (!index)
            {
                if (!m_error)
                    m_error = L"unknown option " + name;
                continue;
            }

            Taken& taken = m_taken[*index];
            taken.present = true;
            if (m_options[*index].valueName.empty())
            {
                if (equals != std::wstring::npos && !m_error)
                    m_error = name + L" takes no value";
                continue;
            }
            if (equals != std::wstring::npos)
            {
                taken.value = argument.substr(equals + 1);
                continue;
            }
            if (at + 1 == m_arguments.size())
            {
                if (!m_error)
                    m_error = name + L" needs a value";
                break;
            }
            ++at;
            taken.value = m_arguments[at];
        }
    }

    std::optional<std::size_t> CommandLine::optionIndex(const std::wstring_view name) const
    {
        for (std::size_t index = 0; index != m_options.size(); ++index)
        {
            if (m_options[index].name == name)
                return index;
        }
        return std::nullopt;
    }

    bool CommandLine::has(const std::wstring_view option) const
    {
        const std::optional<std::size_t> index = optionIndex(option);
        return index && m_taken[*index].present;
    }

    std::optional<std::wstring_view> CommandLine::value(const std::wstring_view option) const
    {
        const std::optional<std::size_t> index = optionIndex(option);
        if (!index || !m_taken[*index].value)
            return std::nullopt;
        return std::wstring_view{ *m_taken[*index].value };
    }

    std::vector<std::wstring_view> CommandLine::positionals() const
    {
        std::vector<std::wstring_view> result{};
        result.reserve(m_positionals.size());
        for (const std::size_t index : m_positionals)
            result.emplace_back(m_arguments[index]);
        return result;
    }

    std::wstring CommandLine::optionLabel(const Option& option) const
    {
        std::wstring label{ option.name };
        if (!option.valueName.empty())
        {
            label += L" ";
            label += option.valueName;
        }
        return label;
    }

    std::wstring CommandLine::usage() const
    {
        std::wstring text = m_programName;
        if (!m_version.empty())
        {
            text += L" ";
            text += m_version;
        }
        text += L"\n";
        if (!m_description.empty())
        {
            text += m_description;
            text += L"\n";
        }
        text += L"\nUsage: ";
        text += m_programName;
        text += L" [options]\n\n";

        const std::wstring_view helpLabel = L"--help, -h";
        const std::wstring_view versionLabel = L"--version, -v";
        std::size_t width = std::max(helpLabel.size(), versionLabel.size());
        for (const Option& option : m_options)
            width = std::max(width, optionLabel(option).size());

        for (const Option& option : m_options)
            text += std::format(L"  {:<{}}  {}\n", optionLabel(option), width, option.help);
        text += std::format(L"  {:<{}}  {}\n", helpLabel, width, L"shows this text");
        text += std::format(L"  {:<{}}  {}\n", versionLabel, width, L"shows the version");
        return text;
    }

    std::wstring CommandLine::versionLine() const
    {
        std::wstring text = m_programName;
        if (!m_version.empty())
        {
            text += L" ";
            text += m_version;
        }
        return text;
    }

    void writeToConsole(const std::wstring_view text)
    {
        const std::string utf8 = toUtf8(text);
        std::fwrite(utf8.data(), 1, utf8.size(), stdout);
        std::fflush(stdout);
    }

    void writeErrorToConsole(const std::wstring_view text)
    {
        const std::string utf8 = toUtf8(text);
        std::fwrite(utf8.data(), 1, utf8.size(), stderr);
        std::fflush(stderr);
    }

    std::optional<int> answerCommandLine(const CommandLine& commandLine)
    {
        if (commandLine.help())
        {
            writeToConsole(commandLine.usage());
            return ExitCode::Ok;
        }
        if (commandLine.version())
        {
            writeToConsole(commandLine.versionLine());
            writeToConsole(L"\n");
            return ExitCode::Ok;
        }
        if (const std::optional<std::wstring>& error = commandLine.error())
        {
            writeErrorToConsole(*error);
            writeErrorToConsole(L"\n");
            writeErrorToConsole(commandLine.usage());
            return ExitCode::Misused;
        }
        return std::nullopt;
    }
}
