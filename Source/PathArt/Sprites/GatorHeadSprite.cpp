module ClaFi.PathArt.GatorHeadSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace GatorTeeth
    {
        constexpr Color Lower{ 220, 220, 220 };
        constexpr Color Upper{ 255, 255, 255 };
    }

    void drawGatorHead(const SceneFrame& frame, const GatorHeadPose& pose,
        const GatorHeadColors& colors)
    {
        const float opacity = pose.opacity;
        if (opacity <= 0.0f)
            return;

        const float x = pose.at.x;
        const float y = pose.at.y;
        const float s = pose.size;
        const float t = pose.cycleTime;
        const SolidColor body{ colors.body.withOpacity(opacity) };
        const Matrix3x2 baseTx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(s);
        PixelPath path;

        // Jaw
        path.moveTo(80.0f, 25.0f);
        path.lineTo(-60.0f, 25.0f);
        path.lineTo(-60.0f, 35.0f);
        path.lineTo(80.0f, 35.0f);
        path.close();
        fillShape(frame, path, body, baseTx);

        // Lower Teeth
        path.clear();
        for (int i = 0; i < 4; ++i)
        {
            const float tx = -10.0f - static_cast<float>(i) * 14.0f;
            path.moveTo(tx, 25.0f);
            path.lineTo(tx - 4.0f, 25.0f);
            path.lineTo(tx - 2.0f, 20.0f);
            path.close();
        }
        fillShape(frame, path, SolidColor{ GatorTeeth::Lower.withOpacity(opacity) }, baseTx);

        // Upper Jaw Snapping
        const float snapVal = (std::fmod(t * 5.0f, 1.0f) > 0.88f) ? 0.05f : 0.0f;
        const Matrix3x2 jawTx = Matrix3x2::Translation(x + 30.0f * s, y + 25.0f * s)
            * Matrix3x2::Rotation(snapVal * k_2Pi) * Matrix3x2::Scale(s);

        path.clear();
        path.moveTo(0.0f, 0.0f);
        path.lineTo(-100.0f, 0.0f);
        path.lineTo(-95.0f, -15.0f);
        path.lineTo(0.0f, -15.0f);
        path.close();
        fillShape(frame, path, body, jawTx);

        // Upper Teeth
        path.clear();
        path.moveTo(-20.0f, 0.0f);
        for (int i = 0; i < 5; ++i)
        {
            path.lineBy(-3.5f, 10.0f);
            path.lineBy(-3.5f, -10.0f);
            path.moveBy(-9.0f, 0.0f);
        }
        fillShape(frame, path, SolidColor{ GatorTeeth::Upper.withOpacity(opacity) }, jawTx);

        // Eye Bump
        path.clear();
        path.moveTo(-15.0f, -15.0f);
        path.quadTo({ -25.0f, -28.0f }, { -35.0f, -15.0f });
        path.close();
        fillShape(frame, path, body, jawTx);

        const float blink = (std::sin(t * 15.0f) > 0.9f) ? 0.0f : 1.0f;
        if (blink > 0.0f)
        {
            path.clear();
            path.moveTo(-22.0f, -16.0f);
            path.lineTo(-30.0f, -20.0f);
            path.lineTo(-28.0f, -15.0f);
            path.close();
            fillShape(frame, path, SolidColor{ colors.eye.withOpacity(opacity) }, jawTx);
        }
    }
}
