module ClaFi.PathArt.CurtainCallAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BouquetSprite;
import ClaFi.PathArt.GatorSprite;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.RabbitSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.TopHatSprite;
import ClaFi.PathArt.WolfSprite;
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
        constexpr Color k_bouquetShadow{ 0x59120807 };

        // A bouquet thrown from the audience: it lands in front of the cast and lies there.
        struct Throw
        {
            float at; // seconds into the act it leaves the thrower's hand
            float landX; // where it lands across the stage, in units from the middle
            float landY; // how far in front of the floor line it lands, in units
            float fromX; // where it is thrown from, in units across from where it lands
            float peak; // the top of its arc, in units above the floor line
            BouquetRoses roses;
            float turns; // how many times it turns over in the air
            float rest; // its turn as it lies on the boards
        };
        constexpr std::array<Throw, 9> k_throws{
            Throw{ 1.5f, -230.0f, 14.0f, -78.0f, 297.0f, BouquetRoses::Red, 1.3f, 1.45f },
            Throw{ 2.4f, 80.0f, 24.0f, -3.0f, 294.0f, BouquetRoses::Pink, -1.1f, -1.5f },
            Throw{ 3.3f, -95.0f, 32.0f, 96.0f, 244.0f, BouquetRoses::Yellow, 1.6f, 1.4f },
            Throw{ 4.1f, 200.0f, 16.0f, -16.0f, 262.0f, BouquetRoses::White, -1.4f, -1.45f },
            Throw{ 4.9f, -165.0f, 28.0f, 97.0f, 284.0f, BouquetRoses::Pink, 1.2f, -1.5f },
            Throw{ 5.6f, 130.0f, 36.0f, 33.0f, 279.0f, BouquetRoses::Red, -1.0f, 1.5f },
            Throw{ 7.0f, -20.0f, 20.0f, -105.0f, 268.0f, BouquetRoses::Yellow, 1.4f, -1.4f },
            Throw{ 7.8f, 265.0f, 30.0f, -86.0f, 273.0f, BouquetRoses::White, -1.2f, 1.45f },
            Throw{ 8.4f, -275.0f, 34.0f, -57.0f, 260.0f, BouquetRoses::Red, 1.1f, -1.5f }
        };
        constexpr float k_flightSeconds{ 0.85f };
        // The bouquet the android catches leaves the thrower's hand then.
        constexpr float k_catchThrownAt{ 6.3f };
        // A bouquet held or caught sits this far above the hand, flowers up.
        constexpr float k_bouquetOverHand{ 22.0f };
        // The hat comes off at this size, as worn, and grows to its own as it is lifted.
        constexpr float k_wornHatSize{ 0.92f };
        // The hand holds the hat by its brim, this far from the brim's middle in the hat's units.
        constexpr float k_hatGrip{ 36.0f };

        // A nod or a bow: down from `from`, held, and back up by `to`.
        [[nodiscard]] float nodAt(const float actTime, const float from, const float to)
        {
            const float span = to - from;
            return smoothStep(from, from + 0.35f * span, actTime)
                * (1.0f - smoothStep(to - 0.4f * span, to, actTime));
        }

        // Overshoots its end a little and settles back, as the rabbit pops up.
        [[nodiscard]] float popUp(const float share)
        {
            if (share <= 0.0f)
                return 0.0f;
            constexpr float k_overshoot{ 1.70158f };
            const float rest = std::min(share, 1.0f) - 1.0f;
            return 1.0f + (k_overshoot + 1.0f) * rest * rest * rest + k_overshoot * rest * rest;
        }

        // A point along an arc from `from` over `over` to `to`, `share` of the way.
        [[nodiscard]] FloatPoint alongArc(const FloatPoint from, const FloatPoint over,
            const FloatPoint to, const float share)
        {
            const float rest = 1.0f - share;
            return from * (rest * rest) + over * (2.0f * share * rest) + to * (share * share);
        }
    }

    // Seconds: the android lifts its top hat off from 1.15 and sweeps it down with a bow to 2.3,
    // the rabbit pops out of it at 2.45; it waves from 3.2, catches a bouquet at 7.15 and holds it
    // up from 7.75; the whole cast bows together from 8.3. Bouquets land throughout.
    void CurtainCallAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float floorY = stage.floorY;
        const auto between = [&](float from, float to) {
            return smoothStep(from, to, actTime);
        };
        m_actTime = actTime;
        m_middleX = stage.centerX;
        m_floorY = floorY;
        m_bottomY = frame.height;
        m_spread = std::min(1.0f, frame.width / unit / 900.0f);
        const float spread = m_spread;
        const float together = nodAt(actTime, 8.3f, 8.95f);

        // the android doffs its hat with a bow, waves, catches a bouquet and bows again with it
        const float bow = std::max(nodAt(actTime, 1.9f, 3.0f), together);
        const float androidX = stage.centerX + 20.0f * unit * spread;
        m_android = standingAndroid(frame, { androidX, floorY }, time.cycleTime, 6.0f * bow);
        m_android.bow = bow;
        const float sweep = between(1.8f, 2.3f);
        FloatPoint rightHand = pointBetween(ActHands::RightOnHip, { 40.0f, -176.0f },
            between(1.15f, 1.45f));
        rightHand = pointBetween(rightHand, { 56.0f, -206.0f }, between(1.5f, 1.8f));
        if (actTime >= 1.8f)
        {
            const float swing = std::lerp(-1.45f, 0.85f, sweep);
            rightHand = AndroidBody::RightShoulder
                + FloatPoint{ std::cos(swing), std::sin(swing) } * 102.0f;
        }
        rightHand = rightHand + FloatPoint{ 10.0f, 6.0f } * together;
        m_holdingHat = actTime >= 1.5f;
        m_android.wearsHat = !m_holdingHat;

        const float waving = between(3.2f, 3.6f) * (1.0f - between(5.4f, 5.8f));
        const float reachingUp = between(6.55f, 7.1f);
        const float toChest = between(7.15f, 7.55f);
        const float holdingUp = between(7.75f, 8.15f);
        FloatPoint leftHand = pointBetween(ActHands::LeftOnHip,
            { -112.0f + 14.0f * std::sin(k_2Pi * actTime * 1.6f), -178.0f }, waving);
        leftHand = pointBetween(leftHand, { -96.0f, -214.0f }, reachingUp);
        leftHand = pointBetween(leftHand, { -34.0f, -78.0f }, toChest);
        leftHand = pointBetween(leftHand, { -104.0f, -196.0f }, holdingUp);
        const bool caught = actTime > 7.1f;
        m_android.left.hand = leftHand;
        m_android.left.handShape = caught ? HandShape::Fist : HandShape::Open;
        m_android.left.forearmOnTop = caught;
        m_android.right.hand = rightHand;
        m_android.right.handShape = m_holdingHat ? HandShape::Fist : HandShape::Open;
        m_android.right.forearmOnTop = m_holdingHat;
        if (actTime < 1.2f)
            m_android.look = 3.0f * std::sin(k_2Pi * actTime / 1.2f);
        else
            m_android.look = waving > 0.5f ? -4.0f : 2.0f * std::sin(k_2Pi * actTime / 3.0f);
        if (actTime > 8.6f)
            m_android.lights = flashingLights(actTime);

        // The hat turns over as the arm sweeps it down; past level it shows its inside, and it
        // ends upside down at the android's side for the rabbit to come out of.
        const float hatSize = std::lerp(k_wornHatSize, 1.0f, between(1.5f, 1.8f)) * unit;
        const float hatTurn = 0.14f + (k_pi - 0.14f) * sweep;
        const bool hatOver = hatTurn > 0.5f * k_pi;
        const FloatPoint hand = androidUpperToFrame(frame, m_android, rightHand);
        m_hat = {
            .brim = hand + rotated({ -k_hatGrip * hatSize, 0.0f }, hatTurn),
            .turn = hatOver ? hatTurn - k_pi : hatTurn,
            .size = hatSize,
            .upsideDown = hatOver
        };
        const float rabbitNod = std::max(nodAt(actTime, 3.4f, 4.0f), together);
        m_rabbit = {
            .rim = m_hat.brim,
            .turn = m_hat.turn,
            .size = hatSize,
            .rise = popUp((actTime - 2.45f) / 0.55f) * (1.0f - 0.14f * rabbitNod),
            .look = 3.0f * std::sin(k_2Pi * (actTime - 3.0f) / 2.4f) * between(3.0f, 3.3f),
            .time = actTime
        };

        // the bouquet it catches, thrown to where its left hand will be
        m_catchThrown = actTime >= k_catchThrownAt;
        const FloatPoint leftHandAt = androidUpperToFrame(frame, m_android, leftHand);
        const FloatPoint held = leftHandAt - FloatPoint{ 0.0f, k_bouquetOverHand * unit };
        const float flown = (actTime - k_catchThrownAt) / k_flightSeconds;
        m_caughtBouquet = {
            .middle = held,
            .turn = -0.35f + 0.2f * toChest,
            .size = unit,
            .roses = BouquetRoses::Red
        };
        if (flown < 1.0f)
        {
            const FloatPoint from = { leftHandAt.x - 120.0f * unit, frame.height + 40.0f * unit };
            const FloatPoint over = { (from.x + leftHandAt.x) * 0.5f, floorY - 380.0f * unit };
            m_caughtBouquet.middle = alongArc(from, over, held, flown);
            m_caughtBouquet.turn = -0.35f - k_2Pi * 1.2f * (1.0f - flown);
        }

        // the gator in from the wings, grinning and nodding
        const float gatorNod = std::max({ nodAt(actTime, 2.3f, 3.1f), nodAt(actTime, 5.0f, 5.8f),
            together });
        m_gator = {
            .neck = { stage.centerX - 285.0f * unit * spread, floorY },
            .size = 0.72f * unit,
            .headLift = 0.2f - 0.2f * gatorNod,
            .gape = 0.24f + 0.06f * std::sin(k_2Pi * actTime / 2.6f),
            .tailDrop = 6.0f * std::sin(k_2Pi * actTime / 3.1f),
            .glance = 1.0f,
            .lid = fractionalPart(actTime / 3.3f) > 0.96f ? 1.0f : 0.0f
        };

        // the marionette on its own feet: a curtsy with the bow, a wave, two more curtsies
        const float curtsy = std::max({ nodAt(actTime, 2.0f, 2.9f), nodAt(actTime, 6.0f, 6.8f),
            together });
        const float wave = between(3.8f, 4.1f) * (1.0f - between(5.3f, 5.6f));
        m_marionette = standingMarionette(unit, { stage.centerX - 125.0f * unit * spread, floorY },
            curtsy, wave, actTime);

        // the wolf, facing the rest: wagging, panting, nodding, an eye on the rabbit
        const float wolfUnit = 0.95f * unit;
        const float wolfNod = std::max({ nodAt(actTime, 2.2f, 2.9f), nodAt(actTime, 4.5f, 5.2f),
            together });
        m_wolf = {
            .center = {
                stage.centerX + 290.0f * unit * spread,
                floorY + 8.0f * unit - 54.0f * wolfUnit,
            },
            .size = wolfUnit,
            .turn = 0.06f * wolfNod,
            .stride = 0.15f * std::sin(k_2Pi * actTime * 2.0f),
            .leap = 0.0f,
            .sprawl = 0.0f,
            .onBack = 0.0f,
            .dangle = 0.0f,
            .dazed = 0.0f,
            .headTurn = 0.42f * wolfNod,
            .tongueTurn = 0.2f * std::sin(k_2Pi * actTime * 3.0f),
            .jawOpen = 0.18f * (1.0f - 0.6f * wolfNod),
            .glance = m_rabbit.rise > 0.5f ? 2.5f : 0.0f,
            .cycleTime = time.cycleTime,
            .wag = 1.0f
        };
    }

    // In a row from the left: the gator, the marionette, the android with its hat and the
    // rabbit, the wolf; the bouquets land in front of them all.
    void CurtainCallAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        drawGator(frame, m_gator);
        drawGatorUpperJaw(frame, m_gator);
        drawMarionetteFigure(frame, m_marionette);
        drawAndroid(frame, m_android);
        if (m_holdingHat)
        {
            drawTopHat(frame, m_hat);
            if (m_hat.upsideDown)
                drawRabbit(frame, m_rabbit);
            drawTopHatBrimLip(frame, m_hat);
            drawAndroidForearm(frame, m_android, AudienceSide::Right);
        }
        if (m_catchThrown)
            drawBouquet(frame, m_caughtBouquet);
        if (m_android.left.forearmOnTop)
            drawAndroidForearm(frame, m_android, AudienceSide::Left);
        const SceneFrame wolfFrame = frame.within(
            Matrix3x2::Translation(2.0f * m_wolf.center.x, 0.0f) * Matrix3x2::Scale(-1.0f, 1.0f));
        drawWolf(wolfFrame, m_wolf, WolfCoats::Grey);

        // Each thrown bouquet flies over an arc from below the front of the stage, lands, bounces
        // once and lies where it fell.
        for (const Throw& toss : k_throws)
        {
            const float flown = (m_actTime - toss.at) / k_flightSeconds;
            if (flown < 0.0f)
                continue;
            const FloatPoint land = {
                m_middleX + toss.landX * unit * m_spread,
                m_floorY + toss.landY * unit,
            };
            BouquetPose bouquet = {
                .middle = land,
                .turn = toss.rest,
                .size = unit,
                .roses = toss.roses
            };
            if (flown < 1.0f)
            {
                const FloatPoint from = { land.x + toss.fromX * unit, m_bottomY + 40.0f * unit };
                const FloatPoint over = { (from.x + land.x) * 0.5f, m_floorY - toss.peak * unit };
                bouquet.middle = alongArc(from, over, land, flown);
                bouquet.turn = toss.rest - k_2Pi * toss.turns * (1.0f - flown);
            }
            else
            {
                const float lying = m_actTime - toss.at - k_flightSeconds;
                if (lying < 0.4f)
                {
                    bouquet.middle.y -= 9.0f * unit * std::abs(std::sin(k_pi * lying / 0.2f))
                        * std::exp(-lying / 0.12f);
                }
                fillOval(frame, land + FloatPoint{ 0.0f, 7.0f * unit }, 26.0f * unit, 4.0f * unit,
                    SolidColor{ k_bouquetShadow });
            }
            drawBouquet(frame, bouquet);
        }
    }
}
