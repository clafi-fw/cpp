module ClaFi.PathArt.JackOLanternSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace JackOLanternColors
    {
        constexpr Color PumpkinGlow{ 0xFFFF9F43 };
        constexpr Color PumpkinShade{ 0xFFC9561A };
        constexpr Color Pumpkin{ 0xFFE8762B };
        constexpr Color Stem{ 0xFF4E6B2A };
        constexpr Color Candle{ 0xFFFFD27A };
    }

    void drawJackOLantern(const SceneFrame& frame, const JackOLanternPose& pose)
    {
        const float unit = frame.unit;
        const float cycleTime = pose.cycleTime;
        const float x = pose.base.x;
        const float y = pose.base.y;
        // Two waves beating against each other and a fast third make the candle flicker.
        const float beat = std::sin(k_2Pi * 37.0f * cycleTime)
            * std::sin(k_2Pi * 53.0f * cycleTime + 1.0f);
        const float flicker = 0.75f + 0.15f * beat + 0.1f * std::sin(k_2Pi * 91.0f * cycleTime);
        drawGlow(frame, { x, y - 12.0f * unit }, 60.0f * unit,
            JackOLanternColors::PumpkinGlow.withOpacity(0.32f * flicker));
        fillOval(frame, { x - 9.0f * unit, y - 12.0f * unit }, 11.0f * unit, 12.0f * unit,
            SolidColor{ JackOLanternColors::PumpkinShade });
        fillOval(frame, { x + 9.0f * unit, y - 12.0f * unit }, 11.0f * unit, 12.0f * unit,
            SolidColor{ JackOLanternColors::PumpkinShade });
        fillOval(frame, { x, y - 13.0f * unit }, 12.0f * unit, 13.0f * unit,
            SolidColor{ JackOLanternColors::Pumpkin });
        drawSegment(frame, { x, y - 25.0f * unit }, { x + 3.0f * unit, y - 31.0f * unit },
            SolidColor{ JackOLanternColors::Stem }, 3.0f * unit);

        // the carved face: two eyes and a jagged mouth
        PixelPath face;
        face.moveTo(x - 9.0f * unit, y - 16.0f * unit);
        face.lineTo(x - 3.0f * unit, y - 16.0f * unit);
        face.lineTo(x - 6.0f * unit, y - 21.0f * unit);
        face.close();
        face.moveTo(x + 3.0f * unit, y - 16.0f * unit);
        face.lineTo(x + 9.0f * unit, y - 16.0f * unit);
        face.lineTo(x + 6.0f * unit, y - 21.0f * unit);
        face.close();
        face.moveTo(x - 9.0f * unit, y - 9.0f * unit);
        for (int i = 0; i <= 6; ++i)
        {
            const float toothX = -9.0f + static_cast<float>(i) * 3.0f;
            const float toothY = i % 2 == 0 ? -9.0f : -6.0f;
            face.lineTo(x + toothX * unit, y + toothY * unit);
        }
        face.lineTo(x + 6.0f * unit, y - 5.0f * unit);
        face.lineTo(x - 6.0f * unit, y - 5.0f * unit);
        face.close();
        const float candle = std::clamp(flicker, 0.0f, 1.0f);
        fillShape(frame, face, SolidColor{ JackOLanternColors::Candle.withOpacity(candle) });
    }
}
