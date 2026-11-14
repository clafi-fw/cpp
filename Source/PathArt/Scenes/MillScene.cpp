module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.Air;
import ClaFi.PathArt.CowSprite;
import ClaFi.PathArt.FirefliesEffect;
import ClaFi.PathArt.FirTreeSprite;
import ClaFi.PathArt.GatorHeadSprite;
import ClaFi.PathArt.LightningEffect;
import ClaFi.PathArt.MillSprite;
import ClaFi.PathArt.PeekingWolfSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.TrunkSprite;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WaterLilySprite;
import ClaFi.PathArt.WaterWheelSprite;
import ClaFi.PathArt.WeatherEffect;
import ClaFi.Core.Graphics.Cpu.Canvas;
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

        // A run of grass blades along the near bank.
        struct BladeRun
        {
            float fromShare; // of the frame's width
            float toShare;
            int count;
            float seed;
        };

        // A cattail on the near bank: where it stands, how tall, and where its sway starts.
        struct Cattail
        {
            float xShare;
            float height; // units of the frame
            float swayPhase;
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
        constexpr int k_farHillTrees{ 70 };
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
        constexpr int k_meadowBlades{ 900 };
        constexpr int k_meadowFlowers{ 150 };
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

        // The river from where it comes out at the far hills' foot down to the pond.
        constexpr std::array<StreamPoint, 7> k_upstream{
            StreamPoint{ { 0.41f, 0.5605f }, 0.0f },
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

        constexpr float k_firSize{ 1.2f };
        // Back to front; each stands still, so the land keeps it.
        constexpr std::array<StandingTree, 6> k_stillFirs{
            StandingTree{ { 0.09f, 0.655f }, 0.8f },
            StandingTree{ { 0.155f, 0.665f }, 0.9f },
            StandingTree{ { 0.70f, 0.64f }, 0.9f },
            StandingTree{ { 0.118f, 0.778f }, 0.85f },
            StandingTree{ { 0.79f, 0.785f }, 0.9f },
            StandingTree{ { 0.045f, 0.812f }, 1.0f }
        };
        constexpr std::array<StandingTree, 2> k_broadleaves{
            StandingTree{ { 0.205f, 0.672f }, 2.2f },
            StandingTree{ { 0.58f, 0.70f }, 2.0f }
        };
        constexpr FloatPoint k_cowFootShare{ 0.445f, 0.718f };
        constexpr float k_cowSize{ 0.55f };
        // The fir the predator peeks from behind, across the field a little behind the cow's line.
        constexpr StandingTree k_wolfFir{ { 0.884f, k_cowFootShare.y - 0.018f }, 1.05f };
        constexpr FloatPoint k_lilyShare{ 0.31f, 0.915f };
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
        constexpr std::array<BladeRun, 2> k_bladeRuns{
            BladeRun{ -0.02f, 0.36f, 120, 131.0f },
            BladeRun{ 0.66f, 1.02f, 110, 141.0f }
        };
        constexpr std::array<BankFlower, 6> k_bankFlowers{
            BankFlower{ 0.17f, 30.0f, 0 },
            BankFlower{ 0.235f, 40.0f, 1 },
            BankFlower{ 0.27f, 52.0f, 2 },
            BankFlower{ 0.72f, 36.0f, 1 },
            BankFlower{ 0.78f, 26.0f, 0 },
            BankFlower{ 0.9f, 30.0f, 2 }
        };

        [[nodiscard]] MillColors millColorsOf(const SceneTheme&);
        // The near bank's colour for the land's: darker, and no more colourful.
        [[nodiscard]] Color nearBankColor(Color landColor);
        // The parallax of the valley floor at `y`, the mill's foot being 1.
        [[nodiscard]] float parallaxAt(const SceneFrame&, float y);
        // Pixels per unit of the frame for what stands at `y`, at its distance.
        [[nodiscard]] float sizeAt(const SceneFrame&, float y);
        // The land's clear colour at row `y`.
        [[nodiscard]] Color landAt(const SceneFrame&, const SceneTheme&, float y);
        // The land from `top` down, each row hazed as far as it lies, and at least as `parallax`.
        [[nodiscard]] LinearGradient landGradient(const SceneFrame&, const SceneTheme&, float top,
            float parallax);
        [[nodiscard]] FloatPoint pointAt(const SceneFrame&, FloatPoint share);
        // A closed curve through the points, appended to the path.
        void addSmoothLoop(PixelPath&, std::span<const FloatPoint> points);
        // The left and right edges of a stream, in pixels, walking down it.
        void streamEdges(const SceneFrame&, std::span<const StreamPoint> stream,
            std::vector<FloatPoint>& left, std::vector<FloatPoint>& right);
        [[nodiscard]] PixelPath riverOutline(const SceneFrame&);
        [[nodiscard]] float nearBankTopAt(const SceneFrame&, float x);
        [[nodiscard]] float ridgeHeightShare(const MountainRange&, float xShare);
        [[nodiscard]] float farHillsTopAt(const SceneFrame&, float x);
        // How near the viewer row `y` lies, from 0 at the horizon to 1 at the frame's bottom.
        [[nodiscard]] float nearnessAt(const SceneFrame&, float y);

        void drawSky(const SceneFrame&, const SceneTheme&, FloatPoint sun, float sunRadius);
        void drawMountains(const SceneFrame&, const SceneTheme&, const MountainRange&);
        void drawFarHills(const SceneFrame&, const SceneTheme&);
        void drawValley(const SceneFrame&, const SceneTheme&);
        void drawMeadow(const SceneFrame&, const SceneTheme&);
        void drawFootpath(const SceneFrame&, const SceneTheme&);
        void drawFir(const SceneFrame&, const SceneTheme&, const StandingTree&);
        void drawBroadleaf(const SceneFrame&, const SceneTheme&, const StandingTree&);
        void drawRiverBed(const SceneFrame&, const SceneTheme&);
        void drawBridge(const SceneFrame&, const SceneTheme&);
        void drawWaterLight(const SceneFrame&, const SceneTheme&, float sunX, float cycleTime);
        void drawNearBank(const SceneFrame&, const SceneTheme&, float cycleTime);
    }

    void MillScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        m_offset = bounds.topLeft(); // Anchored to widget's logical position
        const float w = bounds.width();
        const float h = bounds.height();
        m_objectsScale = std::min(h, w) / 600.0f;
        m_sunPos = { w * 0.75f, h * 0.15f };
        m_sunRadius = 35.0f * m_objectsScale;
        const float mS = m_objectsScale;
        const float cycleTime = time.cycleTime;

        if (m_stillLayersSize != bounds.dimensions()
            || m_stillLayersThemeVersion != m_themes.wornVersion())
        {
            keepStillLayers(bounds);
        }

        canvas.drawBitmap(m_sky, m_offset);
        // The stars, the sun's rays and the clouds stand behind the land, which hides their foot.
        drawStars(frame, cycleTime, StarField{
            .style = StarStyle::Sparkles,
            .count = 120,
            .top = 0.0f,
            .bottom = h * k_horizonShare,
            .color = theme().sunDisk,
            .opacity = theme().hasStars
        });
        drawSunCorona(canvas, m_offset.x + m_sunPos.x, m_offset.y + m_sunPos.y, m_sunRadius,
            cycleTime);

        const float orbX = m_offset.x + w * 0.5f;
        const float orbY = m_offset.y + h * 0.2f;
        const float rX = w * 0.35f;
        const float rY = h * 0.04f;
        const float a1 = 0.5f * cycleTime * k_2Pi;
        const float a2 = a1 + 3.1415f;
        drawCloud(canvas, orbX + std::cos(a1) * rX, orbY + std::sin(a1) * rY,
            (0.8f + (std::sin(a1) + 1.0f) * 0.2f) * m_objectsScale);
        drawCloud(canvas, orbX + std::cos(a2) * rX, orbY + std::sin(a2) * rY,
            (0.8f + (std::sin(a2) + 1.0f) * 0.2f) * m_objectsScale);

        canvas.drawBitmap(m_land, m_offset);
        drawWaterLight(frame, theme(), m_sunPos.x, cycleTime);

        // river fog along the pond's far shore
        canvas.fillRectangle(bounds, PointGlow{
            .lightPos = pointAt(frame, { 0.6f, 0.795f }) + m_offset,
            .lightColor = faded(theme().cloud, 0.33f),
            .lightSpread = 60.0f * m_objectsScale,
            .xRatio = 0.08f,
            .shape = GlowShape::Circle
        });

        // The predator behind the fir on the right: a gator in the African themes, a wolf
        // elsewhere, crossfading between them.
        const FloatPoint wolfFirFoot = pointAt(frame, k_wolfFir.footShare);
        const float wolfFirSize = k_firSize * k_wolfFir.size * sizeAt(frame, wolfFirFoot.y);
        const FloatPoint predator = {
            wolfFirFoot.x - 24.2f * wolfFirSize,
            wolfFirFoot.y - 20.0f * wolfFirSize
        };
        const float predatorSize = wolfFirSize * 0.667f;
        drawGatorHead(frame, GatorHeadPose{
            .at = predator,
            .size = predatorSize,
            .cycleTime = cycleTime,
            .opacity = theme().isAfrica
        }, GatorHeadColors{
            .body = theme().wolfBody,
            .eye = theme().wolfEye
        });
        drawPeekingWolf(frame, PeekingWolfPose{
            .at = predator,
            .size = predatorSize,
            .cycleTime = cycleTime,
            .opacity = 1.0f - theme().isAfrica
        }, PeekingWolfColors{
            .body = theme().wolfBody,
            .ear = theme().wolfEar,
            .eye = theme().wolfEye
        });
        drawFir(frame, theme(), k_wolfFir);

        const FloatPoint cowFoot = pointAt(frame, k_cowFootShare);
        const float cowSize = k_cowSize * sizeAt(frame, cowFoot.y);
        const Color meadowBehindCow = hazed(theme(), landAt(frame, theme(), cowFoot.y),
            parallaxAt(frame, cowFoot.y));
        // The cow's whole figure stands against the meadow.
        drawCow(frame, CowPose{
            .at = { cowFoot.x, cowFoot.y - 40.0f * cowSize },
            .size = cowSize,
            .cycleTime = cycleTime
        }, CowColors{
            .body = theme().cowBody,
            .spots = theme().cowSpots,
            .snout = theme().cowSnout,
            .detail = theme().cowDetail,
            .shadow = theme().shadow,
            .skyBehind = meadowBehindCow,
            .groundBehind = meadowBehindCow
        });

        const FloatPoint millFoot = pointAt(frame, k_millFootShare);
        const float millSize = k_millSize * mS;
        const MillPose mill = {
            .base = millFoot,
            .size = millSize,
            .wingTurns = cycleTime
        };
        const FloatPoint wheelCenter = { millFoot.x - 65.0f * millSize, millFoot.y };
        drawWaterWheel(frame, WaterWheelPose{
            .center = wheelCenter,
            .size = millSize,
            .turns = cycleTime * 2.0f
        }, theme().millWheel);
        // The river covers the wheel's lower half.
        const float waterLine = m_offset.y + millFoot.y;
        canvas.pushClip(FloatRect{ bounds.left, waterLine, bounds.right, bounds.bottom });
        fillDisc(frame, wheelCenter, 44.0f * millSize,
            SolidColor{ theme().riverTop.withOpacity(0.6f) });
        canvas.popClip();
        // The house's reflection, which the sprite draws below its foot, is left out.
        canvas.pushClip(FloatRect{ bounds.left, bounds.top, bounds.right, waterLine });
        const MillColors millColors = millColorsOf(theme());
        drawMillHouse(frame, mill, millColors);
        canvas.popClip();
        drawWaterFoam(canvas, m_offset.x + millFoot.x - 75.0f * millSize, waterLine, millSize,
            cycleTime * 1.25f);
        drawLightning(frame, cycleTime, Lightning{
            .strength = theme().hasRain,
            .color = theme().sunDisk
        });
        drawMillWings(frame, mill, millColors);

        const FloatPoint lily = pointAt(frame, k_lilyShare);
        drawWaterLily(frame, WaterLilyPose{
            .center = lily,
            .size = k_lilySize * sizeAt(frame, lily.y),
            .cycleTime = cycleTime
        }, WaterLilyColors{
            .pad = theme().lilyPad,
            .flower = theme().lilyFlower
        });
        drawNearBank(frame, theme(), cycleTime);

        Fireflies fireflies = firefliesOf(theme());
        fireflies.area = { 0.0f, h * 0.6f, w, h * 0.88f };
        drawFireflies(frame, cycleTime, fireflies);
        Weather weather = weatherOf(theme());
        weather.dust *= 0.5f; // faded, as it blows across the whole picture
        drawWeather(frame, cycleTime, weather);
    }

    // In Clear Day's colours whatever theme is worn: an icon is one picture in every theme.
    void MillScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        constexpr float k_figureSpan{ 480.0f };
        constexpr Themes::ClearDay k_iconTheme{};
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const float unit = frame.unit;
        const MillPose mill = {
            .base = { frame.width * 0.5f, frame.height * 0.5f + 240.0f * unit },
            .size = 2.0f * unit,
            .wingTurns = 0.06f
        };
        const MillColors millColors = millColorsOf(k_iconTheme);
        // The house's reflection hangs below its foot, and the wing tips reach past the top.
        canvas.pushClip(FloatRect{ bounds.left, bounds.top, bounds.right,
            bounds.top + mill.base.y });
        drawMillHouse(frame, mill, millColors);
        drawMillWings(frame, mill, millColors);
        canvas.popClip();
    }

    // **The picked theme's colours, not the blend's**: a host asks the moment a theme is picked,
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
            drawSky(frame, theme(), m_sunPos, m_sunRadius);
        });
        keep(m_land, true, [&](const SceneFrame& frame) {
            drawMountains(frame, theme(), k_farRange);
            drawMountains(frame, theme(), k_nearRange);
            drawFarHills(frame, theme());
            drawValley(frame, theme());
            drawMeadow(frame, theme());
            drawFootpath(frame, theme());
            for (const StandingTree& tree : std::span{ k_stillFirs }.first(3))
                drawFir(frame, theme(), tree);
            for (const StandingTree& tree : k_broadleaves)
                drawBroadleaf(frame, theme(), tree);
            drawRiverBed(frame, theme());
            drawBridge(frame, theme());
            for (const StandingTree& tree : std::span{ k_stillFirs }.last(3))
                drawFir(frame, theme(), tree);
        });

        g_rasterBuffers.hasActiveClip = heldClipActive;
        g_rasterBuffers.activeClipMaskPtr = heldClipMask;
        g_rasterBuffers.clipBox = heldClipBox;
        g_rasterBuffers.m_clipStride = heldClipStride;
    }

    void MillScene::drawWaterFoam(Canvas& canvas, const float x, const float y, const float mS,
        const float t) const
    {
        const Matrix3x2 tx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(mS);
        for (int i = -2; i <= 2; ++i)
        {
            const float index = static_cast<float>(i);
            PixelPath path;
            const float ox = index * 15.0f + std::sin(t * 10.0f + index) * 5.0f;
            path.moveTo(ox - 10.0f, 0.0f);
            path.quadTo({ ox, -10.0f }, { ox + 10.0f, 0.0f });
            canvas.fillPath(path, SolidColor{ theme().foam }, &tx);
        }
    }

    void MillScene::drawSunCorona(Canvas& canvas, const float x, const float y, const float r,
        const float t)
    {
        const float halfTime = t * 0.5f;
        const float minL = r * 1.3f;
        const float spr = r * 0.25f;
        const float rW = r * 0.12f;
        const float rB = r * 1.1f;
        for (int i = 0; i < 16; ++i)
        {
            const float index = static_cast<float>(i);
            const float sV = std::sin(k_2Pi * (halfTime * 2.0f) + index * 0.8f);
            const float dL = minL + (spr * (0.5f + 0.5f * sV));
            const Matrix3x2 tx = Matrix3x2::Translation(x, y)
                * Matrix3x2::Rotation((halfTime + index / 16.0f) * k_2Pi);

            PixelPath path;
            path.moveTo(-rW, -rB);
            path.lineTo(rW, -rB);
            path.lineTo(0.0f, -dL);
            path.close();
            const Color ray = i % 2 == 0 ? theme().sunCorona1 : theme().sunCorona2;
            canvas.fillPath(path, SolidColor{ ray }, &tx);
        }
    }

    void MillScene::drawCloud(Canvas& canvas, const float x, const float y, const float s)
    {
        const Matrix3x2 tx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(s);
        PixelPath path;
        path.moveTo(-60.0f, 0.0f);
        path.quadTo({ -40.0f, -40.0f }, { -20.0f, -20.0f });
        path.quadTo({ 0.0f, -60.0f }, { 20.0f, -20.0f });
        path.quadTo({ 40.0f, -40.0f }, { 60.0f, 0.0f });
        path.lineTo(60.0f, 20.0f);
        path.lineTo(-60.0f, 20.0f);
        path.close();

        // Direct, zero-copy, multi-layered drawing! [CP]
        canvas.drawPath(
            path,
            {
                PathDrawLayer::Fill(theme().cloud),
                PathDrawLayer::OuterGlow(theme().cloud, 10.0f)
            },
            &tx
        );
    }

    namespace
    {
        MillColors millColorsOf(const SceneTheme& theme)
        {
            return {
                .bodyTop = theme.millBodyTop,
                .bodyBottom = theme.millBodyBottom,
                .roofTop = theme.millRoofTop,
                .roofBottom = theme.millRoofBottom,
                .wingWood = theme.chase.bark,
                .wingWoodStroke = theme.chase.barkShade,
                .wingFabric = theme.millWingFabric,
                .wingFabricStroke = theme.chase.barkShade
            };
        }

        Color nearBankColor(const Color landColor)
        {
            return colorBetween(landColor, Color{ 0xFF000000 }, 0.05f);
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
            return colorBetween(theme.hillTop, theme.hillBottom, share);
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
                    .color = hazed(theme, landAt(frame, theme, y), rowParallax) });
            }
            return land;
        }

        FloatPoint pointAt(const SceneFrame& frame, const FloatPoint share)
        {
            return { frame.width * share.x, frame.height * share.y };
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

        float nearBankTopAt(const SceneFrame& frame, const float x)
        {
            const float share = x / frame.width;
            for (std::size_t i = 0; i + 1 != k_nearBank.size(); ++i)
            {
                const FloatPoint from = k_nearBank[i];
                const FloatPoint to = k_nearBank[i + 1];
                if (share < from.x || share > to.x)
                    continue;
                const float along = (share - from.x) / (to.x - from.x);
                return frame.height * (from.y + (to.y - from.y) * along);
            }
            return frame.height * k_nearBank.back().y;
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

        float nearnessAt(const SceneFrame& frame, const float y)
        {
            const float horizon = frame.height * k_horizonShare;
            return std::clamp((y - horizon) / (frame.height - horizon), 0.0f, 1.0f);
        }

        void drawSky(const SceneFrame& frame, const SceneTheme& theme, const FloatPoint sun,
            const float sunRadius)
        {
            const float w = frame.width;
            const float h = frame.height;
            PixelPath path;
            path.moveTo(0.0f, 0.0f);
            path.lineTo(w, 0.0f);
            path.lineTo(w, h);
            path.lineTo(0.0f, h);
            path.close();
            frame.canvas.drawPath(path, {
                PathDrawLayer::LinearGradient({ 0.0f, 0.0f }, { 0.0f, h * k_horizonShare },
                    theme.skyTop, theme.skyBottom),
                PathDrawLayer::PointGlow({
                    .lightPos = sun,
                    .lightColor = theme.sunGlow,
                    .lightSpread = 166.0f * frame.unit,
                    .xRatio = 0.5f,
                    .shape = GlowShape::Circle
                })
            }, &frame.toCanvas);

            // the sun's disc, or the moon's crescent where the theme has stars
            path.clear();
            const float s = theme.hasStars;
            const Matrix3x2 sunTx = frame.toCanvas * Matrix3x2::Translation(sun)
                * Matrix3x2::Rotation((s * 0.1f) * k_2Pi);
            const float r = sunRadius;
            const float c = r * 0.4142f;
            const float q = r * 0.7071f;
            const float eq = -q * (1.0f - s) + (0.28f * r) * s;
            const float er = -r * (1.0f - s) + (0.50f * r) * s;
            const float cc1 = -c * (1.0f - s) + (0.15f * r) * s;
            const float cc2 = -r * (1.0f - s) + (0.50f * r) * s;
            path.moveTo(0.0f, -r);
            path.quadTo({ c, -r }, { q, -q });
            path.quadTo({ r, -c }, { r, 0.0f });
            path.quadTo({ r, c }, { q, q });
            path.quadTo({ c, r }, { 0.0f, r });
            path.quadTo({ cc1, r }, { eq, q });
            path.quadTo({ cc2, c }, { er, 0.0f });
            path.quadTo({ cc2, -c }, { eq, -q });
            path.quadTo({ cc1, -r }, { 0.0f, -r });
            path.close();
            frame.canvas.drawPath(path, {
                PathDrawLayer::Fill(theme.sunDisk),
                PathDrawLayer::Stroke(theme.sunStroke, 2.0f)
            }, &sunTx);
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
                const Color clear = colorBetween(theme.chase.mountainTop,
                    theme.chase.mountainFoot, position);
                rock.stops.push_back({ .position = position,
                    .color = hazed(theme, clear, range.parallax, aboveGround) });
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
                hazed(theme, landAt(frame, theme, frame.height * 0.48f), k_farHillsParallax),
                hazed(theme, landAt(frame, theme, groundY), k_farHillsParallax)));

            // trees as the Chase's far hills draw them, smaller for the distance
            const Color bark = hazed(theme, theme.chase.bark, k_farHillsParallax);
            const Color barkShade = hazed(theme, theme.chase.barkShade, k_farHillsParallax);
            const SolidColor crown{ hazed(theme, theme.treeLeavesBase, k_farHillsParallax) };
            const SolidColor crownLight{
                hazed(theme, theme.chase.crownLight, k_farHillsParallax) };
            const SolidColor crownMiddle{
                hazed(theme, theme.chase.crownMiddle, k_farHillsParallax) };
            for (int i = 0; i != k_farHillTrees; ++i)
            {
                const float x = pseudoRandom(i, 11.0f) * frame.width;
                const float downSlope = pseudoRandom(i, 12.0f) * 0.022f * frame.height;
                const float y = std::min(groundY, farHillsTopAt(frame, x) + downSlope) + 2.0f;
                const float size = (0.13f + 0.1f * pseudoRandom(i, 13.0f)) * frame.unit;
                drawTrunk(frame, { x, y }, 3.0f * size, 22.0f * size, bark, barkShade);
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

        // Short blades of grass and flowers' specks over the valley, smaller far off.
        void drawMeadow(const SceneFrame& frame, const SceneTheme& theme)
        {
            const float groundY = frame.height * k_horizonShare;
            const std::array<Color, 3> bladeColors{ theme.chase.grass, theme.chase.bladeMiddle,
                theme.chase.bladeDark };
            const std::array<Color, 3> petals{ theme.petal1, theme.petal2, theme.petal3 };
            const float bandDepth = (frame.height - groundY)
                / static_cast<float>(k_meadowDepthBands);
            for (int band = 0; band != k_meadowDepthBands; ++band)
            {
                const float bandTop = groundY + bandDepth * static_cast<float>(band);
                const float bandMiddle = bandTop + bandDepth * 0.5f;
                const float nearness = (bandMiddle - groundY) / (frame.height - groundY);
                const float parallax = parallaxAt(frame, bandMiddle);
                for (std::size_t kind = 0; kind != bladeColors.size(); ++kind)
                {
                    ScatteredShapes blades{ frame,
                        SolidColor{ faded(hazed(theme, bladeColors[kind], parallax),
                            0.35f + 0.35f * nearness) },
                        std::max(0.6f, 1.4f * nearness * frame.unit) };
                    for (int i = static_cast<int>(kind); i < k_meadowBlades; i += 3)
                    {
                        const float y = groundY + (frame.height - groundY)
                            * std::pow(pseudoRandom(i, 21.0f), 0.8f);
                        if (y < bandTop || y >= bandTop + bandDepth)
                            continue;
                        const float x = pseudoRandom(i, 22.0f) * frame.width;
                        const float depth = (y - groundY) / (frame.height - groundY);
                        const float length = (1.5f + 5.0f * depth) * frame.unit
                            * (0.6f + 0.6f * pseudoRandom(i, 23.0f));
                        PixelPath& blade = blades.next();
                        blade.moveTo(x, y);
                        blade.lineTo(x + length * 0.25f, y - length);
                    }
                    blades.draw();

                    ScatteredShapes specks{ frame, SolidColor{ faded(hazed(theme, petals[kind],
                        parallax), 0.35f + 0.5f * nearness) } };
                    for (int i = static_cast<int>(kind); i < k_meadowFlowers; i += 3)
                    {
                        const float y = groundY + 30.0f + (frame.height * 0.8f - groundY - 30.0f)
                            * std::pow(pseudoRandom(i, 31.0f), 0.6f);
                        if (y < bandTop || y >= bandTop + bandDepth)
                            continue;
                        const float x = pseudoRandom(i, 32.0f) * frame.width;
                        const float depth = (y - groundY) / (frame.height - groundY);
                        addEllipse(specks.next(), { x, y }, (0.4f + 2.4f * depth) * frame.unit,
                            (0.4f + 2.4f * depth) * frame.unit);
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
                hazed(theme, theme.chase.trail, k_knollParallax), theme.chase.trail));
        }

        void drawFir(const SceneFrame& frame, const SceneTheme& theme, const StandingTree& tree)
        {
            const FloatPoint foot = pointAt(frame, tree.footShare);
            const float parallax = parallaxAt(frame, foot.y);
            drawFirTree(frame, foot, k_firSize * tree.size * sizeAt(frame, foot.y), FirTreeColors{
                .trunk = hazed(theme, theme.chase.bark, parallax),
                .trunkShade = hazed(theme, theme.chase.barkShade, parallax),
                .leaves = hazed(theme, theme.treeLeavesBase, parallax),
                .shadow = theme.shadow
            });
        }

        // The Chase's far hill tree, standing nearer.
        void drawBroadleaf(const SceneFrame& frame, const SceneTheme& theme,
            const StandingTree& tree)
        {
            const FloatPoint foot = pointAt(frame, tree.footShare);
            const float parallax = parallaxAt(frame, foot.y);
            const float size = tree.size * sizeAt(frame, foot.y);
            drawGlow(frame, foot, 20.0f * size, faded(theme.shadow, 0.3f), 0.5f);
            const Color bark = hazed(theme, theme.chase.bark, parallax);
            const Color barkShade = hazed(theme, theme.chase.barkShade, parallax);
            drawTrunk(frame, foot, 3.0f * size, 22.0f * size, bark, barkShade);
            fillDisc(frame, { foot.x, foot.y - 32.0f * size }, 16.0f * size,
                SolidColor{ hazed(theme, theme.treeLeavesBase, parallax) });
            fillDisc(frame, { foot.x - 9.0f * size, foot.y - 25.0f * size }, 11.0f * size,
                SolidColor{ hazed(theme, theme.chase.crownLight, parallax) });
            fillDisc(frame, { foot.x + 9.0f * size, foot.y - 26.0f * size }, 12.0f * size,
                SolidColor{ hazed(theme, theme.chase.crownMiddle, parallax) });
        }

        // The water shows the sky's light near the horizon and its own deep colour close by.
        void drawRiverBed(const SceneFrame& frame, const SceneTheme& theme)
        {
            const float groundY = frame.height * k_horizonShare;
            const PixelPath river = riverOutline(frame);
            const Color bank = colorBetween(landAt(frame, theme, frame.height * 0.8f),
                Color{ 0xFF000000 }, 0.3f);
            strokeShape(frame, river, SolidColor{ faded(bank, 0.55f) }, 2.4f * frame.unit);
            const Color farWater = colorBetween(theme.riverTop, theme.skyBottom, 0.55f);
            fillShape(frame, river, LinearGradient{
                .startPoint = { 0.0f, groundY },
                .endPoint = { 0.0f, frame.height },
                .stops = {
                    { .position = 0.0f,
                        .color = hazed(theme, farWater, parallaxAt(frame, groundY)) },
                    { .position = 0.25f,
                        .color = colorBetween(theme.riverTop, theme.skyBottom, 0.3f) },
                    { .position = 0.55f, .color = theme.riverTop },
                    { .position = 1.0f,
                        .color = colorBetween(theme.riverBottom, theme.skyTop, 0.15f) }
                }
            });
        }

        // An arched footbridge of planks with a rail on posts.
        void drawBridge(const SceneFrame& frame, const SceneTheme& theme)
        {
            const FloatPoint foot = pointAt(frame, k_bridgeFootShare);
            const float size = k_bridgeSize * sizeAt(frame, foot.y);
            const Matrix3x2 place = Matrix3x2::Translation(foot) * Matrix3x2::Scale(size);
            const Color wood = theme.chase.bough;
            const Color woodShade = colorBetween(wood, Color{ 0xFF000000 }, 0.35f);
            const Color railLight = colorBetween(wood, theme.sunDisk, 0.12f);

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

        // Glints drifting with the current, and under the sun its glitter on the pond.
        void drawWaterLight(const SceneFrame& frame, const SceneTheme& theme, const float sunX,
            const float cycleTime)
        {
            constexpr int k_glints{ 70 };
            constexpr int k_glitters{ 46 };
            const float w = frame.width;
            const float h = frame.height;
            const float unit = frame.unit;
            const PixelPath river = riverOutline(frame);
            frame.canvas.pushClip(river, &frame.toCanvas);
            for (int i = 0; i != k_glints; ++i)
            {
                const float y = h * (0.70f + 0.30f * std::pow(pseudoRandom(i, 41.0f), 0.7f));
                const float nearness = nearnessAt(frame, y);
                // Each drifts a whole number of times across the frame in a cycle.
                const float crossings = std::round(1.0f + 3.0f * nearness);
                const float x = fractionalPart(pseudoRandom(i, 42.0f) + cycleTime * crossings)
                    * (w + 200.0f) - 100.0f;
                const float life = 0.5f + 0.5f * std::sin(cycleTime * k_2Pi * std::round(
                    5.0f + 5.0f * pseudoRandom(i, 43.0f)) + static_cast<float>(i));
                const float reach = (5.0f + 16.0f * nearness) * unit
                    * (0.6f + 0.6f * pseudoRandom(i, 44.0f));
                PixelPath glint;
                glint.moveTo(x - reach, y);
                glint.quadTo({ x, y - 2.5f * nearness * unit }, { x + reach, y });
                strokeShape(frame, glint, SolidColor{ faded(theme.riverWave,
                    life * (0.35f + 0.5f * nearness)) }, std::max(0.8f, 1.6f * nearness * unit));
            }
            for (int i = 0; i != k_glitters; ++i)
            {
                const float rowShare = pseudoRandom(i, 51.0f);
                const float y = h * (0.81f + 0.19f * rowShare);
                const float nearness = nearnessAt(frame, y);
                const float spread = (14.0f + 70.0f * rowShare) * unit;
                const float x = sunX + (pseudoRandom(i, 52.0f) - 0.5f) * 2.0f * spread;
                const float twinkle = std::max(0.0f, std::sin(cycleTime * k_2Pi * std::round(
                    14.0f + 12.0f * pseudoRandom(i, 53.0f)) + static_cast<float>(i) * 2.1f));
                const float reach = (4.0f + 10.0f * nearness) * unit;
                const float thickness = 1.6f * unit * (0.5f + nearness);
                fillBox(frame, { x - reach, y - thickness * 0.5f, x + reach, y + thickness * 0.5f },
                    SolidColor{ faded(theme.riverReflect, std::min(1.0f, twinkle * 1.4f)) });
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
            const Color land = nearBankColor(theme.hillBottom);
            const Color wetEdge = faded(colorBetween(land, Color{ 0xFF000000 }, 0.5f), 0.8f);
            strokeShape(frame, bank, SolidColor{ wetEdge }, 3.0f * unit);
            fillShape(frame, bank, LinearGradient::Simple({ 0.0f, h * 0.86f }, { 0.0f, h },
                land, colorBetween(land, Color{ 0xFF000000 }, 0.18f)));

            const float swayPhase = cycleTime * k_2Pi * 8.0f;
            for (const BladeRun& run : k_bladeRuns)
            {
                ScatteredShapes lightBlades{ frame, SolidColor{ theme.chase.grass } };
                ScatteredShapes darkBlades{ frame, SolidColor{ theme.chase.frontGrassDark } };
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

            const Color stem = theme.reedStem;
            const Color stemShade = colorBetween(stem, Color{ 0xFF000000 }, 0.25f);
            const Color head = theme.reedHead;
            const Color headLight = faded(colorBetween(head, theme.sunDisk, 0.25f), 0.8f);
            for (const Cattail& cattail : k_cattails)
            {
                const float x = w * cattail.xShare;
                const float y = nearBankTopAt(frame, x) + 6.0f * unit;
                const float height = cattail.height * unit;
                const float swayAngle = cycleTime * k_2Pi * 7.0f + cattail.swayPhase;
                const float sway = std::sin(swayAngle) * 6.0f * unit;
                // a long leaf leaning each way, the left one in shade
                for (const float side : { -1.0f, 1.0f })
                {
                    const float reach = (side < 0.0f ? 26.0f : 18.0f) * unit;
                    const float leafHeight = height * (side < 0.0f ? 0.78f : 0.62f);
                    PixelPath leaf;
                    leaf.moveTo(x - 2.6f * unit, y);
                    leaf.quadTo({ x + side * 6.0f * unit + sway * 0.2f, y - leafHeight * 0.6f },
                        { x + side * reach + sway * 0.7f, y - leafHeight });
                    leaf.quadTo({ x + side * 3.0f * unit, y - leafHeight * 0.5f },
                        { x + 2.6f * unit, y });
                    leaf.close();
                    fillShape(frame, leaf, SolidColor{ side < 0.0f ? stemShade : stem });
                }
                PixelPath stalk;
                stalk.moveTo(x, y);
                stalk.quadTo({ x + sway * 0.2f, y - height * 0.6f }, { x + sway, y - height });
                strokeShape(frame, stalk, SolidColor{ stem }, 3.0f * unit);
                const FloatPoint headMiddle = { x + sway * 0.93f, y - height * 0.86f };
                const Matrix3x2 headPlace = Matrix3x2::Translation(headMiddle)
                    * Matrix3x2::Rotation(sway / height);
                PixelPath headShape;
                addEllipse(headShape, {}, 4.6f * unit, 13.0f * unit);
                fillShape(frame, headShape, SolidColor{ head }, headPlace);
                PixelPath headShine;
                addEllipse(headShine, { 1.6f * unit, -2.0f * unit }, 1.6f * unit, 9.0f * unit);
                fillShape(frame, headShine, SolidColor{ headLight }, headPlace);
            }

            const std::array<Color, 3> petals{ theme.petal1, theme.petal2, theme.petal3 };
            for (std::size_t i = 0; i != k_bankFlowers.size(); ++i)
            {
                const BankFlower& flower = k_bankFlowers[i];
                const float x = w * flower.xShare;
                const float y = nearBankTopAt(frame, x) + flower.depth * unit;
                const float sway = std::sin(swayPhase + static_cast<float>(i)) * 2.0f * unit;
                drawSegment(frame, { x, y + 18.0f * unit }, { x + sway, y },
                    SolidColor{ theme.chase.grass }, 1.6f * unit);
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
                fillDisc(frame, { x + sway, y }, 2.4f * unit,
                    SolidColor{ theme.chase.flowerHeart });
            }
        }
    }
}
