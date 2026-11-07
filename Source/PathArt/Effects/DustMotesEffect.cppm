export module ClaFi.PathArt.DustMotesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Dust motes rising through a beam of light, fading in and out as they go.
    export struct DustMotes
    {
        int count{ 24 };
        FloatPoint lamp; // the top of the beam
        FloatPoint pool; // the middle of the pool of light the beam makes on the floor
        float top{ 0.0f }; // the height the motes have faded out by
        float spread{ 0.0f }; // how wide the motes scatter at the floor
        Color color{ 0xFFFFF1C9 };
        float risesPerCycle{ 3.0f };
    };

    export void drawDustMotes(const SceneFrame&, float cycleTime, const DustMotes&);
}
