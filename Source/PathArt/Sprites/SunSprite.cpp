module ClaFi.PathArt.SunSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace SunColors
    {
        constexpr Color SunHalo{ 0xFFFF8C2A };
        constexpr Color SunGlow{ 0xFFFFD27A };
        constexpr Color RayLight{ 0x8CFFE9A8 };
        constexpr Color RayDeep{ 0x8CFFB347 };
        constexpr Color SunCore{ 0xFFFFFCEC };
        constexpr Color SunBody{ 0xFFFFD45A };
        constexpr Color SunRim{ 0xFFFF9D2E };
    }

    void drawSun(const SceneFrame& frame, const SunPose& pose)
    {
        const float unit = frame.unit;
        const FloatPoint center = pose.center;
        const float cycleTime = pose.cycleTime;
        const float radius = 40.0f * unit;
        drawGlow(frame, center, 230.0f * unit, SunColors::SunHalo.withOpacity(0.32f));
        drawGlow(frame, center, 100.0f * unit, SunColors::SunGlow.withOpacity(0.55f));

        // The rays turn once a cycle and each flickers at its own phase.
        std::array<PixelPath, 2> rays{};
        for (int i = 0; i != 22; ++i)
        {
            const float turn = k_2Pi * (cycleTime + static_cast<float>(i) / 22.0f);
            const float phase = static_cast<float>(i) * 1.9f;
            const float wave = std::sin(k_2Pi * pose.flickersPerCycle * cycleTime + phase);
            const float flicker = 0.5f + 0.5f * wave;
            const float length = radius * (1.25f + 0.25f * flicker);
            PixelPath& path = rays[static_cast<std::size_t>(i % 2)];
            path.moveTo(center + rotated({ -radius * 0.14f, -radius * 0.9f }, turn));
            path.lineTo(center + rotated({ radius * 0.14f, -radius * 0.9f }, turn));
            path.lineTo(center + rotated({ 0.0f, -length }, turn));
            path.close();
        }
        fillShape(frame, rays[0], SolidColor{ SunColors::RayLight });
        fillShape(frame, rays[1], SolidColor{ SunColors::RayDeep });
        fillDisc(frame, center, radius, RadialGradient{
            .center = center,
            .offset = { -radius * 0.2f, -radius * 0.2f },
            .radiusX = radius,
            .radiusY = radius,
            .stops = {
                { .position = 0.0f, .color = SunColors::SunCore },
                { .position = 0.65f, .color = SunColors::SunBody },
                { .position = 1.0f, .color = SunColors::SunRim }
            }
        });
    }
}
