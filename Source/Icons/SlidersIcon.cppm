export module ClaFi.Icons.SlidersIcon;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::SlidersIcon
{
    using namespace ::ClaFi::Graphics;

    // Three sliders, a knob riding each: the mark a page of values wears. See Icons#slidersicon
    export void draw(DrawIconEvent&);


    //-------------------------------------------------------------------------


    namespace
    {
        // The tracks, as shares of the icon's square. They stop short of its edges, so a knob near
        // either end still stands as a knob rather than as a disc cut by the edge.
        constexpr float k_trackLeftShare = 0.14f;
        constexpr float k_trackRightShare = 0.86f;

        // A knob, and the halo that clears the track beneath it. The track would otherwise meet
        // the disc, and neither would read at sixteen design units.
        constexpr float k_knobRadiusShare = 0.11f;
        constexpr float k_haloRadiusShare = 0.16f;

        // A knob's place: the row it rides and its distance along that row, both shares of the
        // square.
        struct Knob
        {
            float x;
            float y;
        };

        // The three at different places, so the mark reads as settings rather than as a pattern.
        constexpr Knob k_knobs[]{
            { 0.66f, 0.25f },
            { 0.34f, 0.50f },
            { 0.56f, 0.75f }
        };
    }


    //-------------------------------------------------------------------------


    void draw(DrawIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        // Nothing here reaches the edge of the square, so the shape takes the whole of it and the
        // stroke needs no room given back; every share below is of that square.
        const float size = std::min(iconRect.width(), iconRect.height());
        const FloatRect box = iconRect.centerRect(size);
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        Canvas& canvas = event.canvas();

        const Color trackColor = event.textRgb(InkGrade::Strong);
        const float left = box.left + size * k_trackLeftShare;
        const float right = box.left + size * k_trackRightShare;

        // The three tracks in one path: one stroke call, and the same weight on each.
        PixelPath tracks;
        for (const Knob& knob : k_knobs)
        {
            const float y = box.top + size * knob.y;
            tracks.moveTo(left, y);
            tracks.lineTo(right, y);
        }
        canvas.drawPath(tracks, { PathDrawLayer::Stroke(trackColor, strokeWidth) });

        // A halo in the surface color clears the track under a knob, and the knob takes the accent
        // as the part the reader moves.
        const Color haloColor = event.surfaceRgb();
        const Color knobColor = event.accentRgb(InkGrade::Strongest);
        for (const Knob& knob : k_knobs)
        {
            const FloatPoint center{ box.left + size * knob.x, box.top + size * knob.y };
            canvas.fillCircle(center, size * k_haloRadiusShare, haloColor);
            canvas.fillCircle(center, size * k_knobRadiusShare, knobColor);
        }
    }
}
