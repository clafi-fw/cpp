export module ClaFi.PathArt.StarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A band of four-point stars that twinkle in size and brightness.
    export struct StarField
    {
        int count{ 100 };
        float top{ 0.0f };
        float bottom{ 0.0f };
        float seed{ 0.0f }; // picks the stars - two fields with one seed are one field
        Color color{ 0xFFEEF0FF };
        float opacity{ 1.0f };
        // How often the slowest star twinkles in a cycle; a long cycle raises it to keep the pace.
        float twinklesPerCycle{ 1.0f };
        // How far the field has moved, in frame widths right and band heights down. See PathArt
        FloatPoint drift{};
        float bottomBrightness{ 1.0f };
    };

    export void drawStars(const SceneFrame&, float cycleTime, const StarField&);
}
