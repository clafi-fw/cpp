export module ClaFi.Icons.ErrorIcon;

import ClaFi.Icons.MessageBadge;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Icons::ErrorIcon
{
    using namespace ::ClaFi::Graphics;

    // A cross on a disc, in the theme's reddish hue.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    void paint(PaintIconEvent& event)
    {
        const MessageBadge::GlyphSlot glyph = MessageBadge::paintDisc(event, InkWell::Red);

        // The cross is kept well inside the disc: its arms run diagonally, which is where a disc
        // has least room, and an arm reaching as far as an upright would touch the edge.
        PixelPath path;
        path.moveTo(glyph.at(0.345f, 0.345f));
        path.lineTo(glyph.at(0.655f, 0.655f));
        path.moveTo(glyph.at(0.655f, 0.345f));
        path.lineTo(glyph.at(0.345f, 0.655f));
        event.canvas().drawPath(path, { PathDrawLayer::stroke(glyph.color, glyph.strokeWidth) });
    }

}
