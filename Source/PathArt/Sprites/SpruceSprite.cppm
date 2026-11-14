export module ClaFi.PathArt.SpruceSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    export struct SpruceColors
    {
        Color needles;
        Color sunlit; // the tiers' right sides
        Color shaded; // their left sides
        Color trunk;
        Color trunkShade;
        Color shadow; // clear when drawSpruceShadow has laid it already
    };

    // A spruce of five drooping tiers, 154 units tall, standing on `foot` over its shadow.
    export void drawSpruce(const SceneFrame&, FloatPoint foot, float size, const SpruceColors&);

    // The shadow drawSpruce draws first, for a spruce whose shadow goes under something else.
    export void drawSpruceShadow(const SceneFrame&, FloatPoint foot, float size, Color shadow);
}
