module ClaFi.PathArt.FirefliesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawFireflies(const SceneFrame& frame, const float cycleTime, const Fireflies& fireflies)
    {
        const float opacity = fireflies.opacity;
        if (opacity <= 0.001f)
            return;

        const float t = cycleTime;
        const FloatRect& area = fireflies.area;
        const float unit = frame.unit * fireflies.size;
        for (int i = 0; i < fireflies.count; ++i)
        {
            const float index = static_cast<float>(i);
            const float xBase = area.left + pseudoRandom(i) * area.width();
            const float yBase = area.top + pseudoRandom(i, 5.0f) * area.height();
            const float dx = std::sin(t * k_2Pi * 1.0f + index) * 50.0f * unit;
            const float dy = std::cos(t * k_2Pi * 1.0f + index * 0.7f) * 30.0f * unit;
            const float flicker = (std::sin(t * k_2Pi * 2.0f + index) + 1.0f) * 0.5f;
            if (flicker > 0.4f)
            {
                const FloatPoint pos = { xBase + dx, yBase + dy };
                drawGlow(frame, pos, 15.0f * unit, faded(fireflies.glow, 0.5f * opacity));
                fillDisc(frame, pos, 3.5f * unit,
                    SolidColor{ fireflies.light.withOpacity(opacity) });
            }
        }
    }
}
