module ClaFi.PathArt.BatSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace BatColors
    {
        constexpr Color Silhouette{ 0xFF06070D };
    }

    void drawBat(const SceneFrame& frame, const BatPose& pose)
    {
        const float flap = pose.flap;
        const SceneFrame bat = frame.within(Matrix3x2::Translation(pose.center)
            * Matrix3x2::Scale(pose.size));
        const SolidColor black{ BatColors::Silhouette };
        for (const float side : { -1.0f, 1.0f })
        {
            PixelPath wing;
            wing.moveTo(side * 2.0f, -1.0f);
            wing.lineTo(side * 13.0f, -4.0f - 9.0f * flap);
            wing.lineTo(side * 22.0f, -2.0f - 7.0f * flap);
            wing.quadTo({ side * 18.0f, 3.0f }, { side * 14.0f, 2.0f });
            wing.quadTo({ side * 11.0f, 6.0f }, { side * 8.0f, 3.0f });
            wing.quadTo({ side * 5.0f, 6.0f }, { side * 2.0f, 3.0f });
            wing.close();
            fillShape(bat, wing, black);
        }
        fillOval(bat, { 0.0f, 1.0f }, 3.6f, 5.2f, black);
        PixelPath ears;
        ears.moveTo(-3.0f, -3.0f);
        ears.lineTo(-2.2f, -7.0f);
        ears.lineTo(-0.8f, -3.6f);
        ears.close();
        ears.moveTo(3.0f, -3.0f);
        ears.lineTo(2.2f, -7.0f);
        ears.lineTo(0.8f, -3.6f);
        ears.close();
        fillShape(bat, ears, black);
    }
}
