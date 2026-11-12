module ClaFi.PathArt.RabbitSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace RabbitColors
    {
        constexpr Color Fur{ 0xFFF4F1EC };
        constexpr Color Muzzle{ 0xFFE7E2DA };
        constexpr Color EarInside{ 0xFFF2A5B5 };
        constexpr Color Eye{ 0xFF2B1D14 };
        constexpr Color EyeShine{ 0xFFFFFFFF };
        constexpr Color Nose{ 0xFFE8738B };
        constexpr Color Whisker{ 0xCC8C8379 };
    }

    namespace
    {
        // Below the rim the whole rabbit has sunk out of sight.
        constexpr float k_sunkDepth{ 64.0f };
    }

    // Everything below the rim is clipped away, so the rabbit rises out of whatever it is in.
    void drawRabbit(const SceneFrame& frame, const RabbitPose& pose)
    {
        if (pose.rise <= 0.0f)
            return;
        const SceneFrame rabbit = frame.within(Matrix3x2::Translation(pose.rim)
            * Matrix3x2::Rotation(pose.turn) * Matrix3x2::Scale(pose.size));
        PixelPath aboveRim;
        aboveRim.drawRoundedRect(FloatRect{ -60.0f, -140.0f, 60.0f, 0.5f }, 0.0f, 0.0f);
        rabbit.canvas.pushClip(aboveRim, &rabbit.toCanvas);

        const float sunk = (1.0f - pose.rise) * k_sunkDepth;
        const float time = pose.time;
        const float look = pose.look;
        const float twitch = 0.12f * std::sin(k_2Pi * time * 1.7f)
            * std::max(0.0f, std::sin(k_2Pi * time * 0.31f));
        const SolidColor fur{ RabbitColors::Fur };
        for (const float side : { -1.0f, 1.0f })
        {
            const float lean = side * 0.2f + (side < 0.0f ? twitch : -twitch * 0.5f);
            const SceneFrame ear = rabbit.within(Matrix3x2::Translation(side * 6.0f, -30.0f + sunk)
                * Matrix3x2::Rotation(lean));
            fillOval(ear, { 0.0f, -15.0f }, 5.5f, 17.0f, fur);
            fillOval(ear, { 0.0f, -14.0f }, 2.6f, 12.0f, SolidColor{ RabbitColors::EarInside });
        }
        fillDisc(rabbit, { 0.0f, -18.0f + sunk }, 15.0f, fur);
        fillOval(rabbit, { 0.0f, -8.0f + sunk }, 13.0f, 7.0f, SolidColor{ RabbitColors::Muzzle });
        for (const float side : { -1.0f, 1.0f })
        {
            const FloatPoint eye = { side * 5.5f + look, -21.0f + sunk };
            fillDisc(rabbit, eye, 2.4f, SolidColor{ RabbitColors::Eye });
            fillDisc(rabbit, eye + FloatPoint{ -0.8f, -0.8f }, 0.8f,
                SolidColor{ RabbitColors::EyeShine });
        }
        const float sniff = 0.6f * std::sin(k_2Pi * time * 5.0f);
        fillOval(rabbit, { look * 0.5f, -13.0f + sunk + sniff }, 2.6f, 1.9f,
            SolidColor{ RabbitColors::Nose });
        PixelPath whiskers;
        for (const float side : { -1.0f, 1.0f })
        {
            const float rootX = side * 4.0f + look * 0.5f;
            whiskers.moveTo(rootX, -11.0f + sunk);
            whiskers.lineTo(rootX + side * 12.0f, -13.0f + sunk);
            whiskers.moveTo(rootX, -10.0f + sunk);
            whiskers.lineTo(rootX + side * 11.0f, -8.0f + sunk);
        }
        strokeShape(rabbit, whiskers, SolidColor{ RabbitColors::Whisker }, 0.6f);
        if (pose.rise > 0.85f)
        {
            for (const float side : { -1.0f, 1.0f })
                fillOval(rabbit, { side * 11.0f, -1.0f + (1.0f - pose.rise) * 30.0f }, 5.5f, 3.6f,
                    fur);
        }
        rabbit.canvas.popClip();
    }
}
