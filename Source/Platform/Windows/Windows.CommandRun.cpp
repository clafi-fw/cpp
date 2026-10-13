module;
#include "Windows.Headers.h"

module ClaFi.Core.System.CommandRun;

import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace ClaFi
{
    // The system's own cmd.exe over the line, and a thread waiting for it to end. See Platform
    class CommandRun::Impl
    {
    public:
        Impl(CommandRun&, std::wstring_view commandLine);
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        // Sets the stop event the thread waits on beside the shell, and joins it.
        ~Impl();
    private:
        // Starts the shell over the line, and answers whether it started.
        [[nodiscard]] bool start(std::wstring_view commandLine);
        void threadFunc();
    private:
        CommandRun& m_owner;
        HANDLE m_stop{ nullptr };
        HANDLE m_process{ nullptr };
        std::thread m_thread{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi
{
    namespace
    {
        // What cmd.exe exits with where no program answers to the name a line starts with.
        constexpr DWORD k_notFoundExitCode{ 9009 };
    }

    // CommandRun::Impl

    CommandRun::Impl::Impl(CommandRun& owner, const std::wstring_view commandLine)
        :
        m_owner{ owner },
        m_stop{ ::CreateEventW(nullptr, TRUE, FALSE, nullptr) }
    {
        if (!m_stop || !start(commandLine))
        {
            m_owner.reportDone(CommandOutcome::NotStarted, 0);
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

    // **Through the system's own cmd.exe**, so the line runs as typed into a console: a program
    // found on the PATH whatever its extension - an editor's launcher is often a .cmd - and its
    // quotes read as cmd reads them, /s taking off the pair put round the whole. No window: the
    // shell has nothing to show, and what it starts opens its own.
    bool CommandRun::Impl::start(const std::wstring_view commandLine)
    {
        wchar_t system[MAX_PATH]{};
        const UINT length = ::GetSystemDirectoryW(system, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return false;
        const std::wstring shell = std::wstring{ system, length } + L"\\cmd.exe";
        std::wstring line = L"cmd.exe /s /c \"" + std::wstring{ commandLine } + L"\"";
        STARTUPINFOW startup{ .cb = sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION process{};
        if (!::CreateProcessW(shell.c_str(), line.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        {
            return false;
        }
        ::CloseHandle(process.hThread);
        m_process = process.hProcess;
        return true;
    }

    // **9009 is cmd's own answer** for a name nothing answers to. Any other code but zero is
    // Failed, whether cmd or the program gave it.
    void CommandRun::Impl::threadFunc()
    {
        const std::array<HANDLE, 2> handles = { m_process, m_stop };
        if (::WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE) != WAIT_OBJECT_0)
            return;
        DWORD exitCode = 0;
        if (!::GetExitCodeProcess(m_process, &exitCode))
            return;
        CommandOutcome outcome = CommandOutcome::Failed;
        if (exitCode == 0)
            outcome = CommandOutcome::Ended;
        else if (exitCode == k_notFoundExitCode)
            outcome = CommandOutcome::NotFound;
        m_owner.reportDone(outcome, exitCode);
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
