export module ClaFi.PathArt.GhostSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the ghost floats at one moment of its flight.
    export struct GhostPose
    {
        FloatPoint center;
        float size; // pixels per unit of the ghost, which is about 90 units wide
        // -1 flying left to 1 flying right: the sheet leans that way and the eyes look that way.
        float heading;
        float cycleTime; // the time its arms wave, its hem ripples, it blinks and talks by
    };

    // A sheet ghost with waving arms and a rippling hem.
    export void drawGhost(const SceneFrame&, const GhostPose&);
}
