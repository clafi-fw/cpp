export module ClaFi.StdActions.Transfer;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.TextEngine.StyleSheet;
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
        // Written the way the document engine writes a Text. That is what gives the looks a
        // spelling without inventing a second one for the clipboard, and what lets two builds that
        // write a Text alike read each other.
        [[nodiscard]] static std::string toBytes(const Text&);
        [[nodiscard]] static Text fromBytes(std::string_view bytes);
    };

    // The styles a copied text names beyond the framework's, beside it on the clipboard.
    export struct ClaFiStyleSheet
    {
        using Content = StyleSheet;
        static constexpr std::wstring_view k_name = L"ClaFi.StyleSheet";

        [[nodiscard]] static Format format();
        [[nodiscard]] static std::string toBytes(const StyleSheet&);
        [[nodiscard]] static StyleSheet fromBytes(std::string_view bytes);
    };

    // Puts a text on the source, and beside it the styles it names. See StdActions#addtext
    export void addText(Source&, const Text&);
    // The text an offer holds, its styles found through the sheet given. See StdActions#taketext
    export [[nodiscard]] std::optional<Text> takeText(Offer&, const StyleSheet* sheet);

    // Rich down to plain is what lets a copy of one format paste into an application that has
    // never heard of ClaFi. Plain up to rich is the same text in one run of no look.
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
