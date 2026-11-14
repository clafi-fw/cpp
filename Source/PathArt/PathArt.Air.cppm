export module ClaFi.PathArt.Air;

import ClaFi.PathArt.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The share of a passing camera's speed at which a layer takes the theme's airHaze.
    export constexpr float k_hazeReferenceParallax{ 0.2f };

    // A layer moving at `parallax` of a camera's speed, seen through the theme's air. See PathArt
    export [[nodiscard]] Color hazed(const SceneTheme&, Color clear, float parallax,
        float aboveGround = 0.0f);
}
