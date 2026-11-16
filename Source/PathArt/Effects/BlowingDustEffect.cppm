export module ClaFi.PathArt.BlowingDustEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Streaks of dust blown leftward in gusts and eddies, two to five widths a cycle at pace 1.
    export struct BlowingDust
    {
        float opacity{ 1.0f };
        Color color1{ 0xFFFFFFFF }; // one end of the range a streak's colour is picked from
        Color color2{ 0xFFFFFFFF }; // the other end
        int count{ 200 };
        float pace{ 1.0f }; // times the usual speed, rounded to whole widths a cycle
        float seed{ 0.0f }; // shifts where the streaks are, so two sets over one frame differ
    };

    export void drawBlowingDust(const SceneFrame&, float cycleTime, const BlowingDust&);
}
