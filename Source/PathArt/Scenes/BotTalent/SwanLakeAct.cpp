module ClaFi.PathArt.SwanLakeAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.TutuSprite;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        constexpr float k_beatSeconds{ 0.3125f };
        // How far apart the dancers stand, in units, in a frame at least 640 units wide.
        constexpr float k_dancerSpacing{ 112.0f };
        // How far each dancer runs behind the music, in seconds, so they keep almost together.
        constexpr std::array<float, 4> k_lateness{ 0.0f, 0.035f, -0.03f, 0.07f };
        // The dancer that turns its head the wrong way once, and the turn it gets wrong.
        constexpr std::size_t k_wrongWayDancer{ 2 };
        constexpr int k_wrongWayFlick{ 5 };
        // A foot's turn to stand on the tip of its shoe, and how much higher the ankle stands.
        constexpr float k_pointe{ 1.45f };
        constexpr float k_pointeRise{ 6.0f };
        // The joined hands hang this far above the middle between two dancers' pelvises.
        constexpr float k_handsHeight{ 64.0f };
        constexpr float k_flickTurn{ 0.14f }; // the heads' sharp turn either way, in radians
        // The same tutu four times, each a little off: a shade, a size, a set on the hips.
        constexpr std::array<TutuLook, 4> k_tutus{ {
            {},
            { .tint = Color{ 0xFFFFFFFF }, .tintShare = 0.35f, .size = 0.97f, .tilt = 0.03f },
            { .tint = Color{ 0xFFFFB08A }, .tintShare = 0.3f, .size = 1.03f, .tilt = -0.025f },
            { .tint = Color{ 0xFFC8A2F0 }, .tintShare = 0.28f, .size = 0.99f, .tilt = 0.02f }
        } };

        // One dancer at its own time into the act, its hands left to the line.
        [[nodiscard]] AndroidPose dancerAt(const SceneFrame& frame, const FloatPoint floorPoint,
            const float actTime, const float cycleTime, const bool wrongWay)
        {
            const float unit = frame.unit;
            AndroidPose dancer = {
                .lights = ChestLights::Pink,
                .wearsTutu = true,
                .wearsPointeShoes = true,
                .cycleTime = cycleTime
            };
            const float beat = actTime / k_beatSeconds;
            const float up = smoothStep(0.85f, 1.05f, actTime);
            float rise = k_pointeRise * up;
            float lifted = 0.0f;
            float apart = 7.0f;
            float dip = 0.0f;
            float leftTrembling = 0.0f;
            float rightTrembling = 0.0f;
            bool leftLifts = false;
            if ((actTime >= 1.0f && actTime < 4.75f) || actTime >= 8.1f)
            {
                // a step a beat, the feet taking turns to lift to the other shin
                lifted = std::pow(std::sin(k_pi * fractionalPart(beat)), 2.0f);
                leftLifts = static_cast<int>(std::floor(beat)) % 2 == 0;
            }
            else if (actTime >= 4.75f && actTime < 6.0f)
            {
                // echappes: out to second on flat feet in a plie, then back to fifth on pointe
                const float echappe = (actTime - 4.75f) / k_beatSeconds;
                const float within = smoothStep(0.0f, 0.25f, fractionalPart(echappe));
                const bool opening = static_cast<int>(std::floor(echappe)) % 2 == 0;
                const float open = opening ? within : 1.0f - within;
                apart = std::lerp(7.0f, 32.0f, open);
                dip = 10.0f * open;
                rise = k_pointeRise * (1.0f - open) + 8.0f * std::sin(k_pi * within);
            }
            else if (actTime >= 6.0f && actTime < 8.1f)
            {
                // a bourree, the feet trembling on their points
                const float step = std::sin(k_2Pi * actTime * 9.0f)
                    * smoothStep(6.0f, 6.1f, actTime) * (1.0f - smoothStep(7.95f, 8.1f, actTime));
                leftTrembling = 4.0f * std::max(0.0f, step);
                rightTrembling = 4.0f * std::max(0.0f, -step);
            }

            const float pelvisHeight = AndroidBody::StandingHeight + rise - dip + 3.0f * lifted;
            dancer.pelvis = { floorPoint.x, floorPoint.y - pelvisHeight * unit };
            const float feetY = pelvisHeight - AndroidBody::AnkleHeight - rise;
            dancer.left.foot = { -apart, feetY - leftTrembling };
            dancer.right.foot = { apart, feetY - rightTrembling };
            const float pointeTurn = k_pointe * std::max(0.0f, rise) / k_pointeRise;
            dancer.left.footTurn = -std::min(pointeTurn, k_pointe);
            dancer.right.footTurn = std::min(pointeTurn, k_pointe);
            AndroidSide& lifting = leftLifts ? dancer.left : dancer.right;
            const float side = leftLifts ? -1.0f : 1.0f;
            lifting.foot = FloatPoint::between(lifting.foot, { -3.0f * side, 70.0f }, lifted);
            lifting.footTurn = std::lerp(lifting.footTurn, 2.36f * side, lifted);

            // the heads turn sharply every two beats, the right way first
            const float flicking = smoothStep(1.0f, 1.15f, actTime)
                * (1.0f - smoothStep(4.6f, 4.75f, actTime))
                + smoothStep(6.0f, 6.15f, actTime);
            const float flicks = actTime / (2.0f * k_beatSeconds);
            const int flick = static_cast<int>(std::floor(flicks));
            const auto sideOf = [&](const int turn) {
                const float way = turn % 2 == 0 ? 1.0f : -1.0f;
                return wrongWay && turn == k_wrongWayFlick ? -way : way;
            };
            const float turned = std::lerp(sideOf(flick - 1), sideOf(flick),
                smoothStep(0.0f, 0.12f, fractionalPart(flicks)));
            dancer.headTurn = k_flickTurn * turned * flicking;
            dancer.look = 5.0f * turned * flicking;
            return dancer;
        }
    }

    // Seconds: on pointe from 1, a step a beat with the feet lifting in turn as the line
    // travels right to 4.75, the heads turning every two beats; two echappes to 6; a bourree
    // back to the left to 8.1, the heads turning again; then the steps again, on into the
    // curtain.
    void SwanLakeAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float spacing = k_dancerSpacing * stage.closeness * unit;
        for (std::size_t i = 0; i != m_dancers.size(); ++i)
        {
            const float dancerTime = actTime - k_lateness[i];
            const float travel = 50.0f * smoothStep(1.0f, 4.75f, dancerTime)
                - 70.0f * smoothStep(6.0f, 8.1f, dancerTime)
                + 40.0f * smoothStep(8.1f, 10.0f, dancerTime);
            const FloatPoint floorPoint = {
                stage.centerX + (travel * stage.closeness) * unit
                    + (static_cast<float>(i) - 1.5f) * spacing,
                stage.floorY
            };
            m_dancers[i] = dancerAt(frame, floorPoint, dancerTime, time.cycleTime,
                i == k_wrongWayDancer);
            m_dancers[i].tutuLook = k_tutus[i];
        }

        // the arms crossed in front, each hand holding a neighbor's across the gap between them
        for (std::size_t i = 0; i != m_dancers.size(); ++i)
        {
            AndroidPose& dancer = m_dancers[i];
            dancer.left.elbow = ElbowBend::Down;
            dancer.right.elbow = ElbowBend::Down;
            dancer.left.handShape = HandShape::Fist;
            dancer.right.handShape = HandShape::Fist;
            const auto holdWith = [&](const AndroidPose& neighbor) {
                const FloatPoint between = FloatPoint::between(dancer.pelvis, neighbor.pelvis, 0.5f);
                return androidFrameToUpper(frame, dancer,
                    between - FloatPoint{ 0.0f, k_handsHeight * unit });
            };
            dancer.left.hand = i + 1 < m_dancers.size()
                ? holdWith(m_dancers[i + 1])
                : ActHands::LeftOnHip;
            dancer.right.hand = i > 0 ? holdWith(m_dancers[i - 1]) : ActHands::RightOnHip;
        }
        m_spotlightX = FloatPoint::between(m_dancers.front().pelvis, m_dancers.back().pelvis, 0.5f).x;
    }

    void SwanLakeAct::draw(const SceneFrame& frame) const
    {
        for (const AndroidPose& dancer : m_dancers)
            drawAndroid(frame, dancer);
    }
}
