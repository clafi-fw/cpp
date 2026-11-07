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
        const SolidColor streak{ dust.color.withOpacity(dust.opacity) };
        for (int i = 0; i < dust.count; ++i)
        {
            const float x = std::fmod(pseudoRandom(i) * w + cycleTime * w * 3.0f, w);
            const float y = pseudoRandom(i, 7.0f) * h;
            PixelPath path;
            path.moveTo(x, y);
            path.lineTo(x + 10.0f * unit, y + 2.0f * unit);
            strokeShape(frame, path, streak, 1.0f);
        }
    }
}
