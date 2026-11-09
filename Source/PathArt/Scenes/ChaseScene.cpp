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
        // The hills and their trees are greens taken 0.55 of the way to MountainFoot, so they
        // stand back behind the trail.
        constexpr Color HillTop{ 0xFF93BEA4 };
        constexpr Color HillFoot{ 0xFF7BAE95 };
        constexpr Color HillTreeTrunk{ 0xFF87868A };
        constexpr Color HillTreeCrown{ 0xFF6C9F8D };
        constexpr Color HillTreeCrownLight{ 0xFF74A993 };
        constexpr Color HillTreeCrownMiddle{ 0xFF6FA38E };
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
        constexpr Color AppleLight{ 0xFFE84B3C };
        constexpr Color AppleDark{ 0xFFA3241F };
        constexpr Color AppleShine{ 0x99FFFFFF };
        constexpr Color Leaf{ 0xFF4FA34A };
        constexpr Color BootLight{ 0xFF8A6440 };
        constexpr Color BootDark{ 0xFF5E3F25 };
        constexpr Color BootSole{ 0xFF2F2621 };
        constexpr Color BootPatch{ 0xFFA07A4F };
        constexpr Color Lace{ 0xFFE8D9B5 };
        constexpr Color SpinesDark{ 0xFF5C4630 };
        constexpr Color SpinesLight{ 0xFF7D6146 };
        constexpr Color HedgehogFace{ 0xFF9A7655 };
        constexpr Color HedgehogEar{ 0xFF7A5A40 };
        constexpr Color EyeShine{ 0xFFFFFFFF };
        constexpr Color Ink{ 0xFF1B1D22 };
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

        // The second episode's moments, as shares of it: the wolf closes in on the hare, drops
        // back, gathers himself and leaps at her, into the branch at k_cameraStops[1].start.
        namespace BranchChase
        {
            constexpr float SurgeStart{ 0.14f };
            constexpr float Closest{ 0.29f };
            constexpr float DropBackStart{ 0.33f };
            constexpr float DroppedBack{ 0.41f };
            constexpr float EffortStart{ 0.43f };
            constexpr float CrouchStart{ 0.535f };
            constexpr float Takeoff{ 0.555f };
            constexpr float LeapTop{ 0.585f };
        }
        // The wolf's middle behind the hare's feet at his closest, his nose just short of her tail.
        constexpr float k_wolfClosestBehindHare{ 122.0f };
        constexpr float k_wolfDroppedBack{ -70.0f };
        constexpr float k_wolfAtTakeoff{ -25.0f };
        constexpr float k_wolfAtBranch{ 25.0f };
        constexpr float k_leapHeight{ 64.0f };
        // From the wolf's middle to the end of the branch as it meets his face.
        constexpr float k_branchReach{ 66.0f };

        // How the wolf runs at the hare in the second episode, up to the branch.
        struct BranchRun
        {
            float x; // units from the middle of the frame
            float lift;
            float turn;
            float crouch; // units his body sinks before the leap
            float leap; // 0 running, 1 stretched out in the air
            float jawOpen; // 0 shut, 1 gaping at the hare
            float strain; // 0 to 1 as he presses on
            float weariness; // 0 to 1 as he drops back
            float snap; // 0 to 1 as his head lunges at her tail
        };

        // Units the hare runs ahead of k_hareRunning, bolting from the wolf in the second episode.
        [[nodiscard]] float hareBoltAt(float episodeTime);
        // His gap to the hare at his closest is held in pixels, whatever `closeness`.
        [[nodiscard]] BranchRun branchRunAt(float episodeTime, float closeness);

        // What the wolf comes to grief on: the branch in the second episode, and in the first one
        // of four, a cycle each in turn.
        enum class Obstacle
        {
            Log,
            Apple,
            Boot,
            Hedgehog,
            Branch
        };
        constexpr std::array<Obstacle, 4> k_firstRunObstacles{
            Obstacle::Log,
            Obstacle::Apple,
            Obstacle::Boot,
            Obstacle::Hedgehog
        };

        // How what is on the trail moves at one moment.
        struct ObstacleMotion
        {
            float branchSway; // units the branch's end is pushed down, up below 0
            float curl; // 0 the hedgehog on its feet, 1 rolled into a ball
            float steps; // radians of the hedgehog's walk
        };
        // The hedgehog walks off at 160 units a second, which is this many in an episode's time.
        constexpr float k_hedgehogWalk{ 3200.0f };

        // `x` is the middle of what lies on the trail, or the end of the branch.
        void drawObstacle(const SceneFrame&, Obstacle, float x, float groundY, float footY,
            const ObstacleMotion&);
        // Each is drawn in units about the middle of where it meets the trail, facing the wolf.
        void drawLog(const SceneFrame& onTrail);
        void drawApple(const SceneFrame& onTrail);
        void drawBoot(const SceneFrame& onTrail);
        void drawHedgehog(const SceneFrame& onTrail, float curl, float steps);
        void drawBranchTree(const SceneFrame&, float x, float groundY, float footY,
            float branchSway);

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

    // Two 20-second episodes a cycle: the wolf trips over something on the trail, then leaps into
    // a branch. Each time the camera stops while he is down, and the hare turns round to taunt him.
    // See PathArt#chasescene
    void ChaseScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float width = frame.width;
        // A narrow frame draws the two closer together.
        const float closeness = std::min(1.0f, (width / unit) / 760.0f);
        const float travel = travelAt(time.cycleTime);
        const int episode = static_cast<int>(std::floor(time.cycleTime * k_episodesPerCycle));
        const float episodeTime = fractionalPart(time.cycleTime * k_episodesPerCycle);
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

        // The obstacle reaches the wolf as the camera starts to stop. The branch's reach is held in
        // pixels, so it meets his face in a narrow frame too.
        const float trip = k_cameraStops[static_cast<std::size_t>(episode)].start;
        const float obstacleAtTrip = episode == 0
            ? k_wolfAtTrip + 30.0f
            : k_wolfAtBranch + k_branchReach / closeness;
        const std::int64_t firstRunObstacleCount
            = static_cast<std::int64_t>(k_firstRunObstacles.size());
        const std::int64_t firstRunIndex = time.cyclesPassed % firstRunObstacleCount;
        const Obstacle obstacle = episode == 0
            ? k_firstRunObstacles[static_cast<std::size_t>(firstRunIndex)]
            : Obstacle::Branch;
        const float hareClears = trip - (k_hareRunning - obstacleAtTrip) / k_fullSpeedTravel;
        // The hedgehog rolls up once the hare has passed over it, and walks off once the wolf is
        // down, away from him.
        const bool hedgehog = obstacle == Obstacle::Hedgehog;
        const float curl = hedgehog
            ? smoothStep(hareClears + 0.002f, hareClears + 0.012f, episodeTime)
                * (1.0f - smoothStep(0.598f, 0.61f, episodeTime))
            : 0.0f;
        const float walkedAway = hedgehog
            ? k_hedgehogWalk * std::max(0.0f, episodeTime - 0.612f)
            : 0.0f;
        const float obstacleX = obstacleAtTrip + cameraTravel(episodeTime, trip) - walkedAway;

        // The wolf: chasing, falling, lying dazed, getting up and running back into place.
        float wolfX = k_wolfChasing;
        float lift = 0.0f;
        float turn = 0.0f;
        float crouch = 0.0f;
        float leap = 0.0f;
        float jawOpen = 0.0f;
        float sprawl = 0.0f;
        float onBack = 0.0f;
        float dazed = 0.0f;
        float running = 1.0f;
        float strain = 0.0f;
        float weariness = 0.0f;
        float snap = 0.0f;
        const float fallEnd = 0.62f;
        const float lyingX = episode == 0
            ? -70.0f
            : k_wolfAtBranch - cameraTravel(trip, fallEnd) - 18.0f;
        if (episodeTime < trip)
        {
            if (episode == 0)
            {
                wolfX = k_wolfChasing + 60.0f * smoothStep(0.05f, 0.5f, episodeTime);
            }
            else
            {
                const BranchRun run = branchRunAt(episodeTime, closeness);
                wolfX = run.x;
                lift = run.lift;
                turn = run.turn;
                crouch = run.crouch;
                leap = run.leap;
                jawOpen = run.jawOpen;
                running = 1.0f - run.leap;
                strain = run.strain;
                weariness = run.weariness;
                snap = run.snap;
            }
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
                // stopped dead in the air by the branch, a moment hung on it, then down onto his
                // back
                const BranchRun atBranch = branchRunAt(trip, closeness);
                const float dropping = std::clamp((fall - 0.2f) / 0.8f, 0.0f, 1.0f);
                wolfX = k_wolfAtBranch - cameraTravel(trip, episodeTime)
                    - 18.0f * smoothStep(0.0f, 0.5f, fall);
                lift = atBranch.lift * (1.0f - dropping * dropping);
                turn = std::lerp(atBranch.turn, -std::numbers::pi_v<float>,
                    smoothStep(0.2f, 0.95f, fall));
                leap = 1.0f - smoothStep(0.2f, 0.6f, fall);
                // the branch shuts his jaws for him
                jawOpen = 1.0f - smoothStep(0.0f, 0.1f, fall);
                onBack = smoothStep(0.45f, 1.0f, fall);
                dazed = 1.0f;
                running = 0.0f;
            }
        }
        else if (episodeTime < k_cameraWaiting)
        {
            wolfX = lyingX;
            // He comes down from the branch hard enough to bounce once.
            if (episode == 1)
            {
                const float bounce = std::min(1.0f, (episodeTime - fallEnd) / 0.016f);
                lift = 7.0f * std::sin(std::numbers::pi_v<float> * bounce);
            }
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
        const float bob = (4.0f + 3.0f * weariness) * std::abs(std::sin(stride)) * running;
        const float wolfSize = 0.95f * unit;
        const float standingHeight = 52.0f - 34.0f * sprawl - 30.0f * onBack - crouch;
        const FloatPoint wolf = {
            toScreen(wolfX + 8.0f * snap),
            footY - (standingHeight + lift + bob) * wolfSize
        };
        const bool bonked = episode == 1 && episodeTime > trip && episodeTime < trip + 0.012f;
        // On his back the head drops toward the ground rather than into it; the branch knocks it
        // back.
        const float headDrop = 0.7f * onBack + (bonked ? -0.3f : 0.0f);
        // Pressing on, he lowers his head; worn out, it hangs; in the air it reaches for the hare.
        const float headReach = 0.1f * strain + 0.15f * weariness + 0.3f * snap - 0.12f * leap;
        const float tongueSwing = (0.4f + 0.4f * weariness) * std::sin(stride * 2.0f) * running;
        const float eyesOnHare = std::max(strain, leap);
        const WolfPose wolfPose = {
            .center = wolf,
            .size = wolfSize,
            .turn = turn + 0.07f * std::sin(stride) * running,
            .stride = stride,
            .leap = leap,
            .sprawl = sprawl,
            .onBack = onBack,
            .dazed = dazed,
            .headTurn = std::lerp(0.1f * std::sin(stride), 0.3f, sprawl) + headDrop + headReach,
            .tongueTurn = 0.3f + tongueSwing + 0.6f * dazed + 0.5f * std::max(strain, leap),
            .jawOpen = jawOpen,
            .glance = 0.6f * std::sin(k_2Pi * 3.0f * time.cycleTime) * (1.0f - eyesOnHare),
            .cycleTime = time.cycleTime
        };

        // The hare: running ahead, clearing the obstacle, sitting up to taunt, and off again. In
        // the second episode she bolts as the wolf comes close and eases off as he drops back.
        float bolt = 0.0f;
        float alarm = 0.0f;
        if (episode == 1)
        {
            bolt = hareBoltAt(episodeTime);
            alarm = std::max(smoothStep(0.23f, 0.27f, episodeTime)
                * (1.0f - smoothStep(0.36f, 0.44f, episodeTime)),
                bump(episodeTime, BranchChase::Takeoff + 0.012f, 0.012f));
        }
        const float runningAhead = smoothStep(episode == 0 ? 0.5f : 0.57f, 0.66f, episodeTime);
        const float hareX = k_hareRunning + (bolt + 100.0f * runningAhead)
            * (1.0f - smoothStep(0.86f, 1.0f, episodeTime));
        const float sitting = smoothStep(0.62f, 0.66f, episodeTime)
            * (1.0f - smoothStep(0.83f, 0.86f, episodeTime));
        const float turnedRound = smoothStep(0.66f, 0.69f, episodeTime)
            * (1.0f - smoothStep(0.82f, 0.85f, episodeTime));
        const float facing = 1.0f - 2.0f * turnedRound;
        const float hop = k_2Pi * (travel + hareX) / periodNear(1.0f, 180.0f);
        const float hopScale = episode == 0
            ? 1.0f + 1.7f * bump(episodeTime, hareClears, 0.012f)
            : 1.0f + 0.45f * alarm;
        const float air = std::max(0.0f, std::sin(hop)) * (1.0f - sitting);
        const float hareSize = unit;
        const float squash = std::max(0.0f, -std::sin(hop)) * (1.0f - sitting);
        const float hareOnScreen = toScreen(hareX);
        const float hareLift = air * 30.0f * hopScale * hareSize;
        const HarePose harePose = {
            .foot = { hareOnScreen, footY - hareLift },
            .size = hareSize,
            .facing = std::abs(facing) < 0.08f ? (facing < 0.0f ? -0.08f : 0.08f) : facing,
            .air = air,
            .stretch = 1.0f + (0.16f + 0.08f * alarm) * air - 0.08f * squash,
            .bodyTurn = std::lerp(-0.12f * air, -0.85f, sitting),
            .sitting = sitting,
            .earTurn = std::lerp(-1.3f + 0.25f * air - 0.35f * alarm, -0.15f, sitting),
            .earWiggle = sitting * 0.22f * std::sin(k_2Pi * 24.0f * episodeTime),
            .footLift = sitting * 7.0f * std::max(0.0f, std::sin(k_2Pi * 36.0f * episodeTime))
        };

        // The branch springs up where his head hit it and shakes itself still.
        const float sinceBonk = episodeTime - trip;
        const float branchSway = episode == 1 && sinceBonk > 0.0f
            ? -10.0f * std::exp(-sinceBonk / 0.025f) * std::sin(k_2Pi * sinceBonk / 0.0125f)
            : 0.0f;
        const float obstacleOnScreen = toScreen(obstacleX);
        if (obstacleOnScreen > -220.0f * unit && obstacleOnScreen < width + 200.0f * unit)
        {
            drawObstacle(frame, obstacle, obstacleOnScreen, groundY, footY, ObstacleMotion{
                .branchSway = branchSway,
                .curl = curl,
                .steps = k_2Pi * walkedAway / 9.0f
            });
        }

        const SolidColor shadow{ ChaseColors::Shadow };
        const float wolfShadow = 46.0f * unit * (1.0f - 0.3f * std::min(1.0f, lift / 60.0f));
        fillOval(frame, { wolf.x, footY + 2.0f * unit }, wolfShadow, 5.0f * unit, shadow);
        const float hareShadow = 26.0f * unit * (1.0f - 0.3f * air);
        fillOval(frame, { hareOnScreen, footY + 2.0f * unit }, hareShadow, 4.0f * unit, shadow);
        // dust kicked up behind him as he leaps
        const float sinceTakeoff = episodeTime - BranchChase::Takeoff;
        if (episode == 1 && sinceTakeoff > 0.0f && sinceTakeoff < 0.04f)
        {
            const float takeoffX = k_wolfAtTakeoff - 20.0f
                - cameraTravel(BranchChase::Takeoff, episodeTime);
            drawDustPuff(frame, DustPuff{
                .ground = { toScreen(takeoffX), footY },
                .settle = sinceTakeoff / 0.04f,
                .color = ChaseColors::Dust
            });
        }
        drawHare(frame, harePose);
        drawWolf(frame, wolfPose);

        // dust where he lands, and stars round his head while he is down
        const float landing = episode == 0 ? 0.6f : fallEnd - 0.003f;
        if (episodeTime > landing && episodeTime < landing + 0.12f)
        {
            drawDustPuff(frame, DustPuff{
                .ground = { wolf.x, footY },
                .settle = (episodeTime - landing) / 0.12f,
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
                    SolidColor{ ChaseColors::HillTreeTrunk });
                fillDisc(frame, { x, y - 32.0f * size }, 16.0f * size,
                    SolidColor{ ChaseColors::HillTreeCrown });
                fillDisc(frame, { x - 9.0f * size, y - 25.0f * size }, 11.0f * size,
                    SolidColor{ ChaseColors::HillTreeCrownLight });
                fillDisc(frame, { x + 9.0f * size, y - 26.0f * size }, 12.0f * size,
                    SolidColor{ ChaseColors::HillTreeCrownMiddle });
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
        // Tufts, pebbles and flowers each go down over every tile before the next kind does, so a
        // flower stands over a tuft whichever tile either is in.
        const float tilePeriod = periodNear(1.0f, 400.0f);
        ScatteredShapes tufts{ frame, SolidColor{ ChaseColors::Tuft }, 2.0f * unit };
        forEachTile(frame, 1.0f, tilePeriod, travel, [&](float tileLeft, float tileWidth) {
            for (int i = 0; i != 6; ++i)
            {
                const float x = tileLeft + tileWidth * pseudoRandom(i, 121.0f);
                const float row = pseudoRandom(i, 122.0f) < 0.5f ? 4.0f : 44.0f;
                const float y = groundY + row * unit + pseudoRandom(i, 123.0f) * 6.0f * unit;
                PixelPath& tuft = tufts.next();
                for (int blade = -1; blade <= 1; ++blade)
                {
                    const float bladeX = static_cast<float>(blade) * 4.0f;
                    const float bladeHeight = 8.0f + 3.0f * static_cast<float>(1 - std::abs(blade));
                    tuft.moveTo(x, y);
                    tuft.lineTo(x + bladeX * unit, y - bladeHeight * unit);
                }
            }
        });
        tufts.draw();
        ScatteredShapes pebbles{ frame, SolidColor{ ChaseColors::Pebble } };
        forEachTile(frame, 1.0f, tilePeriod, travel, [&](float tileLeft, float tileWidth) {
            for (int i = 0; i != 4; ++i)
            {
                const FloatPoint pebble = {
                    tileLeft + tileWidth * pseudoRandom(i, 124.0f),
                    groundY + (16.0f + 14.0f * pseudoRandom(i, 125.0f)) * unit
                };
                const float pebbleWidth = (2.5f + 2.0f * pseudoRandom(i, 126.0f)) * unit;
                addEllipse(pebbles.next(), pebble, pebbleWidth, 1.8f * unit);
            }
        });
        pebbles.draw();
        // The petals of a flower stand just apart, so one shape holds them all under an even-odd
        // fill.
        ScatteredShapes petals{ frame, SolidColor{ ChaseColors::Petal } };
        ScatteredShapes hearts{ frame, SolidColor{ ChaseColors::FlowerHeart } };
        forEachTile(frame, 1.0f, tilePeriod, travel, [&](float tileLeft, float tileWidth) {
            const float petalRadius = 1.75f * unit;
            for (int i = 0; i != 3; ++i)
            {
                const FloatPoint flower = {
                    tileLeft + tileWidth * pseudoRandom(i, 127.0f),
                    groundY + (48.0f + 30.0f * pseudoRandom(i, 128.0f)) * unit
                };
                PixelPath& flowerPetals = petals.next();
                for (int petal = 0; petal != 5; ++petal)
                {
                    const float angle = k_2Pi * static_cast<float>(petal) / 5.0f;
                    const FloatPoint out = { std::cos(angle), std::sin(angle) };
                    addEllipse(flowerPetals, flower + out * (3.0f * unit), petalRadius,
                        petalRadius);
                }
                addEllipse(hearts.next(), flower, 1.8f * unit, 1.8f * unit);
            }
        });
        petals.draw();
        hearts.draw();
    }

    void ChaseScene::drawFrontGrass(const SceneFrame& frame, const float travel)
    {
        const float unit = frame.unit;
        const float height = frame.height;
        const float grassPeriod = periodNear(1.35f, 520.0f);
        // Every third blade is dark, and the dark ones go down first.
        for (const bool dark : { true, false })
        {
            const Color shade = dark ? ChaseColors::FrontGrassDark : ChaseColors::FrontGrass;
            ScatteredShapes blades{ frame, SolidColor{ shade } };
            forEachTile(frame, 1.35f, grassPeriod, travel, [&](float tileLeft, float tileWidth) {
                for (int i = 0; i != 22; ++i)
                {
                    if ((i % 3 == 0) != dark)
                        continue;
                    const float slot = static_cast<float>(i) + pseudoRandom(i, 141.0f);
                    const float x = tileLeft + tileWidth * slot / 22.0f;
                    const float bladeHeight = (24.0f + 34.0f * pseudoRandom(i, 142.0f)) * unit;
                    PixelPath& blade = blades.next();
                    blade.moveTo(x - 3.0f * unit, height + 2.0f * unit);
                    blade.quadTo({ x + 2.0f * unit, height - bladeHeight * 0.6f },
                        { x + (6.0f - 12.0f * pseudoRandom(i, 143.0f)) * unit,
                            height - bladeHeight });
                    blade.quadTo({ x + 3.0f * unit, height - bladeHeight * 0.5f },
                        { x + 4.0f * unit, height + 2.0f * unit });
                    blade.close();
                }
            });
            blades.draw();
        }
    }

    namespace
    {
        float hareBoltAt(const float episodeTime)
        {
            return 50.0f * smoothStep(0.24f, 0.31f, episodeTime)
                - 30.0f * smoothStep(0.34f, 0.46f, episodeTime);
        }

        // Each stretch of the run is eased in and out; the leap is a throw whose top would come
        // just past the branch.
        BranchRun branchRunAt(const float episodeTime, const float closeness)
        {
            const float contact = k_cameraStops[1].start;
            const float closestX = k_hareRunning + hareBoltAt(BranchChase::Closest)
                - k_wolfClosestBehindHare / closeness;
            const float takeoff = BranchChase::Takeoff;
            const float closingIn = smoothStep(BranchChase::SurgeStart, BranchChase::Closest,
                episodeTime);
            const float droppingBack = smoothStep(BranchChase::DropBackStart,
                BranchChase::DroppedBack, episodeTime);
            const float comingOn = smoothStep(BranchChase::EffortStart, takeoff, episodeTime);
            const float flight = std::max(0.0f, (episodeTime - takeoff) / (contact - takeoff));
            const float fromTop = (episodeTime - BranchChase::LeapTop)
                / (BranchChase::LeapTop - takeoff);
            const float inAir = smoothStep(takeoff, takeoff + 0.006f, episodeTime);
            const float crouching = smoothStep(BranchChase::CrouchStart, takeoff, episodeTime)
                * (1.0f - smoothStep(takeoff, takeoff + 0.005f, episodeTime));
            const float firstStrain = smoothStep(BranchChase::SurgeStart,
                BranchChase::SurgeStart + 0.04f, episodeTime)
                * (1.0f - smoothStep(BranchChase::DropBackStart,
                BranchChase::DropBackStart + 0.03f, episodeTime));
            const float lastStrain = smoothStep(BranchChase::EffortStart,
                BranchChase::EffortStart + 0.03f, episodeTime) * (1.0f - inAir);
            const float weariness = smoothStep(BranchChase::DropBackStart,
                BranchChase::DropBackStart + 0.03f, episodeTime)
                * (1.0f - smoothStep(BranchChase::EffortStart - 0.01f,
                BranchChase::EffortStart + 0.03f, episodeTime));
            const float strain = firstStrain + lastStrain;
            // At his closest the jaws open on her tail and shut at the end of the lunge.
            const float snapAt = BranchChase::Closest + 0.008f;
            const float chomp = smoothStep(snapAt - 0.012f, snapAt - 0.004f, episodeTime)
                * (1.0f - smoothStep(snapAt - 0.001f, snapAt + 0.0015f, episodeTime));
            return {
                .x = k_wolfChasing
                    + (closestX - k_wolfChasing) * closingIn
                    - (closestX - k_wolfDroppedBack) * droppingBack
                    + (k_wolfAtTakeoff - k_wolfDroppedBack) * comingOn
                    + (k_wolfAtBranch - k_wolfAtTakeoff) * flight,
                .lift = episodeTime > takeoff ? k_leapHeight * (1.0f - fromTop * fromTop) : 0.0f,
                .turn = 0.05f * strain + 0.1f * crouching - (0.32f - 0.17f * flight) * inAir,
                .crouch = 9.0f * crouching,
                .leap = smoothStep(takeoff, takeoff + 0.01f, episodeTime),
                .jawOpen = chomp + smoothStep(takeoff + 0.003f, takeoff + 0.014f, episodeTime),
                .strain = strain,
                .weariness = weariness,
                .snap = bump(episodeTime, snapAt, 0.005f)
            };
        }

        void drawObstacle(const SceneFrame& frame, const Obstacle obstacle, const float x,
            const float groundY, const float footY, const ObstacleMotion& motion)
        {
            const SceneFrame onTrail = frame.within(Matrix3x2::Translation(x, footY)
                * Matrix3x2::Scale(frame.unit));
            switch (obstacle)
            {
                case Obstacle::Log:
                    drawLog(onTrail);
                    break;
                case Obstacle::Apple:
                    drawApple(onTrail);
                    break;
                case Obstacle::Boot:
                    drawBoot(onTrail);
                    break;
                case Obstacle::Hedgehog:
                    drawHedgehog(onTrail, motion.curl, motion.steps);
                    break;
                case Obstacle::Branch:
                    drawBranchTree(frame, x, groundY, footY, motion.branchSway);
                    break;
            }
        }

        void drawLog(const SceneFrame& onTrail)
        {
            fillRoundedBox(onTrail, { -26.0f, -18.0f, 26.0f, 0.0f }, 9.0f,
                LinearGradient::Simple({ 0.0f, -18.0f }, {}, ChaseColors::LogLight,
                ChaseColors::LogDark));
            fillOval(onTrail, { 22.0f, -9.0f }, 5.0f, 9.0f, SolidColor{ ChaseColors::LogFace });
            fillOval(onTrail, { 22.0f, -9.0f }, 2.5f, 4.5f, SolidColor{ ChaseColors::LogRings });
        }

        void drawApple(const SceneFrame& onTrail)
        {
            PixelPath apple;
            apple.moveTo(0.0f, -17.0f);
            apple.cubicTo({ 6.0f, -22.0f }, { 14.0f, -18.0f }, { 12.0f, -8.0f });
            apple.cubicTo({ 11.0f, -1.0f }, { 4.0f, 1.0f }, { 0.0f, -1.0f });
            apple.cubicTo({ -4.0f, 1.0f }, { -11.0f, -1.0f }, { -12.0f, -8.0f });
            apple.cubicTo({ -14.0f, -18.0f }, { -6.0f, -22.0f }, { 0.0f, -17.0f });
            apple.close();
            fillShape(onTrail, apple, LinearGradient::Simple({ -8.0f, -18.0f }, { 8.0f, 0.0f },
                ChaseColors::AppleLight, ChaseColors::AppleDark));
            fillOval(onTrail, { -6.0f, -13.0f }, 2.5f, 4.0f, SolidColor{ ChaseColors::AppleShine },
                0.4f);
            drawSegment(onTrail, { 0.0f, -17.0f }, { 1.5f, -24.0f },
                SolidColor{ ChaseColors::BarkShade }, 1.8f);
            fillOval(onTrail, { 5.5f, -22.0f }, 5.0f, 2.2f, SolidColor{ ChaseColors::Leaf }, -0.5f);
        }

        // An old boot, its toe toward the wolf.
        void drawBoot(const SceneFrame& onTrail)
        {
            PixelPath leather;
            leather.moveTo(15.0f, -3.0f);
            leather.lineTo(15.0f, -32.0f);
            leather.lineTo(0.0f, -32.0f);
            leather.lineTo(0.0f, -19.0f);
            leather.quadTo({ -6.0f, -17.0f }, { -14.0f, -14.0f });
            leather.quadTo({ -21.0f, -12.0f }, { -21.0f, -6.0f });
            leather.lineTo(-21.0f, -3.0f);
            leather.close();
            fillShape(onTrail, leather, LinearGradient::Simple({ 0.0f, -32.0f }, { 0.0f, -3.0f },
                ChaseColors::BootLight, ChaseColors::BootDark));
            fillBox(onTrail, { -0.5f, -33.0f, 15.5f, -29.5f }, SolidColor{ ChaseColors::BootDark });
            fillBox(onTrail, { 7.0f, -16.0f, 12.0f, -11.0f }, SolidColor{ ChaseColors::BootPatch });
            fillRoundedBox(onTrail, { -22.0f, -4.0f, 16.0f, 0.0f }, 1.5f,
                SolidColor{ ChaseColors::BootSole });
            PixelPath laces;
            for (const float y : { -27.0f, -23.0f, -19.0f })
            {
                laces.moveTo(1.0f, y - 1.5f);
                laces.lineTo(6.0f, y + 1.5f);
                laces.moveTo(1.0f, y + 1.5f);
                laces.lineTo(6.0f, y - 1.5f);
            }
            strokeShape(onTrail, laces, SolidColor{ ChaseColors::Lace }, 1.2f);
        }

        // A hedgehog crouched over its feet, its snout toward the wolf. Curling, its spines run on
        // round under it and its face shrinks into them; walking, its feet patter and it bobs.
        void drawHedgehog(const SceneFrame& onTrail, const float curl, const float steps)
        {
            constexpr int k_spineCount{ 18 };
            const SceneFrame hedgehog = onTrail.within(Matrix3x2::Translation(0.0f,
                -0.8f * std::abs(std::sin(steps))));
            const FloatPoint middle = pointBetween({ 5.0f, -1.0f }, { 2.0f, -16.0f }, curl);
            const float radiusX = std::lerp(22.0f, 16.0f, curl);
            const float radiusY = std::lerp(25.0f, 16.0f, curl);
            // The outline runs from the left over the top to the right, and on round under the
            // body as it curls, a whole turn when it is a ball.
            const float firstAngle = std::numbers::pi_v<float> * (1.0f + 0.5f * curl);
            const float span = std::numbers::pi_v<float> * (1.0f + curl);
            const auto onBody = [&](float share, float reach) {
                const float angle = firstAngle - span * share;
                return middle
                    + FloatPoint{ radiusX * std::cos(angle), -radiusY * std::sin(angle) } * reach;
            };
            PixelPath spines;
            PixelPath back;
            for (int i = 0; i <= 2 * k_spineCount; ++i)
            {
                const float share = static_cast<float>(i) / static_cast<float>(2 * k_spineCount);
                const FloatPoint spineTip = onBody(share, i % 2 == 1 ? 1.0f : 0.78f);
                const FloatPoint backEdge = onBody(share, 0.7f);
                if (i == 0)
                {
                    spines.moveTo(spineTip);
                    back.moveTo(backEdge);
                }
                else
                {
                    spines.lineTo(spineTip);
                    back.lineTo(backEdge);
                }
            }
            spines.close();
            back.close();
            fillShape(hedgehog, spines, SolidColor{ ChaseColors::SpinesDark });
            fillShape(hedgehog, back, SolidColor{ ChaseColors::SpinesLight });
            PixelPath quills;
            for (int i = 0; i != 7; ++i)
            {
                const float share = 0.15f + 0.7f * static_cast<float>(i) / 6.0f;
                quills.moveTo(onBody(share, 0.42f));
                quills.lineTo(onBody(share, 0.76f));
            }
            strokeShape(hedgehog, quills, SolidColor{ ChaseColors::SpinesDark }, 1.4f);
            if (curl >= 1.0f)
                return;

            const float footWidth = 4.0f * (1.0f - curl);
            const float patter = 2.5f * std::sin(steps);
            fillOval(onTrail, { -10.0f + patter, -0.7f }, footWidth, 2.0f,
                SolidColor{ ChaseColors::Ink });
            fillOval(onTrail, { 13.0f - patter, -0.7f }, footWidth, 2.0f,
                SolidColor{ ChaseColors::Ink });
            const FloatPoint neck = { -6.0f, -8.0f };
            const SceneFrame head = hedgehog.within(Matrix3x2::Translation(neck)
                * Matrix3x2::Scale(1.0f - curl) * Matrix3x2::Translation(-neck.x, -neck.y));
            fillOval(head, { -14.0f, -9.0f }, 10.5f, 8.5f, SolidColor{ ChaseColors::HedgehogFace });
            PixelPath snout;
            snout.moveTo(-18.0f, -14.0f);
            snout.quadTo({ -29.0f, -10.0f }, { -34.0f, -6.5f });
            snout.quadTo({ -29.0f, -2.5f }, { -17.0f, -2.5f });
            snout.close();
            fillShape(head, snout, SolidColor{ ChaseColors::HedgehogFace });
            fillDisc(head, { -12.0f, -17.0f }, 2.6f, SolidColor{ ChaseColors::HedgehogEar });
            fillDisc(head, { -34.0f, -6.5f }, 2.3f, SolidColor{ ChaseColors::Ink });
            fillDisc(head, { -18.0f, -12.0f }, 2.0f, SolidColor{ ChaseColors::Ink });
            fillDisc(head, { -17.4f, -12.6f }, 0.6f, SolidColor{ ChaseColors::EyeShine });
        }

        // A tree whose lowest branch reaches back over the trail, as high as the head of a wolf in
        // the middle of a leap; `x` is where it ends.
        void drawBranchTree(const SceneFrame& frame, const float x, const float groundY,
            const float footY, const float branchSway)
        {
            const float unit = frame.unit;
            const float trunkX = x + 125.0f * unit;
            const float trunkLeft = trunkX - 14.0f * unit;
            const float trunkRight = trunkX + 14.0f * unit;
            const float endSway = branchSway * unit;
            fillBox(frame, { trunkLeft, groundY - 180.0f * unit, trunkRight, footY + 2.0f * unit },
                LinearGradient::Simple({ trunkLeft, 0.0f }, { trunkRight, 0.0f },
                ChaseColors::Bark, ChaseColors::BarkShade));
            PixelPath branch;
            branch.moveTo(trunkX - 10.0f * unit, footY - 124.0f * unit);
            branch.quadTo({ x + 50.0f * unit, footY - 138.0f * unit + 0.5f * endSway },
                { x - 6.0f * unit, footY - 150.0f * unit + endSway });
            branch.lineTo(x - 4.0f * unit, footY - 143.0f * unit + endSway);
            branch.quadTo({ x + 50.0f * unit, footY - 127.0f * unit + 0.5f * endSway },
                { trunkX - 10.0f * unit, footY - 110.0f * unit });
            branch.close();
            fillShape(frame, branch, SolidColor{ ChaseColors::LogDark });
            fillDisc(frame, { x + 16.0f * unit, footY - 153.0f * unit + 0.8f * endSway },
                9.0f * unit, SolidColor{ ChaseColors::CrownLight });
            fillDisc(frame, { trunkX, groundY - 196.0f * unit }, 60.0f * unit,
                SolidColor{ ChaseColors::Crown });
            fillDisc(frame, { trunkX - 40.0f * unit, groundY - 170.0f * unit }, 40.0f * unit,
                SolidColor{ ChaseColors::CrownLight });
            fillDisc(frame, { trunkX + 40.0f * unit, groundY - 172.0f * unit }, 44.0f * unit,
                SolidColor{ ChaseColors::CrownMiddle });
        }
    }
}
