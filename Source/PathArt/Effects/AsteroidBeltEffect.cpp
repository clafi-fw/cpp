module ClaFi.PathArt.AsteroidBeltEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawAsteroidBelt(const SceneFrame& frame, const float cycleTime, const AsteroidBelt& belt,
        const BeltHalf half)
    {
        const bool frontHalf = half == BeltHalf::Near;
        // Every fifth rock is pale, and the pale ones go down over the others.
        for (const bool pale : { false, true })
        {
            ScatteredShapes rocks{ frame, SolidColor{ pale ? belt.paleRock : belt.rock } };
            for (int i = 0; i != belt.count; ++i)
            {
                if ((i % 5 == 0) != pale)
                    continue;
                const float angle = k_2Pi * (pseudoRandom(i, 4.0f)
                    + belt.orbitsPerCycle * cycleTime);
                if ((std::sin(angle) >= 0.0f) != frontHalf)
                    continue;
                const float radius = std::lerp(belt.innerRadius, belt.outerRadius,
                    pseudoRandom(i, 3.0f));
                // The belt has some thickness, and a rock on the near side is a little larger.
                const float scatter = (pseudoRandom(i, 6.0f) - 0.5f) * 7.0f * frame.unit;
                const FloatPoint at = {
                    belt.center.x + std::cos(angle) * radius,
                    belt.center.y + std::sin(angle) * radius * belt.tilt + scatter
                };
                const float nearness = 1.0f + 0.12f * std::sin(angle);
                const float size = (0.7f + 1.3f * pseudoRandom(i, 5.0f)) * frame.unit * nearness;
                addEllipse(rocks.next(), at, size, size);
            }
            rocks.draw();
        }
    }
}
