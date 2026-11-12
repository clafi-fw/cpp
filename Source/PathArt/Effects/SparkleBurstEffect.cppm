export module ClaFi.PathArt.SparkleBurstEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // Sparkles thrown out from a point, flying apart, shrinking and fading.
    export struct SparkleBurst
    {
        FloatPoint center;
        float age; // 0 as it bursts to 1 once the last sparkle has faded
        int count{ 8 };
        float seed{ 0.0f }; // tells one burst's scatter from another's
        float size{ 1.0f }; // the sparkles' size and reach, 66 units at 1
        // Taken in turn, a sparkle each.
        std::array<Color, 3> colors{ Color{ 0xFFFFD34D }, Color{ 0xFFFFF6DC },
            Color{ 0xFF7AF4FF } };
    };

    export void drawSparkleBurst(const SceneFrame&, const SparkleBurst&);
}
