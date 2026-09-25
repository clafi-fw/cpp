export module ClaFi.Icons.SunIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons::SunIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        constexpr int k_rayCount = 8;
        // Fractions of the icon size. The outermost ray end sits at half the size from the centre,
        // so the mark fills the rect the caller reserved for it and no more.
        constexpr float k_rayStartRatio = 0.37f;
        constexpr float k_rayEndRatio = 0.50f;
    }

    // The disc's radius as a fraction of the icon size, and where a mark standing in for it goes.
    export constexpr float k_discRatio = 0.26f;

    // Appends the rays as open subpaths, so one stroke pass covers both them and the disc the
    // caller has already put into the path.
    export void addSunRays(PixelPath& path, FloatPoint center, float size)
    {
        float rayStart = size * k_rayStartRatio;
        float rayEnd = size * k_rayEndRatio;

        for (int i = 0; i < k_rayCount; ++i)
        {
            float angle = i * k_2Pi / k_rayCount;
            float sa = std::sin(angle);
            float ca = std::cos(angle);
            path.moveTo(center.x + ca * rayStart, center.y + sa * rayStart);
            path.lineTo(center.x + ca * rayEnd, center.y + sa * rayEnd);
        }
    }

    export void paint(PaintIconEvent& event)
    {
        Canvas& canvas = event.canvas();
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Color color = event.textRgb(InkGrade::Strongest);
        const FloatPoint center = { size * 0.5f, size * 0.5f };
        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        // drawCircle clears the path, so the disc has to go in before the rays are appended.
        path.drawCircle(center, size * k_discRatio);
        addSunRays(path, center, size);

        canvas.drawPath(path, { PathDrawLayer::stroke(color, strokeWidth) }, &transform);
    }

}
