module ClaFi.PathArt.JugglingAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
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
        // The pull that brings a thrown ball down, in units a second each second.
        constexpr float k_gravity{ 8.0f * k_throwHeight / (k_flightSeconds * k_flightSeconds) };
        // Throw 18 goes too far; the leg kicks its ball back as throw 24. See PathArt#jugglingact
        constexpr int k_wildThrow{ 18 };
        constexpr int k_kickedThrow{ k_wildThrow + 6 };
        constexpr std::size_t k_wildBall{ static_cast<std::size_t>(k_wildThrow % 3) };
        constexpr float k_wildThrownAt{ static_cast<float>(k_wildThrow) * k_throwSeconds };
        constexpr float k_wildFlightSeconds{ 1.05f };
        constexpr float k_landedAt{ k_wildThrownAt + k_wildFlightSeconds };
        constexpr float k_rollSeconds{ 0.18f };
        constexpr float k_kickedAt{ static_cast<float>(k_kickedThrow) * k_throwSeconds };
        constexpr float k_backInHandAt{ k_kickedAt + k_flightSeconds };
        // In the lunge the pelvis goes this far right and down, in units, the upper body leaning.
        constexpr FloatPoint k_lungeShift{ 62.0f, 50.0f };
        constexpr float k_lungeBend{ -0.12f };
        // The right ankle in the lunge, the leg out nearly straight, its heel down and toes up.
        constexpr FloatPoint k_lungeAnkle{ 100.6f, 51.8f };
        constexpr float k_lungeFootTurn{ -1.35f };
        constexpr float k_kickLift{ 22.0f }; // how far the ankle rises to kick, in units
        // The wild ball about the right ankle: landing on the shin, then at rest against the toes.
        constexpr FloatPoint k_ballLanding{ -37.0f, -44.0f };
        constexpr FloatPoint k_ballOnShin{ -15.0f, -32.0f };
        // The hands out for balance while they hold the other two balls.
        constexpr FloatPoint k_balancingLeftHand{ -88.0f, -58.0f };
        constexpr FloatPoint k_balancingRightHand{ 80.0f, -70.0f };

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

        // The first throw a hand holds back while the wild ball is away. The hand keeps that
        // throw's ball from catching it until the throw six later.
        [[nodiscard]] constexpr int withheldThrow(const int hand)
        {
            return k_wildThrow + 1 + (k_wildThrow + 1 + hand) % 2;
        }

        [[nodiscard]] constexpr float holdingFrom(const int hand)
        {
            return static_cast<float>(withheldThrow(hand) - 3) * k_throwSeconds + k_flightSeconds;
        }

        [[nodiscard]] constexpr float holdingUntil(const int hand)
        {
            return static_cast<float>(withheldThrow(hand) + 6) * k_throwSeconds;
        }

        // A hand on its way through the cascade, or out for balance while it holds its ball.
        [[nodiscard]] FloatPoint jugglingHandAt(const int hand, const float tau)
        {
            const FloatPoint balancing = hand == 0 ? k_balancingLeftHand : k_balancingRightHand;
            const float holding = smoothStep(holdingFrom(hand), holdingFrom(hand) + 0.35f, tau)
                * (1.0f - smoothStep(holdingUntil(hand) - 0.4f, holdingUntil(hand) - 0.06f, tau));
            if (holding <= 0.0f)
                return handAt(hand, tau);
            return FloatPoint::between(handAt(hand, tau), balancing, holding);
        }

        // A ball `flown` seconds into a flight of `flightSeconds` from `from` to `to`.
        [[nodiscard]] FloatPoint flightAt(const FloatPoint from, const FloatPoint to,
            const float flightSeconds, const float flown, const float gravity)
        {
            FloatPoint at = FloatPoint::between(from, to, flown / flightSeconds);
            at.y -= 0.5f * gravity * flown * (flightSeconds - flown);
            return at;
        }

        // The android at `tau`: once both hands hold a ball it lunges right, and it rises from the
        // lunge as it kicks the wild ball back up.
        [[nodiscard]] AndroidPose jugglerAt(const ActStage& stage, const float tau,
            const float cycleTime)
        {
            const SceneFrame& frame = stage.frame;
            const float unit = frame.unit;
            const float pi = std::numbers::pi_v<float>;
            const float lunge = smoothStep(holdingFrom(0), holdingFrom(0) + 0.46f, tau)
                * (1.0f - smoothStep(k_kickedAt, k_kickedAt + 0.4f, tau));
            const float bounce = 3.0f * std::pow(std::sin(pi * tau / k_throwSeconds), 2.0f)
                * (1.0f - lunge);
            const float x = stage.centerX + 30.0f * unit * stage.closeness;
            AndroidPose android = standingAndroid(frame, { x, stage.floorY }, cycleTime, bounce,
                22.0f);
            android.left.hand = jugglingHandAt(0, tau);
            android.right.hand = jugglingHandAt(1, tau);
            if (lunge <= 0.0f)
                return android;
            const FloatPoint leftAnkle = androidBodyToFrame(frame, android, android.left.foot);
            const FloatPoint standingRightAnkle = androidBodyToFrame(frame, android,
                android.right.foot);
            android.pelvis = android.pelvis + k_lungeShift * (lunge * unit);
            android.bend = k_lungeBend * lunge;
            const FloatPoint standingRightFoot = androidFrameToBody(frame, android,
                standingRightAnkle);
            const float kick = k_kickLift * smoothStep(k_kickedAt - 0.16f, k_kickedAt, tau);

            // while the ball rests on the leg the android keeps it there: the body teeters,
            // the arms answer it and the leg corrects under the ball
            const float sinceRest = tau - k_landedAt - k_rollSeconds;
            const float balancing = smoothStep(0.0f, 0.2f, sinceRest)
                * (1.0f - smoothStep(k_kickedAt - 0.3f, k_kickedAt - 0.16f, tau));
            const float teeter = balancing * (0.65f * std::sin(k_2Pi * 1.1f * sinceRest)
                + 0.35f * std::sin(k_2Pi * 1.9f * sinceRest + 0.7f));
            const float ankleLift = balancing
                * (0.5f + 0.5f * std::sin(k_2Pi * 1.4f * sinceRest - 1.6f));
            const FloatPoint lungingRightFoot = k_lungeAnkle
                - FloatPoint{ 0.0f, kick + 6.0f * ankleLift };
            const FloatPoint rightFoot = FloatPoint::between(standingRightFoot, lungingRightFoot, lunge)
                - FloatPoint{ 0.0f, 12.0f * std::sin(pi * lunge) };
            // the feet stay where they stand as the body teeters over them
            const FloatPoint rightAnkle = androidBodyToFrame(frame, android, rightFoot);
            android.pelvis.x += 4.0f * teeter * unit;
            android.bend += 0.09f * teeter;
            android.headTurn = 0.05f * teeter;
            android.left.hand.y -= 18.0f * teeter;
            android.right.hand.y += 18.0f * teeter;
            android.left.foot = androidFrameToBody(frame, android, leftAnkle);
            android.right.foot = androidFrameToBody(frame, android, rightAnkle);
            android.right.footTurn = k_lungeFootTurn * lunge;
            return android;
        }

        // A point about the right ankle, placed in the frame.
        [[nodiscard]] FloatPoint byRightAnkle(const SceneFrame& frame, const AndroidPose& android,
            const FloatPoint offset)
        {
            return androidBodyToFrame(frame, android, android.right.foot + offset);
        }

        // The wild ball from its throw until it is back in the right hand, in the frame.
        [[nodiscard]] FloatPoint wildBallAt(const ActStage& stage, const AndroidPose& android,
            const float tau, const float cycleTime)
        {
            const SceneFrame& frame = stage.frame;
            const float gravity = k_gravity * frame.unit;
            const FloatPoint overHand = { 0.0f, -k_ballOverHand };
            if (tau < k_landedAt)
            {
                const AndroidPose thrower = jugglerAt(stage, k_wildThrownAt, cycleTime);
                const AndroidPose catcher = jugglerAt(stage, k_landedAt, cycleTime);
                return flightAt(androidUpperToFrame(frame, thrower, thrower.left.hand + overHand),
                    byRightAnkle(frame, catcher, k_ballLanding), k_wildFlightSeconds,
                    tau - k_wildThrownAt, gravity);
            }
            if (tau < k_kickedAt)
            {
                const float rolled = smoothStep(k_landedAt, k_landedAt + k_rollSeconds, tau);
                return byRightAnkle(frame, android,
                    FloatPoint::between(k_ballLanding, k_ballOnShin, rolled));
            }
            const AndroidPose kicker = jugglerAt(stage, k_kickedAt, cycleTime);
            const AndroidPose catcher = jugglerAt(stage, k_backInHandAt, cycleTime);
            return flightAt(byRightAnkle(frame, kicker, k_ballOnShin),
                androidUpperToFrame(frame, catcher, catcher.right.hand + overHand),
                k_flightSeconds, tau - k_kickedAt, gravity);
        }
    }

    // Throw n goes from hand n mod 2 and carries ball n mod 3, so each ball changes hands every
    // throw it makes.
    void JugglingAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float tau = actTime + 6.0f * k_throwSeconds;
        m_android = jugglerAt(stage, tau, time.cycleTime);
        const float teeteringHeadTurn = m_android.headTurn;

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
            FloatPoint at = FloatPoint::between(from, to, share);
            at.y -= 4.0f * k_throwHeight * share * (1.0f - share);
            m_balls[static_cast<std::size_t>(ball)] = at;
        }
        for (const int hand : { 0, 1 })
        {
            if (tau < holdingFrom(hand) || tau >= holdingUntil(hand))
                continue;
            const FloatPoint holder = hand == 0 ? m_android.left.hand : m_android.right.hand;
            m_balls[static_cast<std::size_t>(withheldThrow(hand) % 3)] = holder + overHand;
        }
        if (tau >= k_wildThrownAt && tau < k_backInHandAt)
        {
            m_balls[k_wildBall] = androidFrameToUpper(frame, m_android,
                wildBallAt(stage, m_android, tau, time.cycleTime));
        }

        const FloatPoint highest = *std::ranges::min_element(m_balls, {}, &FloatPoint::y);
        m_android.look = std::clamp(highest.x / 9.0f, -6.0f, 6.0f) - 2.0f;
        m_android.headTurn = std::clamp(highest.x / 900.0f, -0.06f, 0.06f);
        // the eyes follow the wild ball down to the leg and stay on it until the kick
        const float watching = smoothStep(k_wildThrownAt + 0.45f, k_wildThrownAt + 0.65f, tau)
            * (1.0f - smoothStep(k_kickedAt + 0.15f, k_kickedAt + 0.45f, tau));
        if (watching > 0.0f)
        {
            const FloatPoint wild = m_balls[k_wildBall];
            m_android.look = std::lerp(m_android.look, std::clamp(wild.x / 9.0f, -6.0f, 6.0f),
                watching);
            m_android.headTurn = std::lerp(m_android.headTurn, 0.1f, watching)
                + teeteringHeadTurn;
        }
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
