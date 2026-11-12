module ClaFi.PathArt.HareSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace HareColors
    {
        constexpr Color EyeWhite{ 0xFFFFFFFF };
        constexpr Color HareEye{ 0xFF2A1B10 };
    }

    namespace
    {
        // The share of the way to black her shade goes where the light reaches her least.
        constexpr float k_unlitShare{ 0.25f };
    }

    // Drawn from the point between her feet, facing right; a negative facing mirrors her.
    void drawHare(const SceneFrame& frame, const HarePose& pose, const HareCoat& coat)
    {
        const SceneFrame hare = frame.within(Matrix3x2::Translation(pose.foot)
            * Matrix3x2::Scale(pose.size * pose.facing, pose.size));
        const float air = pose.air;
        // Lit from above like the wolf, each part darkens downward, at most to `unlit`.
        const Color unlit = colorBetween(coat.shade, Color{ 0xFF000000 }, k_unlitShare);
        const auto litFromAbove = [](float top, float bottom, Color upper, Color lower) {
            return LinearGradient::Simple({ 0.0f, top }, { 0.0f, bottom }, upper, lower);
        };

        fillOval(hare, { -12.0f, -12.0f }, 14.0f, 10.0f,
            litFromAbove(-10.0f, 10.0f, coat.shade, unlit), -0.3f);
        const FloatPoint hindFoot = {
            std::lerp(-14.0f, -30.0f, air) + 8.0f,
            std::lerp(-2.0f, -8.0f, air) - pose.footLift
        };
        fillOval(hare, hindFoot, 15.0f, 4.5f, litFromAbove(-4.5f, 4.5f, coat.shade, unlit),
            std::lerp(0.0f, -0.4f, air));

        const SceneFrame body = hare.within(Matrix3x2::Translation(-4.0f, -22.0f)
            * Matrix3x2::Rotation(pose.bodyTurn)
            * Matrix3x2::Scale(pose.stretch, 1.0f / pose.stretch));
        fillOval(body, {}, 26.0f, 15.0f, litFromAbove(-15.0f, 15.0f, coat.light, unlit));
        fillOval(body, { 8.0f, 8.0f }, 14.0f, 6.0f, SolidColor{ coat.belly });
        const FloatPoint tail = { -29.0f + pose.sitting * 8.0f, -26.0f - pose.sitting * 4.0f };
        fillDisc(hare, tail, 7.0f, LinearGradient::Simple(tail + FloatPoint{ 0.0f, -7.0f },
            tail + FloatPoint{ 0.0f, 7.0f }, coat.tail, colorBetween(coat.tail, coat.shade, 0.5f)));

        // The forelegs, the far one first, their angles measured from straight down.
        const float reach = std::lerp(std::lerp(0.2f, -0.9f, air) + 0.4f * pose.sitting, 2.4f,
            pose.holding);
        const auto pawOf = [](FloatPoint shoulder, float angle, float length) {
            return shoulder + FloatPoint{ std::sin(angle), std::cos(angle) } * length;
        };
        const FloatPoint farShoulder = { 14.0f, -14.0f };
        const FloatPoint nearShoulder = { 18.0f, -14.0f };
        const FloatPoint farPaw = pawOf(farShoulder, reach, 15.0f);
        const FloatPoint nearPaw = pawOf(nearShoulder, reach + 0.3f, 14.0f);
        drawSegment(hare, farShoulder, farPaw,
            LinearGradient::Simple(farShoulder, farPaw, coat.shade, unlit), 5.0f);
        drawSegment(hare, nearShoulder, nearPaw,
            LinearGradient::Simple(nearShoulder, nearPaw, coat.fur, coat.shade), 5.0f);

        const FloatPoint head = {
            std::lerp(24.0f, 12.0f, pose.sitting),
            std::lerp(-34.0f, -54.0f, pose.sitting)
        };
        for (const bool near : { false, true })
        {
            const float wiggle = pose.earWiggle * (near ? 1.0f : -1.0f);
            const float earTurn = pose.earTurn + (near ? 0.18f : 0.0f) + wiggle;
            const FloatPoint earRoot = { head.x - 4.0f + (near ? 5.0f : 0.0f), head.y - 8.0f };
            const SceneFrame ear = hare.within(Matrix3x2::Translation(earRoot)
                * Matrix3x2::Rotation(earTurn));
            fillOval(ear, { 0.0f, -17.0f }, 5.5f, 18.0f, litFromAbove(-18.0f, 18.0f,
                near ? coat.light : coat.shade, near ? coat.shade : unlit));
            fillOval(ear, { 0.0f, -17.0f }, 2.6f, 13.0f, SolidColor{ coat.earInside });
        }
        // The head and muzzle take one gradient, so they read as one piece. The muzzle is a path:
        // fillOval reads its brush about the oval's own middle.
        const LinearGradient headLight = LinearGradient::Simple(head + FloatPoint{ 0.0f, -12.0f },
            head + FloatPoint{ 0.0f, 12.0f }, coat.light, coat.shade);
        fillDisc(hare, head, 12.0f, headLight);
        PixelPath muzzle;
        muzzle.drawEllipse(head + FloatPoint{ 9.0f, 3.0f }, 6.0f, 5.0f);
        fillShape(hare, muzzle, headLight);
        fillDisc(hare, head + FloatPoint{ 4.0f, -3.0f }, 2.8f, SolidColor{ HareColors::HareEye });
        fillDisc(hare, head + FloatPoint{ 4.8f, -4.0f }, 0.9f, SolidColor{ HareColors::EyeWhite });
        fillOval(hare, head + FloatPoint{ 14.0f, 1.5f }, 2.0f, 1.6f,
            SolidColor{ coat.nose });
        if (pose.teeth > 0.0f)
        {
            // two front teeth under the nose, half a head long when bared
            const float toothLength = 6.5f * pose.teeth;
            fillOval(hare, head + FloatPoint{ 12.8f, 5.5f }, 3.4f, 1.0f + 1.6f * pose.teeth,
                SolidColor{ coat.mouth });
            for (const float left : { 10.6f, 12.9f })
            {
                const FloatRect tooth = {
                    head.x + left,
                    head.y + 4.5f,
                    head.x + left + 2.2f,
                    head.y + 4.5f + toothLength
                };
                fillRoundedBox(hare, tooth, 0.7f, SolidColor{ coat.tooth });
                PixelPath edge;
                edge.moveTo(tooth.left, tooth.top);
                edge.lineTo(tooth.left, tooth.bottom);
                edge.lineTo(tooth.right, tooth.bottom);
                edge.lineTo(tooth.right, tooth.top);
                strokeShape(hare, edge, SolidColor{ coat.toothEdge }, 0.5f);
            }
        }
        PixelPath whiskers;
        whiskers.moveTo(head + FloatPoint{ 12.0f, 4.0f });
        whiskers.lineTo(head + FloatPoint{ 22.0f, 2.0f });
        whiskers.moveTo(head + FloatPoint{ 12.0f, 5.0f });
        whiskers.lineTo(head + FloatPoint{ 21.0f, 7.0f });
        strokeShape(hare, whiskers, SolidColor{ coat.whisker }, 0.8f);
    }
}
