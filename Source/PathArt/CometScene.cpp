module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace CometColors
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
        constexpr Color DustHead{ 0xFFFFF4DC };
        constexpr Color DustMiddle{ 0xFFFFC68A };
        constexpr Color DustEnd{ 0xFFFF9A5C };
        constexpr Color IonTail{ 0xFF9FE6FF };
        constexpr Color ColdSpark{ 0xFFBFE9FF };
        constexpr Color WarmSpark{ 0xFFFFE2B0 };
        constexpr Color Coma{ 0xFFCFEAFF };
        constexpr Color Nucleus{ 0xFFFFFFFF };
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

    void CometScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float lakeY = frame.height * k_lakeShare;
        fillBox(frame, { 0.0f, 0.0f, frame.width, lakeY }, LinearGradient{
            .startPoint = { 0.0f, 0.0f },
            .endPoint = { 0.0f, lakeY },
            .stops = {
                { .position = 0.0f, .color = CometColors::SkyTop },
                { .position = 0.6f, .color = CometColors::SkyMiddle },
                { .position = 1.0f, .color = CometColors::SkyHorizon }
            }
        });
        drawAboveLake(frame, cycleTime, lakeY, false);
        drawLake(frame, cycleTime, lakeY);
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

    FloatPoint CometScene::pointOnCurve(const Pass& pass, const float along)
    {
        const float rest = 1.0f - along;
        return pass.start * (rest * rest) + pass.control * (2.0f * rest * along)
            + pass.end * (along * along);
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
            .start = start,
            .control = { std::lerp(start.x, end.x, 0.62f), start.y - frame.height * 0.03f },
            .end = end,
            .passTime = passTime,
            .along = falling ? std::pow(passTime / k_fallShare, 1.35f) : 1.0f,
            .afterglow = falling ? 0.0f : (passTime - k_fallShare) / (1.0f - k_fallShare)
        };
    }

    void CometScene::drawAboveLake(const SceneFrame& frame, const float cycleTime,
        const float lakeY, const bool reflection)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const Pass pass = passAt(frame, cycleTime);

        if (pass.afterglow > 0.0f)
        {
            const float flash = 0.16f * std::pow(1.0f - pass.afterglow, 5.0f);
            fillBox(frame, { 0.0f, 0.0f, width, lakeY },
                SolidColor{ CometColors::Flash.withOpacity(flash) });
        }
        drawStars(frame, cycleTime, StarField{
            .count = 140,
            .top = 0.0f,
            .bottom = frame.height * 0.6f,
            .seed = 70.0f,
            .color = CometColors::Star,
            .opacity = reflection ? 0.6f : 1.0f
        });
        drawShootingStars(frame, cycleTime);

        PixelPath farRidge;
        farRidge.moveTo(0.0f, lakeY);
        for (float x = 0.0f; x <= width + 8.0f * unit; x += 8.0f * unit)
            farRidge.lineTo(x, farRidgeY(frame, x));
        farRidge.lineTo(width + 8.0f * unit, lakeY);
        farRidge.close();
        fillShape(frame, farRidge, LinearGradient::Simple({ 0.0f, frame.height * 0.35f },
            { 0.0f, lakeY }, CometColors::FarRidgeTop, CometColors::FarRidgeBottom));

        // The landing lights the sky behind the near ridge, which hides the comet as it lands.
        if (pass.afterglow > 0.0f)
        {
            const float fading = 1.0f - pass.afterglow;
            drawGlow(frame, pass.end, (70.0f + 240.0f * std::sqrt(pass.afterglow)) * unit,
                CometColors::Afterglow.withOpacity(0.8f * std::pow(fading, 1.6f)), 0.6f);
            drawGlow(frame, pass.end, 50.0f * unit,
                CometColors::ImpactCore.withOpacity(std::pow(fading, 4.0f)));
        }
        drawCometAndTails(frame, pass, cycleTime);

        PixelPath nearRidge;
        PixelPath rim;
        nearRidge.moveTo(0.0f, lakeY);
        for (float x = 0.0f; x <= width + 6.0f * unit; x += 6.0f * unit)
        {
            nearRidge.lineTo(x, nearRidgeY(frame, x));
            if (x == 0.0f)
                rim.moveTo(x, nearRidgeY(frame, x));
            else
                rim.lineTo(x, nearRidgeY(frame, x));
        }
        nearRidge.lineTo(width + 6.0f * unit, lakeY);
        nearRidge.close();
        fillShape(frame, nearRidge, LinearGradient::Simple({ 0.0f, frame.height * 0.5f },
            { 0.0f, lakeY }, CometColors::NearRidgeTop, CometColors::NearRidgeBottom));
        if (pass.afterglow > 0.0f)
        {
            const float rimOpacity = 0.85f * std::pow(1.0f - pass.afterglow, 1.5f);
            strokeShape(frame, rim, LinearGradient{
                .startPoint = { pass.end.x - 220.0f * unit, 0.0f },
                .endPoint = { pass.end.x + 220.0f * unit, 0.0f },
                .stops = {
                    { .position = 0.0f, .color = CometColors::Afterglow.withOpacity(0.0f) },
                    { .position = 0.5f, .color = CometColors::RidgeRim.withOpacity(rimOpacity) },
                    { .position = 1.0f, .color = CometColors::Afterglow.withOpacity(0.0f) }
                }
            }, 1.8f * unit);
        }
    }

    // Three a cycle, each half a second, in the quiet between passes.
    void CometScene::drawShootingStars(const SceneFrame& frame, const float cycleTime)
    {
        const float unit = frame.unit;
        const FloatPoint heading = { -0.82f, 0.57f };
        for (int i = 0; i != 3; ++i)
        {
            const float startTime = (static_cast<float>(i) + 0.35f) / 3.0f
                + 0.05f * pseudoRandom(i, 9.0f);
            const float flight = (cycleTime - startTime) / 0.014f;
            if (flight < 0.0f || flight > 1.0f)
                continue;
            const FloatPoint from = {
                frame.width * (0.15f + 0.7f * pseudoRandom(i, 7.0f)),
                frame.height * (0.06f + 0.22f * pseudoRandom(i, 8.0f))
            };
            const FloatPoint head = from + heading * (160.0f * unit * flight);
            const FloatPoint tail = head - heading * (50.0f * unit);
            const float brightness = 0.9f * std::sin(std::numbers::pi_v<float> * flight);
            drawSegment(frame, head, tail, LinearGradient::Simple(head, tail,
                CometColors::ShootingStar.withOpacity(brightness),
                CometColors::ShootingStar.withOpacity(0.0f)), 1.6f * unit);
        }
    }

    void CometScene::drawCometAndTails(const SceneFrame& frame, const Pass& pass,
        const float cycleTime)
    {
        const float unit = frame.unit;
        const float tailFade = 1.0f - smoothStep(k_fallShare, k_fallShare + 0.08f, pass.passTime);
        if (tailFade <= 0.0f)
            return;
        const FloatPoint head = pointOnCurve(pass, pass.along);

        // The dust tail follows the curve the comet came along, fanning out as it goes and
        // growing with the comet's speed.
        const float speed = pass.passTime < k_fallShare
            ? 0.5f + 1.3f * std::sqrt(pass.passTime / k_fallShare)
            : 1.8f;
        const float span = 0.05f + 0.1f * speed;
        constexpr int k_samples = 16;
        std::array<FloatPoint, k_samples + 1> outerEdge{};
        std::array<FloatPoint, k_samples + 1> innerEdge{};
        FloatPoint tailEnd = head;
        for (int i = 0; i <= k_samples; ++i)
        {
            const float share = static_cast<float>(i) / k_samples;
            const float along = pass.along - span * share;
            const FloatPoint point = pointOnCurve(pass, along);
            const FloatPoint ahead = pointOnCurve(pass, along + 0.002f);
            const FloatPoint step = ahead - point;
            const float stepLength = std::hypot(step.x, step.y);
            const FloatPoint tangent = stepLength > 0.0f
                ? step * (1.0f / stepLength)
                : FloatPoint{ 1.0f, 0.0f };
            const FloatPoint outward = { -tangent.y, tangent.x };
            const float taper = 15.0f * std::pow(1.0f - share, 1.1f) + 2.0f * share;
            const float halfWidth = taper * unit * (0.7f + 0.3f * speed);
            const float fan = 1.0f + share * 1.6f;
            outerEdge[static_cast<std::size_t>(i)] = point + outward * (halfWidth * fan);
            innerEdge[static_cast<std::size_t>(i)] = point - outward * (halfWidth * 0.5f);
            tailEnd = point;
        }
        PixelPath dustTail;
        dustTail.moveTo(outerEdge[0]);
        for (std::size_t i = 1; i != outerEdge.size(); ++i)
            dustTail.lineTo(outerEdge[i]);
        for (std::size_t i = innerEdge.size(); i != 0; --i)
            dustTail.lineTo(innerEdge[i - 1]);
        dustTail.close();
        const Color headDust = CometColors::DustHead.withOpacity(0.85f * tailFade);
        const Color middleDust = CometColors::DustMiddle.withOpacity(0.45f * tailFade);
        fillShape(frame, dustTail, LinearGradient{
            .startPoint = head,
            .endPoint = tailEnd,
            .stops = {
                { .position = 0.0f, .color = headDust },
                { .position = 0.35f, .color = middleDust },
                { .position = 1.0f, .color = CometColors::DustEnd.withOpacity(0.0f) }
            }
        });

        // The ion tail runs straight back from the head, a little off the line the comet came
        // along.
        const FloatPoint justBehind = pointOnCurve(pass, std::max(0.0f, pass.along - 0.01f));
        FloatPoint back = justBehind - head;
        const float backLength = std::hypot(back.x, back.y);
        back = backLength > 0.0f
            ? rotated(back * (1.0f / backLength), 0.16f)
            : FloatPoint{ -1.0f, 0.0f };
        const float ionLength = std::hypot(tailEnd.x - head.x, tailEnd.y - head.y) * 1.25f;
        const FloatPoint ionEnd = head + back * ionLength;
        const FloatPoint across = FloatPoint{ -back.y, back.x } * (3.0f * unit);
        PixelPath ionTail;
        ionTail.moveTo(head + across);
        ionTail.lineTo(ionEnd);
        ionTail.lineTo(head - across);
        ionTail.close();
        const Color ion = CometColors::IonTail;
        fillShape(frame, ionTail, LinearGradient::Simple(head, ionEnd,
            ion.withOpacity(0.7f * tailFade), ion.withOpacity(0.0f)));

        // Sparks shed along the way drift down and fade.
        for (int i = 0; i != 26; ++i)
        {
            const float born = static_cast<float>(i) * 0.021f;
            const float age = (pass.passTime - born) / 0.3f;
            if (age <= 0.0f || age >= 1.0f || born > k_fallShare)
                continue;
            const FloatPoint origin = pointOnCurve(pass, std::pow(born / k_fallShare, 1.35f));
            const float sinking = 40.0f * unit * age * age;
            const FloatPoint spark = {
                origin.x + (pseudoRandom(i, 41.0f) - 0.5f) * 50.0f * unit * age,
                origin.y + (pseudoRandom(i, 42.0f) - 0.3f) * 30.0f * unit * age + sinking
            };
            const float phase = static_cast<float>(i);
            const float twinkle = 0.6f + 0.4f * std::sin(k_2Pi * 40.0f * cycleTime + phase);
            const Color color = i % 3 == 0 ? CometColors::ColdSpark : CometColors::WarmSpark;
            drawSparkle(frame, spark, (2.0f + 2.0f * pseudoRandom(i, 43.0f)) * unit * twinkle,
                color.withOpacity((1.0f - age) * 0.9f * tailFade));
        }

        if (pass.passTime < k_fallShare)
        {
            drawGlow(frame, head, 48.0f * unit, CometColors::Coma.withOpacity(0.55f));
            drawGlow(frame, head, 16.0f * unit, CometColors::Nucleus.withOpacity(0.95f));
            fillDisc(frame, head, 4.0f * unit, SolidColor{ CometColors::Nucleus });
        }
    }

    // The lake shows everything above it upside down and dimmed, with ripples running across.
    void CometScene::drawLake(const SceneFrame& frame, const float cycleTime, const float lakeY)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const FloatRect lake = { 0.0f, lakeY, width, height };
        fillBox(frame, lake, LinearGradient::Simple({ 0.0f, lakeY }, { 0.0f, height },
            CometColors::LakeTop, CometColors::LakeBottom));

        PixelPath lakeOutline;
        lakeOutline.moveTo(0.0f, lakeY);
        lakeOutline.lineTo(width, lakeY);
        lakeOutline.lineTo(width, height);
        lakeOutline.lineTo(0.0f, height);
        lakeOutline.close();
        frame.canvas.pushClip(lakeOutline, &frame.toCanvas);
        {
            const ScopedCanvasOpacity dimmed{ frame.canvas, k_reflectionOpacity };
            const SceneFrame mirrored = frame.within(Matrix3x2::Translation(0.0f, lakeY * 2.0f)
                * Matrix3x2::Scale(1.0f, -1.0f));
            drawAboveLake(mirrored, cycleTime, lakeY, true);
        }
        frame.canvas.popClip();
        fillBox(frame, lake, LinearGradient::Simple({ 0.0f, lakeY }, { 0.0f, height },
            CometColors::LakeShadeTop, CometColors::LakeShadeBottom));

        PixelPath ripples;
        for (int i = 0; i != 26; ++i)
        {
            const float depth = std::pow(pseudoRandom(i, 51.0f), 0.8f);
            const float y = lakeY + (height - lakeY) * depth;
            const float length = (30.0f + 70.0f * pseudoRandom(i, 52.0f)) * unit * (0.6f + depth);
            const float laps = static_cast<float>(1 + i % 3);
            const float x = fractionalPart(pseudoRandom(i, 53.0f) + cycleTime * laps)
                * (width + length * 2.0f) - length;
            ripples.moveTo(x, y);
            ripples.lineTo(x + length, y);
        }
        strokeShape(frame, ripples, SolidColor{ CometColors::Ripple }, 1.1f * unit,
            StrokeCap::Butt);

        // the shore, a line of pines
        fillBox(frame, { 0.0f, lakeY - 2.0f * unit, width, lakeY + 3.0f * unit },
            SolidColor{ CometColors::Shore });
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
            fillShape(frame, pine, SolidColor{ CometColors::Shore });
        }
    }
}
