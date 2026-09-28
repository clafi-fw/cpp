export module ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::BrowseUpIcon
{
    using namespace ::ClaFi::Graphics;

    // Which way the browser's arrow points. Back and Forward are the Up arrow turned.
    export enum class ArrowHeading
    {
        Up,
        Left,
        Right
    };

    // The turn from pointing up to the heading, about the centre of a square of `size`.
    [[nodiscard]] Matrix3x2 turnTo(const ArrowHeading heading, const float size)
    {
        switch (heading)
        {
            case ArrowHeading::Up:
                return Matrix3x2::identity();
            case ArrowHeading::Left:
                return { 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, size };
            case ArrowHeading::Right:
                return { 0.0f, 1.0f, -1.0f, 0.0f, size, 0.0f };
        }
        return Matrix3x2::identity();
    }

    // The arrow centred in the icon's square, pointing the way the heading says.
    export void paintHeading(PaintIconEvent& event, const ArrowHeading heading)
    {
        const FloatRect iconRect = event.iconRect();
        const Color strokeColor = event.textRgb(InkGrade::Strongest);

        float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);//std::max(1.0f, size * 0.08f);
        size -= strokeWidth;

        const Matrix3x2 placement = Matrix3x2::translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });
        // The turn is the right operand: a point meets it first, and the placement second.
        const Matrix3x2 transform = placement * turnTo(heading, size);
        PixelPath path;

        path.moveTo(size * 0.50f, size * 0.925f);   // stem bottom
        path.lineTo(size * 0.50f, size * 0.075f);   // tip
        path.moveTo(size * 0.25f, size * 0.325f);   // left barb
        path.lineTo(size * 0.50f, size * 0.075f);   // tip
        path.lineTo(size * 0.75f, size * 0.325f);   // right barb

        event.canvas().drawPath(path, { PathDrawLayer::stroke(strokeColor, strokeWidth) }, &transform);
    }

    export void paint(PaintIconEvent& event)
    {
        paintHeading(event, ArrowHeading::Up);
    }

}
