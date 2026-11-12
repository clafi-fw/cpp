module ClaFi.PathArt.AcrobaticsAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace AcrobaticsColors
    {
        constexpr Color SparkGold{ 0xFFFFD34D };
        constexpr Color SparkRed{ 0xFFFF8A7A };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // Feet apart and arms up in a V: the star it stands in between the tricks.
        constexpr float k_starHeight{ 113.0f };
        constexpr std::array<FloatPoint, 2> k_starFeet{ FloatPoint{ -45.0f, 99.0f },
            FloatPoint{ 45.0f, 99.0f } };
        constexpr std::array<FloatPoint, 2> k_lowHands{ FloatPoint{ -66.0f, -22.0f },
            FloatPoint{ 66.0f, -22.0f } };
        constexpr std::array<FloatPoint, 2> k_tuckedFeet{ FloatPoint{ -38.0f, 44.0f },
            FloatPoint{ 38.0f, 44.0f } };
        constexpr std::array<FloatPoint, 2> k_tuckedHands{ FloatPoint{ -54.0f, 22.0f },
            FloatPoint{ 54.0f, 22.0f } };
        // Upside down on the antenna, the hands either side of the head take the weight.
        constexpr std::array<FloatPoint, 2> k_tripodHands{ FloatPoint{ -62.0f, -206.0f },
            FloatPoint{ 62.0f, -206.0f } };
        constexpr std::array<FloatPoint, 2> k_spinHands{ FloatPoint{ -134.0f, -120.0f },
            FloatPoint{ 134.0f, -120.0f } };
        constexpr std::array<FloatPoint, 2> k_spinFeet{ FloatPoint{ -68.0f, 92.0f },
            FloatPoint{ 68.0f, 92.0f } };
        constexpr std::array<FloatPoint, 2> k_raisedHands{ ActHands::LeftRaised,
            ActHands::RightRaised };
        // How far it travels to the left going over onto its head, and again coming down.
        constexpr float k_overStep{ 70.0f };
        constexpr float k_crouch{ 34.0f };

        using HandOrFootPair = std::array<FloatPoint, 2>;

        [[nodiscard]] HandOrFootPair pairBetween(const HandOrFootPair& from,
            const HandOrFootPair& to, const float share)
        {
            return { pointBetween(from[0], to[0], share), pointBetween(from[1], to[1], share) };
        }

        void setHands(AndroidPose& pose, const HandOrFootPair& hands)
        {
            pose.left.hand = hands[0];
            pose.right.hand = hands[1];
        }

        void setFeet(AndroidPose& pose, const HandOrFootPair& feet)
        {
            pose.left.foot = feet[0];
            pose.right.foot = feet[1];
        }
    }

    // Seconds: the star to 0.9, the somersault to 2.4, the star to 3.0, over onto the head to
    // 3.9, the headspin to 7.0, down onto the feet to 7.9, then the ta-da.
    void AcrobaticsAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float floorY = stage.floorY;
        const float startX = stage.centerX + 70.0f * unit * stage.closeness;
        m_floorY = floorY;
        m_sparks = 0.0f;
        m_android = { .cycleTime = time.cycleTime };
        const auto standInStar = [&](float x, float dip, const HandOrFootPair& hands) {
            m_android.pelvis = { x, floorY - (k_starHeight - dip) * unit };
            setFeet(m_android, { k_starFeet[0] - FloatPoint{ 0.0f, dip },
                k_starFeet[1] - FloatPoint{ 0.0f, dip } });
            setHands(m_android, hands);
        };

        if (actTime < 0.9f)
            standInStar(startX, 4.0f * std::pow(std::sin(k_pi * actTime / 0.45f), 2.0f),
                k_raisedHands);
        else if (actTime < 2.4f)
        {
            // the side somersault: crouch, a tucked turn high in the air, land in a crouch
            const float share = (actTime - 0.9f) / 1.5f;
            if (share < 0.2f)
            {
                const float down = smoothStep(0.0f, 0.2f, share);
                standInStar(startX, k_crouch * down, pairBetween(k_raisedHands, k_lowHands, down));
            }
            else if (share < 0.78f)
            {
                const float flight = (share - 0.2f) / 0.58f;
                const float tuck = std::sin(k_pi * flight);
                const float height = k_starHeight - k_crouch
                    + 180.0f * 4.0f * flight * (1.0f - flight);
                m_android.tilt = -k_2Pi * smoothStep(0.0f, 1.0f, flight);
                m_android.pelvis = { startX, floorY - height * unit };
                const HandOrFootPair crouchedFeet = {
                    k_starFeet[0] - FloatPoint{ 0.0f, k_crouch },
                    k_starFeet[1] - FloatPoint{ 0.0f, k_crouch }
                };
                setFeet(m_android, pairBetween(crouchedFeet, k_tuckedFeet, tuck));
                setHands(m_android, pairBetween(k_lowHands, k_tuckedHands, tuck));
            }
            else
            {
                const float up = smoothStep(0.0f, 1.0f, (share - 0.78f) / 0.22f);
                standInStar(startX, k_crouch * (1.0f - up),
                    pairBetween(k_lowHands, k_raisedHands, up));
            }
        }
        else if (actTime < 3.0f)
            standInStar(startX, 0.0f, k_raisedHands);
        else if (actTime < 3.9f)
        {
            const float over = smoothStep(0.0f, 1.0f, (actTime - 3.0f) / 0.9f);
            m_android.tilt = -k_pi * over;
            m_android.pelvis.x = startX - k_overStep * unit * over;
            setFeet(m_android, k_starFeet);
            setHands(m_android,
                pairBetween(k_raisedHands, k_tripodHands, smoothStep(0.0f, 0.6f, over)));
            restAndroidOnFloor(frame, m_android, floorY);
        }
        else if (actTime < 7.0f)
        {
            // upside down on the antenna; the hands come off and it spins three times
            const float share = std::clamp((actTime - 4.3f) / 2.7f, 0.0f, 1.0f);
            const float out = smoothStep(0.0f, 0.12f, share)
                * (1.0f - smoothStep(0.88f, 1.0f, share));
            m_spinTurns = 3.0f * (share - std::sin(k_2Pi * share) / k_2Pi);
            m_android.facing = std::cos(k_2Pi * m_spinTurns);
            m_android.tilt = -k_pi + 0.05f * std::sin(k_2Pi * m_spinTurns) * out;
            const float wiggle = actTime < 4.3f
                ? 6.0f * std::sin(k_2Pi * (actTime - 3.9f) / 0.4f)
                : 0.0f;
            const HandOrFootPair upFeet = {
                FloatPoint{ -45.0f - wiggle, 99.0f },
                FloatPoint{ 45.0f - wiggle, 99.0f }
            };
            setFeet(m_android, pairBetween(upFeet, k_spinFeet, out));
            setHands(m_android, pairBetween(k_tripodHands, k_spinHands, out));
            m_android.pelvis = {
                startX - k_overStep * unit,
                floorY + AndroidBody::AntennaTip.y * unit
            };
            m_android.antenna = AntennaLight::On;
            if (out > 0.2f)
                m_sparks = out;
        }
        else if (actTime < 7.9f)
        {
            const float down = smoothStep(0.0f, 1.0f, (actTime - 7.0f) / 0.9f);
            m_android.tilt = -k_pi - k_pi * down;
            m_android.pelvis.x = startX - k_overStep * unit * (1.0f + down);
            setFeet(m_android, k_starFeet);
            setHands(m_android,
                pairBetween(k_tripodHands, k_raisedHands, smoothStep(0.4f, 1.0f, down)));
            restAndroidOnFloor(frame, m_android, floorY);
        }
        else
        {
            const float landing = std::clamp((actTime - 7.9f) / 0.5f, 0.0f, 1.0f);
            standInStar(startX - 2.0f * k_overStep * unit,
                5.0f * std::pow(std::sin(k_pi * landing), 2.0f), k_raisedHands);
            m_android.lights = flashingLights(actTime);
        }
    }

    // Sparks fly from where the antenna spins on the floor, gold and red.
    void AcrobaticsAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
        if (m_sparks <= 0.0f)
            return;
        const float unit = frame.unit;
        const FloatPoint spin = { m_android.pelvis.x, m_floorY - 2.0f * unit };
        for (int i = 0; i != 6; ++i)
        {
            const float life = fractionalPart(m_spinTurns * 1.5f + static_cast<float>(i) / 6.0f);
            const float angle = k_2Pi * (static_cast<float>(i) / 6.0f + m_spinTurns * 0.37f);
            const float reach = (10.0f + 60.0f * life) * unit;
            const FloatPoint at = spin + FloatPoint{
                std::cos(angle) * reach,
                -22.0f * unit * life * (1.0f + pseudoRandom(i, 9.0f))
            };
            const Color color = i % 2 == 1
                ? AcrobaticsColors::SparkGold
                : AcrobaticsColors::SparkRed;
            drawSparkle(frame, at, 5.0f * unit * (1.0f - life),
                faded(color, (1.0f - life) * m_sparks));
        }
    }
}
