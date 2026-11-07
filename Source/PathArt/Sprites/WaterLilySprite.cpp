module ClaFi.PathArt.WaterLilySprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawWaterLily(const SceneFrame& frame, const WaterLilyPose& pose,
        const WaterLilyColors& colors)
    {
        const float bob = std::sin(pose.cycleTime * k_2Pi * 1.5f) * 3.0f * pose.size;
        const Matrix3x2 place = Matrix3x2::Translation(pose.center.x, pose.center.y + bob)
            * Matrix3x2::Scale(pose.size);

        // The pad is a disc with a notch cut to the middle.
        PixelPath pad;
        const float r = 22.0f;
        pad.moveTo(0.0f, 0.0f);
        pad.lineTo(r, -5.0f);
        pad.quadTo({ r, r }, { 0, r });
        pad.quadTo({ -r, r }, { -r, 0 });
        pad.quadTo({ -r, -r }, { 0, -r });
        pad.quadTo({ r, -r }, { r, -5 });
        pad.close();
        fillShape(frame, pad, SolidColor{ colors.pad }, place);

        for (int i = 0; i < 5; ++i)
        {
            PixelPath petal;
            petal.moveTo(0.0f, 0.0f);
            petal.lineTo(-6.0f, -12.0f);
            petal.lineTo(0.0f, -18.0f);
            petal.lineTo(6.0f, -12.0f);
            petal.close();
            const float petalTurns = static_cast<float>(i) * 0.2f;
            fillShape(frame, petal, SolidColor{ colors.flower },
                place * Matrix3x2::Rotation(petalTurns * k_2Pi));
        }
    }
}
