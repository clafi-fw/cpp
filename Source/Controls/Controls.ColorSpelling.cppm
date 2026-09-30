export module ClaFi.Controls.ColorSpelling;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // How a colour is written for the user - on screen and in a copy. See Controls
    export enum class ColorSpelling
    {
        HexRgb,       // 5F3929
        HexBgr,       // 29395F - the order a COLORREF is written in
        DecimalRgb,   // 95 57 41
        CssHex        // #5F3929
    };

    // The colour as plain text, the way a copy hands it to another application.
    export [[nodiscard]] std::wstring spell(Color, ColorSpelling);
    // What a picker lists the spelling as.
    export [[nodiscard]] std::wstring_view nameOf(ColorSpelling);
    // The same digits into a text, each channel's run in that channel's pigment.
    export void appendSpelling(Text&, Color, ColorSpelling);
}
