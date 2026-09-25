export module ClaFi.Icons.PasteIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::PasteIcon
{
    using namespace ::ClaFi::Graphics;

    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        size -= strokeWidth;

        const Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        Canvas& canvas = event.canvas();
        PixelPath path;

        // A strong grey holds the object, the accent marks what names the command. Here that is
        // the content on the board: the board is where it lands, the lines are what arrived.
        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color accentColor = event.accentRgb(InkGrade::Strongest);

        const float radius = size * 0.12f;
        float boardLeft = size * 0.06f;
        float boardRight = size * 0.94f;
        float boardTop = size * 0.16f;

        // The board.
        path.moveTo(boardLeft + radius, boardTop);
        path.lineTo(boardRight - radius, boardTop);
        path.quadTo({ boardRight, boardTop }, { boardRight, boardTop + radius });
        path.lineTo(boardRight, size - radius);
        path.quadTo({ boardRight, size }, { boardRight - radius, size });
        path.lineTo(boardLeft + radius, size);
        path.quadTo({ boardLeft, size }, { boardLeft, size - radius });
        path.lineTo(boardLeft, boardTop + radius);
        path.quadTo({ boardLeft, boardTop }, { boardLeft + radius, boardTop });
        path.close();

        float clipLeft = size * 0.30f;
        float clipRight = size * 0.70f;
        float clipBottom = size * 0.28f;
        float clipRadius = size * 0.07f;

        // The clasp. It reaches the top edge and closes below the board's own top edge, which is
        // what makes it read as gripping the board rather than sitting beside it.
        path.moveTo(clipLeft, clipBottom);
        path.lineTo(clipLeft, clipRadius);
        path.quadTo({ clipLeft, 0.0f }, { clipLeft + clipRadius, 0.0f });
        path.lineTo(clipRight - clipRadius, 0.0f);
        path.quadTo({ clipRight, 0.0f }, { clipRight, clipRadius });
        path.lineTo(clipRight, clipBottom);
        path.close();
        canvas.drawPath(path, { PathDrawLayer::stroke(bodyColor, strokeWidth) }, &transform);

        // Two lines of content on the board.
        path.clear();
        path.moveTo(size * 0.26f, size * 0.57f);
        path.lineTo(size * 0.74f, size * 0.57f);
        path.moveTo(size * 0.26f, size * 0.78f);
        path.lineTo(size * 0.74f, size * 0.78f);
        canvas.drawPath(path, { PathDrawLayer::stroke(accentColor, strokeWidth) }, &transform);
    }

}
