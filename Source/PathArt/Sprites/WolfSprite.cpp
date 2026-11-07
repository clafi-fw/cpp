module ClaFi.PathArt.WolfSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace WolfColors
    {
        constexpr Color WolfLight{ 0xFF7D8592 };
        constexpr Color WolfShade{ 0xFF59606B };
        constexpr Color WolfCoat{ 0xFF6C7480 };
        constexpr Color WolfFar{ 0xFF4F565F };
        constexpr Color WolfDark{ 0xFF3D434C };
        constexpr Color WolfBelly{ 0xE6B3B9C2 };
        constexpr Color Tongue{ 0xFFEF6F7F };
        constexpr Color Ink{ 0xFF1B1D22 };
        constexpr Color EyeWhite{ 0xFFFFFFFF };
    }

    // Drawn about the middle of his body, facing right; a leg's angles are measured from straight
    // down.
    void drawWolf(const SceneFrame& frame, const WolfPose& pose)
    {
        const SceneFrame wolf = frame.within(Matrix3x2::Translation(pose.center)
            * Matrix3x2::Rotation(pose.turn) * Matrix3x2::Scale(pose.size));

        struct Leg
        {
            FloatPoint hip;
            bool far;
            float phase;
            bool hind;
        };
        constexpr std::array<Leg, 4> k_legs{
            Leg{ { -30.0f, 4.0f }, true, std::numbers::pi_v<float> + 0.5f, true },
            Leg{ { 22.0f, 4.0f }, true, 0.5f, false },
            Leg{ { -24.0f, 7.0f }, false, std::numbers::pi_v<float>, true },
            Leg{ { 28.0f, 7.0f }, false, 0.0f, false }
        };
        const auto drawLeg = [&](const Leg& leg) {
            const float phase = pose.stride + leg.phase;
            float upper = (leg.hind ? -0.1f : 0.15f) + 0.7f * std::sin(phase);
            float lower = leg.hind
                ? upper + 0.35f + 0.5f * std::max(0.0f, -std::cos(phase))
                : upper - 0.3f - 0.55f * std::max(0.0f, std::cos(phase));
            // Sprawled, the legs stretch out fore and aft; on his back they twitch in the air.
            upper = std::lerp(upper, leg.hind ? -1.45f : 1.4f, pose.sprawl);
            lower = std::lerp(lower, leg.hind ? -1.55f : 1.5f, pose.sprawl);
            const float twitch = 0.3f * std::sin(k_2Pi * 60.0f * pose.cycleTime + leg.phase * 3.0f);
            upper = std::lerp(upper, twitch, pose.onBack);
            lower = std::lerp(lower, twitch + 0.5f, pose.onBack);
            const FloatPoint thigh = FloatPoint{ std::sin(upper), std::cos(upper) } * 22.0f;
            const FloatPoint shin = FloatPoint{ std::sin(lower), std::cos(lower) } * 22.0f;
            const FloatPoint knee = leg.hip + thigh;
            const FloatPoint paw = knee + shin;
            const SolidColor coat{ leg.far ? WolfColors::WolfFar : WolfColors::WolfCoat };
            drawSegment(wolf, leg.hip, knee, coat, 9.0f);
            drawSegment(wolf, knee, paw, coat, 7.0f);
            const SolidColor pad{ leg.far ? WolfColors::WolfDark : WolfColors::WolfFar };
            fillOval(wolf, paw + FloatPoint{ 2.0f, 1.0f }, 6.5f, 3.6f, pad, -lower * 0.3f);
        };
        drawLeg(k_legs[0]);
        drawLeg(k_legs[1]);

        const float swish = std::sin(pose.stride) * (1.0f - pose.sprawl - pose.onBack);
        const float wag = -0.15f + 0.35f * swish + 0.4f * pose.onBack;
        const SceneFrame tail = wolf.within(Matrix3x2::Translation(-42.0f, -6.0f)
            * Matrix3x2::Rotation(wag));
        PixelPath brush;
        brush.moveTo(0.0f, -5.0f);
        brush.quadTo({ -22.0f, -20.0f }, { -42.0f, -10.0f });
        brush.quadTo({ -30.0f, 3.0f }, { 0.0f, 6.0f });
        brush.close();
        fillShape(tail, brush, SolidColor{ WolfColors::WolfCoat });
        PixelPath tip;
        tip.moveTo(-30.0f, -15.0f);
        tip.quadTo({ -38.0f, -15.0f }, { -42.0f, -10.0f });
        tip.quadTo({ -37.0f, -5.0f }, { -31.0f, -4.0f });
        tip.close();
        fillShape(tail, tip, SolidColor{ WolfColors::WolfDark });

        PixelPath body;
        body.moveTo(-40.0f, -6.0f);
        body.cubicTo({ -30.0f, -22.0f }, { 20.0f, -26.0f }, { 36.0f, -12.0f });
        body.cubicTo({ 46.0f, -4.0f }, { 44.0f, 14.0f }, { 30.0f, 16.0f });
        body.cubicTo({ 10.0f, 22.0f }, { -20.0f, 20.0f }, { -34.0f, 14.0f });
        body.cubicTo({ -46.0f, 10.0f }, { -48.0f, -2.0f }, { -40.0f, -6.0f });
        body.close();
        fillShape(wolf, body, LinearGradient::Simple({ 0.0f, -24.0f }, { 0.0f, 20.0f },
            WolfColors::WolfLight, WolfColors::WolfShade));
        PixelPath belly;
        belly.moveTo(-26.0f, 12.0f);
        belly.cubicTo({ -10.0f, 20.0f }, { 16.0f, 20.0f }, { 32.0f, 12.0f });
        belly.cubicTo({ 20.0f, 10.0f }, { -10.0f, 8.0f }, { -26.0f, 12.0f });
        belly.close();
        fillShape(wolf, belly, SolidColor{ WolfColors::WolfBelly });
        drawLeg(k_legs[2]);
        drawLeg(k_legs[3]);

        const SceneFrame head = wolf.within(Matrix3x2::Translation(34.0f, -14.0f)
            * Matrix3x2::Rotation(pose.headTurn));
        PixelPath ears;
        ears.moveTo(4.0f, -20.0f);
        ears.lineTo(2.0f, -38.0f);
        ears.lineTo(14.0f, -24.0f);
        ears.close();
        fillShape(head, ears, SolidColor{ WolfColors::WolfShade });
        ears.clear();
        ears.moveTo(12.0f, -22.0f);
        ears.lineTo(14.0f, -40.0f);
        ears.lineTo(22.0f, -21.0f);
        ears.close();
        fillShape(head, ears, SolidColor{ WolfColors::WolfShade });
        fillOval(head, { 16.0f, -8.0f }, 16.0f, 13.0f, SolidColor{ WolfColors::WolfLight });
        PixelPath snout;
        snout.moveTo(22.0f, -16.0f);
        snout.lineTo(44.0f, -9.0f);
        snout.quadTo({ 49.0f, -5.0f }, { 44.0f, -1.0f });
        snout.lineTo(24.0f, 3.0f);
        snout.close();
        fillShape(head, snout, SolidColor{ WolfColors::WolfLight });
        PixelPath jaw;
        jaw.moveTo(24.0f, 3.0f);
        jaw.lineTo(42.0f, 2.0f);
        jaw.lineTo(26.0f, 9.0f);
        jaw.close();
        fillShape(head, jaw, SolidColor{ WolfColors::WolfShade });
        const SceneFrame tongue = head.within(Matrix3x2::Translation(33.0f, 3.0f)
            * Matrix3x2::Rotation(pose.tongueTurn));
        PixelPath lick;
        lick.moveTo(0.0f, 0.0f);
        lick.quadTo({ 7.0f, 10.0f }, { 3.0f, 18.0f });
        lick.quadTo({ -1.0f, 21.0f }, { -4.0f, 16.0f });
        lick.quadTo({ -3.0f, 8.0f }, { 0.0f, 0.0f });
        lick.close();
        fillShape(tongue, lick, SolidColor{ WolfColors::Tongue });
        fillOval(head, { 45.0f, -6.0f }, 3.8f, 3.2f, SolidColor{ WolfColors::Ink });
        if (pose.dazed > 0.5f)
        {
            PixelPath crossed;
            crossed.moveTo(16.0f, -16.0f);
            crossed.lineTo(24.0f, -8.0f);
            crossed.moveTo(24.0f, -16.0f);
            crossed.lineTo(16.0f, -8.0f);
            strokeShape(head, crossed, SolidColor{ WolfColors::Ink }, 2.2f);
        }
        else
        {
            fillDisc(head, { 20.0f, -12.0f }, 5.5f, SolidColor{ WolfColors::EyeWhite });
            fillDisc(head, { 22.5f, -12.0f + pose.glance }, 2.8f, SolidColor{ WolfColors::Ink });
        }
    }
}
