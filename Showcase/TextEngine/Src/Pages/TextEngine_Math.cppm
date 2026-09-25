export module ClaFi.Showcase.TextEngine.Math;

import ClaFi.Core.TextEngine.Text;

namespace ClaFi::Showcase::TextEngine
{
    // A page of mathematical notation, written as text.
    //
    // Nothing here is an image and nothing here is a control: a formula is one Text, and every
    // part of it is something the engine already does - a run raised or lowered by [sup] and
    // [sub], a family carrying the symbols, a rule or a bracket drawn into the flow as an inline
    // object, rows lined up on tab stops, a paragraph centred.
    //
    // WHAT THE PAGE HAS TO STATE FOR ITSELF is the arrangement. The engine breaks lines and
    // places runs; it does not stack a numerator over a denominator, and it does not grow a
    // bracket to the height of what it encloses. So a display fraction is written as three rows -
    // over the bar, the bar, under the bar - and a matrix bracket is written as one drawn piece
    // per row, tiling because each piece is exactly as tall as its row. A drawn rule is as wide
    // as it is told, because a Text is written before anything has been shaped and the page has
    // nothing to measure.
    export [[nodiscard]] Text buildMathShowcase();
}
