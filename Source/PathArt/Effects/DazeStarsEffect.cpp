module ClaFi.PathArt.DazeStarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // The circle is 40 units wide and 12 tall: a ring seen from the side.
    void drawDazeStars(const SceneFrame& frame, const DazeStars& stars)
    {
        const float unit = frame.unit;
        for (int i = 0; i != 3; ++i)
        {
            const float angle = k_2Pi * (stars.turns + static_cast<float>(i) / 3.0f);
            const FloatPoint round = { std::cos(angle) * 20.0f, std::sin(angle) * 6.0f };
            PixelPath star;
            addSparkle(star, stars.center + round * unit, 6.0f * unit);
            strokeShape(frame, star, SolidColor{ stars.outline }, 1.4f * unit);
            fillShape(frame, star, SolidColor{ stars.color });
        }
    }
}
