export module ClaFi.Showcase.TextEngine.Scripts;

import ClaFi.Core.TextEngine.Text;

namespace ClaFi::Showcase::TextEngine
{
    // Fifty-six writing systems, three characters each, drawn in the styles the page states and no
    // family named at all - so every row is a question put to the platform's fallback rather than
    // to a font the page chose.
    //
    // WHAT A ROW ANSWERS WITH is one of three things, and telling them apart is the whole use of
    // the page. A row that draws its characters found a face. A row of boxes found a face that has
    // no glyph for them and drew .notdef. A row of blanks found a face whose .notdef is empty,
    // which is what a text face answers with for a script it never heard of. Only the first is
    // coverage; the other two are the same miss wearing different clothes.
    //
    // The characters come from the code points, not from any name written beside them, so a row
    // cannot show one script while claiming another.
    //
    // Each row is its own paragraph, which is what makes the page trustworthy: the shaper is asked
    // to guess a script per item, and a line mixing fifty-six of them would be guessed once and
    // wrongly. A row holds one script, its name, and its code points, so the guess it is asked for
    // is the one it can answer.
    export [[nodiscard]] Text buildScriptsShowcase();
}
