export module ClaFi.Icons.Magnifier;

import ClaFi.Icons.PlusMark;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
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
        // The shape in units of the lens radius. The ring is stroked on the radius itself.
        constexpr float k_strokeRatio = 0.287f;
        constexpr float k_tailWidthShare = 0.9f; // of the ring's band
        constexpr float k_tailCornerShare = 0.23f; // of the tail's width, rounding its flat end
        // On the ring's centre line, so the near end stays under the band however the two round.
        constexpr float k_tailFrom = 1.0f;
        constexpr float k_tailTo = 1.85f;
        // Down and to the right, 22.6 degrees off vertical.
        constexpr FloatPoint k_tailDirection{ 0.3836f, 0.9235f };

        // The outline's box in lens radii, measured from the lens centre.
        struct Extent
        {
            float left{ 0.0f };
            float top{ 0.0f };
            float right{ 0.0f };
            float bottom{ 0.0f };
            [[nodiscard]] constexpr float width() const { return right - left; }
            [[nodiscard]] constexpr float height() const { return bottom - top; }
        };

        // A halo widens the box by one band on every side. The tail's end corners count unrounded.
        [[nodiscard]] constexpr Extent extentOf(Halo);
        [[nodiscard]] PixelPath tailOutline(FloatPoint center, float radius, float strokeWidth);
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
        const Extent extent = extentOf(halo);
        const float radius = std::min(bounds.width() / extent.width(), bounds.height() / extent.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float haloWidth = withHalo ? strokeWidth : 0.0f;

        // With a tail the whole outline is centred, which lifts the lens to leave the tail room
        // below. The lens is sized for the tail either way, so turning tails off does not hand
        // back a bigger circle - only a centred one.
        const FloatPoint middle = bounds.center();
        const FloatPoint center = tail == Tail::Yes
            ? FloatPoint{
                middle.x - radius * (extent.left + extent.right) * 0.5f,
                middle.y - radius * (extent.top + extent.bottom) * 0.5f }
            : middle;

        Canvas& canvas = event.canvas();
        const Color ink = event.textRgb(InkGrade::Strongest);
        // The glass is the form's surface rather than this control's, so the lens reads as a
        // hole through whatever it is lying on - and it is the same colour the separating band is
        // painted in, which is why one fill does both jobs below. That makes it a colour of the
        // icon's own as far as the event is concerned - no theme faded it on the way in - so it
        // goes through the fade by hand, and so does the mark derived from it.
        const Color rootSurface = event.bakedColors().rootSurface().toColor();
        const Color surface = event.applyDisabledFactor(rootSurface);

        const PixelPath tailPath = tailOutline(center, radius, strokeWidth);

        // The band first, as one silhouette: the tail's outline stroked twice the halo wide, then
        // the disc over it. Both are the same colour, so the disc doubles as the glass and the
        // whole outline comes out as one shape rather than two overlapping halos.
        if (tail == Tail::Yes && withHalo)
            canvas.drawPath(tailPath, surface, haloWidth * 2.0f);

        canvas.fillCircle(center, radius + haloWidth, surface);

        // After the glass, so it butts into the ring instead of crossing the lens.
        if (tail == Tail::Yes)
            canvas.fillPath(tailPath, ink);

        canvas.drawCircle(center, radius, ink, strokeWidth);

        if (lens == Lens::Empty)
            return;

        PlusMark mark{
            .canvas = canvas,
            .center = center,
            .size = radius * 1.32f,
            .lineWidth = event.scaledStrokeWidth(Thickness::Regular),
            .color = event.inkRgb(InkWell::accentInk())
        };
        mark.paintPlusOrMinus(lens == Lens::Plus);
    }

    FloatRect rectForLensRadius(FloatPoint center, float lensRadius)
    {
        const Extent extent = extentOf(Halo::No);
        const float halfWidth = lensRadius * extent.width() * 0.5f;
        const float halfHeight = lensRadius * extent.height() * 0.5f;
        return { center.x - halfWidth, center.y - halfHeight, center.x + halfWidth, center.y + halfHeight };
    }

    namespace
    {
        constexpr Extent extentOf(Halo halo)
        {
            const float lens = 1.0f + k_strokeRatio * 0.5f;
            const float halfWidth = k_strokeRatio * k_tailWidthShare * 0.5f;
            const float endX = k_tailDirection.x * k_tailTo;
            const float endY = k_tailDirection.y * k_tailTo;
            const float acrossX = -k_tailDirection.y * halfWidth;
            const float acrossY = k_tailDirection.x * halfWidth;
            const float band = halo == Halo::Yes ? k_strokeRatio : 0.0f;

            return {
                .left = std::min({ -lens, endX + acrossX, endX - acrossX }) - band,
                .top = std::min({ -lens, endY + acrossY, endY - acrossY }) - band,
                .right = std::max({ lens, endX + acrossX, endX - acrossX }) + band,
                .bottom = std::max({ lens, endY + acrossY, endY - acrossY }) + band
            };
        }

        PixelPath tailOutline(FloatPoint center, float radius, float strokeWidth)
        {
            const float width = strokeWidth * k_tailWidthShare;
            const FloatPoint across = {
                -k_tailDirection.y * width * 0.5f,
                k_tailDirection.x * width * 0.5f
            };
            const FloatPoint from = {
                center.x + k_tailDirection.x * radius * k_tailFrom,
                center.y + k_tailDirection.y * radius * k_tailFrom
            };
            const FloatPoint to = {
                center.x + k_tailDirection.x * radius * k_tailTo,
                center.y + k_tailDirection.y * radius * k_tailTo
            };
            const std::array<FloatPoint, 4> corners = {
                FloatPoint{ from.x + across.x, from.y + across.y },
                FloatPoint{ to.x + across.x, to.y + across.y },
                FloatPoint{ to.x - across.x, to.y - across.y },
                FloatPoint{ from.x - across.x, from.y - across.y }
            };

            PixelPath outline;
            outline.addRoundedPolygon(corners, width * k_tailCornerShare);
            return outline;
        }
    }

}
