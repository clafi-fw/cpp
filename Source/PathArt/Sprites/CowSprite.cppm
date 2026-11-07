export module ClaFi.PathArt.CowSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the cow grazes and when it is drawn.
    export struct CowPose
    {
        FloatPoint at; // the middle of its body
        float size; // pixels per unit of the cow, which is 90 units long
        float cycleTime; // its tail swishes and its head bobs as it chews by it
    };

    // A cow's colours, and what its limbs stand against so that they stay legible. See PathArt
    export struct CowColors
    {
        Color body;
        Color spots;
        Color snout;
        Color detail; // the legs, the horns, the tail and the outlines
        Color shadow;
        Color skyBehind; // what the horns and the tail stand against
        Color groundBehind; // what the legs stand against
    };

    // A spotted cow grazing, with a soft shadow under it.
    export void drawCow(const SceneFrame&, const CowPose&, const CowColors&);
}
