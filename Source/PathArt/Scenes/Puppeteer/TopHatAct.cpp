module ClaFi.PathArt.TopHatAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.RabbitSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SparkleBurstEffect;
import ClaFi.PathArt.TopHatSprite;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace TopHatActColors
    {
        constexpr Color Puff{ 0xFFF6EFE2 };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // The hat in the act is larger than the android's head, by its own units.
        constexpr float k_hatSize{ 1.2f };
        // The hand holds the crown's end, this far below the brim in the hat's units.
        constexpr float k_crownGrip{ 60.0f };
        constexpr float k_wandLength{ 66.0f };
        // The wand comes down onto the brim at this angle for the taps.
        constexpr float k_tapAngle{ k_pi * 0.84f };
        constexpr std::array<float, 3> k_taps{ 2.0f, 2.5f, 3.0f };
        constexpr float k_poof{ 3.4f };

        // Overshoots its end a little and settles back, as the rabbit pops up.
        [[nodiscard]] float popUp(const float share)
        {
            if (share <= 0.0f)
                return 0.0f;
            constexpr float k_overshoot{ 1.70158f };
            const float rest = std::min(share, 1.0f) - 1.0f;
            return 1.0f + (k_overshoot + 1.0f) * rest * rest * rest + k_overshoot * rest * rest;
        }
    }

    // Seconds: the hat shown empty to 1.7, three taps at 2, 2.5 and 3, the poof at 3.4, the rabbit
    // up by 4.2, the hat raised at 4.4, the wand circling, the ta-da from 7.6.
    void TopHatAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float x = stage.centerX + 60.0f * unit * stage.closeness;
        const float bob = 2.0f * std::pow(std::sin(k_pi * actTime / 1.25f), 2.0f);
        m_android = standingAndroid(frame, { x, stage.floorY }, time.cycleTime, bob, 20.0f);

        // the hat in the left hand: tipped to show it empty, held still for the taps, raised
        const float raise = smoothStep(4.4f, 5.0f, actTime);
        const FloatPoint leftHand = {
            std::lerp(-96.0f, -116.0f, raise),
            std::lerp(-30.0f, -98.0f, raise)
        };
        const float shown = actTime < 1.7f
            ? std::pow(std::sin(k_pi * actTime / 0.85f), 2.0f)
            : 0.0f;
        m_hatTurn = -0.8f * shown + 0.08f * raise * std::sin(k_2Pi * actTime * 0.6f);
        m_brim = leftHand + rotated({ 0.0f, -k_crownGrip * k_hatSize }, m_hatTurn);
        m_brimEdge = m_brim + rotated({ 26.0f * k_hatSize, -k_hatSize }, m_hatTurn);

        // the wand in the right hand: down by the hip, over the hat for the taps, circling, up
        const FloatPoint tapDirection = { std::cos(k_tapAngle), std::sin(k_tapAngle) };
        const FloatPoint overHat = m_brimEdge - tapDirection * k_wandLength
            - FloatPoint{ 0.0f, 6.0f };
        FloatPoint rightHand = ActHands::RightOnHip;
        float wandAngle = k_pi * 0.6f;
        if (actTime >= 1.5f && actTime < k_poof)
        {
            const float into = smoothStep(1.5f, 1.9f, actTime);
            rightHand = pointBetween(ActHands::RightOnHip, overHat, into);
            float lifted = 1.0f;
            for (const float tap : k_taps)
                lifted = std::min(lifted,
                    1.0f - std::exp(-std::pow((actTime - tap) / 0.07f, 2.0f)));
            wandAngle = std::lerp(k_pi * 0.6f, k_tapAngle - 0.45f * lifted, into);
        }
        else if (actTime >= k_poof && actTime < 7.6f)
        {
            const float out = smoothStep(k_poof, 3.9f, actTime);
            const float circling = k_2Pi * (actTime - 3.9f) / 1.2f;
            const FloatPoint circle = { std::cos(circling) * 10.0f, std::sin(circling) * 8.0f };
            rightHand = pointBetween(overHat, FloatPoint{ 66.0f, -118.0f } + circle, out);
            wandAngle = std::lerp(k_tapAngle - 0.45f,
                -k_pi * 0.35f + 0.25f * std::sin(k_2Pi * actTime / 1.2f), out);
        }
        else if (actTime >= 7.6f)
        {
            const float out = smoothStep(7.6f, 8.1f, actTime);
            rightHand = pointBetween({ 66.0f, -118.0f }, ActHands::RightRaised, out);
            wandAngle = std::lerp(-k_pi * 0.35f, -k_pi * 0.42f, out);
            if (actTime > 8.1f)
                m_android.lights = flashingLights(actTime);
        }
        for (AndroidSide* side : { &m_android.left, &m_android.right })
        {
            side->handShape = HandShape::Fist;
            side->forearmOnTop = true;
        }
        m_android.left.hand = leftHand;
        m_android.right.hand = rightHand;
        m_wandTip = rightHand
            + FloatPoint{ std::cos(wandAngle), std::sin(wandAngle) } * k_wandLength;

        m_rabbitRise = popUp((actTime - 3.55f) / 0.6f);
        m_rabbitLook = 3.0f * std::sin(k_2Pi * (actTime - 4.2f) / 2.4f)
            * smoothStep(4.2f, 4.6f, actTime);
        m_android.look = actTime < 1.7f
            ? -6.0f
            : (actTime < 4.2f ? -3.0f : 2.0f * std::sin(k_2Pi * actTime / 3.0f));
        m_android.headTurn = actTime < k_poof ? -0.08f : 0.04f * std::sin(k_2Pi * actTime / 3.0f);
        m_spotlightX = x - 20.0f * unit;
        m_actTime = actTime;
    }

    // The rabbit stands between the hat and its brim's near lip; the hand holding the hat and
    // the one holding the wand go over them.
    void TopHatAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        const float actTime = m_actTime;
        drawAndroid(frame, m_android);
        const FloatPoint brim = androidUpperToFrame(frame, m_android, m_brim);
        const TopHatPose hat{
            .brim = brim,
            .turn = m_hatTurn,
            .size = k_hatSize * unit,
            .upsideDown = true
        };
        drawTopHat(frame, hat);
        drawRabbit(frame, RabbitPose{
            .rim = brim,
            .turn = m_hatTurn,
            .size = k_hatSize * unit,
            .rise = m_rabbitRise,
            .look = m_rabbitLook,
            .time = actTime
        });
        drawTopHatBrimLip(frame, hat);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
        const FloatPoint tip = androidUpperToFrame(frame, m_android, m_wandTip);
        drawWand(frame, androidUpperToFrame(frame, m_android, m_android.right.hand), tip);
        drawAndroidForearm(frame, m_android, AudienceSide::Right);

        // the taps, the poof out of the hat, and the trail off the wand's tip
        const FloatPoint brimEdge = androidUpperToFrame(frame, m_android, m_brimEdge);
        for (std::size_t i = 0; i != k_taps.size(); ++i)
            drawSparkleBurst(frame, SparkleBurst{
                .center = brimEdge,
                .age = (actTime - k_taps[i]) / 0.45f,
                .count = 6,
                .seed = 20.0f + static_cast<float>(i),
                .size = 0.6f,
                .colors = { MagicColors::Gold, MagicColors::White, MagicColors::Gold }
            });
        const float puff = (actTime - k_poof) / 0.9f;
        if (puff > 0.0f && puff < 1.0f)
        {
            for (int i = 0; i != 8; ++i)
            {
                const float angle = k_pi + k_pi * (static_cast<float>(i) + 0.5f) / 8.0f;
                const float reach = (12.0f + 34.0f * std::sqrt(puff)) * unit;
                const FloatPoint at = brim + FloatPoint{
                    std::cos(angle) * reach * 1.3f,
                    std::sin(angle) * reach - 10.0f * unit * puff
                };
                fillDisc(frame, at, (9.0f + 8.0f * puff) * unit,
                    SolidColor{ TopHatActColors::Puff.withOpacity(0.55f * (1.0f - puff)) });
            }
        }
        drawSparkleBurst(frame, SparkleBurst{
            .center = brim - FloatPoint{ 0.0f, 6.0f * unit },
            .age = (actTime - k_poof) / 0.8f,
            .count = 10,
            .seed = 31.0f,
            .colors = { MagicColors::Gold, MagicColors::Cyan, MagicColors::White }
        });
        if (actTime > 3.9f)
        {
            for (int i = 0; i != 5; ++i)
            {
                const float life = fractionalPart(actTime * 1.6f + static_cast<float>(i) / 5.0f);
                const FloatPoint at = tip + FloatPoint{
                    (pseudoRandom(i, 41.0f) - 0.5f) * 16.0f * unit,
                    26.0f * unit * life
                };
                const Color color = i % 2 == 1 ? MagicColors::Gold : MagicColors::White;
                drawSparkle(frame, at, 4.5f * unit * (1.0f - life), faded(color, 1.0f - life));
            }
        }
    }
}
