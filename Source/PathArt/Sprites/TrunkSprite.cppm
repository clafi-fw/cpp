export module ClaFi.PathArt.TrunkSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A trunk standing on `foot`, its roots flaring out at the ground and its bark grooved.
    export void drawTrunk(const SceneFrame&, FloatPoint foot, float halfWidth, float height,
        Color bark, Color barkShade);
}
