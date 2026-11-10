export module ClaFi.PathArt.FirefliesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Fireflies round their places in an area, each blinking on and off twice a cycle. See PathArt
    export struct Fireflies
    {
        float opacity{ 1.0f };
        Color light{ 0xFFFFFFFF }; // the firefly itself
        Color glow{ 0xFFFFFFFF }; // the glow round it, at half its alpha
        int count{ 20 };
        FloatRect area{}; // where the places lie
        float size{ 1.0f }; // of a firefly and its wandering, below 1 for a swarm further off
    };

    export void drawFireflies(const SceneFrame&, float cycleTime, const Fireflies&);
}
