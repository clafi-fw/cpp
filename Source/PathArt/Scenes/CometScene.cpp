module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.CloudMap;
import ClaFi.PathArt.CometSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.ShootingStarsEffect;
import ClaFi.PathArt.StarsEffect;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WaterReflection;
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

    namespace CometSceneColors
    {
        namespace
        {
            struct TwoColors
            {
                Color top;
                Color bottom;
            };
        }

        constexpr TwoColors Sky{
            0xFF050716,
            0xFF1A1842
        };

        constexpr Color Flash{ 0xFFFF9A66 };
        constexpr Color Star{ 0xFFEEF0FF };
        constexpr Color ShootingStar{ 0xFFFFFFFF };
        // The comet's cold light on the mountains it passes over.
        constexpr Color CometLight{ 0xFFCFE2FF };
        
        // The fog lying in the valley: silver-blue.
        constexpr Color Fog{ 0xFFD6DFF2 };

        constexpr TwoColors FarRidge{
            0xFF292455,
            0xFF161228
        };
        constexpr TwoColors NearRidge{
            0xFF1C2439,
            0xFF090714
        };
        constexpr Color Shore = 0xFF020705;

        constexpr Color Afterglow{ 0xFFFFB070 };
        constexpr Color ImpactCore{ 0xFFFFF1D0 };
        constexpr Color RidgeRim = { 0xFFFFD8A8 };

        constexpr Color LakeTop = Sky.bottom;// { 0xFF1E2B47 };
        constexpr Color LakeBottom{ 0xFF04070F };
        constexpr Color LakeShadeTop{ 0x26060C1B };
        constexpr Color LakeShadeBottom{ 0xBF03060F };
        
        constexpr Color Ripple{ 0x26C6D4F4 };
        constexpr Color Glitter{ 0xFFE9EFFF };
        
        
        // The near shore's pines and boulders, the darkest thing in the frame.
        constexpr Color NearRock{ 0xFF060911 };
        constexpr Color NearPine{ 0xFF030B0A };

        // The rounded boulder's form, borrowed from the Chase: its lit crown, its shine, its crack.
        constexpr Color BoulderLight{ 0xFF101827 };
        constexpr Color BoulderShine{ 0xFF1B2639 };
        constexpr Color BoulderCrack{ 0xFF02040A };
    }

    namespace
    {
        constexpr std::array<std::wstring_view, 1> k_switches{ L"Draw foreground" };
        constexpr std::size_t k_foregroundSwitch{ 0 };
        constexpr std::wstring_view k_foregroundShownNode{ L"ForegroundShown" };

        constexpr float k_passesPerCycle{ 2.0f };
        
        // The comet fall duration in cycles - strictly < 1.0f
        constexpr float k_fallDuration{ 0.6f };
        constexpr float k_ridgeLightFadeInDuration{ 0.06f };

        // Where the lake begins, as a share of the frame's height.
        constexpr float k_lakeShare{ 0.72f };
        constexpr float k_reflectionOpacity{ 0.666f };
        // The rounded stone mirrors in the water at full share, in its own colours.
        constexpr float k_stoneReflectionOpacity{ 1.0f };
        // How far the ripples move the lake's top row and its nearest, in units of the frame.
        constexpr float k_farRippleReach{ 0.4f };
        constexpr float k_nearRippleReach{ 4.0f };
        
        // The glitter path: the Mill's sun path, with the comet's light moving over the sparkles.
        constexpr int k_glitterFieldCount{ 3000 };

        constexpr int k_pathGlowCount{ 7 };
        constexpr float k_glitterOpacity{ 0.5f };
        // The rounded stone's place: centred a little right, its base in the water, squashed.
        constexpr FloatPoint k_stoneAt{ 0.545f, 0.930f };
        constexpr float k_stoneWidth{ 0.0024f };
        constexpr float k_stoneSquish{ 0.68f };

        // The scene's air, stated the way a theme states it: its own sky's ends and the share a
        // layer at the reference distance takes.
        constexpr SceneTheme k_air{
            .skyTop = CometSceneColors::Sky.top,
            .skyBottom = CometSceneColors::Sky.bottom,
            .airHaze = 0.06f
        };
        
        // The ranges' distances: the far one as far as the Chase's far mountains, the near one at
        // the reference distance, where a layer takes the air's share exactly.
        constexpr float k_farRangeParallax = 0.02f;
        constexpr float k_nearRangeParallax = SceneTheme::HazeReferenceParallax;
        constexpr float k_shoreParallax = k_nearRangeParallax;

        // Where a ridge's haze gradient starts, above every peak, and the steps it is drawn in.
        constexpr float k_ridgeTopShare{ 0.35f };
        constexpr int k_ridgeHazeSteps{ 12 };

        // Where a share sits in its band, nudged so neighbours neither clump nor stand in a row.
        float spreadShare(const int index, const int count, const float seed)
        {
            constexpr float k_jitter{ 0.8f };
            const float nudge = k_jitter * (pseudoRandom(index, seed) - 0.5f);
            return (static_cast<float>(index) + 0.5f + nudge) / static_cast<float>(count);
        }

        // A range's paint, hazed as the standard air hazes a layer at its distance: both ranges
        // start from the same clear colours, thinner with height above the lake.
        LinearGradient hazedRidge(const SceneFrame& frame, const float parallax, CometSceneColors::TwoColors colors)
        {
            const float lakeY = frame.height * k_lakeShare;
            const float top = frame.height * k_ridgeTopShare;
            LinearGradient rock{
                .startPoint = { 0.0f, top },
                .endPoint = { 0.0f, lakeY },
                .stops = {}
            };
            rock.stops.reserve(k_ridgeHazeSteps + 1);
            for (int step = 0; step <= k_ridgeHazeSteps; ++step)
            {
                const float position = static_cast<float>(step)
                    / static_cast<float>(k_ridgeHazeSteps);
                const float aboveGround = (1.0f - position) * (lakeY - top) / frame.height;
                const Color clear = Color::between(colors.top, colors.bottom, position);
                rock.stops.push_back({
                    .position = position,
                    .color = k_air.hazed(clear, parallax, aboveGround)
                });
            }
            return rock;
        }

        // The fog's strips of the cloud map: a grid of the map's cover over each band, which the
        // band's pixels read between. See PathArt#cloudkind
        constexpr int k_fogColumns{ 160 };
        constexpr int k_fogRows{ 40 };
        constexpr float k_fogLatitudeSpan{ 0.55f }; // radians down a band
        // The clouds the fog is cut from: the Earth's heaps, seeded once.
        constexpr CloudKind k_fogKind{ CloudKind::Puffy };
        constexpr std::uint32_t k_fogSeed{ 47u };
        constexpr float k_fogCover{ 0.55f };

        // One row of a fog band's bitmap, worked out once for the row: its fog colour, the light's
        // channels, its share of the band's vertical profile, and the grid rows it reads between.
        struct FogRow
        {
            float red;
            float green;
            float blue;
            float lightRed;
            float lightGreen;
            float lightBlue;
            float profile;
            int above;
            int below;
            float share;
        };

        // The frame mirrored about the horizontal line at `groundY`, so what is drawn in it sits
        // below that line as its reflection - how the Mill reflects its ducks, reeds and cow.
        SceneFrame mirroredAbout(const SceneFrame& frame, const float groundY)
        {
            return frame.within(Matrix3x2::Translation(0.0f, 2.0f * groundY)
                * Matrix3x2::Scale(1.0f, -1.0f));
        }

        // What one glitter of the lake's field is, before a frame asks anything of it: how far down
        // the path it stands, how far across the water, the share of the water's own light it takes,
        // and the rate and phase it twinkles at. Worked out once; the light moves over it after.
        struct Glitter
        {
            float along;
            float across;
            float waterShare;
            float twinkleRate;
            float twinklePhase;
        };

        // The field's glitters, in their order: each keeps the place the water gave it, so the only
        // thing a frame decides is how bright the light over it is.
        const std::array<Glitter, k_glitterFieldCount>& glitterField()
        {
            static const std::array<Glitter, k_glitterFieldCount> s_field = [] {
                std::array<Glitter, k_glitterFieldCount> field{};
                for (int i = 0; i != k_glitterFieldCount; ++i)
                {
                    field[static_cast<std::size_t>(i)] = Glitter{
                        .along = std::pow(spreadShare(i, k_glitterFieldCount, 51.0f), 0.7f),
                        .across = fractionalPart(static_cast<float>(i) * 0.618034f + 0.08f * pseudoRandom(i, 52.0f)),
                        .waterShare = pseudoRandom(i, 55.0f) < 0.02f ? 0.35f : 0.0f,
                        .twinkleRate = std::round(14.0f + 12.0f * pseudoRandom(i, 53.0f)),
                        .twinklePhase = static_cast<float>(i) * 2.1f
                    };
                }
                return field;
            }();
            return s_field;
        }

        // A pine after the concept's: a leaning trunk carrying tiers of fronds that sweep up and
        // out, their needles leaning along each branch toward its tip.
        void drawPine(const SceneFrame& frame, const FloatPoint base, const float height,
            const int seed)
        {
            const SolidColor ink{ CometSceneColors::NearPine };
            const float lean = 0.05f * (pseudoRandom(seed, 81.0f) - 0.5f);
            const float spineThickness = std::max(1.0f, height * 0.010f);
            const float needleThickness = std::max(0.7f, height * 0.005f);
            // The distance between neighbour needles, as a share of the tree's height.
            constexpr float k_needleSpacing{ 0.010f };
            const FloatPoint top{ base.x + lean * height, base.y - height * 0.97f };
            PixelPath trunk;
            trunk.moveTo(base);
            trunk.lineTo(top);
            strokeShape(frame, trunk, ink, std::max(1.8f, height * 0.026f), StrokeCap::Round);
            int index = 0;
            // One frond: a spine that lifts off the trunk then sags, its needles leaning along it
            // toward the tip. The needles sit one every `k_needleSpacing` of the tree's height,
            // measured along the spine, so their distance stays even from the foot to the tip.
            const auto frond = [&](const FloatPoint from, const FloatPoint lift,
                const FloatPoint sag, const FloatPoint to, const float reach) {
                const auto spine = [&](const float t) {
                    const float u = 1.0f - t;
                    const float a = u * u * u;
                    const float b = 3.0f * u * u * t;
                    const float c = 3.0f * u * t * t;
                    const float d = t * t * t;
                    return FloatPoint{ a * from.x + b * lift.x + c * sag.x + d * to.x,
                        a * from.y + b * lift.y + c * sag.y + d * to.y };
                };
                // The spine, sampled finely so the needles can be laid by distance along it.
                constexpr int k_samples{ 16 };
                FloatPoint spinePoints[k_samples + 1];
                float spineLengths[k_samples + 1];
                spinePoints[0] = from;
                spineLengths[0] = 0.0f;
                for (int sample = 1; sample <= k_samples; ++sample)
                {
                    spinePoints[sample] = spine(static_cast<float>(sample)
                        / static_cast<float>(k_samples));
                    const FloatPoint step{ spinePoints[sample].x - spinePoints[sample - 1].x,
                        spinePoints[sample].y - spinePoints[sample - 1].y };
                    spineLengths[sample] = spineLengths[sample - 1]
                        + std::sqrt(step.x * step.x + step.y * step.y);
                }
                PixelPath spinePath;
                spinePath.moveTo(from);
                for (int sample = 1; sample <= k_samples; ++sample)
                    spinePath.lineTo(spinePoints[sample]);
                strokeShape(frame, spinePath, ink, spineThickness, StrokeCap::Round);
                // Needles, evenly spaced; only the broad fronds take the flake tufts.
                const float total = spineLengths[k_samples];
                const int needles = std::max(3, static_cast<int>(
                    std::lround(total / (height * k_needleSpacing))));
                for (int needle = 0; needle != needles; ++needle)
                {
                    const float target = (static_cast<float>(needle) + 0.5f)
                        / static_cast<float>(needles) * total;
                    int sample = 0;
                    while (sample < k_samples && spineLengths[sample + 1] < target)
                        ++sample;
                    const float span = std::max(1.0e-4f,
                        spineLengths[sample + 1] - spineLengths[sample]);
                    const float share = (target - spineLengths[sample]) / span;
                    const FloatPoint at{ spinePoints[sample].x
                        + (spinePoints[sample + 1].x - spinePoints[sample].x) * share,
                        spinePoints[sample].y
                        + (spinePoints[sample + 1].y - spinePoints[sample].y) * share };
                    const FloatPoint along{
                        (spinePoints[sample + 1].x - spinePoints[sample].x) / span,
                        (spinePoints[sample + 1].y - spinePoints[sample].y) / span };
                    const float length = reach * (0.40f + 0.45f * (target / total))
                        * (0.8f + 0.4f * pseudoRandom(index++, seed + 13.0f));
                    for (int side = 0; side != 2; ++side)
                    {
                        const float slant = (0.50f + 0.40f
                            * pseudoRandom(index++, seed + 14.0f))
                            * (side == 0 ? 1.0f : -1.0f);
                        const float cosine = std::cos(slant);
                        const float sine = std::sin(slant);
                        PixelPath barb;
                        barb.moveTo(at);
                        barb.lineTo(at.x + (along.x * cosine - along.y * sine) * length,
                            at.y + (along.x * sine + along.y * cosine) * length);
                        strokeShape(frame, barb, ink, needleThickness, StrokeCap::Round);
                    }
                }
            };
            const int tiers = 16;
            for (int tier = 0; tier != tiers; ++tier)
            {
                const float along = (static_cast<float>(tier) + 0.55f)
                    / static_cast<float>(tiers);
                const FloatPoint stem{ base.x + lean * height * along,
                    base.y - height * 0.97f * along };
                for (int side = 0; side != 2; ++side)
                {
                    const float direction = side == 0 ? -1.0f : 1.0f;
                    // The pair at the very top is kept short, so the crown comes to a point there
                    // instead of ending in two long arms.
                    const float topTaper = tier + 2 >= tiers
                        ? (tier + 1 >= tiers ? 0.70f : 0.85f)
                        : 1.0f;
                    const float length = height * (0.23f - 0.16f * along) * topTaper
                        * (0.75f + 0.5f * pseudoRandom(index++, seed + 11.0f));
                    const float rise = 0.12f + 0.62f * pseudoRandom(index++, seed + 12.0f);
                    const FloatPoint tip{ stem.x + direction * length, stem.y - length * rise };
                    const FloatPoint lift{ stem.x + direction * length * 0.42f,
                        stem.y - length * 0.55f };
                    const FloatPoint sag{ stem.x + direction * length * 0.85f,
                        stem.y - length * 0.22f };
                    // The low, long branches are full; the short top ones thin out.
                    const float fluff = std::clamp(std::pow(length / (0.23f * height), 1.4f),
                        0.15f, 1.0f);
                    frond(stem, lift, sag, tip, length * 0.45f);
                    // A fork off the sag, smaller and steeper, where the branch is broad.
                    if (fluff > 0.35f)
                    {
                        const FloatPoint forkTip{ sag.x + direction * length * 0.35f,
                            sag.y - length * 0.40f };
                        frond(sag, { sag.x + direction * length * 0.20f,
                            sag.y - length * 0.30f },
                            { sag.x + direction * length * 0.32f, sag.y - length * 0.10f },
                            forkTip, length * 0.14f);
                    }
                }
            }
            // The crown's own fronds: one leader pointing at the sky, longer than the two flanking
            // stubs, so the top comes to a point.
            frond(top, { top.x, top.y - height * 0.027f }, { top.x + lean * height * 0.027f,
                top.y - height * 0.049f }, { top.x + lean * height * 0.054f,
                top.y - height * 0.075f }, height * 0.020f);
            frond({ top.x, top.y + height * 0.01f }, { top.x - height * 0.015f,
                top.y - height * 0.005f }, { top.x - height * 0.025f, top.y - height * 0.015f },
                { top.x - height * 0.04f, top.y - height * 0.03f }, height * 0.018f);
            frond({ top.x, top.y + height * 0.01f }, { top.x + height * 0.015f,
                top.y - height * 0.005f }, { top.x + height * 0.025f, top.y - height * 0.015f },
                { top.x + height * 0.04f, top.y - height * 0.03f }, height * 0.018f);
        }

        // A rock: a dark body with sharp points breaking out of it, the far pines' triangles in
        // stone.
        void drawRock(const SceneFrame& frame, const FloatPoint base, const float width,
            const float height, const int seed)
        {
            const SolidColor ink{ CometSceneColors::NearRock };
            const float left = base.x - width * 0.5f;
            const float right = base.x + width * 0.5f;
            const float peakAt = 0.24f + 0.30f * pseudoRandom(seed, 62.0f);
            const float peakHigh = height * (0.82f + 0.18f * pseudoRandom(seed, 63.0f));
            const float secondAt = peakAt + 0.20f + 0.18f * pseudoRandom(seed, 64.0f);
            const float secondHigh = peakHigh * (0.45f + 0.30f * pseudoRandom(seed, 65.0f));
            const float shoulder = height * (0.34f + 0.20f * pseudoRandom(seed, 66.0f));
            PixelPath rock;
            rock.moveTo(left, base.y);
            rock.lineTo(left + width * 0.06f, base.y - shoulder * 0.82f);
            rock.lineTo(left + width * (peakAt - 0.10f), base.y - peakHigh * 0.62f);
            rock.lineTo(left + width * peakAt, base.y - peakHigh);
            rock.lineTo(left + width * (peakAt + 0.10f), base.y - peakHigh * 0.55f);
            rock.lineTo(left + width * (secondAt - 0.07f), base.y - secondHigh * 0.55f);
            rock.lineTo(left + width * secondAt, base.y - secondHigh);
            rock.lineTo(left + width * (secondAt + 0.07f), base.y - secondHigh * 0.50f);
            rock.lineTo(right - width * 0.08f, base.y - shoulder * 0.62f);
            rock.lineTo(right, base.y - shoulder * 0.16f);
            rock.lineTo(right + width * 0.05f, base.y);
            rock.close();
            fillShape(frame, rock, ink);
        }

        // The Chase's roadside boulder, its rounded stone, shine and crack, laid at `base` and
        // squashed by `squish` - a smoother, wider stone among the sharp ones. Its reflection is
        // drawn each frame by `drawStoneReflection`, so the lake's ripples run through it.
        void drawRoundedBoulder(const SceneFrame& frame, const FloatPoint base, const float scale,
            const float squish, const Color light = CometSceneColors::BoulderLight,
            const Color shade = CometSceneColors::NearRock,
            const Color shine = CometSceneColors::BoulderShine)
        {
            const SceneFrame stone = frame.within(
                Matrix3x2::Translation(base) * Matrix3x2::Scale(scale, scale * squish));
            PixelPath rock;
            rock.moveTo(-17.0f, 0.0f);
            rock.cubicTo({ -20.0f, -9.0f }, { -14.0f, -20.0f }, { -3.0f, -22.0f });
            rock.cubicTo({ 7.0f, -24.0f }, { 16.0f, -17.0f }, { 18.0f, -9.0f });
            rock.cubicTo({ 20.0f, -3.0f }, { 18.0f, 0.5f }, { 14.0f, 0.5f });
            rock.close();
            PixelPath crack;
            crack.moveTo(5.0f, -15.0f);
            crack.lineTo(8.0f, -10.0f);
            crack.lineTo(6.0f, -6.0f);
            crack.lineTo(8.5f, -2.5f);
            fillShape(stone, rock, LinearGradient::Simple({ -8.0f, -21.0f }, { 10.0f, 0.0f },
                light, shade));
            fillOval(stone, { -5.0f, -17.0f }, 6.5f, 2.6f, SolidColor{ shine }, -0.25f);
            strokeShape(stone, crack, SolidColor{ CometSceneColors::BoulderCrack }, 0.6f);
        }
    }

    void CometScene::prepare(const SceneParams& params)
    {
        m_farRidge.prepare(params);
        m_nearRidge.prepare(params);
        m_lakeWater.prepare(params);
        m_mirroredFarRidge.prepare(params);
        m_mirroredNearRidge.prepare(params);
        m_lakeShade.prepare(params);
        m_shore.prepare(params);
        m_mirroredShore.prepare(params);
        if (m_foregroundShown)
        {
            m_foreground.prepare(params);
        }
    }

    void CometScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        takeFogMap();
        drawSky(frame);
        drawAboveLake(frame, time, false);
        drawLake(frame, time);
        if (m_foregroundShown)
        {
            m_foreground.draw(frame);
        }
    }

    void CometScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        constexpr float k_figureSpan{ 121.0f };
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const FloatPoint center = { frame.width * 0.5f, frame.height * 0.5f };
        const auto at = [&frame, center](const float x, const float y) {
            return center + FloatPoint{ x, y } * frame.unit;
        };
        drawComet(frame, CometPose{
            .course = { .start = at(204.0f, -238.0f), .control = at(77.0f, -99.0f),
                .end = at(-50.0f, 51.0f) },
            .passTime = 0.9f,
            .fallShare = 1.0f,
            .cycleTime = 0.2f
        });
    }

    SceneReadoutColors CometScene::readoutColors() const
    {
        return {
            .field = CometSceneColors::FarRidge.bottom,
            .series = CometSceneColors::Ripple
        };
    }

    std::span<const std::wstring_view> CometScene::switches() const
    {
        return k_switches;
    }

    bool CometScene::isSwitchOn(const std::size_t index) const
    {
        if (index != k_foregroundSwitch)
        {
            noReach("CometScene offers no such switch");
        }
        return m_foregroundShown;
    }

    void CometScene::setSwitch(const std::size_t index, const bool on)
    {
        if (index != k_foregroundSwitch)
        {
            noReach("CometScene offers no such switch");
        }
        m_foregroundShown = on;
    }

    void CometScene::storeView(Dom::Section& view) const
    {
        (view / k_foregroundShownNode).set(m_foregroundShown);
    }

    void CometScene::restoreView(const Dom::Section& view)
    {
        m_foregroundShown = (view / k_foregroundShownNode).get<bool>();
    }

    Dom::Dt::Section CometScene::viewSchema()
    {
        return Dom::Dt::Section{
            k_name,
            Dom::Dt::Value{ k_foregroundShownNode, false }
        };
    }

    float CometScene::farRidgeY(const SceneFrame& frame, const float x)
    {
        const float share = x / frame.width;
        const float peaks = 0.12f * bump(share, 0.33f, 0.2f) + 0.07f * bump(share, 0.86f, 0.12f);
        const float ragged = 0.014f * std::sin(share * 43.0f) + 0.008f * std::sin(share * 97.0f);
        return frame.height * (0.54f - peaks - ragged);
    }

    float CometScene::nearRidgeY(const SceneFrame& frame, const float x)
    {
        const float share = x / frame.width;
        const float peaks = 0.09f * bump(share, 0.72f, 0.17f) + 0.05f * bump(share, 0.18f, 0.13f);
        const float ragged = 0.012f * std::sin(share * 61.0f) + 0.007f * std::sin(share * 131.0f);
        return frame.height * (0.66f - peaks - ragged);
    }

    // The first pass falls to the right, the second to the left, each landing on a peak.
    CometScene::Pass CometScene::passAt(const SceneFrame& frame, const float cycleTime)
    {
        const bool fromLeft = std::floor(cycleTime * k_passesPerCycle) == 0.0f;
        const float passTime = fractionalPart(cycleTime * k_passesPerCycle);
        const bool falling = passTime < k_fallDuration;
        const FloatPoint start = {
            frame.width * (fromLeft ? -0.04f : 1.04f),
            frame.height * (fromLeft ? 0.06f : 0.12f)
        };
        const float endX = frame.width * (fromLeft ? 0.73f : 0.2f);
        const FloatPoint end = { endX, nearRidgeY(frame, endX) + 28.0f * frame.unit };
        // The control sits a little above the start: the comet sets off across the sky, then falls.
        return {
            .course = {
                .start = start,
                .control = { std::lerp(start.x, end.x, 0.62f), start.y - frame.height * 0.03f },
                .end = end
            },
            .passTime = passTime,
            .afterglow = falling ? 0.0f : (passTime - k_fallDuration) / (1.0f - k_fallDuration)
        };
    }

    SceneFrame CometScene::reflectionOf(const SceneFrame& frame)
    {
        const float lakeY = frame.height * k_lakeShare;
        return frame.within(Matrix3x2::Translation(0.0f, lakeY * 2.0f)
            * Matrix3x2::Scale(1.0f, -1.0f));
    }

    // The fog's clouds are a CloudMap, built as the Earth's are: started off the paint on the
    // first frames, taken when it is ready, and kept. See PathArt#earthscene
    void CometScene::takeFogMap()
    {
        if (m_fogMap)
            return;
        if (!m_fogMapBuilt.valid())
        {
            m_fogMapBuilt = std::async(std::launch::async, []() {
                return std::make_unique<CloudMap>(k_fogKind, k_fogSeed, k_fogCover);
            });
        }
        if (m_fogMapBuilt.wait_for(std::chrono::seconds{ 0 }) == std::future_status::ready)
            m_fogMap = m_fogMapBuilt.get();
    }

    // **The fog is a band of the Earth's clouds laid over the ranges**: a strip of a cloud map,
    // read as a texture whose cover is the fog's opacity, so the clouds' own bills and gaps show
    // over the ridges. Nothing is lifted out as a shape.
    void CometScene::drawFogBand(const SceneFrame& frame, const SceneTime time,
        const Pass& pass, const FogBand& band, Graphics::Bitmap& pixels) const
    {
        if (!m_fogMap)
            return;
        const float unit = frame.unit;
        const float width = frame.width;
        const float top = frame.height * band.topShare;
        const float bottom = frame.height * band.bottomShare;
        // Where the band stands on the canvas, and how many pixels it takes there.
        const FloatPoint at = frame.toCanvas.transform({ 0.0f, top });
        const FloatPoint far = frame.toCanvas.transform({ width, bottom });
        const IntSize size{ std::max(1, static_cast<int>(std::lround(far.x - at.x))),
            std::max(1, static_cast<int>(std::lround(far.y - at.y))) };
        if (pixels.width() != size.x || pixels.height() != size.y)
        {
            pixels.resize(size);
            std::fill_n(pixels.data(), static_cast<std::size_t>(size.x)
                * static_cast<std::size_t>(size.y), Color{});
        }
        // Each band turns the map once in its own number of cycles, a whole turn, so its loop meets
        // itself and the fog never jumps; a longer period is a slower band.
        const float turn = fractionalPart((static_cast<float>(time.cyclesPassed) + time.cycleTime)
            / band.period);
        const float drift = k_2Pi * band.laps * turn;
        // The map's cover on a grid over the band, which its pixels read between.
        const std::size_t gridNum = static_cast<std::size_t>(k_fogColumns)
            * static_cast<std::size_t>(k_fogRows);
        std::vector<float> longitudes(gridNum);
        std::vector<float> latitudes(gridNum);
        std::vector<float> covers(gridNum);
        std::vector<float> longitudeSpans(gridNum,
            band.span / static_cast<float>(k_fogColumns));
        std::vector<float> latitudeSpans(gridNum,
            k_fogLatitudeSpan / static_cast<float>(k_fogRows));
        for (int column = 0; column != k_fogColumns; ++column)
        {
            const float longitude = band.longitude + drift
                + (static_cast<float>(column) + 0.5f) / static_cast<float>(k_fogColumns)
                * band.span;
            for (int row = 0; row != k_fogRows; ++row)
            {
                const std::size_t point = static_cast<std::size_t>(column)
                    * static_cast<std::size_t>(k_fogRows) + static_cast<std::size_t>(row);
                longitudes[point] = longitude;
                latitudes[point] = band.latitude - (static_cast<float>(row) + 0.5f)
                    / static_cast<float>(k_fogRows) * k_fogLatitudeSpan;
            }
        }
        m_fogMap->readCover({
            .longitudes = longitudes, .latitudes = latitudes,
            .longitudeSpans = longitudeSpans, .latitudeSpans = latitudeSpans
        }, covers);
        // The comet's light on the band: cold while it falls, warm as it lands.
        const float lightAlong = pass.afterglow > 0.0f
            ? 1.0f
            : std::pow(pass.passTime / k_fallDuration, 1.35f);
        const FloatPoint lightAt = pointOnCourse(pass.course, lightAlong);
        const float lightFade = pass.afterglow > 0.0f
            ? std::pow(1.0f - pass.afterglow, 1.4f)
            : 1.0f - smoothStep(0.52f, k_fallDuration, pass.passTime);
        const Color light = (pass.afterglow > 0.0f ? CometSceneColors::Afterglow
            : CometSceneColors::CometLight).fullyOpaque();
        const float lightReach = 600.0f * unit;
        const std::size_t acrossPixels = static_cast<std::size_t>(size.x);
        const std::size_t downPixels = static_cast<std::size_t>(size.y);
        // Each column's grid cells and its share of the comet's light, once for the band.
        std::vector<int> cellLeft(acrossPixels);
        std::vector<int> cellRight(acrossPixels);
        std::vector<float> cellShare(acrossPixels);
        std::vector<float> lightShares(acrossPixels);
        for (int column = 0; column != size.x; ++column)
        {
            const std::size_t index = static_cast<std::size_t>(column);
            const float across = (static_cast<float>(column) + 0.5f)
                / static_cast<float>(size.x) * static_cast<float>(k_fogColumns) - 0.5f;
            const int left = std::clamp(static_cast<int>(std::floor(across)), 0,
                k_fogColumns - 1);
            cellLeft[index] = left * k_fogRows;
            cellRight[index] = std::min(left + 1, k_fogColumns - 1) * k_fogRows;
            cellShare[index] = std::clamp(across - static_cast<float>(left), 0.0f, 1.0f);
            const float sceneX = (static_cast<float>(column) + 0.5f)
                / static_cast<float>(size.x) * width;
            lightShares[index] = 0.6f * lightFade * (1.0f - smoothStep(0.0f, lightReach,
                std::abs(sceneX - lightAt.x)));
        }
        // Each row's fog colour, the light's channels, its profile and grid rows, once for the band.
        std::vector<FogRow> bandRows(downPixels);
        for (int row = 0; row != size.y; ++row)
        {
            const float downShare = (static_cast<float>(row) + 0.5f) / static_cast<float>(size.y);
            const float gridRow = downShare * static_cast<float>(k_fogRows) - 0.5f;
            const int above = std::clamp(static_cast<int>(std::floor(gridRow)), 0, k_fogRows - 1);
            const Color shade = CometSceneColors::Fog;
            bandRows[static_cast<std::size_t>(row)] = {
                .red = static_cast<float>(shade.red),
                .green = static_cast<float>(shade.green),
                .blue = static_cast<float>(shade.blue),
                .lightRed = static_cast<float>(light.red),
                .lightGreen = static_cast<float>(light.green),
                .lightBlue = static_cast<float>(light.blue),
                .profile = band.strength * smoothStep(0.0f, 0.42f, downShare)
                    * (1.0f - smoothStep(0.88f, 1.0f, downShare)),
                .above = above,
                .below = std::min(above + 1, k_fogRows - 1),
                .share = std::clamp(gridRow - static_cast<float>(above), 0.0f, 1.0f)
            };
        }
        // The fog itself: the cover is its opacity, softer at the band's edges, the comet's light
        // tinting the columns it stands over. The rows are spread over the cores, two at a time -
        // the fog is too soft for the difference between neighbours to show.
        const float* grid = covers.data();
        spreadOverCores((size.y + 1) / 2, [&](const int pair) {
            const int row = pair * 2;
            const FogRow& fog = bandRows[static_cast<std::size_t>(row)];
            Color* upper = pixels.data() + static_cast<std::ptrdiff_t>(row) * pixels.stride();
            Color* lower = row + 1 < size.y ? upper + pixels.stride() : nullptr;
            for (int column = 0; column != size.x; ++column)
            {
                const std::size_t index = static_cast<std::size_t>(column);
                const float* left = grid + cellLeft[index];
                const float* right = grid + cellRight[index];
                const float across = cellShare[index];
                const float top = left[fog.above] + (right[fog.above] - left[fog.above]) * across;
                const float bottom = left[fog.below] + (right[fog.below] - left[fog.below])
                    * across;
                const float alpha = (top + (bottom - top) * fog.share) * fog.profile;
                const float lit = lightShares[index];
                const auto channel = [&](const float own, const float fromLight) {
                    return static_cast<ColorByte>((own + (fromLight - own) * lit) * alpha);
                };
                const Color color{ channel(fog.red, fog.lightRed),
                    channel(fog.green, fog.lightGreen), channel(fog.blue, fog.lightBlue),
                    static_cast<ColorByte>(alpha * 255.0f) };
                upper[column] = color;
                if (lower)
                    lower[column] = color;
            }
        });
        // Lay it over the canvas where the band stands, outside the frame's transform, its rows
        // striped over the cores. See Graphics#pixelview
        const Matrix3x2 canvasTransform = frame.canvas.transform();
        frame.canvas.resetTransform();
        const PixelView bandView = pixels.pixelView(at);
        frame.canvas.drawPixelViews(std::span<const PixelView>{ &bandView, 1 });
        frame.canvas.setTransform(canvasTransform);
    }

    // The ridges are kept as layers drawn through the upright frame, the reflection's included:
    // a layer fits a frame moved by whole pixels from the one it was kept for, not one turned over.
    void CometScene::drawAboveLake(const SceneFrame& upright, const SceneTime time,
        const bool reflection) const
    {
        const float cycleTime = time.cycleTime;
        const SceneFrame frame = reflection ? reflectionOf(upright) : upright;
        const float unit = frame.unit;
        const float width = frame.width;
        const float lakeY = frame.height * k_lakeShare;
        const Pass pass = passAt(frame, cycleTime);

        if (pass.afterglow > 0.0f)
        {
            const float flash = 0.16f * std::pow(1.0f - pass.afterglow, 5.0f);
            fillBox(frame, { 0.0f, 0.0f, width, lakeY },
                SolidColor{ CometSceneColors::Flash.withOpacity(flash) });
        }
        drawStars(frame, cycleTime, StarField{
            .count = 140,
            .top = 0.0f,
            .bottom = frame.height * 0.6f,
            .seed = 70.0f,
            .color = CometSceneColors::Star,
            .opacity = reflection ? 0.6f : 1.0f
        });
        // Three a cycle, each half a second, in the quiet between passes.
        drawShootingStars(frame, cycleTime, ShootingStars{
            .top = frame.height * 0.06f,
            .bottom = frame.height * 0.28f,
            .color = CometSceneColors::ShootingStar
        });
        (reflection ? m_mirroredFarRidge : m_farRidge).draw(upright);
        // The farthest fog lies over the far range, the comet's light falling on it after.
        if (!reflection)
            drawFogBand(frame, time, pass, k_farFog, m_farFogPixels);

        // The comet lights the mountains beneath it: a cold pool on the far range's face with a lit
        // crest above it, travelling with the head and giving way to the landing's warm glow.
        const float lightAlong = pass.afterglow > 0.0f
            ? 1.0f
            : std::pow(pass.passTime / k_fallDuration, 1.35f);

        const FloatPoint cometLight = pointOnCourse(pass.course, lightAlong);
        const float lightFade = smoothStep(0.0f, k_ridgeLightFadeInDuration, pass.passTime)
            * (1.0f - smoothStep(0.52f, k_fallDuration, pass.passTime));
        if (lightFade > 0.0f)
        {
            PixelPath farRidge;
            farRidge.moveTo(0.0f, lakeY);
            for (float x = 0.0f; x <= width + 8.0f * unit; x += 8.0f * unit)
                farRidge.lineTo(x, farRidgeY(frame, x));
            farRidge.lineTo(width + 8.0f * unit, lakeY);
            farRidge.close();

            frame.canvas.pushClip(farRidge, &frame.toCanvas);
            drawGlow(frame, { cometLight.x, cometLight.y },
                166.0f * unit, CometSceneColors::CometLight.withOpacity(0.13f * lightFade), 0.5f);
            frame.canvas.popClip();

            PixelPath rim;
            for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
            {
                if (x == 0.0f)
                    rim.moveTo(x, farRidgeY(frame, x));
                else
                    rim.lineTo(x, farRidgeY(frame, x));
            }
            const Color lit = CometSceneColors::CometLight.withOpacity(0.40f * lightFade);
            const Color dark = CometSceneColors::CometLight.withOpacity(0.0f);
            strokeShape(frame, rim, LinearGradient{
                .startPoint = { cometLight.x - 230.0f * unit, 0.0f },
                .endPoint = { cometLight.x + 230.0f * unit, 0.0f },
                .stops = {
                    { .position = 0.0f, .color = dark },
                    { .position = 0.5f, .color = lit },
                    { .position = 1.0f, .color = dark }
                }
            }, 1.6f * unit);
        }

        // The landing lights the sky behind the near ridge, which hides the comet as it lands.
        if (pass.afterglow > 0.0f)
        {
            const float fading = 1.0f - pass.afterglow;
            // The wash is gone before the pass ends, so the boundary has nothing left to switch off.
            const float endFade = 1.0f - smoothStep(0.86f, 0.97f, pass.passTime);
            drawGlow(frame, pass.course.end, (70.0f + 240.0f * std::sqrt(pass.afterglow)) * unit,
                CometSceneColors::Afterglow.withOpacity(0.8f * std::pow(fading, 1.6f) * endFade),
                0.6f);
            drawGlow(frame, pass.course.end, 50.0f * unit,
                CometSceneColors::ImpactCore.withOpacity(std::pow(fading, 4.0f) * endFade));
        }
        drawComet(frame, CometPose{
            .course = pass.course,
            .passTime = pass.passTime,
            .fallShare = k_fallDuration,
            .cycleTime = cycleTime
        });

        // The fog lies in the valley, behind the near range, whose crest stands in front of it.
        if (!reflection)
            drawFogBand(frame, time, pass, k_valleyFog, m_valleyFogPixels);
        
        (reflection ? m_mirroredNearRidge : m_nearRidge).draw(upright);
        if (pass.afterglow > 0.0f)
        {
            PixelPath rim;
            for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
            {
                if (x == 0.0f)
                    rim.moveTo(x, nearRidgeY(frame, x));
                else
                    rim.lineTo(x, nearRidgeY(frame, x));
            }
            constexpr float rimStrength = 1.0f;
            const float rimOpacity = rimStrength * std::pow(1.0f - pass.afterglow, 1.5f);
            
            const Color rimLit = CometSceneColors::RidgeRim.withOpacity(rimOpacity);
            const Color rimDark = CometSceneColors::Afterglow.withOpacity(0.0f);

            strokeShape(frame, rim, LinearGradient{
                .startPoint = { pass.course.end.x - 220.0f * unit, 0.0f },
                .endPoint = { pass.course.end.x + 220.0f * unit, 0.0f },
                .stops = {
                    { .position = 0.0f, .color = rimDark },
                    { .position = 0.5f, .color = rimLit },
                    { .position = 1.0f, .color = rimDark }
                }
            }, 1.8f * unit);
        }
        else if (lightFade > 0.0f)
        {
            PixelPath rim;
            for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
            {
                if (x == 0.0f)
                    rim.moveTo(x, nearRidgeY(frame, x));
                else
                    rim.lineTo(x, nearRidgeY(frame, x));
            }
            const Color lit = CometSceneColors::CometLight.withOpacity(0.20f * lightFade);
            const Color dark = CometSceneColors::CometLight.withOpacity(0.0f);
            strokeShape(frame, rim, LinearGradient{
                .startPoint = { cometLight.x - 200.0f * unit, 0.0f },
                .endPoint = { cometLight.x + 200.0f * unit, 0.0f },
                .stops = {
                    { .position = 0.0f, .color = dark },
                    { .position = 0.5f, .color = lit },
                    { .position = 1.0f, .color = dark }
                }
            }, 1.4f * unit);
        }
    }

    // The lake shows everything above it upside down and dimmed, with ripples running across.
    // Nothing in the mirror is kept still: the stars twinkle in front of the far ridge.
    void CometScene::drawLake(const SceneFrame& frame, const SceneTime time)
    {
        const float cycleTime = time.cycleTime;
        const float width = frame.width;
        const float height = frame.height;
        const float lakeY = height * k_lakeShare;
        m_lakeWater.draw(frame);

        PixelPath lakeOutline;
        lakeOutline.moveTo(0.0f, lakeY);
        lakeOutline.lineTo(width, lakeY);
        lakeOutline.lineTo(width, height);
        lakeOutline.lineTo(0.0f, height);
        lakeOutline.close();
        const Ripples mirrorRipples = {
            .top = lakeY,
            .farReach = k_farRippleReach,
            .nearReach = k_nearRippleReach,
            .cycleTime = cycleTime,
            .opacity = k_reflectionOpacity
        };
        // The pines stand nearest, so their reflection is laid in last.
        m_reflection.draw(frame, lakeOutline, {}, [&](const SceneFrame& mirror) {
            drawAboveLake(mirror, time, true);
            m_mirroredShore.draw(mirror);
        }, mirrorRipples);
        // The rounded stone mirrors in the water, at its own share. The stone is flipped about the
        // line it meets the water at, so its image lands below it, in the water. The water it shows
        // in is the patch round its base its image can reach rather than the whole lake: the patch
        // is a little wider than the stone, so the rows' sideways shifts stay inside it, and its top
        // starts above the waterline so that row is covered whole. The rows still take the lake's
        // own Ripples, and a column outside the patch mirrors nothing.
        const FloatPoint stoneBase = { width * k_stoneAt.x, height * k_stoneAt.y };
        const float stoneReach = 22.0f * width * k_stoneWidth + k_nearRippleReach * frame.unit;
        PixelPath stoneWater;
        stoneWater.moveTo(stoneBase.x - stoneReach, stoneBase.y - 2.0f);
        stoneWater.lineTo(stoneBase.x + stoneReach, stoneBase.y - 2.0f);
        stoneWater.lineTo(stoneBase.x + stoneReach, height);
        stoneWater.lineTo(stoneBase.x - stoneReach, height);
        stoneWater.close();
        const Ripples stoneRipples = {
            .top = lakeY,
            .farReach = k_farRippleReach,
            .nearReach = k_nearRippleReach,
            .cycleTime = cycleTime,
            .opacity = k_stoneReflectionOpacity
        };
        m_stoneReflection.draw(frame, stoneWater, {}, [&](const SceneFrame& mirror) {
            drawRoundedBoulder(mirroredAbout(mirror, height * k_stoneAt.y), stoneBase,
                width * k_stoneWidth, k_stoneSquish);
        }, stoneRipples);
        m_lakeShade.draw(frame);
        
        // The stone stands in the water before the trail is laid, so the comet's glow lights it.
        drawRoundedBoulder(frame, { width * k_stoneAt.x, height * k_stoneAt.y },
            width * k_stoneWidth, k_stoneSquish);
        drawGlitterPath(frame, passAt(frame, cycleTime), mirrorRipples, cycleTime);
        m_shore.draw(frame);
    }

    // The lake's light: the Mill's sun path, from under the comet at the far shore down to the
    // near water, with its soft glow and silver sparkles. The sparkles are the water's: they stay
    // where they are, and the light moving over them fades them in and out.
    void CometScene::drawGlitterPath(const SceneFrame& frame, const Pass& pass,
        const Ripples& ripples, const float cycleTime)
    {
        const float unit = frame.unit;
        const float lakeY = frame.height * k_lakeShare;
        const float nearY = 0.98f * frame.height;
        // While the comet falls it lights the water from its head; after the landing, from the
        // ridge it fell behind.
        const float headAlong = pass.afterglow > 0.0f
            ? 1.0f
            : std::pow(pass.passTime / k_fallDuration, 1.35f);
        const FloatPoint light = pointOnCourse(pass.course, headAlong);
        const float shine = pass.afterglow > 0.0f
            ? std::pow(1.0f - pass.afterglow, 1.6f)
            : 1.0f;
        const FloatPoint far = { light.x, lakeY + 0.008f * frame.height };
        const FloatPoint near = { light.x, nearY };

        // The water is a rectangle across the frame's foot, so it is clipped as one: a geometric
        // clip would rasterize the lake's coverage into a mask on every frame.
        const FloatPoint lakeTopLeft = frame.toCanvas.transform({ 0.0f, lakeY });
        const FloatPoint lakeBottomRight = frame.toCanvas.transform({ frame.width,
            frame.height });
        frame.canvas.pushClip(FloatRect{ lakeTopLeft.x, lakeTopLeft.y, lakeBottomRight.x,
            lakeBottomRight.y });

        // The moving area: a soft glow along the path, flat on the water and wider as it comes.
        for (int i = 0; i != k_pathGlowCount; ++i)
        {
            const float along = (static_cast<float>(i) + 0.5f)
                / static_cast<float>(k_pathGlowCount);
            drawGlow(frame, FloatPoint::between(far, near, along), (6.0f + 40.0f * along) * unit,
                CometSceneColors::Glitter.withOpacity(0.04f * shine), 0.8f);
        }

        // The stone and its shadow: no sparkle in the band they make - a little past the stone's
        // own width and down to its reflection, so none leaks along the waterline. Worked out once a
        // frame rather than once a glitter.
        const float stoneScale = frame.width * k_stoneWidth;
        const float shadowDepth = 22.0f * stoneScale * k_stoneSquish;
        const float stoneEdge = 21.0f * stoneScale;
        const float stoneX = frame.width * k_stoneAt.x;
        const float stoneTop = frame.height * k_stoneAt.y - shadowDepth;
        const float stoneBottom = frame.height * k_stoneAt.y + shadowDepth * 0.9f;
        // Each sparkle keeps the place the water gave it. The light over it decides how bright it
        // is, so the area moving is the glitters fading out and new ones fading in.
        const std::array<Glitter, k_glitterFieldCount>& field = glitterField();
        for (int i = 0; i != k_glitterFieldCount; ++i)
        {
            const Glitter& glitter = field[static_cast<std::size_t>(i)];
            const float along = glitter.along;
            const float spread = (2.0f + 80.0f * along * along) * unit;
            const float x = glitter.across * frame.width;
            const float y = lakeY + (nearY - lakeY) * along;
            if (std::abs(x - stoneX) <= stoneEdge && y >= stoneTop && y <= stoneBottom)
                continue;
            const float trailCoverage = 1.0f - smoothStep(0.60f * spread, spread,
                std::abs(x - light.x));
            // The lake twinkles either side of the trail too: a share of the field is the water's
            // own sparkle, which the sky lights rather than the comet.
            const float waterSparkle = glitter.waterShare;
            if (trailCoverage <= 0.0f && waterSparkle <= 0.0f)
                continue;
            const float twinkle = std::sin(cycleTime * k_2Pi * glitter.twinkleRate
                + glitter.twinklePhase);
            if (twinkle <= 0.0f)
                continue;
            const bool onTrail = trailCoverage * shine >= waterSparkle;
            const float size = onTrail ? 1.0f : 0.65f;
            const float reach = (1.0f + 7.0f * along) * unit * size;
            const float thickness = (0.4f + 1.1f * along) * unit;
            const float top = y - thickness * 0.5f;
            const float bottom = y + thickness * 0.5f;
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
            fillShape(frame, sparkle, SolidColor{ CometSceneColors::Glitter.withOpacity(
                k_glitterOpacity * std::min(1.0f, twinkle * 1.4f)
                * std::max(trailCoverage * shine, waterSparkle)) });
        }
        frame.canvas.popClip();
    }

    void CometScene::drawSky(const SceneFrame& frame)
    {
        const float lakeY = frame.height * k_lakeShare;
        fillBox(frame, { 0.0f, 0.0f, frame.width, lakeY }, LinearGradient{
            .startPoint = { 0.0f, 0.0f },
            .endPoint = { 0.0f, lakeY },
            .stops = {
                { .position = 0.0f, .color = CometSceneColors::Sky.top },
                { .position = 1.0f, .color = CometSceneColors::Sky.bottom }
            }
        });
    }

    // The far range shares the near range's clear colours; the air between them, at each range's
    // distance, is what parts the two.
    void CometScene::drawFarRidge(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float lakeY = frame.height * k_lakeShare;
        PixelPath farRidge;
        farRidge.moveTo(0.0f, lakeY);
        for (float x = 0.0f; x <= width + 8.0f * unit; x += 8.0f * unit)
            farRidge.lineTo(x, farRidgeY(frame, x));
        farRidge.lineTo(width + 8.0f * unit, lakeY);
        farRidge.close();
        fillShape(frame, farRidge, hazedRidge(frame, k_farRangeParallax, CometSceneColors::FarRidge));
    }

    void CometScene::drawNearRidge(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float lakeY = frame.height * k_lakeShare;
        PixelPath nearRidge;
        nearRidge.moveTo(0.0f, lakeY);
        for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
            nearRidge.lineTo(x, nearRidgeY(frame, x));
        nearRidge.lineTo(width + 6.0f * unit, lakeY);
        nearRidge.close();
        fillShape(frame, nearRidge, hazedRidge(frame, k_nearRangeParallax, CometSceneColors::NearRidge));
    }

    void CometScene::drawMirroredFarRidge(const SceneFrame& frame)
    {
        drawFarRidge(reflectionOf(frame));
    }

    void CometScene::drawMirroredNearRidge(const SceneFrame& frame)
    {
        drawNearRidge(reflectionOf(frame));
    }

    void CometScene::drawLakeWater(const SceneFrame& frame)
    {
        const float lakeY = frame.height * k_lakeShare;
        fillBox(frame, { 0.0f, lakeY, frame.width, frame.height }, LinearGradient::Simple(
            { 0.0f, lakeY }, { 0.0f, frame.height }, CometSceneColors::LakeTop,
            CometSceneColors::LakeBottom));
    }

    void CometScene::drawLakeShade(const SceneFrame& frame)
    {
        const float lakeY = frame.height * k_lakeShare;
        fillBox(frame, { 0.0f, lakeY, frame.width, frame.height }, LinearGradient::Simple(
            { 0.0f, lakeY }, { 0.0f, frame.height }, CometSceneColors::LakeShadeTop,
            CometSceneColors::LakeShadeBottom));
    }

    // The shore, a line of pines.
    void CometScene::drawShore(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float lakeY = frame.height * k_lakeShare;

        const Color hazedShore = k_air.hazed(CometSceneColors::Shore, k_shoreParallax);


        fillBox(frame, { 0.0f, lakeY - 2.0f * unit, width, lakeY + 3.0f * unit }, SolidColor{ hazedShore });
        const float spacing = 11.0f * unit;
        const int count = static_cast<int>(std::ceil(width / spacing)) + 2;
        for (int i = 0; i != count; ++i)
        {
            if (pseudoRandom(i, 62.0f) < 0.35f)
                continue;
            const float x = static_cast<float>(i) * spacing + pseudoRandom(i, 61.0f) * 6.0f * unit;
            // Two random numbers multiplied make most pines short and a few tall.
            const float tallness = pseudoRandom(i, 63.0f) * pseudoRandom(i, 64.0f);
            const float treeHeight = (10.0f + 26.0f * tallness) * unit;
            PixelPath pine;
            pine.moveTo(x - treeHeight * 0.28f, lakeY + unit);
            pine.lineTo(x, lakeY - treeHeight);
            pine.lineTo(x + treeHeight * 0.28f, lakeY + unit);
            pine.close();
            fillShape(frame, pine, SolidColor{ hazedShore });
        }
    }

    void CometScene::drawMirroredShore(const SceneFrame& frame)
    {
        drawShore(reflectionOf(frame));
    }

    // The near shore in front of everything: pines either side and boulders standing in the water,
    // their points echoing the far pines.
    void CometScene::drawForeground(const SceneFrame& frame)
    {
        const float width = frame.width;
        const float height = frame.height;
        
        drawPine(frame, { width * 0.055f, height * 1.06f }, height * 0.62f, 5);
        drawPine(frame, { width * 0.125f, height * 1.02f }, height * 0.40f, 9);
        drawPine(frame, { width * 0.29f, height * 1.01f }, height * 0.24f, 23);
        drawPine(frame, { width * 0.885f, height * 1.02f }, height * 0.36f, 17);
        drawPine(frame, { width * 0.945f, height * 1.06f }, height * 0.56f, 13);

        // The boulders standing in the water.
        drawRock(frame, { width * 0.230f, height * 1.02f }, width * 0.12f, height * 0.19f, 20);
        drawRock(frame, { width * 0.330f, height * 1.02f }, width * 0.12f, height * 0.16f, 24);

        drawRock(frame, { width * 0.420f, height * 1.01f }, width * 0.11f, height * 0.13f, 27);
        drawRock(frame, { width * 0.755f, height * 1.02f }, width * 0.22f, height * 0.29f, 31);
        // The rocks the near edge of the water breaks on, low across the bottom.
        drawRock(frame, { width * 0.100f, height * 1.01f }, width * 0.30f, height * 0.09f, 51);
        drawRock(frame, { width * 0.500f, height * 1.01f }, width * 0.16f, height * 0.045f, 57);
        drawRock(frame, { width * 0.900f, height * 1.01f }, width * 0.28f, height * 0.08f, 63);
    }
}
