export module ClaFi.PathArt.ReedsSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where a clump of reeds stands and when it is drawn.
    export struct ReedsPose
    {
        FloatPoint base; // the foot of the first reed, at the water's edge
        float size; // pixels per unit of the reeds, which stand 45 units tall
        float cycleTime; // they sway once a cycle
    };

    export struct ReedColors
    {
        Color stem;
        Color head;
    };

    // Three reeds swaying at the water's edge, and their reflection under them.
    export void drawReeds(const SceneFrame&, const ReedsPose&, const ReedColors&);
}
