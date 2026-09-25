export module ClaFi.Icons.SelectAllIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::SelectAllIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // Where along an edge the ink is, as fractions of that edge. The runs reach both ends, so
        // the four corners of the marquee are solid and the gaps fall inside the sides.
        struct DashSpan
        {
            float from;
            float to;
        };

        constexpr DashSpan dashSpans[] = {
            { 0.0f, 0.30f },
            { 0.39f, 0.61f },
            { 0.70f, 1.0f },
        };

        void addDashedEdge(PixelPath& path, FloatPoint from, FloatPoint to)
        {
            FloatPoint delta = { to.x - from.x, to.y - from.y };
            for (const DashSpan& span : dashSpans)
            {
                path.moveTo({ from.x + delta.x * span.from, from.y + delta.y * span.from });
                path.lineTo({ from.x + delta.x * span.to, from.y + delta.y * span.to });
            }
        }
    }

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
        // the content: the marquee reaches round every line of it, which is the whole of what
        // this command says.
        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color accentColor = event.accentRgb(InkGrade::Strongest);

        float low = size * 0.05f;
        float high = size * 0.95f;

        // A selection marquee drawn all the way round.
        addDashedEdge(path, { low, low }, { high, low });
        addDashedEdge(path, { high, low }, { high, high });
        addDashedEdge(path, { high, high }, { low, high });
        addDashedEdge(path, { low, high }, { low, low });
        canvas.drawPath(path, { PathDrawLayer::stroke(bodyColor, strokeWidth) }, &transform);

        // The content inside it. The short last line reads as the end of the text, so the
        // marquee reads as having reached past it.
        path.clear();
        path.moveTo(size * 0.24f, size * 0.32f);
        path.lineTo(size * 0.76f, size * 0.32f);
        path.moveTo(size * 0.24f, size * 0.50f);
        path.lineTo(size * 0.76f, size * 0.50f);
        path.moveTo(size * 0.24f, size * 0.68f);
        path.lineTo(size * 0.58f, size * 0.68f);
        canvas.drawPath(path, { PathDrawLayer::stroke(accentColor, strokeWidth) }, &transform);
    }

}
