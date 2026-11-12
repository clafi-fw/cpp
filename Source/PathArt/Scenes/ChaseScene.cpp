module ClaFi.PathArt.ChaseScene;

import ClaFi.PathArt.DazeStarsEffect;
import ClaFi.PathArt.DustPuffEffect;
import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.HareSprite;
import ClaFi.PathArt.LightningEffect;
import ClaFi.PathArt.ReedsSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.TrunkSprite;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WeatherEffect;
import ClaFi.PathArt.WolfSprite;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;
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
        constexpr Color Shine{ 0x99FFFFFF };
        constexpr Color EyeShine{ 0xFFFFFFFF };
        constexpr Color Puff{ 0xB3FFFFFF };
        constexpr Color BallEdge{ 0x40000000 };
        constexpr Color Ink{ 0xFF1B1D22 };
        constexpr Color DazeStar{ 0xFFFFD34D };
        constexpr Color DazeStarEdge{ 0xFF83868C };
    }

    // The two who run the chase in daylight, as the icon draws them in every theme.
    namespace ChaseCoats
    {
        constexpr WolfCoat GreyWolf{
            .light = Color{ 0xFF7D8592 },
            .shade = Color{ 0xFF59606B },
            .coat = Color{ 0xFF6C7480 },
            .far = Color{ 0xFF4F565F },
            .dark = Color{ 0xFF3D434C },
            .belly = Color{ 0xE6B3B9C2 },
            .tongue = Color{ 0xFFEF6F7F },
            .mouth = Color{ 0xFF5A1C28 },
            .tooth = Color{ 0xFFFBF6E8 },
            .toothEdge = Color{ 0xFF9C9488 }
        };
        constexpr HareCoat BrownHare{
            .light = Color{ 0xFFC9935E },
            .fur = Color{ 0xFFB98552 },
            .shade = Color{ 0xFFA87444 },
            .belly = Color{ 0xE6F0DFC0 },
            .tail = Color{ 0xFFFFFFFF },
            .earInside = Color{ 0xFFF2A7A0 },
            .nose = Color{ 0xFFD9787A },
            .whisker = Color{ 0x805B3A1E },
            .mouth = Color{ 0xFF5A2A26 },
            .tooth = Color{ 0xFFFFFFFF },
            .toothEdge = Color{ 0xFFA89E94 }
        };
    }

    namespace
    {
        // The theme's stars twinkle and its weather comes round twice a cycle, every 30 seconds,
        // near the mill's 40.
        constexpr float k_lively{ 2.0f };
        // Distance beyond the trail, in the trail's own, at which a layer is half skyBottom.
        constexpr float k_halfHaze{ 2.46f };
        // A layer moving at `parallax` of the camera's speed, seen through the haze. See PathArt
        [[nodiscard]] Color hazed(const SceneTheme&, Color clear, float parallax);
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
        // The camera's travel by `episodeTime` of an episode, as a share of a full-speed episode's.
        [[nodiscard]] float cameraShare(float episodeTime, int episode);
        // When the wolf comes to rest from his fall.
        constexpr float k_fallEnd{ 0.62f };
        // The camera stands from the stop to here, then speeds up again until k_cameraRunning.
        constexpr float k_cameraWaiting{ 0.82f };
        constexpr float k_cameraRunning{ 0.9f };
        // The trail runs this many units below the ground line, and the wolf is drawn at this many
        // pixels to the frame's unit.
        constexpr float k_trailBelowGround{ 22.0f };
        constexpr float k_wolfScale{ 0.95f };
        // Where the wolf and the hare run, in units from the middle of the frame.
        constexpr float k_wolfChasing{ -200.0f };
        constexpr float k_hareRunning{ 150.0f };
        // The wolf's middle behind the hare's feet as he trips, his nose half a hare short of her
        // tail.
        constexpr float k_wolfBehindHareAtTrip{ 150.0f };
        // In the first run of a cycle he lunges this many units at the hare's tail once, this
        // share of the run in, and snaps his jaws shut on nothing.
        constexpr float k_firstRunLunge{ 60.0f };
        constexpr float k_firstRunSnap{ 0.5f };
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
        // From the branch's end to the trunk's middle and to its near side, in units of the frame.
        constexpr float k_trunkFromBranchEnd{ 125.0f };
        constexpr float k_trunkHalfWidth{ 14.0f };
        // Over the branch he leaps this high, tops out this soon and lands this far past the
        // branch's end, where that tree's trunk stands this far back and its branch reaches on.
        constexpr float k_leapHeightOverBranch{ 150.0f };
        constexpr float k_leapTopOverBranch{ 0.572f };
        constexpr float k_wolfPastBranchEnd{ 30.0f };
        constexpr float k_wolfPastTrunk{ 75.0f };
        // The crow's stump stands this far past the branch's end, the roadside stump at this
        // size, and the crow takes off from it this many seconds before his face meets it.
        constexpr float k_crowStumpFromBranchEnd{ 44.0f };
        constexpr float k_crowStumpSize{ 1.8f };
        constexpr float k_crowTakeoff{ -0.4f };
        // Met, the crow's eyes pop wide - past full size in k_crowEyesPop seconds, settled by
        // k_crowEyesSettled - and come back to size as the k_crowShaken seconds of its shaking end.
        constexpr float k_crowEyesPop{ 0.08f };
        constexpr float k_crowEyesPopOver{ 0.35f };
        constexpr float k_crowEyesSettled{ 0.2f };
        constexpr float k_crowShaken{ 1.0f };

        // How the wolf's leap at the hare ends in the branch run, a different way each cycle.
        enum class LeapEnd
        {
            Branch, // the low branch meets his face
            Trunk, // he flies into the trunk, slides down it and topples onto his back
            OverBranch, // he leaps too high, lands on the branch past the trunk and slips off it
            Crow, // no tree stands there, and a crow flying the other way meets his face
            OtherWolf // another couple runs the other way, and the two wolves meet face to face
        };
        // What the tree lets fall as he hits it; the hare's apple falls every time, and the two at
        // the top of the crown never do.
        enum class AppleFall
        {
            Four, // the four nearest the trail
            Six // all but the top two
        };
        // Where the crow is as the two come: flying at them, or sitting on a stump by the trail
        // until they frighten it up into his leap.
        enum class CrowStart
        {
            Flying,
            OnStump
        };
        // The branch run of a cycle. How the leap ends goes round in a turn five cycles long, and
        // what falls, the crow's start and which hare leaps change from one turn to the next.
        struct TreeRun
        {
            LeapEnd leapEnd;
            AppleFall appleFall;
            CrowStart crowStart;
            bool ourHareLeaps; // meeting the other couple, ours bounds over their hare, not under
        };
        constexpr std::array<LeapEnd, 5> k_leapEnds{
            LeapEnd::Branch,
            LeapEnd::Trunk,
            LeapEnd::OverBranch,
            LeapEnd::Crow,
            LeapEnd::OtherWolf
        };
        constexpr std::array<AppleFall, 2> k_appleFalls{
            AppleFall::Four,
            AppleFall::Six
        };
        constexpr std::array<CrowStart, 2> k_crowStarts{
            CrowStart::Flying,
            CrowStart::OnStump
        };
        [[nodiscard]] TreeRun treeRunOf(std::int64_t cyclesPassed);
        // From the branch's end to the trunk's middle of the run's tree, in units of the frame.
        [[nodiscard]] float trunkFromBranchEnd(LeapEnd);
        // Whether an apple tree stands where the leap ends.
        [[nodiscard]] bool treeStands(LeapEnd);

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
        [[nodiscard]] BranchRun branchRunAt(float episodeTime, float closeness, LeapEnd);
        // His jaws open as he lunges at her tail and shut at the end of the lunge, at `snapAt`.
        [[nodiscard]] float chompAt(float episodeTime, float snapAt);
        // Where the branch's end is as the wolf's leap ends, in units from the middle of the frame;
        // for the crow, where it meets him.
        [[nodiscard]] float branchEndAtContact(float closeness, LeapEnd);

        // The wolf from where his leap ends in the branch run: down, lying dazed, up and back.
        struct TreeFall
        {
            float x; // units from the middle of the frame
            float lift;
            float turn;
            float leap;
            float jawOpen;
            float sprawl;
            float onBack;
            float dangle;
            float dazed;
            float running;
            float branchWeight; // units his weight bends the branch's end down
        };
        [[nodiscard]] TreeFall treeFallAt(float episodeTime, float closeness, LeapEnd);
        // When he comes to rest on his back in the branch run.
        [[nodiscard]] float treeRestAt(LeapEnd);
        // Where the hare and the wolf run by `episodeTime` of an episode, in units from the middle
        // of the frame.
        [[nodiscard]] float hareXAt(int episode, float episodeTime, float closeness,
            const TreeRun&);
        [[nodiscard]] float wolfXAt(int episode, float episodeTime, float closeness, LeapEnd);
        // The other couple are ours turned about the point on the ground where the wolves meet, so
        // the two hares cross there at one moment, when this many shares of the run have passed.
        [[nodiscard]] float haresCrossAt(float closeness, const TreeRun&);
        // For this many shares of the run either side of it, one hare bounds over the other.
        constexpr float k_haresPassing{ 0.012f };
        // Once the wolves are down, the other couple's hare runs this many shares of the run ahead
        // of ours and their wolf this many behind.
        constexpr float k_theirHareAhead{ 0.008f };
        constexpr float k_theirWolfBehind{ 0.012f };

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
            Snail,
            CrowOnStump // a crow on a tree stump, which gets away before they reach it
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
        // What stands by the path stirs as each of the two comes within this many units of it.
        constexpr float k_stirringReach{ 70.0f };
        // Which edge of the trail something by the path stands on.
        enum class Verge
        {
            Far,
            Near
        };
        // The near verge's own order, one at each of its places along the trail in turn.
        constexpr std::array<Roadside, 13> k_nearRoadside{
            Roadside::Snail,
            Roadside::Boulder,
            Roadside::Daisy,
            Roadside::Frog,
            Roadside::CrowOnStump,
            Roadside::Signpost,
            Roadside::Apple,
            Roadside::Butterfly,
            Roadside::TreeStump,
            Roadside::BananaPeel,
            Roadside::Reeds,
            Roadside::Bird,
            Roadside::Toadstool
        };
        // The near verge has twice the golden ratio's square of places in a cycle's travel against
        // the far verge's six, so the two never fall into step.
        constexpr double k_nearRoadsidesPerCycle{ 5.23606797749979 };
        constexpr double k_nearRoadsideOffset{ 0.5 }; // of a spacing, from the start of the trail
        // The far verge's places between those of its runs, in an order of their own: three in a
        // cycle's travel, the first this share of a spacing from the start of the trail, which sets
        // one about where each run starts.
        constexpr std::array<Roadside, 13> k_farBetweenRoadside{
            Roadside::Daisy,
            Roadside::CrowOnStump,
            Roadside::Boulder,
            Roadside::Toadstool,
            Roadside::Snail,
            Roadside::Signpost,
            Roadside::Reeds,
            Roadside::Frog,
            Roadside::Butterfly,
            Roadside::TreeStump,
            Roadside::Apple,
            Roadside::Bird,
            Roadside::BananaPeel
        };
        constexpr double k_farBetweenPerCycle{ 3.0 };
        constexpr double k_farBetweenOffset{ 0.0 };
        // Neither row has a place within this many units of where the camera stands in a run, so
        // nothing by the path crowds what the wolf runs into there.
        constexpr double k_collisionStretchReach{ 700.0 };
        // A near place that would stand within this many units of one on the far verge moves on
        // to stand this far past it.
        constexpr double k_nearClearOfFar{ 500.0 };
        // Units below the ground line each verge is, and the near one's size against the far's.
        constexpr float k_farVergeDrop{ 10.0f };
        constexpr float k_nearVergeDrop{ 38.0f };
        constexpr float k_nearVergeSize{ 1.15f };
        // Seconds after the hare that the wolf comes as close to the far verge's thing in a run
        // over the trail, which the timing of his part of the stirring is stated against.
        constexpr float k_wolfAfterHare{ 0.89f };

        // How the rubber duck stands at one moment.
        struct DuckMotion
        {
            float deflation{ 0.0f }; // 0 full of air, 1 flat, below 0 swelling past full
            float airOut{ 0.0f }; // opacity of the air puffing out of it
            float legs{ 0.0f }; // share of its legs' length grown, past 1 as they pop out
            float strides{ 0.0f }; // radians of its walk, 0 standing
        };

        // How what is on the trail moves at one moment.
        struct ObstacleMotion
        {
            float branchSway; // units the branch's end is pushed down, up below 0
            float curl; // 0 the hedgehog on its feet, 1 rolled into a ball
            float steps; // radians of the hedgehog's walk
            DuckMotion duck;
            float rollAngle; // radians the ball has turned through
            float bladeLean; // radians the grass leans forward from where it stands
            float peelLift; // units the banana peel is in the air
            float peelSpin; // radians the banana peel has turned through
            float rakeSwing; // radians the rake's handle has swung up off the trail
            float bootTip; // radians the boot has tipped over, away from the wolf
            float sinceHit; // shares of the episode since the wolf hit the branch, below 0 before
            float appleSplit; // 0 the apple whole, 1 in two halves lying apart
            float crownShake; // units the tree's crown is shaken sideways, flown into
            bool hareHasApple; // the hare has taken her apple off the ground
            TreeRun treeRun; // the branch run's leap end, apple fall and crow start
        };
        // The hedgehog walks off at 160 units a second, which is this many in an episode's time.
        constexpr float k_hedgehogWalk{ 3200.0f };
        constexpr float k_episodeSeconds{ 20.0f };
        // When the duck, flat since the trip, fills up again, grows legs and sets off.
        constexpr float k_duckRefills{ 0.65f };
        constexpr float k_duckGrowsLegs{ 0.668f };
        constexpr float k_duckSetsOff{ 0.678f };
        // It walks off away from the wolf at 240 units a second, which is this many in an
        // episode's time, on legs this long, covering this many units in a step of each foot.
        constexpr float k_duckWalk{ 4800.0f };
        constexpr float k_duckLegLength{ 11.0f };
        constexpr float k_duckStride{ 26.0f };

        // An apple on the tree, in units from the foot of its trunk; one that falls lets go that
        // long after the wolf hits the tree, and drifts that far as it lands.
        struct TreeApple
        {
            FloatPoint onCrown;
            bool amongFour; // falls in AppleFall::Four
            bool amongSix; // falls in AppleFall::Six
            float lettingGo;
            float drift;
            float landing; // units below the ground line, where the trail is
        };
        constexpr std::array<TreeApple, 8> k_treeApples{
            TreeApple{ { -58.0f, -158.0f }, true, true, 0.0f, -14.0f, 18.0f },
            TreeApple{ { -22.0f, -214.0f }, false, false, 0.0f, 0.0f, 0.0f },
            TreeApple{ { 18.0f, -232.0f }, false, false, 0.0f, 0.0f, 0.0f },
            TreeApple{ { 30.0f, -188.0f }, true, true, 0.003f, 10.0f, 22.0f },
            TreeApple{ { 62.0f, -150.0f }, true, true, 0.006f, 16.0f, 15.0f },
            TreeApple{ { -10.0f, -160.0f }, true, true, 0.0015f, -6.0f, 26.0f },
            TreeApple{ { -46.0f, -196.0f }, false, true, 0.0045f, 8.0f, 20.0f },
            TreeApple{ { 74.0f, -182.0f }, false, true, 0.0075f, -10.0f, 26.0f }
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
        void drawObstacle(const SceneFrame&, const SceneTheme&, Obstacle, float x, float groundY,
            float footY, const ObstacleMotion&);
        // Each is drawn in units about the middle of where it meets the trail, facing the wolf.
        void drawBananaPeel(const SceneFrame& onTrail, const ChaseTheme&, float lift, float spin);
        // Splitting, its halves lean apart and turn their flesh toward the viewer.
        void drawApple(const SceneFrame& onTrail, const ChaseTheme&, float split);
        // Tipping, it turns about the back of its heel.
        void drawBoot(const SceneFrame& onTrail, const ChaseTheme&, float tip);
        void drawHedgehog(const SceneFrame& onTrail, const ChaseTheme&, float curl, float steps);
        void drawRubberDuck(const SceneFrame& onTrail, const ChaseTheme&, const DuckMotion&);
        void drawBall(const SceneFrame& onTrail, const ChaseTheme&, float rollAngle);
        void drawGrassPatch(const SceneFrame& onTrail, const ChaseTheme&, float bladeLean);
        // Lying tines up, its head toward the wolf; its handle swings up about the head.
        void drawRake(const SceneFrame& onTrail, const ChaseTheme&, float swing);
        void drawBranchTree(const SceneFrame&, const SceneTheme&, float x, float groundY,
            float footY, const ObstacleMotion&);
        // `x` is where it meets the wolf's face.
        void drawCrow(const SceneFrame&, const ChaseTheme&, float x, float groundY, float footY,
            float sinceHit, CrowStart);
        // How wide the crow's eyes are `seconds` after his face met it: popping, then settling.
        [[nodiscard]] float crowEyesWideAt(float seconds);
        // Folded, its wings lie along its back and `flap` is not used; `eyesWide` is 1 when wide.
        void drawCrowBody(const SceneFrame& crow, const ChaseTheme&, float flap, float eyesWide,
            bool folded);
        // The stump the crow sits on, on the far verge under where it meets his face.
        void drawCrowStump(const SceneFrame&, const ChaseTheme&, float x, float groundY);
        // How what stands by the path moves as the two run past it.
        struct RoadsideMotion
        {
            float gust; // radians the wind of their passing bends reeds and the daisy after them
            // Seconds since the hare came within k_stirringReach, below 0 before.
            float sinceHareClose;
            // sinceHareClose as it would read with the wolf k_wolfAfterHare behind her.
            float wolfTime;
            float seconds; // a running clock, for the bird's pecking and the butterfly's wings
        };
        // The wind behind each of them as they pass bends it after them, the wolf's more; `x` is
        // where it stands, in units from the middle of the frame like the two.
        [[nodiscard]] float wakeGust(float x, float hareX, float wolfX);
        // The theme's three petal colours by turns, for the flowers along a row.
        [[nodiscard]] Color petalOf(const SceneTheme&, std::int64_t index);
        // `x` is its middle on that edge of the trail, `petal` a daisy's or the butterfly's spots.
        void drawRoadside(const SceneFrame&, const ChaseTheme&, Roadside, Verge, float x,
            float groundY, Color petal, const RoadsideMotion&);
        // How the boulder and the banana peel jump in the wake of each of them.
        struct Hop
        {
            float lift; // units off the ground
            float tilt; // radians
            float squash; // 0 to 1, as it lands
        };
        [[nodiscard]] Hop hopAt(const RoadsideMotion&);
        // Something that hops, drawn about the middle of its foot.
        [[nodiscard]] SceneFrame hopping(const SceneFrame& onVerge, const Hop&);
        void drawBoulder(const SceneFrame& onVerge, const ChaseTheme&, const Hop&);
        void drawDaisy(const SceneFrame& onVerge, const ChaseTheme&, Color petalColor,
            const RoadsideMotion&);
        void drawSignpost(const SceneFrame& onVerge, const ChaseTheme&, const RoadsideMotion&);
        void drawToadstool(const SceneFrame& onVerge, const ChaseTheme&, const RoadsideMotion&);
        // The fluff the wolf's wake tears from the reeds' heads, drawn in the reeds' units.
        void drawReedFluff(const SceneFrame& onVerge, const ChaseTheme&, float wolfTime);
        void drawTreeStump(const SceneFrame& onVerge, const ChaseTheme&);
        void drawBird(const SceneFrame& onVerge, const ChaseTheme&, float seconds,
            float sinceHareClose);
        void drawStumpCrow(const SceneFrame& onVerge, const ChaseTheme&, float seconds,
            float sinceHareClose);
        void drawButterfly(const SceneFrame& onVerge, const ChaseTheme&, Color spots, float seconds,
            float sinceHareClose);
        void drawFrog(const SceneFrame& onVerge, const ChaseTheme&, float sinceHareClose, Verge);
        void drawSnail(const SceneFrame& onVerge, const ChaseTheme&, const RoadsideMotion&);

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

        // One range of the mountains: k_mountainWaves scaled and shifted, at its own distance.
        struct MountainRange
        {
            float parallax; // its share of the camera's speed
            float wantedPeriod; // units its tile's period is fitted near
            float heightScale; // of the waves' amplitudes
            float phaseShift; // added to every wave's phase, so its peaks fall elsewhere
            float baseShare; // the ridges' middle line, as a share of the frame's height
            float baseLift; // units the middle line stands above that share
            float gradientTopShare; // where its colours start, as a share of the height
            float haze; // its share of skyBottom, less than its distance gives, so rock shows
        };
        constexpr MountainRange k_farRange{
            .parallax = 0.07f,
            .wantedPeriod = 2000.0f,
            .heightScale = 1.15f,
            .phaseShift = 1.0f,
            .baseShare = 0.6f,
            .baseLift = 40.0f,
            .gradientTopShare = 0.25f,
            .haze = 0.7f
        };
        constexpr MountainRange k_nearRange{
            .parallax = 0.12f,
            .wantedPeriod = 1500.0f,
            .heightScale = 0.7f,
            .phaseShift = 2.3f,
            .baseShare = 0.625f,
            .baseLift = 0.0f,
            .gradientTopShare = 0.38f,
            .haze = 0.63f
        };
    }

    // Three 20-second episodes a cycle: twice the wolf trips over something on the trail, then he
    // leaps into a branch. Each time the camera stops while he is down, and the hare taunts him.
    // See PathArt#chasescene
    void ChaseScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime hostTime)
    {
        const SceneTime time = { hostTime.cycleTime, hostTime.cyclesPassed + m_cyclesAhead };
        m_nextCycle = time.cyclesPassed + 1;
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const SceneTheme& theme = m_themes.worn();
        const float unit = frame.unit;
        const float width = frame.width;
        // A narrow frame draws the two closer together.
        const float closeness = std::min(1.0f, (width / unit) / 760.0f);
        const float travel = travelAt(time.cycleTime);
        const int episode = static_cast<int>(std::floor(time.cycleTime * k_episodesPerCycle));
        const float episodeTime = fractionalPart(time.cycleTime * k_episodesPerCycle);
        const bool branchRun = episode == k_branchEpisode;
        const TreeRun treeRun = treeRunOf(time.cyclesPassed);
        const LeapEnd leapEnd = treeRun.leapEnd;
        // The hare has an apple to eat at him unless the crow met him instead of a tree.
        const bool appleRun = branchRun && treeStands(leapEnd);
        const float groundY = frame.height * 0.78f;
        const float footY = groundY + k_trailBelowGround * unit;
        const auto toScreen = [&](float units) {
            return width * 0.5f + units * unit * closeness;
        };
        // Units the camera covers from one moment of this episode to another.
        const auto cameraTravel = [&](float from, float to) {
            return (cameraShare(to, episode) - cameraShare(from, episode)) * k_fullSpeedTravel;
        };

        drawBackdrop(frame, theme, time.cycleTime, travel, groundY);
        drawGround(frame, theme, travel, groundY);

        // The obstacle reaches the wolf as the camera starts to stop. His gap to the hare and the
        // branch's reach are held in pixels, so they hold in a narrow frame too.
        const float trip = branchRun ? k_branchStop.start : k_tripStop.start;
        const float wolfAtTrip = k_hareRunning - k_wolfBehindHareAtTrip / closeness;
        const float obstacleAtTrip = branchRun
            ? branchEndAtContact(closeness, leapEnd)
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
        // The duck goes flat under the wolf's paws. Once he is down it fills up again, swelling a
        // little past full, its legs pop out, and it walks off the way the hedgehog does, faster.
        DuckMotion duckMotion = {};
        float duckWalked = 0.0f;
        if (obstacle == Obstacle::RubberDuck)
        {
            const float flattened = smoothStep(trip - 0.002f, trip + 0.02f, episodeTime);
            const float refill = std::clamp((episodeTime - k_duckRefills) / 0.015f, 0.0f, 1.0f);
            const float emptyLeft = (1.0f - refill) * (1.0f - refill) * (1.0f - refill);
            const float growing = std::clamp((episodeTime - k_duckGrowsLegs) / 0.008f, 0.0f,
                1.0f);
            const float growingLeft = 1.0f - growing;
            // a back ease out, which takes the legs a tenth past their length before they settle
            constexpr float k_legsOvershoot{ 1.70158f };
            // up to speed a tenth of a second after it sets off
            constexpr float k_upToSpeed{ 0.005f };
            const float sinceSetOff = episodeTime - k_duckSetsOff;
            if (sinceSetOff > k_upToSpeed)
                duckWalked = k_duckWalk * (sinceSetOff - 0.5f * k_upToSpeed);
            else if (sinceSetOff > 0.0f)
                duckWalked = 0.5f * k_duckWalk * sinceSetOff * sinceSetOff / k_upToSpeed;
            duckMotion = {
                .deflation = flattened * emptyLeft
                    - 0.2f * std::sin(std::numbers::pi_v<float> * refill) * refill * refill,
                .airOut = 4.0f * flattened * (1.0f - flattened),
                .legs = growing > 0.0f
                    ? 1.0f - (k_legsOvershoot + 1.0f) * growingLeft * growingLeft * growingLeft
                        + k_legsOvershoot * growingLeft * growingLeft
                    : 0.0f,
                .strides = k_2Pi * duckWalked / k_duckStride
            };
        }
        // The ball rolls on a little, slowing.
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
            - duckWalked + rolledOff - rolledAway + peelAhead;
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
                : 1.1f * std::exp(-sinceLetGo / 0.0375f) * std::cos(k_2Pi * sinceLetGo / 0.02f);
        }

        // The wolf at `t` of the episode, with the camera having covered `travelThen`: chasing,
        // falling, lying dazed, getting up and running back into place.
        struct WolfMoment
        {
            float x; // units from the middle of the frame
            float lift;
            float dazed;
            float branchWeight;
            WolfPose pose;
        };
        const float wolfSize = k_wolfScale * unit;
        const auto wolfAt = [&](float t, float travelThen) {
            const float wolfX = wolfXAt(episode, t, closeness, leapEnd);
            float lift = 0.0f;
            float turn = 0.0f;
            float crouch = 0.0f;
            float leap = 0.0f;
            float jawOpen = 0.0f;
            float sprawl = 0.0f;
            float onBack = 0.0f;
            float dangle = 0.0f;
            float dazed = 0.0f;
            float running = 1.0f;
            float strain = 0.0f;
            float weariness = 0.0f;
            float snap = 0.0f;
            float branchWeight = 0.0f;
            if (t < trip)
            {
                if (!branchRun)
                {
                    // gaining on the hare ever faster and pressing harder, until the obstacle stops
                    // him; in the first run he snaps at her once on the way
                    const float towardTrip = t / trip;
                    strain = towardTrip * towardTrip;
                    if (episode == 0)
                    {
                        snap = bump(t, k_firstRunSnap, 0.005f);
                        jawOpen = chompAt(t, k_firstRunSnap);
                    }
                }
                else
                {
                    const BranchRun run = branchRunAt(t, closeness, leapEnd);
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
            else if (branchRun)
            {
                const TreeFall fall = treeFallAt(t, closeness, leapEnd);
                lift = fall.lift;
                turn = fall.turn;
                leap = fall.leap;
                jawOpen = fall.jawOpen;
                sprawl = fall.sprawl;
                onBack = fall.onBack;
                dangle = fall.dangle;
                dazed = fall.dazed;
                running = fall.running;
                branchWeight = fall.branchWeight;
            }
            else if (t < k_fallEnd)
            {
                // over the obstacle, head over heels, and flat on his belly, stopped short while
                // the hare runs on
                const float fall = (t - trip) / (k_fallEnd - trip);
                strain = 1.0f - smoothStep(0.0f, 0.2f, fall);
                turn = k_2Pi * smoothStep(0.05f, 0.9f, fall);
                lift = 60.0f * std::sin(std::numbers::pi_v<float> * std::min(1.0f, fall * 1.1f));
                sprawl = smoothStep(0.75f, 1.0f, fall);
                running = 1.0f - smoothStep(0.0f, 0.4f, fall);
            }
            else if (t < k_cameraWaiting)
            {
                sprawl = 1.0f;
                dazed = 1.0f;
                running = 0.0f;
            }
            else if (t < 0.86f)
            {
                const float rise = (t - k_cameraWaiting) / (0.86f - k_cameraWaiting);
                sprawl = 1.0f - smoothStep(0.0f, 1.0f, rise);
                lift = 16.0f * std::sin(std::numbers::pi_v<float> * rise);
                dazed = 1.0f - rise;
                running = smoothStep(0.5f, 1.0f, rise);
            }
            // The gallop follows the ground the wolf covers, so his feet keep pace with it.
            const float stride = k_2Pi * (travelThen + wolfX) / periodNear(1.0f, 150.0f);
            const float bob = (4.0f + 3.0f * weariness) * std::abs(std::sin(stride)) * running;
            const float standingHeight = 52.0f - 34.0f * sprawl - 30.0f * onBack - crouch;
            const FloatPoint wolf = {
                toScreen(wolfX + 8.0f * snap),
                footY - (standingHeight + lift + bob) * wolfSize
            };
            const bool bonked = branchRun && leapEnd != LeapEnd::OverBranch && t > trip
                && t < trip + 0.012f;
            // On his back the head drops toward the ground rather than into it; what he meets in
            // the air and the rake's handle knock it back; hung over the branch, it hangs.
            const float headDrop = 0.7f * onBack + (bonked ? -0.3f : 0.0f) - 0.35f * rakeOnFace
                + 0.6f * dangle;
            // Pressing on, he lowers his head; worn out, it hangs; in the air it reaches for the
            // hare.
            const float headReach = 0.1f * strain + 0.15f * weariness + 0.3f * snap - 0.12f * leap;
            const float tongueSwing = (0.4f + 0.4f * weariness) * std::sin(stride * 2.0f) * running;
            const float eyesOnHare = std::max(strain, leap);
            const float cycleThen = time.cycleTime + (t - episodeTime) / k_episodesPerCycle;
            return WolfMoment{
                .x = wolfX,
                .lift = lift,
                .dazed = dazed,
                .branchWeight = branchWeight,
                .pose = WolfPose{
                    .center = wolf,
                    .size = wolfSize,
                    .turn = turn + 0.07f * std::sin(stride) * running,
                    .stride = stride,
                    .leap = leap,
                    .sprawl = sprawl,
                    .onBack = onBack,
                    .dangle = dangle,
                    .dazed = dazed,
                    .headTurn = std::lerp(0.1f * std::sin(stride), 0.3f, sprawl) + headDrop
                        + headReach,
                    .tongueTurn = 0.3f + tongueSwing + 0.6f * dazed + 0.5f * std::max(strain, leap),
                    .jawOpen = jawOpen,
                    .glance = 0.6f * std::sin(k_2Pi * 3.0f * cycleThen) * (1.0f - eyesOnHare),
                    .cycleTime = cycleThen
                }
            };
        };
        const WolfMoment ourWolf = wolfAt(episodeTime, travel);
        const float wolfX = ourWolf.x;
        const float lift = ourWolf.lift;
        const float dazed = ourWolf.dazed;
        const float branchWeight = ourWolf.branchWeight;
        const WolfPose& wolfPose = ourWolf.pose;
        const FloatPoint wolf = wolfPose.center;

        // In the branch run she hops back to the fallen apple until her forepaws are over it,
        // picks it up and eats it in bites, crumbs flying, while the wolf lies dazed.
        float appleHop = 0.0f;
        float holding = 0.0f;
        float appleLeft = 1.0f;
        FloatPoint appleOnGround = {};
        if (appleRun)
        {
            // Where the apple lies while the camera stands; taken as the camera moves on, it would
            // carry her back with the ground.
            const FloatPoint restingMiddle = restingPlace(k_treeApples[k_hareApple]);
            const float treeWhileStanding = obstacleAtTrip
                + cameraTravel(HareApple::HopStart, trip);
            const float trunkOnScreen = toScreen(treeWhileStanding)
                + trunkFromBranchEnd(leapEnd) * unit;
            appleOnGround = {
                trunkOnScreen + restingMiddle.x * unit,
                groundY + restingMiddle.y * unit
            };
            const float hopping = std::clamp((episodeTime - HareApple::HopStart)
                / (HareApple::HopEnd - HareApple::HopStart), 0.0f, 1.0f);
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
        if (appleRun)
        {
            for (int bite = 0; bite != HareApple::BiteCount; ++bite)
            {
                const float biteAt = HareApple::FirstBite
                    + HareApple::BiteGap * static_cast<float>(bite);
                hareTeeth = std::max(hareTeeth, bump(episodeTime, biteAt, 0.004f));
            }
        }
        // The hare at `t` of the episode, with the camera having covered `travelThen`: running
        // ahead, clearing the obstacle, sitting up to taunt, and off again. `over` is 1 for the
        // hare bounding over the other couple's, -1 for the one ducking under her.
        struct HareMoment
        {
            float x; // units from the middle of the frame
            HarePose pose;
        };
        const float hareSize = unit;
        const bool otherCouple = branchRun && leapEnd == LeapEnd::OtherWolf;
        const auto hareAt = [&](float t, float travelThen, float over, float appleHopThen,
            float holdingThen, float teethThen) {
            // Over the trail her ears go flat as the wolf closes in; in the branch run she bolts as
            // he comes close and eases off as he drops back.
            float alarm = 0.0f;
            if (branchRun)
            {
                alarm = std::max(smoothStep(0.23f, 0.27f, t)
                    * (1.0f - smoothStep(0.36f, 0.44f, t)),
                    bump(t, BranchChase::Takeoff + 0.012f, 0.012f));
            }
            else
            {
                alarm = smoothStep(0.3f, trip, t)
                    * (1.0f - smoothStep(trip, trip + 0.03f, t));
            }
            const float hareX = hareXAt(episode, t, closeness, treeRun);
            const float sitting = smoothStep(0.62f, 0.66f, t)
                * (1.0f - smoothStep(0.83f, 0.86f, t)) * (1.0f - appleHopThen);
            const float turnedRound = smoothStep(0.66f, 0.69f, t)
                * (1.0f - smoothStep(0.82f, 0.85f, t));
            const float facing = 1.0f - 2.0f * turnedRound;
            const float hop = k_2Pi * (travelThen + hareX) / periodNear(1.0f, 180.0f);
            // over the trail she clears the obstacle high, and in the first run jumps at his snap
            const float startled = episode == 0 ? bump(t, k_firstRunSnap + 0.004f, 0.008f) : 0.0f;
            const float hopScale = branchRun
                ? 1.0f + 0.45f * alarm
                : 1.0f + 0.45f * alarm + 1.7f * bump(t, hareClears, 0.012f) + 0.8f * startled;
            const float air = appleHopThen > 0.0f
                ? appleHopThen
                : std::max(0.0f, std::sin(hop)) * (1.0f - sitting);
            const float squash = std::max(0.0f, -std::sin(hop)) * (1.0f - sitting);
            const float hareLift = air * 30.0f * hopScale * hareSize;
            // Where another couple runs the other way, the two hares cross as the wolves leap; one
            // bounds high over the other, which ducks with its ears laid back.
            float bounding = 0.0f;
            if (otherCouple)
            {
                const float passing = (t - haresCrossAt(closeness, treeRun)) / k_haresPassing;
                bounding = std::abs(passing) < 1.0f
                    ? std::cos(0.5f * std::numbers::pi_v<float> * passing)
                    : 0.0f;
            }
            const float leaping = std::max(0.0f, over) * bounding;
            const float ducking = std::max(0.0f, -over) * bounding;
            const float poseAir = std::max(air * (1.0f - ducking), leaping);
            const float footLift = hareLift * (1.0f - leaping) * (1.0f - ducking)
                + 70.0f * leaping * hareSize;
            return HareMoment{
                .x = hareX,
                .pose = HarePose{
                    .foot = { toScreen(hareX), footY - footLift },
                    .size = hareSize,
                    .facing = std::abs(facing) < 0.08f ? (facing < 0.0f ? -0.08f : 0.08f) : facing,
                    .air = poseAir,
                    .stretch = 1.0f + (0.16f + 0.08f * alarm) * poseAir - 0.08f * squash
                        - 0.12f * ducking,
                    .bodyTurn = std::lerp(-0.12f * poseAir, -0.85f, sitting),
                    .sitting = sitting,
                    .earTurn = std::lerp(-1.3f + 0.25f * poseAir - 0.35f * alarm, -0.15f, sitting)
                        - 0.4f * ducking,
                    .earWiggle = sitting * 0.22f * std::sin(k_2Pi * 24.0f * t),
                    .footLift = sitting * 7.0f * std::max(0.0f, std::sin(k_2Pi * 36.0f * t)),
                    .holding = holdingThen,
                    .teeth = teethThen
                }
            };
        };
        const float ourOver = treeRun.ourHareLeaps ? 1.0f : -1.0f;
        const HareMoment ourHare = hareAt(episodeTime, travel, ourOver, appleHop, holding,
            hareTeeth);
        const float hareX = ourHare.x;
        const HarePose& harePose = ourHare.pose;
        const float hareOnScreen = harePose.foot.x;

        // The branch springs up where his head hit it and shakes itself still; under him it bends
        // and springs back as he slides off. Flown into, the tree shakes its crown.
        const float sinceBonk = episodeTime - trip;
        float branchSway = 0.0f;
        float crownShake = 0.0f;
        if (branchRun && sinceBonk > 0.0f)
        {
            if (leapEnd == LeapEnd::Branch)
            {
                branchSway = -10.0f * std::exp(-sinceBonk / 0.025f)
                    * std::sin(k_2Pi * sinceBonk / 0.0125f);
            }
            else if (leapEnd == LeapEnd::OverBranch)
            {
                branchSway = branchWeight;
            }
            else if (leapEnd == LeapEnd::Trunk)
            {
                crownShake = 5.0f * std::exp(-sinceBonk / 0.03f)
                    * std::sin(k_2Pi * sinceBonk / 0.015f);
            }
        }
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
            .duck = duckMotion,
            .rollAngle = (rolledOff - rolledAway) / k_ballRadius,
            .bladeLean = bladeLean,
            .peelLift = 240.0f * peelFlight * (1.0f - peelFlight),
            .peelSpin = 2.0f * k_2Pi * peelFlight,
            .rakeSwing = rakeSwing,
            .bootTip = bootTip,
            .sinceHit = sinceBonk,
            .appleSplit = appleSplit,
            .crownShake = crownShake,
            .hareHasApple = appleRun && episodeTime >= HareApple::HopEnd,
            .treeRun = treeRun
        };
        // A swinging rake goes over the wolf, so its handle is seen to catch him, and the crow
        // tumbles off his face in front of him.
        const bool obstacleOverWolf = rakeSwing > 0.0f || (branchRun && leapEnd == LeapEnd::Crow);
        // What stands by the far edge of the path in this run, and the next run's as it comes into
        // a wide frame.
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
            const float hareCloseAt = passes
                - (k_hareRunning + k_stirringReach) / k_fullSpeedTravel;
            const float sinceHareClose = (runTime - hareCloseAt) * k_episodeSeconds;
            const bool flies = roadside == Roadside::Bird || roadside == Roadside::Butterfly
                || roadside == Roadside::CrowOnStump;
            const bool inView = roadsideOnScreen > -80.0f * unit
                && roadsideOnScreen < width + 80.0f * unit;
            if (!inView && !(flies && sinceHareClose > 0.0f && sinceHareClose < 3.0f))
                continue;
            drawRoadside(frame, theme.chase, roadside, Verge::Far, roadsideOnScreen, groundY,
                petalOf(theme, run), RoadsideMotion{
                .gust = wakeGust(roadsideX, hareX, wolfX),
                .sinceHareClose = sinceHareClose,
                .wolfTime = sinceHareClose,
                .seconds = runTime * k_episodeSeconds
            });
        }
        drawVergeRow(frame, theme, time, groundY, closeness, false);
        if (obstacleShown && branchRun && leapEnd == LeapEnd::Crow
            && treeRun.crowStart == CrowStart::OnStump)
        {
            drawCrowStump(frame, theme.chase, obstacleOnScreen, groundY);
        }
        if (obstacleShown && !obstacleOverWolf)
            drawObstacle(frame, theme, obstacle, obstacleOnScreen, groundY, footY, obstacleMotion);

        // The other couple are ours turned about where the wolves meet, drawn while near the
        // frame. Once the wolves are down they keep a time of their own: their hare runs a little
        // ahead of ours, their wolf a little behind.
        const auto theirX = [&](float ours) {
            return 2.0f * obstacleOnScreen - ours;
        };
        const float theirHareTime = episodeTime
            + k_theirHareAhead * smoothStep(0.59f, 0.61f, episodeTime);
        const float theirWolfTime = episodeTime
            - k_theirWolfBehind * smoothStep(0.64f, 0.70f, episodeTime);
        HarePose theirHare = hareAt(theirHareTime,
            travel + cameraTravel(episodeTime, theirHareTime), -ourOver, 0.0f, 0.0f, 0.0f).pose;
        theirHare.foot.x = theirX(theirHare.foot.x);
        theirHare.facing = -theirHare.facing;
        const WolfMoment theirWolf = wolfAt(theirWolfTime,
            travel + cameraTravel(episodeTime, theirWolfTime));
        const float theirWolfX = theirX(theirWolf.pose.center.x);
        const SceneFrame theirWolfFrame = frame.within(Matrix3x2::Translation(theirWolfX
            + theirWolf.pose.center.x, 0.0f) * Matrix3x2::Scale(-1.0f, 1.0f));
        const bool theyAreShown = otherCouple && theirHare.foot.x < width + 120.0f * unit
            && theirWolfX > -120.0f * unit;
        const SolidColor shadow{ theme.chase.shadow };
        const float wolfShadow = 46.0f * unit * (1.0f - 0.3f * std::min(1.0f, lift / 60.0f));
        fillOval(frame, { wolf.x, footY + 2.0f * unit }, wolfShadow, 5.0f * unit, shadow);
        const float hareShadow = 26.0f * unit * (1.0f - 0.3f * harePose.air);
        fillOval(frame, { hareOnScreen, footY + 2.0f * unit }, hareShadow, 4.0f * unit, shadow);
        if (theyAreShown)
        {
            fillOval(frame, { theirWolfX, footY + 2.0f * unit },
                46.0f * unit * (1.0f - 0.3f * std::min(1.0f, theirWolf.lift / 60.0f)),
                5.0f * unit, shadow);
            fillOval(frame, { theirHare.foot.x, footY + 2.0f * unit },
                26.0f * unit * (1.0f - 0.3f * theirHare.air), 4.0f * unit, shadow);
        }
        // dust kicked up behind him as he leaps
        const float sinceTakeoff = episodeTime - BranchChase::Takeoff;
        if (branchRun && sinceTakeoff > 0.0f && sinceTakeoff < 0.04f)
        {
            const float takeoffX = k_wolfAtTakeoff - 20.0f
                - cameraTravel(BranchChase::Takeoff, episodeTime);
            drawDustPuff(frame, DustPuff{
                .ground = { toScreen(takeoffX), footY },
                .settle = sinceTakeoff / 0.04f,
                .color = theme.chase.dust
            });
            if (theyAreShown)
            {
                drawDustPuff(theirWolfFrame, DustPuff{
                    .ground = { toScreen(takeoffX), footY },
                    .settle = sinceTakeoff / 0.04f,
                    .color = theme.chase.dust
                });
            }
        }
        drawHare(frame, harePose, theme.chase.brownHare);
        if (theyAreShown)
            drawHare(frame, theirHare, theme.chase.greyHare);
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
                theme.chase, 0.0f);
        }
        if (appleRun)
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
                        ? theme.chase.appleLight
                        : theme.chase.appleFlesh;
                    fillDisc(frame, at, 1.8f * unit,
                        SolidColor{ crumbColor.withOpacity(1.0f - seconds / 0.3f) });
                }
            }
        }
        drawWolf(frame, wolfPose, theme.chase.greyWolf);
        if (theyAreShown)
            drawWolf(theirWolfFrame, theirWolf.pose, theme.chase.blackWolf);
        if (obstacleShown && obstacleOverWolf)
            drawObstacle(frame, theme, obstacle, obstacleOnScreen, groundY, footY, obstacleMotion);

        // dust where he lands, and stars round his head while he is down
        const float landing = branchRun ? treeRestAt(leapEnd) - 0.003f : 0.6f;
        if (episodeTime > landing && episodeTime < landing + 0.12f)
        {
            const DustPuff dust = {
                .ground = { wolf.x, footY },
                .settle = (episodeTime - landing) / 0.12f,
                .color = theme.chase.dust
            };
            drawDustPuff(frame, dust);
            if (theyAreShown)
                drawDustPuff(theirWolfFrame, dust);
        }
        const auto drawStars = [&](const SceneFrame& onFrame, const WolfMoment& moment, float t) {
            if (moment.dazed <= 0.0f)
                return;
            const WolfPose& pose = moment.pose;
            const FloatPoint head = pose.center
                + rotated({ 54.0f * wolfSize, -24.0f * wolfSize }, pose.turn);
            drawDazeStars(onFrame, DazeStars{
                .center = { head.x, std::min(head.y, pose.center.y) - 26.0f * unit },
                .turns = 12.0f * t,
                .color = ChaseColors::DazeStar.withOpacity(moment.dazed),
                .outline = ChaseColors::DazeStarEdge.withOpacity(moment.dazed)
            });
        };
        drawStars(frame, ourWolf, episodeTime);
        if (theyAreShown)
            drawStars(theirWolfFrame, theirWolf, theirWolfTime);
        drawVergeRow(frame, theme, time, groundY, closeness, true);
        drawFrontGrass(frame, theme, travel);
    }

    void ChaseScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        constexpr float k_figureSpan{ 567.0f };
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const float unit = frame.unit;
        const FloatPoint center = { frame.width * 0.5f, frame.height * 0.5f };
        drawHare(frame, HarePose{
            .foot = center + FloatPoint{ 190.5f, 112.75f } * unit,
            .size = 2.0f * unit,
            .facing = 1.0f,
            .air = 0.7f,
            .stretch = 0.8f,
            .bodyTurn = 0.0f,
            .sitting = 0.0f,
            .earTurn = 0.3f,
            .earWiggle = 0.0f,
            .footLift = 0.0f,
            .holding = 0.0f,
            .teeth = 0.0f
        }, ChaseCoats::BrownHare);
        drawWolf(frame, WolfPose{
            .center = center + FloatPoint{ -89.5f, 7.75f } * unit,
            .size = 2.3f * unit,
            .turn = 0.0f,
            .stride = 1.2f,
            .leap = 0.6f,
            .sprawl = 0.0f,
            .onBack = 0.0f,
            .dangle = 0.0f,
            .dazed = 0.0f,
            .headTurn = 0.0f,
            .tongueTurn = 0.3f,
            .jawOpen = 0.6f,
            .glance = 0.0f,
            .cycleTime = 0.0f
        }, ChaseCoats::GreyWolf);
    }

    // **The picked theme's colours, not the blend's** - see readoutColorsOf.
    SceneReadoutColors ChaseScene::readoutColors() const
    {
        return readoutColorsOf(m_themes.picked());
    }

    void ChaseScene::storeView(Dom::Section& view) const
    {
        (view / k_nextCycleNode).set(static_cast<int>(m_nextCycle));
    }

    void ChaseScene::restoreView(const Dom::Section& view)
    {
        m_cyclesAhead = (view / k_nextCycleNode).get<int>();
        m_nextCycle = m_cyclesAhead;
    }

    // A first run starts at the first cycle.
    Dom::Dt::Section ChaseScene::viewSchema()
    {
        return Dom::Dt::Section{
            k_name,
            Dom::Dt::Value{ k_nextCycleNode, 0 }
        };
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
    void ChaseScene::drawBackdrop(const SceneFrame& frame, const SceneTheme& theme,
        const float cycleTime, const float travel, const float groundY)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        fillBox(frame, { 0.0f, 0.0f, width, groundY }, LinearGradient::Simple({ 0.0f, 0.0f },
            { 0.0f, groundY }, theme.skyTop, theme.skyBottom));
        drawStars(frame, cycleTime, StarField{
            .style = StarStyle::Sparkles,
            .count = 120,
            .top = 0.0f,
            .bottom = groundY,
            .color = theme.sunDisk,
            .opacity = theme.hasStars,
            .twinklesPerCycle = k_lively
        });
        const FloatPoint sun = { width * 0.84f, height * 0.14f };
        drawGlow(frame, sun, 130.0f * unit, theme.sunGlow.withOpacity(0.65f));
        PixelPath sunDisc;
        sunDisc.drawCircle(sun, 24.0f * unit);
        const PathDrawLayer sunRim = PathDrawLayer::OuterGlow(theme.sunDisk.withOpacity(0.75f),
            18.0f * unit);
        frame.canvas.drawPath(sunDisc, { sunRim, PathDrawLayer::Fill(theme.sunDisk) },
            &frame.toCanvas);

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
                fillShape(frame, cloud, SolidColor{ theme.cloud });
            }
        });

        // A rainy theme's lightning strikes behind the mountains and lights the sky.
        const float effectsTime = fractionalPart(cycleTime * k_lively);
        SceneFrame aboveGround = frame;
        aboveGround.height = groundY;
        drawLightning(aboveGround, effectsTime, Lightning{
            .strength = theme.hasRain,
            .color = theme.sunDisk
        });

        // The ridges are one path across the frame each, so no seam shows where two tiles meet.
        const auto drawMountains = [&](const MountainRange& range) {
            std::array<Wave, k_mountainWaves.size()> waves = k_mountainWaves;
            for (Wave& wave : waves)
            {
                wave.amplitude *= range.heightScale;
                wave.phase += range.phaseShift;
            }
            const float period = periodNear(range.parallax, range.wantedPeriod);
            const float baseY = height * range.baseShare - range.baseLift * unit;
            PixelPath ridges;
            ridges.moveTo(0.0f, groundY);
            for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
            {
                const float ridge = ridgeHeight(period, x / unit + range.parallax * travel,
                    waves);
                ridges.lineTo(x, baseY - ridge * unit);
            }
            ridges.lineTo(width + 6.0f * unit, groundY);
            ridges.close();
            fillShape(frame, ridges, LinearGradient::Simple(
                { 0.0f, height * range.gradientTopShare }, { 0.0f, groundY },
                colorBetween(theme.chase.mountainTop, theme.skyBottom, range.haze),
                colorBetween(theme.chase.mountainFoot, theme.skyBottom, range.haze)));
        };
        drawMountains(k_farRange);
        drawMountains(k_nearRange);

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
            hazed(theme, theme.hillTop, 0.35f), hazed(theme, theme.hillBottom, 0.35f)));
        const Color bark = hazed(theme, theme.chase.bark, 0.35f);
        const Color barkShade = hazed(theme, theme.chase.barkShade, 0.35f);
        const SolidColor crown{ hazed(theme, theme.treeLeavesBase, 0.35f) };
        const SolidColor crownLight{ hazed(theme, theme.chase.crownLight, 0.35f) };
        const SolidColor crownMiddle{ hazed(theme, theme.chase.crownMiddle, 0.35f) };
        forEachTile(frame, 0.35f, hillPeriod, travel, [&](float tileLeft, float) {
            for (int i = 0; i != 4; ++i)
            {
                const float jitter = 0.06f * pseudoRandom(i, 111.0f);
                const float alongTile = hillPeriod
                    * (0.1f + 0.24f * static_cast<float>(i) + jitter);
                const float x = tileLeft + alongTile * unit;
                const float y = hillY(alongTile) + 6.0f * unit;
                const float size = (0.7f + 0.4f * pseudoRandom(i, 112.0f)) * unit;
                drawTrunk(frame, { x, y }, 3.0f * size, 22.0f * size, bark, barkShade);
                fillDisc(frame, { x, y - 32.0f * size }, 16.0f * size, crown);
                fillDisc(frame, { x - 9.0f * size, y - 25.0f * size }, 11.0f * size, crownLight);
                fillDisc(frame, { x + 9.0f * size, y - 26.0f * size }, 12.0f * size, crownMiddle);
            }
        });
        // **The theme's fireflies and weather keep to the far hills' layer**, smaller and fainter
        // for the distance, and move with the hills. The fireflies are hazed as the hills are. The
        // weather falls against the sky as well, so it is drawn fainter instead. It fills each tile
        // down to the ground line.
        const float farSize = 0.6f;
        Fireflies fireflies = firefliesOf(theme);
        fireflies.light = hazed(theme, fireflies.light, 0.35f);
        fireflies.glow = hazed(theme, fireflies.glow, 0.35f);
        fireflies.count = 22;
        fireflies.size = farSize;
        Weather weather = weatherOf(theme);
        weather.opacity = 0.6f;
        forEachTile(frame, 0.35f, hillPeriod, travel, [&](float tileLeft, float tileWidth) {
            fireflies.area = { tileLeft, groundY - 90.0f * unit, tileLeft + tileWidth,
                groundY - 20.0f * unit };
            drawFireflies(frame, effectsTime, fireflies);
            SceneFrame overHills = frame.within(Matrix3x2::Translation(tileLeft, 0.0f));
            overHills.width = tileWidth;
            overHills.height = groundY;
            overHills.unit = unit * farSize;
            drawWeather(overHills, effectsTime, weather);
        });
    }

    void ChaseScene::drawGround(const SceneFrame& frame, const SceneTheme& theme,
        const float travel, const float groundY)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const FloatRect grass = { 0.0f, groundY, width, frame.height };
        fillBox(frame, grass, LinearGradient::Simple({ 0.0f, groundY }, { 0.0f, frame.height },
            theme.hillTop, theme.hillBottom));
        fillBox(frame, { 0.0f, groundY + 10.0f * unit, width, groundY + 36.0f * unit },
            SolidColor{ theme.chase.trail });
        // Tufts, pebbles and flowers each go down over every tile before the next kind does, so a
        // flower stands over a tuft whichever tile either is in.
        const float tilePeriod = periodNear(1.0f, 400.0f);
        ScatteredShapes tufts{ frame, SolidColor{ theme.chase.grass }, 2.0f * unit };
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
        ScatteredShapes pebbles{ frame, SolidColor{ theme.chase.pebble } };
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
        // fill. The three flowers of a tile wear the theme's three petal colours, and every
        // flower's petals go down before the hearts. They take half the lawn's colour where they
        // grow, so they lie in the lawn rather than on it.
        const Color lawn = colorBetween(theme.hillTop, theme.hillBottom,
            63.0f * unit / (frame.height - groundY));
        const auto inLawn = [&](Color color) {
            return SolidColor{ colorBetween(color, lawn, 0.5f) };
        };
        const auto flowerAt = [&](float tileLeft, float tileWidth, int i) {
            // each keeps to its own third of the tile, so no two grow into one
            const float across = (static_cast<float>(i) + 0.2f
                + 0.6f * pseudoRandom(i, 127.0f)) / 3.0f;
            return FloatPoint{
                tileLeft + tileWidth * across,
                groundY + (48.0f + 30.0f * pseudoRandom(i, 128.0f)) * unit
            };
        };
        ScatteredShapes petals[] = {
            ScatteredShapes{ frame, inLawn(theme.petal1) },
            ScatteredShapes{ frame, inLawn(theme.petal2) },
            ScatteredShapes{ frame, inLawn(theme.petal3) }
        };
        forEachTile(frame, 1.0f, tilePeriod, travel, [&](float tileLeft, float tileWidth) {
            const float petalRadius = 1.75f * unit;
            for (int i = 0; i != 3; ++i)
            {
                const FloatPoint flower = flowerAt(tileLeft, tileWidth, i);
                PixelPath& flowerPetals = petals[i].next();
                for (int petal = 0; petal != 5; ++petal)
                {
                    const float angle = k_2Pi * static_cast<float>(petal) / 5.0f;
                    const FloatPoint out = { std::cos(angle), std::sin(angle) };
                    addEllipse(flowerPetals, flower + out * (3.0f * unit), petalRadius,
                        petalRadius);
                }
            }
        });
        for (ScatteredShapes& shapes : petals)
            shapes.draw();
        ScatteredShapes hearts{ frame, inLawn(theme.chase.flowerHeart) };
        forEachTile(frame, 1.0f, tilePeriod, travel, [&](float tileLeft, float tileWidth) {
            for (int i = 0; i != 3; ++i)
            {
                addEllipse(hearts.next(), flowerAt(tileLeft, tileWidth, i), 1.8f * unit,
                    1.8f * unit);
            }
        });
        hearts.draw();
    }

    void ChaseScene::drawFrontGrass(const SceneFrame& frame, const SceneTheme& theme,
        const float travel)
    {
        const float unit = frame.unit;
        const float height = frame.height;
        const float grassPeriod = periodNear(1.35f, 520.0f);
        const FloatPoint tallestTip = { 0.0f, height - (24.0f + 34.0f) * unit };
        // Every third blade is dark, and the dark ones go down first. A blade's root wears the
        // lawn's colour at the frame's bottom, so it grows out of the lawn.
        for (const bool dark : { true, false })
        {
            const Color shade = dark ? theme.chase.frontGrassDark : theme.chase.grass;
            ScatteredShapes blades{ frame, LinearGradient::Simple(tallestTip, { 0.0f, height },
                shade, theme.hillBottom) };
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

    // A row's places lie along the whole trail, a cycle's travel shared out evenly between them: on
    // the near verge k_nearRoadsidesPerCycle, so one comes by at any moment of a run, and on the
    // far verge k_farBetweenPerCycle between the runs' own. A near place keeps clear of the far
    // verge's, and those that would stand where the wolf comes down are left out. The hare's part
    // of the stirring starts the first time she comes close and the wolf's part the first time he
    // does, each found by going back over the scene's time; each part then plays through as if
    // that one had run by at full speed.
    void ChaseScene::drawVergeRow(const SceneFrame& frame, const SceneTheme& theme,
        const SceneTime time, const float groundY, const float closeness, const bool nearEdge)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        constexpr double k_secondsPerCycle = static_cast<double>(k_cycle.count());
        // The search for the two coming close steps back this far at a time, from now to when the
        // place was this many units ahead of the middle of the frame.
        constexpr double k_searchStep = 0.05 / k_secondsPerCycle;
        constexpr double k_searchFrom = 1800.0;
        constexpr int k_searchSteps = 1200;
        constexpr int k_bisections = 16;
        // The bird and the butterfly fly on for this long, and the bird this many units, so one
        // that sat behind the frame can fly into it.
        constexpr float k_flightSeconds = 3.0f;
        constexpr float k_flightReach = 1300.0f;
        // The units a second the two go by at, at full speed.
        constexpr float k_passingSpeed = k_fullSpeedTravel / k_episodeSeconds;

        double cycleTravel = 0.0;
        for (int episode = 0; episode != static_cast<int>(k_episodesPerCycle); ++episode)
            cycleTravel += static_cast<double>(cameraShare(1.0f, episode) * k_fullSpeedTravel);
        const auto cameraAt = [&](double cycles) {
            const double cyclesBefore = std::floor(cycles);
            return cyclesBefore * cycleTravel
                + static_cast<double>(travelAt(static_cast<float>(cycles - cyclesBefore)));
        };
        // Whether each of the two is within k_stirringReach of the place at `cycles`.
        struct Closeness
        {
            bool hare;
            bool wolf;
        };
        const auto closenessAt = [&](double place, double cycles) {
            const float cycleTime = static_cast<float>(cycles - std::floor(cycles));
            const int episode = static_cast<int>(std::floor(cycleTime * k_episodesPerCycle));
            const float episodeTime = fractionalPart(cycleTime * k_episodesPerCycle);
            const TreeRun run = treeRunOf(static_cast<std::int64_t>(std::floor(cycles)));
            const double closeFrom = place - cameraAt(cycles)
                - static_cast<double>(k_stirringReach);
            return Closeness{
                static_cast<double>(hareXAt(episode, episodeTime, closeness, run)) >= closeFrom,
                static_cast<double>(wolfXAt(episode, episodeTime, closeness, run.leapEnd))
                    >= closeFrom
            };
        };
        const double now = static_cast<double>(time.cyclesPassed)
            + static_cast<double>(time.cycleTime);
        // The first moment each came close; the hare can come close twice, hopping back past it
        // to her apple, and so can the wolf, falling back off the branch.
        struct CloseMoments
        {
            std::optional<double> hare;
            std::optional<double> wolf;
        };
        const auto closeMomentsAt = [&](double place) {
            const auto firstClose = [&](double shortAt, double closeAt, bool Closeness::* runner) {
                for (int bisection = 0; bisection != k_bisections; ++bisection)
                {
                    const double middle = 0.5 * (shortAt + closeAt);
                    if (closenessAt(place, middle).*runner)
                        closeAt = middle;
                    else
                        shortAt = middle;
                }
                return closeAt;
            };
            CloseMoments moments = {};
            double laterCycles = now;
            Closeness closenessLater = closenessAt(place, now);
            for (int step = 1; step <= k_searchSteps; ++step)
            {
                const double cycles = now - k_searchStep * static_cast<double>(step);
                if (place - cameraAt(cycles) >= k_searchFrom)
                    break;
                const Closeness closenessThen = closenessAt(place, cycles);
                if (!closenessThen.hare && closenessLater.hare)
                    moments.hare = firstClose(cycles, laterCycles, &Closeness::hare);
                if (!closenessThen.wolf && closenessLater.wolf)
                    moments.wolf = firstClose(cycles, laterCycles, &Closeness::wolf);
                laterCycles = cycles;
                closenessLater = closenessThen;
            }
            return moments;
        };
        const auto secondsSince = [&](const std::optional<double>& moment) {
            return moment ? static_cast<float>((now - *moment) * k_secondsPerCycle) : -1.0f;
        };
        // Where one of the two would be against the place, running by at full speed since it came
        // close; one that has not come close blows no wind.
        const auto runningBy = [&](const std::optional<double>& moment) {
            return moment
                ? secondsSince(moment) * k_passingSpeed - k_stirringReach
                : -static_cast<float>(k_searchFrom);
        };

        const std::span<const Roadside> roadsides = nearEdge
            ? std::span<const Roadside>{ k_nearRoadside }
            : std::span<const Roadside>{ k_farBetweenRoadside };
        const double perCycle = nearEdge ? k_nearRoadsidesPerCycle : k_farBetweenPerCycle;
        const double offset = nearEdge ? k_nearRoadsideOffset : k_farBetweenOffset;
        const double cameraNow = cameraAt(now);
        const double spacing = cycleTravel / perCycle;
        const auto intoCycle = [&](double place) {
            return place - std::floor(place / cycleTravel) * cycleTravel;
        };
        // Where the camera stands in each run while the wolf is down, and the far verge's places
        // within a cycle's travel, its runs' own and those between.
        std::array<double, 3> stands = {};
        std::array<double, 6> farPlaces = {};
        std::size_t farCount = 0;
        double runStart = 0.0;
        for (int episode = 0; episode != static_cast<int>(k_episodesPerCycle); ++episode)
        {
            const float passes = episode == k_branchEpisode
                ? k_roadsidePassesBeforeBranch
                : k_roadsidePasses;
            farPlaces[farCount++] = runStart
                + static_cast<double>(cameraShare(passes, episode) * k_fullSpeedTravel);
            stands[static_cast<std::size_t>(episode)] = runStart
                + static_cast<double>(cameraShare(k_cameraWaiting, episode) * k_fullSpeedTravel);
            runStart += static_cast<double>(cameraShare(1.0f, episode) * k_fullSpeedTravel);
        }
        const auto byACollision = [&](double place) {
            const double intoTrail = intoCycle(place);
            return std::ranges::any_of(stands, [&](double stand) {
                const double fromStand = std::abs(intoTrail - stand);
                return std::min(fromStand, cycleTravel - fromStand) < k_collisionStretchReach;
            });
        };
        const double betweenSpacing = cycleTravel / k_farBetweenPerCycle;
        for (int between = 0; between != static_cast<int>(k_farBetweenPerCycle); ++between)
        {
            const double betweenPlace = (static_cast<double>(between) + k_farBetweenOffset)
                * betweenSpacing;
            if (!byACollision(betweenPlace))
                farPlaces[farCount++] = betweenPlace;
        }
        // In order along the trail, so a near place moved past one far place is checked against
        // the next.
        std::ranges::sort(std::span{ farPlaces }.first(farCount));
        const auto clearOfFar = [&](double place) {
            const double cycleStart = place - intoCycle(place);
            for (const double turn : { -cycleTravel, 0.0, cycleTravel })
            {
                for (std::size_t far = 0; far != farCount; ++far)
                {
                    const double farPlace = cycleStart + turn + farPlaces[far];
                    if (std::abs(place - farPlace) < k_nearClearOfFar)
                        place = farPlace + k_nearClearOfFar;
                }
            }
            return place;
        };
        const float halfSpan = width * 0.5f / (unit * closeness);
        const float margin = 80.0f * (nearEdge ? k_nearVergeSize : 1.0f);
        const double firstPlace = cameraNow
            - static_cast<double>(halfSpan + margin + k_flightReach);
        const double lastPlace = cameraNow + static_cast<double>(halfSpan + margin);
        const std::int64_t firstIndex = static_cast<std::int64_t>(std::ceil(firstPlace / spacing
            - offset));
        const std::int64_t lastIndex = static_cast<std::int64_t>(std::floor(lastPlace / spacing
            - offset));
        const std::int64_t count = static_cast<std::int64_t>(roadsides.size());
        for (std::int64_t index = firstIndex; index <= lastIndex; ++index)
        {
            const double evenPlace = (static_cast<double>(index) + offset) * spacing;
            const double place = nearEdge ? clearOfFar(evenPlace) : evenPlace;
            if (byACollision(place))
                continue;
            const float onScreen = width * 0.5f
                + static_cast<float>(place - cameraNow) * unit * closeness;
            const std::int64_t inTurn = (index % count + count) % count;
            const Roadside roadside = roadsides[static_cast<std::size_t>(inTurn)];
            const CloseMoments moments = closeMomentsAt(place);
            const float sinceHareClose = secondsSince(moments.hare);
            const bool flies = roadside == Roadside::Bird || roadside == Roadside::Butterfly
                || roadside == Roadside::CrowOnStump;
            const bool inView = onScreen > -margin * unit && onScreen < width + margin * unit;
            if (!inView && !(flies && sinceHareClose > 0.0f && sinceHareClose < k_flightSeconds))
                continue;
            drawRoadside(frame, theme.chase, roadside, nearEdge ? Verge::Near : Verge::Far,
                onScreen, groundY, petalOf(theme, index), RoadsideMotion{
                    .gust = wakeGust(0.0f, runningBy(moments.hare), runningBy(moments.wolf)),
                    .sinceHareClose = sinceHareClose,
                    .wolfTime = moments.wolf
                        ? secondsSince(moments.wolf) + k_wolfAfterHare
                        : -1.0f,
                    .seconds = static_cast<float>(std::fmod(now * k_secondsPerCycle, 30.0))
                });
        }
    }

    namespace
    {
        float cameraShare(const float episodeTime, const int episode)
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

        TreeRun treeRunOf(const std::int64_t cyclesPassed)
        {
            const auto inTurn = [](std::int64_t passed, std::size_t count) {
                const std::int64_t turns = static_cast<std::int64_t>(count);
                return static_cast<std::size_t>((passed % turns + turns) % turns);
            };
            const std::int64_t turnsPassed = (cyclesPassed
                - static_cast<std::int64_t>(inTurn(cyclesPassed, k_leapEnds.size())))
                / static_cast<std::int64_t>(k_leapEnds.size());
            return {
                .leapEnd = k_leapEnds[inTurn(cyclesPassed, k_leapEnds.size())],
                .appleFall = k_appleFalls[inTurn(turnsPassed, k_appleFalls.size())],
                .crowStart = k_crowStarts[inTurn(turnsPassed, k_crowStarts.size())],
                .ourHareLeaps = inTurn(turnsPassed, 2) == 0
            };
        }

        float hareBoltAt(const float episodeTime)
        {
            return 50.0f * smoothStep(0.24f, 0.31f, episodeTime)
                - 30.0f * smoothStep(0.34f, 0.46f, episodeTime);
        }

        // Each stretch of the run is eased in and out; the leap is a throw whose top would come
        // just past the branch.
        BranchRun branchRunAt(const float episodeTime, const float closeness,
            const LeapEnd leapEnd)
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
            // Over the branch he leaps higher and tops out sooner, coming down onto it with his
            // nose tipping down.
            const bool overBranch = leapEnd == LeapEnd::OverBranch;
            const float leapTop = overBranch ? k_leapTopOverBranch : BranchChase::LeapTop;
            const float leapHeight = overBranch ? k_leapHeightOverBranch : k_leapHeight;
            const float wolfAtContact = overBranch
                ? k_wolfAtBranch + (k_branchReach + k_wolfPastBranchEnd) / closeness
                : k_wolfAtBranch;
            const float nosingDown = overBranch ? 0.5f : 0.17f;
            const float fromTop = (episodeTime - leapTop) / (leapTop - takeoff);
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
            const float chomp = chompAt(episodeTime, snapAt);
            return {
                .x = k_wolfChasing
                    + (closestX - k_wolfChasing) * closingIn
                    - (closestX - k_wolfDroppedBack) * droppingBack
                    + (k_wolfAtTakeoff - k_wolfDroppedBack) * comingOn
                    + (wolfAtContact - k_wolfAtTakeoff) * flight,
                .lift = episodeTime > takeoff ? leapHeight * (1.0f - fromTop * fromTop) : 0.0f,
                .turn = 0.05f * strain + 0.1f * crouching - (0.32f - nosingDown * flight) * inAir,
                .crouch = 9.0f * crouching,
                .leap = smoothStep(takeoff, takeoff + 0.01f, episodeTime),
                .jawOpen = chomp + smoothStep(takeoff + 0.003f, takeoff + 0.014f, episodeTime),
                .strain = strain,
                .weariness = weariness,
                .snap = bump(episodeTime, snapAt, 0.005f)
            };
        }

        float chompAt(const float episodeTime, const float snapAt)
        {
            return smoothStep(snapAt - 0.012f, snapAt - 0.004f, episodeTime)
                * (1.0f - smoothStep(snapAt - 0.001f, snapAt + 0.0015f, episodeTime));
        }

        bool treeStands(const LeapEnd leapEnd)
        {
            return leapEnd != LeapEnd::Crow && leapEnd != LeapEnd::OtherWolf;
        }

        float trunkFromBranchEnd(const LeapEnd leapEnd)
        {
            return leapEnd == LeapEnd::OverBranch
                ? k_wolfPastBranchEnd - k_wolfPastTrunk
                : k_trunkFromBranchEnd;
        }

        // Flown into, the trunk's near side stands where the branch's end would.
        float branchEndAtContact(const float closeness, const LeapEnd leapEnd)
        {
            const float trunkSide = leapEnd == LeapEnd::Trunk
                ? k_trunkFromBranchEnd - k_trunkHalfWidth
                : 0.0f;
            return k_wolfAtBranch + (k_branchReach - trunkSide) / closeness;
        }

        // From the leap's end to where he lies, each way down is its own; the bounce as he lands,
        // the getting up and the running back into place are the same for all.
        TreeFall treeFallAt(const float episodeTime, const float closeness, const LeapEnd leapEnd)
        {
            const float pi = std::numbers::pi_v<float>;
            const float contact = k_branchStop.start;
            const float restAt = treeRestAt(leapEnd);
            const auto travelSinceContact = [&](float to) {
                return (cameraShare(to, k_branchEpisode) - cameraShare(contact, k_branchEpisode))
                    * k_fullSpeedTravel;
            };
            const BranchRun atContact = branchRunAt(contact, closeness, leapEnd);
            // His middle as the leap ends, where he lies, and how his pose comes down to it.
            float lyingX = 0.0f;
            TreeFall fall = {
                .x = 0.0f,
                .lift = 0.0f,
                .turn = 0.0f,
                .leap = 0.0f,
                .jawOpen = 0.0f,
                .sprawl = 0.0f,
                .onBack = 0.0f,
                .dangle = 0.0f,
                .dazed = 1.0f,
                .running = 0.0f,
                .branchWeight = 0.0f
            };
            switch (leapEnd)
            {
                case LeapEnd::Branch:
                case LeapEnd::Crow:
                case LeapEnd::OtherWolf:
                {
                    // stopped dead in the air, a moment hung there, then down onto his back
                    lyingX = k_wolfAtBranch - travelSinceContact(restAt) - 18.0f;
                    if (episodeTime < restAt)
                    {
                        const float down = (episodeTime - contact) / (restAt - contact);
                        const float dropping = std::clamp((down - 0.2f) / 0.8f, 0.0f, 1.0f);
                        fall.x = k_wolfAtBranch - travelSinceContact(episodeTime)
                            - 18.0f * smoothStep(0.0f, 0.5f, down);
                        fall.lift = atContact.lift * (1.0f - dropping * dropping);
                        fall.turn = std::lerp(atContact.turn, -pi, smoothStep(0.2f, 0.95f, down));
                        fall.leap = 1.0f - smoothStep(0.2f, 0.6f, down);
                        // what he meets shuts his jaws for him
                        fall.jawOpen = 1.0f - smoothStep(0.0f, 0.1f, down);
                        fall.onBack = smoothStep(0.45f, 1.0f, down);
                        return fall;
                    }
                    break;
                }
                case LeapEnd::Trunk:
                {
                    // Flat against the trunk, belly first and nose up, he slides down it, then
                    // topples back off it onto his back.
                    constexpr float k_flattened = 0.584f;
                    constexpr float k_slidDown = 0.618f;
                    constexpr float k_bellyDepth = 17.0f;
                    constexpr float k_toppleBack = 40.0f;
                    // his middle's height over his feet's line, in his units, at the foot of the
                    // trunk, and lying
                    constexpr float k_atTrunkFoot = 66.0f;
                    constexpr float k_lyingHeight = 22.0f;
                    const float againstTrunk = k_wolfAtBranch
                        + (k_branchReach - k_bellyDepth) / closeness;
                    lyingX = againstTrunk - k_toppleBack / closeness - travelSinceContact(restAt);
                    if (episodeTime < restAt)
                    {
                        const float flattening = smoothStep(contact, k_flattened, episodeTime);
                        const float sliding = std::clamp((episodeTime - k_flattened)
                            / (k_slidDown - k_flattened), 0.0f, 1.0f);
                        const float toppling = smoothStep(k_slidDown, restAt, episodeTime);
                        fall.sprawl = flattening * (1.0f - toppling);
                        fall.onBack = toppling;
                        fall.leap = 1.0f - flattening;
                        fall.turn = std::lerp(std::lerp(atContact.turn, -0.5f * pi, flattening),
                            -pi, toppling);
                        fall.x = std::lerp(std::lerp(k_wolfAtBranch, againstTrunk, flattening),
                            againstTrunk - k_toppleBack / closeness, toppling)
                            - travelSinceContact(episodeTime);
                        // his middle held where the leap had it, then sliding down ever faster,
                        // then tipping back with a small hop
                        const float atContactHeight = 52.0f + atContact.lift;
                        const float height = toppling > 0.0f
                            ? std::lerp(k_atTrunkFoot, k_lyingHeight, toppling)
                                + 10.0f * std::sin(pi * toppling)
                            : std::lerp(atContactHeight, k_atTrunkFoot, sliding * sliding);
                        const float standing = 52.0f - 34.0f * fall.sprawl - 30.0f * fall.onBack;
                        fall.lift = height - standing;
                        return fall;
                    }
                    break;
                }
                case LeapEnd::OverBranch:
                {
                    // Across the branch, legs and tail hanging, he sways with it until he slips
                    // back down it and drops onto his back at the foot of the trunk.
                    constexpr float k_slipping = 0.70f;
                    constexpr float k_slipBack = 85.0f;
                    constexpr float k_lyingHeight = 22.0f;
                    const float onBranch = k_wolfAtBranch
                        + (k_branchReach + k_wolfPastBranchEnd) / closeness;
                    lyingX = onBranch - k_slipBack / closeness - travelSinceContact(restAt);
                    const float sinceLanding = episodeTime - contact;
                    // the branch gives under him and swings, then springs back as he goes
                    const float weight = 9.0f * smoothStep(0.0f, 0.006f, sinceLanding)
                        + 4.0f * std::exp(-sinceLanding / 0.02f)
                            * std::sin(k_2Pi * sinceLanding / 0.018f);
                    const float sinceSlip = episodeTime - k_slipping;
                    const float release = sinceSlip > 0.0f
                        ? -6.0f * std::exp(-sinceSlip / 0.02f)
                            * std::sin(k_2Pi * sinceSlip / 0.014f)
                        : 0.0f;
                    fall.branchWeight = episodeTime < k_slipping ? weight : release;
                    if (episodeTime < restAt)
                    {
                        const float slipping = std::clamp((episodeTime - k_slipping)
                            / (restAt - k_slipping), 0.0f, 1.0f);
                        const float settling = smoothStep(0.0f, 0.006f, sinceLanding);
                        const float sway = 0.1f * std::exp(-sinceLanding / 0.04f)
                            * std::sin(k_2Pi * sinceLanding / 0.03f);
                        fall.leap = 1.0f - settling;
                        fall.jawOpen = atContact.jawOpen * (1.0f - settling);
                        fall.dangle = settling * (1.0f - smoothStep(0.0f, 0.5f, slipping));
                        fall.onBack = smoothStep(0.2f, 1.0f, slipping);
                        fall.turn = std::lerp(std::lerp(atContact.turn, 0.12f, settling) + sway,
                            -pi, smoothStep(0.0f, 0.9f, slipping));
                        fall.x = onBranch
                            - k_slipBack / closeness * smoothStep(0.0f, 0.6f, slipping)
                            - travelSinceContact(episodeTime);
                        const float onBranchHeight = 52.0f + atContact.lift - 0.8f * weight;
                        const float height = std::lerp(onBranchHeight, k_lyingHeight,
                            slipping * slipping);
                        const float standing = 52.0f - 30.0f * fall.onBack;
                        fall.lift = height - standing;
                        return fall;
                    }
                    break;
                }
            }
            // He comes down hard enough to bounce once, lies dazed on his back, gets up and runs
            // back into place.
            if (episodeTime < k_cameraWaiting)
            {
                const float bounce = std::min(1.0f, (episodeTime - restAt) / 0.016f);
                fall.x = lyingX;
                fall.lift = 7.0f * std::sin(pi * bounce);
                fall.turn = -pi;
                fall.onBack = 1.0f;
                return fall;
            }
            if (episodeTime < 0.86f)
            {
                const float rise = (episodeTime - k_cameraWaiting) / (0.86f - k_cameraWaiting);
                fall.x = lyingX;
                fall.turn = -pi * (1.0f - smoothStep(0.0f, 1.0f, rise));
                fall.onBack = 1.0f - smoothStep(0.0f, 0.6f, rise);
                fall.lift = 40.0f * std::sin(pi * rise);
                fall.dazed = 1.0f - rise;
                fall.running = smoothStep(0.5f, 1.0f, rise);
                return fall;
            }
            fall.x = std::lerp(lyingX, k_wolfChasing, smoothStep(0.86f, 1.0f, episodeTime));
            fall.dazed = 0.0f;
            fall.running = 1.0f;
            return fall;
        }

        float treeRestAt(const LeapEnd leapEnd)
        {
            switch (leapEnd)
            {
                case LeapEnd::Trunk:
                    return 0.645f;
                case LeapEnd::OverBranch:
                    return 0.74f;
                case LeapEnd::Branch:
                case LeapEnd::Crow:
                case LeapEnd::OtherWolf:
                    break;
            }
            return k_fallEnd;
        }

        // Under the tree she hops back until her forepaws, 24 units ahead of her, are over her
        // apple; after the crow there is none, and she only sits and taunts.
        float hareXAt(const int episode, const float episodeTime, const float closeness,
            const TreeRun& treeRun)
        {
            const bool branchRun = episode == k_branchEpisode;
            const float trip = branchRun ? k_branchStop.start : k_tripStop.start;
            const float bolt = branchRun ? hareBoltAt(episodeTime) : 0.0f;
            const float runningAhead = smoothStep(branchRun ? 0.57f : trip, 0.66f, episodeTime);
            const float hareSitsAt = k_hareRunning + bolt + 100.0f * runningAhead;
            float hareAtApple = 0.0f;
            if (branchRun && treeStands(treeRun.leapEnd))
            {
                const float branchEndAtTrip = branchEndAtContact(closeness, treeRun.leapEnd);
                const float travelSinceTrip = (cameraShare(HareApple::HopStart, episode)
                    - cameraShare(trip, episode)) * k_fullSpeedTravel;
                const float appleFromTrunk = restingPlace(k_treeApples[k_hareApple]).x;
                const float pawsOverApple = branchEndAtTrip - travelSinceTrip
                    + (trunkFromBranchEnd(treeRun.leapEnd) + appleFromTrunk + 24.0f) / closeness;
                const float hopping = std::clamp((episodeTime - HareApple::HopStart)
                    / (HareApple::HopEnd - HareApple::HopStart), 0.0f, 1.0f);
                hareAtApple = (pawsOverApple - hareSitsAt) * smoothStep(0.0f, 1.0f, hopping);
            }
            return k_hareRunning + (hareSitsAt + hareAtApple - k_hareRunning)
                * (1.0f - smoothStep(0.86f, 1.0f, episodeTime));
        }

        // He gains on her, falls over what is on the trail or from where his leap ends, lies dazed
        // and runs back into place.
        float wolfXAt(const int episode, const float episodeTime, const float closeness,
            const LeapEnd leapEnd)
        {
            if (episode == k_branchEpisode)
            {
                return episodeTime < k_branchStop.start
                    ? branchRunAt(episodeTime, closeness, leapEnd).x
                    : treeFallAt(episodeTime, closeness, leapEnd).x;
            }
            const float trip = k_tripStop.start;
            const float wolfAtTrip = k_hareRunning - k_wolfBehindHareAtTrip / closeness;
            if (episodeTime < trip)
            {
                const float towardTrip = episodeTime / trip;
                const float lunge = episode == 0
                    ? k_firstRunLunge * bump(episodeTime, k_firstRunSnap, 0.012f)
                    : 0.0f;
                return std::lerp(k_wolfChasing, wolfAtTrip, towardTrip * towardTrip) + lunge;
            }
            if (episodeTime < k_fallEnd)
            {
                const float fall = (episodeTime - trip) / (k_fallEnd - trip);
                return std::lerp(wolfAtTrip, k_wolfDownAfterTrip, smoothStep(0.0f, 1.0f, fall));
            }
            if (episodeTime < 0.86f)
                return k_wolfDownAfterTrip;
            return std::lerp(k_wolfDownAfterTrip, k_wolfChasing,
                smoothStep(0.86f, 1.0f, episodeTime));
        }

        float haresCrossAt(const float closeness, const TreeRun& treeRun)
        {
            const float contact = k_branchStop.start;
            const float meetingAtContact = branchEndAtContact(closeness, treeRun.leapEnd);
            float before = 0.4f;
            float after = contact;
            for (int halving = 0; halving != 20; ++halving)
            {
                const float middle = 0.5f * (before + after);
                const float meeting = meetingAtContact
                    + (cameraShare(contact, k_branchEpisode) - cameraShare(middle, k_branchEpisode))
                        * k_fullSpeedTravel;
                if (hareXAt(k_branchEpisode, middle, closeness, treeRun) < meeting)
                    before = middle;
                else
                    after = middle;
            }
            return 0.5f * (before + after);
        }

        void drawObstacle(const SceneFrame& frame, const SceneTheme& theme, const Obstacle obstacle,
            const float x, const float groundY, const float footY, const ObstacleMotion& motion)
        {
            const SceneFrame onTrail = frame.within(Matrix3x2::Translation(x, footY)
                * Matrix3x2::Scale(frame.unit));
            switch (obstacle)
            {
                case Obstacle::BananaPeel:
                    drawBananaPeel(onTrail, theme.chase, motion.peelLift, motion.peelSpin);
                    break;
                case Obstacle::Apple:
                    drawApple(onTrail, theme.chase, motion.appleSplit);
                    break;
                case Obstacle::Boot:
                    drawBoot(onTrail, theme.chase, motion.bootTip);
                    break;
                case Obstacle::Hedgehog:
                    drawHedgehog(onTrail, theme.chase, motion.curl, motion.steps);
                    break;
                case Obstacle::RubberDuck:
                    drawRubberDuck(onTrail, theme.chase, motion.duck);
                    break;
                case Obstacle::Ball:
                    drawBall(onTrail, theme.chase, motion.rollAngle);
                    break;
                case Obstacle::GrassPatch:
                    drawGrassPatch(onTrail, theme.chase, motion.bladeLean);
                    break;
                case Obstacle::Rake:
                    drawRake(onTrail, theme.chase, motion.rakeSwing);
                    break;
                case Obstacle::Branch:
                    if (motion.treeRun.leapEnd == LeapEnd::Crow)
                    {
                        drawCrow(frame, theme.chase, x, groundY, footY, motion.sinceHit,
                            motion.treeRun.crowStart);
                    }
                    else if (treeStands(motion.treeRun.leapEnd))
                    {
                        drawBranchTree(frame, theme, x, groundY, footY, motion);
                    }
                    break;
            }
        }

        // A peel of three flaps splayed on the trail, turning about its middle as it flies.
        void drawBananaPeel(const SceneFrame& onTrail, const ChaseTheme& chase, const float lift,
            const float spin)
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
                fillShape(peel, flap, SolidColor{ chase.peel });
                PixelPath inside;
                inside.moveTo(root);
                inside.quadTo(root + along * 0.5f + across * 0.4f, root + along * 0.85f);
                inside.quadTo(root + along * 0.5f - across * 0.1f, root);
                inside.close();
                fillShape(peel, inside, SolidColor{ chase.peelInside });
                fillDisc(peel, tip, 1.8f, SolidColor{ chase.peelTip });
            };
            drawFlap({ 1.5f, -10.0f }, { 14.0f, -1.5f }, 0.55f);
            drawFlap({ -1.5f, -10.0f }, { -20.0f, -2.0f }, -0.55f);
            fillRoundedBox(peel, { -4.0f, -15.0f, 4.0f, -4.0f }, 3.5f,
                SolidColor{ chase.peelShade });
            drawFlap({ 0.0f, -9.0f }, { 4.0f, 0.0f }, 0.5f);
            drawSegment(peel, { 0.0f, -14.0f }, { 2.0f, -19.0f },
                SolidColor{ chase.peelTip }, 2.6f);
        }

        void drawApple(const SceneFrame& onTrail, const ChaseTheme& chase, const float split)
        {
            const LinearGradient skin = LinearGradient::Simple({ -8.0f, -18.0f }, { 8.0f, 0.0f },
                chase.appleLight, chase.appleDark);
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
                    SolidColor{ chase.barkShade }, 1.8f);
                fillOval(onTrail, { 5.5f, -22.0f }, 5.0f, 2.2f, SolidColor{ chase.leaf },
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
                fillOval(half, faceMiddle, faceWidth, 7.2f, SolidColor{ chase.appleFlesh });
                for (const float seedY : { -11.0f, -7.5f })
                {
                    fillOval(half, { faceMiddle.x, seedY }, 0.8f * split, 1.4f * split,
                        SolidColor{ chase.appleSeed });
                }
                if (side > 0.0f)
                {
                    drawSegment(half, stemTop, { 1.5f, -24.0f },
                        SolidColor{ chase.barkShade }, 1.8f);
                    fillOval(half, { 5.5f, -22.0f }, 5.0f, 2.2f, SolidColor{ chase.leaf },
                        -0.5f);
                }
            }
        }

        // An old boot, its toe toward the wolf.
        void drawBoot(const SceneFrame& onTrail, const ChaseTheme& chase, const float tip)
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
                chase.bootLight, chase.bootDark));
            fillBox(boot, { -0.5f, -33.0f, 15.5f, -29.5f }, SolidColor{ chase.bootDark });
            fillBox(boot, { 7.0f, -16.0f, 12.0f, -11.0f }, SolidColor{ chase.bootPatch });
            fillRoundedBox(boot, { -22.0f, -4.0f, 16.0f, 0.0f }, 1.5f,
                SolidColor{ chase.bootSole });
            PixelPath laces;
            for (const float y : { -27.0f, -23.0f, -19.0f })
            {
                laces.moveTo(1.0f, y - 1.5f);
                laces.lineTo(6.0f, y + 1.5f);
                laces.moveTo(1.0f, y + 1.5f);
                laces.lineTo(6.0f, y - 1.5f);
            }
            strokeShape(boot, laces, SolidColor{ chase.lace }, 1.2f);
        }

        // A hedgehog crouched over its feet, its snout toward the wolf. Curling, its spines run on
        // round under it and its face shrinks into them; walking, its feet patter and it bobs.
        void drawHedgehog(const SceneFrame& onTrail, const ChaseTheme& chase, const float curl,
            const float steps)
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
            fillShape(hedgehog, spines, SolidColor{ chase.spinesDark });
            fillShape(hedgehog, back, SolidColor{ chase.spinesLight });
            PixelPath quills;
            for (int i = 0; i != 7; ++i)
            {
                const float share = 0.15f + 0.7f * static_cast<float>(i) / 6.0f;
                quills.moveTo(onBody(share, 0.42f));
                quills.lineTo(onBody(share, 0.76f));
            }
            strokeShape(hedgehog, quills, SolidColor{ chase.spinesDark }, 1.4f);
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
            fillOval(head, { -14.0f, -9.0f }, 10.5f, 8.5f, SolidColor{ chase.hedgehogFace });
            PixelPath snout;
            snout.moveTo(-18.0f, -14.0f);
            snout.quadTo({ -29.0f, -10.0f }, { -34.0f, -6.5f });
            snout.quadTo({ -29.0f, -2.5f }, { -17.0f, -2.5f });
            snout.close();
            fillShape(head, snout, SolidColor{ chase.hedgehogFace });
            fillDisc(head, { -12.0f, -17.0f }, 2.6f, SolidColor{ chase.hedgehogEar });
            fillDisc(head, { -34.0f, -6.5f }, 2.3f, SolidColor{ ChaseColors::Ink });
            fillDisc(head, { -18.0f, -12.0f }, 2.0f, SolidColor{ ChaseColors::Ink });
            fillDisc(head, { -17.4f, -12.6f }, 0.6f, SolidColor{ ChaseColors::EyeShine });
        }

        // A rubber duck, its beak toward the wolf. Deflating, it sinks flat and spreads, its head
        // droops and the air puffs out behind it. On legs it stands up off the trail, and walking
        // it rocks from foot to foot.
        void drawRubberDuck(const SceneFrame& onTrail, const ChaseTheme& chase,
            const DuckMotion& motion)
        {
            const float deflation = motion.deflation;
            const float widthScale = 1.0f + 0.2f * deflation;
            const float heightScale = 1.0f - 0.62f * deflation;
            const bool walking = motion.strides > 0.0f;
            // lowest as one foot takes over from the other
            const float dip = walking ? 1.4f * (1.0f - std::abs(std::cos(motion.strides))) : 0.0f;
            const float rockAngle = walking ? 0.08f * std::sin(motion.strides) : 0.0f;
            const FloatPoint rockPivot = { 3.0f, 0.0f };
            const Matrix3x2 onLegs = Matrix3x2::Translation(0.0f,
                    dip - k_duckLegLength * motion.legs)
                * Matrix3x2::Translation(rockPivot) * Matrix3x2::Rotation(rockAngle)
                * Matrix3x2::Translation(-rockPivot.x, -rockPivot.y);
            const SceneFrame duck = onTrail.within(onLegs
                * Matrix3x2::Scale(widthScale, heightScale));
            // a leg from the hip to a webbed foot, which swings forward lifted and comes back down
            const auto drawLeg = [&](const float hipX, const float phase, const Color color) {
                const FloatPoint hip = onLegs.transform({ hipX * widthScale, -2.0f * heightScale });
                const float stepAngle = motion.strides + phase;
                const float footSwing = walking ? 6.5f * std::sin(stepAngle) : 0.0f;
                const float footLift = walking ? 3.5f * std::max(0.0f, -std::cos(stepAngle)) : 0.0f;
                const FloatPoint foot = { hipX * widthScale + footSwing, -footLift };
                drawSegment(onTrail, hip, foot + FloatPoint{ 0.0f, -1.2f }, SolidColor{ color },
                    1.8f);
                PixelPath web;
                web.moveTo(1.4f, -0.3f);
                web.lineTo(-5.6f, -1.9f);
                web.quadTo({ -4.6f, -0.9f }, { -6.4f, -0.3f });
                web.quadTo({ -4.6f, 0.2f }, { -5.6f, 1.3f });
                web.close();
                fillShape(onTrail, web, SolidColor{ color }, Matrix3x2::Translation(foot)
                    * Matrix3x2::Rotation(-0.12f * footLift)
                    * Matrix3x2::Scale(std::min(1.0f, motion.legs)));
            };
            if (motion.legs > 0.0f)
                drawLeg(7.0f, std::numbers::pi_v<float>, chase.duckBeakShade);
            PixelPath body;
            body.moveTo(-14.0f, -10.0f);
            body.cubicTo({ -15.0f, -2.0f }, { -7.0f, 0.0f }, { 2.0f, 0.0f });
            body.cubicTo({ 12.0f, 0.0f }, { 17.0f, -3.0f }, { 17.0f, -9.0f });
            body.lineTo(20.0f, -18.0f);
            body.quadTo({ 13.0f, -16.0f }, { 8.0f, -15.0f });
            body.quadTo({ -4.0f, -13.0f }, { -14.0f, -10.0f });
            body.close();
            fillShape(duck, body, LinearGradient::Simple({ 0.0f, -18.0f }, {},
                chase.duckLight, chase.duckShade));
            fillOval(duck, { 4.0f, -8.0f }, 7.0f, 3.5f, SolidColor{ chase.duckShade },
                -0.3f);
            const FloatPoint neck = { -7.0f, -13.0f };
            const SceneFrame head = duck.within(Matrix3x2::Translation(neck)
                * Matrix3x2::Rotation(-0.5f * deflation)
                * Matrix3x2::Translation(-neck.x, -neck.y));
            fillDisc(head, { -8.0f, -21.0f }, 8.0f, SolidColor{ chase.duckLight });
            PixelPath beak;
            beak.moveTo(-14.0f, -22.0f);
            beak.quadTo({ -21.0f, -23.0f }, { -24.0f, -19.0f });
            beak.quadTo({ -20.0f, -17.0f }, { -14.0f, -18.0f });
            beak.close();
            fillShape(head, beak, SolidColor{ chase.duckBeak });
            PixelPath lowerBeak;
            lowerBeak.moveTo(-14.0f, -18.0f);
            lowerBeak.quadTo({ -19.0f, -17.5f }, { -22.0f, -18.0f });
            lowerBeak.quadTo({ -19.0f, -15.5f }, { -14.0f, -16.0f });
            lowerBeak.close();
            fillShape(head, lowerBeak, SolidColor{ chase.duckBeakShade });
            fillDisc(head, { -10.0f, -23.0f }, 1.7f, SolidColor{ ChaseColors::Ink });
            fillDisc(head, { -9.5f, -23.5f }, 0.5f, SolidColor{ ChaseColors::EyeShine });
            if (motion.legs > 0.0f)
                drawLeg(-1.0f, 0.0f, chase.duckBeak);
            // the air leaving it, strongest halfway down and gone once it is flat
            if (motion.airOut > 0.0f)
            {
                PixelPath streaks;
                for (const float slope : { -0.5f, 0.0f, 0.5f })
                {
                    streaks.moveTo(22.0f, -10.0f + 6.0f * slope);
                    streaks.lineTo(30.0f, -10.0f + 14.0f * slope);
                }
                strokeShape(onTrail, streaks,
                    SolidColor{ ChaseColors::Puff.withOpacity(motion.airOut) }, 1.6f);
            }
        }

        // A beach ball of six panels, which turn as it rolls.
        void drawBall(const SceneFrame& onTrail, const ChaseTheme& chase, const float rollAngle)
        {
            const std::array<Color, 6> panels{
                chase.ballRed,
                chase.ballWhite,
                chase.ballBlue,
                chase.ballWhite,
                chase.ballYellow,
                chase.ballWhite
            };
            const SceneFrame ball = onTrail.within(Matrix3x2::Translation(0.0f, -k_ballRadius)
                * Matrix3x2::Rotation(rollAngle));
            for (std::size_t panel = 0; panel != panels.size(); ++panel)
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
                fillShape(ball, wedge, SolidColor{ panels[panel] });
            }
            fillDisc(ball, {}, 3.0f, SolidColor{ chase.ballWhite });
            PixelPath rim;
            addEllipse(rim, {}, k_ballRadius, k_ballRadius);
            strokeShape(ball, rim, SolidColor{ ChaseColors::BallEdge }, 1.0f);
            fillOval(onTrail, { -5.0f, -19.0f }, 4.0f, 2.5f, SolidColor{ ChaseColors::Shine },
                -0.5f);
        }

        Color hazed(const SceneTheme& theme, const Color clear, const float parallax)
        {
            const float distance = 1.0f / parallax - 1.0f;
            return colorBetween(clear, theme.skyBottom, distance / (distance + k_halfHaze));
        }

        float wakeGust(const float x, const float hareX, const float wolfX)
        {
            const auto wake = [&](float runnerX, float strength) {
                const float behind = (runnerX - x - 15.0f) / 35.0f;
                return strength * std::exp(-behind * behind);
            };
            return wake(hareX, 0.35f) + wake(wolfX, 0.5f);
        }

        Color petalOf(const SceneTheme& theme, const std::int64_t index)
        {
            const Color petals[] = { theme.petal1, theme.petal2, theme.petal3 };
            return petals[(index % 3 + 3) % 3];
        }

        void drawRoadside(const SceneFrame& frame, const ChaseTheme& chase, const Roadside roadside,
            const Verge verge, const float x, const float groundY, const Color petal,
            const RoadsideMotion& motion)
        {
            const bool nearVerge = verge == Verge::Near;
            const float size = (nearVerge ? k_nearVergeSize : 1.0f) * frame.unit;
            const float vergeY = groundY
                + (nearVerge ? k_nearVergeDrop : k_farVergeDrop) * frame.unit;
            const SceneFrame onVerge = frame.within(Matrix3x2::Translation(x, vergeY)
                * Matrix3x2::Scale(size));
            const auto shadow = [&](float shadowWidth, float opacity) {
                fillOval(onVerge, { 1.0f, 0.0f }, shadowWidth, 2.5f,
                    SolidColor{ faded(chase.shadow, opacity) });
            };
            switch (roadside)
            {
                case Roadside::Apple:
                {
                    const Hop hop = hopAt(motion);
                    shadow(10.0f * (1.0f - hop.lift / 54.0f), 1.0f);
                    drawApple(hopping(onVerge, hop).within(Matrix3x2::Scale(0.8f)), chase, 0.0f);
                    break;
                }
                case Roadside::BananaPeel:
                {
                    const Hop hop = hopAt(motion);
                    shadow(14.0f * (1.0f - hop.lift / 54.0f), 1.0f);
                    drawBananaPeel(hopping(onVerge, hop), chase, 0.0f, 0.0f);
                    break;
                }
                case Roadside::Boulder:
                    drawBoulder(onVerge, chase, hopAt(motion));
                    break;
                case Roadside::Daisy:
                    drawDaisy(onVerge, chase, petal, motion);
                    break;
                case Roadside::Reeds:
                {
                    const float reedsSize = 0.9f * size;
                    drawReeds(frame, ReedsPose{
                        .base = { x - 12.0f * reedsSize, vergeY },
                        .size = reedsSize,
                        .cycleTime = -1.0f / k_2Pi, // their own sway fans them evenly
                        .lean = motion.gust,
                        .fur = 1.0f - 0.65f * smoothStep(1.0f, 1.3f, motion.wolfTime),
                        .inWater = false
                    }, ReedColors{ .stem = chase.reedStem, .head = chase.reedHead });
                    drawReedFluff(onVerge, chase, motion.wolfTime);
                    break;
                }
                case Roadside::Signpost:
                    shadow(7.0f, 1.0f);
                    drawSignpost(onVerge, chase, motion);
                    break;
                case Roadside::Toadstool:
                    shadow(12.0f, 1.0f);
                    drawToadstool(onVerge, chase, motion);
                    break;
                case Roadside::TreeStump:
                {
                    const Hop hop = hopAt(motion);
                    shadow(19.0f * (1.0f - hop.lift / 54.0f), 1.0f);
                    drawTreeStump(hopping(onVerge, hop), chase);
                    break;
                }
                case Roadside::Bird:
                    shadow(10.0f, std::clamp(1.0f - 2.0f * motion.sinceHareClose, 0.0f, 1.0f));
                    drawBird(onVerge.within(Matrix3x2::Scale(1.5f)), chase, motion.seconds,
                        motion.sinceHareClose);
                    break;
                case Roadside::Butterfly:
                    drawButterfly(onVerge.within(Matrix3x2::Scale(1.3f)), chase, petal,
                        motion.seconds, motion.sinceHareClose);
                    break;
                case Roadside::Frog:
                    drawFrog(onVerge.within(Matrix3x2::Scale(1.2f)), chase,
                        motion.sinceHareClose, verge);
                    break;
                case Roadside::Snail:
                    shadow(12.0f, 1.0f);
                    drawSnail(onVerge, chase, motion);
                    break;
                case Roadside::CrowOnStump:
                    shadow(19.0f, 1.0f);
                    drawTreeStump(onVerge, chase);
                    drawStumpCrow(onVerge, chase, motion.seconds, motion.sinceHareClose);
                    break;
            }
        }

        // Once just after the hare is by, higher just after the wolf is, then once more.
        Hop hopAt(const RoadsideMotion& motion)
        {
            struct Bounce
            {
                float start;
                float seconds;
                float height;
                float tilt;
                bool afterWolf;
            };
            constexpr std::array<Bounce, 3> k_bounces{
                Bounce{ 0.34f, 0.22f, 6.75f, -0.06f, false },
                Bounce{ 1.32f, 0.38f, 21.6f, 0.14f, true },
                Bounce{ 1.7f, 0.22f, 6.75f, -0.05f, true }
            };
            Hop hop = {};
            for (const Bounce& bounce : k_bounces)
            {
                const float bounceClock = bounce.afterWolf
                    ? motion.wolfTime
                    : motion.sinceHareClose;
                const float inAir = (bounceClock - bounce.start) / bounce.seconds;
                if (inAir > 0.0f && inAir < 1.0f)
                {
                    const float arc = std::sin(std::numbers::pi_v<float> * inAir);
                    hop.lift = bounce.height * arc;
                    hop.tilt = bounce.tilt * arc;
                }
                hop.squash = std::max(hop.squash, (bounce.height / 21.6f)
                    * bump(bounceClock, bounce.start + bounce.seconds + 0.03f, 0.05f));
            }
            return hop;
        }

        SceneFrame hopping(const SceneFrame& onVerge, const Hop& hop)
        {
            return onVerge.within(Matrix3x2::Translation(0.0f, -hop.lift)
                * Matrix3x2::Rotation(hop.tilt) * Matrix3x2::Scale(1.0f + 0.06f * hop.squash,
                1.0f - 0.1f * hop.squash));
        }

        void drawBoulder(const SceneFrame& onVerge, const ChaseTheme& chase, const Hop& hop)
        {
            constexpr float k_size = 1.35f;
            const SceneFrame ground = onVerge.within(Matrix3x2::Scale(k_size));
            fillOval(ground, { 1.0f, 0.0f }, 17.5f * (1.0f - hop.lift / 54.0f), 1.9f,
                SolidColor{ chase.shadow });
            const SceneFrame stone = hopping(onVerge, hop).within(Matrix3x2::Scale(k_size));
            PixelPath rock;
            rock.moveTo(-17.0f, 0.0f);
            rock.cubicTo({ -20.0f, -9.0f }, { -14.0f, -20.0f }, { -3.0f, -22.0f });
            rock.cubicTo({ 7.0f, -24.0f }, { 16.0f, -17.0f }, { 18.0f, -9.0f });
            rock.cubicTo({ 20.0f, -3.0f }, { 18.0f, 0.5f }, { 14.0f, 0.5f });
            rock.close();
            fillShape(stone, rock, LinearGradient::Simple({ -8.0f, -21.0f }, { 10.0f, 0.0f },
                chase.stoneLight, chase.stoneDark));
            fillOval(stone, { -5.0f, -17.0f }, 6.5f, 2.6f, SolidColor{ ChaseColors::Shine },
                -0.25f);
            PixelPath crack;
            crack.moveTo(5.0f, -15.0f);
            crack.lineTo(8.0f, -10.0f);
            crack.lineTo(6.0f, -6.0f);
            crack.lineTo(8.5f, -2.5f);
            strokeShape(stone, crack, SolidColor{ chase.stoneCrack }, 1.0f);
            for (const float tuftX : { -15.0f, 15.0f })
            {
                for (int blade = -1; blade <= 1; ++blade)
                {
                    const float bladeX = static_cast<float>(blade) * 3.0f;
                    const float bladeHeight = 7.0f + 2.0f * static_cast<float>(1 - std::abs(blade));
                    drawSegment(ground, { tuftX, 0.5f }, { tuftX + bladeX, -bladeHeight },
                        SolidColor{ chase.grass }, 1.6f);
                }
            }
        }

        // Three times the size of the daisies in the grass, nodding in the gust. The wolf's wake
        // blows five petals off, which flutter down after him and lie where they land.
        void drawDaisy(const SceneFrame& onVerge, const ChaseTheme& chase, const Color petalColor,
            const RoadsideMotion& motion)
        {
            constexpr std::array<int, 5> k_blownOff{ 0, 3, 5, 8, 10 };
            // the nod at the height of the wolf's wake, when he hides the head
            constexpr float k_nodBlown = 0.48f;
            constexpr float k_blownAt = 1.12f;
            // a blown petal falls at 18 units a second, gathering 28 units a second each second
            constexpr float k_fallSpeed = 18.0f;
            constexpr float k_fallGathering = 14.0f;
            const float nod = 0.08f + 0.8f * motion.gust;
            const FloatPoint head = { 42.0f * std::sin(nod), -42.0f * std::cos(nod) };
            PixelPath stem;
            stem.moveTo(0.0f, 0.5f);
            stem.quadTo({ 0.0f, -22.0f }, head);
            strokeShape(onVerge, stem, SolidColor{ chase.grass }, 1.8f);
            fillOval(onVerge, { -5.0f, -12.0f }, 6.0f, 2.2f, SolidColor{ chase.daisyLeaf }, 0.5f);
            fillOval(onVerge, { 5.0f + 6.0f * nod, -19.0f }, 6.0f, 2.2f,
                SolidColor{ chase.daisyLeaf }, nod - 0.5f);
            const SceneFrame bloom = onVerge.within(Matrix3x2::Translation(head)
                * Matrix3x2::Rotation(nod));
            const SolidColor petalFill{ petalColor };
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
                const float flying = motion.wolfTime - (k_blownAt + 0.035f * order);
                if (blown == k_blownOff.end() || flying < 0.0f)
                {
                    fillOval(bloom, out * 6.5f, 4.2f, 1.9f, petalFill, angle);
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
                fillOval(onVerge, at, 4.2f, 1.9f * std::max(0.3f, flutter), petalFill,
                    landed ? 0.15f * order - 0.3f : spin);
            }
            fillDisc(bloom, {}, 3.8f, SolidColor{ chase.flowerHeart });
        }

        // A post with a board pointing the way they run. The hare's rush turns the board once
        // about the post, the wolf's one and a half times more, so it is left pointing back the
        // way they came. Turning, it is seen edge-on a quarter of the way round and from the back
        // halfway.
        void drawSignpost(const SceneFrame& onVerge, const ChaseTheme& chase,
            const RoadsideMotion& motion)
        {
            drawSegment(onVerge, { 0.0f, 0.0f }, { 0.0f, -44.0f }, SolidColor{ chase.bark },
                4.0f);
            const auto turned = [&](float clock, float from, float seconds) {
                const float left = 1.0f - std::clamp((clock - from) / seconds, 0.0f, 1.0f);
                return 1.0f - left * left * left;
            };
            const float revolved = k_2Pi * (turned(motion.sinceHareClose, 0.15f, 0.8f)
                + 1.5f * turned(motion.wolfTime, 1.0f, 1.4f));
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
            fillShape(boardFrame, board, SolidColor{ chase.board });
            const float edgeOn = 1.0f - std::abs(facing);
            const SolidColor shade{ chase.boardShade.withOpacity(0.7f * edgeOn) };
            fillShape(boardFrame, board, shade);
            strokeShape(boardFrame, board, SolidColor{ chase.bough }, 1.0f);
            // the writing is on the face that looks out at the path
            const float writing = smoothStep(0.3f, 0.6f, facing);
            if (writing > 0.0f)
            {
                const SolidColor ink{ chase.bough.withOpacity(writing) };
                drawSegment(boardFrame, { -8.0f, -36.5f }, { 9.0f, -36.5f }, ink, 1.2f);
                drawSegment(boardFrame, { -8.0f, -33.5f }, { 4.0f, -33.5f }, ink, 1.2f);
            }
        }

        // Its cap hops on the stalk after the hare goes by, and off it after the wolf, landing on
        // the ground beside the stalk.
        void drawToadstool(const SceneFrame& onVerge, const ChaseTheme& chase,
            const RoadsideMotion& motion)
        {
            PixelPath stalk;
            stalk.moveTo(-4.0f, 0.0f);
            stalk.cubicTo({ -3.0f, -6.0f }, { -3.5f, -12.0f }, { -3.0f, -16.0f });
            stalk.lineTo(3.0f, -16.0f);
            stalk.cubicTo({ 3.5f, -12.0f }, { 3.0f, -6.0f }, { 4.0f, 0.0f });
            stalk.close();
            fillShape(onVerge, stalk, LinearGradient::Simple({ -4.0f, 0.0f }, { 4.0f, 0.0f },
                chase.stalk, chase.stalkShade));
            const float pi = std::numbers::pi_v<float>;
            const float onStalk = std::clamp((motion.sinceHareClose - 0.34f) / 0.24f, 0.0f, 1.0f);
            const float offStalk = std::clamp((motion.wolfTime - 1.32f) / 0.45f, 0.0f, 1.0f);
            const float settling = std::clamp((motion.wolfTime - 1.77f) / 0.15f, 0.0f, 1.0f);
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
                    SolidColor{ faded(chase.shadow, offStalk * offStalk) });
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
                chase.capLight, chase.capDark));
            const SolidColor spot{ chase.capSpot };
            fillOval(capFrame, { -8.0f, -20.0f }, 2.6f, 2.0f, spot);
            fillOval(capFrame, { 0.0f, -24.5f }, 2.2f, 1.6f, spot);
            fillOval(capFrame, { 7.5f, -20.0f }, 2.4f, 1.9f, spot);
            fillOval(capFrame, { -2.0f, -17.5f }, 1.5f, 1.2f, spot);
            fillOval(capFrame, { 11.5f, -16.0f }, 1.3f, 1.0f, spot);
        }

        // Four tufts from each head, torn off while the wolf hides the reeds, from where they would
        // be at a lean of 0.5, the height of his wake. They are carried after him, rising and then
        // sinking, and thin out to nothing.
        void drawReedFluff(const SceneFrame& onVerge, const ChaseTheme& chase, const float wolfTime)
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
                    const float seconds = wolfTime - (1.02f + 0.25f * pseudoRandom(seed, 131.0f));
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
                    const SolidColor fluff{ chase.fluff.withOpacity(1.0f
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
        void drawTreeStump(const SceneFrame& onVerge, const ChaseTheme& chase)
        {
            drawTrunk(onVerge, {}, 11.0f, 20.0f, chase.bark, chase.barkShade);
            fillOval(onVerge, { 0.0f, -20.0f }, 11.0f, 3.2f, SolidColor{ chase.stumpTop });
            PixelPath rings;
            addEllipse(rings, { 0.0f, -20.0f }, 7.5f, 2.1f);
            addEllipse(rings, { 0.0f, -20.0f }, 4.0f, 1.1f);
            strokeShape(onVerge, rings, SolidColor{ chase.stumpRing }, 0.7f);
            fillDisc(onVerge, { 0.3f, -20.0f }, 0.8f, SolidColor{ chase.stumpRing });
            drawSegment(onVerge, { 8.0f, -21.0f }, { 10.5f, -28.0f },
                SolidColor{ chase.stumpSprout }, 1.0f);
            fillOval(onVerge, { 12.5f, -28.5f }, 3.0f, 1.4f, SolidColor{ chase.leaf },
                -0.4f);
        }

        // A sparrow pecking at the verge; taking fright, it climbs away ahead of them, its wing
        // beating up over its back and down under its belly.
        void drawBird(const SceneFrame& onVerge, const ChaseTheme& chase, const float seconds,
            const float sinceHareClose)
        {
            const float flying = std::max(0.0f, sinceHareClose);
            const FloatPoint away = {
                420.0f * flying,
                -(110.0f * flying + 170.0f * flying * flying)
            };
            const float climb = -0.35f * std::min(1.0f, flying * 6.0f);
            const SceneFrame bird = onVerge.within(Matrix3x2::Translation(away)
                * Matrix3x2::Translation(0.0f, -4.0f) * Matrix3x2::Rotation(climb));
            const SolidColor body{ chase.birdBody };
            const SolidColor wing{ chase.birdWing };
            const float headDown = sinceHareClose > 0.0f
                ? 0.0f
                : bump(fractionalPart(seconds / 0.6f), 0.5f, 0.15f);
            if (sinceHareClose <= 0.0f)
            {
                const SolidColor legs{ chase.birdBeak };
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
            fillOval(bird, { 1.5f, -2.5f }, 4.5f, 2.8f, SolidColor{ chase.birdBelly });
            const FloatPoint headMiddle = { 6.0f + 2.0f * headDown, -9.0f + 5.0f * headDown };
            fillDisc(bird, headMiddle, 3.6f, body);
            fillDisc(bird, headMiddle + FloatPoint{ 1.2f, -0.6f }, 0.9f,
                SolidColor{ ChaseColors::Ink });
            PixelPath beak;
            beak.moveTo(headMiddle + FloatPoint{ 3.2f, -0.6f });
            beak.lineTo(headMiddle + FloatPoint{ 6.4f, 0.4f + 1.5f * headDown });
            beak.lineTo(headMiddle + FloatPoint{ 3.2f, 0.9f });
            beak.close();
            fillShape(bird, beak, SolidColor{ chase.birdBeak });
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

        // A crow sitting on the stump, pecking at it now and then; taking fright as the hare comes,
        // it springs up and labours off ahead of them, its legs hanging a moment.
        void drawStumpCrow(const SceneFrame& onVerge, const ChaseTheme& chase, const float seconds,
            const float sinceHareClose)
        {
            constexpr float k_size = 0.5f;
            const FloatPoint perch = { -1.0f, -20.0f - 13.0f * k_size };
            const float flying = std::max(0.0f, sinceHareClose);
            const bool sitting = sinceHareClose <= 0.0f;
            const FloatPoint middle = perch + FloatPoint{
                300.0f * flying,
                -(90.0f * flying + 200.0f * flying * flying)
            };
            const float pecking = sitting
                ? std::pow(std::max(0.0f, std::sin(k_2Pi * 0.7f * seconds)), 8.0f)
                : 0.0f;
            const float turn = sitting
                ? 0.3f - 0.45f * pecking
                : 0.4f - 0.25f * std::min(1.0f, flying * 2.0f);
            const float legsOut = sitting ? 1.0f : 1.0f - smoothStep(0.0f, 0.2f, flying);
            // facing right, the way the two run, so drawn turned over
            const auto onCrow = [&](FloatPoint local) {
                const FloatPoint turned = rotated(local, turn);
                return middle + FloatPoint{ -turned.x, turned.y } * k_size;
            };
            if (legsOut > 0.0f)
            {
                for (const float hipX : { -2.0f, 3.0f })
                {
                    const FloatPoint hip = onCrow({ hipX, 6.0f });
                    const FloatPoint foot = sitting
                        ? FloatPoint{ hip.x + 0.5f, -20.0f }
                        : hip + FloatPoint{ -1.0f, 3.0f };
                    const SolidColor leg{ chase.crowBeak };
                    drawSegment(onVerge, hip, foot, leg, 0.7f);
                    drawSegment(onVerge, foot, foot + FloatPoint{ 2.0f, 0.3f }, leg, 0.55f);
                    drawSegment(onVerge, foot, foot + FloatPoint{ -1.0f, 0.3f }, leg, 0.55f);
                }
            }
            drawCrowBody(onVerge.within(Matrix3x2::Translation(middle)
                * Matrix3x2::Scale(-k_size, k_size) * Matrix3x2::Rotation(turn)), chase,
                std::sin(k_2Pi * 7.0f * flying + 0.5f * std::numbers::pi_v<float>), 0.0f, sitting);
        }

        // Resting on a tuft, it opens and closes its wings; taking fright, it flutters up and
        // drifts off ahead of them.
        void drawButterfly(const SceneFrame& onVerge, const ChaseTheme& chase, const Color spots,
            const float seconds, const float sinceHareClose)
        {
            for (int blade = -1; blade <= 1; ++blade)
            {
                const float bladeX = static_cast<float>(blade);
                drawSegment(onVerge, { 1.5f * bladeX, 0.5f },
                    { 3.5f * bladeX, -7.0f - 2.0f * (1.0f - std::abs(bladeX)) },
                    SolidColor{ chase.grass }, 1.4f);
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
            const SolidColor edge{ chase.wingEdge };
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
                    fillShape(wings, *wing, SolidColor{ chase.wing });
                    strokeShape(wings, *wing, edge, 0.7f);
                }
                fillOval(wings, { 8.0f, -12.3f }, 2.3f, 1.5f, edge, -0.4f);
                fillDisc(wings, { 8.3f, -12.4f }, 0.6f, SolidColor{ spots });
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

        // Sitting by the path; taking fright, it hops twice back into the grass - from the far
        // verge away, smaller each time, from the near one toward the viewer, larger.
        void drawFrog(const SceneFrame& onVerge, const ChaseTheme& chase,
            const float sinceHareClose, const Verge verge)
        {
            struct Spot
            {
                FloatPoint at;
                float size;
            };
            using Spots = std::array<Spot, 3>;
            constexpr Spots k_farSpots{
                Spot{ { 0.0f, 0.0f }, 1.0f },
                Spot{ { 14.0f, -7.0f }, 0.78f },
                Spot{ { 24.0f, -13.0f }, 0.6f }
            };
            constexpr Spots k_nearSpots{
                Spot{ { 0.0f, 0.0f }, 1.0f },
                Spot{ { 14.0f, 7.0f }, 1.15f },
                Spot{ { 26.0f, 15.0f }, 1.3f }
            };
            const Spots& spots = verge == Verge::Near ? k_nearSpots : k_farSpots;
            constexpr float k_hopSeconds = 0.25f;
            Spot spot = spots.back();
            float stretch = 0.0f;
            float lift = 0.0f;
            for (std::size_t hop = 0; hop + 1 != spots.size(); ++hop)
            {
                const float takeoff = 0.05f + 0.32f * static_cast<float>(hop);
                const float progress = (sinceHareClose - takeoff) / k_hopSeconds;
                if (progress >= 1.0f)
                    continue;
                const float hopping = std::max(0.0f, progress);
                const Spot& from = spots[hop];
                const Spot& to = spots[hop + 1];
                spot = {
                    from.at + (to.at - from.at) * hopping,
                    std::lerp(from.size, to.size, hopping)
                };
                stretch = std::sin(std::numbers::pi_v<float> * hopping);
                lift = 16.0f * from.size * stretch;
                break;
            }
            fillOval(onVerge, spot.at + FloatPoint{ 1.0f, 0.0f }, 9.0f * spot.size,
                2.2f * spot.size, SolidColor{ chase.shadow });
            const SceneFrame frog = onVerge.within(Matrix3x2::Translation(spot.at.x,
                spot.at.y - lift) * Matrix3x2::Scale(spot.size)
                * Matrix3x2::Translation(0.0f, -6.0f) * Matrix3x2::Rotation(-0.45f * stretch)
                * Matrix3x2::Translation(0.0f, 6.0f));
            const SolidColor dark{ chase.frogDark };
            const FloatPoint hindFoot = FloatPoint{ -2.0f, -0.3f }
                + (FloatPoint{ -15.0f, 3.0f } - FloatPoint{ -2.0f, -0.3f }) * stretch;
            const FloatPoint foreFoot = FloatPoint{ 7.5f, 0.0f }
                + (FloatPoint{ 13.0f, -3.0f } - FloatPoint{ 7.5f, 0.0f }) * stretch;
            drawSegment(frog, { -5.0f, -5.0f }, hindFoot, dark, 2.6f);
            fillOval(frog, hindFoot, 3.5f, 1.1f, dark);
            fillOval(frog, { -5.0f, -4.5f }, 5.5f, 4.2f * (1.0f - 0.4f * stretch), dark, -0.3f);
            fillOval(frog, { 1.0f, -8.0f }, 8.5f, 6.2f, SolidColor{ chase.frogLight },
                -0.4f);
            fillOval(frog, { 4.0f, -5.5f }, 4.5f, 2.6f, SolidColor{ chase.frogBelly },
                -0.4f);
            drawSegment(frog, { 5.0f, -5.0f }, foreFoot, dark, 2.0f);
            fillOval(frog, foreFoot + FloatPoint{ 0.8f, 0.0f }, 2.2f, 0.9f, dark);
            fillDisc(frog, { 5.5f, -13.0f }, 3.2f, SolidColor{ chase.frogLight });
            fillDisc(frog, { 6.0f, -13.5f }, 2.2f, SolidColor{ ChaseColors::EyeShine });
            fillDisc(frog, { 6.7f, -13.4f }, 1.1f, SolidColor{ ChaseColors::Ink });
            drawSegment(frog, { 6.5f, -9.5f }, { 10.5f, -10.8f }, dark, 0.7f);
        }

        // Out of its shell, it draws in as the hare comes, and out again once the wolf is by.
        void drawSnail(const SceneFrame& onVerge, const ChaseTheme& chase,
            const RoadsideMotion& motion)
        {
            const float drawnIn = smoothStep(0.0f, 0.15f, motion.sinceHareClose)
                * (1.0f - smoothStep(1.3f, 1.9f, motion.wolfTime));
            const float out = 1.0f - drawnIn;
            const SolidColor body{ chase.snailBody };
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
                    fillDisc(onVerge, eye, 0.9f * out, SolidColor{ chase.shellLine });
                }
            }
            const FloatPoint shellMiddle = { -2.0f, -9.0f + 1.2f * drawnIn };
            fillDisc(onVerge, shellMiddle, 8.0f, LinearGradient::Simple(shellMiddle
                + FloatPoint{ -5.0f, -6.0f }, shellMiddle + FloatPoint{ 5.0f, 6.0f },
                chase.shellLight, chase.shellDark));
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
            strokeShape(onVerge, spiral, SolidColor{ chase.shellLine }, 1.0f);
        }

        // A clump of tall grass on the trail, the back blades darker. Each blade bends from its
        // root, its tip leaning furthest.
        void drawGrassPatch(const SceneFrame& onTrail, const ChaseTheme& chase,
            const float bladeLean)
        {
            struct Blade
            {
                float root;
                float height;
                float tilt;
                Color color;
            };
            const std::array<Blade, 11> blades{
                Blade{ -12.0f, 22.0f, -0.35f, chase.bladeDark },
                Blade{ -4.0f, 30.0f, -0.1f, chase.bladeDark },
                Blade{ 5.0f, 27.0f, 0.15f, chase.bladeDark },
                Blade{ 13.0f, 21.0f, 0.4f, chase.bladeDark },
                Blade{ -15.0f, 17.0f, -0.55f, chase.bladeMiddle },
                Blade{ -8.0f, 25.0f, -0.25f, chase.bladeMiddle },
                Blade{ 1.0f, 33.0f, 0.0f, chase.bladeMiddle },
                Blade{ 9.0f, 26.0f, 0.25f, chase.bladeMiddle },
                Blade{ -10.0f, 18.0f, -0.4f, chase.bladeLight },
                Blade{ -1.0f, 23.0f, -0.05f, chase.bladeLight },
                Blade{ 7.0f, 19.0f, 0.3f, chase.bladeLight }
            };
            for (const Blade& blade : blades)
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

        void drawRake(const SceneFrame& onTrail, const ChaseTheme& chase, const float swing)
        {
            const FloatPoint pivot = { -4.0f, -2.0f };
            const SceneFrame rake = onTrail.within(Matrix3x2::Translation(pivot)
                * Matrix3x2::Rotation(-swing) * Matrix3x2::Translation(-pivot.x, -pivot.y));
            fillRoundedBox(rake, { -4.0f, -4.0f, 106.0f, -1.0f }, 1.5f,
                LinearGradient::Simple({ 0.0f, -4.0f }, { 0.0f, -1.0f }, chase.rakeHandle,
                chase.rakeHandleShade));
            PixelPath tines;
            for (int tine = 0; tine != 6; ++tine)
            {
                const float x = -12.0f + 2.4f * static_cast<float>(tine);
                tines.moveTo(x, -3.0f);
                tines.lineTo(x - 0.6f, -10.0f);
            }
            strokeShape(rake, tines, SolidColor{ chase.rakeMetal }, 1.3f);
            fillRoundedBox(rake, { -13.0f, -4.5f, -1.0f, -1.5f }, 1.0f,
                SolidColor{ chase.rakeMetal });
        }

        // A tree whose lowest branch reaches back over the trail, as high as the head of a wolf in
        // the middle of a leap; `x` is where it ends. The tree he flies into has no low branch, and
        // on the one he leaps onto it reaches on from the trunk's far side instead.
        void drawBranchTree(const SceneFrame& frame, const SceneTheme& theme, const float x,
            const float groundY, const float footY, const ObstacleMotion& motion)
        {
            const TreeRun& treeRun = motion.treeRun;
            const float unit = frame.unit;
            const float trunkX = x + trunkFromBranchEnd(treeRun.leapEnd) * unit;
            const float endSway = motion.branchSway * unit;
            const float trunkFoot = footY + 2.0f * unit;
            drawTrunk(frame, { trunkX, trunkFoot }, k_trunkHalfWidth * unit,
                trunkFoot - (groundY - 180.0f * unit), theme.chase.bark, theme.chase.barkShade);
            if (treeRun.leapEnd != LeapEnd::Trunk)
            {
                // laid out reaching back to `x`, then over the branch turned about the trunk to
                // reach on
                const float branchTrunkX = x + k_trunkFromBranchEnd * unit;
                const SceneFrame onBranch = treeRun.leapEnd == LeapEnd::OverBranch
                    ? frame.within(Matrix3x2::Translation(trunkX + branchTrunkX, 0.0f)
                        * Matrix3x2::Scale(-1.0f, 1.0f))
                    : frame;
                PixelPath branch;
                branch.moveTo(branchTrunkX - 10.0f * unit, footY - 124.0f * unit);
                branch.quadTo({ x + 50.0f * unit, footY - 138.0f * unit + 0.5f * endSway },
                    { x - 6.0f * unit, footY - 150.0f * unit + endSway });
                branch.lineTo(x - 4.0f * unit, footY - 143.0f * unit + endSway);
                branch.quadTo({ x + 50.0f * unit, footY - 127.0f * unit + 0.5f * endSway },
                    { branchTrunkX - 10.0f * unit, footY - 110.0f * unit });
                branch.close();
                fillShape(onBranch, branch, SolidColor{ theme.chase.barkShade });
                fillDisc(onBranch, { x + 16.0f * unit, footY - 153.0f * unit + 0.8f * endSway },
                    9.0f * unit, SolidColor{ theme.chase.treeCrownLight });
            }
            const float shake = motion.crownShake * unit;
            fillDisc(frame, { trunkX + shake, groundY - 196.0f * unit }, 60.0f * unit,
                SolidColor{ theme.chase.treeCrown });
            fillDisc(frame, { trunkX - 40.0f * unit + shake, groundY - 170.0f * unit },
                40.0f * unit, SolidColor{ theme.chase.treeCrownLight });
            fillDisc(frame, { trunkX + 40.0f * unit + shake, groundY - 172.0f * unit },
                44.0f * unit, SolidColor{ theme.chase.treeCrownMiddle });

            // Shaken loose, an apple falls, bounces a third as high as its speed allows and lies
            // where it lands.
            const AppleFall appleFall = treeRun.appleFall;
            for (std::size_t index = 0; index != k_treeApples.size(); ++index)
            {
                if (motion.hareHasApple && index == k_hareApple)
                    continue;
                const TreeApple& apple = k_treeApples[index];
                const bool falls = (appleFall == AppleFall::Four && apple.amongFour)
                    || (appleFall == AppleFall::Six && apple.amongSix);
                FloatPoint middle = apple.onCrown + FloatPoint{ motion.crownShake, 0.0f };
                float spin = 0.0f;
                if (falls)
                {
                    const float seconds = (motion.sinceHit - apple.lettingGo) * k_episodeSeconds;
                    if (seconds > 0.0f)
                    {
                        middle = apple.onCrown;
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
                            const float sinceLanding = std::min(seconds - fallSeconds,
                                bounceSeconds);
                            middle.y = ground - bounceSpeed * sinceLanding
                                + 0.5f * k_appleGravity * sinceLanding * sinceLanding;
                        }
                        const float moving = std::min(1.0f, seconds
                            / (fallSeconds + bounceSeconds));
                        middle.x += apple.drift * moving;
                        spin = 2.5f * moving * (apple.drift < 0.0f ? -1.0f : 1.0f);
                    }
                }
                const FloatPoint onScreen = {
                    trunkX + middle.x * unit,
                    groundY + middle.y * unit
                };
                drawApple(frame.within(Matrix3x2::Translation(onScreen)
                    * Matrix3x2::Rotation(spin) * Matrix3x2::Scale(k_appleScale * unit)
                    * Matrix3x2::Translation(0.0f, k_appleMiddle)), theme.chase, 0.0f);
            }
        }

        FloatPoint restingPlace(const TreeApple& apple)
        {
            return { apple.onCrown.x + apple.drift, apple.landing - k_appleMiddle * k_appleScale };
        }

        float crowEyesWideAt(const float seconds)
        {
            if (seconds < k_crowEyesPop)
                return (1.0f + k_crowEyesPopOver) * seconds / k_crowEyesPop;
            const float settling = (seconds - k_crowEyesPop) / (k_crowEyesSettled - k_crowEyesPop);
            return 1.0f + k_crowEyesPopOver * std::max(0.0f, 1.0f - settling);
        }

        // A crow, side on and facing left, flapping, about the middle of its body; its wings beat
        // up over its back and down under its belly.
        void drawCrowBody(const SceneFrame& crow, const ChaseTheme& chase, const float flap,
            const float eyesWide, const bool folded)
        {
            const auto drawWing = [&](float lean, const Color& color) {
                const FloatPoint shoulder = { 0.0f, -4.0f };
                const FloatPoint tip = shoulder + FloatPoint{ 16.0f, -26.0f * lean };
                PixelPath wing;
                wing.moveTo(shoulder + FloatPoint{ -6.0f, 0.0f });
                wing.quadTo(shoulder + FloatPoint{ 2.0f, -16.0f * lean }, tip);
                wing.lineTo(tip + FloatPoint{ 7.0f, 4.0f * lean });
                wing.quadTo(shoulder + FloatPoint{ 14.0f, -4.0f * lean },
                    shoulder + FloatPoint{ 10.0f, 2.0f });
                wing.close();
                fillShape(crow, wing, SolidColor{ color });
            };
            if (!folded)
                drawWing(0.75f * flap + 0.2f, chase.crowWingFar);
            PixelPath tail;
            tail.moveTo(12.0f, -3.0f);
            tail.lineTo(30.0f, -8.0f);
            tail.lineTo(32.0f, 2.0f);
            tail.lineTo(13.0f, 4.0f);
            tail.close();
            fillShape(crow, tail, SolidColor{ chase.crowWing });
            fillOval(crow, { 2.0f, 0.0f }, 15.0f, 8.5f, SolidColor{ chase.crowBody },
                -0.05f);
            fillDisc(crow, { -13.0f, -4.0f }, 7.0f, SolidColor{ chase.crowBody });
            PixelPath beak;
            beak.moveTo(-18.0f, -7.0f);
            beak.lineTo(-29.0f, -3.5f);
            beak.lineTo(-18.0f, -1.0f);
            beak.close();
            fillShape(crow, beak, SolidColor{ chase.crowBeak });
            if (eyesWide > 0.0f)
            {
                // Both eyes, each a white disc on a rim of ink with a pinpoint pupil, bulging a
                // little past the head; the far one comes out from behind it as they widen.
                const SolidColor white{ ChaseColors::EyeShine };
                const SolidColor ink{ ChaseColors::Ink };
                const auto drawEye = [&](FloatPoint middle, float radius, FloatPoint pupilOffset) {
                    fillDisc(crow, middle, radius + 0.6f, ink);
                    fillDisc(crow, middle, radius, white);
                    fillDisc(crow, middle + pupilOffset, 1.1f - 0.25f * eyesWide, ink);
                };
                if (eyesWide > 0.25f)
                {
                    drawEye({ -9.6f + 0.6f * eyesWide, -6.2f - 0.8f * eyesWide },
                        2.0f + 1.5f * eyesWide, { -0.3f * eyesWide, 0.4f * eyesWide });
                }
                drawEye({ -14.0f - 0.7f * eyesWide, -6.0f - 0.7f * eyesWide },
                    2.0f + 2.5f * eyesWide, { -0.6f * eyesWide, 0.3f * eyesWide });
            }
            else
            {
                fillDisc(crow, { -14.0f, -6.0f }, 2.0f, SolidColor{ ChaseColors::EyeShine });
                fillDisc(crow, { -14.6f, -6.0f }, 1.1f, SolidColor{ ChaseColors::Ink });
            }
            if (folded)
            {
                // tucked along its back, the tips lying over the tail
                PixelPath wing;
                wing.moveTo(-7.0f, -6.0f);
                wing.quadTo({ 8.0f, -11.0f }, { 25.0f, -4.0f });
                wing.quadTo({ 10.0f, 3.0f }, { -5.0f, 0.0f });
                wing.close();
                fillShape(crow, wing, SolidColor{ chase.crowWing });
            }
            else
            {
                drawWing(flap, chase.crowWing);
            }
        }

        void drawCrowStump(const SceneFrame& frame, const ChaseTheme& chase, const float x,
            const float groundY)
        {
            const SceneFrame onVerge = frame.within(Matrix3x2::Translation(
                x + k_crowStumpFromBranchEnd * frame.unit, groundY + k_farVergeDrop * frame.unit)
                * Matrix3x2::Scale(k_crowStumpSize * frame.unit));
            fillOval(onVerge, { 1.0f, 0.0f }, 19.0f, 2.5f, SolidColor{ chase.shadow });
            drawTreeStump(onVerge, chase);
        }

        // Flying, it comes in from ahead at the height of his head, 160 units a second over the
        // ground. On its stump it sits pecking, looks up as the two come and flaps up into his
        // leap. Meeting his face, it tumbles back with feathers flying and its eyes popped wide,
        // then flaps off up and away, shaken, its eyes settling as the shaking ends; the feathers
        // drift down round him and lie where they land.
        void drawCrow(const SceneFrame& frame, const ChaseTheme& chase, const float x,
            const float groundY, const float footY, const float sinceHit, const CrowStart crowStart)
        {
            constexpr float k_flightSpeed = 160.0f;
            constexpr float k_tumbleSeconds = 0.5f;
            constexpr int k_featherCount = 7;
            const float pi = std::numbers::pi_v<float>;
            const float unit = frame.unit;
            const float seconds = sinceHit * k_episodeSeconds;
            const FloatPoint contact = { x + 22.0f * unit, footY - 146.0f * unit };
            const float wingBeats = 5.0f;
            FloatPoint middle = contact;
            float turn = 0.0f;
            float facing = 1.0f;
            float flap = std::sin(k_2Pi * wingBeats * seconds);
            float eyesWide = 0.0f;
            bool folded = false;
            // its legs, out while it stands and as it takes off, and the stump top under its feet
            float legsOut = 0.0f;
            float stumpTop = 0.0f;
            if (seconds < 0.0f && crowStart == CrowStart::Flying)
            {
                middle.x -= k_flightSpeed * seconds * unit;
                middle.y += 3.0f * std::sin(k_2Pi * wingBeats * seconds + 1.0f) * unit;
            }
            else if (seconds < 0.0f)
            {
                stumpTop = groundY + (k_farVergeDrop - 20.0f * k_crowStumpSize) * unit;
                const FloatPoint perch = {
                    x + (k_crowStumpFromBranchEnd - 3.0f) * unit,
                    stumpTop - 13.0f * unit
                };
                const float rising = std::clamp(1.0f - seconds / k_crowTakeoff, 0.0f, 1.0f);
                if (rising <= 0.0f)
                {
                    // a peck at the stump now and then, until it looks up at the two coming
                    const float pecking = std::pow(std::max(0.0f, std::sin(k_2Pi * 0.7f * seconds)),
                        8.0f) * (1.0f - smoothStep(-2.2f, -1.6f, seconds));
                    const float crouch = smoothStep(k_crowTakeoff - 0.14f, k_crowTakeoff - 0.02f,
                        seconds);
                    middle = perch + FloatPoint{ 0.0f, 3.0f * crouch } * unit;
                    turn = 0.3f - 0.45f * pecking + 0.15f * smoothStep(-2.2f, -1.6f, seconds)
                        - 0.3f * crouch;
                    folded = true;
                    legsOut = 1.0f;
                }
                else
                {
                    // up off the stump fast, and slowing as it climbs, its legs left hanging
                    const float climbing = 1.0f - (1.0f - rising) * (1.0f - rising);
                    middle = FloatPoint{
                        std::lerp(perch.x, contact.x, rising),
                        std::lerp(perch.y, contact.y, climbing)
                    };
                    turn = 0.35f - 0.25f * rising;
                    flap = std::sin(k_2Pi * 8.0f * (seconds - k_crowTakeoff) + 0.5f * pi);
                    legsOut = 1.0f - smoothStep(0.0f, 0.3f, rising);
                }
            }
            else if (seconds < k_tumbleSeconds)
            {
                // knocked back and over, wings flung about
                const float tumbling = seconds / k_tumbleSeconds;
                middle += FloatPoint{ 50.0f * tumbling, 30.0f * tumbling * tumbling } * unit;
                turn = -1.6f * k_2Pi * (1.0f - (1.0f - tumbling) * (1.0f - tumbling));
                flap = std::sin(k_2Pi * 3.0f * seconds);
                eyesWide = crowEyesWideAt(seconds);
            }
            else
            {
                // off up and away the way it came, wobbling
                const float away = seconds - k_tumbleSeconds;
                middle += FloatPoint{ 50.0f + 190.0f * away, 30.0f - 110.0f * away
                    + 6.0f * std::sin(k_2Pi * 2.0f * away) } * unit;
                turn = 0.25f * std::sin(k_2Pi * 1.5f * away) - 0.2f;
                facing = -1.0f;
                flap = std::sin(k_2Pi * 7.0f * away);
                eyesWide = 1.0f - smoothStep(k_crowShaken - 0.15f, k_crowShaken + 0.15f, away);
            }
            if (legsOut > 0.0f)
            {
                for (const float hipX : { -2.0f, 3.0f })
                {
                    const FloatPoint hip = middle + rotated(FloatPoint{ hipX, 6.0f }, turn) * unit;
                    const FloatPoint hanging = hip + FloatPoint{ 2.0f, 6.0f } * unit;
                    const FloatPoint standing = { hip.x - 1.0f * unit, stumpTop - 1.0f * unit };
                    const FloatPoint foot = folded ? standing : pointBetween(hip, hanging, legsOut);
                    const SolidColor leg{ chase.crowBeak };
                    drawSegment(frame, hip, foot, leg, 1.4f * unit);
                    drawSegment(frame, foot, foot + FloatPoint{ -4.0f, 0.5f } * unit, leg,
                        1.1f * unit);
                    drawSegment(frame, foot, foot + FloatPoint{ 2.0f, 0.5f } * unit, leg,
                        1.1f * unit);
                }
            }
            const float onScreenRight = frame.width + 60.0f * unit;
            if (middle.x < onScreenRight && middle.y > -60.0f * unit)
            {
                drawCrowBody(frame.within(Matrix3x2::Translation(middle)
                    * Matrix3x2::Rotation(turn) * Matrix3x2::Scale(facing * unit, unit)), chase,
                    flap, eyesWide, folded);
            }
            if (seconds <= 0.0f)
                return;
            // Each feather drifts down at its own pace, swinging from side to side, and lies flat
            // on the trail once it reaches it.
            for (int feather = 0; feather != k_featherCount; ++feather)
            {
                const float order = static_cast<float>(feather);
                const float outward = -0.5f * pi + 2.6f * (pseudoRandom(feather, 171.0f) - 0.5f);
                const float burst = 40.0f + 50.0f * pseudoRandom(feather, 172.0f);
                const float sinking = 22.0f + 14.0f * pseudoRandom(feather, 173.0f);
                const float landing = groundY + (12.0f + 18.0f * pseudoRandom(feather, 174.0f))
                    * unit;
                const float spread = std::min(seconds, 0.4f);
                FloatPoint at = contact + FloatPoint{
                    std::cos(outward) * burst * (spread - 0.5f * spread * spread / 0.4f),
                    std::sin(outward) * burst * (spread - 0.5f * spread * spread / 0.4f)
                        + sinking * seconds
                } * unit;
                const float swing = std::sin(k_2Pi * 0.9f * seconds + order);
                at.x += 9.0f * swing * unit;
                const bool landed = at.y >= landing;
                at.y = std::min(at.y, landing);
                const float lie = landed ? 0.1f * order - 0.3f : 0.5f * swing + order;
                fillOval(frame, at, 8.0f * unit, 2.4f * unit,
                    SolidColor{ feather % 2 == 0 ? chase.crowBody : chase.crowWing },
                    lie);
                drawSegment(frame, at - rotated(FloatPoint{ 9.5f, 0.0f }, lie) * unit,
                    at + rotated(FloatPoint{ 6.0f, 0.0f }, lie) * unit,
                    SolidColor{ chase.crowBeak }, 0.7f * unit);
            }
        }
    }
}
