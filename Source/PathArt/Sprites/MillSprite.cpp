module ClaFi.PathArt.MillSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawMillHouse(const SceneFrame& frame, const MillPose& pose, const MillColors& colors)
    {
        for (const float turns : { 0.0f, 0.5f })
        {
            const Matrix3x2 tx = Matrix3x2::Translation(pose.base)
                * Matrix3x2::Rotation(turns * k_2Pi) * Matrix3x2::Scale(pose.size);

            PixelPath path;
            path.moveTo(-60.0f, 0.0f);
            path.lineTo(-60.0f, -100.0f);
            path.lineTo(60.0f, -100.0f);
            path.lineTo(60.0f, 0.0f);
            path.close();
            fillShape(frame, path, LinearGradient::Simple({ 40.0f, -100.0f }, { -40.0f, -80.0f },
                colors.bodyTop, colors.bodyBottom), tx);

            path.clear();
            path.moveTo(-75.0f, -100.0f);
            path.lineTo(0.0f, -160.0f);
            path.lineTo(75.0f, -100.0f);
            path.close();
            fillShape(frame, path, LinearGradient::Simple({ 40.0f, -100.0f }, { -40.0f, -80.0f },
                colors.roofTop, colors.roofBottom), tx);
        }
    }

    void drawMillWings(const SceneFrame& frame, const MillPose& pose, const MillColors& colors)
    {
        const float s = pose.size;
        const FloatPoint hub = { pose.base.x, pose.base.y - 130.0f * s };
        const Matrix3x2 baseTx = Matrix3x2::Translation(hub) * Matrix3x2::Scale(s);

        PixelPath path;
        path.moveTo(-10.0f, -10.0f);
        path.lineTo(10.0f, -10.0f);
        path.lineTo(10.0f, 10.0f);
        path.lineTo(-10.0f, 10.0f);
        path.close();
        fillShape(frame, path, SolidColor{ colors.wingWood }, baseTx);

        const SolidColor wood{ colors.wingWood };
        const SolidColor woodStroke{ colors.wingWoodStroke };
        const SolidColor fabric{ colors.wingFabric.withOpacity(0.75f) };
        const SolidColor fabricStroke{ colors.wingFabricStroke.withOpacity(0.75f) };
        for (int wing = 0; wing != 4; ++wing)
        {
            const float i = static_cast<float>(wing);
            const Matrix3x2 wingTx = Matrix3x2::Translation(hub)
                * Matrix3x2::Rotation((pose.wingTurns + (i * 0.25f)) * k_2Pi) * Matrix3x2::Scale(s);

            path.clear();
            path.moveTo(-4.0f, 0.0f);
            path.lineTo(4.0f, 0.0f);
            path.lineTo(4.0f, -120.0f);
            path.lineTo(-4.0f, -120.0f);
            path.close();
            fillShape(frame, path, wood, wingTx);
            strokeShape(frame, path, woodStroke, 1.5f, StrokeCap::Round, wingTx);

            path.clear();
            path.moveTo(4.0f, -40.0f);
            path.lineTo(30.0f, -45.0f);
            path.lineTo(30.0f, -115.0f);
            path.lineTo(4.0f, -110.0f);
            path.close();
            fillShape(frame, path, fabric, wingTx);
            strokeShape(frame, path, fabricStroke, 1.5f, StrokeCap::Round, wingTx);
        }
    }
}
