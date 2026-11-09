export module ClaFi.PathArt.ReedsSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where a clump of reeds stands and when it is drawn.
    export struct ReedsPose
    {
        FloatPoint base; // the foot of the first reed
        float size; // pixels per unit of the reeds, which stand 45 units tall
        float cycleTime; // they sway once a cycle
        float lean{ 0.0f }; // radians a gust bends them by, on top of their sway
        float fur{ 1.0f }; // 1 the heads in full fluff, 0 stripped to their core
        bool inWater{ true }; // their reflection is drawn under them
    };

    export struct ReedColors
    {
        Color stem;
        Color head;
    };

    // Three reeds swaying, and in water their reflection under them.
    export void drawReeds(const SceneFrame&, const ReedsPose&, const ReedColors&);
}
