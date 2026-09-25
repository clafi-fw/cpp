export module ClaFi.StdActions.Transfer;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // The framework's own rich text on the clipboard, as a named format. See StdActions
    export struct ClaFiText
    {
        using Content = Text;
        static constexpr std::wstring_view k_name = L"ClaFi.Text";

        [[nodiscard]] static Format format();
        // Written the way the document engine writes a Text. That is what gives the markers a
        // spelling without inventing a second one for the clipboard, and what lets a build from
        // last month read what this one wrote.
        [[nodiscard]] static std::string toBytes(const Text&);
        [[nodiscard]] static Text fromBytes(std::string_view bytes);
    };

    // Rich down to plain is what lets a copy of one format paste into an application that has
    // never heard of ClaFi. Plain up to rich is the same text with no markers in it.
    template<>
        struct FormatConversion<ClaFiText, PlainText>
    {
        [[nodiscard]] static std::wstring convert(const Text&);
    };

    template<>
        struct FormatConversion<PlainText, ClaFiText>
    {
        [[nodiscard]] static Text convert(const std::wstring&);
    };
}

namespace ClaFi::StdActions
{
    // Puts the framework's own formats and the conversions between them in the Transfer tables,
    // and has the paste action follow the clipboard given. ApplicationBase::initialize calls it,
    // so an application has rich text on the clipboard without writing anything.
    export void registerTransferFormats(Transfer::Clipboard&);
}
