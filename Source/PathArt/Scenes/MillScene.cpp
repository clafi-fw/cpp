module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.BroadleafTreeSprite;
import ClaFi.PathArt.CelestialSprite;
import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.GatorHeadSprite;
import ClaFi.PathArt.GrassPatchSprite;
import ClaFi.PathArt.LightningEffect;
import ClaFi.PathArt.PeekingWolfSprite;
import ClaFi.PathArt.PosedCowSprite;
import ClaFi.PathArt.TimberMillSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SpruceSprite;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WaterLilySprite;
import ClaFi.PathArt.WaterReflection;
import ClaFi.PathArt.WeatherEffect;
import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Graphics::Cpu;

    namespace
    {
        // A point of a stream's middle line and how wide it is there, in shares of the frame.
        struct StreamPoint
        {
            FloatPoint share;
            float widthShare;
        };

        // A mountain's peak, in shares of the frame.
        struct Peak
        {
            float centerShare;
            float heightShare;
            float leftWidthShare; // how far it falls away to its left
            float rightWidthShare;
        };

        // One range of the mountains and how far it stands.
        struct MountainRange
        {
            float parallax;
            float roughnessPhase; // where the small ridges along it fall
            std::span<const Peak> peaks;
        };

        // A tree standing on the land, its foot in shares of the frame.
        struct StandingTree
        {
            FloatPoint footShare;
            float size; // of the tree at the mill's distance, in units of the frame
        };

        // A stretch of the story, from one of its seconds to another.
        struct Stretch
        {
            float from;
            float until;
        };

        // Where a grove stands on the far hills, in shares of the width.
        struct Grove
        {
            float fromShare;
            float untilShare;
        };

        // A fish leaping out of the pond and back in.
        struct FishJump
        {
            float at; // the story's second it leaves the water
            FloatPoint share; // where, in shares of the frame
        };

        // How the cow stands at a moment of the story, and the rings its drinking spreads.
        struct CowMoment
        {
            PosedCowPose pose;
            std::optional<float> ripplesAge; // seconds since its muzzle last broke the water
        };

        // One of the depths the theme's weather and fireflies are shared out over.
        struct EffectLayer
        {
            float bottomShare; // the row its weather falls down to, in shares of the frame
            float density; // its pieces to the area, 1 being the usual count over the frame
            float size; // of its pieces, against what falls by the mill
            float opacity; // of its weather
            float firefliesTopShare; // the band its fireflies wander in, in shares of the frame
            float firefliesBottomShare;
            int fireflies;
        };

        // What a duck wears.
        struct DuckColors
        {
            Color body;
            Color wing;
            Color beak;
            Color eye;
            Color wake;
        };

        // Where one of the ducks swims at a moment of the story.
        struct SwimmingDuck
        {
            FloatPoint at; // the middle of its waterline
            float size;
            float paddle; // its head's bob, from -1 to 1
        };

        // The mother first, then her three ducklings.
        using DuckFamily = std::array<SwimmingDuck, 4>;

        // Where a duck keeps behind the mother, and how it falls back and catches up about there.
        struct DuckPlace
        {
            float behind; // in the mother's units
            float drift; // in the mother's units, either way
            float driftPace; // radians a second of the story
            float driftPhase;
        };

        // A run of grass blades along the near bank.
        struct BladeRun
        {
            float fromShare; // of the frame's width
            float toShare;
            int count;
            float seed;
        };

        // A cattail on a bank: where along it, how tall, and where its sway starts.
        struct Cattail
        {
            float xShare;
            float height; // in units at its distance
            float swayPhase;
        };

        // A cattail in a reed bed.
        struct ReedBedCattail
        {
            float offset; // in its bed's half widths, right of the bed's middle
            float height; // in units at its distance
            float swayPhase;
        };

        // Tall grass and the cattails of k_reedBedCattails on a bank of the river.
        struct ReedBed
        {
            FloatPoint share; // its middle at its roots
            float scale; // against the size of things at its distance
            float halfWidthShare; // of the frame's width
            int bladeCount;
            float seed; // sets its blades apart from another bed's, and its sway
            bool cattailsFlipped; // its cattails stand mirrored across its middle
        };

        // A cattail's stem, which its leaves take, and its head.
        struct CattailColors
        {
            Color stem;
            Color head;
        };

        // Where a daisy stands on the near bank, and how far under the bank's top, in units.
        struct BankFlower
        {
            float xShare;
            float depth;
            int petal; // 0 to 2, which of the theme's petals it wears
        };

        // The far hills' foot, where the valley floor starts, as a share of the frame's height.
        constexpr float k_horizonShare{ 0.56f };
        constexpr float k_vanishingShare{ 0.542f }; // where the valley floor's rows would meet
        constexpr FloatPoint k_millFootShare{ 0.668f, 0.80f };
        constexpr float k_millSize{ 1.03f }; // units of the frame per unit of the mill

        constexpr float k_mountainTopShare{ 0.28f }; // where mountainTop sits, above every peak
        constexpr std::size_t k_mountainHazeSteps{ 12 };
        constexpr std::array<Peak, 11> k_farPeaks{
            Peak{ -0.02f, 0.15f, 0.06f, 0.08f },
            Peak{ 0.09f, 0.20f, 0.07f, 0.05f },
            Peak{ 0.15f, 0.16f, 0.03f, 0.06f },
            Peak{ 0.27f, 0.13f, 0.07f, 0.05f },
            Peak{ 0.38f, 0.17f, 0.06f, 0.09f },
            Peak{ 0.53f, 0.25f, 0.10f, 0.06f },
            Peak{ 0.585f, 0.20f, 0.03f, 0.07f },
            Peak{ 0.69f, 0.15f, 0.05f, 0.04f },
            Peak{ 0.80f, 0.19f, 0.07f, 0.04f },
            Peak{ 0.86f, 0.225f, 0.03f, 0.08f },
            Peak{ 1.01f, 0.16f, 0.07f, 0.05f }
        };
        constexpr std::array<Peak, 7> k_nearPeaks{
            Peak{ 0.04f, 0.085f, 0.07f, 0.10f },
            Peak{ 0.24f, 0.115f, 0.09f, 0.06f },
            Peak{ 0.33f, 0.08f, 0.04f, 0.08f },
            Peak{ 0.48f, 0.07f, 0.08f, 0.07f },
            Peak{ 0.68f, 0.12f, 0.10f, 0.07f },
            Peak{ 0.76f, 0.095f, 0.03f, 0.06f },
            Peak{ 0.95f, 0.10f, 0.07f, 0.08f }
        };
        constexpr MountainRange k_farRange{ 0.02f, 1.3f, k_farPeaks };
        constexpr MountainRange k_nearRange{ 0.05f, 2.1f, k_nearPeaks };

        constexpr float k_farHillsParallax{ 0.07f };
        // The stretches of the hills that show between what stands nearer.
        constexpr std::array<Grove, 4> k_farGroves{ {
            { 0.185f, 0.288f },
            { 0.321f, 0.401f },
            { 0.431f, 0.608f },
            { 0.748f, 0.894f }
        } };
        // A grove's tree count, were it as wide as the frame.
        constexpr float k_groveTreesPerWidth{ 150.0f };
        constexpr int k_groveTreesMost{ 64 }; // above any grove's count, so no seed is shared
        // How far down its hill a grove reaches, as a share of the hill's height, above the knolls.
        constexpr float k_groveSlopeReach{ 0.7f };
        constexpr float k_knollParallax{ 0.4f }; // the knolls' tops; their feet take the floor's
        constexpr std::array<FloatPoint, 10> k_leftKnoll{
            FloatPoint{ -0.03f, 0.70f },
            FloatPoint{ -0.02f, 0.585f },
            FloatPoint{ 0.05f, 0.556f },
            FloatPoint{ 0.11f, 0.55f },
            FloatPoint{ 0.17f, 0.566f },
            FloatPoint{ 0.225f, 0.60f },
            FloatPoint{ 0.255f, 0.64f },
            FloatPoint{ 0.262f, 0.70f },
            FloatPoint{ 0.20f, 0.80f },
            FloatPoint{ 0.0f, 0.85f }
        };
        constexpr std::array<FloatPoint, 9> k_rightKnoll{
            FloatPoint{ 0.50f, 0.75f },
            FloatPoint{ 0.585f, 0.66f },
            FloatPoint{ 0.65f, 0.605f },
            FloatPoint{ 0.73f, 0.566f },
            FloatPoint{ 0.82f, 0.546f },
            FloatPoint{ 0.92f, 0.55f },
            FloatPoint{ 1.03f, 0.57f },
            FloatPoint{ 1.04f, 0.80f },
            FloatPoint{ 0.80f, 0.80f }
        };
        constexpr int k_meadowTufts{ 100 };
        constexpr int k_meadowFlowers{ 50 };
        constexpr int k_meadowDepthBands{ 4 }; // the rows the meadow's specks are hazed in
        constexpr std::array<StreamPoint, 9> k_footpath{
            StreamPoint{ { 0.13f, 0.572f }, 0.002f },
            StreamPoint{ { 0.155f, 0.585f }, 0.004f },
            StreamPoint{ { 0.185f, 0.605f }, 0.006f },
            StreamPoint{ { 0.215f, 0.628f }, 0.008f },
            StreamPoint{ { 0.245f, 0.648f }, 0.009f },
            StreamPoint{ { 0.29f, 0.651f }, 0.008f },
            StreamPoint{ { 0.33f, 0.656f }, 0.008f },
            StreamPoint{ { 0.355f, 0.664f }, 0.006f },
            StreamPoint{ { 0.38f, 0.672f }, 0.0f }
        };

        // The river from the far hills' crest under the mountains, down to the pond.
        constexpr std::array<StreamPoint, 11> k_upstream{
            StreamPoint{ { 0.393f, 0.5472f }, 0.0f },
            StreamPoint{ { 0.405f, 0.5485f }, 0.0012f },
            StreamPoint{ { 0.414f, 0.552f }, 0.002f },
            StreamPoint{ { 0.414f, 0.558f }, 0.003f },
            StreamPoint{ { 0.405f, 0.5645f }, 0.0045f },
            StreamPoint{ { 0.39f, 0.572f }, 0.006f },
            StreamPoint{ { 0.36f, 0.59f }, 0.013f },
            StreamPoint{ { 0.322f, 0.613f }, 0.021f },
            StreamPoint{ { 0.292f, 0.643f }, 0.03f },
            StreamPoint{ { 0.278f, 0.677f }, 0.04f },
            StreamPoint{ { 0.29f, 0.712f }, 0.05f }
        };
        // The pond's far shore, from the river's mouth to past the frame's right side.
        constexpr std::array<FloatPoint, 8> k_farShore{
            FloatPoint{ 0.345f, 0.742f },
            FloatPoint{ 0.42f, 0.764f },
            FloatPoint{ 0.52f, 0.782f },
            FloatPoint{ 0.62f, 0.795f },
            FloatPoint{ 0.72f, 0.803f },
            FloatPoint{ 0.84f, 0.81f },
            FloatPoint{ 0.95f, 0.815f },
            FloatPoint{ 1.04f, 0.817f }
        };
        // The pond's left shore, from past the frame's left side up to the river's mouth.
        constexpr std::array<FloatPoint, 5> k_leftShore{
            FloatPoint{ -0.04f, 0.85f },
            FloatPoint{ 0.06f, 0.838f },
            FloatPoint{ 0.15f, 0.806f },
            FloatPoint{ 0.215f, 0.772f },
            FloatPoint{ 0.25f, 0.748f }
        };
        constexpr std::array<FloatPoint, 11> k_nearBank{
            FloatPoint{ -0.03f, 0.862f },
            FloatPoint{ 0.07f, 0.866f },
            FloatPoint{ 0.16f, 0.882f },
            FloatPoint{ 0.25f, 0.912f },
            FloatPoint{ 0.34f, 0.952f },
            FloatPoint{ 0.46f, 0.975f },
            FloatPoint{ 0.58f, 0.968f },
            FloatPoint{ 0.68f, 0.94f },
            FloatPoint{ 0.77f, 0.905f },
            FloatPoint{ 0.87f, 0.888f },
            FloatPoint{ 1.03f, 0.884f }
        };
        constexpr FloatPoint k_bridgeFootShare{ 0.287f, 0.652f };
        constexpr float k_bridgeSize{ 1.63f };
        // How the pond breaks what it mirrors, in units of the frame, and how much it shows.
        constexpr float k_farRippleReach{ 0.4f };
        constexpr float k_nearRippleReach{ 4.0f };
        constexpr float k_reflectionOpacity{ 0.42f };

        // Back to front, behind the broadleaf trees.
        constexpr std::array<StandingTree, 2> k_knollFirs{
            StandingTree{ { 0.09f, 0.655f }, 0.8f },
            StandingTree{ { 0.155f, 0.665f }, 0.9f }
        };
        // Back to front, in front of the river bed; the pond mirrors them.
        constexpr std::array<StandingTree, 3> k_shoreFirs{
            StandingTree{ { 0.118f, 0.778f }, 0.85f },
            StandingTree{ { 0.79f, 0.785f }, 0.9f },
            StandingTree{ { 0.045f, 0.812f }, 1.0f }
        };
        constexpr std::array<StandingTree, 2> k_broadleaves{
            StandingTree{ { 0.205f, 0.672f }, 0.95f },
            StandingTree{ { 0.53f, 0.70f }, 0.85f }
        };
        constexpr FloatPoint k_cowFootShare{ 0.458f, 0.742f };
        constexpr float k_cowSize{ 0.74f };
        constexpr float k_cowTuftSize{ 0.8f }; // of the cow's units to one of the grass patch's
        // The grazing cow's motions, each a whole number of times a cycle.
        constexpr float k_cowNods{ 19.0f };
        constexpr float k_cowChews{ 64.0f };
        constexpr float k_cowSwishes{ 15.0f };
        // How far the tuft's tips swing toward her muzzle as she takes a bite, and the share of
        // that swing kept while her jaw is open.
        constexpr float k_cowTuftPull{ 0.4f };
        constexpr float k_cowTuftHeld{ 0.35f };
        // How far across the frame, in her units, she stands from her grazing place before her
        // muzzle stops reaching the tuft.
        constexpr float k_cowTuftReach{ 60.0f };
        // The fir the predator peeks from behind, across the field.
        constexpr StandingTree k_wolfFir{ { 0.884f, 0.70f }, 1.0f };
        // Far to near: the far hills, the field, the mill, the near bank. See PathArt#millscene
        constexpr std::array<EffectLayer, 4> k_effectLayers{ {
            { k_horizonShare, 0.4f, 0.5f, 0.6f, 0.5f, 0.555f, 10 },
            { 0.73f, 0.3f, 0.75f, 0.75f, 0.58f, 0.72f, 8 },
            { 0.87f, 0.3f, 1.0f, 0.9f, 0.74f, 0.86f, 6 },
            { 1.0f, 0.4f, 1.3f, 1.0f, 0.86f, 0.97f, 4 }
        } };
        // More than the 9 an effect reads past its own seed, so no two layers share a place.
        constexpr float k_effectSeedStep{ 10.0f };
        // The story's length in its own seconds, two cycles of 40.
        constexpr float k_storySeconds{ 80.0f };
        // Where the cow walks to drink, on the pond's far shore by the river's mouth.
        constexpr FloatPoint k_cowDrinkShare{ 0.375f, 0.747f };
        // From 40 s on past the story's end, to 16 s into the next.
        constexpr Stretch k_ducksSwim{ 40.0f, 96.0f };
        // The last duckling drifts the most, and keeps back far enough to miss the one ahead.
        constexpr std::array<DuckPlace, 4> k_duckPlaces{
            DuckPlace{ 0.0f, 0.0f, 0.0f, 0.0f },
            DuckPlace{ -46.0f, 5.0f, 0.85f, 0.5f },
            DuckPlace{ -84.0f, 7.0f, 0.65f, 2.6f },
            DuckPlace{ -130.0f, 14.0f, 0.5f, 4.4f }
        };
        constexpr float k_motherSway{ 0.67f }; // in units, either way across her course
        constexpr float k_ducklingSway{ 2.4f }; // in units, either way across its course
        constexpr Stretch k_birdsFly{ 46.0f, 72.0f };
        constexpr std::array<FishJump, 2> k_fishJumps{ {
            { 14.0f, { 0.53f, 0.88f } },
            { 58.0f, { 0.46f, 0.85f } }
        } };
        // Every 20 s the wolf's eyes shut for 4; every 5 s his left ear flicks up for 0.5 s.
        constexpr float k_wolfBlinkSeconds{ 20.0f };
        constexpr Stretch k_wolfEyesShut{ 10.5f, 14.5f }; // within each of k_wolfBlinkSeconds
        constexpr float k_wolfEarFlickSeconds{ 5.0f };
        constexpr float k_wolfEarFlicked{ 0.5f }; // seconds at the start of each flick
        constexpr float k_wolfEarFlick{ 0.2f }; // turns the flick takes his ear up by
        constexpr float k_wolfEarLift{ 0.06f }; // turns his ear rests up by, clear of the branches
        constexpr float k_wolfSize{ 0.8f }; // in units of the frame at his fir's distance
        constexpr FloatPoint k_lilyShare{ 0.487f, 0.958f }; // just off the near bank's lowest dip
        constexpr float k_lilyFlatness{ 0.35f }; // its height against its width, lying on the water
        constexpr float k_lilySize{ 0.62f };
        constexpr std::array<Cattail, 9> k_cattails{
            Cattail{ 0.03f, 120.0f, 0.0f },
            Cattail{ 0.065f, 150.0f, 1.0f },
            Cattail{ 0.1f, 110.0f, 2.2f },
            Cattail{ 0.14f, 135.0f, 0.7f },
            Cattail{ 0.215f, 95.0f, 1.6f },
            Cattail{ 0.83f, 130.0f, 0.4f },
            Cattail{ 0.87f, 105.0f, 1.9f },
            Cattail{ 0.93f, 145.0f, 2.6f },
            Cattail{ 0.97f, 118.0f, 1.1f }
        };
        // The near bank's top color, this share of the way from the hills' top to their foot, as
        // the Chase's lawn is at that row; it reaches the foot's color at the frame's bottom.
        constexpr float k_nearBankTopShare{ 0.63f };
        // At the water's edge on the pond's left shore, across from the near bank's.
        constexpr std::array<Cattail, 3> k_leftShoreCattails{
            Cattail{ 0.168f, 84.0f, 0.9f },
            Cattail{ 0.186f, 66.0f, 2.3f },
            Cattail{ 0.204f, 78.0f, 1.4f }
        };
        // At the water's edge on the far shore, between the cow and the wheel.
        constexpr std::array<Cattail, 3> k_farShoreCattails{
            Cattail{ 0.497f, 70.0f, 0.3f },
            Cattail{ 0.512f, 92.0f, 1.7f },
            Cattail{ 0.528f, 64.0f, 2.9f }
        };
        // The cattails across the pond against the near bank's, for being further off.
        constexpr float k_shoreCattailScale{ 0.7f };
        constexpr std::array<ReedBed, 5> k_reedBeds{
            ReedBed{ { 0.4097f, 0.5552f }, 1.25f, 0.0025f, 12, 0.0f, false }, // the far bend's tip
            ReedBed{ { 0.251f, 0.6724f }, 0.55f, 0.012f, 12, 1.0f, true }, // the bridge's left end
            ReedBed{ { 0.3148f, 0.6714f }, 0.55f, 0.012f, 12, 2.0f, false }, // its right end
            ReedBed{ { 0.337f, 0.5843f }, 1.0f, 0.0065f, 12, 3.0f, true }, // the reach's far bank
            ReedBed{ { 0.3765f, 0.5985f }, 1.0f, 0.0083f, 12, 4.0f, false } // the reach's near bank
        };
        constexpr std::array<ReedBedCattail, 3> k_reedBedCattails{
            ReedBedCattail{ -0.44f, 78.0f, 0.0f },
            ReedBedCattail{ 0.144f, 92.0f, 1.3f },
            ReedBedCattail{ 0.64f, 70.0f, 2.6f }
        };
        constexpr std::array<BladeRun, 2> k_bladeRuns{
            BladeRun{ -0.02f, 0.36f, 120, 131.0f },
            BladeRun{ 0.66f, 1.02f, 110, 141.0f }
        };
        constexpr std::array<BankFlower, 6> k_bankFlowers{
            BankFlower{ 0.17f, 30.0f, 0 },
            BankFlower{ 0.235f, 40.0f, 1 },
            BankFlower{ 0.087f, 56.0f, 2 },
            BankFlower{ 0.72f, 36.0f, 1 },
            BankFlower{ 0.78f, 26.0f, 0 },
            BankFlower{ 0.9f, 30.0f, 2 }
        };

        [[nodiscard]] TimberMillColors millColorsOf(const SceneTheme&);
        // Where the mill stands in the frame, its wings turning once a cycle and its wheel twice.
        [[nodiscard]] TimberMillPose millPoseOf(const SceneFrame&, float cycleTime);
        // Seconds into the scene's story, which runs over two cycles.
        [[nodiscard]] float storyAt(SceneTime);
        // The cow's day: a walk to the water, a drink, the walk back, grazing, and a look up.
        [[nodiscard]] CowMoment cowAt(const SceneFrame&, float story, float cycleTime);
        [[nodiscard]] PosedCowColors cowColorsOf(const SceneTheme&);
        // The tuft the cow munches, under her muzzle where she grazes: the Chase's grass patch.
        void drawCowTuft(const SceneFrame&, const SceneTheme&, const PosedCowPose&);
        // A mother duck and three ducklings crossing the pond, while they are out.
        [[nodiscard]] std::optional<DuckFamily> ducksAt(const SceneFrame&, float story);
        // The mother's colors for index 0, a duckling's for the rest.
        [[nodiscard]] DuckColors duckColorsOf(const SceneTheme&, std::size_t duckIndex);
        // The parallax of the valley floor at `y`, the mill's foot being 1.
        [[nodiscard]] float parallaxAt(const SceneFrame&, float y);
        // Pixels per unit of the frame for what stands at `y`, at its distance.
        [[nodiscard]] float sizeAt(const SceneFrame&, float y);
        // The land's clear color at row `y`.
        [[nodiscard]] Color landAt(const SceneFrame&, const SceneTheme&, float y);
        // The land from `top` down, each row hazed as far as it lies, and at least as `parallax`.
        [[nodiscard]] LinearGradient landGradient(const SceneFrame&, const SceneTheme&, float top,
            float parallax);
        [[nodiscard]] FloatPoint pointAt(const SceneFrame&, FloatPoint share);
        // The index-th of count shares spread evenly over 0 to 1, each moved within its stretch.
        [[nodiscard]] float spreadShare(int index, int count, float seed);
        // The frame upside down about row `groundY`, as still water shows what stands there.
        [[nodiscard]] SceneFrame mirroredAbout(const SceneFrame&, float groundY);
        // A closed curve through the points, appended to the path.
        void addSmoothLoop(PixelPath&, std::span<const FloatPoint> points);
        // The left and right edges of a stream, in pixels, walking down it.
        void streamEdges(const SceneFrame&, std::span<const StreamPoint> stream,
            std::vector<FloatPoint>& left, std::vector<FloatPoint>& right);
        [[nodiscard]] PixelPath riverOutline(const SceneFrame&);
        // The land as the frame shows it: all below the higher of the two ranges' ridges.
        [[nodiscard]] PixelPath landOutline(const SceneFrame&);
        // A cloud about its middle, 120 units wide.
        [[nodiscard]] PixelPath cloudOutline();
        [[nodiscard]] float nearBankTopAt(const SceneFrame&, float x);
        // The row of a line of shares at `x`, straight between its points.
        [[nodiscard]] float rowAlong(const SceneFrame&, std::span<const FloatPoint> line, float x);
        [[nodiscard]] float ridgeHeightShare(const MountainRange&, float xShare);
        [[nodiscard]] float farHillsTopAt(const SceneFrame&, float x);

        void drawSky(const SceneFrame&, const SceneTheme&, FloatPoint sun);
        void drawMountains(const SceneFrame&, const SceneTheme&, const MountainRange&);
        void drawFarHills(const SceneFrame&, const SceneTheme&);
        void drawValley(const SceneFrame&, const SceneTheme&);
        void drawMeadow(const SceneFrame&, const SceneTheme&, const PixelPath& river);
        void drawFootpath(const SceneFrame&, const SceneTheme&);
        void drawFir(const SceneFrame&, const SceneTheme&, const StandingTree&,
            bool castsShadow = true);
        void drawFirShadow(const SceneFrame&, const SceneTheme&, const StandingTree&);
        void drawBroadleaf(const SceneFrame&, const SceneTheme&, const StandingTree&);
        void drawRiverBed(const SceneFrame&, const SceneTheme&, const PixelPath& river);
        void drawBridge(const SceneFrame&, const SceneTheme&);
        // What still stands mirrored in the water: the mountains, the far hills, the bridge, the
        // firs by the shore and the mill's house.
        void drawStillReflections(const SceneFrame&, const SceneTheme&, const TimberMillColors&);
        void drawMovingReflections(const SceneFrame&, const SceneTheme&, float cycleTime,
            const CowMoment&, const std::optional<DuckFamily>& ducks);
        // Rings spreading on the water from `center`, `age` seconds after something broke it.
        void drawRings(const SceneFrame&, FloatPoint center, float age, float size, Color wave);
        // Five birds in a V, crossing the sky from right to left.
        void drawFlock(const SceneFrame&, const SceneTheme&, float story);
        // A duck swimming right, above its waterline.
        void drawDuck(const SceneFrame&, const SwimmingDuck&, const DuckColors&);
        // The lines a duck leaves on the water behind it, broken by the ripples as reflections are.
        void drawDuckWake(const SceneFrame&, const SwimmingDuck&, const DuckColors&,
            const Ripples&);
        void drawDucks(const SceneFrame&, const SceneTheme&,
            const std::optional<DuckFamily>& ducks, const Ripples&);
        void drawFishJumps(const SceneFrame&, const SceneTheme&, float story);
        // A butterfly over the near bank by day, its path coming round once a story.
        void drawButterfly(const SceneFrame&, const SceneTheme&, float story);
        // The fireflies and the weather of one of k_effectLayers.
        void drawEffectLayer(const SceneFrame&, const SceneTheme&, float cycleTime,
            std::size_t layerIndex);
        void drawWaterLight(const SceneFrame&, const SceneTheme&, float sunX, float cycleTime,
            const Ripples&);
        void drawNearBank(const SceneFrame&, const SceneTheme&, float cycleTime);
        // A cattail and its two leaves, `scale` pixels to its unit, swaying by `sway` from -1 to 1.
        void drawCattail(const SceneFrame&, const CattailColors&, FloatPoint foot, float scale,
            float height, float sway);
        // The cattails across the pond, each mirrored about its foot when `mirrored`.
        void drawShoreCattails(const SceneFrame&, const SceneTheme&, float cycleTime,
            bool mirrored);
        // The reed beds on the river's banks, each mirrored about its roots when `mirrored`.
        void drawReedBeds(const SceneFrame&, const SceneTheme&, float cycleTime, bool mirrored);
        void drawReedBed(const SceneFrame&, const SceneTheme&, const ReedBed&, float cycleTime,
            bool mirrored);
    }

    void MillScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        m_offset = bounds.topLeft(); // Anchored to widget's logical position
        const float w = bounds.width();
        const float h = bounds.height();
        m_objectsScale = std::min(h, w) / 600.0f;
        m_sunPos = { w * 0.3f, h * 0.25f };
        const float cycleTime = time.cycleTime;

        if (m_stillLayersSize != bounds.dimensions()
            || m_stillLayersThemeVersion != m_themes.wornVersion())
        {
            keepStillLayers(bounds);
        }

        canvas.drawBitmap(m_sky, m_offset);
        // The stars and the clouds stand behind the land, which hides their foot.
        drawStars(frame, cycleTime, StarField{
            .count = 240,
            .top = 0.0f,
            .bottom = h * k_horizonShare,
            .color = theme().sunDisk,
            .opacity = theme().hasStars,
            .bottomBrightness = 0.15f
        });

        const float rX = w * 0.35f;
        const float rY = h * 0.04f;
        const float a1 = 0.5f * cycleTime * k_2Pi;
        // Each cloud is lit by as much of the sun's glow as falls on its middle.
        const Color cloud = theme().cloud;
        const Color glowOnCloud = theme().sunGlow.withOpacity(cloud.alpha / 255.0f);
        for (const float turn : { a1, a1 + 3.1415f })
        {
            const FloatPoint middle = {
                w * 0.5f + std::cos(turn) * rX,
                h * 0.2f + std::sin(turn) * rY,
            };
            const Matrix3x2 place = Matrix3x2::Translation(m_offset.x + middle.x,
                m_offset.y + middle.y)
                * Matrix3x2::Scale((0.8f + (std::sin(turn) + 1.0f) * 0.2f) * m_objectsScale);
            drawCloud(canvas, place, Color::between(cloud, glowOnCloud,
                celestialGlowAt(frame, m_sunPos, middle, theme().celestialGlowStrength)));
        }
        // The bolt comes down behind the land, and the flash lights the sky alone.
        SceneFrame sky = frame;
        sky.height = h * k_horizonShare;
        drawLightning(sky, cycleTime, Lightning{
            .strength = theme().hasRain,
            .color = theme().sunDisk
        });

        canvas.drawBitmap(m_land, m_offset);
        const float story = storyAt(time);
        drawFlock(frame, theme(), story);
        drawEffectLayer(frame, theme(), cycleTime, 0);
        const CowMoment cow = cowAt(frame, story, cycleTime);
        const std::optional<DuckFamily> ducks = ducksAt(frame, story);
        const TimberMillColors millColors = millColorsOf(theme());
        const Ripples ripples = {
            .top = h * k_horizonShare,
            .farReach = k_farRippleReach,
            .nearReach = k_nearRippleReach,
            .cycleTime = cycleTime,
            .opacity = k_reflectionOpacity
        };
        m_reflection.draw(frame, riverOutline(frame), [&](const SceneFrame& mirror) {
            drawStillReflections(mirror, theme(), millColors);
        }, [&](const SceneFrame& mirror) {
            drawMovingReflections(mirror, theme(), cycleTime, cow, ducks);
        }, ripples);
        drawBridge(frame, theme());
        drawWaterLight(frame, theme(), m_sunPos.x, cycleTime, ripples);
        if (cow.ripplesAge)
        {
            const FloatPoint muzzle = {
                cow.pose.foot.x + cow.pose.facing * 70.0f * cow.pose.size,
                cow.pose.foot.y + 2.0f * cow.pose.size
            };
            drawRings(frame, muzzle, *cow.ripplesAge, cow.pose.size * 1.1f, theme().foam);
        }

        // The predator behind the fir on the right: a gator in the African themes, a wolf
        // elsewhere, crossfading between them. He lies on the fir's shadow, so its edge stays off
        // his face, and his theme colors are already shaded.
        drawFirShadow(frame, theme(), k_wolfFir);
        const FloatPoint wolfFirFoot = pointAt(frame, k_wolfFir.footShare);
        const float wolfFirUnit = sizeAt(frame, wolfFirFoot.y);
        const FloatPoint predator = {
            wolfFirFoot.x - 30.5f * wolfFirUnit,
            wolfFirFoot.y - 25.2f * wolfFirUnit
        };
        const float predatorSize = 0.84f * wolfFirUnit;
        drawGatorHead(frame, GatorHeadPose{
            .at = predator,
            .size = predatorSize,
            .cycleTime = cycleTime,
            .opacity = theme().isAfrica
        }, GatorHeadColors{
            .body = theme().wolfBody,
            .eye = theme().wolfEye
        });
        const float blinkSecond = std::fmod(story, k_wolfBlinkSeconds);
        const bool wolfEyesShut = blinkSecond >= k_wolfEyesShut.from
            && blinkSecond < k_wolfEyesShut.until;
        const bool wolfEarFlicked = std::fmod(story, k_wolfEarFlickSeconds) < k_wolfEarFlicked;
        // The wolf creeps in his fir's shadow: the top of his head is behind its lowest branches,
        // his left ear sticks out past their end, his right one shows under them toward the
        // trunk, his chin is below the trunk's foot, and his hind leg shows right of the trunk.
        const float firUnit = k_wolfFir.size * wolfFirUnit;
        drawPeekingWolf(frame, PeekingWolfPose{
            .at = { wolfFirFoot.x - 23.0f * firUnit, wolfFirFoot.y - 17.5f * firUnit },
            .hip = { wolfFirFoot.x + 4.0f * firUnit, wolfFirFoot.y - 16.0f * firUnit },
            .size = k_wolfSize * wolfFirUnit,
            .cycleTime = cycleTime,
            .eyesOpen = !wolfEyesShut,
            .earTurn = k_wolfEarLift + (wolfEarFlicked ? k_wolfEarFlick : 0.0f),
            .opacity = 1.0f - theme().isAfrica
        }, PeekingWolfColors{
            .body = theme().wolfBody,
            .eye = theme().wolfEye
        });
        drawFir(frame, theme(), k_wolfFir, false); // its shadow is under him already

        drawPosedCow(frame, cow.pose, cowColorsOf(theme()));
        drawCowTuft(frame, theme(), cow.pose);
        drawEffectLayer(frame, theme(), cycleTime, 1);

        drawShoreCattails(frame, theme(), cycleTime, false);
        drawReedBeds(frame, theme(), cycleTime, false);
        const TimberMillPose mill = millPoseOf(frame, cycleTime);
        fillOval(frame, { mill.base.x + 10.0f * mill.size, mill.base.y + 2.0f * mill.size },
            80.0f * mill.size, 6.0f * mill.size, SolidColor{ theme().shadow.faded(0.8f) });
        drawTimberMillSmoke(frame, mill, millColors);
        drawTimberMillWheel(frame, mill, millColors);
        drawTimberMillHouse(frame, mill, millColors);
        drawTimberMillWater(frame, mill, millColors);
        drawTimberMillWings(frame, mill, millColors);
        // The river's fog along the far shore veils the far banks and all that stands on them.
        canvas.fillRectangle(bounds, PointGlow{
            .lightPos = pointAt(frame, { 0.6f, 0.795f }) + m_offset,
            .lightColor = theme().cloud.withOpacity(0.33f),
            .lightSpread = 60.0f * m_objectsScale,
            .xRatio = 0.08f,
            .shape = GlowShape::Circle
        });
        drawFishJumps(frame, theme(), story);
        drawDucks(frame, theme(), ducks, ripples);
        drawEffectLayer(frame, theme(), cycleTime, 2);

        const FloatPoint lily = pointAt(frame, k_lilyShare);
        const SceneFrame water = frame.within(Matrix3x2::Translation(lily)
            * Matrix3x2::Scale(1.0f, k_lilyFlatness) * Matrix3x2::Translation(-lily.x, -lily.y));
        Color lilyFlower = theme().petal3;
        lilyFlower.paintOverGeneral(theme().lilyFlowerTint);
        drawWaterLily(water, WaterLilyPose{
            .center = lily,
            .size = k_lilySize * sizeAt(frame, lily.y),
            .cycleTime = cycleTime
        }, WaterLilyColors{
            .pad = theme().treeLeavesBase.fullyOpaque(),
            .flower = lilyFlower
        });
        drawNearBank(frame, theme(), cycleTime);
        drawButterfly(frame, theme(), story);

        drawEffectLayer(frame, theme(), cycleTime, 3);

        // A little celestial color change is acceptable rather than wasting time on the same glow
        // so no clip here and no drawCelestialGlow in drawSky
        //canvas.pushClip(landOutline(frame), &frame.toCanvas);
        drawCelestialGlow(frame, m_sunPos, theme().sunGlow, theme().celestialGlowStrength);
        //canvas.popClip();
    }

    // In Clear Day's colors whatever theme is worn: an icon is one picture in every theme.
    void MillScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        constexpr float k_figureSpan{ 480.0f };
        constexpr Themes::ClearDay k_iconTheme{};
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const float unit = frame.unit;
        // The figure runs from the wheel's far rim to the wings' tips, 249 by 243 units.
        const TimberMillPose mill = {
            .base = { frame.width * 0.5f + 14.0f * unit, frame.height * 0.5f + 204.0f * unit },
            .size = 1.7f * unit,
            .wingTurns = 0.06f,
            .wheelTurns = 0.0f,
            .cycleTime = 0.0f
        };
        const TimberMillColors millColors = millColorsOf(k_iconTheme);
        // The wheel's foot, which the river covers in the scene, ends at the house's foot.
        canvas.pushClip(FloatRect{ bounds.left, bounds.top, bounds.right,
            bounds.top + mill.base.y });
        drawTimberMillWheel(frame, mill, millColors);
        drawTimberMillHouse(frame, mill, millColors);
        drawTimberMillWings(frame, mill, millColors);
        canvas.popClip();
    }

    // **The picked theme's colors, not the blend's**: a host asks the moment a theme is picked,
    // while the blend still wears the one before it.
    SceneReadoutColors MillScene::readoutColors() const
    {
        return readoutColorsOf(pickedTheme());
    }

    // The CPU rasterizer's clip is one state for every canvas, and this runs in the middle of a
    // paint, so the clip that paint stated is held across the drawing.
    void MillScene::keepStillLayers(const FloatRect& bounds)
    {
        m_stillLayersSize = bounds.dimensions();
        m_stillLayersThemeVersion = m_themes.wornVersion();
        const IntSize size = m_stillLayersSize.roundOut();
        const IntRect whole = { 0, 0, size.x, size.y };
        const FloatRect local = { 0.0f, 0.0f, bounds.width(), bounds.height() };

        const bool heldClipActive = g_rasterBuffers.hasActiveClip;
        const float* heldClipMask = g_rasterBuffers.activeClipMaskPtr;
        const FloatRect heldClipBox = g_rasterBuffers.clipBox;
        const int heldClipStride = g_rasterBuffers.m_clipStride;

        const auto keep = [&](Bitmap& layer, const bool transparent, const auto& drawing) {
            auto backend = std::make_unique<CpuBackend>(size);
            backend->setTransparentBase(transparent);
            Canvas layerCanvas{ std::move(backend) };
            layerCanvas.beginPaint(nullptr, whole);
            drawing(SceneFrame::FromBounds(layerCanvas, local));
            Bitmap* drawn = nullptr;
            layerCanvas.endPaint(nullptr, whole, drawn);
            layer = *drawn;
        };
        keep(m_sky, false, [&](const SceneFrame& frame) {
            drawSky(frame, theme(), m_sunPos);
        });
        keep(m_land, true, [&](const SceneFrame& frame) {
            drawMountains(frame, theme(), k_farRange);
            drawMountains(frame, theme(), k_nearRange);
            drawFarHills(frame, theme());
            drawValley(frame, theme());
            const PixelPath river = riverOutline(frame);
            drawMeadow(frame, theme(), river);
            drawFootpath(frame, theme());
            for (const StandingTree& tree : k_knollFirs)
                drawFir(frame, theme(), tree);
            for (const StandingTree& tree : k_broadleaves)
                drawBroadleaf(frame, theme(), tree);
            drawRiverBed(frame, theme(), river);
            for (const StandingTree& tree : k_shoreFirs)
                drawFir(frame, theme(), tree);
        });

        m_reflection.forget();

        g_rasterBuffers.hasActiveClip = heldClipActive;
        g_rasterBuffers.activeClipMaskPtr = heldClipMask;
        g_rasterBuffers.clipBox = heldClipBox;
        g_rasterBuffers.m_clipStride = heldClipStride;
    }

    void MillScene::drawCloud(Canvas& canvas, const Matrix3x2& place, const Color color)
    {
        canvas.drawPath(cloudOutline(), {
            PathDrawLayer::Fill(color),
            PathDrawLayer::OuterGlow(color, 10.0f)
        }, &place);
    }

    namespace
    {
        TimberMillColors millColorsOf(const SceneTheme& theme)
        {
            const Color black{ 0xFF000000 };
            return {
                .bodyTop = theme.mill.bodyTop,
                .bodyBottom = theme.mill.bodyBottom,
                .roofTop = theme.mill.roofTop,
                .roofBottom = theme.mill.roofBottom,
                .timber = theme.mill.timber,
                .bark = theme.bark,
                .barkShade = theme.barkShade,
                .stoneLight = theme.stoneLight,
                .stoneDark = theme.stoneDark,
                .stoneCrack = theme.stoneCrack,
                .sunlight = theme.sunDisk,
                .glass = Color::between(theme.skyBottom.fullyOpaque(), black, 0.45f),
                .lamp = theme.mill.windowLight,
                .shadow = theme.shadow,
                .wingFabric = theme.mill.wingFabric,
                .wheel = theme.mill.wheel,
                .foam = theme.foam,
                .smoke = Color::between(theme.cloud, theme.skyBottom.fullyOpaque(), 0.25f)
            };
        }

        TimberMillPose millPoseOf(const SceneFrame& frame, const float cycleTime)
        {
            return {
                .base = pointAt(frame, k_millFootShare),
                .size = k_millSize * frame.unit,
                .wingTurns = cycleTime,
                .wheelTurns = cycleTime * 2.0f,
                .cycleTime = cycleTime
            };
        }

        float storyAt(const SceneTime time)
        {
            const float storyCycle = static_cast<float>(time.cyclesPassed & 1);
            return (storyCycle + time.cycleTime) * k_storySeconds * 0.5f;
        }

        CowMoment cowAt(const SceneFrame& frame, const float story, const float cycleTime)
        {
            const float grazingHead = 1.15f + 0.06f * std::sin(cycleTime * k_cowNods * k_2Pi);
            const float chew = 2.0f * std::abs(std::sin(cycleTime * k_cowChews
                * std::numbers::pi_v<float>));
            float walked = 0.0f; // from where it grazes, 0, to where it drinks, 1
            float facing = -1.0f;
            float head;
            float jaw = 0.0f;
            std::optional<float> stride{};
            if (story < 2.0f)
            {
                head = grazingHead + (0.05f - grazingHead) * smoothStep(0.0f, 2.0f, story);
                jaw = chew;
            }
            else if (story < 10.0f)
            {
                walked = (story - 2.0f) / 8.0f;
                head = 0.1f;
                stride = (story - 2.0f) * 0.9f;
            }
            else if (story < 12.0f)
            {
                walked = 1.0f;
                head = 0.1f + 1.25f * smoothStep(10.0f, 12.0f, story);
            }
            else if (story < 24.0f)
            {
                walked = 1.0f;
                head = 1.35f + 0.03f * std::sin(story * 4.0f);
            }
            else if (story < 26.0f)
            {
                walked = 1.0f;
                head = 1.35f - 1.25f * smoothStep(24.0f, 26.0f, story);
            }
            else if (story < 27.0f)
            {
                walked = 1.0f;
                facing = -1.0f + 2.0f * smoothStep(26.0f, 27.0f, story);
                head = 0.1f;
            }
            else if (story < 35.0f)
            {
                walked = 1.0f - (story - 27.0f) / 8.0f;
                facing = 1.0f;
                head = 0.1f;
                stride = (story - 27.0f) * 0.9f;
            }
            else if (story < 36.0f)
            {
                facing = 1.0f - 2.0f * smoothStep(35.0f, 36.0f, story);
                head = 0.1f;
            }
            else if (story < 38.0f)
            {
                head = 0.1f + (grazingHead - 0.1f) * smoothStep(36.0f, 38.0f, story);
                jaw = chew;
            }
            else
            {
                // It looks up from the grass as the birds come over.
                const float lookingUp = smoothStep(48.0f, 49.0f, story)
                    - smoothStep(52.0f, 53.0f, story);
                head = grazingHead * (1.0f - lookingUp);
                jaw = chew;
            }

            // A ripple each 2.6 seconds from 12 to 24, the last left to spread for 3.5.
            std::optional<float> ripplesAge{};
            const float sinceDrinking = story - 12.0f;
            if (sinceDrinking >= 0.0f)
            {
                const float lastRipple = std::floor(std::min(sinceDrinking, 11.99f) / 2.6f) * 2.6f;
                if (sinceDrinking - lastRipple < 3.5f)
                    ripplesAge = sinceDrinking - lastRipple;
            }

            const FloatPoint foot = pointAt(frame, FloatPoint::between(k_cowFootShare, k_cowDrinkShare,
                walked));
            return {
                .pose = {
                    .foot = foot,
                    .size = k_cowSize * sizeAt(frame, foot.y),
                    .facing = facing,
                    .headDrop = head,
                    .stride = stride,
                    .chew = jaw,
                    .tailSwing = 0.25f * std::sin(cycleTime * k_cowSwishes * k_2Pi)
                },
                .ripplesAge = ripplesAge
            };
        }

        PosedCowColors cowColorsOf(const SceneTheme& theme)
        {
            return {
                .body = theme.cowBody,
                .spots = theme.cowSpots,
                .snout = theme.cowSnout,
                .detail = theme.cowDetail,
                .shadow = theme.shadow,
                .limbInkShare = theme.cowLimbInkShare,
                .tips = theme.cowTips
            };
        }

        void drawCowTuft(const SceneFrame& frame, const SceneTheme& theme, const PosedCowPose& pose)
        {
            const FloatPoint foot = pointAt(frame, k_cowFootShare);
            const float size = k_cowSize * sizeAt(frame, foot.y);
            const FloatPoint root = { foot.x - 58.0f * size, foot.y + 6.0f * size };
            const float parallax = parallaxAt(frame, root.y);
            // The blades give only where her muzzle is: she stands at them with her head in the
            // grass, and the tips swing toward it as her jaw shuts on a bite and spring back as
            // it opens.
            const float standing = smoothStep(k_cowTuftReach, 0.0f,
                std::abs(pose.foot.x - foot.x) / pose.size);
            const float muzzleInTuft = standing * smoothStep(0.9f, 1.1f, pose.headDrop);
            const float pullShare = k_cowTuftHeld
                + (1.0f - k_cowTuftHeld) * (1.0f - 0.5f * pose.chew);
            drawGrassPatch(frame.within(Matrix3x2::Translation(root)
                * Matrix3x2::Scale(k_cowTuftSize * size)), {
                .dark = theme.hazed(theme.grassPatch.dark, parallax),
                .middle = theme.hazed(theme.grassPatch.middle, parallax),
                .light = theme.hazed(theme.grassPatch.light, parallax)
            }, k_cowTuftPull * muzzleInTuft * pullShare);
        }

        std::optional<DuckFamily> ducksAt(const SceneFrame& frame, const float story)
        {
            // Counted on past the story's end, where the ducks are still swimming.
            const float swimming = std::fmod(story - k_ducksSwim.from + k_storySeconds,
                k_storySeconds);
            const float swum = swimming / (k_ducksSwim.until - k_ducksSwim.from);
            if (swum > 1.0f)
                return std::nullopt;
            const float motherSize = sizeAt(frame, frame.height * 0.89f) * 0.62f;
            DuckFamily ducks{};
            for (std::size_t i = 0; i != ducks.size(); ++i)
            {
                const float index = static_cast<float>(i);
                const DuckPlace& place = k_duckPlaces[i];
                const float behind = place.behind
                    + place.drift * std::sin(swimming * place.driftPace + place.driftPhase);
                const float x = frame.width * (-0.06f + 1.12f * swum) + behind * motherSize;
                const float sway = i == 0 ? k_motherSway : k_ducklingSway;
                ducks[i] = {
                    .at = {
                        x,
                        frame.height * (0.885f + 0.012f * std::sin(x / frame.width * 5.0f))
                            + sway * frame.unit * std::sin(swimming * 2.0f + index)
                    },
                    .size = i == 0 ? motherSize : motherSize * 0.62f,
                    .paddle = std::sin(swimming * 3.0f + index)
                };
            }
            return ducks;
        }

        DuckColors duckColorsOf(const SceneTheme& theme, const std::size_t duckIndex)
        {
            if (duckIndex == 0)
            {
                return {
                    .body = theme.motherDuckBody,
                    .wing = theme.motherDuckWing,
                    .beak = theme.duckBeak,
                    .eye = theme.motherDuckEye,
                    .wake = theme.foam
                };
            }
            return {
                .body = theme.duckLight.fullyOpaque(),
                .wing = theme.duckShade.fullyOpaque(),
                .beak = theme.duckBeak,
                .eye = theme.cowDetail,
                .wake = theme.foam
            };
        }

        float parallaxAt(const SceneFrame& frame, const float y)
        {
            const float vanishing = frame.height * k_vanishingShare;
            const float millFoot = frame.height * k_millFootShare.y;
            return std::max(0.01f, (y - vanishing) / (millFoot - vanishing));
        }

        float sizeAt(const SceneFrame& frame, const float y)
        {
            return frame.unit * parallaxAt(frame, y);
        }

        Color landAt(const SceneFrame& frame, const SceneTheme& theme, const float y)
        {
            const float share = std::clamp((y - frame.height * 0.4f) / (frame.height * 0.6f), 0.0f,
                1.0f);
            return Color::between(theme.hillTop, theme.hillBottom, share);
        }

        LinearGradient landGradient(const SceneFrame& frame, const SceneTheme& theme,
            const float top, const float parallax)
        {
            constexpr std::size_t k_steps{ 12 };
            LinearGradient land{
                .startPoint = { 0.0f, top },
                .endPoint = { 0.0f, frame.height },
                .stops = {}
            };
            land.stops.reserve(k_steps + 1);
            for (std::size_t step = 0; step <= k_steps; ++step)
            {
                const float position = static_cast<float>(step) / static_cast<float>(k_steps);
                const float y = top + (frame.height - top) * position;
                const float rowParallax = std::max(parallax, parallaxAt(frame, y));
                land.stops.push_back({ .position = position,
                    .color = theme.hazed(landAt(frame, theme, y), rowParallax) });
            }
            return land;
        }

        FloatPoint pointAt(const SceneFrame& frame, const FloatPoint share)
        {
            return { frame.width * share.x, frame.height * share.y };
        }

        float spreadShare(const int index, const int count, const float seed)
        {
            constexpr float k_jitter{ 0.8f }; // of a stretch: they neither clump nor stand in a row
            const float nudge = k_jitter * (pseudoRandom(index, seed) - 0.5f);
            return (static_cast<float>(index) + 0.5f + nudge) / static_cast<float>(count);
        }

        SceneFrame mirroredAbout(const SceneFrame& frame, const float groundY)
        {
            return frame.within(Matrix3x2::Translation(0.0f, 2.0f * groundY)
                * Matrix3x2::Scale(1.0f, -1.0f));
        }

        // Catmull-Rom through the points, each span a cubic.
        void addSmoothLoop(PixelPath& path, const std::span<const FloatPoint> points)
        {
            const std::size_t count = points.size();
            const auto at = [&](const std::size_t index) {
                return points[index % count];
            };
            path.moveTo(points[0]);
            for (std::size_t i = 0; i != count; ++i)
            {
                const FloatPoint before = at(i + count - 1);
                const FloatPoint from = at(i);
                const FloatPoint to = at(i + 1);
                const FloatPoint after = at(i + 2);
                const FloatPoint fromHandle = {
                    from.x + (to.x - before.x) / 6.0f,
                    from.y + (to.y - before.y) / 6.0f
                };
                const FloatPoint toHandle = {
                    to.x - (after.x - from.x) / 6.0f,
                    to.y - (after.y - from.y) / 6.0f
                };
                path.cubicTo(fromHandle, toHandle, to);
            }
            path.close();
        }

        void streamEdges(const SceneFrame& frame, const std::span<const StreamPoint> stream,
            std::vector<FloatPoint>& left, std::vector<FloatPoint>& right)
        {
            const std::size_t last = stream.size() - 1;
            for (std::size_t i = 0; i <= last; ++i)
            {
                const FloatPoint before = pointAt(frame, stream[i == 0 ? 0 : i - 1].share);
                const FloatPoint after = pointAt(frame, stream[std::min(i + 1, last)].share);
                const float length = std::max(0.001f, std::hypot(after.x - before.x,
                    after.y - before.y));
                const FloatPoint along = { (after.x - before.x) / length,
                    (after.y - before.y) / length };
                const FloatPoint middle = pointAt(frame, stream[i].share);
                const float halfWidth = frame.width * stream[i].widthShare * 0.5f;
                left.push_back({ middle.x + along.y * halfWidth, middle.y - along.x * halfWidth });
                right.push_back({ middle.x - along.y * halfWidth, middle.y + along.x * halfWidth });
            }
        }

        // The river's east edge down to its mouth, the pond's far shore, the frame's bottom, the
        // pond's left shore, and the river's west edge back up to its source.
        PixelPath riverOutline(const SceneFrame& frame)
        {
            std::vector<FloatPoint> east;
            std::vector<FloatPoint> west;
            streamEdges(frame, k_upstream, east, west);
            std::vector<FloatPoint> points = east;
            for (const FloatPoint share : k_farShore)
                points.push_back(pointAt(frame, share));
            points.push_back(pointAt(frame, { 1.04f, 1.04f }));
            points.push_back(pointAt(frame, { -0.04f, 1.04f }));
            for (const FloatPoint share : k_leftShore)
                points.push_back(pointAt(frame, share));
            points.insert(points.end(), west.rbegin(), west.rend());
            PixelPath outline;
            addSmoothLoop(outline, points);
            return outline;
        }

        PixelPath landOutline(const SceneFrame& frame)
        {
            const float groundY = frame.height * k_horizonShare;
            const float step = 6.0f * frame.unit;
            PixelPath outline;
            outline.moveTo(-step, frame.height);
            for (float x = -step; x <= frame.width + step; x += step)
            {
                const float share = x / frame.width;
                const float ridge = std::max(ridgeHeightShare(k_farRange, share),
                    ridgeHeightShare(k_nearRange, share));
                outline.lineTo(x, groundY - ridge * frame.height);
            }
            outline.lineTo(frame.width + step, frame.height);
            outline.close();
            return outline;
        }

        PixelPath cloudOutline()
        {
            PixelPath outline;
            outline.moveTo(-60.0f, 0.0f);
            outline.quadTo({ -40.0f, -40.0f }, { -20.0f, -20.0f });
            outline.quadTo({ 0.0f, -60.0f }, { 20.0f, -20.0f });
            outline.quadTo({ 40.0f, -40.0f }, { 60.0f, 0.0f });
            outline.lineTo(60.0f, 20.0f);
            outline.lineTo(-60.0f, 20.0f);
            outline.close();
            return outline;
        }

        float nearBankTopAt(const SceneFrame& frame, const float x)
        {
            return rowAlong(frame, k_nearBank, x);
        }

        float rowAlong(const SceneFrame& frame, const std::span<const FloatPoint> line,
            const float x)
        {
            const float share = x / frame.width;
            for (std::size_t i = 0; i + 1 != line.size(); ++i)
            {
                const FloatPoint from = line[i];
                const FloatPoint to = line[i + 1];
                if (share < from.x || share > to.x)
                    continue;
                const float along = (share - from.x) / (to.x - from.x);
                return frame.height * (from.y + (to.y - from.y) * along);
            }
            return frame.height * line.back().y;
        }

        // The peaks' heights blended where they meet, with small ridges along the whole range.
        float ridgeHeightShare(const MountainRange& range, const float xShare)
        {
            constexpr float k_blendSharpness{ 90.0f };
            float blended = 0.0f;
            for (const Peak& peak : range.peaks)
            {
                const float offset = xShare - peak.centerShare;
                const float reach = std::abs(offset)
                    / (offset < 0.0f ? peak.leftWidthShare : peak.rightWidthShare);
                const float height = peak.heightShare * std::exp(-1.4f * std::pow(reach, 1.35f));
                blended += std::exp(k_blendSharpness * height);
            }
            const float phase = range.roughnessPhase;
            return std::log(blended) / k_blendSharpness
                + 0.007f * (std::abs(std::sin(xShare * 31.0f + phase)) - 0.5f)
                + 0.004f * (std::abs(std::sin(xShare * 77.0f + phase * 2.3f)) - 0.5f)
                + 0.002f * std::sin(xShare * 190.0f + phase);
        }

        float farHillsTopAt(const SceneFrame& frame, const float x)
        {
            const float share = x / frame.width;
            const float rise = 0.031f + 0.012f * std::sin(share * 8.5f + 1.2f)
                + 0.008f * std::sin(share * 21.0f + 2.1f) + 0.004f * std::sin(share * 47.0f + 0.4f);
            return frame.height * (k_horizonShare - rise);
        }

        void drawSky(const SceneFrame& frame, const SceneTheme& theme, const FloatPoint sun)
        {
            fillBox(frame, { 0.0f, 0.0f, frame.width, frame.height },
                LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, frame.height * k_horizonShare },
                theme.skyTop, theme.skyBottom));
            
            // no glow here - it is drawn in MillScene::draw
            //drawCelestialGlow(frame, sun, theme.sunGlow);

            drawCelestial(frame, CelestialPose{ .center = sun, .night = theme.hasStars },
                theme.sunDisk);
        }

        // Hazed by height, as the Chase's ranges are: the feet take their distance's share of the
        // air, the peaks less.
        void drawMountains(const SceneFrame& frame, const SceneTheme& theme,
            const MountainRange& range)
        {
            const float groundY = frame.height * k_horizonShare;
            const float step = 6.0f * frame.unit;
            PixelPath ridges;
            ridges.moveTo(-step, groundY);
            for (float x = -step; x <= frame.width + step; x += step)
                ridges.lineTo(x, groundY - ridgeHeightShare(range, x / frame.width) * frame.height);
            ridges.lineTo(frame.width + step, groundY);
            ridges.close();
            const float gradientTop = frame.height * k_mountainTopShare;
            LinearGradient rock{
                .startPoint = { 0.0f, gradientTop },
                .endPoint = { 0.0f, groundY },
                .stops = {}
            };
            rock.stops.reserve(k_mountainHazeSteps + 1);
            for (std::size_t i = 0; i <= k_mountainHazeSteps; ++i)
            {
                const float position = static_cast<float>(i)
                    / static_cast<float>(k_mountainHazeSteps);
                const float aboveGround = (1.0f - position) * (groundY - gradientTop)
                    / frame.height;
                const Color clear = Color::between(theme.mountainTop,
                    theme.mountainFoot, position);
                rock.stops.push_back({ .position = position,
                    .color = theme.hazed(clear, range.parallax, aboveGround) });
            }
            fillShape(frame, ridges, rock);
        }

        void drawFarHills(const SceneFrame& frame, const SceneTheme& theme)
        {
            const float groundY = frame.height * k_horizonShare;
            const float step = 6.0f * frame.unit;
            PixelPath hills;
            hills.moveTo(-step, groundY + 4.0f);
            for (float x = -step; x <= frame.width + step; x += step)
                hills.lineTo(x, farHillsTopAt(frame, x));
            hills.lineTo(frame.width + step, groundY + 4.0f);
            hills.close();
            fillShape(frame, hills, LinearGradient::Simple(
                { 0.0f, frame.height * 0.51f }, { 0.0f, groundY },
                theme.hazed(landAt(frame, theme, frame.height * 0.48f), k_farHillsParallax),
                theme.hazed(landAt(frame, theme, groundY), k_farHillsParallax)));

            // crowns as the Chase's far hills draw them, smaller for the distance, on plain trunks
            const SolidColor bark{ theme.hazed(theme.bark, k_farHillsParallax) };
            const SolidColor crown{ theme.hazed(theme.treeLeavesBase, k_farHillsParallax) };
            const SolidColor crownLight{
                theme.hazed(theme.crownLight, k_farHillsParallax) };
            const SolidColor crownMiddle{
                theme.hazed(theme.crownMiddle, k_farHillsParallax) };
            struct FarTree
            {
                FloatPoint foot;
                float size;
            };
            // A grove's trees fill an oval down the slope as sunflower seeds fill a disc, a golden
            // angle apart, so they neither clump nor line up.
            constexpr float k_goldenAngle{ 2.39996f };
            std::vector<FarTree> trees;
            for (int groveIndex = 0; groveIndex != std::ssize(k_farGroves); ++groveIndex)
            {
                const Grove& grove = k_farGroves[static_cast<std::size_t>(groveIndex)];
                const float widthShare = grove.untilShare - grove.fromShare;
                const float middle = (grove.fromShare + 0.5f * widthShare) * frame.width;
                const float halfWidth = 0.5f * widthShare * frame.width;
                const float depth = frame.height
                    * (0.025f + 0.01f * pseudoRandom(groveIndex, 17.0f));
                const int count = static_cast<int>(std::round(widthShare * k_groveTreesPerWidth));
                for (int tree = 0; tree != count; ++tree)
                {
                    const int index = groveIndex * k_groveTreesMost + tree;
                    const float reach = std::sqrt((static_cast<float>(tree) + 0.5f)
                        / static_cast<float>(count));
                    const float turn = static_cast<float>(tree) * k_goldenAngle
                        + 6.0f * pseudoRandom(groveIndex, 18.0f)
                        + 0.5f * pseudoRandom(index, 11.0f);
                    const float x = middle + reach * std::cos(turn) * halfWidth;
                    const float hillTop = farHillsTopAt(frame, x);
                    const float slopeHeight = std::min(depth,
                        k_groveSlopeReach * (groundY - hillTop));
                    const float downSlope = (0.5f + 0.5f * reach * std::sin(turn)) * slopeHeight;
                    trees.push_back({
                        .foot = { x, hillTop + downSlope + 2.0f },
                        .size = (0.13f + 0.1f * pseudoRandom(index, 13.0f)) * frame.unit
                    });
                }
            }
            std::ranges::sort(trees, [](const FarTree& further, const FarTree& nearer) {
                return further.foot.y < nearer.foot.y;
            });
            for (const FarTree& tree : trees)
            {
                const float x = tree.foot.x;
                const float y = tree.foot.y;
                const float size = tree.size;
                fillBox(frame, { x - 3.0f * size, y - 22.0f * size, x + 3.0f * size, y }, bark);
                fillDisc(frame, { x, y - 32.0f * size }, 16.0f * size, crown);
                fillDisc(frame, { x - 9.0f * size, y - 25.0f * size }, 11.0f * size, crownLight);
                fillDisc(frame, { x + 9.0f * size, y - 26.0f * size }, 12.0f * size, crownMiddle);
            }
        }

        void drawValley(const SceneFrame& frame, const SceneTheme& theme)
        {
            const float groundY = frame.height * k_horizonShare;
            fillBox(frame, { 0.0f, groundY, frame.width, frame.height },
                landGradient(frame, theme, groundY, 0.0f));
            const LinearGradient knollLand = landGradient(frame, theme, frame.height * 0.5f,
                k_knollParallax);
            const std::array<std::span<const FloatPoint>, 2> knolls{ k_leftKnoll, k_rightKnoll };
            for (const std::span<const FloatPoint> knoll : knolls)
            {
                std::vector<FloatPoint> points;
                for (const FloatPoint share : knoll)
                    points.push_back(pointAt(frame, share));
                PixelPath outline;
                addSmoothLoop(outline, points);
                fillShape(frame, outline, knollLand);
            }
        }

        [[nodiscard]] bool clearsRiver(const std::span<const BakedPathPoint> shore,
            const FloatPoint center, const float radius)
        {
            int winding = 0;
            const float radiusSquared = radius * radius;
            for (std::size_t i = 1; i < shore.size(); ++i)
            {
                const FloatPoint from = shore[i - 1].coord;
                const FloatPoint to = shore[i].coord;
                if (center.y + radius < std::min(from.y, to.y)
                    || center.y - radius > std::max(from.y, to.y))
                    continue;

                const FloatPoint edge = to - from;
                const FloatPoint offset = center - from;
                if ((from.y > center.y) != (to.y > center.y)
                    && from.x + offset.y * edge.x / edge.y > center.x)
                    winding += edge.y > 0.0f ? 1 : -1;

                const float lengthSquared = edge.x * edge.x + edge.y * edge.y;
                if (lengthSquared == 0.0f)
                    continue;
                const float nearestShare = std::clamp(
                    (offset.x * edge.x + offset.y * edge.y) / lengthSquared, 0.0f, 1.0f);
                const FloatPoint delta = offset - edge * nearestShare;
                if (delta.x * delta.x + delta.y * delta.y <= radiusSquared)
                    return false;
            }
            return winding == 0;
        }

        // Tufts of grass and flowers' specks over the valley, smaller far off.
        void drawMeadow(const SceneFrame& frame, const SceneTheme& theme, const PixelPath& river)
        {
            struct MeadowBand
            {
                float top{};
                float nearness{};
                float parallax{};
            };

            struct TuftBounds
            {
                FloatPoint center{};
                float radius{};
            };

            // Drawing a shape reuses the rasterizer's baked points.
            const std::vector<BakedPathPoint> shore = [&river](){
                BakedPixelPath bakedRiver;
                bakedRiver.bake(river);
                return bakedRiver.points();
            }();
            const float groundY = frame.height * k_horizonShare;
            // One tuft in three wears the blades' dark, as one near-bank blade in three does.
            const std::array<Color, 3> tuftColors{ theme.grassBlade.dark,
                theme.grassBlade.light, theme.grassBlade.light };
            const std::array<Color, 3> petals{ theme.petal1, theme.petal2, theme.petal3 };
            const float bandDepth = (frame.height - groundY)
                / static_cast<float>(k_meadowDepthBands);
            std::array<MeadowBand, k_meadowDepthBands> bands{};
            for (std::size_t i = 0; i != bands.size(); ++i)
            {
                const float bandTop = groundY + bandDepth * static_cast<float>(i);
                const float bandMiddle = bandTop + bandDepth * 0.5f;
                bands[i] = {
                    .top = bandTop,
                    .nearness = (bandMiddle - groundY) / (frame.height - groundY),
                    .parallax = parallaxAt(frame, bandMiddle)
                };
            }

            std::vector<TuftBounds> tuftBounds;
            tuftBounds.reserve(k_meadowTufts);
            for (const MeadowBand& band : bands)
            {
                const float tuftAlphaShare = 0.85f + 0.15f * band.nearness;
                const float strokeWidth = std::max(1.0f, 1.8f * band.nearness * frame.unit);
                for (std::size_t kind = 0; kind != tuftColors.size(); ++kind)
                {
                    const Color tuftColor = theme.hazed(tuftColors[kind], band.parallax);
                    ScatteredShapes tufts{ frame, SolidColor{ tuftColor.faded(tuftAlphaShare) },
                        strokeWidth };
                    for (int i = static_cast<int>(kind); i < k_meadowTufts; i += 3)
                    {
                        const float y = groundY + (frame.height - groundY)
                            * std::pow(pseudoRandom(i, 21.0f), 0.8f);
                        if (y < band.top || y >= band.top + bandDepth)
                            continue;
                        const float x = pseudoRandom(i, 22.0f) * frame.width;
                        const float depth = (y - groundY) / (frame.height - groundY);
                        const float height = (2.5f + 7.5f * depth) * frame.unit
                            * (0.6f + 0.6f * pseudoRandom(i, 23.0f));
                        // The shorter blades fit inside the circle through the root and tall tip.
                        const float halfHeight = height * 0.5f;
                        const FloatPoint center = {
                            x,
                            y - halfHeight
                        };
                        const float radius = halfHeight + strokeWidth * 0.5f;
                        if (!clearsRiver(shore, center, radius + 1.0f))
                            continue;
                        tuftBounds.push_back({ center, radius });
                        // As the Chase's lawn tufts: three blades from a root, the middle tallest.
                        PixelPath& tuft = tufts.next();
                        for (int blade = -1; blade <= 1; ++blade)
                        {
                            const float bladeX = static_cast<float>(blade) * 4.0f / 11.0f;
                            const float bladeHeight
                                = (8.0f + 3.0f * static_cast<float>(1 - std::abs(blade))) / 11.0f;
                            tuft.moveTo(x, y);
                            tuft.lineTo(x + bladeX * height, y - bladeHeight * height);
                        }
                    }
                    tufts.draw();
                }
            }

            const float flowerGap = std::max(2.0f, 3.0f * frame.unit);
            const auto clearsTufts = [&](const FloatPoint center, const float radius){
                return std::ranges::none_of(tuftBounds, [&](const TuftBounds& tuft){
                    return distanceBetweenPoints(center, tuft.center)
                        < radius + tuft.radius + flowerGap;
                });
            };
            const auto placeFlower = [&](const FloatPoint center,
                const float radius) -> std::optional<FloatPoint> {
                if (!clearsRiver(shore, center, radius + 1.0f))
                    return {};
                if (clearsTufts(center, radius))
                    return center;

                for (int step = 1; step <= 8; ++step)
                {
                    const float offset = static_cast<float>(step) * flowerGap;
                    for (const float shift : { -offset, offset })
                    {
                        const FloatPoint candidate = {
                            center.x + shift,
                            center.y
                        };
                        if (candidate.x >= radius && candidate.x <= frame.width - radius
                            && clearsTufts(candidate, radius)
                            && clearsRiver(shore, candidate, radius + 1.0f))
                            return candidate;
                    }
                }
                return {};
            };

            for (const MeadowBand& band : bands)
            {
                const float speckAlphaShare = 0.35f + 0.5f * band.nearness;
                for (std::size_t kind = 0; kind != petals.size(); ++kind)
                {
                    ScatteredShapes specks{ frame,
                        SolidColor{ theme.hazed(petals[kind], band.parallax).faded(speckAlphaShare) } };
                    for (int i = static_cast<int>(kind); i < k_meadowFlowers; i += 3)
                    {
                        const float y = groundY + 30.0f + (frame.height * 0.8f - groundY - 30.0f)
                            * std::pow(pseudoRandom(i, 31.0f), 0.6f);
                        if (y < band.top || y >= band.top + bandDepth)
                            continue;
                        const float x = pseudoRandom(i, 32.0f) * frame.width;
                        const float depth = (y - groundY) / (frame.height - groundY);
                        const float radius = (0.4f + 2.4f * depth) * frame.unit;
                        if (const std::optional<FloatPoint> center = placeFlower({ x, y }, radius))
                            addEllipse(specks.next(), *center, radius, radius * 0.5f);
                    }
                    specks.draw();
                }
            }
        }

        void drawFootpath(const SceneFrame& frame, const SceneTheme& theme)
        {
            std::vector<FloatPoint> left;
            std::vector<FloatPoint> right;
            streamEdges(frame, k_footpath, left, right);
            left.insert(left.end(), right.rbegin(), right.rend());
            PixelPath path;
            addSmoothLoop(path, left);
            fillShape(frame, path, LinearGradient::Simple(
                { 0.0f, frame.height * 0.572f }, { 0.0f, frame.height * 0.68f },
                theme.hazed(theme.trail, k_knollParallax), theme.trail));
        }

        void drawFir(const SceneFrame& frame, const SceneTheme& theme, const StandingTree& tree,
            const bool castsShadow)
        {
            const FloatPoint foot = pointAt(frame, tree.footShare);
            const float parallax = parallaxAt(frame, foot.y);
            const Color needles = theme.treeLeavesBase.fullyOpaque();
            const Color sunlit = Color::between(needles, theme.crownLight.fullyOpaque(), 0.7f);
            const Color shaded = Color::between(needles, Color{ 0xFF000000 }, 0.28f);
            drawSpruce(frame, foot, tree.size * sizeAt(frame, foot.y), SpruceColors{
                .needles = theme.hazed(needles, parallax),
                .sunlit = theme.hazed(sunlit, parallax),
                .shaded = theme.hazed(shaded, parallax),
                .trunk = theme.hazed(theme.bark, parallax),
                .trunkShade = theme.hazed(theme.barkShade, parallax),
                .shadow = castsShadow ? theme.shadow : Color{}
            });
        }

        void drawFirShadow(const SceneFrame& frame, const SceneTheme& theme,
            const StandingTree& tree)
        {
            const FloatPoint foot = pointAt(frame, tree.footShare);
            drawSpruceShadow(frame, foot, tree.size * sizeAt(frame, foot.y), theme.shadow);
        }

        void drawBroadleaf(const SceneFrame& frame, const SceneTheme& theme,
            const StandingTree& tree)
        {
            const FloatPoint foot = pointAt(frame, tree.footShare);
            const float parallax = parallaxAt(frame, foot.y);
            const Color crown = theme.treeLeavesBase.fullyOpaque();
            const Color crownShade = Color::between(crown, Color{ 0xFF000000 }, 0.25f);
            drawBroadleafTree(frame, foot, tree.size * sizeAt(frame, foot.y), BroadleafTreeColors{
                .crown = theme.hazed(crown, parallax),
                .crownMiddle = theme.hazed(theme.crownMiddle.fullyOpaque(), parallax),
                .crownLight = theme.hazed(theme.crownLight.fullyOpaque(), parallax),
                .crownShade = theme.hazed(crownShade, parallax).faded(0.6f),
                .trunk = theme.hazed(theme.bark, parallax),
                .trunkShade = theme.hazed(theme.barkShade, parallax),
                .shadow = theme.shadow
            });
        }

        // The water shows the sky's light near the horizon and its own deep color close by.
        void drawRiverBed(const SceneFrame& frame, const SceneTheme& theme, const PixelPath& river)
        {
            const float groundY = frame.height * k_horizonShare;
            const Color farWater = Color::between(theme.riverTop, theme.skyBottom, 0.55f);
            fillShape(frame, river, LinearGradient{
                .startPoint = { 0.0f, groundY },
                .endPoint = { 0.0f, frame.height },
                .stops = {
                    { .position = 0.0f,
                        .color = theme.hazed(farWater, parallaxAt(frame, groundY)) },
                    { .position = 0.25f,
                        .color = Color::between(theme.riverTop, theme.skyBottom, 0.3f) },
                    { .position = 0.55f, .color = theme.riverTop },
                    { .position = 1.0f,
                        .color = Color::between(theme.riverBottom, theme.skyTop, 0.15f) }
                }
            });
        }

        // An arched footbridge of planks with a rail on posts.
        void drawBridge(const SceneFrame& frame, const SceneTheme& theme)
        {
            const FloatPoint foot = pointAt(frame, k_bridgeFootShare);
            const float size = k_bridgeSize * sizeAt(frame, foot.y);
            const Matrix3x2 place = Matrix3x2::Translation(foot) * Matrix3x2::Scale(size);
            const float parallax = parallaxAt(frame, foot.y);
            const Color clearWood = theme.bough;
            const Color wood = theme.hazed(clearWood, parallax);
            const Color woodShade = theme.hazed(Color::between(clearWood, Color{ 0xFF000000 }, 0.35f), parallax);
            const Color railLight = theme.hazed(Color::between(clearWood, theme.sunDisk, 0.12f),
                parallax);

            PixelPath deck;
            deck.moveTo(-62.0f, 0.0f);
            deck.quadTo({ 0.0f, -30.0f }, { 62.0f, 0.0f });
            deck.lineTo(62.0f, 6.0f);
            deck.quadTo({ 0.0f, -22.0f }, { -62.0f, 6.0f });
            deck.close();
            fillShape(frame, deck, SolidColor{ wood }, place);
            PixelPath underside;
            underside.moveTo(-62.0f, 6.0f);
            underside.quadTo({ 0.0f, -22.0f }, { 62.0f, 6.0f });
            underside.lineTo(62.0f, 9.0f);
            underside.quadTo({ 0.0f, -17.0f }, { -62.0f, 9.0f });
            underside.close();
            fillShape(frame, underside, SolidColor{ woodShade }, place);

            PixelPath rail;
            rail.moveTo(-58.0f, -14.0f);
            rail.quadTo({ 0.0f, -44.0f }, { 58.0f, -14.0f });
            strokeShape(frame, rail, SolidColor{ railLight }, 2.6f, StrokeCap::Round, place);
            PixelPath posts;
            for (int i = -2; i <= 2; ++i)
            {
                const float x = static_cast<float>(i) * 27.0f;
                const float deckTop = -15.0f * (1.0f - (x / 62.0f) * (x / 62.0f));
                const float railHeight = -14.0f - 15.0f * (1.0f - (x / 58.0f) * (x / 58.0f));
                posts.moveTo(x, deckTop);
                posts.lineTo(x, railHeight);
            }
            strokeShape(frame, posts, SolidColor{ wood }, 3.2f, StrokeCap::Round, place);
            PixelPath railShade;
            railShade.moveTo(-55.0f, -8.0f);
            railShade.quadTo({ 0.0f, -38.0f }, { 55.0f, -8.0f });
            strokeShape(frame, railShade, SolidColor{ woodShade }, 1.6f, StrokeCap::Round, place);
        }

        // Each mirrored about the line where it meets the ground or the water. The house is kept to
        // the rows below its foot, where its reflection lies.
        void drawStillReflections(const SceneFrame& frame, const SceneTheme& theme,
            const TimberMillColors& millColors)
        {
            const SceneFrame aboutHorizon = mirroredAbout(frame, frame.height * k_horizonShare);
            drawMountains(aboutHorizon, theme, k_farRange);
            drawMountains(aboutHorizon, theme, k_nearRange);
            drawFarHills(aboutHorizon, theme);

            const FloatPoint bridgeFoot = pointAt(frame, k_bridgeFootShare);
            const float bridgeSize = k_bridgeSize * sizeAt(frame, bridgeFoot.y);
            const float bridgeDeck = bridgeFoot.y + 6.0f * bridgeSize;
            drawBridge(mirroredAbout(frame, bridgeDeck), theme);
            for (const StandingTree& tree : k_shoreFirs)
            {
                const float foot = frame.height * tree.footShare.y;
                drawFir(mirroredAbout(frame, foot), theme, tree);
            }

            const TimberMillPose mill = millPoseOf(frame, 0.0f);
            frame.canvas.pushClip(frame.toCanvas.mapRect({ 0.0f, mill.base.y, frame.width,
                frame.height }));
            drawTimberMillHouse(mirroredAbout(frame, mill.base.y), mill, millColors);
            frame.canvas.popClip();
        }

        void drawMovingReflections(const SceneFrame& frame, const SceneTheme& theme,
            const float cycleTime, const CowMoment& cow, const std::optional<DuckFamily>& ducks)
        {
            const TimberMillPose mill = millPoseOf(frame, cycleTime);
            const TimberMillColors millColors = millColorsOf(theme);
            const SceneFrame aboutMillFoot = mirroredAbout(frame, mill.base.y);
            frame.canvas.pushClip(frame.toCanvas.mapRect({ 0.0f, mill.base.y, frame.width,
                frame.height }));
            drawTimberMillWheel(aboutMillFoot, mill, millColors);
            drawTimberMillWings(aboutMillFoot, mill, millColors);
            frame.canvas.popClip();
            // Mirrored about its feet, the cow shows where the water reaches its reflection.
            drawPosedCow(mirroredAbout(frame, cow.pose.foot.y), cow.pose, cowColorsOf(theme));
            drawShoreCattails(frame, theme, cycleTime, true);
            drawReedBeds(frame, theme, cycleTime, true);
            if (!ducks)
                return;
            for (std::size_t i = 0; i != ducks->size(); ++i)
            {
                const SwimmingDuck& duck = (*ducks)[i];
                drawDuck(mirroredAbout(frame, duck.at.y), duck, duckColorsOf(theme, i));
            }
        }

        void drawRings(const SceneFrame& frame, const FloatPoint center, const float age,
            const float size, const Color wave)
        {
            constexpr int k_rings{ 3 };
            constexpr float k_ringLife{ 2.6f };
            for (int k = 0; k != k_rings; ++k)
            {
                const float ringAge = age - static_cast<float>(k) * 0.45f;
                if (ringAge <= 0.0f || ringAge > k_ringLife)
                    continue;
                const float radius = (6.0f + ringAge * 22.0f) * size;
                PixelPath ring;
                ring.drawEllipse(center, radius, radius * 0.3f);
                strokeShape(frame, ring, SolidColor{ wave.faded((1.0f - ringAge / k_ringLife)
                    * 0.9f) }, 1.4f * size);
            }
        }

        void drawFlock(const SceneFrame& frame, const SceneTheme& theme, const float story)
        {
            constexpr std::array<FloatPoint, 5> k_places{ {
                { 0.0f, 0.0f },
                { 22.0f, -10.0f },
                { 22.0f, 10.0f },
                { 44.0f, -20.0f },
                { 44.0f, 20.0f }
            } };
            const float flown = (story - k_birdsFly.from) / (k_birdsFly.until - k_birdsFly.from);
            if (flown < 0.0f || flown > 1.0f)
                return;
            const float unit = frame.unit;
            const FloatPoint leader = pointAt(frame, FloatPoint::between({ 1.05f, 0.24f },
                { -0.08f, 0.2f }, flown));
            const float span = 7.0f * unit;
            PixelPath birds;
            for (std::size_t i = 0; i != k_places.size(); ++i)
            {
                const float flap = std::sin(story * 9.0f + static_cast<float>(i) * 1.3f);
                const FloatPoint at = {
                    leader.x + k_places[i].x * unit,
                    leader.y + k_places[i].y * unit * 0.6f
                };
                birds.moveTo(at.x - span, at.y - flap * span * 0.5f);
                birds.quadTo({ at.x - span * 0.4f, at.y - span * 0.2f }, at);
                birds.quadTo({ at.x + span * 0.4f, at.y - span * 0.2f },
                    { at.x + span, at.y - flap * span * 0.5f });
            }
            strokeShape(frame, birds, SolidColor{ theme.flock }, 1.76f * unit);
        }

        void drawDuck(const SceneFrame& frame, const SwimmingDuck& duck, const DuckColors& colors)
        {
            const SceneFrame body = frame.within(Matrix3x2::Translation(duck.at)
                * Matrix3x2::Scale(duck.size));
            PixelPath shape;
            shape.moveTo(-16.0f, -4.0f);
            shape.cubicTo({ -18.0f, -14.0f }, { 2.0f, -16.0f }, { 10.0f, -10.0f });
            shape.cubicTo({ 16.0f, -6.0f }, { 14.0f, 0.0f }, { 8.0f, 0.0f });
            shape.lineTo(-14.0f, 0.0f);
            shape.close();
            fillShape(body, shape, SolidColor{ colors.body });
            fillOval(body, { -3.0f, -8.0f }, 9.0f, 4.0f, SolidColor{ colors.wing }, -0.15f);
            fillDisc(body, { 10.0f, -16.0f + duck.paddle * 0.6f }, 6.0f, SolidColor{ colors.body });
            shape.clear();
            shape.moveTo(15.0f, -16.0f);
            shape.lineTo(22.0f, -14.5f);
            shape.lineTo(15.0f, -13.0f);
            shape.close();
            fillShape(body, shape, SolidColor{ colors.beak });
            fillDisc(body, { 12.0f, -17.5f }, 1.1f, SolidColor{ colors.eye });
        }

        void drawDuckWake(const SceneFrame& frame, const SwimmingDuck& duck,
            const DuckColors& colors, const Ripples& ripples)
        {
            constexpr float k_wakeWidth{ 1.2f };
            PixelPath wake;
            wake.moveTo(14.0f, 0.0f);
            wake.lineTo(-30.0f, -4.0f);
            wake.moveTo(14.0f, 0.0f);
            wake.lineTo(-30.0f, 5.0f);
            const SolidColor paint{ colors.wake.faded(0.7f) };
            const float margin = k_wakeWidth * duck.size;
            const float top = std::floor(duck.at.y - 4.0f * duck.size - margin);
            const float bottom = duck.at.y + 5.0f * duck.size + margin;
            // A row at a time, clipped to it and moved as the ripples move that row.
            for (float row = top; row < bottom; row += 1.0f)
            {
                const float shift = rippleShiftAt(ripples, frame, row + 0.5f);
                const SceneFrame water = frame.within(Matrix3x2::Translation(duck.at.x + shift,
                    duck.at.y) * Matrix3x2::Scale(duck.size));
                frame.canvas.pushClip(frame.toCanvas.mapRect({ 0.0f, row, frame.width,
                    row + 1.0f }));
                strokeShape(water, wake, paint, k_wakeWidth);
                frame.canvas.popClip();
            }
        }

        void drawDucks(const SceneFrame& frame, const SceneTheme& theme,
            const std::optional<DuckFamily>& ducks, const Ripples& ripples)
        {
            if (!ducks)
                return;
            for (std::size_t i = 0; i != ducks->size(); ++i)
            {
                const SwimmingDuck& duck = (*ducks)[i];
                const DuckColors colors = duckColorsOf(theme, i);
                drawDuckWake(frame, duck, colors, ripples);
                drawDuck(frame, duck, colors);
            }
        }

        void drawFishJumps(const SceneFrame& frame, const SceneTheme& theme, const float story)
        {
            const SolidColor scales{ theme.fish };
            for (const FishJump& jump : k_fishJumps)
            {
                const float age = story - jump.at;
                if (age < 0.0f || age > 4.0f)
                    continue;
                const FloatPoint at = pointAt(frame, jump.share);
                const float size = sizeAt(frame, at.y) * 0.7f;
                drawRings(frame, { at.x - 26.0f * size, at.y }, age, size, theme.foam);
                drawRings(frame, { at.x + 26.0f * size, at.y }, age - 1.0f, size, theme.foam);
                if (age >= 1.0f)
                    continue;
                const FloatPoint arc = {
                    at.x + (-26.0f + 52.0f * age) * size,
                    at.y - std::sin(age * std::numbers::pi_v<float>) * 34.0f * size
                };
                const SceneFrame fish = frame.within(Matrix3x2::Translation(arc)
                    * Matrix3x2::Rotation(-0.9f + 1.8f * age) * Matrix3x2::Scale(size));
                fillOval(fish, {}, 11.0f, 4.0f, scales);
                PixelPath tail;
                tail.moveTo(-9.0f, 0.0f);
                tail.lineTo(-17.0f, -5.0f);
                tail.lineTo(-17.0f, 5.0f);
                tail.close();
                fillShape(fish, tail, scales);
            }
        }

        void drawButterfly(const SceneFrame& frame, const SceneTheme& theme, const float story)
        {
            const float daylight = 1.0f - theme.hasStars;
            if (daylight <= 0.0f)
                return;
            const float turn = story / k_storySeconds * k_2Pi;
            const FloatPoint at = {
                frame.width * (0.12f + 0.07f * std::sin(turn * 5.0f)
                    + 0.01f * std::sin(turn * 17.0f)),
                frame.height * (0.82f + 0.035f * std::sin(turn * 7.0f)
                    + 0.008f * std::cos(turn * 24.0f))
            };
            const float open = std::abs(std::sin(turn * 178.0f));
            const SceneFrame butterfly = frame.within(Matrix3x2::Translation(at)
                * Matrix3x2::Scale(1.3f * frame.unit));
            const SolidColor wing{ theme.shadedForeground(theme.wing).faded(daylight) };
            const SolidColor edge{ theme.shadedForeground(theme.wingEdge).faded(daylight) };
            for (const float side : { -1.0f, 1.0f })
            {
                PixelPath shape;
                addEllipse(shape, {}, 4.5f * open + 0.5f, 4.0f);
                const Matrix3x2 upper = Matrix3x2::Translation(side * 4.0f * open, -3.0f)
                    * Matrix3x2::Rotation(side * 0.4f);
                fillShape(butterfly, shape, wing, upper);
                strokeShape(butterfly, shape, edge, 0.8f, StrokeCap::Round, upper);
                shape.clear();
                addEllipse(shape, {}, 3.0f * open + 0.4f, 2.6f);
                const Matrix3x2 lower = Matrix3x2::Translation(side * 3.0f * open, 2.0f)
                    * Matrix3x2::Rotation(-side * 0.4f);
                fillShape(butterfly, shape, wing, lower);
                strokeShape(butterfly, shape, edge, 0.8f, StrokeCap::Round, lower);
            }
            fillBox(butterfly, { -0.6f, -5.0f, 0.6f, 4.0f }, edge);
        }

        // A layer's weather falls in a frame as tall as its bottom row, so it stops there, and at
        // a pace that makes it fall as many of its own sizes a second as the mill's does.
        void drawEffectLayer(const SceneFrame& frame, const SceneTheme& theme,
            const float cycleTime, const std::size_t layerIndex)
        {
            const EffectLayer& layer = k_effectLayers[layerIndex];
            const float seed = k_effectSeedStep * static_cast<float>(layerIndex);
            Fireflies fireflies = firefliesOf(theme);
            fireflies.count = layer.fireflies;
            fireflies.size = layer.size;
            fireflies.seed = seed;
            fireflies.area = { 0.0f, frame.height * layer.firefliesTopShare, frame.width,
                frame.height * layer.firefliesBottomShare };
            const float parallax = parallaxAt(frame, fireflies.area.bottom);
            fireflies.light = theme.hazed(fireflies.light, parallax);
            fireflies.glow = theme.hazed(fireflies.glow, parallax);
            drawFireflies(frame, cycleTime, fireflies);

            Weather weather = weatherOf(theme);
            weather.dust *= 0.5f; // faded, as it blows across the whole picture
            weather.opacity = layer.opacity;
            weather.density = layer.density * layer.bottomShare;
            weather.fallPace = layer.size / layer.bottomShare;
            weather.windPace = layer.size;
            weather.seed = seed;
            // Mirrored, so the weather blows toward the right, the way the mill's smoke drifts.
            SceneFrame over = frame.within(Matrix3x2::Translation(frame.width, 0.0f)
                * Matrix3x2::Scale(-1.0f, 1.0f));
            over.height = frame.height * layer.bottomShare;
            over.unit = frame.unit * layer.size;
            drawWeather(over, cycleTime, weather);
        }

        // The sun's glitter and glow, on a path from under the sun at the near water up to the
        // bridge, smaller as it goes further off, the sparkles' rows moved as the reflection's are.
        void drawWaterLight(const SceneFrame& frame, const SceneTheme& theme, const float sunX,
            const float cycleTime, const Ripples& ripples)
        {
            constexpr int k_glitters{ 150 };
            constexpr int k_glows{ 7 };
            constexpr float k_glitterOpacity{ 0.75f };
            Color sparkleColor = theme.sunDisk;
            sparkleColor.alpha = theme.sunGlow.alpha;
            const float unit = frame.unit;
            const FloatPoint bridgeFoot = pointAt(frame, k_bridgeFootShare);
            const FloatPoint far = { bridgeFoot.x, bridgeFoot.y + 0.008f * frame.height };
            const FloatPoint near = { sunX, 0.98f * frame.height };
            const PixelPath river = riverOutline(frame);
            frame.canvas.pushClip(river, &frame.toCanvas);
            // A soft glow along the path, flat on the water and wider as it comes nearer.
            for (int i = 0; i != k_glows; ++i)
            {
                const float along = (static_cast<float>(i) + 0.5f) / static_cast<float>(k_glows);
                drawGlow(frame, FloatPoint::between(far, near, along), (6.0f + 40.0f * along) * unit,
                    theme.sunGlow.faded(0.3f), 0.45f);
            }
            for (int i = 0; i != k_glitters; ++i)
            {
                const float along = std::pow(spreadShare(i, k_glitters, 51.0f), 0.7f);
                const FloatPoint middle = FloatPoint::between(far, near, along);
                const float spread = (2.0f + 80.0f * along * along) * unit;
                // Across the path by the golden ratio's turns, so near neighbors along it part.
                const float across = fractionalPart(static_cast<float>(i) * 0.618034f
                    + 0.08f * pseudoRandom(i, 52.0f));
                const float x = middle.x + (across - 0.5f) * 2.0f * spread;
                const float twinkle = std::sin(cycleTime * k_2Pi * std::round(
                    14.0f + 12.0f * pseudoRandom(i, 53.0f)) + static_cast<float>(i) * 2.1f);
                if (twinkle <= 0.0f)
                    continue;
                const float reach = (1.0f + 7.0f * along) * unit;
                const float thickness = (0.4f + 1.1f * along) * unit;
                const float top = middle.y - thickness * 0.5f;
                const float bottom = middle.y + thickness * 0.5f;
                // A slice for each pixel row it covers, moved as the reflection moves that row.
                PixelPath sparkle;
                for (float row = std::floor(top); row < bottom; row += 1.0f)
                {
                    const float sliceTop = std::max(row, top);
                    const float sliceBottom = std::min(row + 1.0f, bottom);
                    const float shift = rippleShiftAt(ripples, frame, row + 0.5f);
                    sparkle.moveTo(x - reach + shift, sliceTop);
                    sparkle.lineTo(x + reach + shift, sliceTop);
                    sparkle.lineTo(x + reach + shift, sliceBottom);
                    sparkle.lineTo(x - reach + shift, sliceBottom);
                    sparkle.close();
                }
                fillShape(frame, sparkle, SolidColor{ sparkleColor.faded(k_glitterOpacity
                    * std::min(1.0f, twinkle * 1.4f)) });
            }
            frame.canvas.popClip();
        }

        // The bank nearest the viewer, with grass, reeds and daisies swaying on it.
        void drawNearBank(const SceneFrame& frame, const SceneTheme& theme, const float cycleTime)
        {
            const float w = frame.width;
            const float h = frame.height;
            const float unit = frame.unit;
            std::vector<FloatPoint> points;
            for (const FloatPoint share : k_nearBank)
                points.push_back(pointAt(frame, share));
            points.push_back(pointAt(frame, { 1.04f, 1.04f }));
            points.push_back(pointAt(frame, { -0.04f, 1.04f }));
            PixelPath bank;
            addSmoothLoop(bank, points);
            // On snow the near bank lies whiter, most of the way to the snow on the hills.
            const float snow = 0.7f * theme.hasSnow;
            const Color lawnTop = Color::between(theme.hillTop, theme.hillBottom, k_nearBankTopShare);
            const Color bankTop = theme.shadedForeground(Color::between(lawnTop, theme.hillTop,
                snow));
            const Color bankBottom = theme.shadedForeground(Color::between(theme.hillBottom,
                theme.hillTop, snow));
            fillShape(frame, bank, LinearGradient::Simple({ 0.0f, h * 0.86f }, { 0.0f, h },
                bankTop, bankBottom));

            const Color grass = theme.shadedForeground(theme.grassBlade.light);
            const Color darkGrass = theme.shadedForeground(theme.grassBlade.dark);
            const float swayPhase = cycleTime * k_2Pi * 8.0f;
            for (const BladeRun& run : k_bladeRuns)
            {
                ScatteredShapes lightBlades{ frame, SolidColor{ grass } };
                ScatteredShapes darkBlades{ frame, SolidColor{ darkGrass } };
                for (int i = 0; i != run.count; ++i)
                {
                    const float x = w * (run.fromShare
                        + (run.toShare - run.fromShare) * pseudoRandom(i, run.seed));
                    const float y = nearBankTopAt(frame, x) + pseudoRandom(i, run.seed + 1.0f)
                        * 0.06f * h + 4.0f;
                    const float height = (12.0f + 44.0f * pseudoRandom(i, run.seed + 2.0f)) * unit;
                    const float sway = std::sin(swayPhase + x * 0.01f) * 4.0f * unit
                        * height / (40.0f * unit);
                    const float lean = (pseudoRandom(i, run.seed + 3.0f) - 0.5f) * 8.0f * unit;
                    const float tipX = x + sway + lean;
                    PixelPath& blade = i % 3 == 0 ? darkBlades.next() : lightBlades.next();
                    blade.moveTo(x - 1.6f * unit, y);
                    blade.quadTo({ x + sway * 0.3f, y - height * 0.6f }, { tipX, y - height });
                    blade.quadTo({ x + sway * 0.3f + 1.0f, y - height * 0.5f },
                        { x + 1.6f * unit, y });
                    blade.close();
                }
                darkBlades.draw();
                lightBlades.draw();
            }

            const CattailColors cattailColors = {
                theme.shadedForeground(theme.reedStem),
                theme.shadedForeground(theme.reedHead),
            };
            for (const Cattail& cattail : k_cattails)
            {
                const float x = w * cattail.xShare;
                const float sway = std::sin(cycleTime * k_2Pi * 7.0f + cattail.swayPhase);
                drawCattail(frame, cattailColors, { x, nearBankTopAt(frame, x) + 6.0f * unit },
                    unit, cattail.height, sway);
            }

            const std::array<Color, 3> petals{
                theme.shadedForeground(theme.petal1),
                theme.shadedForeground(theme.petal2),
                theme.shadedForeground(theme.petal3)
            };
            const Color flowerHeart = theme.shadedForeground(theme.flowerHeart);
            for (std::size_t i = 0; i != k_bankFlowers.size(); ++i)
            {
                const BankFlower& flower = k_bankFlowers[i];
                const float x = w * flower.xShare;
                const float y = nearBankTopAt(frame, x) + flower.depth * unit;
                const float sway = std::sin(swayPhase + static_cast<float>(i)) * 2.0f * unit;
                drawSegment(frame, { x, y + 18.0f * unit }, { x + sway, y },
                    SolidColor{ grass }, 1.6f * unit);
                PixelPath petalRing;
                for (int p = 0; p != 6; ++p)
                {
                    const float angle = static_cast<float>(p) / 6.0f * k_2Pi
                        + static_cast<float>(i);
                    addEllipse(petalRing, { x + sway + std::cos(angle) * 4.5f * unit,
                        y + std::sin(angle) * 3.0f * unit }, 3.0f * unit, 2.2f * unit);
                }
                const Color petal = petals[static_cast<std::size_t>(flower.petal)];
                fillShape(frame, petalRing, SolidColor{ petal });
                fillDisc(frame, { x + sway, y }, 2.4f * unit, SolidColor{ flowerHeart });
            }
        }

        void drawCattail(const SceneFrame& frame, const CattailColors& colors,
            const FloatPoint foot, const float scale, const float height, const float sway)
        {
            const Color stem = colors.stem;
            const Color stemShade = Color::between(stem, Color{ 0xFF000000 }, 0.25f);
            const Color head = colors.head;
            // The head's own color lit, as the Chase lights its reeds' heads, never the sun's.
            Hsl litHead{ head };
            litHead.scaleLuminosity(1.25f);
            const Color headLight = litHead.toColor().faded(0.8f);
            const float x = foot.x;
            const float y = foot.y;
            const float tall = height * scale;
            const float lean = sway * 6.0f * scale;
            // a long leaf leaning each way, the left one in shade
            for (const float side : { -1.0f, 1.0f })
            {
                const float reach = (side < 0.0f ? 26.0f : 18.0f) * scale;
                const float leafHeight = tall * (side < 0.0f ? 0.78f : 0.62f);
                PixelPath leaf;
                leaf.moveTo(x - 2.6f * scale, y);
                leaf.quadTo({ x + side * 6.0f * scale + lean * 0.2f, y - leafHeight * 0.6f },
                    { x + side * reach + lean * 0.7f, y - leafHeight });
                leaf.quadTo({ x + side * 3.0f * scale, y - leafHeight * 0.5f },
                    { x + 2.6f * scale, y });
                leaf.close();
                fillShape(frame, leaf, SolidColor{ side < 0.0f ? stemShade : stem });
            }
            PixelPath stalk;
            stalk.moveTo(x, y);
            stalk.quadTo({ x + lean * 0.2f, y - tall * 0.6f }, { x + lean, y - tall });
            strokeShape(frame, stalk, SolidColor{ stem }, 3.0f * scale);
            const FloatPoint headMiddle = { x + lean * 0.93f, y - tall * 0.86f };
            const Matrix3x2 headPlace = Matrix3x2::Translation(headMiddle)
                * Matrix3x2::Rotation(lean / tall);
            PixelPath headShape;
            addEllipse(headShape, {}, 4.6f * scale, 13.0f * scale);
            fillShape(frame, headShape, SolidColor{ head }, headPlace);
            PixelPath headShine;
            addEllipse(headShine, { 1.6f * scale, -2.0f * scale }, 1.6f * scale, 9.0f * scale);
            fillShape(frame, headShine, SolidColor{ headLight }, headPlace);
        }

        void drawShoreCattails(const SceneFrame& frame, const SceneTheme& theme,
            const float cycleTime, const bool mirrored)
        {
            const CattailColors colors = { theme.reedStem, theme.reedHead };
            const auto drawRow = [&](const std::span<const Cattail> cattails,
                const std::span<const FloatPoint> shore) {
                for (const Cattail& cattail : cattails)
                {
                    const float x = frame.width * cattail.xShare;
                    const float shoreY = rowAlong(frame, shore, x);
                    const float scale = k_shoreCattailScale * sizeAt(frame, shoreY);
                    const FloatPoint foot = { x, shoreY + 2.0f * scale };
                    const float sway = std::sin(cycleTime * k_2Pi * 7.0f + cattail.swayPhase);
                    drawCattail(mirrored ? mirroredAbout(frame, foot.y) : frame, colors, foot,
                        scale, cattail.height, sway);
                }
            };
            drawRow(k_leftShoreCattails, k_leftShore);
            drawRow(k_farShoreCattails, k_farShore);
        }

        void drawReedBeds(const SceneFrame& frame, const SceneTheme& theme, const float cycleTime,
            const bool mirrored)
        {
            for (const ReedBed& bed : k_reedBeds)
                drawReedBed(frame, theme, bed, cycleTime, mirrored);
        }

        void drawReedBed(const SceneFrame& frame, const SceneTheme& theme, const ReedBed& bed,
            const float cycleTime, const bool mirrored)
        {
            const FloatPoint middle = pointAt(frame, bed.share);
            const float parallax = parallaxAt(frame, middle.y);
            const float scale = bed.scale * sizeAt(frame, middle.y);
            const float halfWidth = bed.halfWidthShare * frame.width;
            const float rootsDepth = 6.0f * scale; // how far under its middle a blade's roots go
            // Mirrored about its lowest roots, where the water meets it.
            const SceneFrame drawing = mirrored ? mirroredAbout(frame, middle.y + rootsDepth)
                : frame;
            const float swayPhase = cycleTime * k_2Pi * 8.0f;
            ScatteredShapes lightBlades{ drawing,
                SolidColor{ theme.hazed(theme.grassBlade.light, parallax) } };
            ScatteredShapes darkBlades{ drawing,
                SolidColor{ theme.hazed(theme.grassBlade.dark, parallax) } };
            for (int i = 0; i != bed.bladeCount; ++i)
            {
                // From -1 at the bed's left end to 1 at its right.
                const float across = pseudoRandom(i, 61.0f + bed.seed) * 2.0f - 1.0f;
                const float x = middle.x + across * halfWidth;
                const float y = middle.y + pseudoRandom(i, 62.0f + bed.seed) * rootsDepth;
                // The blades lower toward the bed's ends and lean out over them.
                const float height = (34.0f + 34.0f * pseudoRandom(i, 63.0f + bed.seed))
                    * (1.0f - 0.35f * std::abs(across)) * scale;
                const float lean = (across * 14.0f
                    + (pseudoRandom(i, 64.0f + bed.seed) - 0.5f) * 10.0f) * scale;
                const float sway = std::sin(swayPhase + static_cast<float>(i)) * 3.0f * scale;
                const float tipX = x + lean + sway;
                PixelPath& blade = i % 3 == 0 ? darkBlades.next() : lightBlades.next();
                blade.moveTo(x - 6.0f * scale, y);
                blade.quadTo({ x + lean * 0.3f, y - height * 0.6f }, { tipX, y - height });
                blade.quadTo({ x + lean * 0.3f + 2.0f * scale, y - height * 0.5f },
                    { x + 6.0f * scale, y });
                blade.close();
            }
            darkBlades.draw();
            const CattailColors cattailColors = {
                theme.hazed(theme.reedStem, parallax),
                theme.hazed(theme.reedHead, parallax)
            };
            const float cattailSide = bed.cattailsFlipped ? -1.0f : 1.0f;
            for (const ReedBedCattail& cattail : k_reedBedCattails)
            {
                const float sway = std::sin(cycleTime * k_2Pi * 7.0f + cattail.swayPhase
                    + bed.seed);
                const FloatPoint foot = {
                    middle.x + cattailSide * cattail.offset * halfWidth,
                    middle.y + 3.0f * scale
                };
                drawCattail(drawing, cattailColors, foot, scale, cattail.height, sway);
            }
            lightBlades.draw();
        }
    }
}
