export module ClaFi.Icons.DoorIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::DoorIcon
{
    using namespace ::ClaFi::Graphics;

    // The frame, as shares of the icon's size. It stands left of centre and the arrow takes the
    // room that leaves.
    constexpr float k_frameLeftShare{ 0.16f };
    constexpr float k_frameTopShare{ 0.16f };
    constexpr float k_frameRightShare{ 0.59f };
    constexpr float k_frameBottomShare{ 0.84f };
    constexpr float k_frameRadiusShare{ 0.10f };
    // The arrow starts inside the frame and leaves through the side the frame does not draw. That
    // crossing is what makes the missing side an opening rather than an unfinished rectangle.
    constexpr float k_arrowStartShare{ 0.44f };
    constexpr float k_arrowEndShare{ 0.85f };
    constexpr float k_arrowHeadShare{ 0.14f };

    void buildArrowPath(PixelPath& path, const FloatRect& box, const float size)
    {
        const float centerY = box.top + size * 0.5f;
        const float tipX = box.left + size * k_arrowEndShare;
        const float head = size * k_arrowHeadShare;
        path.moveTo({ box.left + size * k_arrowStartShare, centerY });
        path.lineTo({ tipX, centerY });
        path.moveTo({ tipX - head, centerY - head });
        path.lineTo({ tipX, centerY });
        path.lineTo({ tipX - head, centerY + head });
    }

    export void paint(PaintIconEvent& event)
    {
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const FloatRect& iconRect = event.iconRect();
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const FloatRect box = iconRect.centerRect(size);
        const float radius = size * k_frameRadiusShare;
        Canvas& canvas = event.canvas();

        RoundedRectangleParts frame{};
        frame.bounds = {
            box.left + size * k_frameLeftShare,
            box.top + size * k_frameTopShare,
            box.left + size * k_frameRightShare,
            box.top + size * k_frameBottomShare,
        };
        frame.radii = { radius, 0.0f, 0.0f, radius };
        // The doorway: the side the arrow goes through is the one side not drawn.
        frame.sides = { true, false, true, true };
        canvas.drawPartialRoundedRectangle(frame, event.textRgb(InkGrade::Strong), strokeWidth);

        PixelPath path;
        buildArrowPath(path, box, size);
        canvas.drawPath(path, { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth) });
    }
}
