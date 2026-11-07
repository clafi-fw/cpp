export module ClaFi.PathArt.WaterWheelSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How far the water wheel has turned.
    export struct WaterWheelPose
    {
        FloatPoint center;
        float size; // pixels per unit of the wheel, which is 80 units across
        float turns; // counterclockwise
    };

    // An eight-spoked wheel; its rim is the wood a shade darker.
    export void drawWaterWheel(const SceneFrame&, const WaterWheelPose&, Color wood);
}
