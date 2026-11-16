export module ClaFi.PathArt.RainEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Rain falling across the whole frame, slanting a little to the left.
    export struct Rain
    {
        float opacity{ 1.0f }; // the drops take a quarter of this, times the colour's alpha
        Color color{ 0xFFFFFFFF };
        int count{ 400 };
        float pace{ 1.0f }; // times the usual speed, rounded to whole falls a cycle
        float seed{ 0.0f }; // shifts where the drops are, so two sets over one frame differ
    };

    export void drawRain(const SceneFrame&, float cycleTime, const Rain&);
}
