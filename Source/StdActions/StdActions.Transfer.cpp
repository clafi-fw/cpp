module ClaFi.StdActions.Transfer;

import ClaFi.StdActions;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.Transfer.Bytes;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Dom_TextSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    Format ClaFiText::format()
    {
        return Format::Custom(std::wstring{ k_name });
    }

    std::string ClaFiText::toBytes(const Text& text)
    {
        Dom::Value<Text> node{ nullptr, {} };
        node.set(text);
        const Dom::Value<Text> blank{ nullptr, {} };

        std::wstringstream stream;
        Dom::FileFormat::ClaFi format;
        format.saveSectionToStream(*Dom::withoutDefaults(node, blank), stream);
        return toUtf8(stream.str());
    }

    Text ClaFiText::fromBytes(const std::string_view bytes)
    {
        Dom::Value<Text> node{ nullptr, {} };

        std::wstringstream stream{ fromUtf8(bytes) };
        Dom::FileFormat::ClaFi format;
        format.loadSectionFromStream(node, stream);
        return node.get();
    }

    Format ClaFiStyleSheet::format()
    {
        return Format::Custom(std::wstring{ k_name });
    }

    std::string ClaFiStyleSheet::toBytes(const StyleSheet& sheet)
    {
        Dom::Value<StyleSheet> node{ nullptr, {} };
        node.set(sheet);
        const Dom::Value<StyleSheet> blank{ nullptr, {} };

        std::wstringstream stream;
        Dom::FileFormat::ClaFi format;
        format.saveSectionToStream(*Dom::withoutDefaults(node, blank), stream);
        return toUtf8(stream.str());
    }

    StyleSheet ClaFiStyleSheet::fromBytes(const std::string_view bytes)
    {
        Dom::Value<StyleSheet> node{ nullptr, {} };

        std::wstringstream stream{ fromUtf8(bytes) };
        Dom::FileFormat::ClaFi format;
        format.loadSectionFromStream(node, stream);
        return node.get();
    }

    // **The package holds a text over the framework's sheet**, each style it names beyond the
    // framework's one of the text's own, so nothing in it points into the sheet it was copied
    // from, which may go first. The same styles go beside it as a sheet.
    void addText(Source& source, const Text& text)
    {
        Text carried{ StyleSheet::framework() };
        carried << text;
        StyleSheet styles{ &StyleSheet::framework() };
        styles.takeStyles(carried.styles());
        source.add<ClaFiText>(std::move(carried));
        if (!styles.empty())
            source.add<ClaFiStyleSheet>(std::move(styles));
    }

    // **The text is read through the carried sheet, then put in a text over the sheet stated**:
    // a style named in both is the stated sheet's, and one it lacks comes as one of the text's
    // own. See StdActions#taketext
    std::optional<Text> takeText(Offer& offer, const StyleSheet* sheet)
    {
        const StyleSheet& stated = sheet ? *sheet : StyleSheet::application();
        const std::optional<StyleSheet> carried = take<ClaFiStyleSheet>(offer);
        std::optional<Text> text;
        {
            const StyleSheet::ReadingScope scope{ carried.has_value() ? &carried.value() : &stated };
            text = take<ClaFiText>(offer);
        }
        if (!text.has_value())
            return std::nullopt;

        Text result{ stated };
        result << *text;
        return result;
    }

    std::wstring FormatConversion<ClaFiText, PlainText>::convert(const Text& text)
    {
        return text.plainText();
    }

    Text FormatConversion<PlainText, ClaFiText>::convert(const std::wstring& plainText)
    {
        return Text{ plainText };
    }
}

namespace ClaFi::StdActions
{
    void registerTransferFormats(Transfer::Clipboard& clipboard)
    {
        Transfer::registerByteSpelling<Transfer::ClaFiText>();
        Transfer::registerByteSpelling<Transfer::ClaFiStyleSheet>();
        Transfer::registerConversion<Transfer::ClaFiText, Transfer::PlainText>();
        Transfer::registerConversion<Transfer::PlainText, Transfer::ClaFiText>();

        // **Every paste presenter in every window**, from this one connection. A presenter is
        // attached to the action and not to the clipboard, so the action is what has to be told:
        // invalidateState asks each of them for its answer again. Nothing in an application
        // subscribes to learn whether Paste is available, and nothing polls to find out.
        //
        // Never disconnected, and it does not need to be: the handler holds nothing, and the
        // dispatcher is the clipboard's, which the platform keeps up past the last window.
        clipboard.events().connect<Transfer::ClipboardChangeEvent>(
            [](Transfer::ClipboardChangeEvent&) {
                paste.invalidateState();
            });
    }
}
