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
        constexpr Color HareTail{ 0xFFFFFFFF };
        constexpr Color EarInside{ 0xFFF2A7A0 };
        constexpr Color HareEye{ 0xFF2A1B10 };
        constexpr Color HareNose{ 0xFFD9787A };
        constexpr Color Whisker{ 0x805B3A1E };
        constexpr Color Mouth{ 0xFF5A2A26 };
        constexpr Color Tooth{ 0xFFFFFFFF };
        constexpr Color ToothEdge{ 0xFFA89E94 };
    }

    // Drawn from the point between her feet, facing right; a negative facing mirrors her.
    void drawHare(const SceneFrame& frame, const HarePose& pose, const HareCoat& coat)
    {
        const SceneFrame hare = frame.within(Matrix3x2::Translation(pose.foot)
            * Matrix3x2::Scale(pose.size * pose.facing, pose.size));
        const float air = pose.air;

        fillOval(hare, { -12.0f, -12.0f }, 14.0f, 10.0f, SolidColor{ coat.shade },
            -0.3f);
        const FloatPoint hindFoot = {
            std::lerp(-14.0f, -30.0f, air) + 8.0f,
            std::lerp(-2.0f, -8.0f, air) - pose.footLift
        };
        fillOval(hare, hindFoot, 15.0f, 4.5f, SolidColor{ coat.shade },
            std::lerp(0.0f, -0.4f, air));

        const SceneFrame body = hare.within(Matrix3x2::Translation(-4.0f, -22.0f)
            * Matrix3x2::Rotation(pose.bodyTurn)
            * Matrix3x2::Scale(pose.stretch, 1.0f / pose.stretch));
        fillOval(body, {}, 26.0f, 15.0f, LinearGradient::Simple({ 0.0f, -15.0f }, { 0.0f, 15.0f },
            coat.light, coat.shade));
        fillOval(body, { 8.0f, 8.0f }, 14.0f, 6.0f, SolidColor{ coat.belly });
        fillDisc(hare, { -29.0f + pose.sitting * 8.0f, -26.0f - pose.sitting * 4.0f }, 7.0f,
            SolidColor{ HareColors::HareTail });

        // The forelegs, the far one first, their angles measured from straight down.
        const float reach = std::lerp(std::lerp(0.2f, -0.9f, air) + 0.4f * pose.sitting, 2.4f,
            pose.holding);
        const auto pawOf = [](FloatPoint shoulder, float angle, float length) {
            return shoulder + FloatPoint{ std::sin(angle), std::cos(angle) } * length;
        };
        drawSegment(hare, { 14.0f, -14.0f }, pawOf({ 14.0f, -14.0f }, reach, 15.0f),
            SolidColor{ coat.shade }, 5.0f);
        drawSegment(hare, { 18.0f, -14.0f }, pawOf({ 18.0f, -14.0f }, reach + 0.3f, 14.0f),
            SolidColor{ coat.fur }, 5.0f);

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
            fillOval(ear, { 0.0f, -17.0f }, 5.5f, 18.0f,
                SolidColor{ near ? coat.fur : coat.shade });
            fillOval(ear, { 0.0f, -17.0f }, 2.6f, 13.0f, SolidColor{ HareColors::EarInside });
        }
        fillDisc(hare, head, 12.0f, SolidColor{ coat.light });
        fillOval(hare, head + FloatPoint{ 9.0f, 3.0f }, 6.0f, 5.0f,
            SolidColor{ coat.light });
        fillDisc(hare, head + FloatPoint{ 4.0f, -3.0f }, 2.8f, SolidColor{ HareColors::HareEye });
        fillDisc(hare, head + FloatPoint{ 4.8f, -4.0f }, 0.9f, SolidColor{ HareColors::EyeWhite });
        fillOval(hare, head + FloatPoint{ 14.0f, 1.5f }, 2.0f, 1.6f,
            SolidColor{ HareColors::HareNose });
        if (pose.teeth > 0.0f)
        {
            // two front teeth under the nose, half a head long when bared
            const float toothLength = 6.5f * pose.teeth;
            fillOval(hare, head + FloatPoint{ 12.8f, 5.5f }, 3.4f, 1.0f + 1.6f * pose.teeth,
                SolidColor{ HareColors::Mouth });
            for (const float left : { 10.6f, 12.9f })
            {
                const FloatRect tooth = {
                    head.x + left,
                    head.y + 4.5f,
                    head.x + left + 2.2f,
                    head.y + 4.5f + toothLength
                };
                fillRoundedBox(hare, tooth, 0.7f, SolidColor{ HareColors::Tooth });
                PixelPath edge;
                edge.moveTo(tooth.left, tooth.top);
                edge.lineTo(tooth.left, tooth.bottom);
                edge.lineTo(tooth.right, tooth.bottom);
                edge.lineTo(tooth.right, tooth.top);
                strokeShape(hare, edge, SolidColor{ HareColors::ToothEdge }, 0.5f);
            }
        }
        PixelPath whiskers;
        whiskers.moveTo(head + FloatPoint{ 12.0f, 4.0f });
        whiskers.lineTo(head + FloatPoint{ 22.0f, 2.0f });
        whiskers.moveTo(head + FloatPoint{ 12.0f, 5.0f });
        whiskers.lineTo(head + FloatPoint{ 21.0f, 7.0f });
        strokeShape(hare, whiskers, SolidColor{ HareColors::Whisker }, 0.8f);
    }
}
