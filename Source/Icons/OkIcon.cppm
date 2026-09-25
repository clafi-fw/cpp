export module ClaFi.Icons.OkIcon;

import ClaFi.Icons.MessageBadge;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Icons::OkIcon
{
    using namespace ::ClaFi::Graphics;

    // A check on a disc, in the theme's greenish hue. Its own mark rather than CheckMark's: that
    // one is an indicator and draws itself in through the state factors a control hands it, and
    // an answer that something went well is not a state anything animates into.
    export void paint(PaintIconEvent&);


    //-------------------------------------------------------------------------


    void paint(PaintIconEvent& event)
    {
        const MessageBadge::GlyphSlot glyph = MessageBadge::paintDisc(event, InkWell::Green);

        // The short arm falls further than the long arm rises, which is what makes a check read as
        // a check rather than as a tilted v.
        PixelPath path;
        path.moveTo(glyph.at(0.300f, 0.515f));
        path.lineTo(glyph.at(0.435f, 0.655f));
        path.lineTo(glyph.at(0.710f, 0.350f));
        event.canvas().drawPath(path, { PathDrawLayer::stroke(glyph.color, glyph.strokeWidth) });
    }

}
