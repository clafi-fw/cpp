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
    };

    // A six-armed flake 24 units across, its arms forked halfway out, as strokes.
    export void createSnowFlakePath(Graphics::PixelPath&);
    export void drawSnow(const SceneFrame&, float cycleTime, const Snow&);
}
