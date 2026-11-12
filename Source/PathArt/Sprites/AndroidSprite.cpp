module ClaFi.PathArt.AndroidSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.TopHatSprite;
import ClaFi.PathArt.TutuSprite;
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
        constexpr Color BackLight{ 0xFFC9D1D8 };
        constexpr Color BackShade{ 0xFF7D8995 };
        constexpr Color Vent{ 0xFF5D6874 };
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
        constexpr Color Satin{ 0xFFF4C0CF };
        constexpr Color SatinShine{ 0x80FFFFFF };
        constexpr Color Ribbon{ 0xFFF39BB6 };
        constexpr std::array<Color, 3> ChestLights{ Color{ 0xFF5EF0FF }, Color{ 0xFFFFD166 },
            Color{ 0xFFFF6B8B } };
    }

    namespace
    {
        // The hat sits on the skull at a slant, in the head's units.
        constexpr TopHatPose k_wornHat{
            .brim = { 6.0f, -52.0f },
            .turn = 0.14f,
            .size = 0.92f
        };

        // How far a turn may squeeze the figure, so it never folds to nothing.
        constexpr float k_thinnestFacing{ 0.03f };
        constexpr int k_pinkLight{ 2 };

        [[nodiscard]] float sideSign(AudienceSide);
        [[nodiscard]] float drawnFacing(const AndroidPose&);
        // Bowing toward the audience, the upper body keeps this share of its height.
        [[nodiscard]] float upperHeightShare(const AndroidPose&);
        // The figure's frame in units about the pelvis.
        [[nodiscard]] SceneFrame bodyFrameOf(const SceneFrame&, const AndroidPose&);
        // A two-part limb that stretches rather than stops short of its end.
        [[nodiscard]] Limb stretchToward(FloatPoint root, FloatPoint target, float upperLength,
            float lowerLength, BendToward);
        [[nodiscard]] Limb armOf(const AndroidPose&, AudienceSide);
        [[nodiscard]] FloatPoint bodyToUpper(const AndroidPose&, FloatPoint bodyPoint);
        [[nodiscard]] FloatPoint upperToBody(const AndroidPose&, FloatPoint upperPoint);

        void drawLegs(const SceneFrame& body, const AndroidPose&);
        void drawPointeRibbons(const SceneFrame& body, const Limb& leg);
        void drawTorso(const SceneFrame& upper, const AndroidPose&);
        void drawHead(const SceneFrame& upper, const AndroidPose&);
        void drawForearm(const SceneFrame& upper, const AndroidPose&, AudienceSide);
    }

    // AndroidPose

    AndroidSide& AndroidPose::side(const AudienceSide which)
    {
        return which == AudienceSide::Left ? left : right;
    }

    const AndroidSide& AndroidPose::side(const AudienceSide which) const
    {
        return which == AudienceSide::Left ? left : right;
    }

    AndroidPose standingAndroid(const SceneFrame& frame, const FloatPoint floorPoint,
        const float cycleTime, const float dip, const float stance)
    {
        const float unit = frame.unit;
        AndroidPose pose{
            .pelvis = { floorPoint.x, floorPoint.y - (AndroidBody::StandingHeight - dip) * unit },
            .left = { .hand = { -44.0f, -19.0f } },
            .right = { .hand = { 44.0f, -19.0f } },
            .cycleTime = cycleTime
        };
        const float ankleY = floorPoint.y - AndroidBody::AnkleHeight * unit;
        plantAndroidFeet(frame, pose, { floorPoint.x - stance * unit, ankleY },
            { floorPoint.x + stance * unit, ankleY });
        return pose;
    }

    FloatPoint androidBodyToFrame(const SceneFrame& frame, const AndroidPose& pose,
        const FloatPoint bodyPoint)
    {
        const float unit = frame.unit;
        const FloatPoint scaled = { bodyPoint.x * drawnFacing(pose) * unit, bodyPoint.y * unit };
        return pose.pelvis + rotated(scaled, pose.tilt);
    }

    FloatPoint androidFrameToBody(const SceneFrame& frame, const AndroidPose& pose,
        const FloatPoint framePoint)
    {
        const float unit = frame.unit;
        const FloatPoint scaled = rotated(framePoint - pose.pelvis, -pose.tilt);
        return { scaled.x / (drawnFacing(pose) * unit), scaled.y / unit };
    }

    FloatPoint androidUpperToFrame(const SceneFrame& frame, const AndroidPose& pose,
        const FloatPoint upperPoint)
    {
        return androidBodyToFrame(frame, pose, upperToBody(pose, upperPoint));
    }

    FloatPoint androidFrameToUpper(const SceneFrame& frame, const AndroidPose& pose,
        const FloatPoint framePoint)
    {
        return bodyToUpper(pose, androidFrameToBody(frame, pose, framePoint));
    }

    SceneFrame androidUpperFrame(const SceneFrame& frame, const AndroidPose& pose)
    {
        return bodyFrameOf(frame, pose).within(Matrix3x2::Translation(0.0f, AndroidBody::Waist)
            * Matrix3x2::Rotation(pose.bend) * Matrix3x2::Scale(1.0f, upperHeightShare(pose))
            * Matrix3x2::Translation(0.0f, -AndroidBody::Waist));
    }

    void plantAndroidFeet(const SceneFrame& frame, AndroidPose& pose, const FloatPoint leftAnkle,
        const FloatPoint rightAnkle)
    {
        pose.left.foot = androidFrameToBody(frame, pose, leftAnkle);
        pose.right.foot = androidFrameToBody(frame, pose, rightAnkle);
    }

    // A hand reaches the floor by the edge of its palm, a foot by its sole either side of the
    // ankle.
    void restAndroidOnFloor(const SceneFrame& frame, AndroidPose& pose, const float floorY)
    {
        constexpr float k_soleHalf{ 16.0f };
        constexpr float k_palmRadius{ 9.0f };
        const float facing = drawnFacing(pose);
        float lowest = -std::numeric_limits<float>::max();
        const auto reachDown = [&](FloatPoint bodyPoint, float radius) {
            const FloatPoint turned = rotated({ bodyPoint.x * facing, bodyPoint.y }, pose.tilt);
            lowest = std::max(lowest, turned.y + radius);
        };
        for (const AndroidSide* side : { &pose.left, &pose.right })
        {
            const float soleY = side->foot.y + AndroidBody::AnkleHeight;
            reachDown({ side->foot.x - k_soleHalf, soleY }, 0.0f);
            reachDown({ side->foot.x + k_soleHalf, soleY }, 0.0f);
            reachDown(upperToBody(pose, side->hand), k_palmRadius);
        }
        reachDown(upperToBody(pose, AndroidBody::AntennaTip), 0.0f);
        pose.pelvis.y = floorY - lowest * frame.unit;
    }

    void drawAndroid(const SceneFrame& frame, const AndroidPose& pose)
    {
        const SceneFrame body = bodyFrameOf(frame, pose);
        drawLegs(body, pose);
        fillRoundedBox(body, { -30.0f, -13.0f, 30.0f, 13.0f }, 8.0f,
            SolidColor{ AndroidColors::Pelvis });
        // A tutu looks the same from every side, so it keeps its width as the figure turns.
        if (pose.wearsTutu)
        {
            const SceneFrame hips = frame.within(Matrix3x2::Translation(pose.pelvis)
                * Matrix3x2::Rotation(pose.tilt) * Matrix3x2::Scale(frame.unit));
            drawTutu(hips, pose.tutuTurn);
        }
        const SceneFrame upper = androidUpperFrame(frame, pose);
        drawTorso(upper, pose);
        drawHead(upper, pose);
        for (const AudienceSide which : { AudienceSide::Left, AudienceSide::Right })
        {
            const FloatPoint shoulder = {
                sideSign(which) * AndroidBody::RightShoulder.x,
                AndroidBody::RightShoulder.y
            };
            const Limb arm = armOf(pose, which);
            drawSegment(upper, shoulder, arm.joint, SolidColor{ AndroidColors::Limb }, 13.0f);
            if (!pose.side(which).forearmOnTop)
                drawForearm(upper, pose, which);
            fillDisc(upper, shoulder, 11.0f, SolidColor{ AndroidColors::Joint });
        }
    }

    void drawAndroidForearm(const SceneFrame& frame, const AndroidPose& pose,
        const AudienceSide which)
    {
        drawForearm(androidUpperFrame(frame, pose), pose, which);
    }

    namespace
    {
        float sideSign(const AudienceSide which)
        {
            return which == AudienceSide::Left ? -1.0f : 1.0f;
        }

        float drawnFacing(const AndroidPose& pose)
        {
            const float sign = pose.facing < 0.0f ? -1.0f : 1.0f;
            return sign * std::max(k_thinnestFacing, std::abs(pose.facing));
        }

        float upperHeightShare(const AndroidPose& pose)
        {
            return 1.0f - 0.3f * pose.bow;
        }

        SceneFrame bodyFrameOf(const SceneFrame& frame, const AndroidPose& pose)
        {
            return frame.within(Matrix3x2::Translation(pose.pelvis)
                * Matrix3x2::Rotation(pose.tilt)
                * Matrix3x2::Scale(drawnFacing(pose) * frame.unit, frame.unit));
        }

        Limb stretchToward(const FloatPoint root, const FloatPoint target, const float upperLength,
            const float lowerLength, const BendToward bendToward)
        {
            const FloatPoint toTarget = target - root;
            const float distance = std::hypot(toTarget.x, toTarget.y);
            const float growth = std::max(1.0f, distance / ((upperLength + lowerLength) * 0.998f));
            return reachToward(root, target, upperLength * growth, lowerLength * growth,
                bendToward);
        }

        Limb armOf(const AndroidPose& pose, const AudienceSide which)
        {
            const AndroidSide& side = pose.side(which);
            const FloatPoint shoulder = {
                sideSign(which) * AndroidBody::RightShoulder.x,
                AndroidBody::RightShoulder.y
            };
            BendToward bend = which == AudienceSide::Left ? BendToward::Left : BendToward::Right;
            if (side.elbow == ElbowBend::Down)
                bend = BendToward::Down;
            return stretchToward(shoulder, side.hand, side.upperArm, side.forearm, bend);
        }

        FloatPoint bodyToUpper(const AndroidPose& pose, const FloatPoint bodyPoint)
        {
            const FloatPoint unbent = rotated({ bodyPoint.x, bodyPoint.y - AndroidBody::Waist },
                -pose.bend);
            return { unbent.x, unbent.y / upperHeightShare(pose) + AndroidBody::Waist };
        }

        FloatPoint upperToBody(const AndroidPose& pose, const FloatPoint upperPoint)
        {
            const FloatPoint shortened = {
                upperPoint.x,
                (upperPoint.y - AndroidBody::Waist) * upperHeightShare(pose),
            };
            const FloatPoint bent = rotated(shortened, pose.bend);
            return { bent.x, bent.y + AndroidBody::Waist };
        }

        // Kneeling, a thigh comes down to its knee on the floor and the shin lies behind it,
        // seen end on.
        void drawLegs(const SceneFrame& body, const AndroidPose& pose)
        {
            const SolidColor limb{ AndroidColors::Limb };
            const SolidColor joint{ AndroidColors::Joint };
            const SolidColor foot = pose.wearsPointeShoes
                ? SolidColor{ AndroidColors::Satin }
                : SolidColor{ AndroidColors::Foot };
            for (const AudienceSide which : { AudienceSide::Left, AudienceSide::Right })
            {
                const float sign = sideSign(which);
                const AndroidSide& side = pose.side(which);
                const FloatPoint hip = { sign * AndroidBody::RightHip.x, AndroidBody::RightHip.y };
                if (pose.kneeling)
                {
                    const FloatPoint knee = side.foot;
                    fillOval(body, knee + FloatPoint{ sign * 2.0f, 4.0f }, 12.0f, 8.0f, foot);
                    drawSegment(body, hip, knee, limb, 16.0f);
                    fillDisc(body, knee, 9.0f, joint);
                    continue;
                }
                const BendToward outward = which == AudienceSide::Left
                    ? BendToward::Left
                    : BendToward::Right;
                const Limb leg = stretchToward(hip, side.foot, 47.0f, 47.0f, outward);
                drawSegment(body, hip, leg.joint, limb, 16.0f);
                drawSegment(body, leg.joint, leg.end, limb, 16.0f);
                fillDisc(body, leg.joint, 9.0f, joint);
                if (pose.wearsPointeShoes)
                    drawPointeRibbons(body, leg);
                const SceneFrame sole = body.within(Matrix3x2::Translation(leg.end)
                    * Matrix3x2::Rotation(side.footTurn));
                const float toeSide = which == AudienceSide::Left ? -19.0f : -13.0f;
                fillRoundedBox(sole, { toeSide, -3.0f, toeSide + 32.0f, 14.0f }, 6.0f, foot);
                if (pose.wearsPointeShoes)
                {
                    drawSegment(sole, { toeSide + 4.0f, 1.0f }, { toeSide + 28.0f, 1.0f },
                        SolidColor{ AndroidColors::SatinShine }, 1.6f);
                }
            }
        }

        // Two crossings low on the shin, each a pair of strokes across it.
        void drawPointeRibbons(const SceneFrame& body, const Limb& leg)
        {
            const FloatPoint shin = leg.end - leg.joint;
            const FloatPoint along = shin * (1.0f / std::hypot(shin.x, shin.y));
            const FloatPoint across = { -along.y, along.x };
            PixelPath ribbons;
            for (const float share : { 0.68f, 0.84f })
            {
                const FloatPoint crossing = leg.joint + shin * share;
                for (const float slant : { -5.0f, 5.0f })
                {
                    const FloatPoint halfStroke = across * 8.0f + along * slant;
                    ribbons.moveTo(crossing - halfStroke);
                    ribbons.lineTo(crossing + halfStroke);
                }
            }
            strokeShape(body, ribbons, SolidColor{ AndroidColors::Ribbon }, 2.4f);
        }

        // From behind, the chest panel gives way to three vents.
        void drawTorso(const SceneFrame& upper, const AndroidPose& pose)
        {
            const bool fromBehind = pose.facing < 0.0f;
            PixelPath torso;
            torso.drawRoundedRect(FloatRect{ -42.0f, -117.0f, 42.0f, -9.0f }, 18.0f, 18.0f);
            fillShape(upper, torso, LinearGradient::Simple({ -40.0f, -115.0f }, { 40.0f, -3.0f },
                fromBehind ? AndroidColors::BackLight : AndroidColors::MetalLight,
                fromBehind ? AndroidColors::BackShade : AndroidColors::MetalShade));
            strokeShape(upper, torso, SolidColor{ AndroidColors::MetalEdge }, 1.6f);
            if (fromBehind)
            {
                for (int i = 0; i != 3; ++i)
                {
                    const float top = -92.0f + static_cast<float>(i) * 13.0f;
                    fillRoundedBox(upper, { -20.0f, top, 20.0f, top + 6.0f }, 3.0f,
                        SolidColor{ AndroidColors::Vent });
                }
            }
            else
            {
                fillRoundedBox(upper, { -24.0f, -89.0f, 24.0f, -49.0f }, 7.0f,
                    SolidColor{ AndroidColors::Panel });
                const int chasing = static_cast<int>(fractionalPart(pose.cycleTime * 32.0f) * 3.0f);
                for (int i = 0; i != 3; ++i)
                {
                    const FloatPoint light = { static_cast<float>(i - 1) * 13.0f, -69.0f };
                    const Color color = AndroidColors::ChestLights[static_cast<std::size_t>(i)];
                    const bool lit = pose.lights == ChestLights::All
                        || (pose.lights == ChestLights::Chase && i == chasing)
                        || (pose.lights == ChestLights::Pink && i == k_pinkLight);
                    if (lit)
                        drawGlow(upper, light, 12.0f, color.withOpacity(0.6f));
                    fillDisc(upper, light, 4.2f,
                        SolidColor{ color.withOpacity(lit ? 1.0f : 0.28f) });
                }
            }
            fillBox(upper, { -8.0f, -133.0f, 8.0f, -115.0f }, SolidColor{ AndroidColors::Neck });
        }

        // From behind, the visor gives way to a plate with two bolts.
        void drawHead(const SceneFrame& upper, const AndroidPose& pose)
        {
            const bool fromBehind = pose.facing < 0.0f;
            // The head keeps its shape as the upper body shortens in a bow.
            const SceneFrame head = upper.within(Matrix3x2::Translation(AndroidBody::Neck)
                * Matrix3x2::Scale(1.0f, 1.0f / upperHeightShare(pose))
                * Matrix3x2::Rotation(pose.headTurn));
            const SolidColor joint{ AndroidColors::Joint };
            fillDisc(head, { -38.0f, -28.0f }, 8.0f, joint);
            fillDisc(head, { 38.0f, -28.0f }, 8.0f, joint);
            PixelPath skull;
            skull.drawRoundedRect(FloatRect{ -36.0f, -58.0f, 36.0f, 0.0f }, 22.0f, 22.0f);
            fillShape(head, skull, LinearGradient::Simple({ -30.0f, -58.0f }, { 30.0f, 0.0f },
                fromBehind ? AndroidColors::BackLight : AndroidColors::HeadLight,
                AndroidColors::HeadShade));
            strokeShape(head, skull, SolidColor{ AndroidColors::MetalEdge }, 1.6f);
            if (fromBehind)
            {
                fillRoundedBox(head, { -22.0f, -44.0f, 22.0f, -20.0f }, 9.0f,
                    SolidColor{ AndroidColors::HeadShade.withOpacity(0.7f) });
                fillDisc(head, { -14.0f, -32.0f }, 2.2f, SolidColor{ AndroidColors::Vent });
                fillDisc(head, { 14.0f, -32.0f }, 2.2f, SolidColor{ AndroidColors::Vent });
            }
            else
            {
                // bowing, the face goes down and the eyes look at the floor
                const float faceDrop = 6.0f * pose.bow;
                fillRoundedBox(head, { -28.0f, -46.0f + faceDrop, 28.0f, -21.0f + faceDrop }, 10.0f,
                    SolidColor{ AndroidColors::Visor });
                const float blink = fractionalPart(pose.cycleTime * 4.0f + 0.2f) > 0.975f
                    ? 0.15f
                    : 1.0f;
                const float opening = std::min({ pose.eyeOpening, blink, 1.0f - 0.55f * pose.bow });
                const float eyeY = -34.0f + 1.4f * faceDrop;
                const float eyeHeight = 12.0f * opening;
                for (const float eyeLeft : { -19.0f, 5.0f })
                {
                    const float left = eyeLeft + pose.look;
                    drawGlow(head, { left + 7.0f, eyeY }, 13.0f,
                        AndroidColors::EyeGlow.withOpacity(0.35f));
                    const FloatRect eye = {
                        left,
                        eyeY - eyeHeight * 0.5f,
                        left + 14.0f,
                        eyeY + eyeHeight * 0.5f
                    };
                    fillRoundedBox(head, eye, 4.0f * opening, SolidColor{ AndroidColors::Eye });
                }
                PixelPath grille;
                for (int i = 0; i != 3; ++i)
                {
                    const float x = static_cast<float>(-9 + i * 9);
                    grille.moveTo(x, -12.0f + faceDrop);
                    grille.lineTo(x, -6.0f + 0.6f * faceDrop);
                }
                strokeShape(head, grille, SolidColor{ AndroidColors::Neck }, 2.4f);
            }

            const bool antennaLit = pose.antenna == AntennaLight::On
                || (pose.antenna == AntennaLight::Blinking
                    && fractionalPart(pose.cycleTime * 16.0f) < 0.5f);
            // Through the hat, a shorter stalk stands on the crown, in the hat's units.
            const SceneFrame antennaFrame = pose.wearsHat ? topHatFrame(head, k_wornHat) : head;
            const float antennaFoot = pose.wearsHat ? -56.0f : -58.0f;
            const float stalkTop = pose.wearsHat ? -68.0f : -78.0f;
            const float ballY = stalkTop - 5.0f;
            if (pose.wearsHat)
            {
                drawTopHat(head, k_wornHat);
                drawTopHatBrimLip(head, k_wornHat);
            }
            drawSegment(antennaFrame, { 0.0f, antennaFoot }, { 0.0f, stalkTop },
                SolidColor{ AndroidColors::Neck }, 3.0f);
            if (antennaLit)
                drawGlow(antennaFrame, { 0.0f, ballY }, 14.0f,
                    AndroidColors::AntennaGlow.withOpacity(0.6f));
            fillDisc(antennaFrame, { 0.0f, ballY }, 5.0f,
                SolidColor{ antennaLit ? AndroidColors::AntennaOn : AndroidColors::AntennaOff });
        }

        void drawForearm(const SceneFrame& upper, const AndroidPose& pose,
            const AudienceSide which)
        {
            const Limb arm = armOf(pose, which);
            drawSegment(upper, arm.joint, arm.end, SolidColor{ AndroidColors::Limb }, 12.0f);
            fillDisc(upper, arm.joint, 7.5f, SolidColor{ AndroidColors::Joint });
            if (pose.side(which).handShape == HandShape::Fist)
            {
                PixelPath fist;
                fist.drawRoundedRect(FloatRect{ arm.end.x - 10.0f, arm.end.y - 6.0f,
                    arm.end.x + 10.0f, arm.end.y + 10.0f }, 6.0f, 6.0f);
                fillShape(upper, fist, SolidColor{ AndroidColors::Hand });
                strokeShape(upper, fist, SolidColor{ AndroidColors::MetalEdge }, 1.2f);
                return;
            }
            fillDisc(upper, arm.end, 9.0f, SolidColor{ AndroidColors::Hand });
        }
    }
}
