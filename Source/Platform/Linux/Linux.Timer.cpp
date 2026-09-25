// An implementation unit of ClaFi.Core.System.Timer, for the two hooks it declares and does
// not define. They belong to that module because that is where they were declared, so this is
// the only place they can be given bodies - the same reason Linux.TextLayout.cpp and
// Linux.FormWindow.cpp are implementation units of the modules they complete rather than of the
// platform modules they draw on.
module ClaFi.Core.System.Timer;

import ClaFi.Platform.Linux.Timer;
import ClaFi.Core.System.Utils;

namespace ClaFi::PlatformTimer
{
    using PlatformImplementation::Linux::TimerManager;

    [[nodiscard]] TimerManager& standingManager()
    {
        TimerManager* manager = TimerManager::standing();
        if (manager == nullptr)
            unreachable("PlatformTimer: no timer manager stands - a UiTimer outlives the platform");

        return *manager;
    }

    void start(UiTimer* timer, MilliSeconds delay)
    {
        standingManager().schedule(timer, delay);
    }

    void stop(UiTimer* timer)
    {
        standingManager().cancel(timer);
    }
}
