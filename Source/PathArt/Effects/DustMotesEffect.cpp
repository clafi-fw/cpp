module ClaFi.PathArt.DustMotesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // A mote scatters wider the nearer the floor it is, as the beam widens.
    void drawDustMotes(const SceneFrame& frame, const float cycleTime, const DustMotes& motes)
    {
        for (int i = 0; i != motes.count; ++i)
        {
            const float rise = fractionalPart(pseudoRandom(i, 1.0f)
                - cycleTime * motes.risesPerCycle);
            const float y = std::lerp(motes.pool.y, motes.top, rise);
            const float depth = (y - motes.lamp.y) / (motes.pool.y - motes.lamp.y);
            const float scatter = (pseudoRandom(i) - 0.5f) * motes.spread;
            const float x = std::lerp(motes.lamp.x, motes.pool.x, depth) + scatter * depth;
            const float radius = (0.7f + 1.1f * pseudoRandom(i, 2.0f)) * frame.unit;
            const float opacity = 0.45f * std::sin(std::numbers::pi_v<float> * rise);
            fillDisc(frame, { x, y }, radius, SolidColor{ motes.color.withOpacity(opacity) });
        }
    }
}
