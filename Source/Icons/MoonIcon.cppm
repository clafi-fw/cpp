export module ClaFi.Icons.MoonIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons::MoonIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // The crescent is one circle minus another of the same radius. Equal radii keep the horns
        // symmetric and collapse the intersection to a single angle: the two circles cross at
        // +-acos(biteRatio / 2) from the bite direction, measured from the outer centre. Changing
        // either circle's radius independently breaks that identity and hornAngle stops being the
        // crossing angle.
        constexpr float k_radiusRatio = 0.36f;  // Outer radius, as a fraction of the icon size
        constexpr float k_biteRatio = 0.55f;    // Offset of the cut-out centre, in outer radii
        constexpr float k_biteAngle = -0.72f;   // Direction of the cut-out in radians - up and right

        // A circular arc as quadratic segments of at most 45 degrees. Each control point sits where
        // the two end tangents meet, so every segment boundary lands on the circle exactly.
        void addArc(PixelPath& path, FloatPoint center, float radius, float startAngle, float sweep)
        {
            constexpr float k_maxSegmentSweep = k_2Pi / 8.0f;

            int segmentCount = std::max(1, static_cast<int>(std::ceil(std::abs(sweep) / k_maxSegmentSweep)));
            float step = sweep / segmentCount;
            float ctrlRadius = radius / std::cos(step * 0.5f);

            for (int i = 0; i < segmentCount; ++i)
            {
                float midAngle = startAngle + step * (i + 0.5f);
                float endAngle = startAngle + step * (i + 1);
                path.quadTo(
                    { center.x + std::cos(midAngle) * ctrlRadius, center.y + std::sin(midAngle) * ctrlRadius },
                    { center.x + std::cos(endAngle) * radius, center.y + std::sin(endAngle) * radius }
                );
            }
        }

        // The shift that centres the crescent's box, which sits low and left of its circle.
        [[nodiscard]] FloatPoint boxCentringOffset(float radius)
        {
            const float hornAngle = std::acos(k_biteRatio * 0.5f);
            const float firstHorn = k_biteAngle + hornAngle;
            const float secondHorn = k_biteAngle - hornAngle;
            // The outer arc reaches the left and the bottom, the horns the right and the top.
            const float right = std::max(std::cos(firstHorn), std::cos(secondHorn)) * radius;
            const float top = std::min(std::sin(firstHorn), std::sin(secondHorn)) * radius;
            return { (radius - right) * 0.5f, -(radius + top) * 0.5f };
        }
    }

    // The crescent's outline as a closed subpath, for every icon that draws the moon.
    export void addCrescent(PixelPath& path, FloatPoint center, float radius)
    {
        float bite = radius * k_biteRatio;
        FloatPoint biteCenter = {
            center.x + std::cos(k_biteAngle) * bite,
            center.y + std::sin(k_biteAngle) * bite,
        };
        // Half the angle between the horns, as seen from the outer centre.
        float hornAngle = std::acos(k_biteRatio * 0.5f);
        float halfTurn = k_2Pi * 0.5f;

        // Outer edge: the long way round, away from the cut-out.
        float outerStart = k_biteAngle + hornAngle;
        path.moveTo(center.x + std::cos(outerStart) * radius, center.y + std::sin(outerStart) * radius);
        addArc(path, center, radius, outerStart, k_2Pi - hornAngle * 2.0f);

        // Inner edge: back to the first horn along the cut-out circle. Seen from its own centre
        // the horns sit at the mirrored angles, half a turn away from the outer ones.
        addArc(path, biteCenter, radius, k_biteAngle + halfTurn + hornAngle, -hornAngle * 2.0f);
        path.close();
    }

    export void paint(PaintIconEvent& event)
    {
        Canvas& canvas = event.canvas();
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        Color color = event.textRgb(InkGrade::Strongest);
        FloatPoint center = { size * 0.5f, size * 0.5f };
        Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        const float radius = size * k_radiusRatio;
        addCrescent(path, center + boxCentringOffset(radius), radius);

        canvas.drawPath(path, { PathDrawLayer::stroke(color, strokeWidth) }, &transform);
    }

}
