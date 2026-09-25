export module ClaFi.Icons.InformationIcon;

import ClaFi.Icons.MessageBadge;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Icons::InformationIcon
{
    using namespace ::ClaFi::Graphics;

    // A lower case i on a disc, in the theme's blueish hue - the hue QuestionIcon also carries,
    // the two being a notice and a question rather than two degrees of the same thing. The glyph
    // is the whole of what tells them apart, so neither may be reduced to its colour.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    void paint(PaintIconEvent& event)
    {
        const MessageBadge::GlyphSlot glyph = MessageBadge::paintDisc(event, InkWell::Blue);

        // The tittle first, then the stem: the gap between them is what says i rather than
        // exclamation, and it is the first thing to close up as the badge shrinks, so it is the
        // measurement that decides both.
        event.canvas().fillCircle(glyph.at(0.5f, 0.275f), glyph.dotRadius(), glyph.color);

        PixelPath path;
        path.moveTo(glyph.at(0.5f, 0.420f));
        path.lineTo(glyph.at(0.5f, 0.745f));
        event.canvas().drawPath(path, { PathDrawLayer::stroke(glyph.color, glyph.strokeWidth) });
    }

}
