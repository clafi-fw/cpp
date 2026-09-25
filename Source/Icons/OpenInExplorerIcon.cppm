export module ClaFi.Icons.OpenInExplorerIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Icons::OpenInExplorerIcon
{
    using namespace ::ClaFi::Graphics;

    // A folder with an arrow leaving it. The folder is drawn in the framework's yellow and the
    // arrow in the theme's accent, so what is being opened and the opening of it are told apart by
    // colour as well as by shape.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    // How far back each of the folder's corners is cut, as a fraction of the icon.
    constexpr float k_cornerRadius = 0.09f;

    // The folder is what is being opened, so it carries the framework's yellow. The arrow is the
    // opening of it, and takes the accent, which is the colour every other mark that means the
    // user is acting on something is drawn in.
    constexpr Ink k_folderInk = InkWell::Yellow;

    void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);

        // A stroke lies inside its bounds, so the shape is built half a stroke in and that much
        // smaller rather than pre-inset at each point.
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const Matrix3x2 transform = Matrix3x2::translation(
            iconRect.topLeft() + FloatPoint{ strokeWidth * 0.5f });

        Canvas& canvas = event.canvas();
        PixelPath path;

        // The folder: a tab across the top left, a slant down to the body, and the body. Its right
        // edge stops short of the box because the arrow crosses that corner, and a folder taking
        // the full width would leave the arrow nowhere to leave from.
        //
        // The tab stands a sixth of the icon proud of the body and the slant is long enough to be
        // read as one: a shallower step reads as a nick in the top edge rather than as a tab, and
        // it is the first thing to close up as the icon shrinks.
        const std::array<FloatPoint, 6ull> folder{
            FloatPoint{ 0.0f, size * 0.08f },
            FloatPoint{ size * 0.36f, size * 0.08f },
            FloatPoint{ size * 0.48f, size * 0.24f },
            FloatPoint{ size * 0.90f, size * 0.24f },
            FloatPoint{ size * 0.90f, size * 0.95f },
            FloatPoint{ 0.0f, size * 0.95f }
        };
        path.addRoundedPolygon(folder, size * k_cornerRadius);

        canvas.drawPath(
            path,
            { PathDrawLayer::stroke(event.inkColor(k_folderInk), strokeWidth) },
            &transform
        );

        // The arrow: a diagonal out of the folder and a head at its far end, both reaching the
        // top right corner of the box.
        path.clear();
        path.moveTo(size * 0.55f, size * 0.50f);
        path.lineTo(size * 1.00f, size * 0.05f);
        path.moveTo(size * 0.75f, size * 0.05f);
        path.lineTo(size * 1.00f, size * 0.05f);
        path.lineTo(size * 1.00f, size * 0.30f);

        // It crosses the folder's corner, so it is stroked once wide in the surface colour and
        // then again properly: without that band the two outlines meet and neither is legible.
        // Magnifier's Gap is the same trick carried all the way round a shape.
        canvas.drawPath(
            path,
            { PathDrawLayer::stroke(event.surfaceRgb(), strokeWidth * 2.0f) },
            &transform
        );
        canvas.drawPath(
            path,
            { PathDrawLayer::stroke(event.accentRgb(InkGrade::Strongest), strokeWidth)},
            &transform
        );
    }

}
