module ClaFi.PathArt.DustPuffEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawDustPuff(const SceneFrame& frame, const DustPuff& puff)
    {
        const float unit = frame.unit;
        const float settle = puff.settle;
        for (int i = 0; i != 6; ++i)
        {
            const float offset = (static_cast<float>(i) - 2.5f) * 16.0f * unit * (0.4f + settle);
            const float rise = (6.0f + 16.0f * settle * pseudoRandom(i, 131.0f)) * unit;
            const float opacity = 0.6f * (1.0f - settle);
            fillDisc(frame, { puff.ground.x + offset, puff.ground.y - rise },
                (5.0f + 12.0f * settle) * unit, SolidColor{ puff.color.withOpacity(opacity) });
        }
    }
}
