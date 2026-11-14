module ClaFi.PathArt.Air;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        // Frame heights, as seen at parallax 1, over which the air thins to 1/e.
        constexpr float k_airScaleHeight{ 24.0f };
        // skyBottom's share in the air's colour, which the haze takes a layer toward.
        constexpr float k_hazeSkyBottomShare{ 0.33f };
    }

    Color hazed(const SceneTheme& theme, const Color clear, const float parallax,
        const float aboveGround)
    {
        const float distance = 1.0f / parallax;
        float air = distance - 1.0f;
        if (aboveGround > 0.0f)
        {
            const float thinning = aboveGround / k_airScaleHeight;
            air = (std::exp(-thinning) - std::exp(-thinning * distance)) / thinning;
        }
        const float referenceSpans = air / (1.0f / k_hazeReferenceParallax - 1.0f);
        const Color haze = colorBetween(theme.skyTop, theme.skyBottom, k_hazeSkyBottomShare);
        return colorBetween(clear, haze, 1.0f - std::pow(1.0f - theme.airHaze, referenceSpans));
    }
}
