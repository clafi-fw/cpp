module ClaFi.PathArt.RainEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Each drop falls the frame's height four to six times a cycle at pace 1.
    void drawRain(const SceneFrame& frame, const float cycleTime, const Rain& rain)
    {
        if (rain.opacity <= 0.001f)
            return;

        const float w = frame.width;
        const float h = frame.height;
        const float tilt = 0.15f;
        const float s = std::sin(tilt);
        const float c = std::cos(tilt);
        const SolidColor drop{ rain.color.withOpacity(rain.opacity * 0.5f) };
        for (int i = 0; i < rain.count; ++i)
        {
            const float x = pseudoRandom(i, rain.seed) * w;
            const float loopCount = std::max(1.0f,
                std::round(static_cast<float>(4 + (i % 3)) * rain.pace));
            const float y = std::fmod(pseudoRandom(i, rain.seed + 2.0f) * h
                + cycleTime * (loopCount * h), h);
            const float len = 25.0f * frame.unit;
            PixelPath path;
            path.moveTo(x, y);
            path.lineTo(x - len * s, y + len * c);
            strokeShape(frame, path, drop, 1.5f * frame.unit);
        }
    }
}
