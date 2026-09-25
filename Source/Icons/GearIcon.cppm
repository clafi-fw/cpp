export module ClaFi.Icons.GearIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::GearIcon
{
    using namespace ::ClaFi::Graphics;

    // Six teeth. At sixteen design units a tooth and the valley beside it come to about a pixel
    // each, and a wheel of eight reads as a rosette.
    constexpr int k_teeth{ 6 };
    // The body and the tips, as shares of the icon's half size.
    constexpr float k_rootShare{ 0.52f };
    constexpr float k_tipShare{ 0.78f };
    // What a tooth takes of its own turn: the flank runs for this much of the step, the tip holds
    // until the step's half way, and the valley is the rest.
    constexpr float k_flankShare{ 0.15f };
    constexpr float k_hubShare{ 0.26f };
    // A tooth points at twelve o'clock, which leaves the wheel symmetric about the vertical.
    // Angles as the screen has them: degrees from three o'clock, increasing clockwise.
    constexpr float k_toothDegrees{ 270.0f };

    [[nodiscard]] float radiansOf(const float degrees)
    {
        return degrees * std::numbers::pi_v<float> / 180.0f;
    }

    [[nodiscard]] FloatPoint pointAt(const FloatPoint center, const float radius, const float radians)
    {
        return center + FloatPoint{ std::cos(radians), std::sin(radians) } * radius;
    }

    // Four corners to a tooth: up the leading flank, across the tip, down the trailing flank, and
    // along the valley to where the next one starts.
    void buildWheelPath(PixelPath& path, const FloatPoint center, const float rootRadius, const float tipRadius)
    {
        const float step = radiansOf(360.0f / k_teeth);
        const float start = radiansOf(k_toothDegrees) - step * 0.25f;
        path.moveTo(pointAt(center, rootRadius, start));
        for (int i = 0; i < k_teeth; ++i)
        {
            const float base = start + step * i;
            path.lineTo(pointAt(center, tipRadius, base + step * k_flankShare));
            path.lineTo(pointAt(center, tipRadius, base + step * (0.5f - k_flankShare)));
            path.lineTo(pointAt(center, rootRadius, base + step * 0.5f));
            path.lineTo(pointAt(center, rootRadius, base + step));
        }
        path.close();
    }

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        // The stroke is centred on the path, so half of it stands outside the tips.
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const float halfSize = size * 0.5f;
        const FloatPoint center = event.iconCenter();
        Canvas& canvas = event.canvas();
        PixelPath path;

        buildWheelPath(path, center, halfSize * k_rootShare, halfSize * k_tipShare);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.textRgb(InkGrade::Strong), strokeWidth) });
        canvas.drawCircle(center, halfSize * k_hubShare, event.textRgb(InkGrade::Muted), strokeWidth);
    }
}
