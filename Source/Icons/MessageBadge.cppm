export module ClaFi.Icons.MessageBadge;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi::Icons::MessageBadge
{
    using namespace ::ClaFi::Graphics;

    // What the five message icons are made of: a container filled in the colour its pigment names,
    // with the glyph drawn over it in the colour of the surface the icon stands on. The five
    // differ in their glyph and their pigment and in nothing else, so the container is stated here
    // once and each icon is left holding only what makes it that icon.
    //
    // Not an icon itself: it takes an ink, so it can never be a PaintIconFunc.

    // Where a glyph goes and what it is drawn with, as fractions of the whole badge. See Icons
    export struct GlyphSlot
    {
    public:
        // A point of the glyph, named as a fraction of the badge and answered in form coordinates.
        [[nodiscard]] FloatPoint at(float x, float y) const;
        // A dot of the glyph - the point under an exclamation mark, the tittle of an i - drawn
        // wider than the strokes around it, so that it reads as a mark of its own rather than as
        // the end of one.
        [[nodiscard]] float dotRadius() const;
    public:
        FloatRect box{};
        // Proportional to the badge rather than the Thin stroke the other icons draw with. The
        // fill is what carries a badge's shape, so a thin line laid over it thins away to a thread
        // as the badge grows. Never below that Thin stroke.
        float strokeWidth{ 1.0f };
        // The surface the badge stands on, so the glyph reads as cut through the fill. That is an
        // ink at elevation 0, which lies flush with the surface and carries the same disabled fade
        // the fill does.
        Color color{};
    };

    // The disc four of the five stand on, filling the icon rect.
    export GlyphSlot paintDisc(PaintIconEvent&, Ink);

    // The triangle a warning stands on, filling the icon rect. A triangle is what says warning
    // where a theme's hues are not the ones a reader expects, which is why the shape carries the
    // meaning rather than the colour alone.
    //
    // Its corners are rounded: a fill carries its corners at full strength, and a sharp apex at
    // sixteen pixels is one aliased spike.
    export GlyphSlot paintTriangle(PaintIconEvent&, Ink);


    //-------------------------------------------------------------------------


    // The glyph stroke, as a fraction of the badge.
    constexpr float k_glyphStroke = 0.085f;

    // How far a rounded corner reaches back along each of its edges.
    constexpr float k_cornerRadius = 0.10f;

    // The triangle, as fractions of the badge. It stands clear of the top so that the rounded apex
    // does not read as clipped, and reaches all but a sliver of the sides at the base, which is
    // where a triangle needs its width.
    constexpr float k_apexY = 0.055f;
    constexpr float k_baseY = 0.895f;
    constexpr float k_baseInset = 0.045f;

    // GlyphSlot

    FloatPoint GlyphSlot::at(float x, float y) const
    {
        return { box.left + box.width() * x, box.top + box.height() * y };
    }

    float GlyphSlot::dotRadius() const
    {
        return strokeWidth * 0.75f;
    }

    // MessageBadge

    // The square a badge is drawn in: the largest the icon rect holds, centred in it.
    static FloatRect badgeBox(const PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float size = std::min(iconRect.width(), iconRect.height());
        return FloatRect::fromDimensions(iconRect.center() - FloatPoint{ size * 0.5f }, { size, size });
    }

    static GlyphSlot glyphSlotOf(PaintIconEvent& event, const FloatRect& box)
    {
        return {
            .box = box,
            .strokeWidth = std::max(event.scaledStrokeWidth(Thickness::Thin), box.width() * k_glyphStroke),
            .color = event.inkColor(InkWell::surfaceInk())
        };
    }

    GlyphSlot paintDisc(PaintIconEvent& event, Ink ink)
    {
        const FloatRect box = badgeBox(event);
        event.canvas().fillCircle(box.center(), box.width() * 0.5f, event.inkColor(ink));
        return glyphSlotOf(event, box);
    }

    GlyphSlot paintTriangle(PaintIconEvent& event, Ink ink)
    {
        const FloatRect box = badgeBox(event);
        const float size = box.width();
        const std::array<FloatPoint, 3ull> corners{
            FloatPoint{ box.left + size * 0.5f, box.top + size * k_apexY },
            FloatPoint{ box.left + size * (1.0f - k_baseInset), box.top + size * k_baseY },
            FloatPoint{ box.left + size * k_baseInset, box.top + size * k_baseY }
        };

        PixelPath path;
        path.addRoundedPolygon(corners, size * k_cornerRadius);
        event.canvas().fillPath(path, event.inkColor(ink));
        return glyphSlotOf(event, box);
    }

}
