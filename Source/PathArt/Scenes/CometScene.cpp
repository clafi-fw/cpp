module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.CometSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.ShootingStarsEffect;
import ClaFi.PathArt.StarsEffect;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace CometSceneColors
    {
        constexpr Color SkyTop{ 0xFF050716 };
        constexpr Color SkyMiddle{ 0xFF1B1542 };
        constexpr Color SkyHorizon{ 0xFF4A2556 };
        constexpr Color Flash{ 0xFFFF9A66 };
        constexpr Color Star{ 0xFFEEF0FF };
        constexpr Color ShootingStar{ 0xFFFFFFFF };
        constexpr Color FarRidgeTop{ 0xFF3A2E63 };
        constexpr Color FarRidgeBottom{ 0xFF241C47 };
        constexpr Color NearRidgeTop{ 0xFF1B1538 };
        constexpr Color NearRidgeBottom{ 0xFF120E27 };
        constexpr Color Afterglow{ 0xFFFFB070 };
        constexpr Color ImpactCore{ 0xFFFFF1D0 };
        constexpr Color RidgeRim{ 0xFFFFD8A8 };
        constexpr Color LakeTop{ 0xFF2A1A45 };
        constexpr Color LakeBottom{ 0xFF070A1C };
        constexpr Color LakeShadeTop{ 0x260B0F2A };
        constexpr Color LakeShadeBottom{ 0xBF05081A };
        constexpr Color Ripple{ 0x1FC9B6FF };
        constexpr Color Shore{ 0xFF08061A };
    }

    namespace
    {
        constexpr float k_passesPerCycle{ 2.0f };
        // The share of a pass the comet falls for; the rest is the glow behind the ridge.
        constexpr float k_fallShare{ 0.6f };
        // Where the lake begins, as a share of the frame's height.
        constexpr float k_lakeShare{ 0.72f };
        constexpr float k_reflectionOpacity{ 0.42f };
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
    }

    void CometScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        drawSky(frame);
        drawAboveLake(frame, time.cycleTime, false);
        drawLake(frame, time.cycleTime);
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
        const bool falling = passTime < k_fallShare;
        const FloatPoint start = {
            frame.width * (fromLeft ? -0.04f : 1.04f),
            frame.height * (fromLeft ? 0.06f : 0.12f)
        };
        const float endX = frame.width * (fromLeft ? 0.73f : 0.2f);
        const FloatPoint end = { endX, nearRidgeY(frame, endX) + 8.0f * frame.unit };
        // The control sits a little above the start: the comet sets off across the sky, then falls.
        return {
            .course = {
                .start = start,
                .control = { std::lerp(start.x, end.x, 0.62f), start.y - frame.height * 0.03f },
                .end = end
            },
            .passTime = passTime,
            .afterglow = falling ? 0.0f : (passTime - k_fallShare) / (1.0f - k_fallShare)
        };
    }

    SceneFrame CometScene::reflectionOf(const SceneFrame& frame)
    {
        const float lakeY = frame.height * k_lakeShare;
        return frame.within(Matrix3x2::Translation(0.0f, lakeY * 2.0f)
            * Matrix3x2::Scale(1.0f, -1.0f));
    }

    // The ridges are kept as layers drawn through the upright frame, the reflection's included:
    // a layer fits the frame the scene is drawn in, not one turned over inside it.
    void CometScene::drawAboveLake(const SceneFrame& upright, const float cycleTime,
        const bool reflection) const
    {
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

        // The landing lights the sky behind the near ridge, which hides the comet as it lands.
        if (pass.afterglow > 0.0f)
        {
            const float fading = 1.0f - pass.afterglow;
            drawGlow(frame, pass.course.end, (70.0f + 240.0f * std::sqrt(pass.afterglow)) * unit,
                CometSceneColors::Afterglow.withOpacity(0.8f * std::pow(fading, 1.6f)), 0.6f);
            drawGlow(frame, pass.course.end, 50.0f * unit,
                CometSceneColors::ImpactCore.withOpacity(std::pow(fading, 4.0f)));
        }
        drawComet(frame, CometPose{
            .course = pass.course,
            .passTime = pass.passTime,
            .fallShare = k_fallShare,
            .cycleTime = cycleTime
        });

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
            const float rimOpacity = 0.85f * std::pow(1.0f - pass.afterglow, 1.5f);
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
    }

    // The lake shows everything above it upside down and dimmed, with ripples running across.
    void CometScene::drawLake(const SceneFrame& frame, const float cycleTime) const
    {
        const float unit = frame.unit;
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
        frame.canvas.pushClip(lakeOutline, &frame.toCanvas);
        {
            const ScopedCanvasOpacity dimmed{ frame.canvas, k_reflectionOpacity };
            drawAboveLake(frame, cycleTime, true);
        }
        frame.canvas.popClip();
        m_lakeShade.draw(frame);

        ScatteredShapes ripples{ frame, SolidColor{ CometSceneColors::Ripple }, 1.1f * unit,
            StrokeCap::Butt };
        for (int i = 0; i != 26; ++i)
        {
            const float depth = std::pow(pseudoRandom(i, 51.0f), 0.8f);
            const float y = lakeY + (height - lakeY) * depth;
            const float length = (30.0f + 70.0f * pseudoRandom(i, 52.0f)) * unit * (0.6f + depth);
            const float laps = static_cast<float>(1 + i % 3);
            const float x = fractionalPart(pseudoRandom(i, 53.0f) + cycleTime * laps)
                * (width + length * 2.0f) - length;
            PixelPath& ripple = ripples.next();
            ripple.moveTo(x, y);
            ripple.lineTo(x + length, y);
        }
        ripples.draw();
        m_shore.draw(frame);
    }

    void CometScene::drawSky(const SceneFrame& frame)
    {
        const float lakeY = frame.height * k_lakeShare;
        fillBox(frame, { 0.0f, 0.0f, frame.width, lakeY }, LinearGradient{
            .startPoint = { 0.0f, 0.0f },
            .endPoint = { 0.0f, lakeY },
            .stops = {
                { .position = 0.0f, .color = CometSceneColors::SkyTop },
                { .position = 0.6f, .color = CometSceneColors::SkyMiddle },
                { .position = 1.0f, .color = CometSceneColors::SkyHorizon }
            }
        });
    }

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
        fillShape(frame, farRidge, LinearGradient::Simple({ 0.0f, frame.height * 0.35f },
            { 0.0f, lakeY }, CometSceneColors::FarRidgeTop, CometSceneColors::FarRidgeBottom));
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
        fillShape(frame, nearRidge, LinearGradient::Simple({ 0.0f, frame.height * 0.5f },
            { 0.0f, lakeY }, CometSceneColors::NearRidgeTop, CometSceneColors::NearRidgeBottom));
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
        fillBox(frame, { 0.0f, lakeY - 2.0f * unit, width, lakeY + 3.0f * unit },
            SolidColor{ CometSceneColors::Shore });
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
            fillShape(frame, pine, SolidColor{ CometSceneColors::Shore });
        }
    }
}
