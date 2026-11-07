export module ClaFi.PathArt.BatSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How a bat flies at one moment.
    export struct BatPose
    {
        FloatPoint center;
        float size; // pixels per unit of the bat, which is 44 units across the wings
        float flap; // -1 wings down to 1 wings up
    };

    export void drawBat(const SceneFrame&, const BatPose&);
}
