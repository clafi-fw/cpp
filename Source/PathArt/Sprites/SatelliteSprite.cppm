export module ClaFi.PathArt.SatelliteSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the satellite floats at one moment of its orbit.
    export struct SatellitePose
    {
        FloatPoint center;
        float size; // pixels per unit of the craft, whose panels span 444 units
        float sunlit; // 1 in sunlight, 0 in the Earth's shadow
        // 0 to 1 round the orbit: the craft rocks once a round and its panels turn to the Sun.
        float orbitTime;
        float blinkTime; // the time its lamps blink and its panels glint by
    };

    // A satellite with two solar panels, a dish and blinking navigation lights. See PathArt
    export void drawSatellite(const SceneFrame&, const SatellitePose&);
}
