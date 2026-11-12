module ClaFi.PathArt.RobotDanceAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        // A beat is a quarter of a sway, which is a quarter of the act's ten seconds.
        constexpr float k_beatSeconds{ 0.625f };
        constexpr float k_swaySeconds{ 2.5f };

        // Where the two hands are in one pose, in units about the pelvis.
        struct HandsAt
        {
            FloatPoint left;
            FloatPoint right;
        };

        // The arm wave runs from the left hand to the right, a beat behind, over two beats.
        [[nodiscard]] HandsAt waveAt(const float beat)
        {
            const float wave = k_2Pi * (beat - 8.0f) / 2.0f;
            return {
                { -150.0f, -101.0f + 24.0f * std::sin(wave) },
                { 150.0f, -101.0f + 24.0f * std::sin(wave - 2.0f) }
            };
        }

        // The pose struck on a beat: elbows up and down in turn, hands together, a point to
        // each side, the wave, arms up for the spin, the V.
        [[nodiscard]] HandsAt poseOnBeat(const int beat)
        {
            constexpr HandsAt k_leftUp{ { -104.0f, -152.0f }, { 104.0f, -50.0f } };
            constexpr HandsAt k_rightUp{ { -104.0f, -50.0f }, { 104.0f, -152.0f } };
            constexpr HandsAt k_together{ { -12.0f, -74.0f }, { 12.0f, -74.0f } };
            constexpr HandsAt k_pointRight{ ActHands::LeftOnHip, { 123.0f, -177.0f } };
            constexpr HandsAt k_pointLeft{ { -123.0f, -177.0f }, ActHands::RightOnHip };
            constexpr HandsAt k_up{ { -40.0f, -205.0f }, { 40.0f, -205.0f } };
            if (beat < 0)
                return { ActHands::LeftOnHip, ActHands::RightOnHip };
            if (beat < 4)
                return beat % 2 == 0 ? k_leftUp : k_rightUp;
            if (beat < 6)
                return k_together;
            if (beat == 6)
                return k_pointRight;
            if (beat == 7)
                return k_pointLeft;
            if (beat < 12)
                return waveAt(static_cast<float>(beat));
            if (beat < 14)
                return k_up;
            return { ActHands::LeftRaised, ActHands::RightRaised };
        }
    }

    // Beats 0 to 8 snap through the poses, 8 to 12 wave, 12 to 14 spin, then the V.
    void RobotDanceAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float beat = actTime / k_beatSeconds;
        const int beatIndex = static_cast<int>(std::floor(beat));
        const float intoBeat = beat - static_cast<float>(beatIndex);
        const float swayAngle = k_2Pi * actTime / k_swaySeconds;
        const bool spinning = beat >= 12.0f && beat < 14.0f;
        const float settle = 1.0f - smoothStep(11.6f, 12.0f, beat);
        const float sway = 16.0f * std::sin(swayAngle) * settle;
        const float dip = 7.0f * std::pow(std::cos(std::numbers::pi_v<float> * beat), 2.0f)
            * (spinning ? 0.3f : 1.0f);
        const float centerX = stage.centerX + 30.0f * unit * stage.closeness;

        m_android = {
            .pelvis = {
                centerX + sway * unit,
                stage.floorY - (AndroidBody::StandingHeight - dip) * unit
            },
            .facing = spinning ? std::cos(std::numbers::pi_v<float> * (beat - 12.0f)) : 1.0f,
            .cycleTime = time.cycleTime
        };
        const float liftLeft = std::max(0.0f, std::sin(swayAngle)) * 9.0f * settle;
        const float liftRight = std::max(0.0f, -std::sin(swayAngle)) * 9.0f * settle;
        const float ankleY = stage.floorY - AndroidBody::AnkleHeight * unit;
        plantAndroidFeet(frame, m_android, { centerX - 22.0f * unit, ankleY - liftLeft * unit },
            { centerX + 22.0f * unit, ankleY - liftRight * unit });
        // Turning, the feet turn with the body.
        if (spinning)
        {
            m_android.left.foot = { -19.0f, 103.0f - dip };
            m_android.right.foot = { 19.0f, 103.0f - dip };
        }
        m_android.left.footTurn = 0.02f * liftLeft;
        m_android.right.footTurn = -0.02f * liftRight;

        HandsAt hands{};
        if (beat >= 8.0f && beat < 12.0f)
        {
            const float wave = k_2Pi * (beat - 8.0f) / 2.0f;
            const HandsAt waving = waveAt(beat);
            const HandsAt pointing = poseOnBeat(7);
            const float into = smoothStep(8.0f, 8.3f, beat);
            hands = {
                pointBetween(pointing.left, waving.left, into),
                pointBetween(pointing.right, waving.right, into)
            };
            m_android.headTurn = 0.08f * std::sin(wave - 1.0f);
            m_android.look = 4.0f * std::sin(wave - 1.0f);
        }
        else
        {
            const HandsAt before = poseOnBeat(beatIndex - 1);
            const HandsAt now = poseOnBeat(beatIndex);
            const float snap = smoothStep(0.0f, 0.22f, intoBeat);
            hands = {
                pointBetween(before.left, now.left, snap),
                pointBetween(before.right, now.right, snap)
            };
            if (beatIndex < 8)
            {
                const float tick = beatIndex % 2 == 0 ? -1.0f : 1.0f;
                m_android.headTurn = 0.1f * tick;
                m_android.look = 4.0f * tick;
            }
        }
        const ElbowBend elbows = beatIndex >= 4 && beatIndex < 6 ? ElbowBend::Down : ElbowBend::Out;
        m_android.left.hand = hands.left;
        m_android.left.elbow = elbows;
        m_android.right.hand = hands.right;
        m_android.right.elbow = elbows;
        if (beat >= 14.0f)
            m_android.lights = flashingLights(actTime);
    }

    void RobotDanceAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
    }
}
