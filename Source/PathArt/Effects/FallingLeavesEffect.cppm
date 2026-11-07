export module ClaFi.PathArt.FallingLeavesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Leaves or petals tumbling down across the whole frame, in two colours by turns.
    export struct FallingLeaves
    {
        float opacity{ 1.0f };
        float petals{ 0.0f }; // 0 leaves to 1 petals, which are smaller
        Color color1{ 0xFFFFFFFF };
        Color color2{ 0xFFFFFFFF };
        int count{ 30 };
    };

    export void drawFallingLeaves(const SceneFrame&, float cycleTime, const FallingLeaves&);
}
