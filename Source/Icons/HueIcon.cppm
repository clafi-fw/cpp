export module ClaFi.Icons.HueIcon;

import ClaFi.Icons.ColorChannelSweep;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::HueIcon
{
    using namespace ::ClaFi::Graphics;
    namespace Sweep = ::ClaFi::Icons::ColorChannelSweep;

    export void paint(PaintIconEvent&);


    //----------------------------------------------------------------------------


    namespace
    {
        // The point of the icon is the hue axis, so the other two channels are pinned. Saturation
        // sits at half rather than full: at full the sweep is the loudest thing in a row of column
        // headers, and it has to sit next to two that are mid-saturation by construction.
        constexpr float k_saturation = 0.5f;
        constexpr float k_luminosity = 0.55f;
    }

    void paint(PaintIconEvent& event)
    {
        // The wheel is the icon's own - it is the same hues whatever the theme is - so every
        // colour it produces goes through the event's fade, which is the one thing it would
        // otherwise miss by not taking its ink from textColor().
        Sweep::BoundaryColors colors{};

        for (int i = 0; i <= Sweep::k_segmentCount; ++i)
        {
            float hue = static_cast<float>(i) / Sweep::k_segmentCount;
            colors[i] = event.applyDisabledFactor(Hsl{ hue, k_saturation, k_luminosity }.toColor());
        }

        // Cyclic: the hue at the seam is the same colour from both sides, so the sweep runs across
        // it and the join disappears. The other two channels have two distinct ends and keep theirs.
        Sweep::drawChannelSweep(event.canvas(), event.iconRect(), colors, Sweep::SweepEnds::Cyclic);
    }

}
