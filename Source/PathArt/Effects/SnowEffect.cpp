module ClaFi.PathArt.SnowEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void createSnowFlakePath(PixelPath& p)
    {
        p.clear();
        for (int i = 0; i < 6; ++i)
        {
            const float angle = static_cast<float>(i) * k_2Pi / 6.0f;
            const float sa = std::sin(angle);
            const float ca = std::cos(angle);
            p.moveTo(0.0f, 0.0f);
            p.lineTo(12.0f * sa, 12.0f * ca);
            float sideAngle = angle + 0.8f;
            p.moveTo(6.0f * sa, 6.0f * ca);
            p.lineTo(
                6.0f * sa + 5.0f * std::sin(sideAngle),
                6.0f * ca + 5.0f * std::cos(sideAngle)
            );
            sideAngle = angle - 0.8f;
            p.moveTo(6.0f * sa, 6.0f * ca);
            p.lineTo(
                6.0f * sa + 5.0f * std::sin(sideAngle),
                6.0f * ca + 5.0f * std::cos(sideAngle)
            );
        }
    }

    // Each flake falls the frame's height once or twice a cycle, swaying as it goes.
    void drawSnow(const SceneFrame& frame, const float cycleTime, const Snow& snow)
    {
        if (snow.opacity <= 0.001f)
            return;

        const float t = cycleTime;
        const float w = frame.width;
        const float h = frame.height;
        const float unit = frame.unit;
        PixelPath flake;
        createSnowFlakePath(flake);
        const SolidColor color{ snow.color.withOpacity(snow.opacity) };
        for (int i = 0; i < snow.count; ++i)
        {
            const float index = static_cast<float>(i);
            const float xBase = pseudoRandom(i) * w;
            const float drift = std::sin(t * k_2Pi * 1.0f + index) * 30.0f * unit;
            const float loopCount = static_cast<float>(1 + (i % 2));
            const float y = std::fmod(pseudoRandom(i, 3.0f) * h + t * (loopCount * h), h);
            const float s = (0.3f + pseudoRandom(i, 5.0f) * 0.3f) * unit;
            const Matrix3x2 transform = Matrix3x2::Translation({ xBase + drift, y })
                * Matrix3x2::Scale(s)
                * Matrix3x2::Rotation(calculateSyncRotation(t, i, 6.0f) * k_2Pi);
            strokeShape(frame, flake, color, 1.5f, StrokeCap::Round, transform);
        }
    }
}
