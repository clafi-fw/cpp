export module ClaFi.Core.Syntax.Colorize;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // Writes the source into the text with every token that has an ink of its own coloured in
    // it, as a marker pair per run. For a text that is built once and then stands - a sample in
    // a manual, a page of a showcase. A text that is edited is coloured by a CodeBox instead,
    // which draws the colours over the text rather than writing them into it.
    export void appendColorized(Text&, std::wstring_view source, const Language&,
        const Inks& = defaultInks());

    export [[nodiscard]] Text colorize(std::wstring_view source, const Language&,
        const Inks& = defaultInks());
}
