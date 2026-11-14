export module ClaFi.PathArt.CelestialSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the sun or the moon stands, and how far the night has fallen.
    export struct CelestialPose
    {
        FloatPoint center;
        float night; // 0 by day, 1 at night, when the disc is the moon and shows its craters
    };

    // The sun or the moon: a disc 24 units in radius in a rim of its own light.
    export void drawCelestial(const SceneFrame&, const CelestialPose&, Color disc);
    // The light about the sun or the moon, 130 units high and twice as wide, over all it covers.
    export void drawCelestialGlow(const SceneFrame&, FloatPoint center, Color glow);
    // How much of the glow about `center` lies over `point`, as drawCelestialGlow lays it.
    export [[nodiscard]] float celestialGlowAt(const SceneFrame&, FloatPoint center,
        FloatPoint point);
}
