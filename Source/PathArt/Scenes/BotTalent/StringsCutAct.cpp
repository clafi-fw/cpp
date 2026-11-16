module ClaFi.PathArt.StringsCutAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SparkleBurstEffect;
import ClaFi.PathArt.TopHatSprite;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace StringsCutActColors
    {
        constexpr Color Smoke{ 0xFFF6EFE2 };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        constexpr float k_wandLength{ 66.0f };
        // The wand from the hip up over the head, then once and a half round a small circle.
        constexpr FloatPoint k_wandDown{ 46.0f, -24.0f };
        constexpr FloatPoint k_wandUp{ 70.0f, -176.0f };
        constexpr float k_downAngle{ k_pi * 0.42f };
        constexpr float k_upAngle{ -k_pi * 0.62f };
        // When the wand is raised for each wave: for the strings, the stand, the android itself.
        constexpr std::array<float, 3> k_waves{ 1.2f, 3.6f, 7.0f };
        // Where the left hand opens toward the stand while the wand is waved at it.
        constexpr FloatPoint k_leftHandTowardStand{ -128.0f, -150.0f };
        // Where the left hand rests on the chest while the wand is waved for the last time.
        constexpr FloatPoint k_leftHandOnChest{ -24.0f, -82.0f };
        constexpr float k_vanishAt{ 8.25f }; // the puff of smoke the android goes in
        constexpr float k_vanishSeconds{ 0.1f }; // its fade under the smoke, before its hat drops
        constexpr float k_hatGravity{ 1700.0f }; // in units a second, each second
        constexpr float k_hatBrimDepth{ 7.0f }; // the brim below its middle, in the hat's units

        // The right hand and the wand's angle.
        struct WandHold
        {
            FloatPoint hand; // in units about the pelvis
            float angle;
        };

        [[nodiscard]] WandHold heldBetween(const WandHold& from, const WandHold& to,
            const float share)
        {
            return {
                pointBetween(from.hand, to.hand, share),
                std::lerp(from.angle, to.angle, share)
            };
        }

        // The wand raised from `from` up over the head at `start`, then once and a half round a
        // small circle, held where the circle ends.
        [[nodiscard]] WandHold wavedFrom(const WandHold& from, const float start,
            const float actTime)
        {
            const float raise = smoothStep(start, start + 0.4f, actTime);
            const float circling = k_2Pi * 1.5f * smoothStep(start + 0.4f, start + 1.0f, actTime);
            return {
                pointBetween(from.hand, k_wandUp, raise)
                    + FloatPoint{ 10.0f * std::sin(circling), -8.0f * (1.0f - std::cos(circling)) },
                std::lerp(from.angle, k_upAngle, raise) + 0.5f * std::sin(circling)
            };
        }

        // The wand beating time for the dance.
        [[nodiscard]] WandHold beatingTimeAt(const float actTime)
        {
            const float beat = (actTime - 3.0f) / 0.625f;
            return {
                {
                    92.0f + 8.0f * std::sin(k_pi * beat),
                    -122.0f + 16.0f * std::abs(std::cos(k_pi * beat))
                },
                -k_pi * 0.32f + 0.35f * std::sin(k_pi * beat)
            };
        }

        // The wand waved at the strings, beating time, waved at the stand, beating time again,
        // then waved for the last time.
        [[nodiscard]] WandHold wandAt(const float actTime)
        {
            WandHold hold = wavedFrom({ k_wandDown, k_downAngle }, k_waves[0], actTime);
            hold = heldBetween(hold, beatingTimeAt(actTime), smoothStep(3.0f, 3.4f, actTime));
            hold = wavedFrom(hold, k_waves[1], actTime);
            hold = heldBetween(hold, beatingTimeAt(actTime), smoothStep(4.8f, 5.2f, actTime));
            return wavedFrom(hold, k_waves[2], actTime);
        }

        [[nodiscard]] FloatPoint tipOf(const WandHold& hold)
        {
            return hold.hand
                + FloatPoint{ std::cos(hold.angle), std::sin(hold.angle) } * k_wandLength;
        }

        // Sparkles flying from the wand's tip to a target in an arc, five in a line.
        void drawSparkleFlight(const SceneFrame& frame, const FloatPoint tip,
            const FloatPoint target, const float flight, const float arcHeight)
        {
            if (flight <= 0.0f || flight >= 1.0f)
                return;
            const float unit = frame.unit;
            for (int i = 0; i != 5; ++i)
            {
                const float share = std::clamp(flight - static_cast<float>(i) * 0.08f, 0.0f, 1.0f);
                const FloatPoint at = pointBetween(tip, target, share)
                    - FloatPoint{ 0.0f, arcHeight * unit * std::sin(k_pi * share) };
                const Color color = i % 2 == 1 ? MagicColors::Gold : MagicColors::White;
                drawSparkle(frame, at, 6.0f * unit, color.faded(1.0f - share * 0.3f));
            }
        }

        // Soft clouds of smoke up a column from `from` to `to`, spreading and rising with age.
        struct SmokeColumn
        {
            FloatPoint from;
            FloatPoint to;
            int clouds;
            float age; // seconds since the puff
            float density; // 0 to 1
            float scale; // the clouds' size and reach, 1 for a stand's height
            float seed;
        };

        void drawSmoke(const SceneFrame& frame, const SmokeColumn& smoke)
        {
            const float unit = frame.unit;
            const float last = static_cast<float>(smoke.clouds) - 0.4f;
            for (int cloud = 0; cloud != smoke.clouds; ++cloud)
            {
                const FloatPoint middle = pointBetween(smoke.from, smoke.to,
                    (static_cast<float>(cloud) + 0.4f) / last);
                for (int i = 0; i != 5; ++i)
                {
                    const float angle = k_2Pi * (static_cast<float>(i)
                        + pseudoRandom(cloud * 5 + i, smoke.seed)) / 5.0f;
                    const float reach = (6.0f + 26.0f * std::sqrt(smoke.age)) * smoke.scale * unit;
                    const FloatPoint at = middle + FloatPoint{
                        std::cos(angle) * reach * 1.3f,
                        std::sin(angle) * reach * 0.8f - 18.0f * unit * smoke.age * smoke.scale
                    };
                    drawGlow(frame, at, (30.0f + 22.0f * smoke.age) * smoke.scale * unit,
                        StringsCutActColors::Smoke.withOpacity(0.6f * smoke.density));
                }
            }
        }
    }

    // Seconds: the wand raised from 1.2 and waved round from 1.6; sparkles fly to the control at
    // 2.15 and run down the strings from 2.4; the strings fade by 3 and the control goes up in
    // sparkles; the wand beats time while the marionette dances; raised again from 3.6 and waved
    // round from 4; sparkles fly to the stand at 4.55 and it goes from 5; the wand beats time
    // again while the marionette rises and twirls; still from 6.8, the android waves the wand
    // over itself from 7, sparkles spiral up round it from 7.8, and it goes at 8.25.
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
        m_stand = marionetteStandOf(stage);

        // the head and the left hand turn to the stand while the wand is waved at it; for the
        // last wave the android stands still, facing the audience, its hand on its chest
        const float towardStand = smoothStep(3.5f, 3.9f, actTime)
            * (1.0f - smoothStep(5.2f, 5.6f, actTime));
        const float still = smoothStep(6.8f, 7.2f, actTime);
        const float bob = -2.5f * std::abs(std::sin(2.0f * sway)) * (1.0f - still);
        m_android = standingAndroid(frame, { puppeteerX(stage), stage.floorY }, time.cycleTime,
            bob);
        m_android.wearsHat = actTime < k_vanishAt;
        m_android.headTurn = (-0.1f + 0.06f * std::sin(sway) - 0.1f * towardStand)
            * (1.0f - still);
        m_android.look = (-5.0f + 2.0f * std::sin(sway) - 4.0f * towardStand) * (1.0f - still);

        const WandHold wand = wandAt(actTime);
        m_wandTip = tipOf(wand);

        // the left hand works the control, lets go and opens toward the marionette
        const float freed = smoothStep(2.7f, 3.2f, actTime);
        const FloatPoint grip = androidFrameToUpper(frame, m_android,
            m_marionette.control.center - FloatPoint{ 0.0f, 9.0f * unit });
        FloatPoint leftHand = pointBetween(grip,
            { -108.0f, -64.0f + 10.0f * std::sin(k_2Pi * actTime / 1.25f) }, freed);
        leftHand = pointBetween(leftHand, k_leftHandTowardStand, towardStand);
        leftHand = pointBetween(leftHand, k_leftHandOnChest, still);
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
        m_wornHat = androidWornHat(frame, m_android);
        m_floorY = stage.floorY;
        m_actTime = actTime;
    }

    // The stand goes under the smoke, from 5.1.
    StandPose StringsCutAct::standPose() const
    {
        return { .shown = 1.0f - smoothStep(5.1f, 5.25f, m_actTime) };
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

        // the android fades under the smoke as it thickens, its hat drawn on its own from then
        const float androidShown = 1.0f - smoothStep(k_vanishAt, k_vanishAt + k_vanishSeconds,
            actTime);
        if (androidShown > 0.0f)
        {
            std::optional<ScopedCanvasOpacity> fading;
            if (androidShown < 1.0f)
                fading.emplace(frame.canvas, androidShown);
            drawAndroid(frame, m_android);
        }
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
        if (androidShown > 0.0f)
        {
            std::optional<ScopedCanvasOpacity> fading;
            if (androidShown < 1.0f)
                fading.emplace(frame.canvas, androidShown);
            drawWand(frame, androidUpperToFrame(frame, m_android, m_android.right.hand), tip);
            drawAndroidForearm(frame, m_android, AudienceSide::Right);
        }

        // the trail behind the circling wand, at each wave
        for (const float wave : k_waves)
        {
            if (actTime <= wave + 0.3f || actTime >= std::min(wave + 1.3f, k_vanishAt))
                continue;
            for (int i = 0; i != 7; ++i)
            {
                const float behind = static_cast<float>(i) * 0.045f;
                const FloatPoint at = androidUpperToFrame(frame, m_android,
                    tipOf(wandAt(actTime - behind)));
                const Color color = i % 2 == 1 ? MagicColors::Cyan : MagicColors::Gold;
                drawSparkle(frame, at, (6.0f - static_cast<float>(i) * 0.7f) * unit,
                    color.faded(1.0f - static_cast<float>(i) / 8.0f));
            }
        }
        // sparkles flying across to the control, then glinting down every string
        const MarionetteControl& control = m_marionette.control;
        drawSparkleFlight(frame, tip, control.center, (actTime - 2.15f) / 0.3f, 40.0f);
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
                    MagicColors::White.faded(1.0f - 0.4f * glint));
        }
        drawSparkleBurst(frame, SparkleBurst{
            .center = control.center,
            .age = (actTime - 2.7f) / 0.7f,
            .count = 12,
            .seed = 61.0f
        });

        // sparkles flying over to the stand, then smoke round it for a second and a burst inside
        const MarionetteStand& stand = m_stand;
        const FloatPoint armMiddle = { (stand.top.x + stand.hook.x) * 0.5f, stand.top.y };
        drawSparkleFlight(frame, tip, armMiddle, (actTime - 4.55f) / 0.5f, 70.0f);
        const float smokeAge = actTime - 5.0f;
        if (smokeAge > 0.0f && smokeAge < 1.0f)
            drawSmoke(frame, SmokeColumn{
                .from = stand.foot,
                .to = armMiddle,
                .clouds = 4,
                .age = smokeAge,
                .density = std::min(1.0f, smokeAge * 10.0f) * (1.0f - smokeAge) * (1.0f - smokeAge),
                .scale = 1.0f,
                .seed = 13.0f
            });
        drawSparkleBurst(frame, SparkleBurst{
            .center = pointBetween(stand.foot, armMiddle, 0.55f),
            .age = (actTime - 5.0f) / 0.8f,
            .count = 12,
            .seed = 83.0f,
            .size = 1.3f,
            .colors = { MagicColors::Gold, MagicColors::Cyan, MagicColors::White }
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
                drawSparkle(frame, at, 4.0f * unit * std::sin(k_pi * life), color.faded(magic));
            }
        }
        if (actTime > 3.4f && actTime < k_vanishAt)
        {
            for (int i = 0; i != 4; ++i)
            {
                const float life = fractionalPart(actTime * 1.6f + static_cast<float>(i) / 4.0f);
                const FloatPoint at = tip
                    + FloatPoint{ (pseudoRandom(i, 71.0f) - 0.5f) * 14.0f, 22.0f * life } * unit;
                const Color color = i % 2 == 1 ? MagicColors::Gold : MagicColors::White;
                drawSparkle(frame, at, 4.0f * unit * (1.0f - life), color.faded(1.0f - life));
            }
        }
        // the android's own going: sparkles spiral up round it, it goes in a puff of smoke that
        // hangs on to the curtain, and its hat drops to the floor and rocks still
        const FloatPoint feet = { m_android.pelvis.x, m_floorY };
        const float spiralShare = (actTime - 7.8f) / (k_vanishAt - 7.8f);
        if (spiralShare > 0.0f && spiralShare < 1.0f)
        {
            for (int i = 0; i != 10; ++i)
            {
                const float order = static_cast<float>(i) / 10.0f;
                const float rise = std::clamp(spiralShare * 1.4f - order * 0.4f, 0.0f, 1.0f);
                const float angle = k_2Pi * order + actTime * 9.0f;
                const FloatPoint at = feet + FloatPoint{
                    std::cos(angle) * (62.0f - 20.0f * rise),
                    -20.0f - 300.0f * rise
                } * unit;
                const Color color = i % 2 == 1 ? MagicColors::Cyan : MagicColors::Gold;
                drawSparkle(frame, at, 5.0f * unit * std::sin(k_pi * rise), color);
            }
        }
        const float puffAge = actTime - k_vanishAt;
        if (puffAge > 0.0f)
        {
            drawSmoke(frame, SmokeColumn{
                .from = feet - FloatPoint{ 0.0f, 10.0f * unit },
                .to = feet - FloatPoint{ 0.0f, 320.0f * unit },
                .clouds = 5,
                .age = puffAge + 0.3f,
                .density = std::min(1.0f, puffAge * 16.0f)
                    * (1.0f - 0.45f * smoothStep(0.0f, 1.6f, puffAge)),
                .scale = 1.5f,
                .seed = 29.0f
            });
            drawSparkleBurst(frame, SparkleBurst{
                .center = feet - FloatPoint{ 0.0f, 190.0f * unit },
                .age = puffAge / 0.9f,
                .count = 16,
                .seed = 97.0f,
                .size = 1.8f,
                .colors = { MagicColors::Gold, MagicColors::Cyan, MagicColors::White }
            });
        }
        if (actTime >= k_vanishAt)
        {
            const float hatAge = std::max(0.0f, actTime - (k_vanishAt + k_vanishSeconds));
            TopHatPose hat = m_wornHat;
            const float landingY = m_floorY - k_hatBrimDepth * hat.size;
            const float gravity = k_hatGravity * unit;
            const float fallSeconds = std::sqrt(2.0f * (landingY - m_wornHat.brim.y) / gravity);
            if (hatAge < fallSeconds)
            {
                hat.brim.y += 0.5f * gravity * hatAge * hatAge;
                hat.turn = std::lerp(m_wornHat.turn, -0.18f, hatAge / fallSeconds);
            }
            else
            {
                const float landed = hatAge - fallSeconds;
                const float hop = landed < 0.24f ? std::sin(k_pi * landed / 0.24f) : 0.0f;
                hat.brim.y = landingY - 10.0f * unit * hop;
                hat.turn = -0.18f * std::exp(-landed / 0.15f) * std::cos(k_2Pi * landed / 0.3f);
            }
            drawTopHat(frame, hat);
            drawTopHatBrimLip(frame, hat);
        }
    }
}
