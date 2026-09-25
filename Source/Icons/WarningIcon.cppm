export module ClaFi.Icons.WarningIcon;

import ClaFi.Icons.MessageBadge;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::WarningIcon
{
    using namespace ::ClaFi::Graphics;

    // An exclamation mark on a triangle, in the theme's yellowish hue.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    void paint(PaintIconEvent& event)
    {
        const MessageBadge::GlyphSlot glyph = MessageBadge::paintTriangle(event, InkWell::Yellow);

        // The bar sits below the middle of the badge rather than on it: a triangle's room is at
        // the bottom, and a mark centred in the box would stand with its point in the apex.
        PixelPath path;
        path.moveTo(glyph.at(0.5f, 0.400f));
        path.lineTo(glyph.at(0.5f, 0.635f));
        event.canvas().drawPath(path, { PathDrawLayer::stroke(glyph.color, glyph.strokeWidth) });

        event.canvas().fillCircle(glyph.at(0.5f, 0.755f), glyph.dotRadius(), glyph.color);
    }

}
