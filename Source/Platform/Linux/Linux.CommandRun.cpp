module;
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

module ClaFi.Core.System.CommandRun;

import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // /bin/sh over the line in a process of its own, answered through a pipe. See Platform
    class CommandRun::Impl
    {
    public:
        Impl(CommandRun&, std::wstring_view commandLine);
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        // Writes into the stop pipe the thread polls beside the status pipe, and joins it.
        ~Impl();
    private:
        // Forks the process that runs the shell and waits on it, and answers whether it forked.
        [[nodiscard]] bool start(const std::string& line);
        void threadFunc();
    private:
        CommandRun& m_owner;
        int m_statusRead{ -1 };
        std::array<int, 2> m_stop{ -1, -1 };
        std::thread m_thread{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi
{
    namespace
    {
        // What sh exits with where no program answers to the name a line starts with.
        constexpr int k_notFoundExitCode{ 127 };
        // What sh exits with where the program it found would not run.
        constexpr int k_notRunnableExitCode{ 126 };
        // What the waiting process writes where it could not fork the shell at all.
        constexpr int k_noShell{ -1 };
        // Where the waiting process keeps the status pipe; it closes everything above.
        constexpr int k_statusFd{ 3 };
        // What a shell adds a signal's number to for a program the signal killed.
        constexpr int k_signalledBase{ 128 };

        [[nodiscard]] CommandOutcome outcomeOf(int status);
        // The process between the application and the shell - see the definition.
        [[noreturn]] void waitOnShell(int statusWrite, const char* line);
    }

    // CommandRun::Impl

    CommandRun::Impl::Impl(CommandRun& owner, const std::wstring_view commandLine)
        :
        m_owner{ owner }
    {
        if (::pipe2(m_stop.data(), O_CLOEXEC) != 0 || !start(toUtf8(commandLine)))
        {
            m_owner.reportDone(CommandOutcome::NotStarted, 0);
            return;
        }
        m_thread = std::thread{ &Impl::threadFunc, this };
    }

    CommandRun::Impl::~Impl()
    {
        if (m_thread.joinable())
        {
            constexpr char k_stopByte{ 0 };
            [[maybe_unused]] const ssize_t written = ::write(m_stop[1], &k_stopByte, 1);
            m_thread.join();
        }
        for (const int fd : { m_statusRead, m_stop[0], m_stop[1] })
        {
            if (fd >= 0)
                ::close(fd);
        }
    }

    // **Twice forked**, as Platform::shellExecute is: the outer child forks the waiting process and
    // ends at once, so the waiting process belongs to init and nothing here has a child to reap. An
    // outer child that cannot fork writes NotStarted's answer itself.
    bool CommandRun::Impl::start(const std::string& line)
    {
        std::array<int, 2> status = { -1, -1 };
        if (::pipe2(status.data(), O_CLOEXEC) != 0)
            return false;

        const pid_t outer = ::fork();
        if (outer < 0)
        {
            ::close(status[0]);
            ::close(status[1]);
            return false;
        }
        if (outer == 0)
        {
            const pid_t waiting = ::fork();
            if (waiting == 0)
                waitOnShell(status[1], line.c_str());
            if (waiting < 0)
            {
                [[maybe_unused]] const ssize_t written =
                    ::write(status[1], &k_noShell, sizeof k_noShell);
            }
            ::_exit(0);
        }

        ::close(status[1]);
        m_statusRead = status[0];
        int outerStatus = 0;
        pid_t reaped = ::waitpid(outer, &outerStatus, 0);
        while (reaped < 0 && errno == EINTR)
            reaped = ::waitpid(outer, &outerStatus, 0);
        return true;
    }

    // A pipe that closes without a word means the waiting process went down first, and there is
    // no answer to give.
    void CommandRun::Impl::threadFunc()
    {
        std::array<pollfd, 2> watched = {
            pollfd{ .fd = m_statusRead, .events = POLLIN, .revents = 0 },
            pollfd{ .fd = m_stop[0], .events = POLLIN, .revents = 0 }
        };
        int ready = ::poll(watched.data(), watched.size(), -1);
        while (ready < 0 && errno == EINTR)
            ready = ::poll(watched.data(), watched.size(), -1);
        if (ready <= 0 || watched[1].revents != 0)
            return;

        int status = 0;
        if (::read(m_statusRead, &status, sizeof status) != static_cast<ssize_t>(sizeof status))
            return;
        const std::uint32_t exitCode = status == k_noShell ? 0 : static_cast<std::uint32_t>(status);
        m_owner.reportDone(outcomeOf(status), exitCode);
    }

    namespace
    {
        CommandOutcome outcomeOf(const int status)
        {
            switch (status)
            {
                case k_noShell:
                    return CommandOutcome::NotStarted;
                case 0:
                    return CommandOutcome::Ended;
                case k_notFoundExitCode:
                    return CommandOutcome::NotFound;
                case k_notRunnableExitCode:
                    return CommandOutcome::NotRunnable;
                default:
                    return CommandOutcome::Failed;
            }
        }

        // **It runs on as a copy of the application**, running no program of its own, so it would
        // hold every descriptor the application has open for as long as the program it waits on
        // runs - the display server's connection among them, and the server keeps a client's
        // windows for as long as that connection is open. It keeps the status pipe at k_statusFd,
        // closes everything above, and leaves standard input and output to the shell. A program
        // killed by a signal answers 128 and the signal's number, as a shell reports it.
        void waitOnShell(const int statusWrite, const char* line)
        {
            if (statusWrite != k_statusFd && ::dup3(statusWrite, k_statusFd, O_CLOEXEC) < 0)
                ::_exit(0);
            ::close_range(k_statusFd + 1, ~0u, 0);

            int status = k_noShell;
            const pid_t shell = ::fork();
            if (shell == 0)
            {
                ::execl("/bin/sh", "sh", "-c", line, nullptr);
                ::_exit(k_notFoundExitCode);
            }
            if (shell > 0)
            {
                int waited = 0;
                pid_t reaped = ::waitpid(shell, &waited, 0);
                while (reaped < 0 && errno == EINTR)
                    reaped = ::waitpid(shell, &waited, 0);
                if (reaped == shell && WIFEXITED(waited))
                    status = WEXITSTATUS(waited);
                else if (reaped == shell && WIFSIGNALED(waited))
                    status = k_signalledBase + WTERMSIG(waited);
                else
                    status = 0;
            }
            [[maybe_unused]] const ssize_t written = ::write(k_statusFd, &status, sizeof status);
            ::_exit(0);
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
