module;
#include <sys/eventfd.h>
#include <unistd.h>
export module ClaFi.Platform.Linux.Timer;

import ClaFi.Platform.Linux.Diagnostic;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Timer;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    struct ScheduledTimer
    {
        UiTimer* timer;
        std::chrono::steady_clock::time_point targetTime;
    };

    // Where every UiTimer ticks: a thread that waits out the delays and an eventfd it raises,
    // which the display's loop polls. One per process, the platform's member. See Platform
    export class TimerManager
    {
    public:
        TimerManager();
        ~TimerManager();
        TimerManager(const TimerManager&) = delete;
        TimerManager& operator=(const TimerManager&) = delete;
        // THE ONE STANDING, or null while the platform holds none. A UiTimer sits below every
        // context - the animation controller's, a directory watcher's, a control's - so the two
        // hooks in Linux.Timer.cpp reach the manager through this and nothing else; the manager
        // sets it as it is built and clears it as it goes. The platform stands before anything
        // that owns a UiTimer and after it - see Application - so a hook finding none is a
        // broken order.
        [[nodiscard]] static TimerManager* standing() { return s_standing; }
        void schedule(UiTimer*, MilliSeconds);
        void cancel(UiTimer*);
        void dispatchPending();
        [[nodiscard]] int eventFd() const { return m_eventFd; }
    private:
        void threadLoop();
        void cancelInternal(UiTimer*);
    private:
        inline static TimerManager* s_standing{ nullptr };
        int m_eventFd{ -1 };
        std::thread m_thread;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_running{ true };
        std::vector<ScheduledTimer> m_timers;
        std::mutex m_firedMutex;
        std::vector<UiTimer*> m_firedTimers;
    };
}

namespace ClaFi::PlatformImplementation::Linux
{
    TimerManager::TimerManager()
    {
        m_eventFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        check(m_eventFd != -1);
        m_thread = std::thread{ [this]() {
            threadLoop();
        } };
        s_standing = this;
    }

    TimerManager::~TimerManager()
    {
        s_standing = nullptr;
        {
            std::lock_guard<std::mutex> lock{ m_mutex };
            m_running = false;
        }
        m_cv.notify_one();
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        if (m_eventFd != -1)
        {
            ::close(m_eventFd);
        }
    }

    void TimerManager::schedule(UiTimer* timer, MilliSeconds delay)
    {
        std::lock_guard<std::mutex> lock{ m_mutex };
        cancelInternal(timer);

        // A zero delay means "as often as the loop allows", and it has to be given a floor.
        // The framework asks for one - k_animationTimerInterval is 0ms - and on Windows that
        // is answered by SetTimer, which clamps to USER_TIMER_MINIMUM and delivers WM_TIMER
        // as a synthesised low-priority message that is coalesced and never generated while
        // input is pending. The effective rate is around 100Hz.
        //
        // An eventfd has no such manners. Taken literally, zero re-arms the moment it fires:
        // poll returns immediately, the animation ticks, the timer is restarted with zero,
        // and the loop runs at whatever the CPU manages - tens of thousands of frames, almost
        // all of them advancing nothing, because AnimationController::timerTick skips any
        // animation less than a millisecond old. Same clamp, same reason.
        constexpr std::uint32_t k_minimumDelayMs = 10;

        auto target = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(std::max(delay.value, k_minimumDelayMs));
        auto it = std::upper_bound(m_timers.begin(), m_timers.end(), target,
            [](std::chrono::steady_clock::time_point val, const ScheduledTimer& item) {
                return val < item.targetTime;
            });

        m_timers.insert(it, ScheduledTimer{ timer, target });
        m_cv.notify_one();
    }

    void TimerManager::cancel(UiTimer* timer)
    {
        {
            std::lock_guard<std::mutex> lock{ m_mutex };
            cancelInternal(timer);
        }
        {
            std::lock_guard<std::mutex> firedLock{ m_firedMutex };
            std::erase(m_firedTimers, timer);
        }
    }

    void TimerManager::dispatchPending()
    {
        std::uint64_t counter = 0;
        ::read(m_eventFd, &counter, sizeof(counter));

        std::vector<UiTimer*> localFired;
        {
            std::lock_guard<std::mutex> lock{ m_firedMutex };
            localFired = std::move(m_firedTimers);
            m_firedTimers.clear();
        }

        for (UiTimer* timer : localFired)
        {
            timer->fire();
        }
    }

    void TimerManager::threadLoop()
    {
        while (m_running)
        {
            std::unique_lock<std::mutex> lock{ m_mutex };

            if (m_timers.empty())
            {
                m_cv.wait(lock);
            }
            else
            {
                auto now = std::chrono::steady_clock::now();
                auto nextExpireTime = m_timers.front().targetTime;

                if (now >= nextExpireTime)
                {
                    UiTimer* timer = m_timers.front().timer;
                    m_timers.erase(m_timers.begin());

                    {
                        std::lock_guard<std::mutex> firedLock{ m_firedMutex };
                        m_firedTimers.push_back(timer);
                    }

                    std::uint64_t counter = 1;
                    ::write(m_eventFd, &counter, sizeof(counter));
                }
                else
                {
                    m_cv.wait_until(lock, nextExpireTime);
                }
            }
        }
    }

    void TimerManager::cancelInternal(UiTimer* timer)
    {
        auto it = std::find_if(m_timers.begin(), m_timers.end(),
            [timer](const ScheduledTimer& item) {
                return item.timer == timer;
            });

        if (it != m_timers.end())
        {
            m_timers.erase(it);
        }
    }
}
