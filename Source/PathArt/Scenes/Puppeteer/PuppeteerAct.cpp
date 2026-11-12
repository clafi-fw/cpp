module ClaFi.PathArt.PuppeteerAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
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
}
