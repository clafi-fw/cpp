module ClaFi.PathArt.JugglingAct;

import ClaFi.PathArt.AndroidSprite;
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

    namespace JugglingColors
    {
        constexpr std::array<Color, 3> Balls{ Color{ 0xFF2FC4D8 }, Color{ 0xFFF2B632 },
            Color{ 0xFFEF4F73 } };
        constexpr Color Rim{ 0xD93B4450 };
        constexpr Color Shine{ 0xB3FFFFFF };
    }

    namespace
    {
        // A throw every 0.42 seconds, the hands in turn; a ball rests 0.3 seconds in a hand.
        constexpr float k_throwSeconds{ 0.42f };
        constexpr float k_holdSeconds{ 0.3f };
        constexpr float k_flightSeconds{ 3.0f * k_throwSeconds - k_holdSeconds };
        constexpr float k_throwHeight{ 235.0f };
        // A hand catches out wide and throws from near the middle.
        constexpr float k_catchX{ 66.0f };
        constexpr float k_throwX{ 22.0f };
        constexpr float k_handY{ -26.0f };
        // A ball sits this far above the hand holding it.
        constexpr float k_ballOverHand{ 11.0f };

        // A hand's path over two throws: out empty to catch, in with the ball, dipping, to throw.
        [[nodiscard]] FloatPoint handAt(const int hand, const float time)
        {
            const float side = hand == 0 ? -1.0f : 1.0f;
            const float period = 2.0f * k_throwSeconds;
            const float sinceFirstThrow = time - static_cast<float>(hand) * k_throwSeconds;
            const float phase = std::fmod(std::fmod(sinceFirstThrow, period) + period, period);
            const float pi = std::numbers::pi_v<float>;
            if (phase >= period - k_holdSeconds)
            {
                const float carried = (phase - (period - k_holdSeconds)) / k_holdSeconds;
                return {
                    side * std::lerp(k_catchX, k_throwX, carried),
                    k_handY + 14.0f * std::sin(pi * carried)
                };
            }
            const float returned = phase / (period - k_holdSeconds);
            return {
                side * std::lerp(k_throwX, k_catchX, returned),
                k_handY - 8.0f * std::sin(pi * returned)
            };
        }
    }

    // Throw n goes from hand n mod 2 and carries ball n mod 3, so each ball changes hands every
    // throw it makes.
    void JugglingAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float tau = actTime + 6.0f * k_throwSeconds;
        const float bounce = 3.0f
            * std::pow(std::sin(std::numbers::pi_v<float> * tau / k_throwSeconds), 2.0f);
        const float x = stage.centerX + 30.0f * unit * stage.closeness;
        m_android = standingAndroid(frame, { x, stage.floorY }, time.cycleTime, bounce, 22.0f);

        const FloatPoint overHand = { 0.0f, -k_ballOverHand };
        for (int ball = 0; ball != 3; ++ball)
        {
            int throwIndex = static_cast<int>(std::floor(tau / k_throwSeconds));
            while (throwIndex % 3 != ball)
                --throwIndex;
            const int thrower = throwIndex % 2;
            const float thrownAt = static_cast<float>(throwIndex) * k_throwSeconds;
            const float flown = tau - thrownAt;
            if (flown >= k_flightSeconds)
            {
                m_balls[static_cast<std::size_t>(ball)] = handAt(1 - thrower, tau) + overHand;
                continue;
            }
            const float share = flown / k_flightSeconds;
            const FloatPoint from = handAt(thrower, thrownAt) + overHand;
            const FloatPoint to = handAt(1 - thrower, thrownAt + k_flightSeconds) + overHand;
            FloatPoint at = pointBetween(from, to, share);
            at.y -= 4.0f * k_throwHeight * share * (1.0f - share);
            m_balls[static_cast<std::size_t>(ball)] = at;
        }
        m_android.left.hand = handAt(0, tau);
        m_android.right.hand = handAt(1, tau);
        const FloatPoint highest = *std::ranges::min_element(m_balls, {}, &FloatPoint::y);
        m_android.look = std::clamp(highest.x / 9.0f, -6.0f, 6.0f) - 2.0f;
        m_android.headTurn = std::clamp(highest.x / 900.0f, -0.06f, 0.06f);
    }

    void JugglingAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
        const float unit = frame.unit;
        for (std::size_t i = 0; i != m_balls.size(); ++i)
        {
            const FloatPoint at = androidUpperToFrame(frame, m_android, m_balls[i]);
            fillDisc(frame, at, 12.0f * unit, SolidColor{ JugglingColors::Rim });
            fillDisc(frame, at, 10.6f * unit, SolidColor{ JugglingColors::Balls[i] });
            fillDisc(frame, at - FloatPoint{ 3.5f, 3.5f } * unit, 3.2f * unit,
                SolidColor{ JugglingColors::Shine });
        }
    }
}
