module ClaFi.PathArt.PlanetSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace PlanetColors
    {
        constexpr Color Night{ 0xFF04050D };
        constexpr Color White{ 0xFFFFFFFF };
        constexpr Color EarthHalo{ 0xFF7CC4FF };
        constexpr Color Land{ 0xF24F9A45 };
        constexpr Color Ice{ 0xE6F2F6FF };
        constexpr Color MarsCap{ 0xE6F4EFE8 };
        constexpr Color MarsPlain{ 0x8C8F3D22 };
        constexpr Color GreatSpot{ 0xE6B5452F };
        constexpr Color Moon{ 0xFFD4D4D4 };
        constexpr Color RingOuterBehind{ 0xB3D8C08A };
        constexpr Color RingInnerBehind{ 0x8CA8915E };
        constexpr Color RingOuterFront{ 0xCCE6D09C };
        constexpr Color RingInnerFront{ 0xA6B8A06A };
        constexpr std::array<Color, 6> JupiterBands{
            Color{ 0xD9F0D6A8 }, Color{ 0xD9B97A4A }, Color{ 0xD9E8C48F },
            Color{ 0xD9A8693F }, Color{ 0xD9F0D6A8 }, Color{ 0xD9C58D5A }
        };
    }

    namespace
    {
        void drawSaturnRings(const SceneFrame& frame, const PlanetPose& saturn,
            const bool frontHalf)
        {
            const SceneFrame rings = frame.within(Matrix3x2::Translation(saturn.center)
                * Matrix3x2::Rotation(-0.32f));
            const float radius = saturn.radius;
            const auto half = [&](float ringRadius, float width, Color color) {
                PixelPath arc;
                arc.moveTo(frontHalf ? ringRadius : -ringRadius, 0.0f);
                arc.arcTo({ ringRadius, ringRadius * 0.42f }, 0.0f, ArcSize::Small,
                    ArcSweep::Clockwise, { frontHalf ? -ringRadius : ringRadius, 0.0f });
                strokeShape(rings, arc, SolidColor{ color }, width, StrokeCap::Butt);
            };
            half(radius * 2.15f, radius * 0.42f,
                frontHalf ? PlanetColors::RingOuterFront : PlanetColors::RingOuterBehind);
            half(radius * 1.55f, radius * 0.22f,
                frontHalf ? PlanetColors::RingInnerFront : PlanetColors::RingInnerBehind);
        }

        void drawMoon(const SceneFrame& frame, const PlanetPose& earth)
        {
            const float turn = k_2Pi * (16.0f * earth.cycleTime + 0.4f);
            const float orbitRadius = earth.radius * 2.4f;
            const FloatPoint offset = { std::cos(turn), std::sin(turn) * earth.orbitTilt };
            const FloatPoint center = earth.center + offset * orbitRadius;
            const float radius = earth.radius * 0.3f;
            fillDisc(frame, center, radius, SolidColor{ PlanetColors::Moon });
            shadeBall(frame, center, radius, earth.light, PlanetColors::Night.withOpacity(0.8f));
        }
    }

    void drawPlanet(const SceneFrame& frame, const PlanetPose& pose)
    {
        const Planet planet = pose.planet;
        const FloatPoint center = pose.center;
        const float radius = pose.radius;
        const float cycleTime = pose.cycleTime;
        const float moonTurn = k_2Pi * (16.0f * cycleTime + 0.4f);

        if (planet == Planet::Saturn)
            drawSaturnRings(frame, pose, false);
        if (planet == Planet::Earth && std::sin(moonTurn) < 0.0f)
            drawMoon(frame, pose);
        if (planet == Planet::Earth)
            drawGlow(frame, center, radius * 1.8f, PlanetColors::EarthHalo.withOpacity(0.35f));

        PixelPath disc;
        disc.drawCircle(center, radius);
        const Matrix3x2 toCanvas = frame.toCanvas;
        frame.canvas.pushClip(disc, &toCanvas);
        const FloatPoint corner = { radius, radius };
        fillShape(frame, disc, LinearGradient::Simple(center - corner, center + corner,
            colorBetween(pose.color, PlanetColors::White, 0.25f), pose.color));
        switch (planet)
        {
            case Planet::Jupiter:
            {
                const std::size_t bandCount = PlanetColors::JupiterBands.size();
                const float bandStep = radius * 2.0f / static_cast<float>(bandCount + 1);
                for (std::size_t i = 0; i != bandCount; ++i)
                {
                    const float top = center.y - radius + (static_cast<float>(i) + 0.5f) * bandStep;
                    fillBox(frame,
                        { center.x - radius, top, center.x + radius, top + radius * 0.22f },
                        SolidColor{ PlanetColors::JupiterBands[i] });
                }
                // The spot turns with the planet, eight times a cycle, and shows on the near side.
                const float spotTurn = k_2Pi * (8.0f * cycleTime + 0.2f);
                if (std::cos(spotTurn) > 0.0f)
                {
                    const FloatPoint spot = {
                        center.x + std::sin(spotTurn) * radius * 0.75f,
                        center.y + radius * 0.3f
                    };
                    fillOval(frame, spot, radius * 0.22f * std::cos(spotTurn), radius * 0.13f,
                        SolidColor{ PlanetColors::GreatSpot });
                }
                break;
            }
            case Planet::Earth:
                for (int i = 0; i != 3; ++i)
                {
                    const float start = static_cast<float>(i) / 3.0f;
                    const float landTurn = k_2Pi * (12.0f * cycleTime + start);
                    if (std::cos(landTurn) > 0.0f)
                    {
                        const FloatPoint land = {
                            center.x + std::sin(landTurn) * radius * 0.7f,
                            center.y + static_cast<float>(i - 1) * radius * 0.45f
                        };
                        fillOval(frame, land, radius * 0.42f * std::cos(landTurn), radius * 0.3f,
                            SolidColor{ PlanetColors::Land });
                    }
                }
                fillOval(frame, { center.x, center.y - radius * 0.92f }, radius * 0.6f,
                    radius * 0.22f, SolidColor{ PlanetColors::Ice });
                break;
            case Planet::Mars:
                fillOval(frame, { center.x, center.y - radius * 0.9f }, radius * 0.5f,
                    radius * 0.2f, SolidColor{ PlanetColors::MarsCap });
                fillOval(frame, { center.x - radius * 0.2f, center.y + radius * 0.1f },
                    radius * 0.4f, radius * 0.2f, SolidColor{ PlanetColors::MarsPlain });
                break;
            case Planet::Venus:
            case Planet::Saturn:
                for (int i = -1; i <= 1; ++i)
                {
                    const float bandY = center.y + static_cast<float>(i) * radius * 0.45f;
                    const FloatRect band = {
                        center.x - radius,
                        bandY - radius * 0.08f,
                        center.x + radius,
                        bandY + radius * 0.08f
                    };
                    fillBox(frame, band, SolidColor{ PlanetColors::White.withOpacity(0.12f) });
                }
                break;
            case Planet::Mercury:
                break;
        }
        shadeBall(frame, center, radius, pose.light, PlanetColors::Night.withOpacity(0.86f));
        frame.canvas.popClip();

        if (planet == Planet::Saturn)
            drawSaturnRings(frame, pose, true);
        if (planet == Planet::Earth && std::sin(moonTurn) >= 0.0f)
            drawMoon(frame, pose);
    }

    void shadeBall(const SceneFrame& frame, const FloatPoint center, const float radius,
        const LightDirection light, const Color shade)
    {
        // Turned so the light comes from +x: the dark side is the half disc on -x, closed by the
        // terminator, a half ellipse as wide as the light comes toward the viewer.
        const SceneFrame ball = frame.within(Matrix3x2::Translation(center)
            * Matrix3x2::Rotation(std::atan2(light.y, light.x)));
        const float reach = radius * 1.02f;
        const float terminatorWidth = std::max(std::abs(light.toViewer) * radius, 0.01f);
        PixelPath dark;
        dark.moveTo(0.0f, -reach);
        dark.arcTo({ reach, reach }, 0.0f, ArcSize::Small, ArcSweep::CounterClockwise,
            { 0.0f, reach });
        dark.lineTo(0.0f, radius);
        const ArcSweep terminatorSweep = light.toViewer >= 0.0f
            ? ArcSweep::Clockwise
            : ArcSweep::CounterClockwise;
        dark.arcTo({ terminatorWidth, radius }, 0.0f, ArcSize::Small, terminatorSweep,
            { 0.0f, -radius });
        dark.close();
        fillShape(ball, dark, SolidColor{ shade });
    }
}
