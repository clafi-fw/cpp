export module ClaFi.PathArt.MillSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the mill stands and how far its wings have turned.
    export struct MillPose
    {
        FloatPoint base; // the middle of the house's foot
        float size; // pixels per unit of the mill, whose house is 160 units tall to the roof
        float wingTurns; // clockwise
    };

    export struct MillColors
    {
        Color bodyTop;
        Color bodyBottom;
        Color roofTop;
        Color roofBottom;
        Color wingWood;
        Color wingWoodStroke;
        Color wingFabric;
        Color wingFabricStroke;
    };

    // The house, and its reflection hanging upside down from its foot.
    export void drawMillHouse(const SceneFrame&, const MillPose&, const MillColors&);
    // The four wings on their hub, 130 units above the foot.
    export void drawMillWings(const SceneFrame&, const MillPose&, const MillColors&);
}
