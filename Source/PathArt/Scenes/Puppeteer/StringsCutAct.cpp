module ClaFi.PathArt.StringsCutAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SparkleBurstEffect;
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
        constexpr float k_wandLength{ 66.0f };
        // The wand from the hip up over the head, then once and a half round a small circle.
        constexpr FloatPoint k_wandDown{ 46.0f, -24.0f };
        constexpr FloatPoint k_wandUp{ 70.0f, -176.0f };
        constexpr float k_downAngle{ k_pi * 0.42f };
        constexpr float k_upAngle{ -k_pi * 0.62f };

        // The right hand and the wand's angle through the raise and the wave, at a time.
        struct WandHold
        {
            FloatPoint hand;
            float angle;
        };

        [[nodiscard]] WandHold wavingAt(const float actTime)
        {
            const float raise = smoothStep(1.2f, 1.6f, actTime);
            const float circling = k_2Pi * 1.5f * smoothStep(1.6f, 2.2f, actTime);
            WandHold hold = {
                pointBetween(k_wandDown, k_wandUp, raise),
                std::lerp(k_downAngle, k_upAngle, raise),
            };
            if (actTime >= 1.6f && actTime < 2.4f)
            {
                hold.hand = hold.hand
                    + FloatPoint{ 10.0f * std::sin(circling), -8.0f * (1.0f - std::cos(circling)) };
                hold.angle += 0.5f * std::sin(circling);
            }
            return hold;
        }

        [[nodiscard]] FloatPoint tipOf(const WandHold& hold)
        {
            return hold.hand
                + FloatPoint{ std::cos(hold.angle), std::sin(hold.angle) } * k_wandLength;
        }
    }

    // Seconds: the wand raised from 1.2 and waved round from 1.6; sparkles fly to the control at
    // 2.15 and run down the strings from 2.4; the strings fade by 3 and the control goes up in
    // sparkles; the wand beats time while the marionette dances, rises and twirls; the ta-da
    // from 8.3.
    void StringsCutAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float sway = danceSway(time);
        m_marionetteX = marionetteX(stage, sway);
        const float lift = 36.0f
            * std::pow(std::sin(k_pi * std::clamp((actTime - 5.6f) / 1.5f, 0.0f, 1.0f)), 2.0f);
        m_marionette = danceMarionette(unit, { m_marionetteX, stage.floorY - lift * unit }, sway);
        m_twirl = std::cos(k_2Pi * smoothStep(0.0f, 1.0f, (actTime - 5.85f) / 1.0f));

        const float bob = -2.5f * std::abs(std::sin(2.0f * sway));
        m_android = standingAndroid(frame, { puppeteerX(stage), stage.floorY }, time.cycleTime,
            bob);
        m_android.wearsHat = true;
        m_android.headTurn = -0.1f + 0.06f * std::sin(sway);
        m_android.look = -5.0f + 2.0f * std::sin(sway);

        // the wand: waved, then beating time for the dance, then up for the ta-da
        WandHold wand = wavingAt(actTime);
        const float conducting = smoothStep(3.0f, 3.4f, actTime)
            * (1.0f - smoothStep(8.3f, 8.7f, actTime));
        const float beat = (actTime - 3.0f) / 0.625f;
        const FloatPoint beatingTime = {
            92.0f + 8.0f * std::sin(k_pi * beat),
            -122.0f + 16.0f * std::abs(std::cos(k_pi * beat)),
        };
        wand.hand = pointBetween(wand.hand, beatingTime, conducting);
        wand.angle = std::lerp(wand.angle, -k_pi * 0.32f + 0.35f * std::sin(k_pi * beat),
            conducting);
        const float tada = smoothStep(8.3f, 8.8f, actTime);
        wand.hand = pointBetween(wand.hand, ActHands::RightRaised, tada);
        wand.angle = std::lerp(wand.angle, -k_pi * 0.4f, tada);
        m_wandTip = tipOf(wand);

        // the left hand works the control, then lets go and opens toward the marionette
        const float freed = smoothStep(2.7f, 3.2f, actTime);
        const FloatPoint grip = androidFrameToUpper(frame, m_android,
            m_marionette.control.center - FloatPoint{ 0.0f, 9.0f * unit });
        FloatPoint leftHand = pointBetween(grip,
            { -108.0f, -64.0f + 10.0f * std::sin(k_2Pi * actTime / 1.25f) }, freed);
        leftHand = pointBetween(leftHand, { -150.0f, -118.0f }, tada);
        AndroidSide& left = m_android.left;
        left.hand = leftHand;
        left.upperArm = std::lerp(87.0f, 57.0f, freed);
        left.forearm = std::lerp(87.0f, 51.0f, freed);
        left.elbow = freed < 0.5f ? ElbowBend::Down : ElbowBend::Out;
        left.handShape = freed < 0.5f ? HandShape::Fist : HandShape::Open;
        left.forearmOnTop = freed < 0.5f;
        m_android.right.hand = wand.hand;
        m_android.right.handShape = HandShape::Fist;
        m_android.right.forearmOnTop = true;
        if (actTime > 8.8f)
            m_android.lights = flashingLights(actTime);
        m_actTime = actTime;
    }

    // The marionette dances under a glow once its strings have gone; the hand on the control
    // and the one on the wand go over what they hold.
    void StringsCutAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        const float actTime = m_actTime;
        const MarionetteJoints& puppet = m_marionette.joints;
        const float stringsLeft = 1.0f - smoothStep(2.45f, 3.0f, actTime);
        const float controlLeft = 1.0f - smoothStep(2.7f, 3.2f, actTime);
        const float magic = smoothStep(2.6f, 3.3f, actTime);

        drawAndroid(frame, m_android);
        if (magic > 0.0f)
            drawGlow(frame, puppet.pelvis - FloatPoint{ 0.0f, 18.0f * unit }, 82.0f * unit,
                MagicColors::Cyan.withOpacity(
                    0.24f * magic * (0.8f + 0.2f * std::sin(k_2Pi * actTime * 1.5f))));
        drawMarionetteStrings(frame, m_marionette, stringsLeft);
        drawMarionetteControl(frame, m_marionette.control, controlLeft);
        const float twirlFacing = (m_twirl < 0.0f ? -1.0f : 1.0f)
            * std::max(0.04f, std::abs(m_twirl));
        const SceneFrame twirled = frame.within(Matrix3x2::Translation(m_marionetteX, 0.0f)
            * Matrix3x2::Scale(twirlFacing, 1.0f) * Matrix3x2::Translation(-m_marionetteX, 0.0f));
        drawMarionetteFigure(twirled, puppet);
        if (m_android.left.forearmOnTop)
            drawAndroidForearm(frame, m_android, AudienceSide::Left);
        const FloatPoint tip = androidUpperToFrame(frame, m_android, m_wandTip);
        drawWand(frame, androidUpperToFrame(frame, m_android, m_android.right.hand), tip);
        drawAndroidForearm(frame, m_android, AudienceSide::Right);

        // the trail behind the circling wand
        if (actTime > 1.5f && actTime < 2.5f)
        {
            for (int i = 0; i != 7; ++i)
            {
                const float behind = static_cast<float>(i) * 0.045f;
                const FloatPoint at = androidUpperToFrame(frame, m_android,
                    tipOf(wavingAt(actTime - behind)));
                const Color color = i % 2 == 1 ? MagicColors::Cyan : MagicColors::Gold;
                drawSparkle(frame, at, (6.0f - static_cast<float>(i) * 0.7f) * unit,
                    faded(color, 1.0f - static_cast<float>(i) / 8.0f));
            }
        }
        // sparkles flying across to the control, then glinting down every string
        const MarionetteControl& control = m_marionette.control;
        const float flight = (actTime - 2.15f) / 0.3f;
        if (flight > 0.0f && flight < 1.0f)
        {
            for (int i = 0; i != 5; ++i)
            {
                const float share = std::clamp(flight - static_cast<float>(i) * 0.08f, 0.0f, 1.0f);
                const FloatPoint at = pointBetween(tip, control.center, share)
                    - FloatPoint{ 0.0f, 40.0f * unit * std::sin(k_pi * share) };
                const Color color = i % 2 == 1 ? MagicColors::Gold : MagicColors::White;
                drawSparkle(frame, at, 6.0f * unit, faded(color, 1.0f - share * 0.3f));
            }
        }
        const float glint = (actTime - 2.4f) / 0.45f;
        if (glint > 0.0f && glint < 1.0f)
        {
            const std::array<std::array<FloatPoint, 2>, 5> strings{ {
                { control.center, puppet.head - FloatPoint{ 0.0f, 15.0f * unit } },
                { control.endLeft, puppet.armLeft.end },
                { control.endRight, puppet.armRight.end },
                { control.kneeEndLeft, puppet.legLeft.joint },
                { control.kneeEndRight, puppet.legRight.joint }
            } };
            for (const std::array<FloatPoint, 2>& string : strings)
                drawSparkle(frame, pointBetween(string[0], string[1], glint), 6.0f * unit,
                    faded(MagicColors::White, 1.0f - 0.4f * glint));
        }
        drawSparkleBurst(frame, SparkleBurst{
            .center = control.center,
            .age = (actTime - 2.7f) / 0.7f,
            .count = 12,
            .seed = 61.0f
        });
        // sparkles rising round the freed marionette, and falling off the wand as it beats time
        if (magic > 0.0f)
        {
            for (int i = 0; i != 6; ++i)
            {
                const float life = fractionalPart(actTime * 0.7f + static_cast<float>(i) / 6.0f);
                const float angle = k_2Pi * static_cast<float>(i) / 6.0f + actTime * 1.3f;
                const FloatPoint at = puppet.pelvis
                    + FloatPoint{ std::cos(angle) * 46.0f, 30.0f - 110.0f * life } * unit;
                const Color color = i % 2 == 1 ? MagicColors::Cyan : MagicColors::Gold;
                drawSparkle(frame, at, 4.0f * unit * std::sin(k_pi * life), faded(color, magic));
            }
        }
        if (actTime > 3.4f)
        {
            for (int i = 0; i != 4; ++i)
            {
                const float life = fractionalPart(actTime * 1.6f + static_cast<float>(i) / 4.0f);
                const FloatPoint at = tip
                    + FloatPoint{ (pseudoRandom(i, 71.0f) - 0.5f) * 14.0f, 22.0f * life } * unit;
                const Color color = i % 2 == 1 ? MagicColors::Gold : MagicColors::White;
                drawSparkle(frame, at, 4.0f * unit * (1.0f - life), faded(color, 1.0f - life));
            }
        }
    }
}
