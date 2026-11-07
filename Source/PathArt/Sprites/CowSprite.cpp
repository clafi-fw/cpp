module ClaFi.PathArt.CowSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawCow(const SceneFrame& frame, const CowPose& pose, const CowColors& colors)
    {
        const float x = pose.at.x;
        const float y = pose.at.y;
        const float s = pose.size;
        const float t = pose.cycleTime;
        drawGlow(frame, { x, y + 50.0f * s }, 40.0f * s, faded(colors.shadow, 0.3f), 0.5f);

        const SolidColor detail{ colors.detail };
        PixelPath path;
        const Matrix3x2 tailTx = Matrix3x2::Translation(x + 40.0f * s, y - 10.0f * s)
            * Matrix3x2::Rotation((std::sin(t * 30.0f) * 0.4f + 0.5f) * k_2Pi)
            * Matrix3x2::Scale(s);
        path.moveTo(0.0f, 0.0f);
        path.lineTo(0.0f, 25.0f);
        strokeShape(frame, path, detail, 3.0f, StrokeCap::Round, tailTx);

        const Matrix3x2 bodyTx = Matrix3x2::Translation(x, y) * Matrix3x2::Scale(s);
        path.clear();
        path.moveTo(-45.0f, 0.0f);
        path.quadTo({ -45.0f, -40.0f }, { 0.0f, -40.0f });
        path.quadTo({ 45.0f, -40.0f }, { 45.0f, 0.0f });
        path.lineTo(45.0f, 20.0f);
        path.lineTo(-45.0f, 20.0f);
        path.close();
        fillShape(frame, path, SolidColor{ colors.body }, bodyTx);

        path.clear();
        path.moveTo(-20.0f, -35.0f);
        path.quadTo({ -10.0f, -45.0f }, { 0.0f, -30.0f });
        path.lineTo(5.0f, -15.0f);
        path.close();
        path.moveTo(20.0f, -10.0f);
        path.drawCircle({}, 12.0f);
        fillShape(frame, path, SolidColor{ colors.spots }, bodyTx);

        path.clear();
        path.moveTo(-35.0f, 20.0f);
        path.lineTo(-35.0f, 40.0f);
        path.moveTo(-20.0f, 20.0f);
        path.lineTo(-20.0f, 40.0f);
        path.moveTo(20.0f, 20.0f);
        path.lineTo(20.0f, 40.0f);
        path.moveTo(35.0f, 20.0f);
        path.lineTo(35.0f, 40.0f);
        strokeShape(frame, path, detail, 6.0f, StrokeCap::Round, bodyTx);

        // The head dips as the cow chews, twice a turn of the chewing.
        const float m = std::abs(std::sin(t * 15.0f)) * 8.0f;
        const Matrix3x2 headTx = Matrix3x2::Translation(x - 45.0f * s, y - (15.0f - m) * s)
            * Matrix3x2::Rotation(-0.2f * k_2Pi) * Matrix3x2::Scale(s);

        path.clear();
        path.moveTo(-15.0f, -15.0f);
        path.lineTo(15.0f, -15.0f);
        path.lineTo(20.0f, 15.0f);
        path.lineTo(-20.0f, 15.0f);
        path.close();
        fillShape(frame, path, SolidColor{ colors.body }, headTx);
        strokeShape(frame, path, detail, 1.0f, StrokeCap::Round, headTx);

        path.clear();
        path.moveTo(-18.0f, 5.0f);
        path.lineTo(18.0f, 5.0f);
        path.lineTo(18.0f, 18.0f);
        path.lineTo(-18.0f, 18.0f);
        path.close();
        fillShape(frame, path, SolidColor{ colors.snout }, headTx);
        strokeShape(frame, path, detail, 1.0f, StrokeCap::Round, headTx);

        path.clear();
        path.moveTo(-15.0f, -15.0f);
        path.lineTo(-25.0f, -25.0f);
        path.lineTo(-15.0f, -5.0f);
        path.close();
        path.moveTo(15.0f, -15.0f);
        path.lineTo(25.0f, -25.0f);
        path.lineTo(15.0f, -5.0f);
        path.close();
        fillShape(frame, path, detail, headTx);
    }
}
