module ClaFi.PathArt.BlowingDustEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawBlowingDust(const SceneFrame& frame, const float cycleTime, const BlowingDust& dust)
    {
        if (dust.opacity <= 0.001f)
            return;

        const float w = frame.width;
        const float h = frame.height;
        const float unit = frame.unit;
        const SolidColor streak{ dust.color.faded(dust.opacity) };
        const float widthsPerCycle = std::max(1.0f, std::round(3.0f * dust.pace));
        for (int i = 0; i < dust.count; ++i)
        {
            const float x = w - std::fmod(pseudoRandom(i, dust.seed) * w
                + cycleTime * w * widthsPerCycle, w);
            const float y = pseudoRandom(i, dust.seed + 7.0f) * h;
            PixelPath path;
            path.moveTo(x, y);
            path.lineTo(x - 2.5f * unit, y + 0.5f * unit);
            strokeShape(frame, path, streak, unit);
        }
    }
}
