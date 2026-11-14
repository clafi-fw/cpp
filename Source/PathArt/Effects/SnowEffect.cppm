export module ClaFi.PathArt.SnowEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Snowflakes drifting down across the whole frame, each turning slowly.
    export struct Snow
    {
        float opacity{ 1.0f };
        Color color{ 0xFFFFFFFF };
        int count{ 60 };
        float pace{ 1.0f }; // times the usual speed, rounded to whole falls a cycle
        float seed{ 0.0f }; // shifts where the flakes are, so two sets over one frame differ
    };

    // A six-armed flake 24 units across, its arms forked halfway out, as strokes.
    export void createSnowFlakePath(Graphics::PixelPath&);
    export void drawSnow(const SceneFrame&, float cycleTime, const Snow&);
}
