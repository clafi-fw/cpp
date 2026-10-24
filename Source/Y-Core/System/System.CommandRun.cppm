module;
#include "EventBindings.h"

export module ClaFi.Core.System.CommandRun;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace ClaFi
{
    // What the desktop's shell made of a command line. See UI-Types
    export enum class CommandOutcome
    {
        Running,        // no answer yet
        Ended,          // the line ran to its end, and its exit code was zero
        Failed,         // the line ended with an exit code of its own - see exitCode
        NotFound,       // no program answers to the name the line starts with
        NotRunnable,    // the program was found and the system would not run it
        NotStarted      // the shell itself could not be started
    };

    export class CommandRun;

    // The shell has answered a command line. See UI-Types
    export class CommandRunEvent : public EventOf<CommandRun>
    {
    public:
        explicit CommandRunEvent(CommandRun& sender);
    };

    // A command line the desktop's shell runs, answered on the UI thread. See UI-Types
    export class CommandRun : public EventComponent
    {
    public:
        // Starts the line at once.
        explicit CommandRun(std::wstring_view commandLine);
        CommandRun(const CommandRun&) = delete;
        CommandRun& operator=(const CommandRun&) = delete;
        ~CommandRun(); // stops listening and leaves the line running
    public:
        DECLARE_EVENT(CommandRunEvent, OnDone, onDone)
    public:
        [[nodiscard]] CommandOutcome outcome() const { return m_outcome; }
        // What the line exited with, once the outcome is known. See UI-Types
        [[nodiscard]] std::uint32_t exitCode() const { return m_exitCode; }
    private:
        class Impl;
        using ImplPtr = std::unique_ptr<Impl>;
    private:
        // Called by the platform's thread with what the shell answered. Safe from any thread.
        void reportDone(CommandOutcome, std::uint32_t exitCode);
        // Raised by m_doneTimer on the UI thread: takes what the thread left and tells listeners.
        void deliver();
    private:
        // What the thread leaves for deliver - the one state both threads touch.
        std::mutex m_pendingMutex{};
        CommandOutcome m_pendingOutcome{ CommandOutcome::Running };
        std::uint32_t m_pendingExitCode{ 0 };
        CommandOutcome m_outcome{ CommandOutcome::Running };
        std::uint32_t m_exitCode{ 0 };
        // Declared before m_impl, so it stands for as long as the thread that arms it runs.
        UiTimer m_doneTimer{};
        ImplPtr m_impl{};
    };
}
