export module ClaFi.Icons.SaturationIcon;

import ClaFi.Icons.ColorChannelSweep;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::SaturationIcon
{
    using namespace ::ClaFi::Graphics;
    namespace Sweep = ::ClaFi::Icons::ColorChannelSweep;

    export void paint(PaintIconEvent&);


    //----------------------------------------------------------------------------


    namespace
    {
        // Luminosity is pinned near mid so the two ends differ in the channel the icon is about. It
        // still has to move a little: a fully desaturated grey and a saturated colour at one
        // luminosity carry different apparent weight, and the drift compensates. The pair flips on a
        // light theme, where the same reading needs the ramp to run the other way.
        constexpr float k_endLum = 0.7f;
        constexpr float k_startLum = 0.45f;
    }

    void paint(PaintIconEvent& event)
    {
        // The saturated end follows the theme accent, so the icon reads as part of the theme it is
        // sitting in rather than as a fixed sample. The accent rule names a palette hue; anything
        // that leaves the hue alone falls back to the anchor.
        const BakedColors& bakedColors = event.bakedColors();
        const BakedHue& accent = bakedColors.rule(UiElement::Accent).hue;
        float accentHue = accent.setPull > 0.0f ? accent.hue : bakedColors.anchorHue;

        float startLum = std::lerp(k_startLum, 1.0f - k_startLum, bakedColors.lightness);
        float endLum = std::lerp(k_endLum, 1.0f - k_endLum, bakedColors.lightness);

        Sweep::BoundaryColors colors{};

        for (int i = 0; i <= Sweep::k_segmentCount; ++i)
        {
            float saturation = static_cast<float>(i) / Sweep::k_segmentCount;
            float currLum = startLum * (1.0f - saturation) + endLum * saturation;
            // Squared, because saturation reads faster than it climbs: a linear ramp spends most of
            // the sweep looking saturated, and the grey end is what the icon is contrasting against.
            saturation *= saturation;
            // Mixed here rather than taken from textColor(), so the fade has to be applied by
            // hand - the accent hue is the theme's, but this luminosity ramp is the icon's own.
            colors[i] = event.applyDisabledFactor(Hsl{ accentHue, saturation, currLum }.toColor());
        }

        Sweep::drawChannelSweep(event.canvas(), event.iconRect(), colors, Sweep::SweepEnds::Stepped);
    }

}
