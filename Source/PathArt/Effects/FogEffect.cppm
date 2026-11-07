export module ClaFi.PathArt.FogEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A bank of fog drifting across the frame from left to right: a row of soft wide glows.
    export struct FogBank
    {
        float y{ 0.0f };
        float driftsPerCycle{ 1.0f };
        float start{ 0.0f }; // how far into a drift the bank starts, 0 to 1
        Color color{ 0x299FB4E8 };
        int puffs{ 3 };
    };

    export void drawFog(const SceneFrame&, float cycleTime, const FogBank&);
}
