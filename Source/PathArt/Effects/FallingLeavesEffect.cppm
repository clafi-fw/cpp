export module ClaFi.PathArt.FallingLeavesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Leaves or petals tumbling down across the whole frame, in two colors by turns.
    export struct FallingLeaves
    {
        float opacity{ 1.0f };
        float petals{ 0.0f }; // 0 leaves to 1 petals, which are smaller
        Color color1{ 0xFFFFFFFF };
        Color color2{ 0xFFFFFFFF };
        int count{ 30 };
        float pace{ 1.0f }; // times the usual speed, rounded to whole falls a cycle
        float seed{ 0.0f }; // shifts where the leaves are, so two sets over one frame differ
    };

    export void drawFallingLeaves(const SceneFrame&, float cycleTime, const FallingLeaves&);
}
