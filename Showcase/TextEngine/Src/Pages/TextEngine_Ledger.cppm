export module ClaFi.Showcase.TextEngine.Rich;

import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
    // A page of formatted prose, for measuring what a document costs when its paragraphs carry
    // inline objects and change font from run to run.
    //
    // What separates this from the code page beside it is what a line holds rather than how many
    // lines there are. The families are proportional and change from line to line, sizes and
    // weights change inside a line, and most lines carry an inline object: a bullet, a tab stop,
    // a fixed space, a flex space. A page of source carries none of those, so a cost that scales
    // with them shows here and nowhere else in the showcase.
    //
    // WHAT IT SAYS is a logic puzzle of the Einstein kind, at a length no puzzle should reach: an
    // opening that states the cast and how to read a round, rounds of numbered clues, and a
    // closing that claims the thing is solved and answers by reference rather than by naming
    // anyone. The two ends are written once and are built line by line; a round is a table of
    // fifty and is repeated. Clue numbers run across the whole document, so a clue in the last
    // round can send a reader back to one in the first. Every such reference is a link, and the
    // number where a clue is stated is the anchor it lands on - two markers a clue and two a
    // reference, which a key press on this page pays for with the rest.
    //
    // No two clues are written from the same tuple until the five casts have run their product -
    // seventeen people, thirteen trades, eleven objects, seven states and nineteen rooms, each
    // stepped by its own stride, which is 323,323 clues against the twenty-three a round spends.
    //
    // At least lineCount lines, the opening and closing included, and whole rounds only - so the
    // count is a floor.
    export [[nodiscard]] Text buildRichShowcase(std::size_t lineCount);
}
