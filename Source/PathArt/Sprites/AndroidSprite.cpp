module ClaFi.PathArt.AndroidSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace AndroidColors
    {
        constexpr Color MetalLight{ 0xFFEEF2F5 };
        constexpr Color MetalShade{ 0xFF97A3AF };
        constexpr Color MetalEdge{ 0xFF3B4450 };
        constexpr Color Limb{ 0xFF8793A0 };
        constexpr Color Joint{ 0xFF4B5563 };
        constexpr Color Foot{ 0xFF5B6573 };
        constexpr Color Pelvis{ 0xFF7D8995 };
        constexpr Color Panel{ 0xFF1D2430 };
        constexpr Color Neck{ 0xFF6B7684 };
        constexpr Color HeadShade{ 0xFFA3AEB9 };
        constexpr Color HeadLight{ 0xFFF3F6F8 };
        constexpr Color Visor{ 0xFF0F1420 };
        constexpr Color Eye{ 0xFF7AF4FF };
        constexpr Color EyeGlow{ 0xFF5EF0FF };
        constexpr Color AntennaOn{ 0xFFFF5A5A };
        constexpr Color AntennaOff{ 0xFF7A2A2A };
        constexpr Color AntennaGlow{ 0xFFFF4D4D };
        constexpr Color Hand{ 0xFFA9B3BD };
        constexpr std::array<Color, 3> ChestLights{ Color{ 0xFF5EF0FF }, Color{ 0xFFFFD166 },
            Color{ 0xFFFF6B8B } };
    }

    namespace
    {
        // The left arm, from its shoulder to the hand on the grip.
        struct HoldingArm
        {
            FloatPoint shoulder;
            Limb limb;
        };

        // How far the body dips: down on each step, two steps a sway. The feet keep still.
        [[nodiscard]] float bobAt(const float unit, const float sway)
        {
            return -2.5f * unit * std::abs(std::sin(2.0f * sway));
        }

        [[nodiscard]] HoldingArm holdingArmOf(const float unit, const AndroidPose& pose)
        {
            const FloatPoint shoulder = {
                pose.feet.x - 47.0f * unit,
                pose.feet.y - 218.0f * unit + bobAt(unit, pose.sway)
            };
            return {
                .shoulder = shoulder,
                .limb = reachToward(shoulder, pose.grip, 87.0f * unit, 87.0f * unit,
                    BendToward::Down)
            };
        }
    }

    void drawAndroid(const SceneFrame& frame, const AndroidPose& pose)
    {
        const float unit = frame.unit;
        const float androidX = pose.feet.x;
        const float floorY = pose.feet.y;
        const float sway = pose.sway;
        const float cycleTime = pose.cycleTime;
        const float bob = bobAt(unit, sway);
        // Places on the body, in units from its middle and from the floor; only the feet keep
        // still.
        const auto bodyX = [&](float units) {
            return androidX + units * unit;
        };
        const auto bodyY = [&](float units) {
            return floorY + units * unit + bob;
        };
        const auto footY = [&](float units) {
            return floorY + units * unit;
        };
        const SolidColor limb{ AndroidColors::Limb };
        const SolidColor joint{ AndroidColors::Joint };

        // legs and feet
        drawSegment(frame, { bodyX(-17.0f), bodyY(-108.0f) }, { bodyX(-19.0f), footY(-14.0f) },
            limb, 16.0f * unit);
        drawSegment(frame, { bodyX(17.0f), bodyY(-108.0f) }, { bodyX(19.0f), footY(-14.0f) }, limb,
            16.0f * unit);
        fillDisc(frame, { bodyX(-18.0f), footY(-60.0f) + bob * 0.5f }, 9.0f * unit, joint);
        fillDisc(frame, { bodyX(18.0f), footY(-60.0f) + bob * 0.5f }, 9.0f * unit, joint);
        const SolidColor foot{ AndroidColors::Foot };
        fillRoundedBox(frame, { bodyX(-38.0f), footY(-17.0f), bodyX(-6.0f), floorY }, 6.0f * unit,
            foot);
        fillRoundedBox(frame, { bodyX(6.0f), footY(-17.0f), bodyX(38.0f), floorY }, 6.0f * unit,
            foot);

        // pelvis, torso and the chest panel
        fillRoundedBox(frame, { bodyX(-30.0f), bodyY(-130.0f), bodyX(30.0f), bodyY(-104.0f) },
            8.0f * unit, SolidColor{ AndroidColors::Pelvis });
        PixelPath torso;
        const FloatRect torsoBox = { bodyX(-42.0f), bodyY(-234.0f), bodyX(42.0f), bodyY(-126.0f) };
        torso.drawRoundedRect(torsoBox, 18.0f * unit, 18.0f * unit);
        fillShape(frame, torso, LinearGradient::Simple({ bodyX(-40.0f), bodyY(-232.0f) },
            { bodyX(40.0f), bodyY(-120.0f) }, AndroidColors::MetalLight,
            AndroidColors::MetalShade));
        strokeShape(frame, torso, SolidColor{ AndroidColors::MetalEdge }, 1.6f * unit);
        fillRoundedBox(frame, { bodyX(-24.0f), bodyY(-206.0f), bodyX(24.0f), bodyY(-166.0f) },
            7.0f * unit, SolidColor{ AndroidColors::Panel });
        // The chest lights chase the beat, one lit at a time.
        const int litLight = static_cast<int>(fractionalPart(cycleTime * 32.0f) * 3.0f);
        for (int i = 0; i != 3; ++i)
        {
            const FloatPoint light = { bodyX(static_cast<float>(i - 1) * 13.0f), bodyY(-186.0f) };
            const Color color = AndroidColors::ChestLights[static_cast<std::size_t>(i)];
            const bool lit = i == litLight;
            if (lit)
                drawGlow(frame, light, 12.0f * unit, color.withOpacity(0.6f));
            fillDisc(frame, light, 4.2f * unit,
                SolidColor{ color.withOpacity(lit ? 1.0f : 0.28f) });
        }

        // neck and the head, turned toward the puppet
        fillBox(frame, { bodyX(-8.0f), bodyY(-250.0f), bodyX(8.0f), bodyY(-232.0f) },
            SolidColor{ AndroidColors::Neck });
        const SceneFrame head = frame.within(Matrix3x2::Translation(androidX, bodyY(-246.0f))
            * Matrix3x2::Rotation(-0.1f + 0.06f * std::sin(sway)));
        fillDisc(head, { -38.0f * unit, -28.0f * unit }, 8.0f * unit, joint);
        fillDisc(head, { 38.0f * unit, -28.0f * unit }, 8.0f * unit, joint);
        PixelPath skull;
        skull.drawRoundedRect(FloatRect{ -36.0f * unit, -58.0f * unit, 36.0f * unit, 0.0f },
            22.0f * unit, 22.0f * unit);
        fillShape(head, skull, LinearGradient::Simple({ -30.0f * unit, -58.0f * unit },
            { 30.0f * unit, 0.0f }, AndroidColors::HeadLight, AndroidColors::HeadShade));
        strokeShape(head, skull, SolidColor{ AndroidColors::MetalEdge }, 1.6f * unit);
        fillRoundedBox(head, { -28.0f * unit, -46.0f * unit, 28.0f * unit, -21.0f * unit },
            10.0f * unit, SolidColor{ AndroidColors::Visor });
        const float blink = fractionalPart(cycleTime * 4.0f + 0.2f) > 0.975f ? 0.15f : 1.0f;
        const float look = -5.0f * unit + 2.0f * unit * std::sin(sway);
        const float eyeY = -34.0f * unit;
        const float eyeHeight = 12.0f * unit * blink;
        for (const float eyeLeft : { -19.0f, 5.0f })
        {
            const float left = eyeLeft * unit + look;
            drawGlow(head, { left + 7.0f * unit, eyeY }, 13.0f * unit,
                AndroidColors::EyeGlow.withOpacity(0.35f));
            const FloatRect eye = {
                left,
                eyeY - eyeHeight * 0.5f,
                left + 14.0f * unit,
                eyeY + eyeHeight * 0.5f
            };
            fillRoundedBox(head, eye, 4.0f * unit * blink, SolidColor{ AndroidColors::Eye });
        }
        PixelPath grille;
        for (int i = 0; i != 3; ++i)
        {
            const float x = static_cast<float>(-9 + i * 9) * unit;
            grille.moveTo(x, -12.0f * unit);
            grille.lineTo(x, -6.0f * unit);
        }
        strokeShape(head, grille, SolidColor{ AndroidColors::Neck }, 2.4f * unit);
        drawSegment(head, { 0.0f, -58.0f * unit }, { 0.0f, -78.0f * unit },
            SolidColor{ AndroidColors::Neck }, 3.0f * unit);
        const bool antennaLit = fractionalPart(cycleTime * 16.0f) < 0.5f;
        if (antennaLit)
            drawGlow(head, { 0.0f, -83.0f * unit }, 14.0f * unit,
                AndroidColors::AntennaGlow.withOpacity(0.6f));
        fillDisc(head, { 0.0f, -83.0f * unit }, 5.0f * unit,
            SolidColor{ antennaLit ? AndroidColors::AntennaOn : AndroidColors::AntennaOff });

        // the right arm, its hand on the hip
        const FloatPoint shoulder = { bodyX(47.0f), bodyY(-218.0f) };
        const FloatPoint elbow = { bodyX(80.0f), bodyY(-172.0f) };
        const FloatPoint hand = { bodyX(44.0f), bodyY(-136.0f) };
        drawSegment(frame, shoulder, elbow, limb, 13.0f * unit);
        drawSegment(frame, elbow, hand, limb, 12.0f * unit);
        fillDisc(frame, elbow, 7.5f * unit, joint);
        fillDisc(frame, hand, 9.0f * unit, SolidColor{ AndroidColors::Hand });
        fillDisc(frame, shoulder, 11.0f * unit, joint);

        // the left arm's upper part, behind what the hand holds
        const HoldingArm arm = holdingArmOf(unit, pose);
        drawSegment(frame, arm.shoulder, arm.limb.joint, limb, 13.0f * unit);
        fillDisc(frame, arm.shoulder, 11.0f * unit, joint);
    }

    void drawAndroidHand(const SceneFrame& frame, const AndroidPose& pose)
    {
        const float unit = frame.unit;
        const HoldingArm arm = holdingArmOf(unit, pose);
        drawSegment(frame, arm.limb.joint, arm.limb.end, SolidColor{ AndroidColors::Limb },
            12.0f * unit);
        fillDisc(frame, arm.limb.joint, 7.5f * unit, SolidColor{ AndroidColors::Joint });
        const FloatPoint grip = pose.grip;
        const FloatRect hand = {
            grip.x - 10.0f * unit,
            grip.y - 6.0f * unit,
            grip.x + 10.0f * unit,
            grip.y + 10.0f * unit
        };
        PixelPath handPath;
        handPath.drawRoundedRect(hand, 6.0f * unit, 6.0f * unit);
        fillShape(frame, handPath, SolidColor{ AndroidColors::Hand });
        strokeShape(frame, handPath, SolidColor{ AndroidColors::MetalEdge }, 1.2f * unit);
    }
}
