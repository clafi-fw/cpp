export module ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::BrowseUpIcon
{
    using namespace ::ClaFi::Graphics;

    export void paint(PaintIconEvent& event)
    {
        const FloatRect iconRect = event.iconRect();
        const Color strokeColor = event.textRgb(InkGrade::Strongest);

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = std::max(1.0f, size * 0.08f);
        size -= strokeWidth;

        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        // 2. Upward Arrow Stem (Centered)
        path.moveTo(size * 0.50f, size * 0.85f);    // Stem Bottom (inside folder)
        path.lineTo(size * 0.50f, 0.00f);           // Stem Top (touches top edge)

        // 3. Arrow Head
        path.moveTo(size * 0.25f, size * 0.25f);    // Left barb
        path.lineTo(size * 0.50f, 0.00f);           // Tip
        path.lineTo(size * 0.75f, size * 0.25f);    // Right barb

        event.canvas().drawPath(path, { PathDrawLayer::stroke(strokeColor, strokeWidth) }, &transform);
    }

}
