export module ClaFi.Icons.OpenInExplorerIcon;

import ClaFi.Icons.FolderIcon;
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

        // The folder's right edge stops short of the box because the arrow crosses that corner,
        // and a folder taking the full width would leave the arrow nowhere to leave from.
        FolderIcon::addOutline(path, { 0.0f, size * 0.08f, size * 0.90f, size * 0.95f });

        canvas.drawPath(
            path,
            { PathDrawLayer::stroke(event.inkRgb(k_folderInk), strokeWidth) },
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
        // Magnifier's Halo is the same trick carried all the way round a shape.
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
