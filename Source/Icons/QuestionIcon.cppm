export module ClaFi.Icons.QuestionIcon;

import ClaFi.Icons.MessageBadge;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::QuestionIcon
{
    using namespace ::ClaFi::Graphics;

    // A question mark on a disc, in the theme's blueish hue - the hue InformationIcon also
    // carries, the two being a question and a notice rather than two degrees of the same thing.
    // The glyph is the whole of what tells them apart, so neither may be reduced to its colour.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    void paint(PaintIconEvent& event)
    {
        const MessageBadge::GlyphSlot glyph = MessageBadge::paintDisc(event, InkWell::Blue);

        // The hook is three quarter turns and a stem: over the top, down the far side, and back
        // in to the middle, where the stem drops. Drawn rather than set as text so that it holds
        // its weight at sixteen pixels, where a font's question mark closes up.
        PixelPath path;
        path.moveTo(glyph.at(0.345f, 0.375f));
        path.quadTo(glyph.at(0.345f, 0.245f), glyph.at(0.500f, 0.245f));
        path.quadTo(glyph.at(0.655f, 0.245f), glyph.at(0.655f, 0.375f));
        path.quadTo(glyph.at(0.655f, 0.475f), glyph.at(0.500f, 0.525f));
        path.lineTo(glyph.at(0.500f, 0.615f));
        event.canvas().drawPath(path, { PathDrawLayer::stroke(glyph.color, glyph.strokeWidth) });

        event.canvas().fillCircle(glyph.at(0.5f, 0.755f), glyph.dotRadius(), glyph.color);
    }

}
