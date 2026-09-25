module;
#include "Windows.Headers.h"

export module ClaFi.Platform.Windows.Timer;

import ClaFi.Platform.Windows.Window;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    // Where every UiTimer ticks: a message window and its WM_TIMER, one per process. See Platform
    export class TimerManager : public IMessageSink
    {
    public:
        explicit TimerManager(WindowsManager& owner);
        ~TimerManager() override;
        TimerManager(const TimerManager&) = delete;
        TimerManager& operator=(const TimerManager&) = delete;
    public:
        // THE ONE STANDING, or null while the platform holds none. A UiTimer sits below every
        // context - the animation controller's, a directory watcher's, a control's - so the two
        // hooks below reach the manager through this and nothing else; the manager sets it as it
        // is built and clears it as it goes. The platform stands before anything that owns a
        // UiTimer and after it - see Application - so a hook finding none is a broken order.
        [[nodiscard]] static TimerManager* standing() { return s_standing; }
        [[nodiscard]] HWND handle() const { return m_window.handle(); }
        void wndProc(WinApiMsg&) override;
    private:
        inline static TimerManager* s_standing{ nullptr };
        MessageWindow m_window;
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Windows
{
    TimerManager::TimerManager(WindowsManager& owner)
        :
        m_window{ owner }
    {
        m_window.setSink(this);
        s_standing = this;
    }

    TimerManager::~TimerManager()
    {
        s_standing = nullptr;
    }

    void TimerManager::wndProc(WinApiMsg& msg)
    {
        if (msg.msg == WM_TIMER)
        {
            UiTimer* timer = reinterpret_cast<UiTimer*>(msg.wParam);
            if (timer != nullptr)
            {
                PlatformTimer::stop(timer);
                timer->fire();
            }
            msg.handled = true;
        }
    }
}

// THE TWO HOOKS System.Timer DECLARES AND DOES NOT DEFINE.
namespace ClaFi::PlatformTimer
{
    using PlatformImplementation::Windows::TimerManager;

    [[nodiscard]] TimerManager& standingManager()
    {
        TimerManager* manager = TimerManager::standing();
        if (manager == nullptr)
            unreachable("PlatformTimer: no timer manager stands - a UiTimer outlives the platform");

        return *manager;
    }

    void start(UiTimer* timer, MilliSeconds delay)
    {
        if (timer == nullptr)
            return;

        stop(timer);
        ::SetTimer(standingManager().handle(), reinterpret_cast<UINT_PTR>(timer), delay.value,
            nullptr);
    }

    void stop(UiTimer* timer)
    {
        if (timer == nullptr)
            return;

        ::KillTimer(standingManager().handle(), reinterpret_cast<UINT_PTR>(timer));
    }
}
