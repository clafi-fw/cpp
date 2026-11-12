module ClaFi.PathArt.DrumAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.MusicNotesEffect;
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

    namespace DrumColors
    {
        constexpr Color Shell{ 0xFFB8233A };
        constexpr Color ShellLight{ 0x2EFFFFFF };
        constexpr Color ShellShade{ 0x40000000 };
        constexpr Color Lacing{ 0xBFF3EFE6 };
        constexpr Color Hoop{ 0xFFD9A845 };
        constexpr Color Underside{ 0xFF8A1A2C };
        constexpr Color Skin{ 0xFFF3EFE6 };
        constexpr Color SkinStruck{ 0xFFFFFFFF };
        constexpr Color Stick{ 0xFFE8C9A0 };
        constexpr Color StickTip{ 0xFFF3E3C8 };
        constexpr Color Stand{ 0xFF9AA3AD };
        constexpr Color StandFoot{ 0xFF6B7684 };
        constexpr Color Cymbal{ 0xFFD9A845 };
        constexpr Color CymbalShine{ 0xFFF0CF72 };
        constexpr Color CymbalBell{ 0xFF9A7A2C };
        constexpr Color Ring{ 0xB3FFF6DC };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // In units of the upper body: the middle of the drum's head and of the cymbal.
        constexpr FloatPoint k_drumHead{ 0.0f, 6.0f };
        constexpr FloatPoint k_cymbal{ 118.0f, -120.0f };
        constexpr float k_stickLength{ 56.0f };
        constexpr float k_crashAt{ 8.5f };

        // A snare drum in units about the middle of its head.
        void drawSnare(const SceneFrame& drum, const float struck)
        {
            fillBox(drum, { -42.0f, 0.0f, 42.0f, 30.0f }, SolidColor{ DrumColors::Shell });
            fillBox(drum, { -42.0f, 0.0f, 42.0f, 30.0f }, LinearGradient::Simple({ -42.0f, 0.0f },
                { 42.0f, 0.0f }, DrumColors::ShellLight, DrumColors::ShellShade));
            PixelPath lacing;
            for (int i = 0; i != 8; ++i)
            {
                const float x = -38.0f + static_cast<float>(i) * 10.8f;
                lacing.moveTo(x, 4.0f);
                lacing.lineTo(x + 5.4f, 26.0f);
            }
            strokeShape(drum, lacing, SolidColor{ DrumColors::Lacing }, 1.1f);
            const SolidColor hoop{ DrumColors::Hoop };
            fillBox(drum, { -43.0f, -1.0f, 43.0f, 4.0f }, hoop);
            fillBox(drum, { -43.0f, 27.0f, 43.0f, 32.0f }, hoop);
            fillOval(drum, { 0.0f, 31.0f }, 43.0f, 6.0f, SolidColor{ DrumColors::Underside });
            fillOval(drum, { 0.0f, 0.0f }, 43.0f, 8.5f, hoop);
            fillOval(drum, { 0.0f, -0.6f }, 40.0f, 7.0f,
                SolidColor{ colorBetween(DrumColors::Skin, DrumColors::SkinStruck, struck) });
        }

        void drawStick(const SceneFrame& upper, const FloatPoint hand, const float angle)
        {
            const FloatPoint direction = { std::cos(angle), std::sin(angle) };
            const FloatPoint tip = hand + direction * k_stickLength;
            drawSegment(upper, hand - direction * 10.0f, tip, SolidColor{ DrumColors::Stick },
                3.6f);
            fillDisc(upper, tip, 3.0f, SolidColor{ DrumColors::StickTip });
        }

        // Where a hand is as it strikes, once every `period` seconds from `offset`.
        struct Stroke
        {
            FloatPoint hand;
            float raised; // 0 on the drum, 1 at the top of the stroke
            float struck; // 1 the moment it lands, falling to 0
        };

        [[nodiscard]] Stroke strokeAt(const float actTime, const float side, const float offset,
            const float period, const float height)
        {
            const float phase = fractionalPart((actTime - offset) / period);
            const float raised = std::pow(std::sin(k_pi * phase), 2.0f) * height;
            const FloatPoint low = { side * 24.0f, -46.0f };
            const FloatPoint high = { side * 36.0f, -86.0f };
            return {
                pointBetween(low, high, raised),
                raised,
                phase < 0.18f ? 1.0f - phase / 0.18f : 0.0f
            };
        }

        // The stick points at the drum's head and cocks its tip up as the hand rises.
        [[nodiscard]] float stickAngleOf(const float side, const Stroke& stroke)
        {
            const FloatPoint toHead = FloatPoint{ side * 8.0f, k_drumHead.y - 4.0f } - stroke.hand;
            return std::atan2(toHead.y, toHead.x) + side * 0.9f * stroke.raised;
        }
    }

    // Seconds: strokes from each hand every 0.625 seconds, the right stick twirling from 3.75,
    // a roll from 7.4, the crash at 8.5, the ta-da from 9.
    void DrumAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float x = stage.centerX + 10.0f * unit * stage.closeness;
        const float bounce = 3.0f * std::pow(std::cos(k_pi * actTime / 0.3125f), 2.0f);
        m_android = standingAndroid(frame, { x, stage.floorY }, time.cycleTime, bounce, 24.0f);

        const bool rolling = actTime > 7.4f && actTime < 8.4f;
        const float period = rolling ? 0.12f : 0.625f;
        const float height = rolling ? 0.35f : 1.0f;
        const Stroke left = strokeAt(actTime, -1.0f, 0.0f, period, height);
        const Stroke right = strokeAt(actTime, 1.0f, period * 0.5f, period, height);
        const float crash = smoothStep(8.35f, 8.55f, actTime)
            * (1.0f - smoothStep(8.75f, 9.1f, actTime));
        const float tada = smoothStep(9.0f, 9.4f, actTime);
        FloatPoint rightHand = pointBetween(right.hand, { 96.0f, -126.0f }, crash);
        rightHand = pointBetween(rightHand, ActHands::RightRaised, tada);
        const FloatPoint leftHand = pointBetween(left.hand, ActHands::LeftRaised, tada);

        m_leftStick = std::lerp(stickAngleOf(-1.0f, left), -k_pi * 0.6f, tada);
        const bool twirling = actTime > 3.75f && actTime < 4.85f;
        const float twirl = k_2Pi * 2.0f * smoothStep(0.0f, 1.0f, (actTime - 3.75f) / 1.1f);
        m_rightStick = stickAngleOf(1.0f, right) + (twirling ? twirl : 0.0f);
        const FloatPoint toCymbal = k_cymbal - rightHand;
        m_rightStick = std::lerp(m_rightStick, std::atan2(toCymbal.y, toCymbal.x), crash);
        m_rightStick = std::lerp(m_rightStick, -k_pi * 0.4f, tada);

        for (AndroidSide* side : { &m_android.left, &m_android.right })
        {
            side->handShape = HandShape::Fist;
            side->forearmOnTop = true;
        }
        m_android.left.hand = leftHand;
        m_android.right.hand = rightHand;
        m_android.headTurn = 0.06f * std::sin(k_2Pi * actTime / 0.625f);
        m_android.look = twirling ? 5.0f : -2.0f;
        if (actTime > 9.2f)
            m_android.lights = flashingLights(actTime);
        m_struck = std::max(left.struck, right.struck) * (1.0f - tada);
        m_crashAge = actTime > k_crashAt ? actTime - k_crashAt : -1.0f;
        m_floorY = stage.floorY;
        m_spotlightX = x + 30.0f * unit;
        m_actTime = actTime;
    }

    // The cymbal stands on the floor beside the android; the drum hangs in front of it.
    void DrumAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        const FloatPoint cymbal = androidUpperToFrame(frame, m_android, k_cymbal);
        const float standFootX = cymbal.x + 6.0f * unit;
        const float floorLine = m_floorY + 2.0f * unit;
        drawSegment(frame, { standFootX, floorLine }, cymbal, SolidColor{ DrumColors::Stand },
            3.0f * unit, StrokeCap::Butt);
        drawSegment(frame, { standFootX - 18.0f * unit, floorLine },
            { standFootX + 18.0f * unit, floorLine }, SolidColor{ DrumColors::StandFoot },
            3.0f * unit);
        const float shaken = std::clamp(m_crashAge / 1.4f, 0.0f, 1.0f);
        const float wobble = m_crashAge >= 0.0f
            ? 0.35f * std::exp(-shaken * 3.0f) * std::sin(k_2Pi * shaken * 4.0f)
            : 0.0f;
        const SceneFrame plate = frame.within(Matrix3x2::Translation(cymbal)
            * Matrix3x2::Rotation(-0.12f + wobble) * Matrix3x2::Scale(unit));
        fillOval(plate, { 0.0f, 0.0f }, 34.0f, 5.5f, SolidColor{ DrumColors::Cymbal });
        fillOval(plate, { 0.0f, -1.0f }, 30.0f, 3.4f, SolidColor{ DrumColors::CymbalShine });
        fillDisc(plate, { 0.0f, -2.0f }, 3.0f, SolidColor{ DrumColors::CymbalBell });
        if (m_crashAge >= 0.0f)
            drawSparkleBurst(frame, SparkleBurst{
                .center = cymbal,
                .age = shaken,
                .count = 9,
                .seed = 81.0f,
                .size = 0.9f,
                .colors = { MagicColors::Gold, MagicColors::White, MagicColors::Gold }
            });

        drawAndroid(frame, m_android);
        const SceneFrame upper = androidUpperFrame(frame, m_android);
        drawSnare(upper.within(Matrix3x2::Translation(k_drumHead)), m_struck);
        drawStick(upper, m_android.left.hand, m_leftStick);
        drawStick(upper, m_android.right.hand, m_rightStick);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
        drawAndroidForearm(frame, m_android, AudienceSide::Right);
        if (m_struck > 0.2f)
        {
            const FloatPoint ring = androidUpperToFrame(frame, m_android, { 0.0f, -8.0f });
            PixelPath lines;
            for (const float side : { -1.0f, 1.0f })
            {
                lines.moveTo(ring + FloatPoint{ side * 46.0f, -6.0f } * unit);
                lines.lineTo(ring + FloatPoint{ side * 58.0f, -16.0f } * unit);
            }
            strokeShape(frame, lines, SolidColor{ faded(DrumColors::Ring, m_struck) }, 1.6f * unit);
        }
        drawMusicNotes(frame, MusicNotes{
            .origin = androidUpperToFrame(frame, m_android, { 0.0f, -60.0f }),
            .spread = 140.0f,
            .time = m_actTime,
            .seed = 11.0f
        });
    }
}
