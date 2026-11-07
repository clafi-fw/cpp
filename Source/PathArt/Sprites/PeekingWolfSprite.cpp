module ClaFi.PathArt.PeekingWolfSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawPeekingWolf(const SceneFrame& frame, const PeekingWolfPose& pose,
        const PeekingWolfColors& colors)
    {
        const float opacity = pose.opacity;
        if (opacity <= 0.0f)
            return;

        const float x = pose.at.x;
        const float y = pose.at.y;
        const float s = pose.size;
        const float t = pose.cycleTime;
        const SolidColor body{ colors.body.withOpacity(opacity) };
        const float breath = (std::sin(t * k_2Pi * 2.0f) * 3.0f) - 5.0f * s;

        // Hind Leg
        {
            const Matrix3x2 legTx = Matrix3x2::Translation(x + s * 25.0f, y - s * 26.0f)
                * Matrix3x2::Scale(s * 0.66f);
            PixelPath path;
            path.moveTo(30.0f, 37.0f + breath * s);
            path.lineTo(65.0f, 52.0f + breath * s * 0.5f);
            path.lineTo(60.0f, 60.0f);
            fillShape(frame, path, body, legTx);
            strokeShape(frame, path, body, 6.0f, StrokeCap::Round, legTx);
        }

        // Ears
        const float earT = std::sin(t * 50.0f) > 0.95f ? 0.2f : 0.0f;
        const SolidColor ear{ colors.ear.withOpacity(opacity) };
        PixelPath earPath;
        earPath.moveTo(-8.0f, 0.0f);
        earPath.lineTo(8.0f, 0.0f);
        earPath.lineTo(0.0f, -20.0f);
        earPath.close();
        {
            const Matrix3x2 earTx1 = Matrix3x2::Translation(x - 8.0f * s, y + breath)
                * Matrix3x2::Rotation((-0.3f + earT) * k_2Pi) * Matrix3x2::Scale(s * 1.25f);
            fillShape(frame, earPath, ear, earTx1);
        }
        {
            const Matrix3x2 earTx2 = Matrix3x2::Translation(x + 8.0f * s, y + 5.0f * s + breath)
                * Matrix3x2::Rotation(0.25f * k_2Pi) * Matrix3x2::Scale(s * 1.25f);
            fillShape(frame, earPath, ear, earTx2);
        }

        // Snout
        {
            const Matrix3x2 headTx = Matrix3x2::Translation(x, y + breath)
                * Matrix3x2::Scale(s * 1.25f);
            PixelPath path;
            path.moveTo(-10.0f, 0.0f);
            path.lineTo(10.0f, 0.0f);
            path.lineTo(15.0f, 15.0f);
            path.lineTo(-2.0f, 30.0f);
            path.lineTo(-15.0f, 15.0f);
            path.close();
            fillShape(frame, path, body, headTx);
        }

        const float blink = (std::sin(t * 12.0f) > 0.8f) ? 0.0f : 1.0f;
        if (blink > 0.0f)
        {
            const Matrix3x2 eyeTx = Matrix3x2::Translation(x, y + breath + s * 4.0f)
                * Matrix3x2::Scale(s * 1.25f);
            const float eyeBegin = 3.0f;
            const float eyeEnd = 8.0f;
            const float xShift = -1.0f;
            const float yShift = 8.0f;
            const float eyeTop = 10.0f;
            const float eyeBottom = 12.0f;
            PixelPath path;
            path.moveTo(xShift - eyeEnd, yShift);
            path.lineTo(xShift - eyeBegin, eyeTop);
            path.lineTo(xShift - eyeEnd, eyeBottom);
            path.close();
            path.moveTo(xShift + eyeBegin, yShift);
            path.lineTo(xShift + eyeEnd, eyeTop);
            path.lineTo(xShift + eyeBegin, eyeBottom);
            path.close();
            fillShape(frame, path, SolidColor{ colors.eye.withOpacity(opacity) }, eyeTx);
        }
    }
}
