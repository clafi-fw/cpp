export module ClaFi.PathArt.StarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the stars of a field look and twinkle.
    export enum class StarStyle
    {
        Dots, // round dots and one in ten a four-point sparkle, twinkling in brightness
        Sparkles // four-point sparkles that twinkle in size, gone at the low of each twinkle
    };

    // A field of twinkling stars across a band of the frame.
    export struct StarField
    {
        StarStyle style{ StarStyle::Dots };
        int count{ 100 };
        float top{ 0.0f };
        float bottom{ 0.0f };
        float seed{ 0.0f }; // picks the stars - two fields with one seed are one field
        Color color{ 0xFFEEF0FF };
        float opacity{ 1.0f };
        // How often the slowest star twinkles in a cycle; a long cycle raises it to keep the pace.
        float twinklesPerCycle{ 1.0f };
    };

    export void drawStars(const SceneFrame&, float cycleTime, const StarField&);
}
