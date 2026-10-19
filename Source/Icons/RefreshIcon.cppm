export module ClaFi.Icons.RefreshIcon;

import ClaFi.Icons.RestartIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::RefreshIcon
{
    using namespace ::ClaFi::Graphics;

    // The restart ring grown to the span of the browser's arrows, which it stands beside.
    constexpr RestartIcon::RingShape k_shape{
        .radiusShare = 0.82f,
        .headLength = 0.6f,
        .headHalfWidth = 0.42f
    };

    // The ring and its head in the one ink the browser's arrows are drawn in.
    export void paint(PaintIconEvent& event)
    {
        const FloatRect iconRect = event.iconRect();
        const Color strokeColor = event.textRgb(InkGrade::Strongest);

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Matrix3x2 transform = Matrix3x2::Translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        PixelPath path;

        RestartIcon::buildRingPath(path, size, k_shape);
        RestartIcon::buildHeadPath(path, size, k_shape);
        event.canvas().drawPath(path, { PathDrawLayer::Stroke(strokeColor, strokeWidth) },
            &transform);
    }
}
