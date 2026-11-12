module ClaFi.PathArt.PuppeteerScene;

import ClaFi.PathArt.AccordionAct;
import ClaFi.PathArt.AcrobaticsAct;
import ClaFi.PathArt.BallBalanceAct;
import ClaFi.PathArt.BalletAct;
import ClaFi.PathArt.CurtainCallAct;
import ClaFi.PathArt.DrumAct;
import ClaFi.PathArt.DustMotesEffect;
import ClaFi.PathArt.GatorAct;
import ClaFi.PathArt.GuitarAct;
import ClaFi.PathArt.JugglingAct;
import ClaFi.PathArt.KettlebellAct;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.PuppetShowAct;
import ClaFi.PathArt.RobotDanceAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.StringsCutAct;
import ClaFi.PathArt.TopHatAct;
import ClaFi.PathArt.WolfHoopAct;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace PuppeteerColors
    {
        constexpr Color WallTop{ 0xFF2C1832 };
        constexpr Color WallBottom{ 0xFF170C1B };
        constexpr Color WallGlow{ 0xFF6B2C4F };
        constexpr Color FloorBack{ 0xFF3D2515 };
        constexpr Color FloorFront{ 0xFF7C5434 };
        constexpr Color FloorSeam{ 0xFF24150B };
        constexpr Color FloorEdge{ 0xFF0D070B };
        constexpr Color Spotlight{ 0xFFFFF1C9 };
        constexpr Color SpotlightPool{ 0xFFFFE9B0 };
        constexpr Color Dust{ 0xFFFFF1C9 };
        constexpr Color CurtainShade{ 0xFF4F0B18 };
        constexpr Color CurtainLight{ 0xFFAB2740 };
        constexpr Color ValanceTop{ 0xFF9A1D33 };
        constexpr Color ValanceBottom{ 0xFF6A1022 };
        constexpr Color Gold{ 0xFFD9A845 };
        constexpr Color StandWood{ 0xFF6E4526 };
        constexpr Color StandArm{ 0xFF8A5A33 };
        constexpr Color StandFoot{ 0xFF4A2C16 };
    }

    namespace
    {
        // The floor's top edge and the line the feet stand on, as shares of the frame's height.
        constexpr float k_floorTopShare{ 0.64f };
        constexpr float k_floorLineShare{ 0.85f };
        // Ten-second acts in the forty-second cycle; the show runs on over the cycles.
        constexpr float k_actsPerCycle{ 4.0f };
        // The curtain stays shut this long across each change of act, and runs this long either
        // side of it.
        constexpr float k_curtainShutSeconds{ 0.6f };
        constexpr float k_curtainRunSeconds{ 0.75f };
        // The hanging marionette swings 12 times a cycle, so it comes round with it.
        constexpr float k_hangingSwingsPerCycle{ 12.0f };

        [[nodiscard]] float drapeWidthOf(const SceneFrame& frame)
        {
            return std::max(frame.width * 0.11f, 62.0f * frame.unit);
        }

        // The hook of the marionette's stand, just inside the left drapes.
        [[nodiscard]] FloatPoint standHookOf(const SceneFrame& frame)
        {
            const float unit = frame.unit;
            return {
                drapeWidthOf(frame) + 80.0f * unit,
                frame.height * k_floorLineShare - 304.0f * unit
            };
        }

        // 0 open to 1 shut, from how far the act is from its start or its end.
        [[nodiscard]] float curtainClosureAt(const float actTime)
        {
            const float fromChange = std::min(actTime, k_actSeconds - actTime);
            return 1.0f - smoothStep(0.0f, 1.0f,
                (fromChange - k_curtainShutSeconds * 0.5f) / k_curtainRunSeconds);
        }
    }

    PuppeteerScene::PuppeteerScene()
    {
        m_acts.push_back(std::make_unique<PuppetShowAct>());
        m_acts.push_back(std::make_unique<RobotDanceAct>());
        m_acts.push_back(std::make_unique<JugglingAct>());
        m_acts.push_back(std::make_unique<AccordionAct>());
        m_acts.push_back(std::make_unique<AcrobaticsAct>());
        m_acts.push_back(std::make_unique<TopHatAct>());
        m_acts.push_back(std::make_unique<GuitarAct>());
        m_acts.push_back(std::make_unique<BallBalanceAct>());
        m_acts.push_back(std::make_unique<GatorAct>());
        m_acts.push_back(std::make_unique<DrumAct>());
        m_acts.push_back(std::make_unique<BalletAct>());
        m_acts.push_back(std::make_unique<KettlebellAct>());
        m_acts.push_back(std::make_unique<WolfHoopAct>());
        m_acts.push_back(std::make_unique<StringsCutAct>());
        m_acts.push_back(std::make_unique<CurtainCallAct>());
    }

    void PuppeteerScene::prepare(const SceneParams& params)
    {
        m_stage.prepare(params);
        m_valance.prepare(params);
    }

    // A jolt moves the stage and all on it for half a second; the curtains and the valance hang
    // from above and keep still, but for the drapes swinging a little longer.
    void PuppeteerScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float floorY = frame.height * k_floorLineShare;
        const ActStage stage{
            .frame = frame,
            .centerX = frame.width * 0.5f,
            .floorY = floorY,
            .closeness = std::min(1.0f, (frame.width / unit) / 640.0f),
            .drapeWidth = drapeWidthOf(frame)
        };

        const float actsIntoCycle = time.cycleTime * k_actsPerCycle;
        const std::int64_t actCount = static_cast<std::int64_t>(m_acts.size());
        const std::int64_t actNumber = time.cyclesPassed * static_cast<std::int64_t>(k_actsPerCycle)
            + static_cast<std::int64_t>(std::floor(actsIntoCycle));
        PuppeteerAct& act = *m_acts[static_cast<std::size_t>((actNumber % actCount + actCount)
            % actCount)];
        const float actTime = fractionalPart(actsIntoCycle) * k_actSeconds;
        act.pose(stage, actTime, time);

        const float joltAge = act.joltAge();
        FloatPoint shake{};
        float swing = 0.0f;
        if (joltAge >= 0.0f)
        {
            const float fading = std::exp(-joltAge / 0.35f);
            shake = {
                fading * 2.5f * unit * std::sin(k_2Pi * joltAge * 6.0f + 1.0f),
                fading * 7.0f * unit * std::sin(k_2Pi * joltAge * 9.0f)
            };
            swing = std::exp(-joltAge / 0.9f) * std::sin(k_2Pi * joltAge * 1.3f);
            // The stage moved off an edge leaves a strip there, filled in the stage's own colours.
            fillBox(frame, { 0.0f, 0.0f, frame.width, 10.0f * unit },
                SolidColor{ PuppeteerColors::WallTop });
            fillBox(frame, { 0.0f, frame.height - 10.0f * unit, frame.width, frame.height },
                SolidColor{ PuppeteerColors::FloorFront });
        }
        const SceneFrame shaken = joltAge >= 0.0f
            ? frame.within(Matrix3x2::Translation(shake))
            : frame;

        m_stage.draw(frame, shake);
        const float lampX = stage.centerX - 56.0f * unit * stage.closeness;
        const float spotlightX = act.spotlightX();
        drawSpotlight(shaken, lampX, spotlightX, floorY);
        if (!act.usesMarionette())
        {
            const float hangingSwing = 0.3f * swing
                + 0.035f * std::sin(k_2Pi * k_hangingSwingsPerCycle * time.cycleTime);
            drawMarionette(shaken, hangMarionette(unit, standHookOf(frame), hangingSwing));
        }
        act.draw(shaken);
        drawDustMotes(shaken, time.cycleTime, DustMotes{
            .lamp = { lampX, 0.0f },
            .pool = { spotlightX, floorY },
            .top = frame.height * 0.04f,
            .spread = 180.0f * unit,
            .color = PuppeteerColors::Dust
        });

        constexpr float k_step{ 0.01f };
        const float closingSpeed = (curtainClosureAt(actTime + k_step)
            - curtainClosureAt(actTime - k_step)) / (2.0f * k_step);
        drawCurtain(frame, curtainClosureAt(actTime), closingSpeed, time.cycleTime);
        drawDrapes(frame, time.cycleTime, swing);
        m_valance.draw(frame);
    }

    void PuppeteerScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        // From the feet to the top of the icon, where the strings are cut.
        constexpr float k_figureSpan{ 580.0f };
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const float unit = frame.unit;
        const FloatPoint feet = FloatPoint{ frame.width * 0.5f, frame.height * 0.5f }
            + FloatPoint{ -15.0f, 285.0f } * unit;
        canvas.pushClip(bounds);
        drawMarionette(frame, danceMarionette(2.5f * unit, feet, 0.6f));
        canvas.popClip();
    }

    SceneReadoutColors PuppeteerScene::readoutColors() const
    {
        return {
            .field = PuppeteerColors::WallBottom,
            .series = PuppeteerColors::SpotlightPool
        };
    }

    // The stand the marionette hangs on between its acts is kept with the stage.
    void PuppeteerScene::drawStage(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float centerX = width * 0.5f;
        const float floorTop = height * k_floorTopShare;

        fillBox(frame, { 0.0f, 0.0f, width, floorTop },
            LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, floorTop }, PuppeteerColors::WallTop,
                PuppeteerColors::WallBottom));
        drawGlow(frame, { centerX - 40.0f * unit, floorTop - 30.0f * unit }, 300.0f * unit,
            PuppeteerColors::WallGlow.withOpacity(0.35f), 0.55f);

        fillBox(frame, { 0.0f, floorTop, width, height },
            LinearGradient::Simple({ 0.0f, floorTop }, { 0.0f, height }, PuppeteerColors::FloorBack,
                PuppeteerColors::FloorFront));

        // The planks run toward a point high above the stage: the floor as seen from the stalls.
        const float vanishingY = floorTop - height * 0.9f;
        const float share = (floorTop - vanishingY) / (height - vanishingY);
        ScatteredShapes planks{ frame, SolidColor{ PuppeteerColors::FloorSeam.withOpacity(0.5f) },
            1.2f * unit, StrokeCap::Butt };
        for (int i = -18; i <= 18; ++i)
        {
            const float bottomX = centerX + static_cast<float>(i) * 54.0f * unit;
            PixelPath& plank = planks.next();
            plank.moveTo(centerX + (bottomX - centerX) * share, floorTop);
            plank.lineTo(bottomX, height);
        }
        planks.draw();

        ScatteredShapes seams{ frame, SolidColor{ PuppeteerColors::FloorSeam.withOpacity(0.32f) },
            unit, StrokeCap::Butt };
        for (const float depth : { 0.1f, 0.25f, 0.44f, 0.68f, 0.96f })
        {
            const float y = floorTop + (height - floorTop) * depth;
            PixelPath& seam = seams.next();
            seam.moveTo(0.0f, y);
            seam.lineTo(width, y);
        }
        seams.draw();
        fillBox(frame, { 0.0f, floorTop - 4.0f * unit, width, floorTop + 3.0f * unit },
            SolidColor{ PuppeteerColors::FloorEdge.withOpacity(0.85f) });

        // the stand: a pole on a round foot, an arm out to the right, a hook under its end
        const FloatPoint hook = standHookOf(frame);
        const float poleX = hook.x - 34.0f * unit;
        const float armY = hook.y - 18.0f * unit;
        const float floorY = height * k_floorLineShare;
        fillOval(frame, { poleX, floorY + unit }, 24.0f * unit, 5.0f * unit,
            SolidColor{ PuppeteerColors::StandFoot });
        drawSegment(frame, { poleX, floorY }, { poleX, armY },
            SolidColor{ PuppeteerColors::StandWood }, 6.0f * unit, StrokeCap::Butt);
        drawSegment(frame, { poleX - 4.0f * unit, armY }, { hook.x + 5.0f * unit, armY },
            SolidColor{ PuppeteerColors::StandArm }, 6.0f * unit);
        drawSegment(frame, { hook.x, armY + 2.0f * unit }, { hook.x, hook.y - 4.0f * unit },
            SolidColor{ PuppeteerColors::Gold }, 2.0f * unit);
    }

    void PuppeteerScene::drawSpotlight(const SceneFrame& frame, const float lampX,
        const float targetX, const float floorY)
    {
        const float unit = frame.unit;
        PixelPath cone;
        cone.moveTo(lampX - 26.0f * unit, 0.0f);
        cone.lineTo(lampX + 26.0f * unit, 0.0f);
        cone.lineTo(targetX + 112.0f * unit, floorY + 14.0f * unit);
        cone.lineTo(targetX - 112.0f * unit, floorY + 14.0f * unit);
        cone.close();
        const Color beam = PuppeteerColors::Spotlight;
        fillShape(frame, cone, LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, floorY },
            beam.withOpacity(0.02f), beam.withOpacity(0.17f)));
        drawGlow(frame, { targetX, floorY + 6.0f * unit }, 50.0f * unit,
            PuppeteerColors::SpotlightPool.withOpacity(0.5f), 0.42f);
    }

    // Two halves of the drapes' cloth run out from behind them and meet in the middle. Moving, a
    // half's hem trails its top; shut, a warm light rests on it.
    // See PathArt#puppeteerscene-drawcurtain
    void PuppeteerScene::drawCurtain(const SceneFrame& frame, const float closure,
        const float closingSpeed, const float cycleTime)
    {
        if (closure <= 0.0005f)
            return;
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float hidden = drapeWidthOf(frame) * 0.5f;
        const float meeting = width * 0.5f + 5.0f * unit;
        const float edge = std::lerp(hidden, meeting, closure);
        const float trail = -closingSpeed * (meeting - hidden) * 0.07f;
        constexpr int k_folds{ 14 };
        for (const bool leftHalf : { true, false })
        {
            const float inward = leftHalf ? 1.0f : -1.0f;
            const float side = leftHalf ? 0.0f : width;
            const float halfPhase = leftHalf ? 0.0f : 1.3f;
            const auto stirAt = [&](int fold) {
                return 3.0f * unit * std::sin(k_2Pi * 2.0f * cycleTime
                    + static_cast<float>(fold) * 0.7f + halfPhase);
            };
            // a backing under the folds, so no seam between two of them lets the stage through
            PixelPath backing;
            backing.moveTo(side, 0.0f);
            backing.lineTo(side + inward * edge, 0.0f);
            backing.lineTo(side + inward * (edge + trail + stirAt(k_folds - 1)), height);
            backing.lineTo(side, height);
            backing.close();
            fillShape(frame, backing, SolidColor{ PuppeteerColors::CurtainShade });
            for (int fold = 0; fold != k_folds; ++fold)
            {
                const float near = static_cast<float>(fold) / k_folds;
                const float far = static_cast<float>(fold + 1) / k_folds;
                const float topNear = edge * near;
                const float topFar = edge * far;
                const float hemNear = topNear + trail * near * near + stirAt(fold);
                const float hemFar = topFar + trail * far * far + stirAt(fold);
                PixelPath cloth;
                cloth.moveTo(side + inward * topNear, 0.0f);
                cloth.lineTo(side + inward * topFar, 0.0f);
                cloth.lineTo(side + inward * hemFar, height);
                cloth.lineTo(side + inward * hemNear, height);
                cloth.close();
                fillShape(frame, cloth, LinearGradient{
                    .startPoint = { side + inward * topNear, 0.0f },
                    .endPoint = { side + inward * topFar, 0.0f },
                    .stops = {
                        { .position = 0.0f, .color = PuppeteerColors::CurtainShade },
                        { .position = 0.5f, .color = PuppeteerColors::CurtainLight },
                        { .position = 1.0f, .color = PuppeteerColors::CurtainShade }
                    }
                });
            }
        }
        const float floorY = height * k_floorLineShare;
        drawGlow(frame, { width * 0.5f, floorY - 170.0f * unit }, 250.0f * unit,
            PuppeteerColors::SpotlightPool.withOpacity(0.2f * smoothStep(0.6f, 1.0f, closure)),
            0.7f);
    }

    // `swing` throws the folds out further, after a jolt.
    void PuppeteerScene::drawDrapes(const SceneFrame& frame, const float cycleTime,
        const float swing)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float drapeWidth = drapeWidthOf(frame);
        constexpr int k_folds = 5;
        const float foldWidth = drapeWidth / k_folds;
        for (const bool left : { true, false })
        {
            for (int i = 0; i != k_folds; ++i)
            {
                const float foldLeft = left
                    ? static_cast<float>(i) * foldWidth
                    : width - static_cast<float>(i + 1) * foldWidth;
                const float foldRight = foldLeft + foldWidth;
                const float wavePhase = static_cast<float>(i) * 0.9f + (left ? 0.0f : 2.0f);
                const float stir = 4.0f * unit * std::sin(k_2Pi * 2.0f * cycleTime + wavePhase)
                    + 12.0f * unit * swing * (0.6f + 0.1f * static_cast<float>(i));
                PixelPath fold;
                fold.moveTo(foldLeft, 0.0f);
                fold.lineTo(foldRight, 0.0f);
                fold.lineTo(foldRight + stir, height);
                fold.lineTo(foldLeft + stir, height);
                fold.close();
                fillShape(frame, fold, LinearGradient{
                    .startPoint = { foldLeft, 0.0f },
                    .endPoint = { foldRight, 0.0f },
                    .stops = {
                        { .position = 0.0f, .color = PuppeteerColors::CurtainShade },
                        { .position = 0.5f, .color = PuppeteerColors::CurtainLight },
                        { .position = 1.0f, .color = PuppeteerColors::CurtainShade }
                    }
                });
            }
        }
    }

    void PuppeteerScene::drawValance(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float valanceHeight = std::max(height * 0.085f, 40.0f * unit);
        const float scallop = 64.0f * unit;
        PixelPath valance;
        valance.moveTo(0.0f, 0.0f);
        valance.lineTo(width, 0.0f);
        valance.lineTo(width, valanceHeight);
        PixelPath trim;
        trim.moveTo(width, valanceHeight);
        for (float x = width; x > -scallop; x -= scallop)
        {
            valance.quadTo({ x - scallop * 0.5f, valanceHeight + 20.0f * unit },
                { x - scallop, valanceHeight });
            trim.quadTo({ x - scallop * 0.5f, valanceHeight + 20.0f * unit },
                { x - scallop, valanceHeight });
        }
        valance.lineTo(-scallop, 0.0f);
        valance.close();
        fillShape(frame, valance,
            LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, valanceHeight + 10.0f * unit },
            PuppeteerColors::ValanceTop, PuppeteerColors::ValanceBottom));
        strokeShape(frame, trim, SolidColor{ PuppeteerColors::Gold }, 3.0f * unit);
    }
}
