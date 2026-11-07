export module ClaFi.PathArt.WaterLilySprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the water lily floats and when it is drawn.
    export struct WaterLilyPose
    {
        FloatPoint center;
        float size; // pixels per unit of the lily, whose pad is 44 units across
        float cycleTime; // it bobs one and a half times a cycle
    };

    export struct WaterLilyColors
    {
        Color pad;
        Color flower;
    };

    // A lily pad with a five-petalled flower, bobbing on the water.
    export void drawWaterLily(const SceneFrame&, const WaterLilyPose&, const WaterLilyColors&);
}
