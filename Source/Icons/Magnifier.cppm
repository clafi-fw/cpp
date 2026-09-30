export module ClaFi.Icons.Magnifier;

import ClaFi.Icons.PlusMark;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Icons::Magnifier
{
    using namespace ::ClaFi::Graphics;

    // What the lens holds.
    export enum class Lens
    {
        Empty,
        Plus,
        Minus
    };

    // Whether the magnifier is drawn with its handle.
    export enum class Tail
    {
        No,
        Yes
    };

    // A band of theme surface round the outside, so the glass reads as lying on top. See Icons
    export enum class Halo
    {
        No,
        Yes
    };

    // A magnifier filling the icon rect. See Icons#magnifier
    export void paint(PaintIconEvent&, Lens = Lens::Empty, Tail = Tail::Yes, Halo = Halo::No);
    // The same glass in a rect of the caller's choosing, so it can sit on top of something else.
    // Pair it with FloatRect::bottomLeftSquare to badge another icon.
    export void paintIn(PaintIconEvent&, const FloatRect& bounds, Lens = Lens::Empty, Tail = Tail::Yes, Halo = Halo::No);
    // The rect a lens of this radius needs, room for the tail included - for a caller that thinks
    // in lens sizes rather than in icon rects. No allowance for a halo: a caller that sizes by the
    // lens is drawing the glass on its own, which is the case that does not want one.
    export FloatRect rectForLensRadius(FloatPoint center, float lensRadius);


    //-------------------------------------------------------------------------


    namespace
    {
        // The shape, in units of the lens radius, carried over unchanged from the pixel painters.
        // The tail starts a little inside the ring rather than on it, so the join survives whatever
        // the two get rounded to.
        constexpr float k_strokeRatio = 0.2167f;
        constexpr float k_tailFrom = 0.666f;
        // Reaches further than the 1.466 the pixel painters used. That length left the ink barely
        // half the lens across, which is what made the band around it read as the longer of the two.
        constexpr float k_tailTo = 1.7f;

        // What the whole thing spans, in the same units: the lens, half a stroke outside it on the
        // upper left, and the tail reaching past it on the lower right. The lens is sized from this
        // whether or not the tail is drawn, so turning tails off does not hand back a bigger circle
        // - only a centred one.
        constexpr float k_extent = 2.0f + k_strokeRatio + (k_tailTo - 1.0f);
    }

    FloatRect rectForLensRadius(FloatPoint center, float lensRadius)
    {
        const float half = lensRadius * k_extent * 0.5f;
        return { center.x - half, center.y - half, center.x + half, center.y + half };
    }

    void paint(PaintIconEvent& event, Lens lens, Tail tail, Halo halo)
    {
        paintIn(event, event.iconRect(), lens, tail, halo);
    }

    void paintIn(PaintIconEvent& event, const FloatRect& bounds, Lens lens, Tail tail, Halo halo)
    {
        const bool withHalo = halo == Halo::Yes;

        // The band comes out of the same budget the lens is sized from, so asking for one shrinks
        // the glass instead of spilling it past the rect the caller reserved.
        const float side = std::min(bounds.width(), bounds.height());
        const float radius = side / (k_extent + (withHalo ? 2.0f * k_strokeRatio : 0.0f));
        const float strokeWidth = std::max(1.0f, radius * k_strokeRatio);
        const float haloWidth = withHalo ? strokeWidth : 0.0f;

        // With a tail the lens sits up and left, leaving the lower right for it. Without one there
        // is nothing to leave room for, so it goes back to the middle.
        const FloatPoint center = tail == Tail::Yes
            ? FloatPoint{
                bounds.left + haloWidth + radius + strokeWidth * 0.5f,
                bounds.top + haloWidth + radius + strokeWidth * 0.5f }
            : bounds.center();

        Canvas& canvas = event.canvas();
        const Color ink = event.textRgb(InkGrade::Strongest);
        // The glass is the form's surface rather than this control's, so the lens reads as a
        // hole through whatever it is lying on - and it is the same colour the separating band is
        // painted in, which is why one fill does both jobs below. That makes it a colour of the
        // icon's own as far as the event is concerned - no theme faded it on the way in - so it
        // goes through the fade by hand, and so does the mark derived from it.
        const Color rootSurface = event.bakedColors().rootSurface().toColor();
        const Color surface = event.applyDisabledFactor(rootSurface);

        const FloatPoint tailFrom{ center.x + radius * k_tailFrom, center.y + radius * k_tailFrom };
        const FloatPoint tailTo{ center.x + radius * k_tailTo, center.y + radius * k_tailTo };

        // The band first, as one silhouette: the tail stroked wide, then the disc over it. Both are
        // the same colour, so the disc doubles as the glass and the whole outline comes out as one
        // shape rather than two overlapping halos.
        //
        // Same two endpoints as the ink, and nothing added past them. drawLine caps round, so a
        // stroke this much wider already stands one halo width proud of the tip - exactly what it
        // stands along the sides. Extending the line as well put a second halo width there, and the
        // tail ended in a blob of surface with nothing inside it.
        if (tail == Tail::Yes && withHalo)
            canvas.drawLine(tailFrom, tailTo, surface, strokeWidth + haloWidth * 2.0f);

        canvas.fillCircle(center, radius + haloWidth, surface);

        // After the glass, so it butts into the ring instead of crossing the lens. It starts just
        // inside the ring's own band, which is what keeps the two joined.
        if (tail == Tail::Yes)
            canvas.drawLine(tailFrom, tailTo, ink, strokeWidth);

        canvas.drawCircle(center, radius, ink, strokeWidth);

        if (lens == Lens::Empty)
            return;

        PlusMark mark{
            .canvas = canvas,
            .center = center,
            .size = radius,
            .lineWidth = std::max(event.scaleBorder(0.6f), strokeWidth * 0.5f),
            .color = event.inkRgb(InkWell::accentInk())
        };
        mark.paintPlusOrMinus(lens == Lens::Plus);
    }

}
