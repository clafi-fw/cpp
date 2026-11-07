module ClaFi.PathArt.SteamEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawSteam(const SceneFrame& frame, const SteamBurst& burst)
    {
        const float size = burst.size;
        for (int i = 0; i != 12; ++i)
        {
            const float age = (burst.time - static_cast<float>(i) * 0.018f) / 0.5f;
            if (age <= 0.0f || age >= 1.0f)
                continue;
            const FloatPoint puff = {
                burst.vent.x + (pseudoRandom(i, 31.0f) - 0.5f) * 40.0f * size * age
                    + 30.0f * size * age,
                burst.vent.y - 150.0f * size * std::pow(age, 0.8f)
            };
            const float fade = 1.0f - age;
            fillDisc(frame, puff, (6.0f + 26.0f * age) * size,
                SolidColor{ burst.color.withOpacity(0.42f * fade * fade) });
        }
    }
}
