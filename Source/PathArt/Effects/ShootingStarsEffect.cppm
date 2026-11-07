export module ClaFi.PathArt.ShootingStarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Shooting stars crossing the sky one at a time, spread through the cycle.
    export struct ShootingStars
    {
        int count{ 3 };
        float top{ 0.0f }; // the band they set off from
        float bottom{ 0.0f };
        FloatPoint heading{ -0.82f, 0.57f }; // the way they fly, a unit vector
        float flightShare{ 0.014f }; // the share of the cycle each is seen for
        Color color{ 0xFFFFFFFF };
    };

    export void drawShootingStars(const SceneFrame&, float cycleTime, const ShootingStars&);
}
