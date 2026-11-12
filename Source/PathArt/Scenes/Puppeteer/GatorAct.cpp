module ClaFi.PathArt.GatorAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.GatorSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // The gape at its widest, which the android aims its head into.
        constexpr float k_fullLift{ 0.55f };
        constexpr float k_fullGape{ 1.12f };
        // How far into the mouth the middle of the android's head goes, in the gator's units.
        constexpr float k_headDepth{ 72.0f };
        // Kneeling, the pelvis is this high above the floor.
        constexpr float k_kneelingHeight{ 55.0f };
        constexpr float k_snapAt{ 6.02f };
    }

    // Seconds: the gape from 1.3, kneeling and leaning in from 2.3, a hand on the snout, no hands
    // from 3.9, the jaw trembling, out at 5.6 a moment before the snap, the brow wiped, the ta-da.
    void GatorAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float floorY = stage.floorY;

        const float gape = smoothStep(1.3f, 2.3f, actTime);
        const float snap = smoothStep(5.95f, 6.08f, actTime);
        const float tremble = smoothStep(4.4f, 4.6f, actTime)
            * (1.0f - smoothStep(5.6f, 5.8f, actTime)) * 0.045f * std::sin(k_2Pi * actTime * 9.0f);
        m_gator = {
            .neck = { stage.centerX - 40.0f * unit, floorY },
            .size = 1.05f * unit,
            .headLift = k_fullLift * gape * (1.0f - 0.6f * snap)
                + 0.2f * snap * (1.0f - smoothStep(6.6f, 7.6f, actTime)),
            .gape = std::max(0.0f, k_fullGape * gape * (1.0f - snap) + tremble
                - 0.12f * smoothStep(5.6f, 5.95f, actTime)),
            .tailDrop = 6.0f * std::sin(k_2Pi * actTime / 3.1f),
            .glance = actTime > 3.4f && actTime < 6.0f ? 3.0f : (actTime > 6.1f ? -2.0f : 1.0f),
            .lid = actTime > 6.4f ? 0.5f : (fractionalPart(actTime / 3.3f) > 0.96f ? 1.0f : 0.0f)
        };

        // kneel and lean the upper body over until the middle of the head stands in the mouth
        GatorPose wideOpen = m_gator;
        wideOpen.headLift = k_fullLift;
        wideOpen.gape = k_fullGape;
        const FloatPoint mouth = gatorMouthPoint(wideOpen, k_headDepth);
        const float headAboveWaist = AndroidBody::Waist - AndroidBody::HeadMiddle.y;
        const float kneelingPelvisY = floorY - k_kneelingHeight * unit;
        const float waistY = kneelingPelvisY + AndroidBody::Waist * unit;
        const float reachUp = std::clamp((waistY - mouth.y) / (headAboveWaist * unit), -1.0f, 1.0f);
        const float lean = -std::acos(reachUp);
        const float kneelingX = mouth.x - headAboveWaist * unit * std::sin(lean);
        const float standingX = m_gator.neck.x + 220.0f * m_gator.size;
        const float leaningIn = smoothStep(2.3f, 3.4f, actTime)
            * (1.0f - smoothStep(5.6f, 5.9f, actTime));
        const float kneeling = smoothStep(2.3f, 2.9f, actTime)
            * (1.0f - smoothStep(6.1f, 6.8f, actTime));
        const float pelvisX = std::lerp(standingX, kneelingX, kneeling);
        m_android = {
            .pelvis = {
                pelvisX,
                std::lerp(floorY - AndroidBody::StandingHeight * unit, kneelingPelvisY, kneeling)
            },
            .bend = lean * leaningIn,
            .kneeling = kneeling > 0.75f,
            .headTurn = -0.25f * leaningIn,
            .cycleTime = time.cycleTime
        };
        if (m_android.kneeling)
        {
            plantAndroidFeet(frame, m_android, { pelvisX - 26.0f * unit, floorY - 9.0f * unit },
                { pelvisX + 26.0f * unit, floorY - 9.0f * unit });
        }
        else
        {
            const float ankleY = floorY - AndroidBody::AnkleHeight * unit;
            plantAndroidFeet(frame, m_android, { pelvisX - 19.0f * unit, ankleY },
                { pelvisX + 19.0f * unit, ankleY });
        }

        // hands: one presenting the gator, one up on the snout, then arms out with no hands at
        // all, which hold their place as the audience sees it whatever the lean
        const FloatPoint snout = androidFrameToUpper(frame, m_android,
            gatorUpperJawToFrame(m_gator, { 140.0f, -10.0f }));
        const float presenting = smoothStep(0.2f, 0.8f, actTime)
            * (1.0f - smoothStep(2.2f, 2.6f, actTime));
        const float holding = smoothStep(2.6f, 3.0f, actTime)
            * (1.0f - smoothStep(3.9f, 4.2f, actTime));
        const float noHands = smoothStep(3.9f, 4.3f, actTime)
            * (1.0f - smoothStep(5.5f, 5.8f, actTime));
        const float wipingBrow = smoothStep(6.5f, 6.9f, actTime)
            * (1.0f - smoothStep(7.5f, 7.9f, actTime));
        const float tada = smoothStep(7.9f, 8.4f, actTime);
        const auto asSeen = [&](FloatPoint shoulder, FloatPoint reach) {
            return shoulder + rotated(reach, -m_android.bend);
        };
        const FloatPoint leftShoulder = {
            -AndroidBody::RightShoulder.x,
            AndroidBody::RightShoulder.y,
        };
        FloatPoint leftHand = pointBetween(ActHands::LeftOnHip, { -120.0f, -96.0f }, presenting);
        leftHand = pointBetween(leftHand, snout, holding);
        leftHand = pointBetween(leftHand, asSeen(leftShoulder, { 50.0f, -95.0f }), noHands);
        FloatPoint rightHand = pointBetween(ActHands::RightOnHip,
            asSeen(AndroidBody::RightShoulder, { 96.0f, -44.0f }), noHands);
        rightHand = pointBetween(rightHand, { 14.0f, -176.0f }, wipingBrow);
        m_android.left.hand = pointBetween(leftHand, ActHands::LeftRaised, tada);
        m_android.left.handShape = holding > 0.5f ? HandShape::Fist : HandShape::Open;
        m_android.left.forearmOnTop = holding > 0.05f;
        m_android.right.hand = pointBetween(rightHand, ActHands::RightRaised, tada);
        m_android.look = actTime < 2.3f ? -5.0f : (actTime > 6.2f && actTime < 7.9f ? -6.0f : 0.0f);
        if (noHands > 0.5f)
        {
            m_android.lights = flashingLights(actTime * 2.0f);
            m_android.antenna = fractionalPart(actTime * 6.0f) < 0.5f
                ? AntennaLight::On
                : AntennaLight::Off;
        }
        if (actTime > 8.4f)
            m_android.lights = flashingLights(actTime);
        m_chomp = (actTime - k_snapAt) / 0.5f;
        m_spotlightX = std::lerp(m_gator.neck.x + 120.0f * m_gator.size, pelvisX, 0.5f);
    }

    // The android's head goes between the gator's jaws: the upper jaw and the teeth over it.
    void GatorAct::draw(const SceneFrame& frame) const
    {
        drawGator(frame, m_gator);
        drawAndroid(frame, m_android);
        drawGatorUpperJaw(frame, m_gator);
        if (m_android.left.forearmOnTop)
            drawAndroidForearm(frame, m_android, AudienceSide::Left);
        if (m_chomp <= 0.0f || m_chomp >= 1.0f)
            return;
        const float unit = frame.unit;
        const FloatPoint tip = gatorHeadToFrame(m_gator, { 152.0f, 0.0f });
        PixelPath lines;
        for (int i = 0; i != 5; ++i)
        {
            const float angle = -k_pi * 0.85f + static_cast<float>(i) * 0.35f;
            const FloatPoint direction = { std::cos(angle), std::sin(angle) };
            const float inner = (12.0f + 26.0f * m_chomp) * unit;
            const float outer = inner + 12.0f * unit * (1.0f - m_chomp);
            lines.moveTo(tip + direction * inner);
            lines.lineTo(tip + direction * outer);
        }
        strokeShape(frame, lines, SolidColor{ MagicColors::White.withOpacity(1.0f - m_chomp) },
            2.0f * unit);
    }
}
