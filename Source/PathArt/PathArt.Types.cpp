module ClaFi.PathArt.Types;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    Color SceneTheme::hazed(const Color clear, const float parallax, const float aboveGround) const
    {
        // Frame heights, as seen at parallax 1, over which the air thins to 1/e.
        constexpr float k_airScaleHeight{ 24.0f };
        // skyBottom's share in the air's color, which the haze takes a layer toward.
        constexpr float k_hazeSkyBottomShare{ 0.33f };

        const float distance = 1.0f / parallax;
        float air = distance - 1.0f;
        if (aboveGround > 0.0f)
        {
            const float thinning = aboveGround / k_airScaleHeight;
            air = (std::exp(-thinning) - std::exp(-thinning * distance)) / thinning;
        }
        const float referenceSpans = air / (1.0f / SceneTheme::HazeReferenceParallax - 1.0f);
        const Color haze = Color::between(skyTop, skyBottom, k_hazeSkyBottomShare);
        return Color::between(clear, haze, 1.0f - std::pow(1.0f - airHaze, referenceSpans));
    }

    Color SceneTheme::shadedForeground(const Color clear) const
    {
        constexpr float k_foregroundShade{ 0.25f };
        Hsl shaded{ clear };
        shaded.luminosity *= 1.0f - k_foregroundShade;
        shaded.saturation *= 1.0f - k_foregroundShade;
        Color result = shaded.toColor();
        result.alpha = clear.alpha;
        return result;
    }
}
