module;
#include "Windows.Headers.h"
#include <shellapi.h>

module ClaFi.Core.System.CommandRun;

import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace ClaFi
{
    // The desktop's shell over the line's program, and a thread waiting for it to end. See Platform
    class CommandRun::Impl
    {
    public:
        Impl(CommandRun&, std::wstring_view commandLine);
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        // Sets the stop event the thread waits on beside the program, and joins it.
        ~Impl();
    private:
        // Hands the line to the shell, and answers Running or what kept it from running.
        [[nodiscard]] CommandOutcome start(std::wstring_view commandLine);
        void threadFunc();
    private:
        CommandRun& m_owner;
        HANDLE m_stop{ nullptr };
        HANDLE m_process{ nullptr };   // null where the shell started no process of its own
        std::thread m_thread{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi
{
    namespace
    {
        // A command line taken apart: the program it starts, and the parameters after it.
        struct SplitLine
        {
            std::wstring program{};
            std::wstring parameters{};
        };

        [[nodiscard]] SplitLine splitLine(std::wstring_view commandLine);
        // What the shell's refusal says of the program.
        [[nodiscard]] CommandOutcome refusalOf(DWORD error);
    }

    // CommandRun::Impl

    // **A shell that starts no process has handed the line to one already running** - a file
    // opened in a window that is up - and there is nothing to wait for.
    CommandRun::Impl::Impl(CommandRun& owner, const std::wstring_view commandLine)
        :
        m_owner{ owner },
        m_stop{ ::CreateEventW(nullptr, TRUE, FALSE, nullptr) }
    {
        if (!m_stop)
        {
            m_owner.reportDone(CommandOutcome::NotStarted, 0);
            return;
        }
        const CommandOutcome outcome = start(commandLine);
        if (outcome != CommandOutcome::Running)
        {
            m_owner.reportDone(outcome, 0);
            return;
        }
        if (!m_process)
        {
            m_owner.reportDone(CommandOutcome::Ended, 0);
            return;
        }
        m_thread = std::thread{ &Impl::threadFunc, this };
    }

    CommandRun::Impl::~Impl()
    {
        if (m_stop)
            ::SetEvent(m_stop);
        if (m_thread.joinable())
            m_thread.join();
        if (m_process)
            ::CloseHandle(m_process);
        if (m_stop)
            ::CloseHandle(m_stop);
    }

    // **Through ShellExecuteEx**, so the program is found the way the Run box finds a name - on the
    // PATH or under App Paths, whatever its extension - and a name nothing answers to is the
    // shell's own refusal rather than an exit code. The system shows no message of its own.
    CommandOutcome CommandRun::Impl::start(const std::wstring_view commandLine)
    {
        const SplitLine line = splitLine(commandLine);
        if (line.program.empty())
            return CommandOutcome::NotFound;
        SHELLEXECUTEINFOW info{
            .cbSize = sizeof(SHELLEXECUTEINFOW),
            .fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_DOENVSUBST,
            .lpFile = line.program.c_str(),
            .lpParameters = line.parameters.empty() ? nullptr : line.parameters.c_str(),
            .nShow = SW_SHOWNORMAL
        };
        if (!::ShellExecuteExW(&info))
            return refusalOf(::GetLastError());
        m_process = info.hProcess;
        return CommandOutcome::Running;
    }

    void CommandRun::Impl::threadFunc()
    {
        const std::array<HANDLE, 2> handles = { m_process, m_stop };
        if (::WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE) != WAIT_OBJECT_0)
            return;
        DWORD exitCode = 0;
        if (!::GetExitCodeProcess(m_process, &exitCode))
            return;
        const CommandOutcome outcome = exitCode == 0
            ? CommandOutcome::Ended
            : CommandOutcome::Failed;
        m_owner.reportDone(outcome, exitCode);
    }

    namespace
    {
        // **The program is the line's first word, as the system reads one**: the run inside the
        // quotes where the line opens with a quote, else up to the first space. The parameters are
        // what follows the spaces after it, passed on as written.
        SplitLine splitLine(const std::wstring_view commandLine)
        {
            constexpr std::wstring_view k_spaces{ L" \t" };
            const std::size_t start = commandLine.find_first_not_of(k_spaces);
            if (start == std::wstring_view::npos)
                return {};
            const bool quoted = commandLine[start] == L'"';
            const std::size_t first = quoted ? start + 1 : start;
            const std::size_t last = std::min(
                quoted ? commandLine.find(L'"', first) : commandLine.find_first_of(k_spaces, first),
                commandLine.size()
            );
            const std::size_t past = quoted ? std::min(last + 1, commandLine.size()) : last;
            const std::size_t parameters = commandLine.find_first_not_of(k_spaces, past);
            SplitLine result{};
            result.program = std::wstring{ commandLine.substr(first, last - first) };
            if (parameters != std::wstring_view::npos)
                result.parameters = std::wstring{ commandLine.substr(parameters) };
            return result;
        }

        // A name the shell cannot find is NotFound. Every other refusal is about a program it
        // found - a file nothing opens, an elevation declined - which is NotRunnable.
        CommandOutcome refusalOf(const DWORD error)
        {
            switch (error)
            {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    return CommandOutcome::NotFound;
                default:
                    return CommandOutcome::NotRunnable;
            }
        }
    }

    // CommandRun

    CommandRun::CommandRun(const std::wstring_view commandLine)
    {
        m_doneTimer.onTick([this](TimerEvent&) {
            deliver();
        });
        m_impl = std::make_unique<Impl>(*this, commandLine);
    }

    CommandRun::~CommandRun() = default;
}
