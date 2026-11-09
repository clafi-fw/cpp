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
        constexpr Color Tongue{ 0xFFEF6F7F };
        constexpr Color Mouth{ 0xFF5A1C28 };
        constexpr Color Tooth{ 0xFFFBF6E8 };
        constexpr Color ToothEdge{ 0xFF9C9488 };
        constexpr Color Ink{ 0xFF1B1D22 };
        constexpr Color EyeWhite{ 0xFFFFFFFF };
    }

    namespace
    {
        // A tooth on the edge of a jaw, in units of the wolf; `along` runs from the back of the
        // mouth to the front.
        struct Tooth
        {
            float along;
            float length;
            float width;
        };

        // A fang to each jaw, longer than the snout is deep, with small teeth behind it.
        constexpr std::array<Tooth, 3> k_upperTeeth{
            Tooth{ 0.3f, 7.0f, 4.5f },
            Tooth{ 0.56f, 8.0f, 4.5f },
            Tooth{ 0.88f, 19.0f, 7.5f }
        };
        constexpr std::array<Tooth, 3> k_lowerTeeth{
            Tooth{ 0.3f, 6.0f, 4.5f },
            Tooth{ 0.5f, 7.0f, 4.5f },
            Tooth{ 0.74f, 16.0f, 7.0f }
        };

        // Teeth along a jaw's edge from `back` to `front`, grown out to `growth` of their size.
        void drawTeeth(const SceneFrame&, FloatPoint back, FloatPoint front, bool upward,
            std::span<const Tooth> teeth, float growth);
    }

    // Drawn about the middle of his body, facing right; a leg's angles are measured from straight
    // down.
    void drawWolf(const SceneFrame& frame, const WolfPose& pose, const WolfCoat& coat)
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
            // In a leap the forepaws reach down ahead and the hind legs trail; sprawled, the legs
            // stretch out fore and aft; on his back they twitch in the air.
            upper = std::lerp(upper, leg.hind ? -1.25f : 1.15f, pose.leap);
            lower = std::lerp(lower, leg.hind ? -1.5f : 0.7f, pose.leap);
            upper = std::lerp(upper, leg.hind ? -1.45f : 1.4f, pose.sprawl);
            lower = std::lerp(lower, leg.hind ? -1.55f : 1.5f, pose.sprawl);
            const float twitch = 0.3f * std::sin(k_2Pi * 60.0f * pose.cycleTime + leg.phase * 3.0f);
            upper = std::lerp(upper, twitch, pose.onBack);
            lower = std::lerp(lower, twitch + 0.5f, pose.onBack);
            // Hanging, each leg falls straight down from its hip, fore and hind a little apart.
            upper = std::lerp(upper, leg.hind ? -0.12f : 0.12f, pose.dangle);
            lower = std::lerp(lower, leg.hind ? -0.05f : 0.05f, pose.dangle);
            const FloatPoint thigh = FloatPoint{ std::sin(upper), std::cos(upper) } * 22.0f;
            const FloatPoint shin = FloatPoint{ std::sin(lower), std::cos(lower) } * 22.0f;
            const FloatPoint knee = leg.hip + thigh;
            const FloatPoint paw = knee + shin;
            const SolidColor fur{ leg.far ? coat.far : coat.coat };
            drawSegment(wolf, leg.hip, knee, fur, 9.0f);
            drawSegment(wolf, knee, paw, fur, 7.0f);
            const SolidColor pad{ leg.far ? coat.dark : coat.far };
            fillOval(wolf, paw + FloatPoint{ 2.0f, 1.0f }, 6.5f, 3.6f, pad, -lower * 0.3f);
        };
        drawLeg(k_legs[0]);
        drawLeg(k_legs[1]);

        const float swish = std::sin(pose.stride)
            * std::max(0.0f, 1.0f - pose.leap - pose.sprawl - pose.onBack);
        const float wag = -0.15f + 0.35f * swish + 0.4f * pose.onBack - 1.1f * pose.dangle;
        const SceneFrame tail = wolf.within(Matrix3x2::Translation(-42.0f, -6.0f)
            * Matrix3x2::Rotation(wag));
        PixelPath brush;
        brush.moveTo(0.0f, -5.0f);
        brush.quadTo({ -22.0f, -20.0f }, { -42.0f, -10.0f });
        brush.quadTo({ -30.0f, 3.0f }, { 0.0f, 6.0f });
        brush.close();
        fillShape(tail, brush, SolidColor{ coat.coat });
        PixelPath tip;
        tip.moveTo(-30.0f, -15.0f);
        tip.quadTo({ -38.0f, -15.0f }, { -42.0f, -10.0f });
        tip.quadTo({ -37.0f, -5.0f }, { -31.0f, -4.0f });
        tip.close();
        fillShape(tail, tip, SolidColor{ coat.dark });

        PixelPath body;
        body.moveTo(-40.0f, -6.0f);
        body.cubicTo({ -30.0f, -22.0f }, { 20.0f, -26.0f }, { 36.0f, -12.0f });
        body.cubicTo({ 46.0f, -4.0f }, { 44.0f, 14.0f }, { 30.0f, 16.0f });
        body.cubicTo({ 10.0f, 22.0f }, { -20.0f, 20.0f }, { -34.0f, 14.0f });
        body.cubicTo({ -46.0f, 10.0f }, { -48.0f, -2.0f }, { -40.0f, -6.0f });
        body.close();
        fillShape(wolf, body, LinearGradient::Simple({ 0.0f, -24.0f }, { 0.0f, 20.0f },
            coat.light, coat.shade));
        PixelPath belly;
        belly.moveTo(-26.0f, 12.0f);
        belly.cubicTo({ -10.0f, 20.0f }, { 16.0f, 20.0f }, { 32.0f, 12.0f });
        belly.cubicTo({ 20.0f, 10.0f }, { -10.0f, 8.0f }, { -26.0f, 12.0f });
        belly.close();
        fillShape(wolf, belly, SolidColor{ coat.belly });
        drawLeg(k_legs[2]);
        drawLeg(k_legs[3]);

        const SceneFrame head = wolf.within(Matrix3x2::Translation(34.0f, -14.0f)
            * Matrix3x2::Rotation(pose.headTurn));
        PixelPath ears;
        ears.moveTo(4.0f, -20.0f);
        ears.lineTo(2.0f, -38.0f);
        ears.lineTo(14.0f, -24.0f);
        ears.close();
        fillShape(head, ears, SolidColor{ coat.shade });
        ears.clear();
        ears.moveTo(12.0f, -22.0f);
        ears.lineTo(14.0f, -40.0f);
        ears.lineTo(22.0f, -21.0f);
        ears.close();
        fillShape(head, ears, SolidColor{ coat.shade });
        fillOval(head, { 16.0f, -8.0f }, 16.0f, 13.0f, SolidColor{ coat.light });

        // Opening, the snout lifts and the lower jaw drops, both about the back of the mouth.
        const FloatPoint hinge = { 24.0f, 3.0f };
        const auto aboutHinge = [&](float angle) {
            return head.within(Matrix3x2::Translation(hinge) * Matrix3x2::Rotation(angle)
                * Matrix3x2::Translation(-hinge.x, -hinge.y));
        };
        const float snoutAngle = -0.45f * pose.jawOpen;
        const float jawAngle = 0.9f * pose.jawOpen;
        const SceneFrame snoutFrame = aboutHinge(snoutAngle);
        const SceneFrame jawFrame = aboutHinge(jawAngle);
        if (pose.jawOpen > 0.0f)
        {
            // The teeth grow out of the gums as the jaws part.
            const float growth = smoothStep(0.2f, 0.8f, pose.jawOpen);
            PixelPath mouth;
            mouth.moveTo(hinge - FloatPoint{ 3.0f, 0.0f });
            mouth.lineTo(hinge + rotated(FloatPoint{ 44.0f, -1.0f } - hinge, snoutAngle));
            mouth.lineTo(hinge + rotated(FloatPoint{ 42.0f, 2.0f } - hinge, jawAngle));
            mouth.close();
            fillShape(head, mouth, SolidColor{ WolfColors::Mouth });
            drawTeeth(snoutFrame, { 25.0f, 2.8f }, { 44.0f, -1.0f }, false, k_upperTeeth, growth);
            drawTeeth(jawFrame, { 25.0f, 3.0f }, { 42.0f, 2.0f }, true, k_lowerTeeth, growth);
        }
        PixelPath snout;
        snout.moveTo(22.0f, -16.0f);
        snout.lineTo(44.0f, -9.0f);
        snout.quadTo({ 49.0f, -5.0f }, { 44.0f, -1.0f });
        snout.lineTo(24.0f, 3.0f);
        snout.close();
        fillShape(snoutFrame, snout, SolidColor{ coat.light });
        // The lower jaw deepens as it drops, so it reads as a jaw under the teeth.
        PixelPath jaw;
        jaw.moveTo(24.0f, 3.0f);
        jaw.lineTo(42.0f, 2.0f);
        jaw.lineTo(42.0f - pose.jawOpen, 2.0f + 5.0f * pose.jawOpen);
        jaw.lineTo(26.0f + 3.0f * pose.jawOpen, 9.0f + 2.0f * pose.jawOpen);
        jaw.close();
        fillShape(jawFrame, jaw, SolidColor{ coat.shade });
        const SceneFrame tongue = jawFrame.within(Matrix3x2::Translation(33.0f, 3.0f)
            * Matrix3x2::Rotation(pose.tongueTurn));
        PixelPath lick;
        lick.moveTo(0.0f, 0.0f);
        lick.quadTo({ 7.0f, 10.0f }, { 3.0f, 18.0f });
        lick.quadTo({ -1.0f, 21.0f }, { -4.0f, 16.0f });
        lick.quadTo({ -3.0f, 8.0f }, { 0.0f, 0.0f });
        lick.close();
        fillShape(tongue, lick, SolidColor{ WolfColors::Tongue });
        fillOval(snoutFrame, { 45.0f, -6.0f }, 3.8f, 3.2f, SolidColor{ WolfColors::Ink });
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

    namespace
    {
        // A tooth stands from a root sunk into the gum and leans its tip toward the front.
        void drawTeeth(const SceneFrame& frame, const FloatPoint back, const FloatPoint front,
            const bool upward, const std::span<const Tooth> teeth, const float growth)
        {
            const FloatPoint along = front - back;
            const FloatPoint edge = along * (1.0f / std::hypot(along.x, along.y));
            const FloatPoint out = FloatPoint{ -edge.y, edge.x } * (upward ? -1.0f : 1.0f);
            for (const Tooth& tooth : teeth)
            {
                const FloatPoint root = back + along * tooth.along - out * 1.5f;
                const FloatPoint half = edge * (0.5f * tooth.width * growth);
                const float length = tooth.length * growth + 1.5f;
                PixelPath shape;
                shape.moveTo(root - half);
                shape.quadTo(root - half * 0.4f + out * (0.6f * length),
                    root + edge * (0.25f * tooth.width * growth) + out * length);
                shape.lineTo(root + half);
                shape.close();
                fillShape(frame, shape, SolidColor{ WolfColors::Tooth });
                strokeShape(frame, shape, SolidColor{ WolfColors::ToothEdge }, 0.7f);
            }
        }
    }
}
