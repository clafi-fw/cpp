export module ClaFi.Core.System.InkWell;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

namespace ClaFi
{
    // Every ink the framework names, and the one place their numbers are written.
    //
    // Three things live here. The accessors each name one colour and take the grade it is drawn
    // at and nothing else. The tones are what each semantic hue is drawn at on either side of the
    // theme. The constants are the colours that carry a meaning, named once so that two icons
    // drawing the same warning cannot arrive at two yellows.
    //
    // Nothing here is a colour yet. A semantic hue is a region of the wheel that every theme's
    // harmony fills its own way, the accent and the spot are the theme's, and which of a hue's
    // two tones applies is the mode the painter stands in - so an ink becomes a colour where it
    // is used, and not before.
    export namespace InkWell
    {
        // A shade of the ink the paint chain arrived at: Strongest is that ink, and every step
        // below it is a share of the way back toward the surface.
        [[nodiscard]] constexpr Ink textInk(InkGrade grade = InkGrade::Strongest)
        {
            Ink result;
            result.grade = gradeOf(grade);
            return result;
        }

        // ...and a share of a caller's own, for a step the framework does not draw in.
        [[nodiscard]] constexpr Ink textInk(float grade)
        {
            Ink result;
            result.grade = grade;
            return result;
        }

        // Flush with the surface and so invisible on it, which is what something drawn in its own
        // backdrop's colour asks for - a knockout band under a line that crosses another.
        [[nodiscard]] constexpr Ink surfaceInk()
        {
            return textInk(0.0f);
        }

        // The theme's accent rule over the chain's ink, faded toward the surface by the grade the
        // way the text ink is. Stated as a share of a caller's own, or as one of the steps.
        [[nodiscard]] constexpr Ink accentInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Accent;
            return result;
        }

        [[nodiscard]] constexpr Ink accentInk(InkGrade grade = InkGrade::Strongest)
        {
            return accentInk(gradeOf(grade));
        }

        // The theme's spot rule, on the same terms as the accent above: the second accent, kept
        // for what stands apart from the interface rather than answers to it.
        [[nodiscard]] constexpr Ink spotInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Spot;
            return result;
        }

        [[nodiscard]] constexpr Ink spotInk(InkGrade grade = InkGrade::Strongest)
        {
            return spotInk(gradeOf(grade));
        }

        // The palette's semantic hues, each at the tone slotTones states for the side of the
        // theme the painter stands on. The grade fades one toward the surface the way it fades
        // the text ink, so at Strongest a hue stands at its own tone.
        [[nodiscard]] constexpr Ink yellowInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Yellow;
            return result;
        }

        [[nodiscard]] constexpr Ink yellowInk(InkGrade grade = InkGrade::Strongest)
        {
            return yellowInk(gradeOf(grade));
        }

        [[nodiscard]] constexpr Ink greenInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Green;
            return result;
        }

        [[nodiscard]] constexpr Ink greenInk(InkGrade grade = InkGrade::Strongest)
        {
            return greenInk(gradeOf(grade));
        }

        [[nodiscard]] constexpr Ink blueInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Blue;
            return result;
        }

        [[nodiscard]] constexpr Ink blueInk(InkGrade grade = InkGrade::Strongest)
        {
            return blueInk(gradeOf(grade));
        }

        [[nodiscard]] constexpr Ink redInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Red;
            return result;
        }

        [[nodiscard]] constexpr Ink redInk(InkGrade grade = InkGrade::Strongest)
        {
            return redInk(gradeOf(grade));
        }

        // Pure black and white, the same on either side of the theme. The grade fades one toward
        // the surface the way it fades the text ink.
        [[nodiscard]] constexpr Ink blackInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::Black;
            return result;
        }

        [[nodiscard]] constexpr Ink blackInk(InkGrade grade = InkGrade::Strongest)
        {
            return blackInk(gradeOf(grade));
        }

        [[nodiscard]] constexpr Ink whiteInk(float grade)
        {
            Ink result = textInk(grade);
            result.color = InkColor::White;
            return result;
        }

        [[nodiscard]] constexpr Ink whiteInk(InkGrade grade = InkGrade::Strongest)
        {
            return whiteInk(gradeOf(grade));
        }

        // What each semantic hue is drawn at in either colour mode. Declared once, in code, and
        // serving both modes from that one declaration, which is why it states a tone for each
        // rather than one elevation the mode would orient: a yellow border going brown on a dark
        // theme is a border doing its job, and a yellow warning icon going brown is one failing
        // at its.
        //
        // The tones are tuned against pages at luminosity 0.13 and 0.87, from measurements taken
        // at the 3:1 graphics threshold. A yellow's light tone reads as amber rather than lemon
        // because nothing brighter clears 3:1 on a light page - a bright yellow IS light - which
        // is why every system draws that icon amber on light and lemon on dark. Raising it back
        // toward lemon breaks the contrast.
        [[nodiscard]] constexpr SlotTones slotTones(ColorSlot slot)
        {
            switch (slot)
            {
                case ColorSlot::Yellowish:
                    return { .dark{ 1.0f, 0.9f }, .light{ 1.0f, 0.51f } };
                case ColorSlot::Greenish:
                    return { .dark{ 1.0f, 0.84f }, .light{ 1.0f, 0.49f } };
                case ColorSlot::Blueish:
                    return { .dark{ 1.0f, 0.47f }, .light{ 1.0f, 0.41f } };
                case ColorSlot::Reddish:
                    return { .dark{ 1.0f, 0.56f }, .light{ 1.0f, 0.55f } };
                case ColorSlot::Count:
                    break;
            }
            unreachable("a colour slot with no tones");
        }

        constexpr Ink Yellow = yellowInk();
        constexpr Ink Green = greenInk();
        constexpr Ink Blue = blueInk();
        constexpr Ink Red = redInk();
        constexpr Ink Black = blackInk();
        constexpr Ink White = whiteInk();
    }
}
