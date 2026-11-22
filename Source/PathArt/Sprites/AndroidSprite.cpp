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
        constexpr Color FootBlack{ 0xFF101217 };
        constexpr Color FootHighlight{ 0xFFD9E4F0 };
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

    namespace FedoraColors
    {
        constexpr Color FeltLight{ 0xFF424655 };
        constexpr Color FeltShade{ 0xFF181B24 };
        constexpr Color Crease{ 0xFF222632 };
        constexpr Color Band{ 0xFF11131A };
        constexpr Color Rim{ 0xFF697486 };
    }

    namespace JacketColors
    {
        constexpr Color Cloth{ 0xFF171920 };
        constexpr Color Lapel{ 0xFF343945 };
        constexpr Color Edge{ 0xFF697488 };
        constexpr Color Sequin{ 0xFF697482 };
        constexpr Color Glint{ 0xFFF3F5FF };
    }

    namespace GloveColors
    {
        constexpr Color White{ 0xFFFFFFFF };
        constexpr Color Shade{ 0xFFB8C7DB };
        constexpr Color Seam{ 0xFF8999B1 };
        constexpr Color Sequin{ 0xFFA7B8CF };
    }

    namespace
    {
        // The hat sits on the skull at a slant, in the head's units.
        constexpr TopHatPose k_wornHat{
            .brim = { 6.0f, -52.0f },
            .turn = 0.14f,
            .size = 0.92f
        };
        constexpr float k_fedoraAntennaFoot{ -79.0f }; // where the stalk emerges from the crown
        constexpr float k_antennaStalkLength{ 20.0f };
        constexpr float k_antennaTipRadius{ 5.0f };

        // How far a turn may squeeze the figure, so it never folds to nothing.
        constexpr float k_thinnestFacing{ 0.03f };
        constexpr int k_pinkLight{ 2 };
        const FloatRect k_pelvisBox{ -30.0f, -13.0f, 30.0f, 13.0f };
        const FloatRect k_torsoBox{ -42.0f, -117.0f, 42.0f, -9.0f };
        const FloatRect k_neckBox{ -8.0f, -133.0f, 8.0f, -115.0f };
        const FloatRect k_skullBox{ -36.0f, -58.0f, 36.0f, 0.0f };

        [[nodiscard]] float sideSign(AudienceSide);
        [[nodiscard]] float drawnFacing(const AndroidPose&);
        // Bowing toward the audience, the upper body keeps this share of its height.
        [[nodiscard]] float upperHeightShare(const AndroidPose&);
        // The head's place on the upper body: about the neck, turned, its shape kept in a bow.
        [[nodiscard]] Matrix3x2 headToUpper(const AndroidPose&);
        [[nodiscard]] SceneFrame neckFrameOf(const SceneFrame& upper, const AndroidPose&);
        // The figure's frame in units about the pelvis.
        [[nodiscard]] SceneFrame bodyFrameOf(const SceneFrame&, const AndroidPose&);
        // A two-part limb that stretches rather than stops short of its end.
        [[nodiscard]] Limb stretchToward(FloatPoint root, FloatPoint target, float upperLength,
            float lowerLength, BendToward);
        [[nodiscard]] Limb armOf(const AndroidPose&, AudienceSide);
        [[nodiscard]] FloatPoint bodyToUpper(const AndroidPose&, FloatPoint bodyPoint);
        [[nodiscard]] FloatPoint upperToBody(const AndroidPose&, FloatPoint upperPoint);

        void drawLegs(const SceneFrame& body, const AndroidPose&, const SolidColor* silhouette = nullptr);
        void drawPointeRibbons(const SceneFrame& body, const Limb& leg);
        void drawTorso(const SceneFrame& upper, const AndroidPose&);
        void drawSequinJacket(const SceneFrame& upper, const AndroidPose&,
            const SolidColor* silhouette = nullptr);
        void drawJacketSleeve(const SceneFrame& upper, FloatPoint from, FloatPoint to, float width,
            float shimmer, const SolidColor* silhouette);
        void drawFedora(const SceneFrame& head, const SolidColor* silhouette = nullptr);
        void drawHead(const SceneFrame& upper, const AndroidPose&);
        void drawArms(const SceneFrame& upper, const AndroidPose&, const SolidColor* silhouette = nullptr);
        void drawWhiteGlove(const SceneFrame& upper, const Limb& arm, AudienceSide,
            const SolidColor* silhouette);
        void drawForearm(const SceneFrame& upper, const AndroidPose&, AudienceSide,
            const SolidColor* silhouette = nullptr);
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

    FloatPoint androidHeadToUpper(const AndroidPose& pose, const FloatPoint headPoint)
    {
        return headToUpper(pose).transform(headPoint);
    }

    // The brim's middle and a point one unit along it, carried from the head into the frame,
    // give the hat's place, turn and size there.
    TopHatPose androidWornHat(const SceneFrame& frame, const AndroidPose& pose)
    {
        const FloatPoint brim = k_wornHat.brim;
        const FloatPoint along = brim + rotated({ 1.0f, 0.0f }, k_wornHat.turn);
        const FloatPoint brimInFrame = androidUpperToFrame(frame, pose,
            androidHeadToUpper(pose, brim));
        const FloatPoint alongInFrame = androidUpperToFrame(frame, pose,
            androidHeadToUpper(pose, along));
        const FloatPoint unitAlong = alongInFrame - brimInFrame;
        return {
            .brim = brimInFrame,
            .turn = std::atan2(unitAlong.y, unitAlong.x),
            .size = k_wornHat.size * std::hypot(unitAlong.x, unitAlong.y)
        };
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
            const bool roundPalm = side->handShape == HandShape::Open && !side->wearsWhiteGlove;
            reachDown(upperToBody(pose, side->hand), roundPalm ? side->palmRadius : k_palmRadius);
        }
        const FloatPoint headOffset = { pose.headOffsetX, pose.headOffsetY };
        const FloatPoint antennaTip = AndroidBody::AntennaTip + headOffset;
        reachDown(upperToBody(pose, antennaTip), 0.0f);
        pose.pelvis.y = floorY - lowest * frame.unit;
    }

    void drawAndroid(const SceneFrame& frame, const AndroidPose& pose)
    {
        const SceneFrame body = bodyFrameOf(frame, pose);
        drawLegs(body, pose);
        fillRoundedBox(body, k_pelvisBox, 8.0f, SolidColor{ AndroidColors::Pelvis });
        // A tutu looks the same from every side, so it keeps its width as the figure turns.
        if (pose.wearsTutu)
        {
            const SceneFrame hips = frame.within(Matrix3x2::Translation(pose.pelvis)
                * Matrix3x2::Rotation(pose.tilt) * Matrix3x2::Scale(frame.unit));
            drawTutu(hips, pose.tutuTurn, pose.tutuLook);
        }
        const SceneFrame upper = androidUpperFrame(frame, pose);
        drawTorso(upper, pose);
        if (pose.wearsSequinJacket)
        {
            drawSequinJacket(upper, pose);
        }
        drawHead(upper, pose);
        drawArms(upper, pose);
    }

    void drawAndroidSilhouette(const SceneFrame& frame, const AndroidPose& pose, const Color color)
    {
        const SolidColor silhouette{ color };
        const SceneFrame body = bodyFrameOf(frame, pose);
        const SceneFrame upper = androidUpperFrame(frame, pose);
        const SceneFrame head = upper.within(headToUpper(pose));
        drawLegs(body, pose, &silhouette);
        fillRoundedBox(body, k_pelvisBox, 8.0f, silhouette);
        fillRoundedBox(upper, k_torsoBox, 18.0f, silhouette);
        if (pose.wearsSequinJacket)
        {
            drawSequinJacket(upper, pose, &silhouette);
        }
        fillBox(neckFrameOf(upper, pose), k_neckBox, silhouette);
        fillDisc(head, { -38.0f, -28.0f }, 8.0f, silhouette);
        fillDisc(head, { 38.0f, -28.0f }, 8.0f, silhouette);
        fillRoundedBox(head, k_skullBox, 22.0f, silhouette);
        if (pose.wearsFedora)
        {
            drawFedora(head, &silhouette);
        }
        const float antennaFoot = pose.wearsFedora ? k_fedoraAntennaFoot : -58.0f;
        const float stalkTop = antennaFoot - k_antennaStalkLength;
        drawSegment(head, { 0.0f, antennaFoot }, { 0.0f, stalkTop }, silhouette, 3.0f);
        fillDisc(head, { 0.0f, stalkTop - k_antennaTipRadius }, k_antennaTipRadius, silhouette);
        drawArms(upper, pose, &silhouette);
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

        Matrix3x2 headToUpper(const AndroidPose& pose)
        {
            const FloatPoint headOffset = { pose.headOffsetX, pose.headOffsetY };
            const FloatPoint headPivot = AndroidBody::Neck + headOffset;
            return Matrix3x2::Translation(headPivot)
                * Matrix3x2::Scale(1.0f, 1.0f / upperHeightShare(pose))
                * Matrix3x2::Rotation(pose.headTurn);
        }

        SceneFrame neckFrameOf(const SceneFrame& upper, const AndroidPose& pose)
        {
            // The neck follows the head at its top and stays attached at the torso.
            const float neckSpan = k_neckBox.top - k_neckBox.bottom;
            const float neckShear = pose.headOffsetX / neckSpan;
            const float neckStretch = pose.headOffsetY / neckSpan;
            return upper.within({
                .c = neckShear,
                .d = 1.0f + neckStretch,
                .e = -neckShear * k_neckBox.bottom,
                .f = -neckStretch * k_neckBox.bottom
            });
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
            Limb arm = stretchToward(shoulder, side.hand, side.upperArm, side.forearm, bend);
            if (side.elbowPositionBlend > 0.0f)
            {
                arm.joint = FloatPoint::between(arm.joint, side.elbowPosition, side.elbowPositionBlend);
            }
            return arm;
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
        void drawLegs(const SceneFrame& body, const AndroidPose& pose, const SolidColor* silhouette)
        {
            const SolidColor limb = silhouette ? *silhouette : SolidColor{ AndroidColors::Limb };
            const SolidColor joint = silhouette ? *silhouette : SolidColor{ AndroidColors::Joint };
            const Color metalFoot = pose.highlightFeet ? AndroidColors::FootBlack : AndroidColors::Foot;
            const Color footColor = pose.wearsPointeShoes ? AndroidColors::Satin : metalFoot;
            const SolidColor foot = silhouette ? *silhouette : SolidColor{ footColor };
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
                Limb leg = stretchToward(hip, side.foot, 47.0f, 47.0f, outward);
                leg.joint.x += side.kneeOffsetX;
                drawSegment(body, hip, leg.joint, limb, 16.0f);
                drawSegment(body, leg.joint, leg.end, limb, 16.0f);
                fillDisc(body, leg.joint, 9.0f, joint);
                if (pose.wearsPointeShoes && !silhouette)
                    drawPointeRibbons(body, leg);
                const SceneFrame sole = body.within(Matrix3x2::Translation(leg.end)
                    * Matrix3x2::Rotation(side.footTurn));
                const float toeSide = which == AudienceSide::Left ? -19.0f : -13.0f;
                fillRoundedBox(sole, { toeSide, -3.0f, toeSide + 32.0f, 14.0f }, 6.0f, foot);
                if (pose.wearsPointeShoes && !silhouette)
                {
                    drawSegment(sole, { toeSide + 4.0f, 1.0f }, { toeSide + 28.0f, 1.0f },
                        SolidColor{ AndroidColors::SatinShine }, 1.6f);
                }
                else if (pose.highlightFeet && !silhouette)
                {
                    drawSegment(sole, { toeSide + 6.0f, 1.0f }, { toeSide + 26.0f, 1.0f },
                        SolidColor{ AndroidColors::FootHighlight }, 2.0f);
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
            torso.drawRoundedRect(k_torsoBox, 18.0f, 18.0f);
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
            fillBox(neckFrameOf(upper, pose), k_neckBox, SolidColor{ AndroidColors::Neck });
        }

        void drawSequin(const SceneFrame& frame, const FloatPoint center, const float shimmer)
        {
            const float phase = fractionalPart(shimmer);
            const float highlight = 1.0f - smoothStep(0.0f, 0.22f, std::abs(phase - 0.5f));
            fillDisc(frame, center, 1.1f + 0.9f * highlight,
                SolidColor{ Color::between(JacketColors::Sequin, JacketColors::Glint, highlight) });
        }

        void drawSequinJacket(const SceneFrame& upper, const AndroidPose& pose,
            const SolidColor* silhouette)
        {
            const SolidColor cloth = silhouette ? *silhouette : SolidColor{ JacketColors::Cloth };
            if (pose.facing < 0.0f)
            {
                fillRoundedBox(upper, k_torsoBox, 18.0f, cloth);
            }
            for (const float side : { -1.0f, 1.0f })
            {
                const SceneFrame panel = upper.within(Matrix3x2::Scale(side, 1.0f));
                PixelPath front;
                front.moveTo(13.0f, -117.0f);
                front.quadTo({ 43.0f, -119.0f }, { 49.0f, -103.0f });
                front.quadTo({ 34.0f, -62.0f }, { 41.0f, -8.0f });
                front.quadTo({ 33.0f, 4.0f }, { 7.0f, -1.0f });
                front.lineTo(12.0f, -41.0f);
                front.lineTo(22.0f, -80.0f);
                front.close();
                fillShape(panel, front, cloth);
                if (silhouette)
                {
                    continue;
                }
                strokeShape(panel, front, SolidColor{ JacketColors::Edge }, 1.5f);

                PixelPath lapel;
                lapel.moveTo(13.0f, -117.0f);
                lapel.lineTo(23.0f, -111.0f);
                lapel.lineTo(30.0f, -91.0f);
                lapel.lineTo(24.0f, -90.0f);
                lapel.lineTo(28.0f, -83.0f);
                lapel.lineTo(13.0f, -45.0f);
                lapel.lineTo(22.0f, -80.0f);
                lapel.close();
                fillShape(panel, lapel, SolidColor{ JacketColors::Lapel });
                for (int row = 0; row < 10; ++row)
                {
                    const float y = -106.0f + static_cast<float>(row) * 10.5f;
                    const float innerX = row < 4 ? 29.0f : 21.0f;
                    for (int column = 0; column < 3; ++column)
                    {
                        const float x = std::lerp(innerX, 34.0f, static_cast<float>(column) * 0.5f);
                        const float phase = pose.cycleTime * 27.0f + static_cast<float>(row) * 0.27f
                            + static_cast<float>(column) * 0.38f + side * 0.19f;
                        drawSequin(panel, { x, y + 2.0f * static_cast<float>(column) }, phase);
                    }
                }
            }
        }

        void drawJacketSleeve(const SceneFrame& upper, const FloatPoint from, const FloatPoint to,
            const float width, const float shimmer, const SolidColor* silhouette)
        {
            if (silhouette)
            {
                drawSegment(upper, from, to, *silhouette, width);
                return;
            }
            drawSegment(upper, from, to, SolidColor{ JacketColors::Edge }, width);
            drawSegment(upper, from, to, SolidColor{ JacketColors::Cloth }, width - 2.8f);
            const FloatPoint span = to - from;
            const float length = std::hypot(span.x, span.y);
            if (length < 8.0f)
            {
                return;
            }
            const FloatPoint across = FloatPoint{ -span.y, span.x } * (0.23f * width / length);
            for (int row = 0; row < 8; ++row)
            {
                const float along = (static_cast<float>(row) + 0.5f) / 8.0f;
                const float side = row % 2 == 0 ? -1.0f : 1.0f;
                drawSequin(upper, from + span * along + across * side,
                    shimmer + static_cast<float>(row) * 0.37f);
            }
        }

        void drawFedora(const SceneFrame& head, const SolidColor* silhouette)
        {
            const SceneFrame hat = head.within(Matrix3x2::Translation(0.0f, -53.0f)
                * Matrix3x2::Rotation(-0.13f));
            PixelPath brim;
            brim.moveTo(-49.0f, 1.0f);
            brim.cubicTo({ -30.0f, -10.0f }, { 29.0f, -11.0f }, { 51.0f, -2.0f });
            brim.cubicTo({ 43.0f, 9.0f }, { -19.0f, 11.0f }, { -49.0f, 1.0f });
            brim.close();

            PixelPath crown;
            crown.moveTo(-30.0f, 1.0f);
            crown.quadTo({ -29.0f, -24.0f }, { -22.0f, -30.0f });
            crown.cubicTo({ -14.0f, -36.0f }, { -8.0f, -22.0f }, { 6.0f, -29.0f });
            crown.quadTo({ 20.0f, -36.0f }, { 24.0f, -24.0f });
            crown.lineTo(30.0f, -2.0f);
            crown.quadTo({ 4.0f, 5.0f }, { -30.0f, 1.0f });
            crown.close();
            if (silhouette)
            {
                fillShape(hat, brim, *silhouette);
                fillShape(hat, crown, *silhouette);
                return;
            }
            fillShape(hat, brim, SolidColor{ FedoraColors::FeltShade });
            strokeShape(hat, brim, SolidColor{ FedoraColors::Rim }, 1.7f);
            fillShape(hat, crown, LinearGradient::Simple({ -27.0f, -15.0f }, { 29.0f, -9.0f },
                FedoraColors::FeltLight, FedoraColors::FeltShade));
            strokeShape(hat, crown, SolidColor{ FedoraColors::Rim }, 1.4f);

            PixelPath band;
            band.moveTo(-29.0f, -7.0f);
            band.quadTo({ 3.0f, -3.0f }, { 28.0f, -10.0f });
            band.lineTo(30.0f, -2.0f);
            band.quadTo({ 4.0f, 5.0f }, { -30.0f, 1.0f });
            band.close();
            fillShape(hat, band, SolidColor{ FedoraColors::Band });

            PixelPath crease;
            crease.moveTo(-24.0f, -23.0f);
            crease.quadTo({ -10.0f, -15.0f }, { 1.0f, -24.0f });
            strokeShape(hat, crease, SolidColor{ FedoraColors::Crease }, 2.6f);
        }

        // From behind, the visor gives way to a plate with two bolts.
        void drawHead(const SceneFrame& upper, const AndroidPose& pose)
        {
            const bool fromBehind = pose.facing < 0.0f;
            const SceneFrame head = upper.within(headToUpper(pose));
            const SolidColor joint{ AndroidColors::Joint };
            fillDisc(head, { -38.0f, -28.0f }, 8.0f, joint);
            fillDisc(head, { 38.0f, -28.0f }, 8.0f, joint);
            PixelPath skull;
            skull.drawRoundedRect(k_skullBox, 22.0f, 22.0f);
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

            if (pose.wearsFedora)
            {
                drawFedora(head);
            }
            const bool antennaLit = pose.antenna == AntennaLight::On
                || (pose.antenna == AntennaLight::Blinking
                    && fractionalPart(pose.cycleTime * 16.0f) < 0.5f);
            const bool wearsTopHat = pose.wearsHat && !pose.wearsFedora;
            const SceneFrame antennaFrame = wearsTopHat ? topHatFrame(head, k_wornHat) : head;
            const float antennaFoot = wearsTopHat ? -56.0f
                : (pose.wearsFedora ? k_fedoraAntennaFoot : -58.0f);
            const float stalkTop = antennaFoot - (wearsTopHat ? 12.0f : k_antennaStalkLength);
            const float ballY = stalkTop - k_antennaTipRadius;
            if (wearsTopHat)
            {
                drawTopHat(head, k_wornHat);
                drawTopHatBrimLip(head, k_wornHat);
            }
            drawSegment(antennaFrame, { 0.0f, antennaFoot }, { 0.0f, stalkTop },
                SolidColor{ AndroidColors::Neck }, 3.0f);
            if (antennaLit)
                drawGlow(antennaFrame, { 0.0f, ballY }, 14.0f,
                    AndroidColors::AntennaGlow.withOpacity(0.6f));
            fillDisc(antennaFrame, { 0.0f, ballY }, k_antennaTipRadius,
                SolidColor{ antennaLit ? AndroidColors::AntennaOn : AndroidColors::AntennaOff });
        }

        void drawArms(const SceneFrame& upper, const AndroidPose& pose, const SolidColor* silhouette)
        {
            const SolidColor limb = silhouette ? *silhouette : SolidColor{ AndroidColors::Limb };
            const SolidColor joint = silhouette ? *silhouette : SolidColor{ AndroidColors::Joint };
            for (const AudienceSide which : { AudienceSide::Left, AudienceSide::Right })
            {
                const FloatPoint shoulder = {
                    sideSign(which) * AndroidBody::RightShoulder.x,
                    AndroidBody::RightShoulder.y
                };
                const Limb arm = armOf(pose, which);
                if (pose.wearsSequinJacket)
                {
                    drawJacketSleeve(upper, shoulder, arm.joint, 22.0f,
                        pose.cycleTime * 27.0f + sideSign(which) * 0.15f, silhouette);
                }
                else
                {
                    drawSegment(upper, shoulder, arm.joint, limb, 13.0f);
                }
                if (silhouette || !pose.side(which).forearmOnTop)
                {
                    drawForearm(upper, pose, which, silhouette);
                }
                if (!pose.wearsSequinJacket)
                {
                    fillDisc(upper, shoulder, 11.0f, joint);
                }
            }
        }

        void drawWhiteGlove(const SceneFrame& upper, const Limb& arm, const AudienceSide which,
            const SolidColor* silhouette)
        {
            constexpr float k_gloveScale{ 1.8f };
            constexpr float k_cuffX{ -11.0f };
            const FloatPoint forearm = arm.end - arm.joint;
            const float turn = std::atan2(forearm.y, forearm.x);
            const float cuffOffset = k_cuffX * (1.0f - k_gloveScale);
            const SceneFrame hand = upper.within(Matrix3x2::Translation(arm.end)
                * Matrix3x2::Rotation(turn) * Matrix3x2::Translation(cuffOffset, 0.0f)
                * Matrix3x2::Scale(k_gloveScale, -sideSign(which) * k_gloveScale));
            PixelPath glove;
            glove.moveTo(k_cuffX, -6.0f);
            glove.quadTo({ -3.0f, -6.0f }, { 3.0f, -9.0f });
            glove.lineTo(11.0f, -9.0f);
            glove.quadTo({ 16.0f, -9.0f }, { 13.0f, -5.0f });
            glove.quadTo({ 20.0f, -5.0f }, { 16.0f, -1.0f });
            glove.quadTo({ 21.0f, 1.0f }, { 15.0f, 3.0f });
            glove.quadTo({ 17.0f, 7.0f }, { 11.0f, 7.0f });
            glove.lineTo(2.0f, 7.0f);
            glove.quadTo({ 6.0f, 12.0f }, { 1.0f, 13.0f });
            glove.quadTo({ -2.0f, 13.0f }, { -5.0f, 8.0f });
            glove.lineTo(k_cuffX, 6.0f);
            glove.close();
            if (silhouette)
            {
                fillShape(hand, glove, *silhouette);
                return;
            }

            fillShape(hand, glove, LinearGradient::Simple({ 2.0f, -8.0f }, { 4.0f, 13.0f },
                GloveColors::White, GloveColors::Shade));
            strokeShape(hand, glove, SolidColor{ GloveColors::Seam }, 0.9f);
            drawSegment(hand, { -7.0f, -5.0f }, { -7.0f, 6.0f },
                SolidColor{ GloveColors::Seam }, 0.8f);
            constexpr std::array<FloatPoint, 5> k_sequins{
                FloatPoint{ -2.0f, 1.0f }, { 2.0f, -3.0f }, { 3.0f, 4.0f },
                { 7.0f, 0.0f }, { 10.0f, -4.0f }
            };
            for (const FloatPoint center : k_sequins)
            {
                fillDisc(hand, center, 0.85f, SolidColor{ GloveColors::Sequin });
            }
        }

        void drawForearm(const SceneFrame& upper, const AndroidPose& pose, const AudienceSide which,
            const SolidColor* silhouette)
        {
            const Limb arm = armOf(pose, which);
            const SolidColor limb = silhouette ? *silhouette : SolidColor{ AndroidColors::Limb };
            const SolidColor joint = silhouette ? *silhouette : SolidColor{ AndroidColors::Joint };
            const AndroidSide& side = pose.side(which);
            const Color handColor = side.wearsWhiteGlove ? GloveColors::White : AndroidColors::Hand;
            const SolidColor hand = silhouette ? *silhouette : SolidColor{ handColor };
            if (pose.wearsSequinJacket)
            {
                const FloatPoint wrist = FloatPoint::between(arm.joint, arm.end, 0.87f);
                drawJacketSleeve(upper, arm.joint, wrist, 18.0f,
                    pose.cycleTime * 27.0f + 0.43f + sideSign(which) * 0.15f, silhouette);
            }
            else
            {
                drawSegment(upper, arm.joint, arm.end, limb, 12.0f);
                fillDisc(upper, arm.joint, 7.5f, joint);
            }
            if (side.handShape == HandShape::Fist)
            {
                PixelPath fist;
                fist.drawRoundedRect(FloatRect{ arm.end.x - 10.0f, arm.end.y - 6.0f,
                    arm.end.x + 10.0f, arm.end.y + 10.0f }, 6.0f, 6.0f);
                fillShape(upper, fist, hand);
                if (!silhouette)
                {
                    const Color edge = side.wearsWhiteGlove ? GloveColors::Seam
                        : AndroidColors::MetalEdge;
                    strokeShape(upper, fist, SolidColor{ edge }, 1.2f);
                }
                return;
            }
            if (side.wearsWhiteGlove)
            {
                drawWhiteGlove(upper, arm, which, silhouette);
            }
            else
            {
                fillDisc(upper, arm.end, side.palmRadius, hand);
            }
        }
    }
}
