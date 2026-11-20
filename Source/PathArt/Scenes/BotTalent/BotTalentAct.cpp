module ClaFi.PathArt.BotTalentAct;

import ClaFi.PathArt.AndroidSprite;
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

    namespace WandColors
    {
        constexpr Color Shaft{ 0xFF15131A };
        constexpr Color Tip{ 0xFFF3EFE6 };
    }

    namespace StandColors
    {
        constexpr Color Pole{ 0xFF6E4526 };
        constexpr Color Arm{ 0xFF8A5A33 };
        constexpr Color Foot{ 0xFF4A2C16 };
        constexpr Color Hook{ 0xFFD9A845 };
    }

    namespace
    {
        // A bowing stand bends above this share of its pole's height, by up to this angle.
        constexpr float k_standWaist{ 0.35f };
        constexpr float k_standBowAngle{ 0.9f };
    }

    float danceSway(const SceneTime time)
    {
        constexpr float k_swaysPerCycle{ 16.0f };
        return k_2Pi * k_swaysPerCycle * time.cycleTime;
    }

    float marionetteX(const ActStage& stage, const float sway)
    {
        const float unit = stage.frame.unit;
        return stage.centerX - 78.0f * unit * stage.closeness + 34.0f * unit * std::sin(sway);
    }

    float puppeteerX(const ActStage& stage)
    {
        return stage.centerX + 80.0f * stage.frame.unit * stage.closeness;
    }

    ChestLights flashingLights(const float actTime)
    {
        return fractionalPart(actTime * 3.0f) < 0.5f ? ChestLights::All : ChestLights::None;
    }

    // The shaft runs a little past the hand, where the hand drawn over it shows it gripped.
    void drawWand(const SceneFrame& frame, const FloatPoint hand, const FloatPoint tip)
    {
        const float unit = frame.unit;
        const FloatPoint along = tip - hand;
        const float length = std::hypot(along.x, along.y);
        const FloatPoint direction = along * (1.0f / length);
        const FloatPoint tail = hand - direction * (8.0f * unit);
        const FloatPoint tipStart = tip - direction * (10.0f * unit);
        drawSegment(frame, tail, tip, SolidColor{ WandColors::Shaft }, 5.5f * unit);
        drawSegment(frame, tipStart, tip, SolidColor{ WandColors::Tip }, 5.5f * unit);
    }

    MarionetteStand marionetteStandOf(const ActStage& stage)
    {
        const float unit = stage.frame.unit;
        const FloatPoint hook = {
            stage.drapeWidth + 80.0f * unit,
            stage.floorY - 304.0f * unit
        };
        const float poleX = hook.x - 34.0f * unit;
        return {
            .foot = { poleX, stage.floorY },
            .top = { poleX, hook.y - 18.0f * unit },
            .hook = hook
        };
    }

    // Upright, the pole is one straight segment. Bowing, it bends above the waist into an arc of
    // its own length, and the arm and the hook turn with its top.
    void drawMarionetteStand(const SceneFrame& frame, const MarionetteStand& stand,
        const StandPose& pose)
    {
        if (pose.shown <= 0.0f)
            return;
        std::optional<ScopedCanvasOpacity> fading;
        if (pose.shown < 1.0f)
            fading.emplace(frame.canvas, pose.shown);
        const float unit = frame.unit;
        fillOval(frame, stand.foot + FloatPoint{ 0.0f, unit }, 24.0f * unit, 5.0f * unit,
            SolidColor{ StandColors::Foot });
        const float bend = k_standBowAngle * pose.bow;
        FloatPoint top = stand.top;
        if (bend <= 0.0f)
        {
            drawSegment(frame, stand.foot, stand.top, SolidColor{ StandColors::Pole }, 6.0f * unit,
                StrokeCap::Butt);
        }
        else
        {
            const FloatPoint waist = FloatPoint::between(stand.foot, stand.top, k_standWaist);
            const float radius = (waist.y - stand.top.y) / bend;
            top = waist + FloatPoint{ radius * (1.0f - std::cos(bend)), -radius * std::sin(bend) };
            PixelPath pole;
            pole.moveTo(stand.foot);
            pole.lineTo(waist);
            pole.quadTo(waist - FloatPoint{ 0.0f, radius * std::tan(bend * 0.5f) }, top);
            strokeShape(frame, pole, SolidColor{ StandColors::Pole }, 6.0f * unit,
                StrokeCap::Butt);
        }
        const FloatPoint armDirection = { std::cos(bend), std::sin(bend) };
        const FloatPoint hookDirection = { -armDirection.y, armDirection.x };
        const float poleToHook = stand.hook.x - stand.top.x;
        const FloatPoint hookTop = top + armDirection * poleToHook;
        drawSegment(frame, top - armDirection * (4.0f * unit),
            top + armDirection * (poleToHook + 5.0f * unit), SolidColor{ StandColors::Arm },
            6.0f * unit);
        drawSegment(frame, hookTop + hookDirection * (2.0f * unit),
            hookTop + hookDirection * (stand.hook.y - stand.top.y - 4.0f * unit),
            SolidColor{ StandColors::Hook }, 2.0f * unit);
    }
}
