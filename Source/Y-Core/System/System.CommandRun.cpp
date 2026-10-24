module ClaFi.Core.System.CommandRun;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // CommandRunEvent

    CommandRunEvent::CommandRunEvent(CommandRun& sender)
        :
        EventOf<CommandRun>{ sender }
    {
    }

    // CommandRun - portable members. The constructor and destructor live in the platform
    // implementation unit, where Impl is a complete type.

    void CommandRun::reportDone(const CommandOutcome outcome, const std::uint32_t exitCode)
    {
        {
            std::lock_guard<std::mutex> lock{ m_pendingMutex };
            m_pendingOutcome = outcome;
            m_pendingExitCode = exitCode;
        }
        m_doneTimer.start(MilliSeconds{ 0u });
    }

    void CommandRun::deliver()
    {
        {
            std::lock_guard<std::mutex> lock{ m_pendingMutex };
            m_outcome = m_pendingOutcome;
            m_exitCode = m_pendingExitCode;
        }
        emitEvent<CommandRunEvent>(*this);
    }
}
