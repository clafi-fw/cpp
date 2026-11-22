module ClaFi.PathArt.RobotDanceAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.FogEffect;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace RobotDanceLighting
    {
        constexpr Color StageWash{ 0xB806101D };
        constexpr Color Backlight{ 0xDBB1C7E7 };
        constexpr Color FloorLight{ 0xB2D8E8FF };
        constexpr Color Shadow{ 0xFF121C2A };
        constexpr Color DuskAbove{ 0x90050A13 };
        constexpr Color DuskBelow{ 0x4309101D };
        constexpr Color BackSmoke{ 0x5094AECF };
        constexpr Color FrontSmoke{ 0x49C4DBEF };
    }

    namespace SmokeMachineColors
    {
        constexpr Color Lid{ 0xFF7D8B9D };
        constexpr Color Panel{ 0xFF465264 };
        constexpr Color End{ 0xFF303949 };
        constexpr Color Recess{ 0xFF161E2D };
        constexpr Color Rim{ 0xFF8A9AAF };
        constexpr Color Indicator{ 0xFFFFAD5B };
        constexpr Color Vapor{ 0x9DBED2E8 };
    }

    namespace
    {
        // A beat is a quarter of a sway, which is a quarter of the act's ten seconds.
        constexpr float k_beatSeconds{ 0.625f };
        constexpr float k_swaySeconds{ 2.5f };
        constexpr float k_moonwalkStartBeat{ 2.0f };
        constexpr float k_moonwalkTurnBeat{ 6.0f };
        constexpr float k_moonwalkEndBeat{ 10.0f };
        constexpr float k_moonwalkStride{ 30.0f };
        constexpr float k_dancerScale{ 0.84f };
        constexpr float k_shuffleHalfCycleSeconds{ 0.26f };
        constexpr float k_finalePoseSeconds{ 8.72f };
        constexpr float k_finaleSwaySeconds{ 1.24f };
        constexpr float k_finaleLightFadeSeconds{ 9.32f };
        constexpr float k_curtainClosedSeconds{ 9.70f };

        void drawSmokeMachine(const SceneFrame& frame, const FloatPoint foot, const float cycleTime,
            const float light)
        {
            using namespace SmokeMachineColors;
            const SceneFrame machine = frame.within(Matrix3x2::Translation(foot)
                * Matrix3x2::Scale(frame.unit));
            const SolidColor recess{ Recess };
            fillRoundedBox(machine, { -40.0f, -9.0f, -23.0f, 0.0f }, 2.0f, recess);
            fillRoundedBox(machine, { 15.0f, -9.0f, 32.0f, 0.0f }, 2.0f, recess);
            drawSegment(machine, { -22.0f, -42.0f }, { -22.0f, -52.0f }, recess, 4.0f);
            drawSegment(machine, { 10.0f, -42.0f }, { 10.0f, -52.0f }, recess, 4.0f);
            drawSegment(machine, { -22.0f, -52.0f }, { 10.0f, -52.0f }, SolidColor{ Rim }, 4.0f);

            PixelPath lid;
            lid.moveTo(-44.0f, -33.0f);
            lid.lineTo(-32.0f, -44.0f);
            lid.lineTo(46.0f, -44.0f);
            lid.lineTo(34.0f, -33.0f);
            lid.close();
            fillShape(machine, lid, SolidColor{ Lid });
            PixelPath end;
            end.moveTo(34.0f, -33.0f);
            end.lineTo(46.0f, -44.0f);
            end.lineTo(46.0f, -17.0f);
            end.lineTo(34.0f, -6.0f);
            end.close();
            fillShape(machine, end, SolidColor{ End });
            fillBox(machine, { -44.0f, -33.0f, 34.0f, -6.0f }, SolidColor{ Panel });
            for (const float y : { -25.0f, -20.0f, -15.0f })
            {
                drawSegment(machine, { -34.0f, y }, { -13.0f, y }, recess, 2.0f);
            }
            fillDisc(machine, { 12.0f, -19.0f }, 2.7f, SolidColor{ Indicator });
            fillRoundedBox(machine, { 37.0f, -28.0f, 55.0f, -15.0f }, 3.0f, SolidColor{ Rim });
            fillOval(machine, { 54.0f, -21.5f }, 3.5f, 5.0f, recess);

            drawGlow(machine, { 62.0f, -21.5f }, 5.0f, Vapor.faded(0.24f * light), 0.42f);
            constexpr int k_puffs{ 8 };
            for (int puff = 0; puff < k_puffs; ++puff)
            {
                const float along = fractionalPart(cycleTime * 25.0f
                    + static_cast<float>(puff) / k_puffs);
                const float opacity = smoothStep(0.0f, 0.1f, along)
                    * (1.0f - smoothStep(0.42f, 1.0f, along));
                const FloatPoint center = {
                    56.0f + 77.5f * along * (1.0f + along),
                    -21.5f + (12.0f + 5.0f * std::sin(k_2Pi * along)) * along
                };
                drawGlow(machine, center, 5.0f + 21.0f * along,
                    Vapor.faded(0.72f * opacity * light), 0.62f);
            }
        }

        // Where the two hands are in one pose, in units about the pelvis.
        struct HandsAt
        {
            FloatPoint left;
            FloatPoint right;
        };

        [[nodiscard]] float moonwalkSteps(const float beatsSinceStart, const float passBeats)
        {
            const float passTime = std::clamp(beatsSinceStart, 0.0f, passBeats);
            const float fromTurn = std::min(passTime, passBeats - passTime);
            const float ease = fromTurn * (1.0f - smoothStep(0.0f, 0.3f, fromTurn));
            return passTime < passBeats * 0.5f ? passTime - ease : passTime + ease;
        }

        [[nodiscard]] HandsAt poseOnBeat(const int beat)
        {
            constexpr HandsAt k_leftUp{ { -104.0f, -152.0f }, { 104.0f, -50.0f } };
            constexpr HandsAt k_rightUp{ { -104.0f, -50.0f }, { 104.0f, -152.0f } };
            constexpr HandsAt k_loweredHands{ { -70.0f, -20.0f }, { 78.0f, -40.0f } };
            if (beat < 0)
            {
                return { ActHands::LeftOnHip, ActHands::RightOnHip };
            }
            if (beat < 2)
            {
                return beat % 2 == 0 ? k_leftUp : k_rightUp;
            }
            return k_loweredHands;
        }

        // The lifted heel turns around the center of the toe's rounded corner.
        [[nodiscard]] FloatPoint ankleOnFloor(const float x, const ActStage& stage,
            const float toeCenterX, const float footTurn)
        {
            constexpr float k_soleCornerRadius{ 6.0f };
            const FloatPoint toeCenter = { toeCenterX, AndroidBody::AnkleHeight - k_soleCornerRadius };
            const FloatPoint turnedToeCenter = rotated(toeCenter, footTurn);
            return {
                x + (toeCenter.x - turnedToeCenter.x) * stage.frame.unit,
                stage.floorY - (turnedToeCenter.y + k_soleCornerRadius) * stage.frame.unit
            };
        }

        void applyShuffle(AndroidPose& android, const float shuffleTime)
        {
            const float mix = smoothStep(0.0f, 0.20f, shuffleTime);
            const float kneePhase = std::numbers::pi_v<float> * shuffleTime / k_shuffleHalfCycleSeconds;
            const float kneeInset = 15.0f * mix * std::sin(kneePhase);
            android.left.kneeOffsetX = kneeInset;
            android.right.kneeOffsetX = -kneeInset;

            const float upperPhase = std::numbers::pi_v<float> * shuffleTime / k_beatSeconds;
            const float armSway = std::sin(upperPhase);
            android.bend = 0.10f * mix * armSway;
            android.left.elbowPosition.y += 25.0f * armSway;
            android.right.hand.y += 14.0f * armSway;
            android.headTurn -= android.bend;
            android.headOffsetX += 10.0f * std::sin(0.5f * upperPhase);
            android.headOffsetY -= 2.0f * (1.0f - std::cos(2.0f * upperPhase));
        }

        void applyFinalePose(AndroidPose& android, const ActStage& stage, const float amount,
            const float actTime)
        {
            const SceneFrame& frame = stage.frame;
            const float unit = frame.unit;
            const float phase = k_2Pi * (actTime - k_finalePoseSeconds) / k_finaleSwaySeconds;
            const float sway = std::sin(phase);
            const float pulse = std::sin(phase - 0.8f);
            const float armLift = 5.0f * pulse;
            const float centerX = android.pelvis.x;
            const float ankleY = stage.floorY - AndroidBody::AnkleHeight * unit;
            const FloatPoint leftAnkle = FloatPoint::between(
                androidBodyToFrame(frame, android, android.left.foot),
                { centerX - 59.0f * unit, ankleY }, amount);
            const FloatPoint rightAnkle = FloatPoint::between(
                androidBodyToFrame(frame, android, android.right.foot),
                { centerX + 31.0f * unit, ankleY }, amount);
            const FloatPoint pelvisShift = {
                8.0f + 2.0f * sway,
                18.0f + 1.8f * pulse,
            };
            android.pelvis += pelvisShift * (amount * unit);
            plantAndroidFeet(frame, android, leftAnkle, rightAnkle);
            android.left.kneeOffsetX = std::lerp(android.left.kneeOffsetX, -18.0f, amount);
            android.right.kneeOffsetX = std::lerp(android.right.kneeOffsetX, -48.0f, amount);

            android.bend = std::lerp(android.bend, 0.36f + 0.045f * sway, amount);
            android.bow = 0.24f * amount;
            android.left.hand = FloatPoint::between(android.left.hand, { -9.0f, -3.0f }, amount);
            android.left.elbowPosition = FloatPoint::between(android.left.elbowPosition,
                { -54.0f, -33.0f }, amount);
            const FloatPoint raisedHand = { 118.0f, -184.0f - armLift };
            android.right.hand = FloatPoint::between(android.right.hand, raisedHand, amount);
            android.right.elbowPosition = FloatPoint::between(AndroidBody::RightShoulder,
                raisedHand, 0.5f);
            android.right.elbowPositionBlend = amount;

            android.headTurn = std::lerp(android.headTurn, -0.14f - 0.05f * pulse, amount);
            android.headOffsetX = std::lerp(android.headOffsetX, -4.0f, amount);
            android.headOffsetY = std::lerp(android.headOffsetY, 12.0f, amount);
            android.look = std::lerp(android.look, -6.0f, amount);
            android.eyeOpening = std::lerp(android.eyeOpening, 0.55f, amount);
        }
    }

    void RobotDanceAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        m_smokeMachineFoot = marionetteStandOf(stage).foot + FloatPoint{ 14.0f * stage.frame.unit, 0.0f };
        m_floorY = stage.floorY;
        m_backdropY = stage.backdropY;
        m_blackout = 0.9f * smoothStep(8.62f, k_curtainClosedSeconds, actTime);
        m_closingLight = 0.24f * smoothStep(8.94f, 9.24f, actTime)
            * (1.0f - smoothStep(k_finaleLightFadeSeconds, k_curtainClosedSeconds, actTime));
        ActStage dancerStage = stage;
        dancerStage.frame.unit *= k_dancerScale;
        const SceneFrame& frame = dancerStage.frame;
        const float unit = frame.unit;
        const float beat = std::min(actTime / k_beatSeconds, k_moonwalkEndBeat);
        const int beatIndex = static_cast<int>(std::floor(beat));
        const float intoBeat = beat - static_cast<float>(beatIndex);
        const float swayAngle = k_2Pi * actTime / k_swaySeconds;
        const bool moonwalking = beat >= k_moonwalkStartBeat && beat < k_moonwalkEndBeat;
        const bool returning = beat >= k_moonwalkTurnBeat;
        const float travelDirection = returning ? -1.0f : 1.0f;
        const float moonwalk = smoothStep(1.4f, k_moonwalkStartBeat, beat);
        const float openingSway = std::sin(swayAngle) * (1.0f - smoothStep(1.4f, 2.0f, beat));
        const float sway = 16.0f * openingSway;
        const float dip = 7.0f * std::pow(std::cos(std::numbers::pi_v<float> * beat), 2.0f)
            * (1.0f - moonwalk);
        constexpr float k_outboundBeats{ k_moonwalkTurnBeat - k_moonwalkStartBeat };
        constexpr float k_returnBeats{ k_moonwalkEndBeat - k_moonwalkTurnBeat };
        const float outboundSteps = moonwalkSteps(beat - k_moonwalkStartBeat, k_outboundBeats);
        const float returnSteps = moonwalkSteps(beat - k_moonwalkTurnBeat, k_returnBeats);
        const float stepsAcrossStage = outboundSteps - returnSteps;
        const float stageOffset = k_moonwalkStride * (stepsAcrossStage - 0.5f * k_outboundBeats);
        const float passSteps = returning ? returnSteps : outboundSteps;
        const int stepIndex = static_cast<int>(std::floor(passSteps));
        const float intoStep = passSteps - static_cast<float>(stepIndex);
        const float centerX = stage.centerX + (30.0f * stage.closeness + stageOffset) * unit;
        m_android = {
            .pelvis = {
                centerX + sway * unit,
                stage.floorY - (AndroidBody::StandingHeight - dip) * unit
            },
            .left = { .wearsWhiteGlove = true },
            .right = { .palmRadius = 13.0f },
            .cycleTime = time.cycleTime,
            .wearsFedora = true,
            .wearsSequinJacket = true,
            .highlightFeet = true,
        };
        const bool leftSupports = (stepIndex % 2 == 0) != returning;
        const float leftStep = leftSupports ? intoStep : 1.0f - intoStep;
        const float footOffset = (0.5f - leftStep) * travelDirection * k_moonwalkStride * moonwalk;
        const float heelLift = moonwalking ? -0.75f * travelDirection
            * smoothStep(0.0f, 0.17f, intoStep) * (1.0f - smoothStep(0.83f, 1.0f, intoStep))
            : 0.0f;
        const float leftTurn = leftSupports ? heelLift : 0.0f;
        const float rightTurn = leftSupports ? 0.0f : heelLift;
        const float stance = std::lerp(22.0f, 34.0f, moonwalk);
        FloatPoint leftAnkle = ankleOnFloor(centerX + (footOffset - stance) * unit,
            dancerStage, returning ? 7.0f : -13.0f, leftTurn);
        FloatPoint rightAnkle = ankleOnFloor(centerX + (stance - footOffset) * unit,
            dancerStage, returning ? 13.0f : -7.0f, rightTurn);
        leftAnkle.y -= 9.0f * std::max(0.0f, openingSway) * unit;
        rightAnkle.y -= 9.0f * std::max(0.0f, -openingSway) * unit;
        plantAndroidFeet(frame, m_android, leftAnkle, rightAnkle);
        m_android.left.footTurn = leftTurn;
        m_android.right.footTurn = rightTurn;

        const HandsAt before = poseOnBeat(beatIndex - 1);
        const HandsAt now = poseOnBeat(beatIndex);
        const float snap = smoothStep(0.0f, 0.22f, intoBeat);
        const HandsAt hands = {
            FloatPoint::between(before.left, now.left, snap),
            FloatPoint::between(before.right, now.right, snap)
        };
        const float armSwing = 14.0f * std::sin(std::numbers::pi_v<float> * stepsAcrossStage);
        const HandsAt moonwalkHands = {
            { -142.0f, AndroidBody::RightShoulder.y },
            { 78.0f, -57.0f + armSwing }
        };
        const float gazeDirection = std::lerp(-1.0f, 1.0f,
            smoothStep(k_moonwalkTurnBeat - 0.55f, k_moonwalkTurnBeat + 0.1f, beat));
        const float turnSquint = smoothStep(k_moonwalkTurnBeat - 0.45f, k_moonwalkTurnBeat, beat)
            * (1.0f - smoothStep(k_moonwalkTurnBeat, k_moonwalkTurnBeat + 0.35f, beat));
        const float headBob = 2.0f * (1.0f - std::cos(k_2Pi * intoStep));
        m_android.headTurn = 0.24f * moonwalk;
        m_android.headOffsetX = 10.0f * moonwalk
            * std::sin(0.5f * std::numbers::pi_v<float> * stepsAcrossStage);
        m_android.headOffsetY = (8.0f - headBob) * moonwalk;
        m_android.look = 9.0f * (gazeDirection * moonwalk);
        m_android.eyeOpening = 1.0f - 0.72f * turnSquint;
        m_android.left.hand = FloatPoint::between(hands.left, moonwalkHands.left, moonwalk);
        m_android.right.hand = FloatPoint::between(hands.right, moonwalkHands.right, moonwalk);
        m_android.left.elbowPosition = {
            -98.0f,
            AndroidBody::RightShoulder.y + 25.0f * std::sin(std::numbers::pi_v<float> * beat)
        };
        m_android.left.elbowPositionBlend = moonwalk;
        const float shuffleTime = actTime - k_moonwalkEndBeat * k_beatSeconds;
        if (shuffleTime > 0.0f)
        {
            applyShuffle(m_android, shuffleTime);
            const float finale = smoothStep(8.10f, k_finalePoseSeconds, actTime);
            if (finale > 0.0f)
            {
                applyFinalePose(m_android, dancerStage, finale, actTime);
            }
        }
    }

    void RobotDanceAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        SceneFrame dancer = frame;
        dancer.unit *= k_dancerScale;
        const FloatRect stage = { 0.0f, 0.0f, frame.width, frame.height };
        const float light = 1.0f - m_blackout;
        Color stageWash = RobotDanceLighting::StageWash;
        stageWash.blend(stageWash.fullyOpaque(), m_blackout);
        fillBox(frame, stage, SolidColor{ stageWash });

        const float shadowX = m_android.pelvis.x + std::min(173.0f * unit, frame.width * 0.18f);
        const float lightX = std::lerp(frame.width * 0.56f, shadowX, 0.48f);
        drawGlow(frame, { lightX, m_backdropY - 126.0f * unit }, 255.0f * unit,
            RobotDanceLighting::Backlight.faded(light), 0.76f);
        const FloatRect backdrop = { 0.0f, 0.0f, frame.width, m_backdropY };
        frame.canvas.pushClip(frame.toCanvas.mapRect(backdrop));
        const SceneFrame projection = dancer.within(
            Matrix3x2::Translation(shadowX, m_backdropY - 98.0f * unit)
            * Matrix3x2::Scale(1.67f, 1.38f) * Matrix3x2::Translation(m_android.pelvis * -1.0f));
        drawAndroidSilhouette(projection, m_android, RobotDanceLighting::Shadow);
        frame.canvas.popClip();

        drawGlow(frame, { lightX, m_floorY - 38.0f * unit }, 78.0f * unit,
            RobotDanceLighting::FloorLight.faded(light), 0.43f);
        drawSmokeMachine(frame, m_smokeMachineFoot, m_android.cycleTime, light);
        drawFog(frame, m_android.cycleTime, FogBank{
            .y = m_floorY - 98.0f * unit,
            .driftsPerCycle = 1.4f,
            .color = RobotDanceLighting::BackSmoke.faded(light),
            .puffs = 3
        });
        drawAndroid(dancer, m_android);
        Color duskAbove = RobotDanceLighting::DuskAbove;
        Color duskBelow = RobotDanceLighting::DuskBelow;
        duskAbove.blend(duskAbove.fullyOpaque(), m_blackout);
        duskBelow.blend(duskBelow.fullyOpaque(), m_blackout);
        fillBox(frame, stage, LinearGradient::Simple({ 0.0f, m_backdropY - 125.0f * unit },
            { 0.0f, m_floorY }, duskAbove, duskBelow));
        const float frontLight = 0.55f * (1.0f - smoothStep(0.0f, 0.28f, m_blackout))
            + m_closingLight;
        if (frontLight > 0.0f)
        {
            const FloatRect androidBounds = {
                m_android.pelvis.x - 200.0f * dancer.unit,
                m_android.pelvis.y - 250.0f * dancer.unit,
                m_android.pelvis.x + 200.0f * dancer.unit,
                m_floorY + 20.0f * dancer.unit,
            };
            frame.canvas.pushClip(frame.toCanvas.mapRect(androidBounds));
            {
                const ScopedCanvasOpacity lighting{ frame.canvas, frontLight };
                drawAndroid(dancer, m_android);
            }
            frame.canvas.popClip();
        }
        drawFog(frame, m_android.cycleTime, FogBank{
            .y = m_floorY + 32.0f * unit,
            .driftsPerCycle = 1.8f,
            .start = 0.35f,
            .color = RobotDanceLighting::FrontSmoke.faded(light),
            .puffs = 3
        });
    }
}
