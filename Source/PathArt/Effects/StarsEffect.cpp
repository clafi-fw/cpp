module ClaFi.PathArt.StarsEffect;

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
        // Where star `index` stands, moved by the drift and brought round into the band again.
        [[nodiscard]] FloatPoint starAt(const SceneFrame& frame, const StarField& field,
            const int index)
        {
            const float across = fractionalPart(pseudoRandom(index, field.seed) + field.drift.x);
            const float down = fractionalPart(pseudoRandom(index, field.seed + 1.0f)
                + field.drift.y);
            return { across * frame.width, std::lerp(field.top, field.bottom, down) };
        }
    }

    void drawStars(const SceneFrame& frame, const float cycleTime, const StarField& field)
    {
        if (field.opacity <= 0.001f)
        {
            return;
        }

        const SolidColor color{ field.color.withOpacity(field.opacity) };
        for (int i = 0; i != field.count; ++i)
        {
            const float speed = std::floor(1.0f + pseudoRandom(i, field.seed + 2.0f) * 3.0f)
                * field.twinklesPerCycle;
            const float wave = std::sin(cycleTime * k_2Pi * speed + static_cast<float>(i));
            const float twinkle = (wave + 1.0f) * 0.5f;
            if (twinkle <= 0.2f)
            {
                continue;
            }
            const float visibleShare = (twinkle - 0.2f) / 0.8f;
            const float radius = (1.0f + pseudoRandom(i, field.seed + 3.0f) * 2.0f)
                * frame.unit * visibleShare;
            PixelPath path;
            addSparkle(path, starAt(frame, field, i), radius);
            // Fill each star on its own so overlapping stars blend together.
            fillShape(frame, path, color);
        }
    }
}
