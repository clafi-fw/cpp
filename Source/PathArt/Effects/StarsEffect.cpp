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
        // How many opacities a field of dots is drawn in, one fill each on Direct2D.
        constexpr int k_starOpacitySteps{ 6 };

        // Each star takes the opacity of the step it stands at.
        void drawDots(const SceneFrame& frame, const float cycleTime, const StarField& field)
        {
            std::vector<ScatteredShapes> steps;
            steps.reserve(k_starOpacitySteps);
            for (int step = 0; step != k_starOpacitySteps; ++step)
            {
                const float strength = (static_cast<float>(step) + 0.5f) / k_starOpacitySteps;
                steps.emplace_back(frame,
                    SolidColor{ field.color.withOpacity(strength * 0.9f * field.opacity) });
            }
            for (int i = 0; i != field.count; ++i)
            {
                const float x = pseudoRandom(i, field.seed) * frame.width;
                const float y = std::lerp(field.top, field.bottom,
                    pseudoRandom(i, field.seed + 1.0f));
                const float speed = field.twinklesPerCycle * static_cast<float>(1 + i % 4);
                const float wave = std::sin(k_2Pi * speed * cycleTime
                    + static_cast<float>(i) * 1.7f);
                const float twinkle = 0.5f + 0.5f * wave;
                const float strength = 0.25f + 0.75f * twinkle;
                const int step = std::min(k_starOpacitySteps - 1,
                    static_cast<int>(strength * k_starOpacitySteps));
                const float radius = (0.6f + 1.3f * pseudoRandom(i, field.seed + 2.0f))
                    * frame.unit;
                PixelPath& path = steps[static_cast<std::size_t>(step)].next();
                if (pseudoRandom(i, field.seed + 3.0f) > 0.9f)
                    addSparkle(path, { x, y }, radius * 2.6f * (0.6f + 0.4f * twinkle));
                else
                    addEllipse(path, { x, y }, radius, radius);
            }
            for (ScatteredShapes& stars : steps)
                stars.draw();
        }

        // The stars are filled one by one: two that overlap would cut holes in each other in one
        // path.
        void drawSparkles(const SceneFrame& frame, const float cycleTime, const StarField& field)
        {
            const SolidColor color{ field.color.withOpacity(field.opacity) };
            for (int i = 0; i != field.count; ++i)
            {
                const float speed = std::floor(1.0f + pseudoRandom(i, field.seed + 2.0f) * 3.0f)
                    * field.twinklesPerCycle;
                const float wave = std::sin(cycleTime * k_2Pi * speed + static_cast<float>(i));
                const float twinkle = (wave + 1.0f) * 0.5f;
                if (twinkle <= 0.2f)
                    continue;
                const float shown = (twinkle - 0.2f) / 0.8f;
                const float radius = (1.0f + pseudoRandom(i, field.seed + 3.0f) * 2.0f)
                    * frame.unit * shown;
                const FloatPoint center = {
                    pseudoRandom(i, field.seed) * frame.width,
                    std::lerp(field.top, field.bottom, pseudoRandom(i, field.seed + 1.0f))
                };
                PixelPath path;
                addSparkle(path, center, radius);
                fillShape(frame, path, color);
            }
        }
    }

    void drawStars(const SceneFrame& frame, const float cycleTime, const StarField& field)
    {
        if (field.opacity <= 0.001f)
            return;
        switch (field.style)
        {
            case StarStyle::Dots:
                drawDots(frame, cycleTime, field);
                break;
            case StarStyle::Sparkles:
                drawSparkles(frame, cycleTime, field);
                break;
        }
    }
}
