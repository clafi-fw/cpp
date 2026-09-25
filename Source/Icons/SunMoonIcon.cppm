export module ClaFi.Icons.SunMoonIcon;

import ClaFi.Icons.MoonIcon;
import ClaFi.Icons.SunIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::SunMoonIcon
{
    using namespace ::ClaFi::Graphics;

    // The sun's rays around the moon's crescent, at the weight SunIcon and MoonIcon are drawn at.
    export void paint(PaintIconEvent& event)
    {
        Canvas& canvas = event.canvas();
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Color color = event.textRgb(InkGrade::Strongest);
        const FloatPoint center = { size * 0.5f, size * 0.5f };
        const Matrix3x2 transform =
            Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        MoonIcon::addCrescent(path, center, size * SunIcon::k_discRatio);
        SunIcon::addSunRays(path, center, size);

        canvas.drawPath(path, { PathDrawLayer::stroke(color, strokeWidth) }, &transform);
    }
}
