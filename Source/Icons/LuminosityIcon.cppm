export module ClaFi.Icons.LuminosityIcon;

import ClaFi.Icons.ColorChannelSweep;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::LuminosityIcon
{
    using namespace ::ClaFi::Graphics;
    namespace Sweep = ::ClaFi::Icons::ColorChannelSweep;

    export void paint(PaintIconEvent&);


    //----------------------------------------------------------------------------


    namespace
    {
        // How far in from each end of the theme's own range the ramp stops. The low end would
        // otherwise be the surface the icon is drawn on, and that part of the sweep would stop
        // being part of the sweep.
        constexpr float k_lowInset = 0.25f;
        constexpr float k_highInset = 0.04f;
    }

    void paint(PaintIconEvent& event)
    {
        // The ramp runs where the elevation slider runs - from the form surface up to the text
        // colour - so the icon shows the range the control actually moves through. It follows the
        // colour mode for free, both ends being the theme's own, and reverses on a light theme
        // because that is the direction the slider reverses too.
        Color startColor = event.textRgb(InkGrade::Faint);
        Color endColor = event.textRgb(InkGrade::Strongest);

        // Nothing to fade by hand: both ends come off textColor(), which already carries it, and
        // a blend of two faded colours is faded.
        Sweep::BoundaryColors colors{};

        for (int i = 0; i <= Sweep::k_segmentCount; ++i)
        {
            float position = static_cast<float>(i) / Sweep::k_segmentCount;
            float mix = k_lowInset + (1.0f - k_lowInset - k_highInset) * position;
            colors[i] = Color{ startColor, endColor, mix };
        }

        Sweep::drawChannelSweep(event.canvas(), event.iconRect(), colors, Sweep::SweepEnds::Stepped);
    }

}
