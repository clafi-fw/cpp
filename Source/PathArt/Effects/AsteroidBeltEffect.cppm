export module ClaFi.PathArt.AsteroidBeltEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Which half of a ring on a tilted plane: beyond its center or in front of it.
    export enum class BeltHalf
    {
        Far,
        Near
    };

    // A belt of rocks round a center, on a plane tipped toward the viewer.
    export struct AsteroidBelt
    {
        FloatPoint center;
        float innerRadius{ 0.0f };
        float outerRadius{ 0.0f };
        float tilt{ 0.5f }; // how flat the plane is seen: a circle on it is this tall for its width
        int count{ 280 };
        float orbitsPerCycle{ 1.0f };
        Color rock{ 0xD98F8577 };
        Color paleRock{ 0xD9C9B79C }; // one rock in five
    };

    // One half of the belt, so that what stands at its center can be drawn between the two.
    export void drawAsteroidBelt(const SceneFrame&, float cycleTime, const AsteroidBelt&,
        BeltHalf);
}
