export module ClaFi.PathArt.SteamEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A burst of steam from a vent: a dozen puffs that rise, drift and swell as they fade.
    export struct SteamBurst
    {
        FloatPoint vent;
        float size; // pixels per unit of the steam, whose puffs rise 150 units
        float time; // 0 to 1 through the burst; the last puff has faded by 0.7
        Color color{ 0xFFECE6DA };
    };

    export void drawSteam(const SceneFrame&, const SteamBurst&);
}
