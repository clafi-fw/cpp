export module ClaFi.PathArt.FirTreeSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    export struct FirTreeColors
    {
        Color trunk;
        Color trunkShade;
        Color leaves; // the bottom tier; each tier above is lighter and less saturated
        Color shadow;
    };

    // A fir of three tiers, 140 units tall, standing on `base` with a soft shadow under it.
    export void drawFirTree(const SceneFrame&, FloatPoint base, float size, const FirTreeColors&);
}
