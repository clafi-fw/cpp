module ClaFi.PathArt.SparkleBurstEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawSparkleBurst(const SceneFrame& frame, const SparkleBurst& burst)
    {
        const float age = burst.age;
        if (age <= 0.0f || age >= 1.0f)
            return;
        const float unit = frame.unit * burst.size;
        const float count = static_cast<float>(burst.count);
        for (int i = 0; i != burst.count; ++i)
        {
            const float angle = k_2Pi * (static_cast<float>(i) + pseudoRandom(i, burst.seed))
                / count;
            const float reach = (8.0f + 58.0f * std::sqrt(age)
                * (0.55f + 0.45f * pseudoRandom(i, burst.seed + 1.0f))) * unit;
            const float radius = 7.0f * (1.0f - age)
                * (0.6f + 0.4f * pseudoRandom(i, burst.seed + 2.0f)) * unit;
            const FloatPoint at = {
                burst.center.x + std::cos(angle) * reach,
                burst.center.y + std::sin(angle) * reach - 10.0f * age * unit,
            };
            const Color color = burst.colors[static_cast<std::size_t>(i) % burst.colors.size()];
            drawSparkle(frame, at, radius, faded(color, 1.0f - age * age));
        }
    }
}
