export module ClaFi.PathArt.BlowingDustEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Streaks of dust blown leftward across the whole frame, three widths of it a cycle at pace 1.
    export struct BlowingDust
    {
        float opacity{ 1.0f };
        Color color{ 0xFFFFFFFF };
        int count{ 50 };
        float pace{ 1.0f }; // times the usual speed, rounded to whole widths a cycle
        float seed{ 0.0f }; // shifts where the streaks are, so two sets over one frame differ
    };

    export void drawBlowingDust(const SceneFrame&, float cycleTime, const BlowingDust&);
}
