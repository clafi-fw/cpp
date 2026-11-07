module ClaFi.PathArt.WaterWheelSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawWaterWheel(const SceneFrame& frame, const WaterWheelPose& pose, const Color wood)
    {
        const Matrix3x2 place = Matrix3x2::Translation(pose.center);
        const Matrix3x2 scale = Matrix3x2::Scale(pose.size);
        PixelPath rim;
        rim.drawCircle({}, 40.0f);
        const Color rimColor{ wood, 0xff'000000, 0.2f };
        strokeShape(frame, rim, SolidColor{ rimColor }, 5.0f, StrokeCap::Round,
            place * Matrix3x2::Rotation(-pose.turns * k_2Pi) * scale);

        for (int i = 0; i < 8; ++i)
        {
            const float spokeTurns = -pose.turns + static_cast<float>(i) * 0.125f;
            PixelPath spoke;
            spoke.moveTo(-2.0f, 0.0f);
            spoke.lineTo(2.0f, 0.0f);
            spoke.lineTo(5.0f, 38.0f);
            spoke.lineTo(-5.0f, 38.0f);
            spoke.close();
            fillShape(frame, spoke, SolidColor{ wood },
                place * Matrix3x2::Rotation(spokeTurns * k_2Pi) * scale);
        }
    }
}
