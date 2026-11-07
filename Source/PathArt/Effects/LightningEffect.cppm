export module ClaFi.PathArt.LightningEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Lightning three times a cycle: a flickering bolt and a flash over the whole frame.
    export struct Lightning
    {
        float strength{ 1.0f }; // the opacity of the bolt; the flash is a seventh as strong
        Color color{ 0xFFFFFFFF };
    };

    export void drawLightning(const SceneFrame&, float cycleTime, const Lightning&);
}
