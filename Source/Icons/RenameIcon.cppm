export module ClaFi.Icons.RenameIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Icons::RenameIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // The field, as a share of the icon's square. Wider than it is tall, which is what makes
        // it something a name is written IN rather than a box drawn around one.
        constexpr float k_fieldInset = 0.02f;
        constexpr float k_fieldTop = 0.15f;
        constexpr float k_fieldBottom = 0.85f;
        constexpr float k_fieldRadius = 0.14f;

        // The caret standing in the middle of it. Its serifs are what tell it from a divider, and
        // they are the first thing to close up as the icon gets smaller - which is why the caret
        // is a little under a third of the square rather than reaching the field's own edges.
        constexpr float k_caretTop = 0.35f;
        constexpr float k_caretBottom = 0.65f;
        constexpr float k_caretHalfWidth = 0.10f;
        constexpr float k_caretX = 0.5f;
    }

    // A name being typed where it sits: a caret standing in the field the name is written in.
    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        // A square, so the picture reads the same in a slot that is not one. Every share below is
        // of this square, rather than of a fixed number of design units, so the shape at 16 is the
        // shape at 24 rather than the same corner on a smaller box.
        const FloatRect box = iconRect.centerRect(std::min(iconRect.width(), iconRect.height()));
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        Canvas& canvas = event.canvas();

        // A strong grey holds the object, the accent marks what names the command. Here that is
        // the caret: the field is the place a name sits, and the caret is the name being changed.
        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color accentColor = event.accentRgb(InkGrade::Strongest);

        // THE FIELD'S BOUNDS ARE STATED WHOLE. A stroke lies inside the rect it is given, so
        // nothing here pre-insets by half of it.
        const FloatRect field = box.relativeRect(
            { k_fieldInset, k_fieldTop },
            { 1.0f - k_fieldInset, k_fieldBottom }
        );
        const float radius = box.width() * k_fieldRadius;
        canvas.drawRoundedRectangle(field, radius, radius, bodyColor, strokeWidth);

        // A PATH'S STROKE IS CENTRED ON THE LINE, which is the other convention: these points are
        // where the middle of each stroke goes, and that is why the caret is not built the way the
        // field above is.
        PixelPath path;
        path.moveTo(box.relativePt(k_caretX - k_caretHalfWidth, k_caretTop));
        path.lineTo(box.relativePt(k_caretX + k_caretHalfWidth, k_caretTop));
        path.moveTo(box.relativePt(k_caretX, k_caretTop));
        path.lineTo(box.relativePt(k_caretX, k_caretBottom));
        path.moveTo(box.relativePt(k_caretX - k_caretHalfWidth, k_caretBottom));
        path.lineTo(box.relativePt(k_caretX + k_caretHalfWidth, k_caretBottom));
        canvas.drawPath(path, accentColor, strokeWidth);
    }

}
