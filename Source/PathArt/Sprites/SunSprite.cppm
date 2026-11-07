export module ClaFi.PathArt.SunSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The Sun at one moment.
    export struct SunPose
    {
        FloatPoint center;
        float cycleTime; // the rays turn once a cycle
        float flickersPerCycle; // how often each ray flickers in a cycle
    };

    // A glowing Sun with a crown of flickering rays, 40 units in radius.
    export void drawSun(const SceneFrame&, const SunPose&);
}
