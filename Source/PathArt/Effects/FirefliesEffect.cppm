export module ClaFi.PathArt.FirefliesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Fireflies wandering over the middle of the frame, each blinking on and off twice a cycle.
    export struct Fireflies
    {
        float opacity{ 1.0f };
        Color light{ 0xFFFFFFFF }; // the firefly itself
        Color glow{ 0xFFFFFFFF }; // the glow round it, at half its alpha
        int count{ 20 };
    };

    export void drawFireflies(const SceneFrame&, float cycleTime, const Fireflies&);
}
