module;
#include "EventBindings.h"

export module ClaFi.Core.System.Timer;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export class UiTimer;

    /// @brief The timer elapsed. sender() is the timer, so one handler can serve several.
    export class TimerEvent : public EventOf<UiTimer>
    {
    public:
        using EventOf<UiTimer>::EventOf;
    };
}

export namespace ClaFi::PlatformTimer
{
    void start(UiTimer* timer, MilliSeconds delay);
    void stop(UiTimer* timer);
}

namespace ClaFi
{
    // A timer that never ticks while user input is still being delivered. See UI-Types
    export class UiTimer : public EventComponent
    {
    public:
        // Takes the same handler pack every emitter does, so a timer can carry more than one
        // listener and each can disconnect. UiTimer{} with no handler is valid: connect later.
        template <typename... Args>
            requires NotSelfCopy<UiTimer, Args...>
        explicit UiTimer(const Args&... args)
            :
            EventComponent{ args... }
        {
        }
        ~UiTimer();
    public:
        DECLARE_EVENT(TimerEvent, OnTick, onTick)
    public:
        void start(MilliSeconds);
        void stop();
        void fire();
        [[nodiscard]] bool isActive() const { return m_active; }
    private:
        mutable std::mutex m_mutex{};
        bool m_active{ false };
    };
}

namespace ClaFi
{
    UiTimer::~UiTimer()
    {
        stop();
    }

    void UiTimer::start(MilliSeconds delay)
    {
        std::lock_guard<std::mutex> scopeGuard{ m_mutex };
        PlatformTimer::start(this, delay);
        m_active = true;
    }

    void UiTimer::stop()
    {
        std::lock_guard<std::mutex> scopeGuard{ m_mutex };
        if (!m_active)
            return;
        PlatformTimer::stop(this);
        m_active = false;
    }

    void UiTimer::fire()
    {
        {
            std::lock_guard<std::mutex> scopeGuard{ m_mutex };
            m_active = false;
        }
        TimerEvent event{ *this };
        emitEvent(event);
    }
}
