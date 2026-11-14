module ClaFi.PathArt.CelestialSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // A crater on the moon, in units from the disc's middle.
        struct Crater
        {
            FloatPoint middle;
            FloatPoint radii;
            float opacity;
        };

        constexpr float k_discRadius{ 24.0f };
        constexpr float k_rimWidth{ 18.0f };
        constexpr float k_glowSpread{ 130.0f };
        constexpr float k_glowOpacity{ 0.65f };
        // The glow is twice as wide as it is tall, spread along the horizon.
        constexpr float k_glowWidening{ 2.0f };
        // The Ghost scene's three craters, at the size of this disc.
        constexpr std::array<Crater, 3> k_craters{
            Crater{ { -7.3f, -5.2f }, { 4.7f, 3.7f }, 0.55f },
            Crater{ { 8.3f, 6.3f }, { 6.3f, 4.7f }, 0.45f },
            Crater{ { 2.1f, -11.5f }, { 2.6f, 2.1f }, 0.5f }
        };
        // How much darker a crater is than the disc, as the Ghost scene's are than its moon.
        constexpr float k_craterShade{ 0.2f };
    }

    void drawCelestial(const SceneFrame& frame, const CelestialPose& pose, const Color disc)
    {
        const float unit = frame.unit;
        PixelPath outline;
        outline.drawCircle(pose.center, k_discRadius * unit);
        const PathDrawLayer rim = PathDrawLayer::OuterGlow(disc.withOpacity(0.75f),
            k_rimWidth * unit);
        frame.canvas.drawPath(outline, { rim, PathDrawLayer::Fill(disc) }, &frame.toCanvas);
        if (pose.night <= 0.0f)
            return;
        const Color crater = colorBetween(disc, Color{ 0xFF000000 }, k_craterShade);
        for (const Crater& each : k_craters)
        {
            const FloatPoint middle = {
                pose.center.x + each.middle.x * unit,
                pose.center.y + each.middle.y * unit,
            };
            fillOval(frame, middle, each.radii.x * unit, each.radii.y * unit,
                SolidColor{ faded(crater, each.opacity * pose.night) });
        }
    }

    void drawCelestialGlow(const SceneFrame& frame, const FloatPoint center, const Color glow)
    {
        drawGlow(frame, center, k_glowSpread * frame.unit, glow.withOpacity(k_glowOpacity),
            1.0f / k_glowWidening);
    }

    float celestialGlowAt(const SceneFrame& frame, const FloatPoint center, const FloatPoint point)
    {
        const float spread = k_glowSpread * frame.unit;
        const float across = (point.x - center.x) / (spread * k_glowWidening);
        const float down = (point.y - center.y) / spread;
        return k_glowOpacity * std::max(0.0f, 1.0f - std::sqrt(across * across + down * down));
    }
}
