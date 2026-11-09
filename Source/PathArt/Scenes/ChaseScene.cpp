module ClaFi.PathArt.ChaseScene;

import ClaFi.PathArt.DazeStarsEffect;
import ClaFi.PathArt.DustPuffEffect;
import ClaFi.PathArt.HareSprite;
import ClaFi.PathArt.ReedsSprite;
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
        constexpr Color Bough{ 0xFF6B4426 };
        constexpr Color Bark{ 0xFF8A5A33 };
        constexpr Color BarkShade{ 0xFF5B3A20 };
        constexpr Color AppleLight{ 0xFFE84B3C };
        constexpr Color AppleDark{ 0xFFA3241F };
        constexpr Color Shine{ 0x99FFFFFF };
        constexpr Color Leaf{ 0xFF4FA34A };
        constexpr Color AppleFlesh{ 0xFFFBF1D2 };
        constexpr Color AppleSeed{ 0xFF4A2E1A };
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
        constexpr Color DuckLight{ 0xFFFFDC4A };
        constexpr Color DuckShade{ 0xFFE8AE1C };
        constexpr Color DuckBeak{ 0xFFF28C1E };
        constexpr Color DuckBeakShade{ 0xFFC9661A };
        constexpr Color Puff{ 0xB3FFFFFF };
        constexpr Color BallRed{ 0xFFE84B3C };
        constexpr Color BallWhite{ 0xFFF8F6F0 };
        constexpr Color BallBlue{ 0xFF3B7FD9 };
        constexpr Color BallYellow{ 0xFFFFD43B };
        constexpr Color BallEdge{ 0x40000000 };
        constexpr Color BladeDark{ 0xFF2F7A2A };
        constexpr Color BladeMiddle{ 0xFF3F9234 };
        constexpr Color BladeLight{ 0xFF58AE3F };
        constexpr Color Peel{ 0xFFF4D03F };
        constexpr Color PeelShade{ 0xFFD9AE22 };
        constexpr Color PeelInside{ 0xFFFFF3C4 };
        constexpr Color PeelTip{ 0xFF5E4220 };
        constexpr Color RakeHandle{ 0xFFB07A45 };
        constexpr Color RakeHandleShade{ 0xFF8A5A30 };
        constexpr Color RakeMetal{ 0xFF6E747C };
        constexpr Color CapLight{ 0xFFE5483C };
        constexpr Color CapDark{ 0xFFA8261F };
        constexpr Color CapSpot{ 0xFFFAF4E6 };
        constexpr Color Stalk{ 0xFFF2E8D2 };
        constexpr Color StalkShade{ 0xFFD2C2A0 };
        constexpr Color Board{ 0xFFC08A55 };
        constexpr Color BoardShade{ 0xFF8A5A33 };
        constexpr Color StumpTop{ 0xFFD9B98A };
        constexpr Color StumpRing{ 0xFFB08A5A };
        constexpr Color FrogLight{ 0xFF8CC63F };
        constexpr Color FrogDark{ 0xFF4E8A2A };
        constexpr Color FrogBelly{ 0xFFE6EDB0 };
        constexpr Color ShellLight{ 0xFFD08A45 };
        constexpr Color ShellDark{ 0xFF8E4F22 };
        constexpr Color ShellLine{ 0xFF6B3A18 };
        constexpr Color SnailBody{ 0xFFC9B79A };
        constexpr Color Wing{ 0xFFF5A03A };
        constexpr Color WingEdge{ 0xFF3B2A1F };
        constexpr Color StoneLight{ 0xFFB4B2AA };
        constexpr Color StoneDark{ 0xFF76756E };
        constexpr Color StoneCrack{ 0xFF5C5B55 };
        constexpr Color ReedStem{ 0xFF4F8A36 };
        constexpr Color ReedHead{ 0xFF7A5230 };
        constexpr Color Fluff{ 0xFFF2E9D8 };
        constexpr Color FlowerStem{ 0xFF3F8A33 };
        constexpr Color BirdBody{ 0xFF8B6A49 };
        constexpr Color BirdBelly{ 0xFFEADBC0 };
        constexpr Color BirdWing{ 0xFF664A31 };
        constexpr Color BirdBeak{ 0xFFE5A23A };
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
        constexpr float k_episodesPerCycle{ 3.0f };
        // The two episodes before it are runs over obstacles.
        constexpr int k_branchEpisode{ 2 };
        // When the camera starts to slow in an episode, and how long it takes to stop: a trip over
        // an obstacle slows it over 1.4 s, the branch stops it within 0.4 s.
        struct CameraStop
        {
            float start;
            float duration;
        };
        constexpr CameraStop k_tripStop{ 0.55f, 0.07f };
        constexpr CameraStop k_branchStop{ 0.58f, 0.02f };
        // The camera stands from the stop to here, then speeds up again until k_cameraRunning.
        constexpr float k_cameraWaiting{ 0.82f };
        constexpr float k_cameraRunning{ 0.9f };
        // Where the wolf and the hare run, in units from the middle of the frame.
        constexpr float k_wolfChasing{ -200.0f };
        constexpr float k_hareRunning{ 150.0f };
        // The wolf's middle behind the hare's feet as he trips, his nose half a hare short of her
        // tail.
        constexpr float k_wolfBehindHareAtTrip{ 150.0f };
        // Where he lies after a trip, a safe distance short of where the hare sits up.
        constexpr float k_wolfDownAfterTrip{ -70.0f };

        // The branch run's moments, as shares of it: the wolf closes in on the hare, drops
        // back, gathers himself and leaps at her, into the branch at k_branchStop.start.
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

        // How the wolf runs at the hare in the branch run, up to the branch.
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

        // Units the hare runs ahead of k_hareRunning, bolting from the wolf in the branch run.
        [[nodiscard]] float hareBoltAt(float episodeTime);
        // His gap to the hare at his closest is held in pixels, whatever `closeness`.
        [[nodiscard]] BranchRun branchRunAt(float episodeTime, float closeness);

        // What the wolf comes to grief on: the branch in the last run of a cycle, and in each run
        // before it the next of the others in turn.
        enum class Obstacle
        {
            BananaPeel,
            Apple,
            Boot,
            Hedgehog,
            RubberDuck,
            Ball,
            GrassPatch,
            Rake,
            Branch
        };
        constexpr std::array<Obstacle, 8> k_trailObstacles{
            Obstacle::BananaPeel,
            Obstacle::Boot,
            Obstacle::Apple,
            Obstacle::Hedgehog,
            Obstacle::RubberDuck,
            Obstacle::Ball,
            Obstacle::GrassPatch,
            Obstacle::Rake
        };

        // What stands by the path partway into each run; nothing by the path slows the two.
        enum class Roadside
        {
            Apple,
            BananaPeel,
            Boulder,
            Daisy,
            Reeds,
            Signpost,
            Toadstool,
            TreeStump,
            Bird,
            Butterfly,
            Frog,
            Snail
        };
        // One for each run of the four cycles the trail obstacles take; the banana peel and the
        // apple come by earlier in the run that ends on one on the trail.
        constexpr std::array<Roadside, 12> k_roadside{
            Roadside::BananaPeel,
            Roadside::Signpost,
            Roadside::Bird,
            Roadside::Apple,
            Roadside::Toadstool,
            Roadside::Butterfly,
            Roadside::Reeds,
            Roadside::Boulder,
            Roadside::Frog,
            Roadside::Daisy,
            Roadside::TreeStump,
            Roadside::Snail
        };
        // The share of a run at which it passes the middle of the frame; in the branch run, before
        // the wolf closes in.
        constexpr float k_roadsidePasses{ 0.28f };
        constexpr float k_roadsidePassesBeforeBranch{ 0.12f };
        // What stands by the path stirs as the hare comes within this many units of it.
        constexpr float k_hareClose{ 70.0f };

        // How what is on the trail moves at one moment.
        struct ObstacleMotion
        {
            float branchSway; // units the branch's end is pushed down, up below 0
            float curl; // 0 the hedgehog on its feet, 1 rolled into a ball
            float steps; // radians of the hedgehog's walk
            float deflation; // 0 the duck full of air, 1 flat
            float rollAngle; // radians the ball has turned through
            float bladeLean; // radians the grass leans forward from where it stands
            float peelLift; // units the banana peel is in the air
            float peelSpin; // radians the banana peel has turned through
            float rakeSwing; // radians the rake's handle has swung up off the trail
            float bootTip; // radians the boot has tipped over, away from the wolf
            float sinceHit; // shares of the episode since the wolf hit the branch, below 0 before
            float appleSplit; // 0 the apple whole, 1 in two halves lying apart
            bool hareHasApple; // the hare has taken her apple off the ground
        };
        // The hedgehog walks off at 160 units a second, which is this many in an episode's time.
        constexpr float k_hedgehogWalk{ 3200.0f };
        constexpr float k_episodeSeconds{ 20.0f };

        // An apple on the tree, in units from the foot of its trunk; one that falls lets go that
        // long after the wolf hits the branch, and drifts that far as it lands.
        struct TreeApple
        {
            FloatPoint onCrown;
            bool falls;
            float lettingGo;
            float drift;
            float landing; // units below the ground line, where the trail is
        };
        constexpr std::array<TreeApple, 6> k_treeApples{
            TreeApple{ { -58.0f, -158.0f }, true, 0.0f, -14.0f, 18.0f },
            TreeApple{ { -22.0f, -214.0f }, false, 0.0f, 0.0f, 0.0f },
            TreeApple{ { 18.0f, -232.0f }, false, 0.0f, 0.0f, 0.0f },
            TreeApple{ { 30.0f, -188.0f }, true, 0.003f, 10.0f, 22.0f },
            TreeApple{ { 62.0f, -150.0f }, true, 0.006f, 16.0f, 15.0f },
            TreeApple{ { -10.0f, -160.0f }, true, 0.0015f, -6.0f, 26.0f }
        };
        // Units a second a falling apple gathers each second.
        constexpr float k_appleGravity{ 1200.0f };
        // The tree's apples are the trail's apple at a little over half its size, drawn about their
        // middle.
        constexpr float k_appleScale{ 0.55f };
        constexpr float k_appleMiddle{ 9.0f };
        // The fallen apple nearest where the hare sits, which she takes.
        constexpr std::size_t k_hareApple{ 4 };
        // While the wolf lies dazed the hare hops back to it, picks it up and eats it at him.
        namespace HareApple
        {
            constexpr float HopStart{ 0.688f };
            constexpr float HopEnd{ 0.708f };
            constexpr float Grabbed{ 0.716f };
            constexpr float FirstBite{ 0.73f };
            constexpr float BiteGap{ 0.02f };
            constexpr int BiteCount{ 5 };
        }

        // Where a fallen apple's middle comes to rest, in units from the foot of its trunk.
        [[nodiscard]] FloatPoint restingPlace(const TreeApple&);
        constexpr float k_ballRadius{ 13.0f };
        // Units the ball rolls on after the wolf's paws catch it.
        constexpr float k_ballRoll{ 40.0f };

        // `x` is the middle of what lies on the trail, or the end of the branch.
        void drawObstacle(const SceneFrame&, Obstacle, float x, float groundY, float footY,
            const ObstacleMotion&);
        // Each is drawn in units about the middle of where it meets the trail, facing the wolf.
        void drawBananaPeel(const SceneFrame& onTrail, float lift, float spin);
        // Splitting, its halves lean apart and turn their flesh toward the viewer.
        void drawApple(const SceneFrame& onTrail, float split);
        // Tipping, it turns about the back of its heel.
        void drawBoot(const SceneFrame& onTrail, float tip);
        void drawHedgehog(const SceneFrame& onTrail, float curl, float steps);
        void drawRubberDuck(const SceneFrame& onTrail, float deflation);
        void drawBall(const SceneFrame& onTrail, float rollAngle);
        void drawGrassPatch(const SceneFrame& onTrail, float bladeLean);
        // Lying tines up, its head toward the wolf; its handle swings up about the head.
        void drawRake(const SceneFrame& onTrail, float swing);
        void drawBranchTree(const SceneFrame&, float x, float groundY, float footY,
            float branchSway, float sinceHit, bool hareHasApple);
        // How what stands by the path moves as the two run past it.
        struct RoadsideMotion
        {
            float gust; // radians the wind of their passing bends reeds and the daisy after them
            float sinceHareClose; // seconds since the hare came within k_hareClose, below 0 before
            float seconds; // into the run, for the bird's pecking and the butterfly's resting wings
        };
        // `x` is its middle, where it stands on the far edge of the trail.
        void drawRoadside(const SceneFrame&, Roadside, float x, float groundY,
            const RoadsideMotion&);
        // How the boulder and the banana peel jump in the wake of each of them.
        struct Hop
        {
            float lift; // units off the ground
            float tilt; // radians
            float squash; // 0 to 1, as it lands
        };
        [[nodiscard]] Hop hopAt(float sinceHareClose);
        // Something that hops, drawn about the middle of its foot.
        [[nodiscard]] SceneFrame hopping(const SceneFrame& onVerge, const Hop&);
        void drawBoulder(const SceneFrame& onVerge, const Hop&);
        void drawDaisy(const SceneFrame& onVerge, float gust, float sinceHareClose);
        void drawSignpost(const SceneFrame& onVerge, float sinceHareClose);
        void drawToadstool(const SceneFrame& onVerge, float sinceHareClose);
        // The fluff the wolf's wake tears from the reeds' heads, drawn in the reeds' units.
        void drawReedFluff(const SceneFrame& onVerge, float sinceHareClose);
        void drawTreeStump(const SceneFrame& onVerge);
        void drawBird(const SceneFrame& onVerge, float seconds, float sinceHareClose);
        void drawButterfly(const SceneFrame& onVerge, float seconds, float sinceHareClose);
        void drawFrog(const SceneFrame& onVerge, float sinceHareClose);
        void drawSnail(const SceneFrame& onVerge, float sinceHareClose);

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

    // Three 20-second episodes a cycle: twice the wolf trips over something on the trail, then he
    // leaps into a branch. Each time the camera stops while he is down, and the hare taunts him.
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
        const bool branchRun = episode == k_branchEpisode;
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

        // The obstacle reaches the wolf as the camera starts to stop. His gap to the hare and the
        // branch's reach are held in pixels, so they hold in a narrow frame too.
        const float trip = branchRun ? k_branchStop.start : k_tripStop.start;
        const float wolfAtTrip = k_hareRunning - k_wolfBehindHareAtTrip / closeness;
        const float obstacleAtTrip = branchRun
            ? k_wolfAtBranch + k_branchReach / closeness
            : wolfAtTrip + 30.0f;
        const std::int64_t trailObstacleCount
            = static_cast<std::int64_t>(k_trailObstacles.size());
        const std::int64_t trailRunsPassed = time.cyclesPassed * k_branchEpisode + episode;
        const Obstacle obstacle = branchRun
            ? Obstacle::Branch
            : k_trailObstacles[static_cast<std::size_t>(trailRunsPassed % trailObstacleCount)];
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
        // The apple splits under his paw.
        const float appleSplit = obstacle == Obstacle::Apple
            ? smoothStep(trip - 0.001f, trip + 0.008f, episodeTime)
            : 0.0f;
        // The duck goes flat under the wolf's paws, and the ball rolls on a little, slowing.
        const float deflation = obstacle == Obstacle::RubberDuck
            ? smoothStep(trip - 0.002f, trip + 0.02f, episodeTime)
            : 0.0f;
        const float rolling = obstacle == Obstacle::Ball
            ? std::clamp((episodeTime - trip) / 0.05f, 0.0f, 1.0f)
            : 0.0f;
        const float rolledOff = k_ballRoll * (1.0f - (1.0f - rolling) * (1.0f - rolling));
        // While the wolf lies dazed it rolls back a little, slowly, and comes to rest.
        const float rolledAway = obstacle == Obstacle::Ball
            ? 30.0f * smoothStep(0.64f, 0.74f, episodeTime)
            : 0.0f;
        // The banana peel shoots out ahead from under his paw, spinning, and lands flat.
        const float peelFlight = obstacle == Obstacle::BananaPeel
            ? std::clamp((episodeTime - trip) / 0.045f, 0.0f, 1.0f)
            : 0.0f;
        const float peelAhead = 70.0f * (1.0f - (1.0f - peelFlight) * (1.0f - peelFlight));
        const float obstacleX = obstacleAtTrip + cameraTravel(episodeTime, trip) - walkedAway
            + rolledOff - rolledAway + peelAhead;
        // The grass goes flat under him while his legs are in it, then springs back, swinging past
        // upright and settling.
        const float sinceTrip = episodeTime - trip;
        const float sinceLetGo = sinceTrip - 0.012f;
        // His paw on the rake's tines swings its handle up into his face, where it rides a moment;
        // then it goes on past upright, drops back and bounces once.
        float rakeSwing = 0.0f;
        float rakeOnFace = 0.0f;
        if (obstacle == Obstacle::Rake && sinceTrip > 0.0f)
        {
            const float dropping = std::clamp((sinceTrip - 0.012f) / 0.012f, 0.0f, 1.0f);
            const float bounce = std::clamp((sinceTrip - 0.024f) / 0.006f, 0.0f, 1.0f);
            rakeSwing = 1.75f * smoothStep(0.0f, 0.006f, sinceTrip) * (1.0f - dropping * dropping)
                + 0.15f * std::sin(std::numbers::pi_v<float> * bounce);
            rakeOnFace = smoothStep(0.001f, 0.0025f, sinceTrip)
                * (1.0f - smoothStep(0.008f, 0.01f, sinceTrip));
        }
        // The boot, caught at the toe, falls over away from him, gathering speed, and rocks once.
        float bootTip = 0.0f;
        if (obstacle == Obstacle::Boot && sinceTrip > 0.0f)
        {
            const float falling = std::min(1.0f, sinceTrip / 0.012f);
            const float rocking = std::clamp((sinceTrip - 0.012f) / 0.007f, 0.0f, 1.0f);
            bootTip = 0.5f * std::numbers::pi_v<float> * falling * falling
                - 0.12f * std::sin(std::numbers::pi_v<float> * rocking);
        }
        float bladeLean = 0.0f;
        if (obstacle == Obstacle::GrassPatch && sinceTrip > 0.0f)
        {
            bladeLean = sinceLetGo < 0.0f
                ? 1.1f * smoothStep(0.0f, 0.004f, sinceTrip)
                : 1.1f * std::exp(-sinceLetGo / 0.015f) * std::cos(k_2Pi * sinceLetGo / 0.02f);
        }

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
        const float lyingX = branchRun
            ? k_wolfAtBranch - cameraTravel(trip, fallEnd) - 18.0f
            : k_wolfDownAfterTrip;
        if (episodeTime < trip)
        {
            if (!branchRun)
            {
                // gaining on the hare ever faster and pressing harder, until the obstacle stops him
                const float towardTrip = episodeTime / trip;
                const float gained = towardTrip * towardTrip;
                wolfX = std::lerp(k_wolfChasing, wolfAtTrip, gained);
                strain = gained;
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
            if (!branchRun)
            {
                // over the obstacle, head over heels, and flat on his belly, stopped short while
                // the hare runs on
                wolfX = std::lerp(wolfAtTrip, lyingX, smoothStep(0.0f, 1.0f, fall));
                strain = 1.0f - smoothStep(0.0f, 0.2f, fall);
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
            if (branchRun)
            {
                const float bounce = std::min(1.0f, (episodeTime - fallEnd) / 0.016f);
                lift = 7.0f * std::sin(std::numbers::pi_v<float> * bounce);
            }
            turn = branchRun ? -std::numbers::pi_v<float> : 0.0f;
            sprawl = branchRun ? 0.0f : 1.0f;
            onBack = branchRun ? 1.0f : 0.0f;
            dazed = 1.0f;
            running = 0.0f;
        }
        else if (episodeTime < 0.86f)
        {
            const float rise = (episodeTime - k_cameraWaiting) / (0.86f - k_cameraWaiting);
            wolfX = lyingX;
            if (!branchRun)
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
        const bool bonked = branchRun && episodeTime > trip && episodeTime < trip + 0.012f;
        // On his back the head drops toward the ground rather than into it; the branch and the
        // rake's handle knock it back.
        const float headDrop = 0.7f * onBack + (bonked ? -0.3f : 0.0f) - 0.35f * rakeOnFace;
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

        // The hare: running ahead, clearing the obstacle, sitting up to taunt, and off again. Over
        // the trail her ears go flat as the wolf closes in; in the branch run she bolts as he comes
        // close and eases off as he drops back.
        float bolt = 0.0f;
        float alarm = 0.0f;
        if (branchRun)
        {
            bolt = hareBoltAt(episodeTime);
            alarm = std::max(smoothStep(0.23f, 0.27f, episodeTime)
                * (1.0f - smoothStep(0.36f, 0.44f, episodeTime)),
                bump(episodeTime, BranchChase::Takeoff + 0.012f, 0.012f));
        }
        else
        {
            alarm = smoothStep(0.3f, trip, episodeTime)
                * (1.0f - smoothStep(trip, trip + 0.03f, episodeTime));
        }
        const float runningAhead = smoothStep(branchRun ? 0.57f : trip, 0.66f, episodeTime);
        const float hareSitsAt = k_hareRunning + bolt + 100.0f * runningAhead;
        // In the branch run she hops back to the fallen apple until her forepaws are over it,
        // picks it up and eats it in bites, crumbs flying, while the wolf lies dazed.
        float hareAtApple = 0.0f;
        float appleHop = 0.0f;
        float holding = 0.0f;
        float appleLeft = 1.0f;
        FloatPoint appleOnGround = {};
        if (branchRun)
        {
            // Where the apple lies while the camera stands; taken as the camera moves on, it would
            // carry her back with the ground.
            const FloatPoint restingMiddle = restingPlace(k_treeApples[k_hareApple]);
            const float treeWhileStanding = obstacleAtTrip
                + cameraTravel(HareApple::HopStart, trip);
            const float trunkOnScreen = toScreen(treeWhileStanding) + 125.0f * unit;
            appleOnGround = {
                trunkOnScreen + restingMiddle.x * unit,
                groundY + restingMiddle.y * unit
            };
            const float pawsOverApple = (appleOnGround.x + 24.0f * unit - width * 0.5f)
                / (unit * closeness);
            const float hopping = std::clamp((episodeTime - HareApple::HopStart)
                / (HareApple::HopEnd - HareApple::HopStart), 0.0f, 1.0f);
            hareAtApple = (pawsOverApple - hareSitsAt) * smoothStep(0.0f, 1.0f, hopping);
            appleHop = std::sin(std::numbers::pi_v<float> * hopping);
            const float lastBite = HareApple::FirstBite
                + HareApple::BiteGap * static_cast<float>(HareApple::BiteCount - 1);
            holding = smoothStep(HareApple::HopEnd, HareApple::Grabbed, episodeTime)
                * (1.0f - smoothStep(lastBite, lastBite + 0.01f, episodeTime));
            for (int bite = 0; bite != HareApple::BiteCount; ++bite)
            {
                const float biteAt = HareApple::FirstBite
                    + HareApple::BiteGap * static_cast<float>(bite);
                appleLeft -= smoothStep(biteAt, biteAt + 0.002f, episodeTime)
                    / static_cast<float>(HareApple::BiteCount);
            }
        }
        // Her front teeth show while she holds the apple and come out full at each bite.
        float hareTeeth = 0.4f * holding;
        if (branchRun)
        {
            for (int bite = 0; bite != HareApple::BiteCount; ++bite)
            {
                const float biteAt = HareApple::FirstBite
                    + HareApple::BiteGap * static_cast<float>(bite);
                hareTeeth = std::max(hareTeeth, bump(episodeTime, biteAt, 0.004f));
            }
        }
        const float hareX = k_hareRunning + (hareSitsAt + hareAtApple - k_hareRunning)
            * (1.0f - smoothStep(0.86f, 1.0f, episodeTime));
        const float sitting = smoothStep(0.62f, 0.66f, episodeTime)
            * (1.0f - smoothStep(0.83f, 0.86f, episodeTime)) * (1.0f - appleHop);
        const float turnedRound = smoothStep(0.66f, 0.69f, episodeTime)
            * (1.0f - smoothStep(0.82f, 0.85f, episodeTime));
        const float facing = 1.0f - 2.0f * turnedRound;
        const float hop = k_2Pi * (travel + hareX) / periodNear(1.0f, 180.0f);
        const float hopScale = branchRun
            ? 1.0f + 0.45f * alarm
            : 1.0f + 0.45f * alarm + 1.7f * bump(episodeTime, hareClears, 0.012f);
        const float air = appleHop > 0.0f
            ? appleHop
            : std::max(0.0f, std::sin(hop)) * (1.0f - sitting);
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
            .footLift = sitting * 7.0f * std::max(0.0f, std::sin(k_2Pi * 36.0f * episodeTime)),
            .holding = holding,
            .teeth = hareTeeth
        };

        // The branch springs up where his head hit it and shakes itself still.
        const float sinceBonk = episodeTime - trip;
        const float branchSway = branchRun && sinceBonk > 0.0f
            ? -10.0f * std::exp(-sinceBonk / 0.025f) * std::sin(k_2Pi * sinceBonk / 0.0125f)
            : 0.0f;
        const float obstacleOnScreen = toScreen(obstacleX);
        const bool obstacleShown = obstacleOnScreen > -220.0f * unit
            && obstacleOnScreen < width + 200.0f * unit;
        if (rakeOnFace > 0.0f)
        {
            // the handle laid against the front of his face, from where it turns at the rake's head
            const FloatPoint snout = wolf
                + rotated(FloatPoint{ 70.0f, -22.0f } * wolfSize, wolfPose.turn);
            const FloatPoint rakePivot = { obstacleOnScreen - 4.0f * unit, footY - 2.0f * unit };
            const float swingToSnout = std::atan2(rakePivot.y - snout.y, snout.x - rakePivot.x);
            rakeSwing = std::lerp(rakeSwing, swingToSnout, rakeOnFace);
        }
        const ObstacleMotion obstacleMotion = {
            .branchSway = branchSway,
            .curl = curl,
            .steps = k_2Pi * walkedAway / 9.0f,
            .deflation = deflation,
            .rollAngle = (rolledOff - rolledAway) / k_ballRadius,
            .bladeLean = bladeLean,
            .peelLift = 240.0f * peelFlight * (1.0f - peelFlight),
            .peelSpin = 2.0f * k_2Pi * peelFlight,
            .rakeSwing = rakeSwing,
            .bootTip = bootTip,
            .sinceHit = sinceBonk,
            .appleSplit = appleSplit,
            .hareHasApple = branchRun && episodeTime >= HareApple::HopEnd
        };
        // A swinging rake goes over the wolf, so its handle is seen to catch him.
        const bool obstacleOverWolf = rakeSwing > 0.0f;
        // What stands by the path in this run, and the next run's as it comes into a wide frame.
        const std::int64_t runsPerCycle = static_cast<std::int64_t>(k_episodesPerCycle);
        const std::int64_t roadsideCount = static_cast<std::int64_t>(k_roadside.size());
        for (int ahead = 0; ahead != 2; ++ahead)
        {
            const std::int64_t run = time.cyclesPassed * runsPerCycle + episode + ahead;
            const Roadside roadside = k_roadside[static_cast<std::size_t>(run % roadsideCount)];
            const bool beforeBranch = run % runsPerCycle == k_branchEpisode;
            const float passes = beforeBranch ? k_roadsidePassesBeforeBranch : k_roadsidePasses;
            const float roadsideX = ahead == 0
                ? cameraTravel(episodeTime, passes)
                : cameraTravel(episodeTime, 1.0f) + passes * k_fullSpeedTravel;
            const float roadsideOnScreen = toScreen(roadsideX);
            const float runTime = episodeTime - static_cast<float>(ahead);
            const float hareCloseAt = passes - (k_hareRunning + k_hareClose) / k_fullSpeedTravel;
            const float sinceHareClose = (runTime - hareCloseAt) * k_episodeSeconds;
            const bool flies = roadside == Roadside::Bird || roadside == Roadside::Butterfly;
            const bool inView = roadsideOnScreen > -80.0f * unit
                && roadsideOnScreen < width + 80.0f * unit;
            if (!inView && !(flies && sinceHareClose > 0.0f && sinceHareClose < 3.0f))
                continue;
            // the wind behind each of them as they pass bends it after them, the wolf's more
            const auto wake = [&](float runnerX, float strength) {
                const float behind = (runnerX - roadsideX - 15.0f) / 35.0f;
                return strength * std::exp(-behind * behind);
            };
            drawRoadside(frame, roadside, roadsideOnScreen, groundY, RoadsideMotion{
                .gust = wake(hareX, 0.35f) + wake(wolfX, 0.5f),
                .sinceHareClose = sinceHareClose,
                .seconds = runTime * k_episodeSeconds
            });
        }
        if (obstacleShown && !obstacleOverWolf)
            drawObstacle(frame, obstacle, obstacleOnScreen, groundY, footY, obstacleMotion);

        const SolidColor shadow{ ChaseColors::Shadow };
        const float wolfShadow = 46.0f * unit * (1.0f - 0.3f * std::min(1.0f, lift / 60.0f));
        fillOval(frame, { wolf.x, footY + 2.0f * unit }, wolfShadow, 5.0f * unit, shadow);
        const float hareShadow = 26.0f * unit * (1.0f - 0.3f * air);
        fillOval(frame, { hareOnScreen, footY + 2.0f * unit }, hareShadow, 4.0f * unit, shadow);
        // dust kicked up behind him as he leaps
        const float sinceTakeoff = episodeTime - BranchChase::Takeoff;
        if (branchRun && sinceTakeoff > 0.0f && sinceTakeoff < 0.04f)
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
        // Lifted from where it lay to her chest, the apple bobs up to her mouth at each bite and
        // comes back a bite smaller; crumbs of skin and flesh fly from its top.
        float toMouth = 0.0f;
        for (int bite = 0; bite != HareApple::BiteCount; ++bite)
        {
            toMouth += bump(episodeTime, HareApple::FirstBite
                + HareApple::BiteGap * static_cast<float>(bite), 0.003f);
        }
        const FloatPoint atChest = harePose.foot
            + FloatPoint{ (27.0f - 2.0f * toMouth) * harePose.facing, -26.0f - 11.0f * toMouth }
                * hareSize;
        const FloatPoint heldMiddle = pointBetween(appleOnGround, atChest,
            smoothStep(HareApple::HopEnd, HareApple::Grabbed, episodeTime));
        if (holding > 0.0f && appleLeft > 0.0f)
        {
            const float heldScale = k_appleScale * std::sqrt(appleLeft) * unit;
            drawApple(frame.within(Matrix3x2::Translation(heldMiddle)
                * Matrix3x2::Scale(heldScale) * Matrix3x2::Translation(0.0f, k_appleMiddle)),
                0.0f);
        }
        if (branchRun)
        {
            const FloatPoint appleTop = heldMiddle + FloatPoint{ 0.0f, -6.0f * unit };
            for (int bite = 0; bite != HareApple::BiteCount; ++bite)
            {
                const float biteAt = HareApple::FirstBite
                    + HareApple::BiteGap * static_cast<float>(bite);
                const float seconds = (episodeTime - biteAt) * k_episodeSeconds;
                if (seconds <= 0.0f || seconds > 0.3f)
                    continue;
                for (int crumb = 0; crumb != 4; ++crumb)
                {
                    const float spread = pseudoRandom(crumb + 4 * bite, 151.0f);
                    const FloatPoint speed = {
                        (20.0f + 40.0f * spread) * harePose.facing,
                        -30.0f - 30.0f * pseudoRandom(crumb + 4 * bite, 152.0f)
                    };
                    const FloatPoint at = appleTop + (speed * seconds
                        + FloatPoint{ 0.0f, 0.5f * k_appleGravity * seconds * seconds }) * unit;
                    const Color crumbColor = crumb % 2 == 0
                        ? ChaseColors::AppleLight
                        : ChaseColors::AppleFlesh;
                    fillDisc(frame, at, 1.8f * unit,
                        SolidColor{ crumbColor.withOpacity(1.0f - seconds / 0.3f) });
                }
            }
        }
        drawWolf(frame, wolfPose);
        if (obstacleShown && obstacleOverWolf)
            drawObstacle(frame, obstacle, obstacleOnScreen, groundY, footY, obstacleMotion);

        // dust where he lands, and stars round his head while he is down
        const float landing = branchRun ? fallEnd - 0.003f : 0.6f;
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
        const CameraStop stop = episode == k_branchEpisode ? k_branchStop : k_tripStop;
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
        float before = 0.0f;
        for (int earlier = 0; earlier != episode; ++earlier)
            before += cameraShare(1.0f, earlier);
        const float episodeTime = fractionalPart(cycleTime * k_episodesPerCycle);
        return (before + cameraShare(episodeTime, episode)) * k_fullSpeedTravel;
    }

    float ChaseScene::periodNear(const float parallax, const float wanted)
    {
        float cycleShare = 0.0f;
        for (int episode = 0; episode != static_cast<int>(k_episodesPerCycle); ++episode)
            cycleShare += cameraShare(1.0f, episode);
        const float cycleTravel = cycleShare * k_fullSpeedTravel;
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
            const float contact = k_branchStop.start;
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
                case Obstacle::BananaPeel:
                    drawBananaPeel(onTrail, motion.peelLift, motion.peelSpin);
                    break;
                case Obstacle::Apple:
                    drawApple(onTrail, motion.appleSplit);
                    break;
                case Obstacle::Boot:
                    drawBoot(onTrail, motion.bootTip);
                    break;
                case Obstacle::Hedgehog:
                    drawHedgehog(onTrail, motion.curl, motion.steps);
                    break;
                case Obstacle::RubberDuck:
                    drawRubberDuck(onTrail, motion.deflation);
                    break;
                case Obstacle::Ball:
                    drawBall(onTrail, motion.rollAngle);
                    break;
                case Obstacle::GrassPatch:
                    drawGrassPatch(onTrail, motion.bladeLean);
                    break;
                case Obstacle::Rake:
                    drawRake(onTrail, motion.rakeSwing);
                    break;
                case Obstacle::Branch:
                    drawBranchTree(frame, x, groundY, footY, motion.branchSway, motion.sinceHit,
                        motion.hareHasApple);
                    break;
            }
        }

        // A peel of three flaps splayed on the trail, turning about its middle as it flies.
        void drawBananaPeel(const SceneFrame& onTrail, const float lift, const float spin)
        {
            const FloatPoint middle = { 0.0f, -7.0f };
            const SceneFrame peel = onTrail.within(Matrix3x2::Translation(middle.x,
                middle.y - lift) * Matrix3x2::Rotation(spin)
                * Matrix3x2::Translation(-middle.x, -middle.y));
            const auto drawFlap = [&](FloatPoint root, FloatPoint tip, float bulge) {
                const FloatPoint along = tip - root;
                const FloatPoint across = FloatPoint{ -along.y, along.x } * bulge;
                PixelPath flap;
                flap.moveTo(root);
                flap.quadTo(root + along * 0.5f + across, tip);
                flap.quadTo(root + along * 0.5f - across * 0.3f, root);
                flap.close();
                fillShape(peel, flap, SolidColor{ ChaseColors::Peel });
                PixelPath inside;
                inside.moveTo(root);
                inside.quadTo(root + along * 0.5f + across * 0.4f, root + along * 0.85f);
                inside.quadTo(root + along * 0.5f - across * 0.1f, root);
                inside.close();
                fillShape(peel, inside, SolidColor{ ChaseColors::PeelInside });
                fillDisc(peel, tip, 1.8f, SolidColor{ ChaseColors::PeelTip });
            };
            drawFlap({ 1.5f, -10.0f }, { 14.0f, -1.5f }, 0.55f);
            drawFlap({ -1.5f, -10.0f }, { -20.0f, -2.0f }, -0.55f);
            fillRoundedBox(peel, { -4.0f, -15.0f, 4.0f, -4.0f }, 3.5f,
                SolidColor{ ChaseColors::PeelShade });
            drawFlap({ 0.0f, -9.0f }, { 4.0f, 0.0f }, 0.5f);
            drawSegment(peel, { 0.0f, -14.0f }, { 2.0f, -19.0f },
                SolidColor{ ChaseColors::PeelTip }, 2.6f);
        }

        void drawApple(const SceneFrame& onTrail, const float split)
        {
            const LinearGradient skin = LinearGradient::Simple({ -8.0f, -18.0f }, { 8.0f, 0.0f },
                ChaseColors::AppleLight, ChaseColors::AppleDark);
            const FloatPoint stemTop = { 0.0f, -17.0f };
            if (split <= 0.0f)
            {
                PixelPath apple;
                apple.moveTo(stemTop);
                apple.cubicTo({ 6.0f, -22.0f }, { 14.0f, -18.0f }, { 12.0f, -8.0f });
                apple.cubicTo({ 11.0f, -1.0f }, { 4.0f, 1.0f }, { 0.0f, -1.0f });
                apple.cubicTo({ -4.0f, 1.0f }, { -11.0f, -1.0f }, { -12.0f, -8.0f });
                apple.cubicTo({ -14.0f, -18.0f }, { -6.0f, -22.0f }, stemTop);
                apple.close();
                fillShape(onTrail, apple, skin);
                fillOval(onTrail, { -6.0f, -13.0f }, 2.5f, 4.0f, SolidColor{ ChaseColors::Shine },
                    0.4f);
                drawSegment(onTrail, stemTop, { 1.5f, -24.0f },
                    SolidColor{ ChaseColors::BarkShade }, 1.8f);
                fillOval(onTrail, { 5.5f, -22.0f }, 5.0f, 2.2f, SolidColor{ ChaseColors::Leaf },
                    -0.5f);
                return;
            }

            // Each half leans out about its outer foot and slides a little apart; the cut, seen
            // turning toward the viewer, widens from a line to most of the half.
            for (const float side : { -1.0f, 1.0f })
            {
                const FloatPoint foot = { 11.0f * side, -1.0f };
                const SceneFrame half = onTrail.within(Matrix3x2::Translation(3.0f * split * side,
                    0.0f) * Matrix3x2::Translation(foot) * Matrix3x2::Rotation(0.55f * split * side)
                    * Matrix3x2::Translation(-foot.x, -foot.y));
                PixelPath skinHalf;
                skinHalf.moveTo(stemTop);
                skinHalf.cubicTo({ 6.0f * side, -22.0f }, { 14.0f * side, -18.0f },
                    { 12.0f * side, -8.0f });
                skinHalf.cubicTo({ 11.0f * side, -1.0f }, { 4.0f * side, 1.0f }, { 0.0f, -1.0f });
                skinHalf.close();
                fillShape(half, skinHalf, skin);
                const float faceWidth = 0.3f + 5.0f * split;
                const FloatPoint faceMiddle = { (0.5f + 5.0f * split) * side, -9.0f };
                fillOval(half, faceMiddle, faceWidth, 7.2f, SolidColor{ ChaseColors::AppleFlesh });
                for (const float seedY : { -11.0f, -7.5f })
                {
                    fillOval(half, { faceMiddle.x, seedY }, 0.8f * split, 1.4f * split,
                        SolidColor{ ChaseColors::AppleSeed });
                }
                if (side > 0.0f)
                {
                    drawSegment(half, stemTop, { 1.5f, -24.0f },
                        SolidColor{ ChaseColors::BarkShade }, 1.8f);
                    fillOval(half, { 5.5f, -22.0f }, 5.0f, 2.2f, SolidColor{ ChaseColors::Leaf },
                        -0.5f);
                }
            }
        }

        // An old boot, its toe toward the wolf.
        void drawBoot(const SceneFrame& onTrail, const float tip)
        {
            const FloatPoint heel = { 16.0f, 0.0f };
            const SceneFrame boot = onTrail.within(Matrix3x2::Translation(heel)
                * Matrix3x2::Rotation(tip) * Matrix3x2::Translation(-heel.x, -heel.y));
            PixelPath leather;
            leather.moveTo(15.0f, -3.0f);
            leather.lineTo(15.0f, -32.0f);
            leather.lineTo(0.0f, -32.0f);
            leather.lineTo(0.0f, -19.0f);
            leather.quadTo({ -6.0f, -17.0f }, { -14.0f, -14.0f });
            leather.quadTo({ -21.0f, -12.0f }, { -21.0f, -6.0f });
            leather.lineTo(-21.0f, -3.0f);
            leather.close();
            fillShape(boot, leather, LinearGradient::Simple({ 0.0f, -32.0f }, { 0.0f, -3.0f },
                ChaseColors::BootLight, ChaseColors::BootDark));
            fillBox(boot, { -0.5f, -33.0f, 15.5f, -29.5f }, SolidColor{ ChaseColors::BootDark });
            fillBox(boot, { 7.0f, -16.0f, 12.0f, -11.0f }, SolidColor{ ChaseColors::BootPatch });
            fillRoundedBox(boot, { -22.0f, -4.0f, 16.0f, 0.0f }, 1.5f,
                SolidColor{ ChaseColors::BootSole });
            PixelPath laces;
            for (const float y : { -27.0f, -23.0f, -19.0f })
            {
                laces.moveTo(1.0f, y - 1.5f);
                laces.lineTo(6.0f, y + 1.5f);
                laces.moveTo(1.0f, y + 1.5f);
                laces.lineTo(6.0f, y - 1.5f);
            }
            strokeShape(boot, laces, SolidColor{ ChaseColors::Lace }, 1.2f);
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

        // A rubber duck, its beak toward the wolf. Deflating, it sinks flat and spreads, its head
        // droops and the air puffs out behind it.
        void drawRubberDuck(const SceneFrame& onTrail, const float deflation)
        {
            const SceneFrame duck = onTrail.within(Matrix3x2::Scale(1.0f + 0.2f * deflation,
                1.0f - 0.62f * deflation));
            PixelPath body;
            body.moveTo(-14.0f, -10.0f);
            body.cubicTo({ -15.0f, -2.0f }, { -7.0f, 0.0f }, { 2.0f, 0.0f });
            body.cubicTo({ 12.0f, 0.0f }, { 17.0f, -3.0f }, { 17.0f, -9.0f });
            body.lineTo(20.0f, -18.0f);
            body.quadTo({ 13.0f, -16.0f }, { 8.0f, -15.0f });
            body.quadTo({ -4.0f, -13.0f }, { -14.0f, -10.0f });
            body.close();
            fillShape(duck, body, LinearGradient::Simple({ 0.0f, -18.0f }, {},
                ChaseColors::DuckLight, ChaseColors::DuckShade));
            fillOval(duck, { 4.0f, -8.0f }, 7.0f, 3.5f, SolidColor{ ChaseColors::DuckShade },
                -0.3f);
            const FloatPoint neck = { -7.0f, -13.0f };
            const SceneFrame head = duck.within(Matrix3x2::Translation(neck)
                * Matrix3x2::Rotation(-0.5f * deflation)
                * Matrix3x2::Translation(-neck.x, -neck.y));
            fillDisc(head, { -8.0f, -21.0f }, 8.0f, SolidColor{ ChaseColors::DuckLight });
            PixelPath beak;
            beak.moveTo(-14.0f, -22.0f);
            beak.quadTo({ -21.0f, -23.0f }, { -24.0f, -19.0f });
            beak.quadTo({ -20.0f, -17.0f }, { -14.0f, -18.0f });
            beak.close();
            fillShape(head, beak, SolidColor{ ChaseColors::DuckBeak });
            PixelPath lowerBeak;
            lowerBeak.moveTo(-14.0f, -18.0f);
            lowerBeak.quadTo({ -19.0f, -17.5f }, { -22.0f, -18.0f });
            lowerBeak.quadTo({ -19.0f, -15.5f }, { -14.0f, -16.0f });
            lowerBeak.close();
            fillShape(head, lowerBeak, SolidColor{ ChaseColors::DuckBeakShade });
            fillDisc(head, { -10.0f, -23.0f }, 1.7f, SolidColor{ ChaseColors::Ink });
            fillDisc(head, { -9.5f, -23.5f }, 0.5f, SolidColor{ ChaseColors::EyeShine });
            // the air leaving it, strongest halfway down and gone once it is flat
            const float puff = 4.0f * deflation * (1.0f - deflation);
            if (puff > 0.0f)
            {
                PixelPath streaks;
                for (const float slope : { -0.5f, 0.0f, 0.5f })
                {
                    streaks.moveTo(22.0f, -10.0f + 6.0f * slope);
                    streaks.lineTo(30.0f, -10.0f + 14.0f * slope);
                }
                strokeShape(onTrail, streaks, SolidColor{ ChaseColors::Puff.withOpacity(puff) },
                    1.6f);
            }
        }

        // A beach ball of six panels, which turn as it rolls.
        void drawBall(const SceneFrame& onTrail, const float rollAngle)
        {
            constexpr std::array<Color, 6> k_panels{
                ChaseColors::BallRed,
                ChaseColors::BallWhite,
                ChaseColors::BallBlue,
                ChaseColors::BallWhite,
                ChaseColors::BallYellow,
                ChaseColors::BallWhite
            };
            const SceneFrame ball = onTrail.within(Matrix3x2::Translation(0.0f, -k_ballRadius)
                * Matrix3x2::Rotation(rollAngle));
            for (std::size_t panel = 0; panel != k_panels.size(); ++panel)
            {
                const float firstAngle = k_2Pi * static_cast<float>(panel) / 6.0f;
                PixelPath wedge;
                wedge.moveTo(0.0f, 0.0f);
                for (int i = 0; i <= 8; ++i)
                {
                    const float angle = firstAngle + k_2Pi / 6.0f * static_cast<float>(i) / 8.0f;
                    wedge.lineTo(k_ballRadius * std::cos(angle), k_ballRadius * std::sin(angle));
                }
                wedge.close();
                fillShape(ball, wedge, SolidColor{ k_panels[panel] });
            }
            fillDisc(ball, {}, 3.0f, SolidColor{ ChaseColors::BallWhite });
            PixelPath rim;
            addEllipse(rim, {}, k_ballRadius, k_ballRadius);
            strokeShape(ball, rim, SolidColor{ ChaseColors::BallEdge }, 1.0f);
            fillOval(onTrail, { -5.0f, -19.0f }, 4.0f, 2.5f, SolidColor{ ChaseColors::Shine },
                -0.5f);
        }

        void drawRoadside(const SceneFrame& frame, const Roadside roadside, const float x,
            const float groundY, const RoadsideMotion& motion)
        {
            const float unit = frame.unit;
            const float vergeY = groundY + 10.0f * unit;
            const SceneFrame onVerge = frame.within(Matrix3x2::Translation(x, vergeY)
                * Matrix3x2::Scale(unit));
            const auto shadow = [&](float shadowWidth, float opacity) {
                fillOval(onVerge, { 1.0f, 0.0f }, shadowWidth, 2.5f,
                    SolidColor{ ChaseColors::Shadow.withOpacity(opacity) });
            };
            switch (roadside)
            {
                case Roadside::Apple:
                {
                    const Hop hop = hopAt(motion.sinceHareClose);
                    shadow(10.0f * (1.0f - hop.lift / 54.0f), 1.0f);
                    drawApple(hopping(onVerge, hop).within(Matrix3x2::Scale(0.8f)), 0.0f);
                    break;
                }
                case Roadside::BananaPeel:
                {
                    const Hop hop = hopAt(motion.sinceHareClose);
                    shadow(14.0f * (1.0f - hop.lift / 54.0f), 1.0f);
                    drawBananaPeel(hopping(onVerge, hop), 0.0f, 0.0f);
                    break;
                }
                case Roadside::Boulder:
                    drawBoulder(onVerge, hopAt(motion.sinceHareClose));
                    break;
                case Roadside::Daisy:
                    drawDaisy(onVerge, motion.gust, motion.sinceHareClose);
                    break;
                case Roadside::Reeds:
                {
                    const float reedsSize = 0.9f * unit;
                    drawReeds(frame, ReedsPose{
                        .base = { x - 12.0f * reedsSize, vergeY },
                        .size = reedsSize,
                        .cycleTime = -1.0f / k_2Pi, // their own sway fans them evenly
                        .lean = motion.gust,
                        .fur = 1.0f - 0.65f * smoothStep(1.0f, 1.3f, motion.sinceHareClose),
                        .inWater = false
                    }, ReedColors{ .stem = ChaseColors::ReedStem, .head = ChaseColors::ReedHead });
                    drawReedFluff(onVerge, motion.sinceHareClose);
                    break;
                }
                case Roadside::Signpost:
                    shadow(7.0f, 1.0f);
                    drawSignpost(onVerge, motion.sinceHareClose);
                    break;
                case Roadside::Toadstool:
                    shadow(12.0f, 1.0f);
                    drawToadstool(onVerge, motion.sinceHareClose);
                    break;
                case Roadside::TreeStump:
                    shadow(19.0f, 1.0f);
                    drawTreeStump(onVerge);
                    break;
                case Roadside::Bird:
                    shadow(10.0f, std::clamp(1.0f - 2.0f * motion.sinceHareClose, 0.0f, 1.0f));
                    drawBird(onVerge.within(Matrix3x2::Scale(1.5f)), motion.seconds,
                        motion.sinceHareClose);
                    break;
                case Roadside::Butterfly:
                    drawButterfly(onVerge.within(Matrix3x2::Scale(1.3f)), motion.seconds,
                        motion.sinceHareClose);
                    break;
                case Roadside::Frog:
                    drawFrog(onVerge.within(Matrix3x2::Scale(1.2f)), motion.sinceHareClose);
                    break;
                case Roadside::Snail:
                    shadow(12.0f, 1.0f);
                    drawSnail(onVerge, motion.sinceHareClose);
                    break;
            }
        }

        // Once just after the hare is by, higher just after the wolf is, then once more.
        Hop hopAt(const float sinceHareClose)
        {
            struct Bounce
            {
                float start;
                float seconds;
                float height;
                float tilt;
            };
            constexpr std::array<Bounce, 3> k_bounces{
                Bounce{ 0.34f, 0.22f, 6.75f, -0.06f },
                Bounce{ 1.32f, 0.38f, 21.6f, 0.14f },
                Bounce{ 1.7f, 0.22f, 6.75f, -0.05f }
            };
            Hop hop = {};
            for (const Bounce& bounce : k_bounces)
            {
                const float inAir = (sinceHareClose - bounce.start) / bounce.seconds;
                if (inAir > 0.0f && inAir < 1.0f)
                {
                    const float arc = std::sin(std::numbers::pi_v<float> * inAir);
                    hop.lift = bounce.height * arc;
                    hop.tilt = bounce.tilt * arc;
                }
                hop.squash = std::max(hop.squash, (bounce.height / 21.6f)
                    * bump(sinceHareClose, bounce.start + bounce.seconds + 0.03f, 0.05f));
            }
            return hop;
        }

        SceneFrame hopping(const SceneFrame& onVerge, const Hop& hop)
        {
            return onVerge.within(Matrix3x2::Translation(0.0f, -hop.lift)
                * Matrix3x2::Rotation(hop.tilt) * Matrix3x2::Scale(1.0f + 0.06f * hop.squash,
                1.0f - 0.1f * hop.squash));
        }

        void drawBoulder(const SceneFrame& onVerge, const Hop& hop)
        {
            constexpr float k_size = 1.35f;
            const SceneFrame ground = onVerge.within(Matrix3x2::Scale(k_size));
            fillOval(ground, { 1.0f, 0.0f }, 17.5f * (1.0f - hop.lift / 54.0f), 1.9f,
                SolidColor{ ChaseColors::Shadow });
            const SceneFrame stone = hopping(onVerge, hop).within(Matrix3x2::Scale(k_size));
            PixelPath rock;
            rock.moveTo(-17.0f, 0.0f);
            rock.cubicTo({ -20.0f, -9.0f }, { -14.0f, -20.0f }, { -3.0f, -22.0f });
            rock.cubicTo({ 7.0f, -24.0f }, { 16.0f, -17.0f }, { 18.0f, -9.0f });
            rock.cubicTo({ 20.0f, -3.0f }, { 18.0f, 0.5f }, { 14.0f, 0.5f });
            rock.close();
            fillShape(stone, rock, LinearGradient::Simple({ -8.0f, -21.0f }, { 10.0f, 0.0f },
                ChaseColors::StoneLight, ChaseColors::StoneDark));
            fillOval(stone, { -5.0f, -17.0f }, 6.5f, 2.6f, SolidColor{ ChaseColors::Shine },
                -0.25f);
            PixelPath crack;
            crack.moveTo(5.0f, -15.0f);
            crack.lineTo(8.0f, -10.0f);
            crack.lineTo(6.0f, -6.0f);
            crack.lineTo(8.5f, -2.5f);
            strokeShape(stone, crack, SolidColor{ ChaseColors::StoneCrack }, 1.0f);
            for (const float tuftX : { -15.0f, 15.0f })
            {
                for (int blade = -1; blade <= 1; ++blade)
                {
                    const float bladeX = static_cast<float>(blade) * 3.0f;
                    const float bladeHeight = 7.0f + 2.0f * static_cast<float>(1 - std::abs(blade));
                    drawSegment(ground, { tuftX, 0.5f }, { tuftX + bladeX, -bladeHeight },
                        SolidColor{ ChaseColors::Tuft }, 1.6f);
                }
            }
        }

        // Three times the size of the daisies in the grass, nodding in the gust. The wolf's wake
        // blows five petals off, which flutter down after him and lie where they land.
        void drawDaisy(const SceneFrame& onVerge, const float gust, const float sinceHareClose)
        {
            constexpr std::array<int, 5> k_blownOff{ 0, 3, 5, 8, 10 };
            // the nod at the height of the wolf's wake, when he hides the head
            constexpr float k_nodBlown = 0.48f;
            constexpr float k_blownAt = 1.12f;
            // a blown petal falls at 18 units a second, gathering 28 units a second each second
            constexpr float k_fallSpeed = 18.0f;
            constexpr float k_fallGathering = 14.0f;
            const float nod = 0.08f + 0.8f * gust;
            const FloatPoint head = { 42.0f * std::sin(nod), -42.0f * std::cos(nod) };
            PixelPath stem;
            stem.moveTo(0.0f, 0.5f);
            stem.quadTo({ 0.0f, -22.0f }, head);
            strokeShape(onVerge, stem, SolidColor{ ChaseColors::FlowerStem }, 1.8f);
            fillOval(onVerge, { -5.0f, -12.0f }, 6.0f, 2.2f, SolidColor{ ChaseColors::Leaf },
                0.5f);
            fillOval(onVerge, { 5.0f + 6.0f * nod, -19.0f }, 6.0f, 2.2f,
                SolidColor{ ChaseColors::Leaf }, nod - 0.5f);
            const SceneFrame bloom = onVerge.within(Matrix3x2::Translation(head)
                * Matrix3x2::Rotation(nod));
            const SolidColor petalColor{ ChaseColors::Petal };
            const FloatPoint headBlown = {
                42.0f * std::sin(k_nodBlown),
                -42.0f * std::cos(k_nodBlown)
            };
            for (int petal = 0; petal != 12; ++petal)
            {
                const float angle = k_2Pi * static_cast<float>(petal) / 12.0f;
                const FloatPoint out = { std::cos(angle), std::sin(angle) };
                const auto blown = std::ranges::find(k_blownOff, petal);
                const float order = static_cast<float>(std::distance(k_blownOff.begin(), blown));
                const float flying = sinceHareClose - (k_blownAt + 0.035f * order);
                if (blown == k_blownOff.end() || flying < 0.0f)
                {
                    fillOval(bloom, out * 6.5f, 4.2f, 1.9f, petalColor, angle);
                    continue;
                }
                // carried after the wolf and fluttering down, it lies flat once it reaches the
                // ground
                const FloatPoint start = headBlown + rotated(out * 6.5f, k_nodBlown);
                const float landing = (-k_fallSpeed + std::sqrt(k_fallSpeed * k_fallSpeed
                    - 4.0f * k_fallGathering * (start.y + 1.0f))) / (2.0f * k_fallGathering);
                const float seconds = std::min(flying, landing);
                const float wobble = 3.0f * std::sin(k_2Pi * 2.0f * seconds + order);
                const FloatPoint at = {
                    start.x + 55.0f * seconds + wobble,
                    start.y + k_fallSpeed * seconds + k_fallGathering * seconds * seconds
                };
                const bool landed = flying >= landing;
                const float spin = angle + k_nodBlown + 4.0f * seconds;
                const float flutter = landed ? 0.55f : std::abs(std::cos(k_2Pi * 1.5f * seconds
                    + order));
                fillOval(onVerge, at, 4.2f, 1.9f * std::max(0.3f, flutter), petalColor,
                    landed ? 0.15f * order - 0.3f : spin);
            }
            fillDisc(bloom, {}, 3.8f, SolidColor{ ChaseColors::FlowerHeart });
        }

        // A post with a board pointing the way they run. The hare's rush turns the board once
        // about the post, the wolf's one and a half times more, so it is left pointing back the
        // way they came. Turning, it is seen edge-on a quarter of the way round and from the back
        // halfway.
        void drawSignpost(const SceneFrame& onVerge, const float sinceHareClose)
        {
            drawSegment(onVerge, { 0.0f, 0.0f }, { 0.0f, -44.0f }, SolidColor{ ChaseColors::Bark },
                4.0f);
            const auto turned = [&](float from, float seconds) {
                const float left = 1.0f - std::clamp((sinceHareClose - from) / seconds, 0.0f, 1.0f);
                return 1.0f - left * left * left;
            };
            const float revolved = k_2Pi * (turned(0.15f, 0.8f) + 1.5f * turned(1.0f, 1.4f));
            const float facing = std::cos(revolved);
            const float across = std::copysign(std::max(std::abs(facing), 0.06f), facing);
            const SceneFrame boardFrame = onVerge.within(Matrix3x2::Scale(across, 1.0f));
            PixelPath board;
            board.moveTo(-12.0f, -40.0f);
            board.lineTo(16.0f, -40.0f);
            board.lineTo(22.0f, -35.0f);
            board.lineTo(16.0f, -30.0f);
            board.lineTo(-12.0f, -30.0f);
            board.close();
            fillShape(boardFrame, board, SolidColor{ ChaseColors::Board });
            const float edgeOn = 1.0f - std::abs(facing);
            const SolidColor shade{ ChaseColors::BoardShade.withOpacity(0.7f * edgeOn) };
            fillShape(boardFrame, board, shade);
            strokeShape(boardFrame, board, SolidColor{ ChaseColors::Bough }, 1.0f);
            // the writing is on the face that looks out at the path
            const float writing = smoothStep(0.3f, 0.6f, facing);
            if (writing > 0.0f)
            {
                const SolidColor ink{ ChaseColors::Bough.withOpacity(writing) };
                drawSegment(boardFrame, { -8.0f, -36.5f }, { 9.0f, -36.5f }, ink, 1.2f);
                drawSegment(boardFrame, { -8.0f, -33.5f }, { 4.0f, -33.5f }, ink, 1.2f);
            }
        }

        // Its cap hops on the stalk after the hare goes by, and off it after the wolf, landing on
        // the ground beside the stalk.
        void drawToadstool(const SceneFrame& onVerge, const float sinceHareClose)
        {
            PixelPath stalk;
            stalk.moveTo(-4.0f, 0.0f);
            stalk.cubicTo({ -3.0f, -6.0f }, { -3.5f, -12.0f }, { -3.0f, -16.0f });
            stalk.lineTo(3.0f, -16.0f);
            stalk.cubicTo({ 3.5f, -12.0f }, { 3.0f, -6.0f }, { 4.0f, 0.0f });
            stalk.close();
            fillShape(onVerge, stalk, LinearGradient::Simple({ -4.0f, 0.0f }, { 4.0f, 0.0f },
                ChaseColors::Stalk, ChaseColors::StalkShade));
            const float pi = std::numbers::pi_v<float>;
            const float onStalk = std::clamp((sinceHareClose - 0.34f) / 0.24f, 0.0f, 1.0f);
            const float offStalk = std::clamp((sinceHareClose - 1.32f) / 0.45f, 0.0f, 1.0f);
            const float settling = std::clamp((sinceHareClose - 1.77f) / 0.15f, 0.0f, 1.0f);
            // on the ground the cap's rim, 14 units up on the stalk, is at the ground
            const FloatPoint capMoved = {
                19.0f * offStalk,
                -10.0f * std::sin(pi * onStalk) + 13.0f * offStalk
                    - 26.0f * std::sin(pi * offStalk) - 3.0f * std::sin(pi * settling)
            };
            const float capTurn = 0.08f * std::sin(pi * onStalk) - 0.3f * offStalk;
            if (offStalk > 0.0f)
            {
                fillOval(onVerge, { capMoved.x + 1.0f, 0.0f }, 13.0f, 2.2f,
                    SolidColor{ ChaseColors::Shadow.withOpacity(offStalk * offStalk) });
            }
            const SceneFrame capFrame = onVerge.within(Matrix3x2::Translation(capMoved)
                * Matrix3x2::Translation(0.0f, -20.0f) * Matrix3x2::Rotation(capTurn)
                * Matrix3x2::Translation(0.0f, 20.0f));
            PixelPath cap;
            cap.moveTo(-15.0f, -14.0f);
            cap.cubicTo({ -15.0f, -31.0f }, { 15.0f, -31.0f }, { 15.0f, -14.0f });
            cap.cubicTo({ 9.0f, -11.5f }, { -9.0f, -11.5f }, { -15.0f, -14.0f });
            cap.close();
            fillShape(capFrame, cap, LinearGradient::Simple({ -6.0f, -28.0f }, { 6.0f, -12.0f },
                ChaseColors::CapLight, ChaseColors::CapDark));
            const SolidColor spot{ ChaseColors::CapSpot };
            fillOval(capFrame, { -8.0f, -20.0f }, 2.6f, 2.0f, spot);
            fillOval(capFrame, { 0.0f, -24.5f }, 2.2f, 1.6f, spot);
            fillOval(capFrame, { 7.5f, -20.0f }, 2.4f, 1.9f, spot);
            fillOval(capFrame, { -2.0f, -17.5f }, 1.5f, 1.2f, spot);
            fillOval(capFrame, { 11.5f, -16.0f }, 1.3f, 1.0f, spot);
        }

        // Four tufts from each head, torn off while the wolf hides the reeds, from where they would
        // be at a lean of 0.5, the height of his wake. They are carried after him, rising and then
        // sinking, and thin out to nothing.
        void drawReedFluff(const SceneFrame& onVerge, const float sinceHareClose)
        {
            constexpr float k_reedsSize = 0.9f;
            constexpr float k_leanTorn = 0.5f;
            constexpr float k_lifeSeconds = 1.6f;
            for (int reed = 0; reed != 3; ++reed)
            {
                const float index = static_cast<float>(reed);
                // the reeds sprite's own sway, at the cycle time the scene draws them at
                const float turn = k_2Pi * 0.08f * std::sin(index - 1.0f) + k_leanTorn;
                const FloatPoint foot = { (12.0f * index - 12.0f) * k_reedsSize, 0.0f };
                for (int tuft = 0; tuft != 4; ++tuft)
                {
                    const int seed = 4 * reed + tuft;
                    const float order = static_cast<float>(seed);
                    const float seconds = sinceHareClose
                        - (1.02f + 0.25f * pseudoRandom(seed, 131.0f));
                    if (seconds < 0.0f || seconds > k_lifeSeconds)
                        continue;
                    const float alongHead = -33.5f + 4.5f * (static_cast<float>(tuft) - 1.5f);
                    const FloatPoint start = foot
                        + rotated({ 0.0f, alongHead * k_reedsSize }, turn);
                    const float speed = 45.0f + 30.0f * pseudoRandom(seed, 132.0f);
                    const float rise = 8.0f + 16.0f * pseudoRandom(seed, 133.0f);
                    const float wobble = 3.0f + 4.0f * pseudoRandom(seed, 134.0f);
                    const FloatPoint at = start + FloatPoint{
                        speed * seconds + wobble * std::sin(k_2Pi * 1.5f * seconds + order),
                        -rise * seconds + 8.0f * seconds * seconds
                    };
                    const float radius = 1.2f + 0.8f * pseudoRandom(seed, 135.0f) + 0.8f * seconds;
                    const SolidColor fluff{ ChaseColors::Fluff.withOpacity(1.0f
                        - seconds / k_lifeSeconds) };
                    fillDisc(onVerge, at, radius, fluff);
                    fillDisc(onVerge, at + FloatPoint{ 0.8f, -0.4f } * radius, 0.7f * radius,
                        fluff);
                    fillDisc(onVerge, at + FloatPoint{ -0.7f, 0.3f } * radius, 0.6f * radius,
                        fluff);
                }
            }
        }

        // A sawn-off stump, its rings showing and a shoot growing from its edge.
        void drawTreeStump(const SceneFrame& onVerge)
        {
            PixelPath trunk;
            trunk.moveTo(-16.0f, 0.5f);
            trunk.quadTo({ -11.0f, -1.0f }, { -11.0f, -6.0f });
            trunk.lineTo(-11.0f, -20.0f);
            trunk.lineTo(11.0f, -20.0f);
            trunk.lineTo(11.0f, -6.0f);
            trunk.quadTo({ 11.0f, -1.0f }, { 16.0f, 0.5f });
            trunk.close();
            fillShape(onVerge, trunk, LinearGradient::Simple({ -11.0f, 0.0f }, { 11.0f, 0.0f },
                ChaseColors::Bark, ChaseColors::BarkShade));
            for (const float grooveX : { -6.0f, -0.5f, 5.0f })
            {
                drawSegment(onVerge, { grooveX, -3.0f }, { grooveX + 0.5f, -18.0f },
                    SolidColor{ ChaseColors::BarkShade }, 1.0f);
            }
            fillOval(onVerge, { 0.0f, -20.0f }, 11.0f, 3.2f, SolidColor{ ChaseColors::StumpTop });
            PixelPath rings;
            addEllipse(rings, { 0.0f, -20.0f }, 7.5f, 2.1f);
            addEllipse(rings, { 0.0f, -20.0f }, 4.0f, 1.1f);
            strokeShape(onVerge, rings, SolidColor{ ChaseColors::StumpRing }, 0.7f);
            fillDisc(onVerge, { 0.3f, -20.0f }, 0.8f, SolidColor{ ChaseColors::StumpRing });
            drawSegment(onVerge, { 8.0f, -21.0f }, { 10.5f, -28.0f },
                SolidColor{ ChaseColors::FlowerStem }, 1.0f);
            fillOval(onVerge, { 12.5f, -28.5f }, 3.0f, 1.4f, SolidColor{ ChaseColors::Leaf },
                -0.4f);
        }

        // A sparrow pecking at the verge; taking fright, it climbs away ahead of them, its wing
        // beating up over its back and down under its belly.
        void drawBird(const SceneFrame& onVerge, const float seconds, const float sinceHareClose)
        {
            const float flying = std::max(0.0f, sinceHareClose);
            const FloatPoint away = {
                420.0f * flying,
                -(110.0f * flying + 170.0f * flying * flying)
            };
            const float climb = -0.35f * std::min(1.0f, flying * 6.0f);
            const SceneFrame bird = onVerge.within(Matrix3x2::Translation(away)
                * Matrix3x2::Translation(0.0f, -4.0f) * Matrix3x2::Rotation(climb));
            const SolidColor body{ ChaseColors::BirdBody };
            const SolidColor wing{ ChaseColors::BirdWing };
            const float headDown = sinceHareClose > 0.0f
                ? 0.0f
                : bump(fractionalPart(seconds / 0.6f), 0.5f, 0.15f);
            if (sinceHareClose <= 0.0f)
            {
                const SolidColor legs{ ChaseColors::BirdBeak };
                drawSegment(onVerge, { -0.5f, -5.0f }, { -1.0f, 0.0f }, legs, 0.8f);
                drawSegment(onVerge, { 1.5f, -5.0f }, { 1.5f, 0.0f }, legs, 0.8f);
            }
            PixelPath tail;
            tail.moveTo(-5.0f, -5.0f);
            tail.lineTo(-13.0f, -8.5f);
            tail.lineTo(-12.0f, -4.0f);
            tail.close();
            fillShape(bird, tail, wing);
            fillOval(bird, { 0.0f, -4.0f }, 7.0f, 4.8f, body, -0.15f);
            fillOval(bird, { 1.5f, -2.5f }, 4.5f, 2.8f, SolidColor{ ChaseColors::BirdBelly });
            const FloatPoint headMiddle = { 6.0f + 2.0f * headDown, -9.0f + 5.0f * headDown };
            fillDisc(bird, headMiddle, 3.6f, body);
            fillDisc(bird, headMiddle + FloatPoint{ 1.2f, -0.6f }, 0.9f,
                SolidColor{ ChaseColors::Ink });
            PixelPath beak;
            beak.moveTo(headMiddle + FloatPoint{ 3.2f, -0.6f });
            beak.lineTo(headMiddle + FloatPoint{ 6.4f, 0.4f + 1.5f * headDown });
            beak.lineTo(headMiddle + FloatPoint{ 3.2f, 0.9f });
            beak.close();
            fillShape(bird, beak, SolidColor{ ChaseColors::BirdBeak });
            if (sinceHareClose <= 0.0f)
            {
                fillOval(bird, { -1.5f, -5.0f }, 5.0f, 2.6f, wing, -0.25f);
                return;
            }
            // nine beats a second, each a full sweep from over its back to under its belly
            const float span = 10.0f * std::cos(k_2Pi * 9.0f * flying);
            fillOval(bird, { -1.0f, -6.0f - 0.5f * span }, 3.2f,
                std::max(0.8f, 0.5f * std::abs(span)), wing, -0.3f);
        }

        // Resting on a tuft, it opens and closes its wings; taking fright, it flutters up and
        // drifts off ahead of them.
        void drawButterfly(const SceneFrame& onVerge, const float seconds,
            const float sinceHareClose)
        {
            for (int blade = -1; blade <= 1; ++blade)
            {
                const float bladeX = static_cast<float>(blade);
                drawSegment(onVerge, { 1.5f * bladeX, 0.5f },
                    { 3.5f * bladeX, -7.0f - 2.0f * (1.0f - std::abs(bladeX)) },
                    SolidColor{ ChaseColors::Tuft }, 1.4f);
            }
            const float flying = std::max(0.0f, sinceHareClose);
            const FloatPoint away = {
                70.0f * flying + 6.0f * std::sin(k_2Pi * 1.1f * flying),
                -(55.0f * flying + 25.0f * std::min(1.0f, 4.0f * flying))
            };
            const float open = sinceHareClose > 0.0f
                ? std::abs(std::cos(k_2Pi * 8.0f * flying))
                : 0.55f + 0.45f * std::cos(k_2Pi * 0.7f * seconds);
            const SceneFrame body = onVerge.within(Matrix3x2::Translation(FloatPoint{ 0.0f, -9.0f }
                + away) * Matrix3x2::Rotation(0.15f * std::sin(k_2Pi * 1.3f * flying)));
            const SolidColor edge{ ChaseColors::WingEdge };
            for (const float side : { -1.0f, 1.0f })
            {
                const SceneFrame wings = body.within(Matrix3x2::Scale(side * (0.2f + 0.8f * open),
                    1.0f));
                PixelPath forewing;
                forewing.moveTo(0.4f, -7.0f);
                forewing.cubicTo({ 3.0f, -15.0f }, { 11.0f, -15.5f }, { 10.5f, -9.5f });
                forewing.cubicTo({ 10.0f, -6.5f }, { 4.0f, -5.0f }, { 0.4f, -5.0f });
                forewing.close();
                PixelPath hindwing;
                hindwing.moveTo(0.4f, -4.5f);
                hindwing.cubicTo({ 5.5f, -5.5f }, { 8.5f, -1.0f }, { 6.0f, 1.0f });
                hindwing.cubicTo({ 4.0f, 2.5f }, { 1.0f, 0.0f }, { 0.4f, -2.0f });
                hindwing.close();
                for (const PixelPath* wing : { &hindwing, &forewing })
                {
                    fillShape(wings, *wing, SolidColor{ ChaseColors::Wing });
                    strokeShape(wings, *wing, edge, 0.7f);
                }
                fillOval(wings, { 8.0f, -12.3f }, 2.3f, 1.5f, edge, -0.4f);
                fillDisc(wings, { 8.3f, -12.4f }, 0.6f, SolidColor{ ChaseColors::Petal });
            }
            fillOval(body, { 0.0f, -5.5f }, 1.1f, 5.0f, edge);
            fillDisc(body, { 0.0f, -11.2f }, 1.3f, edge);
            for (const float side : { -1.0f, 1.0f })
            {
                const FloatPoint knob = { 2.2f * side, -16.5f };
                drawSegment(body, { 0.0f, -12.0f }, knob, edge, 0.5f);
                fillDisc(body, knob, 0.6f, edge);
            }
        }

        // Sitting by the path; taking fright, it hops twice back into the grass, smaller each
        // time as it goes.
        void drawFrog(const SceneFrame& onVerge, const float sinceHareClose)
        {
            struct Spot
            {
                FloatPoint at;
                float size;
            };
            constexpr std::array<Spot, 3> k_spots{
                Spot{ { 0.0f, 0.0f }, 1.0f },
                Spot{ { 14.0f, -7.0f }, 0.78f },
                Spot{ { 24.0f, -13.0f }, 0.6f }
            };
            constexpr float k_hopSeconds = 0.25f;
            Spot spot = k_spots.back();
            float stretch = 0.0f;
            float lift = 0.0f;
            for (std::size_t hop = 0; hop + 1 != k_spots.size(); ++hop)
            {
                const float takeoff = 0.05f + 0.32f * static_cast<float>(hop);
                const float progress = (sinceHareClose - takeoff) / k_hopSeconds;
                if (progress >= 1.0f)
                    continue;
                const float hopping = std::max(0.0f, progress);
                const Spot& from = k_spots[hop];
                const Spot& to = k_spots[hop + 1];
                spot = {
                    from.at + (to.at - from.at) * hopping,
                    std::lerp(from.size, to.size, hopping)
                };
                stretch = std::sin(std::numbers::pi_v<float> * hopping);
                lift = 16.0f * from.size * stretch;
                break;
            }
            fillOval(onVerge, spot.at + FloatPoint{ 1.0f, 0.0f }, 9.0f * spot.size,
                2.2f * spot.size, SolidColor{ ChaseColors::Shadow });
            const SceneFrame frog = onVerge.within(Matrix3x2::Translation(spot.at.x,
                spot.at.y - lift) * Matrix3x2::Scale(spot.size)
                * Matrix3x2::Translation(0.0f, -6.0f) * Matrix3x2::Rotation(-0.45f * stretch)
                * Matrix3x2::Translation(0.0f, 6.0f));
            const SolidColor dark{ ChaseColors::FrogDark };
            const FloatPoint hindFoot = FloatPoint{ -2.0f, -0.3f }
                + (FloatPoint{ -15.0f, 3.0f } - FloatPoint{ -2.0f, -0.3f }) * stretch;
            const FloatPoint foreFoot = FloatPoint{ 7.5f, 0.0f }
                + (FloatPoint{ 13.0f, -3.0f } - FloatPoint{ 7.5f, 0.0f }) * stretch;
            drawSegment(frog, { -5.0f, -5.0f }, hindFoot, dark, 2.6f);
            fillOval(frog, hindFoot, 3.5f, 1.1f, dark);
            fillOval(frog, { -5.0f, -4.5f }, 5.5f, 4.2f * (1.0f - 0.4f * stretch), dark, -0.3f);
            fillOval(frog, { 1.0f, -8.0f }, 8.5f, 6.2f, SolidColor{ ChaseColors::FrogLight },
                -0.4f);
            fillOval(frog, { 4.0f, -5.5f }, 4.5f, 2.6f, SolidColor{ ChaseColors::FrogBelly },
                -0.4f);
            drawSegment(frog, { 5.0f, -5.0f }, foreFoot, dark, 2.0f);
            fillOval(frog, foreFoot + FloatPoint{ 0.8f, 0.0f }, 2.2f, 0.9f, dark);
            fillDisc(frog, { 5.5f, -13.0f }, 3.2f, SolidColor{ ChaseColors::FrogLight });
            fillDisc(frog, { 6.0f, -13.5f }, 2.2f, SolidColor{ ChaseColors::EyeShine });
            fillDisc(frog, { 6.7f, -13.4f }, 1.1f, SolidColor{ ChaseColors::Ink });
            drawSegment(frog, { 6.5f, -9.5f }, { 10.5f, -10.8f }, dark, 0.7f);
        }

        // Out of its shell, it draws in as the hare comes, and out again once the wolf is by.
        void drawSnail(const SceneFrame& onVerge, const float sinceHareClose)
        {
            const float drawnIn = smoothStep(0.0f, 0.15f, sinceHareClose)
                * (1.0f - smoothStep(1.8f, 2.6f, sinceHareClose));
            const float out = 1.0f - drawnIn;
            const SolidColor body{ ChaseColors::SnailBody };
            fillOval(onVerge, { std::lerp(-1.0f, 2.0f, out), -2.0f }, std::lerp(6.0f, 12.0f, out),
                std::lerp(1.8f, 2.6f, out), body);
            if (out > 0.05f)
            {
                const FloatPoint head = { std::lerp(3.0f, 12.0f, out), -5.5f };
                fillOval(onVerge, head, 2.8f * out, 3.5f * out, body, 0.35f);
                for (const float stalkX : { -1.0f, 1.2f })
                {
                    const FloatPoint root = head + FloatPoint{ stalkX, -2.5f * out };
                    const FloatPoint eye = root + FloatPoint{ 1.2f * stalkX + 1.0f, -6.0f } * out;
                    drawSegment(onVerge, root, eye, body, 0.9f);
                    fillDisc(onVerge, eye, 0.9f * out, SolidColor{ ChaseColors::ShellLine });
                }
            }
            const FloatPoint shellMiddle = { -2.0f, -9.0f + 1.2f * drawnIn };
            fillDisc(onVerge, shellMiddle, 8.0f, LinearGradient::Simple(shellMiddle
                + FloatPoint{ -5.0f, -6.0f }, shellMiddle + FloatPoint{ 5.0f, 6.0f },
                ChaseColors::ShellLight, ChaseColors::ShellDark));
            PixelPath spiral;
            for (int step = 0; step <= 40; ++step)
            {
                const float along = static_cast<float>(step) / 40.0f;
                const float angle = 2.25f * k_2Pi * along;
                const FloatPoint point = shellMiddle
                    + FloatPoint{ std::cos(angle), std::sin(angle) } * (1.0f + 6.0f * along);
                if (step == 0)
                    spiral.moveTo(point);
                else
                    spiral.lineTo(point);
            }
            strokeShape(onVerge, spiral, SolidColor{ ChaseColors::ShellLine }, 1.0f);
        }

        // A clump of tall grass on the trail, the back blades darker. Each blade bends from its
        // root, its tip leaning furthest.
        void drawGrassPatch(const SceneFrame& onTrail, const float bladeLean)
        {
            struct Blade
            {
                float root;
                float height;
                float tilt;
                Color color;
            };
            constexpr std::array<Blade, 11> k_blades{
                Blade{ -12.0f, 22.0f, -0.35f, ChaseColors::BladeDark },
                Blade{ -4.0f, 30.0f, -0.1f, ChaseColors::BladeDark },
                Blade{ 5.0f, 27.0f, 0.15f, ChaseColors::BladeDark },
                Blade{ 13.0f, 21.0f, 0.4f, ChaseColors::BladeDark },
                Blade{ -15.0f, 17.0f, -0.55f, ChaseColors::BladeMiddle },
                Blade{ -8.0f, 25.0f, -0.25f, ChaseColors::BladeMiddle },
                Blade{ 1.0f, 33.0f, 0.0f, ChaseColors::BladeMiddle },
                Blade{ 9.0f, 26.0f, 0.25f, ChaseColors::BladeMiddle },
                Blade{ -10.0f, 18.0f, -0.4f, ChaseColors::BladeLight },
                Blade{ -1.0f, 23.0f, -0.05f, ChaseColors::BladeLight },
                Blade{ 7.0f, 19.0f, 0.3f, ChaseColors::BladeLight }
            };
            for (const Blade& blade : k_blades)
            {
                // the taller a blade, the further the wolf bends it
                const float lean = blade.tilt + bladeLean * (0.7f + blade.height / 100.0f);
                const FloatPoint up = { std::sin(lean), -std::cos(lean) };
                const FloatPoint halfway = { std::sin(0.5f * lean), -std::cos(0.5f * lean) };
                const FloatPoint root = { blade.root, 1.0f };
                const FloatPoint tip = root + up * blade.height;
                PixelPath shape;
                shape.moveTo(root - FloatPoint{ 2.2f, 0.0f });
                shape.quadTo(root + halfway * (0.6f * blade.height) - FloatPoint{ 0.6f, 0.0f },
                    tip);
                shape.quadTo(root + halfway * (0.55f * blade.height) + FloatPoint{ 1.2f, 0.0f },
                    root + FloatPoint{ 2.2f, 0.0f });
                shape.close();
                fillShape(onTrail, shape, SolidColor{ blade.color });
            }
        }

        void drawRake(const SceneFrame& onTrail, const float swing)
        {
            const FloatPoint pivot = { -4.0f, -2.0f };
            const SceneFrame rake = onTrail.within(Matrix3x2::Translation(pivot)
                * Matrix3x2::Rotation(-swing) * Matrix3x2::Translation(-pivot.x, -pivot.y));
            fillRoundedBox(rake, { -4.0f, -4.0f, 106.0f, -1.0f }, 1.5f,
                LinearGradient::Simple({ 0.0f, -4.0f }, { 0.0f, -1.0f }, ChaseColors::RakeHandle,
                ChaseColors::RakeHandleShade));
            PixelPath tines;
            for (int tine = 0; tine != 6; ++tine)
            {
                const float x = -12.0f + 2.4f * static_cast<float>(tine);
                tines.moveTo(x, -3.0f);
                tines.lineTo(x - 0.6f, -10.0f);
            }
            strokeShape(rake, tines, SolidColor{ ChaseColors::RakeMetal }, 1.3f);
            fillRoundedBox(rake, { -13.0f, -4.5f, -1.0f, -1.5f }, 1.0f,
                SolidColor{ ChaseColors::RakeMetal });
        }

        // A tree whose lowest branch reaches back over the trail, as high as the head of a wolf in
        // the middle of a leap; `x` is where it ends.
        void drawBranchTree(const SceneFrame& frame, const float x, const float groundY,
            const float footY, const float branchSway, const float sinceHit,
            const bool hareHasApple)
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
            fillShape(frame, branch, SolidColor{ ChaseColors::Bough });
            fillDisc(frame, { x + 16.0f * unit, footY - 153.0f * unit + 0.8f * endSway },
                9.0f * unit, SolidColor{ ChaseColors::CrownLight });
            fillDisc(frame, { trunkX, groundY - 196.0f * unit }, 60.0f * unit,
                SolidColor{ ChaseColors::Crown });
            fillDisc(frame, { trunkX - 40.0f * unit, groundY - 170.0f * unit }, 40.0f * unit,
                SolidColor{ ChaseColors::CrownLight });
            fillDisc(frame, { trunkX + 40.0f * unit, groundY - 172.0f * unit }, 44.0f * unit,
                SolidColor{ ChaseColors::CrownMiddle });

            // Shaken loose, an apple falls, bounces a third as high as its speed allows and lies
            // where it lands.
            for (std::size_t index = 0; index != k_treeApples.size(); ++index)
            {
                if (hareHasApple && index == k_hareApple)
                    continue;
                const TreeApple& apple = k_treeApples[index];
                FloatPoint middle = apple.onCrown;
                float spin = 0.0f;
                const float seconds = (sinceHit - apple.lettingGo) * k_episodeSeconds;
                if (apple.falls && seconds > 0.0f)
                {
                    const float ground = restingPlace(apple).y;
                    const float drop = ground - apple.onCrown.y;
                    const float fallSeconds = std::sqrt(2.0f * drop / k_appleGravity);
                    const float bounceSpeed = 0.3f * k_appleGravity * fallSeconds;
                    const float bounceSeconds = 2.0f * bounceSpeed / k_appleGravity;
                    if (seconds < fallSeconds)
                    {
                        middle.y += 0.5f * k_appleGravity * seconds * seconds;
                    }
                    else
                    {
                        const float sinceLanding = std::min(seconds - fallSeconds, bounceSeconds);
                        middle.y = ground - bounceSpeed * sinceLanding
                            + 0.5f * k_appleGravity * sinceLanding * sinceLanding;
                    }
                    const float moving = std::min(1.0f, seconds / (fallSeconds + bounceSeconds));
                    middle.x += apple.drift * moving;
                    spin = 2.5f * moving * (apple.drift < 0.0f ? -1.0f : 1.0f);
                }
                const FloatPoint onScreen = {
                    trunkX + middle.x * unit,
                    groundY + middle.y * unit
                };
                drawApple(frame.within(Matrix3x2::Translation(onScreen)
                    * Matrix3x2::Rotation(spin) * Matrix3x2::Scale(k_appleScale * unit)
                    * Matrix3x2::Translation(0.0f, k_appleMiddle)), 0.0f);
            }
        }

        FloatPoint restingPlace(const TreeApple& apple)
        {
            return { apple.onCrown.x + apple.drift, apple.landing - k_appleMiddle * k_appleScale };
        }
    }
}
