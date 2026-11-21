export module ClaFi.PathArt.PosedCowSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // Where the cow stands, which way it looks, and how its head, legs, jaw and tail move.
    export struct PosedCowPose
    {
        FloatPoint foot; // the ground under the middle of its body
        float size; // pixels per unit of the cow, whose body is 100 units long
        float facing; // 1 faces right and -1 left; between the two it is turning
        float headDrop; // radians down from level: about 1.15 to graze and 1.35 to drink
        std::optional<float> stride; // how far through a step it is, in turns, while it walks
        float chew; // how far its jaw hangs open, 0 to 2 units
        float tailSwing; // -0.25 to 0.25
    };

    export struct PosedCowColors
    {
        Color body;
        Color spots;
        Color snout; // the muzzle and the udder
        Color detail; // the hooves, the eye, the nostril and the tail's tuft
        Color shadow;
        float limbInkShare{ 0.0f }; // the share of spots in the far legs, tail and horns
        Color horn{}; // the tint over the horn color derived from coat and detail
    };

    // A spotted cow seen from the side, its far legs and ear a shade darker than the near ones.
    export void drawPosedCow(const SceneFrame&, const PosedCowPose&, const PosedCowColors&);
}
