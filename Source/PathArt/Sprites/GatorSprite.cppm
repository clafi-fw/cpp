export module ClaFi.PathArt.GatorSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the gator lies and when it is drawn.
    export struct GatorPose
    {
        FloatPoint at; // the back of its lower jaw's top edge
        float size; // pixels per unit of the gator, whose jaws are 140 units long
        float cycleTime; // it snaps five times a cycle and blinks by it
        float opacity;
    };

    export struct GatorColors
    {
        Color body;
        Color eye;
    };

    // A gator's head at the water line, its upper jaw snapping.
    export void drawGator(const SceneFrame&, const GatorPose&, const GatorColors&);
}
