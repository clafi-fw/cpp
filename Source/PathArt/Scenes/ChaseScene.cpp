module ClaFi.PathArt.ChaseScene;

import ClaFi.PathArt.DazeStarsEffect;
import ClaFi.PathArt.DustPuffEffect;
import ClaFi.PathArt.HareSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.WolfSprite;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace ChaseColors
    {
        constexpr Color SkyTop{ 0xFF78C3F4 };
        constexpr Color SkyHorizon{ 0xFFE6F7FF };
        constexpr Color SunGlow{ 0xFFFFF7D1 };
        constexpr Color Sun{ 0xFFFFF6CC };
        constexpr Color Cloud{ 0xE6FFFFFF };
        constexpr Color MountainTop{ 0xFFC4D4EA };
        constexpr Color MountainFoot{ 0xFF93AED0 };
        constexpr Color HillTop{ 0xFF93D36F };
        constexpr Color HillFoot{ 0xFF5FAE4F };
        constexpr Color Trunk{ 0xFF7A5636 };
        constexpr Color Crown{ 0xFF3F8F3E };
        constexpr Color CrownLight{ 0xFF4FA34A };
        constexpr Color CrownMiddle{ 0xFF45973F };
        constexpr Color GrassTop{ 0xFF77C24F };
        constexpr Color GrassFoot{ 0xFF4C9638 };
        constexpr Color Trail{ 0xE6D8B67A };
        constexpr Color Tuft{ 0xFF3F8A33 };
        constexpr Color Pebble{ 0xFFB8935A };
        constexpr Color Petal{ 0xFFFFFFFF };
        constexpr Color FlowerHeart{ 0xFFFFD34D };
        constexpr Color LogLight{ 0xFF9A6A3F };
        constexpr Color LogDark{ 0xFF6B4426 };
        constexpr Color LogFace{ 0xFFD8B07A };
        constexpr Color LogRings{ 0xFFB98A55 };
        constexpr Color Bark{ 0xFF8A5A33 };
        constexpr Color BarkShade{ 0xFF5B3A20 };
        constexpr Color Shadow{ 0x472B4A1E };
        constexpr Color Dust{ 0xFFEADBB8 };
        constexpr Color DazeStar{ 0xFFFFD34D };
        constexpr Color FrontGrass{ 0xFF3A7D2C };
        constexpr Color FrontGrassDark{ 0xFF2F6E25 };
    }

    namespace
    {
        // Units the camera covers in an episode at full speed.
        constexpr float k_fullSpeedTravel{ 6600.0f };
        constexpr float k_episodesPerCycle{ 2.0f };
        // When the camera starts to slow in each episode, and how long it takes to stop: the trip
        // over the log slows it over 1.4 s, the branch stops it within 0.4 s.
        struct CameraStop
        {
            float start;
            float duration;
        };
        constexpr std::array<CameraStop, 2> k_cameraStops{
            CameraStop{ 0.55f, 0.07f },
            CameraStop{ 0.58f, 0.02f }
        };
        // The camera stands from the stop to here, then speeds up again until k_cameraRunning.
        constexpr float k_cameraWaiting{ 0.82f };
        constexpr float k_cameraRunning{ 0.9f };
        // Where the wolf and the hare run, in units from the middle of the frame.
        constexpr float k_wolfChasing{ -200.0f };
        constexpr float k_wolfAtTrip{ -140.0f };
        constexpr float k_hareRunning{ 150.0f };

        // One wave of a ridge: its height, how often it repeats over a tile, where it starts.
        struct Wave
        {
            float amplitude;
            float repeats;
            float phase;
        };

        // A ridge of whole waves over the tile's period, so one tile joins the next.
        [[nodiscard]] float ridgeHeight(const float period, const float x,
            std::span<const Wave> waves)
        {
            float height = 0.0f;
            for (const Wave& wave : waves)
                height += wave.amplitude * std::sin(k_2Pi * wave.repeats * x / period + wave.phase);
            return height;
        }

        constexpr std::array<Wave, 4> k_mountainWaves{
            Wave{ 60.0f, 2.0f, 0.4f },
            Wave{ 34.0f, 3.0f, 1.9f },
            Wave{ 18.0f, 7.0f, 0.7f },
            Wave{ 8.0f, 13.0f, 2.2f }
        };
        constexpr std::array<Wave, 3> k_hillWaves{
            Wave{ 22.0f, 1.0f, 0.3f },
            Wave{ 14.0f, 3.0f, 1.1f },
            Wave{ 6.0f, 5.0f, 2.4f }
        };
    }

    // Two 20-second episodes a cycle: the wolf trips over a log, then runs into a branch. Each
    // time the camera stops while he is down, and the hare turns round to taunt him.
    // See PathArt#chasescene
    void ChaseScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float width = frame.width;
        // A narrow frame draws the two closer together.
        const float closeness = std::min(1.0f, (width / unit) / 760.0f);
        const float travel = travelAt(cycleTime);
        const int episode = static_cast<int>(std::floor(cycleTime * k_episodesPerCycle));
        const float episodeTime = fractionalPart(cycleTime * k_episodesPerCycle);
        const float groundY = frame.height * 0.78f;
        const float footY = groundY + 22.0f * unit;
        const auto toScreen = [&](float units) {
            return width * 0.5f + units * unit * closeness;
        };
        // Units the camera covers from one moment of this episode to another.
        const auto cameraTravel = [&](float from, float to) {
            return (cameraShare(to, episode) - cameraShare(from, episode)) * k_fullSpeedTravel;
        };

        drawBackdrop(frame, travel, groundY);
        drawGround(frame, travel, groundY);

        // The obstacle lies on the ground and reaches the wolf as the camera starts to stop.
        const float trip = k_cameraStops[static_cast<std::size_t>(episode)].start;
        const float obstacleAtTrip = episode == 0 ? k_wolfAtTrip + 30.0f : k_wolfAtTrip + 60.0f;
        const float obstacleX = obstacleAtTrip + cameraTravel(episodeTime, trip);
        const float hareClears = trip - (k_hareRunning - obstacleAtTrip) / k_fullSpeedTravel;

        // The wolf: chasing, falling, lying dazed, getting up and running back into place.
        float wolfX = k_wolfChasing;
        float lift = 0.0f;
        float turn = 0.0f;
        float sprawl = 0.0f;
        float onBack = 0.0f;
        float dazed = 0.0f;
        float running = 1.0f;
        const float fallEnd = 0.62f;
        const float lyingX = episode == 0
            ? -70.0f
            : k_wolfAtTrip - cameraTravel(trip, fallEnd) - 32.0f;
        if (episodeTime < trip)
        {
            wolfX = k_wolfChasing + 60.0f * smoothStep(0.05f, 0.5f, episodeTime);
        }
        else if (episodeTime < fallEnd)
        {
            const float fall = (episodeTime - trip) / (fallEnd - trip);
            if (episode == 0)
            {
                // over the log, head over heels, and flat on his belly
                wolfX = k_wolfAtTrip + 70.0f * smoothStep(0.0f, 1.0f, fall);
                turn = k_2Pi * smoothStep(0.05f, 0.9f, fall);
                lift = 60.0f * std::sin(std::numbers::pi_v<float> * std::min(1.0f, fall * 1.1f));
                sprawl = smoothStep(0.75f, 1.0f, fall);
                running = 1.0f - smoothStep(0.0f, 0.4f, fall);
            }
            else
            {
                // into the branch, stopped where it is, and over onto his back
                wolfX = k_wolfAtTrip - cameraTravel(trip, episodeTime)
                    - 32.0f * smoothStep(0.1f, 1.0f, fall);
                turn = -std::numbers::pi_v<float> * smoothStep(0.25f, 1.0f, fall);
                lift = 34.0f * std::sin(std::numbers::pi_v<float> * smoothStep(0.25f, 1.0f, fall));
                onBack = smoothStep(0.4f, 1.0f, fall);
                running = 1.0f - smoothStep(0.0f, 0.25f, fall);
            }
        }
        else if (episodeTime < k_cameraWaiting)
        {
            wolfX = lyingX;
            turn = episode == 0 ? 0.0f : -std::numbers::pi_v<float>;
            sprawl = episode == 0 ? 1.0f : 0.0f;
            onBack = episode == 0 ? 0.0f : 1.0f;
            dazed = 1.0f;
            running = 0.0f;
        }
        else if (episodeTime < 0.86f)
        {
            const float rise = (episodeTime - k_cameraWaiting) / (0.86f - k_cameraWaiting);
            wolfX = lyingX;
            if (episode == 0)
            {
                sprawl = 1.0f - smoothStep(0.0f, 1.0f, rise);
                lift = 16.0f * std::sin(std::numbers::pi_v<float> * rise);
            }
            else
            {
                turn = -std::numbers::pi_v<float> * (1.0f - smoothStep(0.0f, 1.0f, rise));
                onBack = 1.0f - smoothStep(0.0f, 0.6f, rise);
                lift = 40.0f * std::sin(std::numbers::pi_v<float> * rise);
            }
            dazed = 1.0f - rise;
            running = smoothStep(0.5f, 1.0f, rise);
        }
        else
        {
            wolfX = std::lerp(lyingX, k_wolfChasing, smoothStep(0.86f, 1.0f, episodeTime));
        }
        // The gallop follows the ground the wolf covers, so his feet keep pace with it.
        const float stride = k_2Pi * (travel + wolfX) / periodNear(1.0f, 150.0f);
        const float bob = 4.0f * std::abs(std::sin(stride)) * running;
        const float wolfSize = 0.95f * unit;
        const float standingHeight = 52.0f - 34.0f * sprawl - 30.0f * onBack;
        const FloatPoint wolf = {
            toScreen(wolfX),
            footY - (standingHeight + lift + bob) * wolfSize
        };
        const bool bonked = episode == 1 && episodeTime > trip && episodeTime < trip + 0.012f;
        // On his back the head drops toward the ground rather than into it; the branch knocks it
        // back.
        const float headDrop = 0.7f * onBack + (bonked ? -0.3f : 0.0f);
        const WolfPose wolfPose = {
            .center = wolf,
            .size = wolfSize,
            .turn = turn + 0.07f * std::sin(stride) * running,
            .stride = stride,
            .sprawl = sprawl,
            .onBack = onBack,
            .dazed = dazed,
            .headTurn = std::lerp(0.1f * std::sin(stride), 0.3f, sprawl) + headDrop,
            .tongueTurn = 0.3f + 0.4f * std::sin(stride * 2.0f) * running + 0.6f * dazed,
            .glance = 0.6f * std::sin(k_2Pi * 3.0f * cycleTime),
            .cycleTime = cycleTime
        };

        // The hare: running ahead, clearing the obstacle, sitting up to taunt, and off again.
        float hareX = k_hareRunning;
        if (episodeTime >= 0.5f && episodeTime < 0.84f)
            hareX = k_hareRunning + 100.0f * smoothStep(0.5f, 0.66f, episodeTime);
        else if (episodeTime >= 0.84f)
            hareX = k_hareRunning + 100.0f - 100.0f * smoothStep(0.86f, 1.0f, episodeTime);
        const float sitting = smoothStep(0.62f, 0.66f, episodeTime)
            * (1.0f - smoothStep(0.83f, 0.86f, episodeTime));
        const float turnedRound = smoothStep(0.66f, 0.69f, episodeTime)
            * (1.0f - smoothStep(0.82f, 0.85f, episodeTime));
        const float facing = 1.0f - 2.0f * turnedRound;
        const float hop = k_2Pi * (travel + hareX) / periodNear(1.0f, 180.0f);
        const float leap = episode == 0
            ? 1.0f + 1.7f * bump(episodeTime, hareClears, 0.012f)
            : 1.0f;
        const float duck = episode == 1 ? bump(episodeTime, hareClears, 0.02f) : 0.0f;
        const float air = std::max(0.0f, std::sin(hop)) * (1.0f - sitting);
        const float hareSize = unit;
        const float squash = std::max(0.0f, -std::sin(hop)) * (1.0f - sitting);
        const float hareOnScreen = toScreen(hareX);
        const float hareLift = air * 30.0f * leap * (1.0f - duck) * hareSize;
        const HarePose harePose = {
            .foot = { hareOnScreen, footY - hareLift },
            .size = hareSize,
            .facing = std::abs(facing) < 0.08f ? (facing < 0.0f ? -0.08f : 0.08f) : facing,
            .air = air,
            .stretch = 1.0f + 0.16f * air - 0.08f * squash,
            .bodyTurn = std::lerp(-0.12f * air, -0.85f, sitting),
            .sitting = sitting,
            .earTurn = std::lerp(-1.3f + 0.25f * air - 0.45f * duck, -0.15f, sitting),
            .earWiggle = sitting * 0.22f * std::sin(k_2Pi * 24.0f * episodeTime),
            .footLift = sitting * 7.0f * std::max(0.0f, std::sin(k_2Pi * 36.0f * episodeTime))
        };

        const float obstacleOnScreen = toScreen(obstacleX);
        if (obstacleOnScreen > -200.0f * unit && obstacleOnScreen < width + 200.0f * unit)
            drawObstacle(frame, episode, obstacleOnScreen, groundY, footY);

        const SolidColor shadow{ ChaseColors::Shadow };
        const float wolfShadow = 46.0f * unit * (1.0f - 0.3f * std::min(1.0f, lift / 60.0f));
        fillOval(frame, { wolf.x, footY + 2.0f * unit }, wolfShadow, 5.0f * unit, shadow);
        const float hareShadow = 26.0f * unit * (1.0f - 0.3f * air);
        fillOval(frame, { hareOnScreen, footY + 2.0f * unit }, hareShadow, 4.0f * unit, shadow);
        drawHare(frame, harePose);
        drawWolf(frame, wolfPose);

        // dust where he lands, and stars round his head while he is down
        if (episodeTime > 0.6f && episodeTime < 0.72f)
        {
            drawDustPuff(frame, DustPuff{
                .ground = { wolf.x, footY },
                .settle = (episodeTime - 0.6f) / 0.12f,
                .color = ChaseColors::Dust
            });
        }
        if (dazed > 0.0f)
        {
            const FloatPoint head = wolf
                + rotated({ 54.0f * wolfSize, -24.0f * wolfSize }, wolfPose.turn);
            drawDazeStars(frame, DazeStars{
                .center = { head.x, std::min(head.y, wolf.y) - 26.0f * unit },
                .turns = 12.0f * episodeTime,
                .color = ChaseColors::DazeStar.withOpacity(dazed)
            });
        }
        drawFrontGrass(frame, travel);
    }

    float ChaseScene::cameraShare(const float episodeTime, const int episode)
    {
        const CameraStop stop = k_cameraStops[static_cast<std::size_t>(episode)];
        if (episodeTime <= stop.start)
            return episodeTime;
        if (episodeTime <= stop.start + stop.duration)
        {
            const float slowing = episodeTime - stop.start;
            return stop.start + slowing - slowing * slowing / (2.0f * stop.duration);
        }
        const float standing = stop.start + stop.duration * 0.5f;
        if (episodeTime <= k_cameraWaiting)
            return standing;
        const float gathering = k_cameraRunning - k_cameraWaiting;
        if (episodeTime <= k_cameraRunning)
        {
            const float speeding = episodeTime - k_cameraWaiting;
            return standing + speeding * speeding / (2.0f * gathering);
        }
        return standing + gathering * 0.5f + (episodeTime - k_cameraRunning);
    }

    float ChaseScene::travelAt(const float cycleTime)
    {
        const int episode = static_cast<int>(std::floor(cycleTime * k_episodesPerCycle));
        const float before = episode == 1 ? cameraShare(1.0f, 0) : 0.0f;
        const float episodeTime = fractionalPart(cycleTime * k_episodesPerCycle);
        return (before + cameraShare(episodeTime, episode)) * k_fullSpeedTravel;
    }

    float ChaseScene::periodNear(const float parallax, const float wanted)
    {
        const float cycleTravel = (cameraShare(1.0f, 0) + cameraShare(1.0f, 1)) * k_fullSpeedTravel;
        const float count = std::max(1.0f, std::round(parallax * cycleTravel / wanted));
        return parallax * cycleTravel / count;
    }

    void ChaseScene::forEachTile(const SceneFrame& frame, const float parallax, const float period,
        const float travel, const TileDrawer& drawTile)
    {
        const float offset = std::fmod(std::fmod(parallax * travel, period) + period, period);
        const float tileWidth = period * frame.unit;
        const float end = frame.width + tileWidth;
        const float start = -offset * frame.unit - tileWidth;
        for (float tileLeft = start; tileLeft < end; tileLeft += tileWidth)
            drawTile(tileLeft, tileWidth);
    }

    // Each layer moves at its own share of the camera's speed: far ones slow, near ones fast.
    void ChaseScene::drawBackdrop(const SceneFrame& frame, const float travel, const float groundY)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        fillBox(frame, { 0.0f, 0.0f, width, groundY }, LinearGradient::Simple({ 0.0f, 0.0f },
            { 0.0f, groundY }, ChaseColors::SkyTop, ChaseColors::SkyHorizon));
        const FloatPoint sun = { width * 0.84f, height * 0.14f };
        drawGlow(frame, sun, 130.0f * unit, ChaseColors::SunGlow.withOpacity(0.65f));
        fillDisc(frame, sun, 24.0f * unit, SolidColor{ ChaseColors::Sun });

        const float cloudPeriod = periodNear(0.05f, 820.0f);
        forEachTile(frame, 0.05f, cloudPeriod, travel, [&](float tileLeft, float tileWidth) {
            for (int i = 0; i != 2; ++i)
            {
                const float jitter = 0.15f * pseudoRandom(i, 101.0f);
                const float share = 0.2f + 0.5f * static_cast<float>(i) + jitter;
                const float x = tileLeft + tileWidth * share;
                const float y = height * (0.12f + 0.12f * pseudoRandom(i, 102.0f));
                const float size = (0.7f + 0.5f * pseudoRandom(i, 103.0f)) * unit;
                PixelPath cloud;
                cloud.moveTo(x - 60.0f * size, y);
                cloud.quadTo({ x - 40.0f * size, y - 40.0f * size },
                    { x - 20.0f * size, y - 20.0f * size });
                cloud.quadTo({ x, y - 60.0f * size }, { x + 20.0f * size, y - 20.0f * size });
                cloud.quadTo({ x + 40.0f * size, y - 40.0f * size }, { x + 60.0f * size, y });
                cloud.close();
                fillShape(frame, cloud, SolidColor{ ChaseColors::Cloud });
            }
        });

        // The ridges are one path across the frame each, so no seam shows where two tiles meet.
        const float mountainPeriod = periodNear(0.12f, 1700.0f);
        PixelPath mountains;
        mountains.moveTo(0.0f, groundY);
        for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
        {
            const float ridge = ridgeHeight(mountainPeriod, x / unit + 0.12f * travel,
                k_mountainWaves);
            mountains.lineTo(x, height * 0.6f - ridge * unit - 40.0f * unit);
        }
        mountains.lineTo(width + 6.0f * unit, groundY);
        mountains.close();
        fillShape(frame, mountains, LinearGradient::Simple({ 0.0f, height * 0.35f },
            { 0.0f, groundY }, ChaseColors::MountainTop, ChaseColors::MountainFoot));

        const float hillPeriod = periodNear(0.35f, 1150.0f);
        const auto hillY = [&](float x) {
            return height * 0.70f - ridgeHeight(hillPeriod, x, k_hillWaves) * unit;
        };
        PixelPath hills;
        hills.moveTo(0.0f, groundY + 2.0f * unit);
        for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
            hills.lineTo(x, hillY(x / unit + 0.35f * travel));
        hills.lineTo(width + 6.0f * unit, groundY + 2.0f * unit);
        hills.close();
        fillShape(frame, hills, LinearGradient::Simple({ 0.0f, height * 0.62f }, { 0.0f, groundY },
            ChaseColors::HillTop, ChaseColors::HillFoot));
        forEachTile(frame, 0.35f, hillPeriod, travel, [&](float tileLeft, float) {
            for (int i = 0; i != 4; ++i)
            {
                const float jitter = 0.06f * pseudoRandom(i, 111.0f);
                const float alongTile = hillPeriod
                    * (0.1f + 0.24f * static_cast<float>(i) + jitter);
                const float x = tileLeft + alongTile * unit;
                const float y = hillY(alongTile) + 6.0f * unit;
                const float size = (0.7f + 0.4f * pseudoRandom(i, 112.0f)) * unit;
                fillBox(frame, { x - 3.0f * size, y - 22.0f * size, x + 3.0f * size, y },
                    SolidColor{ ChaseColors::Trunk });
                fillDisc(frame, { x, y - 32.0f * size }, 16.0f * size,
                    SolidColor{ ChaseColors::Crown });
                fillDisc(frame, { x - 9.0f * size, y - 25.0f * size }, 11.0f * size,
                    SolidColor{ ChaseColors::CrownLight });
                fillDisc(frame, { x + 9.0f * size, y - 26.0f * size }, 12.0f * size,
                    SolidColor{ ChaseColors::CrownMiddle });
            }
        });
    }

    void ChaseScene::drawGround(const SceneFrame& frame, const float travel, const float groundY)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const FloatRect grass = { 0.0f, groundY, width, frame.height };
        fillBox(frame, grass, LinearGradient::Simple({ 0.0f, groundY }, { 0.0f, frame.height },
            ChaseColors::GrassTop, ChaseColors::GrassFoot));
        fillBox(frame, { 0.0f, groundY + 10.0f * unit, width, groundY + 36.0f * unit },
            SolidColor{ ChaseColors::Trail });
        PixelPath tufts;
        PixelPath pebbles;
        // The petals of a flower stand just apart, so one path holds them all under an even-odd
        // fill.
        PixelPath petals;
        PixelPath hearts;
        const float tilePeriod = periodNear(1.0f, 400.0f);
        forEachTile(frame, 1.0f, tilePeriod, travel, [&](float tileLeft, float tileWidth) {
            for (int i = 0; i != 6; ++i)
            {
                const float x = tileLeft + tileWidth * pseudoRandom(i, 121.0f);
                const float row = pseudoRandom(i, 122.0f) < 0.5f ? 4.0f : 44.0f;
                const float y = groundY + row * unit + pseudoRandom(i, 123.0f) * 6.0f * unit;
                for (int blade = -1; blade <= 1; ++blade)
                {
                    const float bladeX = static_cast<float>(blade) * 4.0f;
                    const float bladeHeight = 8.0f + 3.0f * static_cast<float>(1 - std::abs(blade));
                    tufts.moveTo(x, y);
                    tufts.lineTo(x + bladeX * unit, y - bladeHeight * unit);
                }
            }
            for (int i = 0; i != 4; ++i)
            {
                const FloatPoint pebble = {
                    tileLeft + tileWidth * pseudoRandom(i, 124.0f),
                    groundY + (16.0f + 14.0f * pseudoRandom(i, 125.0f)) * unit
                };
                const float pebbleWidth = (2.5f + 2.0f * pseudoRandom(i, 126.0f)) * unit;
                addEllipse(pebbles, pebble, pebbleWidth, 1.8f * unit);
            }
            const float petalRadius = 1.75f * unit;
            for (int i = 0; i != 3; ++i)
            {
                const FloatPoint flower = {
                    tileLeft + tileWidth * pseudoRandom(i, 127.0f),
                    groundY + (48.0f + 30.0f * pseudoRandom(i, 128.0f)) * unit
                };
                for (int petal = 0; petal != 5; ++petal)
                {
                    const float angle = k_2Pi * static_cast<float>(petal) / 5.0f;
                    const FloatPoint out = { std::cos(angle), std::sin(angle) };
                    addEllipse(petals, flower + out * (3.0f * unit), petalRadius, petalRadius);
                }
                addEllipse(hearts, flower, 1.8f * unit, 1.8f * unit);
            }
        });
        strokeShape(frame, tufts, SolidColor{ ChaseColors::Tuft }, 2.0f * unit);
        fillShape(frame, pebbles, SolidColor{ ChaseColors::Pebble });
        fillShape(frame, petals, SolidColor{ ChaseColors::Petal });
        fillShape(frame, hearts, SolidColor{ ChaseColors::FlowerHeart });
    }

    void ChaseScene::drawObstacle(const SceneFrame& frame, const int episode, const float x,
        const float groundY, const float footY)
    {
        const float unit = frame.unit;
        if (episode == 0)
        {
            const float logTop = footY - 18.0f * unit;
            const FloatPoint logEnd = { x + 22.0f * unit, footY - 9.0f * unit };
            const FloatRect log = { x - 26.0f * unit, logTop, x + 26.0f * unit, footY };
            fillRoundedBox(frame, log, 9.0f * unit, LinearGradient::Simple({ 0.0f, logTop },
                { 0.0f, footY }, ChaseColors::LogLight, ChaseColors::LogDark));
            fillOval(frame, logEnd, 5.0f * unit, 9.0f * unit, SolidColor{ ChaseColors::LogFace });
            fillOval(frame, logEnd, 2.5f * unit, 4.5f * unit, SolidColor{ ChaseColors::LogRings });
            return;
        }
        // A tree whose lowest branch reaches back over the trail at the height of the wolf's head.
        const float trunkX = x + 95.0f * unit;
        const float trunkLeft = trunkX - 14.0f * unit;
        const float trunkRight = trunkX + 14.0f * unit;
        fillBox(frame, { trunkLeft, groundY - 180.0f * unit, trunkRight, footY + 2.0f * unit },
            LinearGradient::Simple({ trunkLeft, 0.0f }, { trunkRight, 0.0f }, ChaseColors::Bark,
            ChaseColors::BarkShade));
        PixelPath branch;
        branch.moveTo(trunkX - 10.0f * unit, footY - 96.0f * unit);
        branch.quadTo({ x + 40.0f * unit, footY - 92.0f * unit },
            { x - 6.0f * unit, footY - 74.0f * unit });
        branch.lineTo(x - 4.0f * unit, footY - 68.0f * unit);
        branch.quadTo({ x + 40.0f * unit, footY - 84.0f * unit },
            { trunkX - 10.0f * unit, footY - 86.0f * unit });
        branch.close();
        fillShape(frame, branch, SolidColor{ ChaseColors::LogDark });
        fillDisc(frame, { x + 12.0f * unit, footY - 86.0f * unit }, 9.0f * unit,
            SolidColor{ ChaseColors::CrownLight });
        fillDisc(frame, { trunkX, groundY - 196.0f * unit }, 60.0f * unit,
            SolidColor{ ChaseColors::Crown });
        fillDisc(frame, { trunkX - 40.0f * unit, groundY - 170.0f * unit }, 40.0f * unit,
            SolidColor{ ChaseColors::CrownLight });
        fillDisc(frame, { trunkX + 40.0f * unit, groundY - 172.0f * unit }, 44.0f * unit,
            SolidColor{ ChaseColors::CrownMiddle });
    }

    void ChaseScene::drawFrontGrass(const SceneFrame& frame, const float travel)
    {
        const float unit = frame.unit;
        const float height = frame.height;
        PixelPath light;
        PixelPath dark;
        const float grassPeriod = periodNear(1.35f, 520.0f);
        forEachTile(frame, 1.35f, grassPeriod, travel, [&](float tileLeft, float tileWidth) {
            for (int i = 0; i != 22; ++i)
            {
                const float slot = static_cast<float>(i) + pseudoRandom(i, 141.0f);
                const float x = tileLeft + tileWidth * slot / 22.0f;
                const float bladeHeight = (24.0f + 34.0f * pseudoRandom(i, 142.0f)) * unit;
                PixelPath& blade = i % 3 == 0 ? dark : light;
                blade.moveTo(x - 3.0f * unit, height + 2.0f * unit);
                blade.quadTo({ x + 2.0f * unit, height - bladeHeight * 0.6f },
                    { x + (6.0f - 12.0f * pseudoRandom(i, 143.0f)) * unit, height - bladeHeight });
                blade.quadTo({ x + 3.0f * unit, height - bladeHeight * 0.5f },
                    { x + 4.0f * unit, height + 2.0f * unit });
                blade.close();
            }
        });
        // One path per shade: the blades stand too far apart to overlap, which an even-odd fill
        // would show as holes.
        fillShape(frame, dark, SolidColor{ ChaseColors::FrontGrassDark });
        fillShape(frame, light, SolidColor{ ChaseColors::FrontGrass });
    }
}
