module ClaFi.PathArt.OrreryScene;

import ClaFi.PathArt.AsteroidBeltEffect;
import ClaFi.PathArt.PlanetSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.SunSprite;
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
    }

    namespace
    {
        // How far the orbit plane is tipped toward the viewer: a circle in it is seen this flat.
        constexpr float k_planeTilt{ 0.5f };
        // The pace things keep at a 40-second cycle, in this scene's 240-second one.
        constexpr float k_lively{ 6.0f };
        constexpr float k_beltOrbitsPerCycle{ 3.0f };
        constexpr int k_asteroidCount{ 280 };

        struct Orbit
        {
            Planet planet;
            float radiusShare; // the orbit's radius as a share of the outermost one's
            float orbitsPerCycle;
            float startTurn; // where the planet stands at the start of the cycle, in turns
            float planetRadius; // the planet's radius in scene units
            Color color;
        };

        // A planet where it stands at the moment drawn.
        struct PlacedPlanet
        {
            PlanetPose pose;
            float depth; // nearer the viewer above 0, beyond the Sun below it
            bool outer; // beyond the asteroid belt
        };

        // Where the orbit plane lies in a frame.
        struct OrbitPlane
        {
            FloatPoint center{};
            float outerRadius{}; // the radius of the outermost orbit
        };

        // The orbit plane a little below the middle of the frame, as large as the frame holds.
        [[nodiscard]] OrbitPlane orbitPlaneIn(const SceneFrame&);
    }

    void OrreryScene::prepare(const SceneParams& params)
    {
        m_space.prepare(params);
        m_farOrbits.prepare(params);
        m_nearOrbits.prepare(params);
    }

    void OrreryScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const OrbitPlane plane = orbitPlaneIn(frame);
        const FloatPoint center = plane.center;
        const float outerRadius = plane.outerRadius;
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
            const float angle = k_2Pi * (orbit.startTurn + orbit.orbitsPerCycle * time.cycleTime);
            const float radius = outerRadius * orbit.radiusShare;
            const float across = std::cos(angle) * radius;
            const float toward = std::sin(angle) * radius;
            planets[i] = {
                .pose = {
                    .planet = orbit.planet,
                    .center = { center.x + across, center.y + toward * k_planeTilt },
                    .radius = orbit.planetRadius * unit * (1.0f + 0.12f * std::sin(angle)),
                    .color = orbit.color,
                    .light = {
                        .x = -across / radius,
                        .y = -toward * k_planeTilt / radius,
                        .toViewer = -toward * depthTilt / radius
                    },
                    .orbitTilt = k_planeTilt,
                    .cycleTime = time.cycleTime
                },
                .depth = std::sin(angle),
                .outer = orbit.radiusShare >= 0.5f
            };
        }
        // Drawn far to near: what is beyond the Sun first, then the Sun, then what is in front of
        // it. The belt lies between the inner planets and the outer two.
        std::array<const PlacedPlanet*, 6> byDepth{};
        for (std::size_t i = 0; i != planets.size(); ++i)
            byDepth[i] = &planets[i];
        std::ranges::sort(byDepth, std::ranges::less{}, &PlacedPlanet::depth);
        const AsteroidBelt belt = {
            .center = center,
            .innerRadius = outerRadius * 0.54f,
            .outerRadius = outerRadius * 0.64f,
            .tilt = k_planeTilt,
            .count = k_asteroidCount,
            .orbitsPerCycle = k_beltOrbitsPerCycle,
            .rock = OrreryColors::Rock,
            .paleRock = OrreryColors::PaleRock
        };

        m_space.draw(frame);
        drawStars(frame, time.cycleTime, StarField{
            .count = 170,
            .top = 0.0f,
            .bottom = frame.height,
            .seed = 50.0f,
            .color = OrreryColors::Star,
            .twinklesPerCycle = k_lively
        });
        m_farOrbits.draw(frame);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth < 0.0f && placed->outer)
                drawPlanet(frame, placed->pose);
        }
        drawAsteroidBelt(frame, time.cycleTime, belt, BeltHalf::Far);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth < 0.0f && !placed->outer)
                drawPlanet(frame, placed->pose);
        }
        drawSun(frame, SunPose{
            .center = center,
            .cycleTime = time.cycleTime,
            .flickersPerCycle = 4.0f * k_lively
        });
        m_nearOrbits.draw(frame);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth >= 0.0f && !placed->outer)
                drawPlanet(frame, placed->pose);
        }
        drawAsteroidBelt(frame, time.cycleTime, belt, BeltHalf::Near);
        for (const PlacedPlanet* placed : byDepth)
        {
            if (placed->depth >= 0.0f && placed->outer)
                drawPlanet(frame, placed->pose);
        }
    }

    void OrreryScene::drawSpace(const SceneFrame& frame)
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
    }

    void OrreryScene::drawFarOrbits(const SceneFrame& frame)
    {
        drawOrbits(frame, false);
    }

    void OrreryScene::drawNearOrbits(const SceneFrame& frame)
    {
        drawOrbits(frame, true);
    }

    // The far halves of the orbits go behind the Sun and the near halves over it.
    void OrreryScene::drawOrbits(const SceneFrame& frame, const bool frontHalf)
    {
        const OrbitPlane plane = orbitPlaneIn(frame);
        const FloatPoint center = plane.center;
        const float outerRadius = plane.outerRadius;
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

    namespace
    {
        OrbitPlane orbitPlaneIn(const SceneFrame& frame)
        {
            return {
                .center = { frame.width * 0.5f, frame.height * 0.52f },
                .outerRadius = std::min(frame.width * 0.44f, frame.height * 0.42f / k_planeTilt)
            };
        }
    }
}
