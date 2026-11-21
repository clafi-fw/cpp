module ClaFi.PathArt.StarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawStars(const SceneFrame& frame, const float cycleTime, const StarField& field)
    {
        if (field.opacity <= 0.001f)
        {
            return;
        }

        for (int i = 0; i < field.count; ++i)
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
            const float across = fractionalPart(pseudoRandom(i, field.seed) + field.drift.x);
            const float down = fractionalPart(pseudoRandom(i, field.seed + 1.0f) + field.drift.y);
            const FloatPoint center = {
                across * frame.width,
                std::lerp(field.top, field.bottom, down)
            };
            const float brightnessRoll = pseudoRandom(i, field.seed + 4.0f);
            const float brightness = std::lerp(0.22f, 1.0f, brightnessRoll * brightnessRoll);
            const float twinkleBrightness = std::lerp(0.55f, 1.0f, visibleShare);
            const float skyBrightness = std::lerp(1.0f, field.bottomBrightness,
                smoothStep(0.2f, 1.0f, down));
            const float opacity = field.opacity * brightness * twinkleBrightness * skyBrightness;
            const SolidColor color = { field.color.withOpacity(opacity) };
            PixelPath path;
            addSparkle(path, center, radius);
            // Fill each star on its own so overlapping stars blend together.
            fillShape(frame, path, color);
        }
    }
}
