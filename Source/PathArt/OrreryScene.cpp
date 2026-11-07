module ClaFi.PathArt.OrreryScene;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace OrreryColors
    {
        constexpr Color SpaceTop{ 0xFF04050D };
        constexpr Color SpaceBottom{ 0xFF0E1130 };
        constexpr Color MilkyWay{ 0xFF6C70B8 };
        constexpr Color Star{ 0xFFEEF0FF };
        constexpr Color Orbit{ 0x299FB0FF };
        constexpr Color Rock{ 0xD98F8577 };
        constexpr Color PaleRock{ 0xD9C9B79C };
        constexpr Color SunHalo{ 0xFFFF8C2A };
        constexpr Color SunGlow{ 0xFFFFD27A };
        constexpr Color RayLight{ 0x8CFFE9A8 };
        constexpr Color RayDeep{ 0x8CFFB347 };
        constexpr Color SunCore{ 0xFFFFFCEC };
        constexpr Color SunBody{ 0xFFFFD45A };
        constexpr Color SunRim{ 0xFFFF9D2E };
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
        // How far the orbit plane is tipped toward the viewer: a circle in it is seen this flat.
        constexpr float k_planeTilt{ 0.5f };
        // The pace things keep at a 40-second cycle, in this scene's 240-second one.
        constexpr float k_lively{ 6.0f };
        constexpr float k_beltOrbitsPerCycle{ 3.0f };
        constexpr int k_asteroidCount{ 280 };
    }

    void OrreryScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const FloatPoint center = { frame.width * 0.5f, frame.height * 0.52f };
        const float outerRadius = std::min(frame.width * 0.44f, frame.height * 0.42f / k_planeTilt);
        const float depthTilt = std::sqrt(1.0f - k_planeTilt * k_planeTilt);

        static constexpr std::array<Orbit, 6> k_orbits{
            Orbit{ Planet::Mercury, 0.2f, 12.0f, 0.1f, 4.6f, Color{ 0xFFA39A90 } },
            Orbit{ Planet::Venus, 0.28f, 8.0f, 0.62f, 8.0f, Color{ 0xFFE7C487 } },
            Orbit{ Planet::Earth, 0.37f, 6.0f, 0.31f, 8.5f, Color{ 0xFF2F73D6 } },
            Orbit{ Planet::Mars, 0.46f, 4.0f, 0.83f, 6.2f, Color{ 0xFFC8613A } },
            Orbit{ Planet::Jupiter, 0.76f, 2.0f, 0.45f, 22.0f, Color{ 0xFFD9A066 } },
            Orbit{ Planet::Saturn, 0.96f, 1.0f, 0.05f, 17.0f, Color{ 0xFFE3C27A } }
        };
        std::array<PlacedPlanet, 6> planets{};
        for (std::size_t i = 0; i != k_orbits.size(); ++i)
        {
            const Orbit& orbit = k_orbits[i];
            const float angle = k_2Pi * (orbit.startTurn + orbit.orbitsPerCycle * cycleTime);
            const float radius = outerRadius * orbit.radiusShare;
            const float across = std::cos(angle) * radius;
            const float toward = std::sin(angle) * radius;
            planets[i] = {
                .orbit = &orbit,
                .center = { center.x + across, center.y + toward * k_planeTilt },
                .depth = std::sin(angle),
                .radius = orbit.planetRadius * unit * (1.0f + 0.12f * std::sin(angle)),
                .light = {
                    .x = -across / radius,
                    .y = -toward * k_planeTilt / radius,
                    .toViewer = -toward * depthTilt / radius
                }
            };
        }
        // Drawn far to near: what is beyond the Sun first, then the Sun, then what is in front of
        // it. The belt lies between the inner planets and the outer two.
        std::array<const PlacedPlanet*, 6> byDepth{};
        for (std::size_t i = 0; i != planets.size(); ++i)
            byDepth[i] = &planets[i];
        std::ranges::sort(byDepth, std::ranges::less{}, &PlacedPlanet::depth);
        const auto isOuter = [](const PlacedPlanet* placed) {
            return placed->orbit->radiusShare >= 0.5f;
        };

        drawSky(frame, cycleTime);
        drawOrbits(frame, center, outerRadius, false);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth < 0.0f && isOuter(placed))
                drawPlanet(frame, *placed, cycleTime);
        }
        drawAsteroids(frame, center, outerRadius, false, cycleTime);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth < 0.0f && !isOuter(placed))
                drawPlanet(frame, *placed, cycleTime);
        }
        drawSun(frame, center, cycleTime);
        drawOrbits(frame, center, outerRadius, true);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth >= 0.0f && !isOuter(placed))
                drawPlanet(frame, *placed, cycleTime);
        }
        drawAsteroids(frame, center, outerRadius, true, cycleTime);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth >= 0.0f && isOuter(placed))
                drawPlanet(frame, *placed, cycleTime);
        }
    }

    void OrreryScene::drawSky(const SceneFrame& frame, const float cycleTime)
    {
        const float width = frame.width;
        const float height = frame.height;
        fillBox(frame, { 0.0f, 0.0f, width, height }, LinearGradient::Simple({ 0.0f, 0.0f },
            { width, height }, OrreryColors::SpaceTop, OrreryColors::SpaceBottom));
        // the Milky Way, a band of glows corner to corner
        for (int i = 0; i != 12; ++i)
        {
            const float along = static_cast<float>(i) / 11.0f;
            const float waver = std::sin(along * 7.0f) * 20.0f * frame.unit;
            const FloatPoint at = {
                std::lerp(-0.1f * width, 1.1f * width, along),
                std::lerp(0.9f * height, 0.05f * height, along) + waver
            };
            drawGlow(frame, at, 110.0f * frame.unit, OrreryColors::MilkyWay.withOpacity(0.07f));
        }
        drawStars(frame, cycleTime, StarField{
            .count = 170,
            .top = 0.0f,
            .bottom = height,
            .seed = 50.0f,
            .color = OrreryColors::Star,
            .twinklesPerCycle = k_lively
        });
    }

    void OrreryScene::drawSun(const SceneFrame& frame, const FloatPoint center,
        const float cycleTime)
    {
        const float unit = frame.unit;
        const float radius = 40.0f * unit;
        drawGlow(frame, center, 230.0f * unit, OrreryColors::SunHalo.withOpacity(0.32f));
        drawGlow(frame, center, 100.0f * unit, OrreryColors::SunGlow.withOpacity(0.55f));

        // The rays turn once a cycle and each flickers at its own phase.
        std::array<PixelPath, 2> rays{};
        for (int i = 0; i != 22; ++i)
        {
            const float turn = k_2Pi * (cycleTime + static_cast<float>(i) / 22.0f);
            const float phase = static_cast<float>(i) * 1.9f;
            const float wave = std::sin(k_2Pi * 4.0f * k_lively * cycleTime + phase);
            const float flicker = 0.5f + 0.5f * wave;
            const float length = radius * (1.25f + 0.25f * flicker);
            PixelPath& path = rays[static_cast<std::size_t>(i % 2)];
            path.moveTo(center + rotated({ -radius * 0.14f, -radius * 0.9f }, turn));
            path.lineTo(center + rotated({ radius * 0.14f, -radius * 0.9f }, turn));
            path.lineTo(center + rotated({ 0.0f, -length }, turn));
            path.close();
        }
        fillShape(frame, rays[0], SolidColor{ OrreryColors::RayLight });
        fillShape(frame, rays[1], SolidColor{ OrreryColors::RayDeep });
        fillDisc(frame, center, radius, RadialGradient{
            .center = center,
            .offset = { -radius * 0.2f, -radius * 0.2f },
            .radiusX = radius,
            .radiusY = radius,
            .stops = {
                { .position = 0.0f, .color = OrreryColors::SunCore },
                { .position = 0.65f, .color = OrreryColors::SunBody },
                { .position = 1.0f, .color = OrreryColors::SunRim }
            }
        });
    }

    // The far halves of the orbits go behind the Sun and the near halves over it.
    void OrreryScene::drawOrbits(const SceneFrame& frame, const FloatPoint center,
        const float outerRadius, const bool frontHalf)
    {
        PixelPath halves;
        for (const float share : { 0.2f, 0.28f, 0.37f, 0.46f, 0.76f, 0.96f })
        {
            const float radius = outerRadius * share;
            const FloatPoint left = { center.x - radius, center.y };
            const FloatPoint right = { center.x + radius, center.y };
            halves.moveTo(frontHalf ? right : left);
            halves.arcTo({ radius, radius * k_planeTilt }, 0.0f, ArcSize::Small,
                ArcSweep::Clockwise, frontHalf ? left : right);
        }
        strokeShape(frame, halves, SolidColor{ OrreryColors::Orbit }, frame.unit);
    }

    void OrreryScene::drawAsteroids(const SceneFrame& frame, const FloatPoint center,
        const float outerRadius, const bool frontHalf, const float cycleTime)
    {
        PixelPath rocks;
        PixelPath paleRocks;
        for (int i = 0; i != k_asteroidCount; ++i)
        {
            const float angle = k_2Pi * (pseudoRandom(i, 4.0f) + k_beltOrbitsPerCycle * cycleTime);
            if ((std::sin(angle) >= 0.0f) != frontHalf)
                continue;
            const float radius = outerRadius * (0.54f + 0.1f * pseudoRandom(i, 3.0f));
            // The belt has some thickness, and a rock on the near side is a little larger.
            const float scatter = (pseudoRandom(i, 6.0f) - 0.5f) * 7.0f * frame.unit;
            const FloatPoint at = {
                center.x + std::cos(angle) * radius,
                center.y + std::sin(angle) * radius * k_planeTilt + scatter
            };
            const float nearness = 1.0f + 0.12f * std::sin(angle);
            const float size = (0.7f + 1.3f * pseudoRandom(i, 5.0f)) * frame.unit * nearness;
            addEllipse(i % 5 == 0 ? paleRocks : rocks, at, size, size);
        }
        if (!rocks.isEmpty())
            fillShape(frame, rocks, SolidColor{ OrreryColors::Rock });
        if (!paleRocks.isEmpty())
            fillShape(frame, paleRocks, SolidColor{ OrreryColors::PaleRock });
    }

    void OrreryScene::drawPlanet(const SceneFrame& frame, const PlacedPlanet& placed,
        const float cycleTime)
    {
        const Planet planet = placed.orbit->planet;
        const FloatPoint center = placed.center;
        const float radius = placed.radius;
        const float moonTurn = k_2Pi * (16.0f * cycleTime + 0.4f);

        if (planet == Planet::Saturn)
            drawSaturnRings(frame, placed, false);
        if (planet == Planet::Earth && std::sin(moonTurn) < 0.0f)
            drawMoon(frame, placed, cycleTime);
        if (planet == Planet::Earth)
            drawGlow(frame, center, radius * 1.8f, OrreryColors::EarthHalo.withOpacity(0.35f));

        PixelPath disc;
        disc.drawCircle(center, radius);
        const Matrix3x2 toCanvas = frame.toCanvas;
        frame.canvas.pushClip(disc, &toCanvas);
        const FloatPoint corner = { radius, radius };
        fillShape(frame, disc, LinearGradient::Simple(center - corner, center + corner,
            colorBetween(placed.orbit->color, OrreryColors::White, 0.25f), placed.orbit->color));
        switch (planet)
        {
            case Planet::Jupiter:
            {
                const std::size_t bandCount = OrreryColors::JupiterBands.size();
                const float bandStep = radius * 2.0f / static_cast<float>(bandCount + 1);
                for (std::size_t i = 0; i != bandCount; ++i)
                {
                    const float top = center.y - radius + (static_cast<float>(i) + 0.5f) * bandStep;
                    fillBox(frame,
                        { center.x - radius, top, center.x + radius, top + radius * 0.22f },
                        SolidColor{ OrreryColors::JupiterBands[i] });
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
                        SolidColor{ OrreryColors::GreatSpot });
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
                            SolidColor{ OrreryColors::Land });
                    }
                }
                fillOval(frame, { center.x, center.y - radius * 0.92f }, radius * 0.6f,
                    radius * 0.22f, SolidColor{ OrreryColors::Ice });
                break;
            case Planet::Mars:
                fillOval(frame, { center.x, center.y - radius * 0.9f }, radius * 0.5f,
                    radius * 0.2f, SolidColor{ OrreryColors::MarsCap });
                fillOval(frame, { center.x - radius * 0.2f, center.y + radius * 0.1f },
                    radius * 0.4f, radius * 0.2f, SolidColor{ OrreryColors::MarsPlain });
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
                    fillBox(frame, band, SolidColor{ OrreryColors::White.withOpacity(0.12f) });
                }
                break;
            case Planet::Mercury:
                break;
        }
        shadeBall(frame, center, radius, placed.light, OrreryColors::Night.withOpacity(0.86f));
        frame.canvas.popClip();

        if (planet == Planet::Saturn)
            drawSaturnRings(frame, placed, true);
        if (planet == Planet::Earth && std::sin(moonTurn) >= 0.0f)
            drawMoon(frame, placed, cycleTime);
    }

    void OrreryScene::drawSaturnRings(const SceneFrame& frame, const PlacedPlanet& saturn,
        const bool frontHalf)
    {
        const SceneFrame rings = frame.within(Matrix3x2::Translation(saturn.center)
            * Matrix3x2::Rotation(-0.32f));
        const float radius = saturn.radius;
        const auto half = [&](float ringRadius, float width, Color color) {
            PixelPath arc;
            arc.moveTo(frontHalf ? ringRadius : -ringRadius, 0.0f);
            arc.arcTo({ ringRadius, ringRadius * 0.42f }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
                { frontHalf ? -ringRadius : ringRadius, 0.0f });
            strokeShape(rings, arc, SolidColor{ color }, width, StrokeCap::Butt);
        };
        half(radius * 2.15f, radius * 0.42f,
            frontHalf ? OrreryColors::RingOuterFront : OrreryColors::RingOuterBehind);
        half(radius * 1.55f, radius * 0.22f,
            frontHalf ? OrreryColors::RingInnerFront : OrreryColors::RingInnerBehind);
    }

    void OrreryScene::drawMoon(const SceneFrame& frame, const PlacedPlanet& earth,
        const float cycleTime)
    {
        const float turn = k_2Pi * (16.0f * cycleTime + 0.4f);
        const float orbitRadius = earth.radius * 2.4f;
        const FloatPoint offset = { std::cos(turn), std::sin(turn) * k_planeTilt };
        const FloatPoint center = earth.center + offset * orbitRadius;
        const float radius = earth.radius * 0.3f;
        fillDisc(frame, center, radius, SolidColor{ OrreryColors::Moon });
        shadeBall(frame, center, radius, earth.light, OrreryColors::Night.withOpacity(0.8f));
    }

    void OrreryScene::shadeBall(const SceneFrame& frame, const FloatPoint center,
        const float radius, const LightDirection light, const Color shade)
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
