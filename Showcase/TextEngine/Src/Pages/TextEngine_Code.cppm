export module ClaFi.Showcase.TextEngine.Code;

import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
    // A page of source, for measuring what an editor's key press costs.
    //
    // What makes source the interesting input is its shape rather than its meaning: a few thousand
    // short paragraphs, each carrying a handful of tokens, which is the case a whole-document
    // rebuild is worst at. The colouring is the CodeBox's, drawn over the text as it is shown, so
    // the text itself carries no marker per token.

    // At least lineCount lines, built by repeating one block with the repetition number worked
    // into its names so that no two paragraphs hold the same text. Whole blocks only, so the
    // count is a floor: the last block runs to its end rather than stopping mid-function.
    export [[nodiscard]] std::wstring buildCppSource(std::size_t lineCount);

    // The source above, set in the monospace style and nothing else.
    export [[nodiscard]] Text buildCodeShowcase(std::size_t lineCount);
}
