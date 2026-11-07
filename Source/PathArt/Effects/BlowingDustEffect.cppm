export module ClaFi.PathArt.BlowingDustEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Streaks of dust blown across the whole frame, three widths of it a cycle.
    export struct BlowingDust
    {
        float opacity{ 1.0f };
        Color color{ 0xFFFFFFFF };
        int count{ 50 };
    };

    export void drawBlowingDust(const SceneFrame&, float cycleTime, const BlowingDust&);
}
