module ClaFi.PathArt.ReedsSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // The reflection is the clump turned half a turn about each foot, at 15 percent.
    void drawReeds(const SceneFrame& frame, const ReedsPose& pose, const ReedColors& colors)
    {
        Hsl headTop{ colors.head };
        Hsl headBottom{ colors.head };
        headTop.scaleLuminosity(1.15f);
        headBottom.scaleLuminosity(0.85f);
        const int drawings = pose.inWater ? 2 : 1;
        for (int r = 0; r != drawings; ++r)
        {
            const float reflection = static_cast<float>(r);
            for (int i = 0; i < 3; ++i)
            {
                const float index = static_cast<float>(i);
                const float x = pose.base.x + (index * 12.0f * pose.size);
                const float sway = std::sin((pose.cycleTime + (reflection * 0.5f)) * k_2Pi + index)
                    * 0.08f;
                const Matrix3x2 place = Matrix3x2::Translation(x, pose.base.y)
                    * Matrix3x2::Rotation((sway + (reflection * 0.5f)) * k_2Pi + pose.lean)
                    * Matrix3x2::Scale(pose.size);
                const float opacity = 1.0f - (reflection * 0.85f);

                PixelPath stem;
                stem.moveTo(0.0f, 0.0f);
                stem.lineTo(0.0f, -40.0f);
                strokeShape(frame, stem, SolidColor{ colors.stem.withOpacity(opacity) }, 2.0f,
                    StrokeCap::Round, place);

                PixelPath head;
                head.moveTo(-2.5f, -25.0f);
                head.lineTo(-2.5f, -42.0f);
                head.quadTo({ -2.5f, -44.5f }, { 0.0f, -44.5f });
                head.quadTo({ 2.5f, -44.5f }, { 2.5f, -42.0f });
                head.lineTo(2.5f, -25.0f);
                head.quadTo({ 2.5f, -22.5f }, { 0.0f, -22.5f });
                head.quadTo({ -2.5f, -22.5f }, { -2.5f, -25.0f });
                head.close();
                fillShape(frame, head, LinearGradient::Simple({ 0.0f, -44.5f }, { 0.0f, -22.5f },
                    headTop.toColor().withOpacity(opacity),
                    headBottom.toColor().withOpacity(opacity)),
                    place * Matrix3x2::Scale(0.35f + 0.65f * pose.fur, 1.0f));
            }
        }
    }
}
