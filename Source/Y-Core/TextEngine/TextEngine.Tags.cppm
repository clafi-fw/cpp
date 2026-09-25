export module ClaFi.Core.TextEngine.Tags;

import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi
{
    // THE ONE SPELLING OF A MARKER AS TEXT, read and written from the same tables. Fmt reads a
    // tag from between the brackets of a markup string, and a document reads and writes one per
    // marker - so a tag a Text can be written as is a tag Fmt understands, and neither side can
    // learn a name the other lacks.
    //
    // A marker has one spelling out and may have several in. The tables list every name a reader
    // accepts, and the first name listed for a marker is the one a writer produces. What takes an
    // argument is spelled "command argument": style body, color accent, size 14, font Consolas,
    // space 4, icon 16,16,13.6, link https://example.com, anchor clue-17. A style also reads as its
    // bare name - body, section, subsection - which is Fmt's shorthand. A category closes with its
    // own name - /color, /style, /script, /size, /font, /link, /anchor - so a closer does not have
    // to know which push it closes.
    //
    // A colour is stated outright (color #FF8800) or named by an ink colour and a grade, each
    // spelled as its enum is and each optional, text and strongest standing in for one left out:
    // color accent, color red muted, color muted. A grade no step names is written as its share.
    //
    // What a tag cannot carry is an icon's painter. An icon reads back at its stated extent and
    // draws nothing, which is what a text taken out of a file can say about it.

    // The marker a tag names, or nothing for a tag no table lists. Blanks around the tag are
    // ignored.
    export [[nodiscard]] std::optional<FormatItem> formatItemOf(std::wstring_view tag);

    // The tag a marker is written as.
    export [[nodiscard]] std::wstring tagOf(const FormatItem&);

    // An icon at the extent a tag states - "w", "w,h" or "w,h,baseline" in design units, the
    // height defaulting to the width and the baseline to 0.85 of the height - with no painter.
    // Fmt hands the painter it was given to the icon this answers.
    export [[nodiscard]] InTextIcon iconOf(std::wstring_view extent);
}
