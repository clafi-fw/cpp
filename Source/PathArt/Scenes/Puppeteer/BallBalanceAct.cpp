module ClaFi.PathArt.BallBalanceAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace BallColors
    {
        constexpr std::array<Color, 4> Wedges{ Color{ 0xFFC2412D }, Color{ 0xFFF6E7C8 },
            Color{ 0xFF3D7FCB }, Color{ 0xFFD9A845 } };
        constexpr Color Shade{ 0x991A0D08 };
        constexpr Color Shine{ 0x80FFFFFF };
        constexpr Color Hub{ 0xFFD9A845 };
        constexpr Color Line{ 0x8C2A1A14 };
        constexpr Color Shadow{ 0x73120807 };
    }

    namespace
    {
        constexpr float k_ballRadius{ 54.0f };
        // The ball rolls to and fro once in 6.6 seconds and comes to rest in the middle by 8.4.
        constexpr float k_rollSeconds{ 6.6f };

        [[nodiscard]] float rolledAt(const float actTime, const float reach)
        {
            return reach * std::sin(k_2Pi * actTime / k_rollSeconds)
                * (1.0f - smoothStep(6.6f, 8.4f, actTime));
        }

        // Eight painted wedges turn with the ball; its shading and shine keep to the light.
        void drawCircusBall(const SceneFrame& frame, const FloatPoint middle, const float radius,
            const float turn)
        {
            for (int i = 0; i != 8; ++i)
            {
                const float from = turn + static_cast<float>(i) * k_2Pi / 8.0f;
                const float to = from + k_2Pi / 8.0f;
                PixelPath wedge;
                wedge.moveTo(middle);
                wedge.lineTo(middle + FloatPoint{ std::cos(from), std::sin(from) } * radius);
                wedge.arcTo({ radius, radius }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
                    middle + FloatPoint{ std::cos(to), std::sin(to) } * radius);
                wedge.close();
                fillShape(frame, wedge,
                    SolidColor{ BallColors::Wedges[static_cast<std::size_t>(i % 4)] });
            }
            PixelPath outline;
            addEllipse(outline, middle, radius, radius);
            frame.canvas.pushClip(outline, &frame.toCanvas);
            drawGlow(frame, middle + FloatPoint{ 0.5f, 0.55f } * radius, 1.35f * radius,
                BallColors::Shade);
            drawGlow(frame, middle - FloatPoint{ 0.38f, 0.42f } * radius, 0.62f * radius,
                BallColors::Shine);
            frame.canvas.popClip();
            const float unit = frame.unit;
            fillDisc(frame, middle, radius * 0.15f, SolidColor{ BallColors::Hub });
            PixelPath hub;
            addEllipse(hub, middle, radius * 0.15f, radius * 0.15f);
            strokeShape(frame, hub, SolidColor{ BallColors::Line }, 1.2f * unit);
            strokeShape(frame, outline, SolidColor{ BallColors::Line }, 1.5f * unit);
        }
    }

    // The android leans the way the ball rolls and wobbles to keep its balance; standing on the
    // ball's top, its feet keep flat as it leans.
    void BallBalanceAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        // The ball rolls no further left than to touch the marionette's stand, inside the drapes.
        const float standX = stage.drapeWidth / unit + 80.0f;
        const float reach = std::clamp(stage.centerX / unit - standX - k_ballRadius, 0.0f,
            150.0f * stage.closeness);
        const float rolled = rolledAt(actTime, reach);
        constexpr float k_step{ 0.02f };
        const float speed = (rolledAt(actTime + k_step, reach) - rolledAt(actTime - k_step, reach))
            / (2.0f * k_step);
        const float topSpeed = reach * k_2Pi / k_rollSeconds + 1e-6f;
        m_floorY = stage.floorY;
        m_ball = { stage.centerX + rolled * unit, stage.floorY - k_ballRadius * unit };
        m_ballTurn = rolled / k_ballRadius;

        const float oneLeg = smoothStep(8.1f, 8.7f, actTime);
        const float wobble = std::sin(k_2Pi * actTime * 1.4f);
        const float lean = 0.13f * speed / topSpeed + 0.05f * wobble * (1.0f - 0.5f * oneLeg);
        const float crouch = 14.0f + 4.0f * wobble;
        const float ankleY = m_ball.y - (k_ballRadius + AndroidBody::AnkleHeight) * unit;
        const float legHeight = AndroidBody::StandingHeight - AndroidBody::AnkleHeight - crouch;
        m_android = {
            .pelvis = FloatPoint{ m_ball.x, ankleY } + rotated({ 0.0f, -legHeight * unit }, lean),
            .tilt = lean,
            .cycleTime = time.cycleTime
        };
        plantAndroidFeet(frame, m_android, { m_ball.x - 15.0f * unit, ankleY },
            { m_ball.x + 15.0f * unit, ankleY });
        m_android.right.foot = pointBetween(m_android.right.foot, { 90.0f, 66.0f }, oneLeg);
        m_android.left.footTurn = -lean;
        m_android.right.footTurn = -lean - 0.9f * oneLeg;

        const float swing = 28.0f * std::sin(k_2Pi * actTime * 0.9f);
        m_android.left.hand = pointBetween({ -142.0f, -112.0f + swing }, ActHands::LeftRaised,
            oneLeg);
        m_android.right.hand = pointBetween({ 142.0f, -112.0f - swing }, ActHands::RightRaised,
            oneLeg);
        m_android.headTurn = -lean * 0.6f;
        m_android.look = std::clamp(-speed / 60.0f, -6.0f, 6.0f);
        if (actTime > 8.7f)
            m_android.lights = flashingLights(actTime);
    }

    void BallBalanceAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        fillOval(frame, { m_ball.x, m_floorY + 2.0f * unit }, 50.0f * unit, 7.0f * unit,
            SolidColor{ BallColors::Shadow });
        drawCircusBall(frame, m_ball, k_ballRadius * unit, m_ballTurn);
        drawAndroid(frame, m_android);
    }
}
