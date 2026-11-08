module ClaFi.PathArt.PuppeteerScene;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.DustMotesEffect;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
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
    }

    namespace
    {
        // The floor's top edge and the line the feet stand on, as shares of the frame's height.
        constexpr float k_floorTopShare{ 0.64f };
        constexpr float k_floorLineShare{ 0.85f };
        // Sways of the dance per cycle; a step falls on every half sway.
        constexpr float k_swaysPerCycle{ 16.0f };
    }

    void PuppeteerScene::prepare(const SceneParams& params)
    {
        m_stage.prepare(params);
        m_valance.prepare(params);
    }

    void PuppeteerScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float centerX = frame.width * 0.5f;
        const float floorY = frame.height * k_floorLineShare;
        // A narrow frame draws the puppet and the android closer together.
        const float closeness = std::min(1.0f, (frame.width / unit) / 640.0f);
        const float sway = k_2Pi * k_swaysPerCycle * time.cycleTime;

        const float puppetX = centerX - 78.0f * unit * closeness + 34.0f * unit * std::sin(sway);
        const MarionettePose marionette = danceMarionette(unit, { puppetX, floorY }, sway);

        m_stage.draw(frame);
        const float lampX = centerX - 56.0f * unit * closeness;
        drawSpotlight(frame, lampX, puppetX, floorY);

        const float androidX = centerX + 80.0f * unit * closeness;
        const AndroidPose android = {
            .feet = { androidX, floorY },
            .sway = sway,
            .grip = marionette.control.center - FloatPoint{ 0.0f, 9.0f * unit },
            .cycleTime = time.cycleTime
        };
        drawAndroid(frame, android);
        drawMarionette(frame, marionette);
        drawAndroidHand(frame, android);

        drawDustMotes(frame, time.cycleTime, DustMotes{
            .lamp = { lampX, 0.0f },
            .pool = { puppetX, floorY },
            .top = frame.height * 0.04f,
            .spread = 180.0f * unit,
            .color = PuppeteerColors::Dust
        });
        drawCurtains(frame, time.cycleTime);
        m_valance.draw(frame);
    }

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
    }

    void PuppeteerScene::drawSpotlight(const SceneFrame& frame, const float lampX,
        const float puppetX, const float floorY)
    {
        const float unit = frame.unit;
        PixelPath cone;
        cone.moveTo(lampX - 26.0f * unit, 0.0f);
        cone.lineTo(lampX + 26.0f * unit, 0.0f);
        cone.lineTo(puppetX + 112.0f * unit, floorY + 14.0f * unit);
        cone.lineTo(puppetX - 112.0f * unit, floorY + 14.0f * unit);
        cone.close();
        const Color beam = PuppeteerColors::Spotlight;
        fillShape(frame, cone, LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, floorY },
            beam.withOpacity(0.02f), beam.withOpacity(0.17f)));
        drawGlow(frame, { puppetX, floorY + 6.0f * unit }, 50.0f * unit,
            PuppeteerColors::SpotlightPool.withOpacity(0.5f), 0.42f);
    }

    void PuppeteerScene::drawCurtains(const SceneFrame& frame, const float cycleTime)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float drapeWidth = std::max(width * 0.11f, 62.0f * unit);
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
                const float swing = 4.0f * unit * std::sin(k_2Pi * 2.0f * cycleTime + wavePhase);
                PixelPath fold;
                fold.moveTo(foldLeft, 0.0f);
                fold.lineTo(foldRight, 0.0f);
                fold.lineTo(foldRight + swing, height);
                fold.lineTo(foldLeft + swing, height);
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
