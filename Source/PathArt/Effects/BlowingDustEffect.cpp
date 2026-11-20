module ClaFi.PathArt.BlowingDustEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // The share of a mote's own drift its gusts may take back at their strongest.
        constexpr float k_gustHold{ 0.8f };
    }

    void drawBlowingDust(const SceneFrame& frame, const float cycleTime, const BlowingDust& dust)
    {
        if (dust.opacity <= 0.001f)
            return;

        const float w = frame.width;
        const float h = frame.height;
        const float unit = frame.unit;
        const float turn = cycleTime * k_2Pi;
        for (int i = 0; i < dust.count; ++i)
        {
            const float seed = dust.seed;
            const Color color = Color::between(dust.color1, dust.color2, pseudoRandom(i, seed + 9.0f));
            const float widths = std::max(1.0f,
                std::round((1.5f + 3.0f * pseudoRandom(i, seed + 1.0f)) * dust.pace));
            const float drift = w * widths; // in pixels a cycle
            // Gusts push a mote on and hold it back, and eddies lift and drop it, each a whole
            // number of turns a cycle, so the cycle closes.
            const float gustTurns = std::round(4.0f + 6.0f * pseudoRandom(i, seed + 2.0f));
            const float liftTurns = std::round(5.0f + 9.0f * pseudoRandom(i, seed + 3.0f));
            const float gustAngle = turn * gustTurns + k_2Pi * pseudoRandom(i, seed + 4.0f);
            const float liftAngle = turn * liftTurns + k_2Pi * pseudoRandom(i, seed + 5.0f);
            const float gustReach = std::min((10.0f + 30.0f * pseudoRandom(i, seed + 6.0f)) * unit,
                k_gustHold * drift / (k_2Pi * gustTurns));
            const float liftReach = (4.0f + 14.0f * pseudoRandom(i, seed + 8.0f)) * unit;
            const float along = pseudoRandom(i, seed) * w + cycleTime * drift
                + gustReach * std::sin(gustAngle);
            const float x = w - std::fmod(along + w, w);
            const float y = pseudoRandom(i, seed + 7.0f) * h + liftReach * std::sin(liftAngle);
            // The streak lies along the way the mote moves, longer the faster it goes.
            const FloatPoint velocity = {
                -(drift + gustReach * k_2Pi * gustTurns * std::cos(gustAngle)),
                liftReach * k_2Pi * liftTurns * std::cos(liftAngle)
            };
            const float speed = std::max(0.001f, std::hypot(velocity.x, velocity.y));
            const float length = 2.5f * unit * std::clamp(speed / drift, 0.4f, 2.0f);
            PixelPath path;
            path.moveTo(x, y);
            path.lineTo(x + velocity.x / speed * length, y + velocity.y / speed * length);
            strokeShape(frame, path, SolidColor{ color.faded(dust.opacity) }, unit);
        }
    }
}
