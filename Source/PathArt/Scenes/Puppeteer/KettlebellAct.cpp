module ClaFi.PathArt.KettlebellAct;

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

    namespace KettlebellColors
    {
        constexpr Color Iron{ 0xFF2F2E36 };
        constexpr Color Handle{ 0xFF2B2A31 };
        constexpr Color HandleShine{ 0x2EFFFFFF };
        constexpr Color Shade{ 0xB30B0B0E };
        constexpr Color Shine{ 0x59FFFFFF };
        constexpr Color Plate{ 0xFFD9A845 };
        constexpr Color PlateLine{ 0xCC6E4A14 };
        constexpr Color Shadow{ 0x66120807 };
        constexpr Color Steam{ 0x8CEEF2F5 };
        constexpr Color Crack{ 0xD9140A05 };
        constexpr Color Dust{ 0x8CEADBB8 };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        constexpr float k_bellRadius{ 40.0f };
        // From the middle of the ball to the top of the handle, where the hands grip it.
        constexpr float k_gripReach{ 73.0f };
        // The ball's flat bottom, below its middle.
        constexpr float k_baseDepth{ 0.88f * k_bellRadius };
        constexpr float k_letGoAt{ 6.45f };
        constexpr float k_dustSeconds{ 0.9f };
        // Units a second squared the bell falls at, and its speed forward as it is let go.
        constexpr float k_gravity{ 2600.0f };
        constexpr float k_throwSpeed{ -300.0f };
        constexpr FloatPoint k_chest{ 0.0f, -82.0f };
        constexpr FloatPoint k_overhead{ 0.0f, -212.0f };

        // The lifter and the bell while the bell is in its hands, at a time into the act.
        struct Lift
        {
            AndroidPose android;
            FloatPoint grip; // the middle of the handle, in units of the upper body
            FloatPoint bell; // the middle of the ball, in the frame
            float bellTurn;
            float shiver; // how hard it trembles, 0 to 1
            float tremble; // the tremble now, -1 to 1
        };

        // Seconds: a squat to the handle from 1, a strain from 1.6, the bell flipped up to the
        // chest from 3, a dip and a press overhead from 4, held trembling from 4.7.
        [[nodiscard]] Lift liftAt(const ActStage& stage, const float actTime, const float cycleTime)
        {
            const SceneFrame& frame = stage.frame;
            const float unit = frame.unit;
            const float x = stage.centerX + 40.0f * unit * stage.closeness;
            const float restY = stage.floorY - k_baseDepth * unit;
            const float squat = smoothStep(1.0f, 1.6f, actTime)
                * (1.0f - smoothStep(3.0f, 3.6f, actTime));
            const float strain = smoothStep(1.6f, 1.8f, actTime)
                * (1.0f - smoothStep(3.0f, 3.1f, actTime));
            const float budge = strain * smoothStep(1.8f, 3.0f, actTime);
            const float flip = smoothStep(0.0f, 1.0f, (actTime - 3.0f) / 0.7f);
            const float pushDip = std::sin(k_pi * std::clamp((actTime - 4.0f) / 0.35f, 0.0f, 1.0f));
            const float press = smoothStep(0.0f, 1.0f, (actTime - 4.25f) / 0.45f);
            const bool overhead = actTime > 4.7f;
            const float shiver = overhead ? 1.0f : (actTime > 1.7f && actTime < 3.0f ? 0.7f : 0.0f);
            const float tremble = shiver * std::sin(k_2Pi * actTime * 23.0f);
            const float kneesWobble = overhead
                ? 5.0f + 4.0f * std::sin(k_2Pi * actTime * 4.5f)
                : 0.0f;
            const float dip = AndroidBody::StandingHeight
                - (std::lerp(AndroidBody::StandingHeight, 72.0f, squat) + 8.0f * budge
                    - 18.0f * pushDip - kneesWobble);
            AndroidPose android = standingAndroid(frame, { x, stage.floorY }, cycleTime, dip,
                34.0f);

            const FloatPoint floorGrip = androidFrameToUpper(frame, android,
                { x, restY - (k_gripReach + 6.0f * budge) * unit });
            FloatPoint grip = pointBetween(floorGrip, k_chest, flip);
            grip = pointBetween(grip, k_overhead, press);
            grip = grip + FloatPoint{ 1.5f * tremble, 1.2f * tremble };
            const float turn = k_pi * flip;
            FloatPoint bell = androidUpperToFrame(frame, android, grip)
                + rotated({ 0.0f, k_gripReach * unit }, turn);
            if (actTime < 1.6f)
                bell = { x, restY };
            return {
                .android = android,
                .grip = grip,
                .bell = bell,
                .bellTurn = actTime < 1.6f ? 0.0f : turn,
                .shiver = shiver,
                .tremble = tremble
            };
        }

        // A kettlebell in its units about the middle of its ball, handle up: a flat-bottomed ball
        // of iron with a gold plate, the handle arched over it.
        void drawKettlebell(const SceneFrame& bell)
        {
            constexpr float k_radius = k_bellRadius;
            PixelPath handle;
            handle.moveTo(-21.0f, -k_radius * 0.55f);
            handle.lineTo(-24.0f, -k_radius - 4.0f);
            handle.arcTo({ 24.0f, 24.0f }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
                { 24.0f, -k_radius - 4.0f });
            handle.lineTo(21.0f, -k_radius * 0.55f);
            strokeShape(bell, handle, SolidColor{ KettlebellColors::Handle }, 10.0f);
            PixelPath shine;
            const float shineFrom = k_pi * 1.15f;
            const float shineTo = k_pi * 1.6f;
            const FloatPoint arcMiddle = { 0.0f, -k_radius - 4.0f };
            shine.moveTo(
                arcMiddle + FloatPoint{ std::cos(shineFrom), std::sin(shineFrom) } * 24.0f);
            shine.arcTo({ 24.0f, 24.0f }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
                arcMiddle + FloatPoint{ std::cos(shineTo), std::sin(shineTo) } * 24.0f);
            strokeShape(bell, shine, SolidColor{ KettlebellColors::HandleShine }, 3.0f);

            const float baseHalf = std::sqrt(k_radius * k_radius - k_baseDepth * k_baseDepth);
            PixelPath ball;
            ball.moveTo(-baseHalf, k_baseDepth);
            ball.arcTo({ k_radius, k_radius }, 0.0f, ArcSize::Large, ArcSweep::Clockwise,
                { baseHalf, k_baseDepth });
            ball.close();
            fillShape(bell, ball, SolidColor{ KettlebellColors::Iron });
            bell.canvas.pushClip(ball, &bell.toCanvas);
            drawGlow(bell, FloatPoint{ 0.45f, 0.5f } * k_radius, k_radius * 1.3f,
                KettlebellColors::Shade);
            drawGlow(bell, FloatPoint{ -0.35f, -0.4f } * k_radius, k_radius * 0.6f,
                KettlebellColors::Shine);
            bell.canvas.popClip();
            fillRoundedBox(bell, { -15.0f, -5.0f, 15.0f, 8.0f }, 3.0f,
                SolidColor{ KettlebellColors::Plate });
            drawSegment(bell, { -9.0f, 1.5f }, { 9.0f, 1.5f },
                SolidColor{ KettlebellColors::PlateLine }, 2.0f, StrokeCap::Butt);
        }

        // Puffs of steam from an ear bolt, rising and swelling out to its side; `age` runs 0 to 1.
        void drawSteamPuffs(const SceneFrame& frame, const FloatPoint ear, const float age,
            const float side)
        {
            const float unit = frame.unit;
            for (int i = 0; i != 3; ++i)
            {
                const float share = std::clamp(age - static_cast<float>(i) * 0.12f, 0.0f, 1.0f);
                if (share <= 0.0f)
                    continue;
                const FloatPoint at = ear
                    + FloatPoint{ side * (6.0f + 26.0f * share), -(4.0f + 34.0f * share) } * unit;
                fillDisc(frame, at, (4.0f + 9.0f * share) * unit,
                    SolidColor{ faded(KettlebellColors::Steam, 1.0f - share) });
            }
        }

        // Dust thrown out to both sides of where the bell landed; `settle` runs 0 to 1.
        void drawLandingDust(const SceneFrame& frame, const FloatPoint landed, const float settle)
        {
            const float unit = frame.unit;
            const float spread = std::sqrt(settle);
            for (int i = 0; i != 10; ++i)
            {
                const float side = i % 2 == 0 ? -1.0f : 1.0f;
                const float spreadShare = spread * (0.5f + 0.5f * pseudoRandom(i, 97.0f));
                const float reach = (20.0f + 70.0f * spreadShare) * unit;
                const float rise = (4.0f + 22.0f * settle * pseudoRandom(i, 98.0f)) * unit;
                fillDisc(frame, { landed.x + side * reach, landed.y - rise },
                    (6.0f + 14.0f * settle) * unit,
                    SolidColor{ faded(KettlebellColors::Dust, 1.0f - settle) });
            }
        }

        // Cracks run out across the boards from where the bell landed, `grown` of their length.
        void drawCracks(const SceneFrame& frame, const FloatPoint landed, const float grown)
        {
            const float unit = frame.unit;
            PixelPath cracks;
            for (int crack = 0; crack != 5; ++crack)
            {
                const float angle = k_pi * (0.05f + 0.9f * (static_cast<float>(crack)
                    + pseudoRandom(crack, 91.0f) * 0.5f) / 5.0f);
                FloatPoint at = landed;
                cracks.moveTo(at);
                for (int step = 1; step <= 4; ++step)
                {
                    const int index = crack * 7 + step;
                    const float length = (14.0f + 10.0f * pseudoRandom(index, 93.0f)) * unit
                        * grown;
                    const float wander = (pseudoRandom(index, 95.0f) - 0.5f) * 0.9f;
                    at = at + FloatPoint{
                        std::cos(angle + wander) * length * 1.9f,
                        std::sin(angle + wander) * length * 0.35f,
                    };
                    cracks.lineTo(at);
                }
            }
            strokeShape(frame, cracks, SolidColor{ KettlebellColors::Crack }, 1.6f * unit);
        }
    }

    // Let go at 6.45, the bell falls forward, turning back onto its base, and lands; the android
    // hops back out of the way, is thrown off its feet by the jolt, looks, and flexes.
    void KettlebellAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const Lift held = liftAt(stage, std::min(actTime, k_letGoAt), time.cycleTime);
        const float x = stage.centerX + 40.0f * unit * stage.closeness;
        m_restY = stage.floorY - k_baseDepth * unit;
        m_android = held.android;
        m_bell = held.bell;
        m_bellTurn = held.bellTurn;
        m_landedAge = -1.0f;
        if (actTime > k_letGoAt)
        {
            const float back = 46.0f * smoothStep(0.0f, 1.0f, (actTime - k_letGoAt) / 0.3f);
            m_android = standingAndroid(frame, { x + back * unit, stage.floorY }, time.cycleTime,
                0.0f, 34.0f);
            const float fallSeconds = std::sqrt(
                2.0f * (m_restY - held.bell.y) / (k_gravity * unit));
            const float fallen = actTime - k_letGoAt;
            if (fallen < fallSeconds)
            {
                m_bell = held.bell + FloatPoint{
                    k_throwSpeed * unit * fallen,
                    0.5f * k_gravity * unit * fallen * fallen,
                };
                m_bellTurn = held.bellTurn + (k_2Pi - held.bellTurn) * fallen / fallSeconds;
            }
            else
            {
                m_landedAge = fallen - fallSeconds;
                const float hop = m_landedAge < 0.22f
                    ? 9.0f * unit * std::sin(k_pi * m_landedAge / 0.22f)
                    : 0.0f;
                m_bell = { held.bell.x + k_throwSpeed * unit * fallSeconds, m_restY - hop };
                m_bellTurn = 0.12f * std::exp(-m_landedAge / 0.35f)
                    * std::sin(k_2Pi * m_landedAge * 3.0f);
            }
        }
        if (m_landedAge >= 0.0f && m_landedAge < 0.35f)
        {
            const float lift = 16.0f * std::sin(k_pi * m_landedAge / 0.35f);
            m_android.pelvis.y -= lift * unit;
            m_android.left.foot.y -= lift;
            m_android.right.foot.y -= lift;
        }

        // hands: rubbing them, on the handle, thrown up out of the way, on the hips, the flex
        const float rubbing = 4.0f * std::sin(k_2Pi * actTime * 5.0f);
        const float onHandle = smoothStep(0.9f, 1.4f, actTime);
        const float thrownUp = smoothStep(k_letGoAt, k_letGoAt + 0.15f, actTime);
        const float onHips = smoothStep(7.4f, 7.8f, actTime);
        const float flexing = smoothStep(8.4f, 8.8f, actTime);
        for (const float side : { -1.0f, 1.0f })
        {
            FloatPoint hand = pointBetween({ side * 8.0f + rubbing, -84.0f },
                held.grip + FloatPoint{ side * 11.0f, 0.0f }, onHandle);
            hand = pointBetween(hand, { side * 70.0f, -190.0f }, thrownUp);
            hand = pointBetween(hand, side < 0.0f ? ActHands::LeftOnHip : ActHands::RightOnHip,
                onHips);
            hand = pointBetween(hand, { side * 104.0f, -152.0f }, flexing);
            AndroidSide& armSide = side < 0.0f ? m_android.left : m_android.right;
            armSide.hand = hand;
            const bool gripping = actTime > 1.3f && actTime < k_letGoAt;
            armSide.handShape = gripping || actTime > 8.4f ? HandShape::Fist : HandShape::Open;
            armSide.forearmOnTop = gripping;
        }
        m_straining = actTime < k_letGoAt && held.shiver > 0.0f;
        m_bellHeld = actTime >= 1.6f && actTime <= k_letGoAt;
        m_android.headTurn = (m_straining ? 0.02f * held.tremble : 0.0f)
            + (actTime > 7.4f && actTime < 8.4f ? -0.08f : 0.0f);
        m_android.look = actTime < 1.6f ? -2.0f : (actTime > 7.0f && actTime < 8.4f ? -6.0f : 0.0f);
        if (m_straining)
        {
            const bool flash = fractionalPart(actTime * 8.0f) < 0.5f;
            m_android.eyeOpening = 0.35f;
            m_android.lights = flash ? ChestLights::All : ChestLights::None;
            m_android.antenna = flash ? AntennaLight::On : AntennaLight::Off;
        }
        if (actTime > 8.8f)
            m_android.lights = flashingLights(actTime);
        m_floorY = stage.floorY;
        m_spotlightX = x - 40.0f * unit;
        m_actTime = actTime;
    }

    void KettlebellAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        const FloatPoint landed = { m_bell.x, m_floorY + 2.0f * unit };
        if (m_landedAge >= 0.0f)
            drawCracks(frame, landed, smoothStep(0.0f, 0.1f, m_landedAge));
        if (!m_bellHeld)
        {
            const float near = std::clamp(1.0f - (m_restY - m_bell.y) / (300.0f * unit), 0.3f,
                1.0f);
            fillOval(frame, { m_bell.x, m_floorY + unit }, 30.0f * unit * near, 5.0f * unit * near,
                SolidColor{ faded(KettlebellColors::Shadow, near) });
        }
        drawAndroid(frame, m_android);
        drawKettlebell(frame.within(Matrix3x2::Translation(m_bell) * Matrix3x2::Rotation(m_bellTurn)
            * Matrix3x2::Scale(unit)));
        if (m_android.left.forearmOnTop)
            drawAndroidForearm(frame, m_android, AudienceSide::Left);
        if (m_android.right.forearmOnTop)
            drawAndroidForearm(frame, m_android, AudienceSide::Right);
        if (m_straining)
        {
            for (const float side : { -1.0f, 1.0f })
            {
                const FloatPoint ear = androidUpperToFrame(frame, m_android, AndroidBody::Neck
                    + rotated({ side * 40.0f, -28.0f }, m_android.headTurn));
                drawSteamPuffs(frame, ear,
                    fractionalPart(m_actTime * 1.6f + (side > 0.0f ? 0.5f : 0.0f)), side);
            }
        }
        if (m_landedAge >= 0.0f && m_landedAge < k_dustSeconds)
            drawLandingDust(frame, { m_bell.x, m_floorY }, m_landedAge / k_dustSeconds);
    }
}
