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
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Dom_TextSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    namespace
    {
        // **Written the way the document engine writes the value**, which gives it a spelling
        // without inventing a second one for the clipboard.
        template<typename Content>
        [[nodiscard]] std::string bytesOf(const Content& content)
        {
            Dom::Value<Content> node{ nullptr, {} };
            node.set(content);
            const Dom::Value<Content> blank{ nullptr, {} };

            std::wstringstream stream;
            Dom::FileFormat::ClaFi format;
            format.saveSectionToStream(*Dom::withoutDefaults(node, blank), stream);
            return toUtf8(stream.str());
        }

        template<typename Content>
        [[nodiscard]] Content contentOf(const std::string_view bytes)
        {
            Dom::Value<Content> node{ nullptr, {} };

            std::wstringstream stream{ fromUtf8(bytes) };
            Dom::FileFormat::ClaFi format;
            format.loadSectionFromStream(node, stream);
            return node.get();
        }

        // **A style goes by its name**, as a text's do: the one of that name the sheet reaches
        // stands for it, and where it reaches none, a copy based on its base as found here becomes
        // one of the sheet's own.
        const TextStyle* carryStyle(StyleSheet& sheet, const TextStyle* style)
        {
            if (!style)
                return nullptr;
            if (const TextStyle* named = sheet.find(style->name))
                return named;
            TextStyle copy = *style;
            copy.base = carryStyle(sheet, style->base);
            return sheet.carry(copy);
        }
    }

    Format ClaFiText::format()
    {
        return Format::Custom(std::wstring{ k_name });
    }

    std::string ClaFiText::toBytes(const Text& text)
    {
        return bytesOf(text);
    }

    Text ClaFiText::fromBytes(const std::string_view bytes)
    {
        return contentOf<Text>(bytes);
    }

    Format ClaFiStyleSheet::format()
    {
        return Format::Custom(std::wstring{ k_name });
    }

    std::string ClaFiStyleSheet::toBytes(const StyleSheet& sheet)
    {
        return bytesOf(sheet);
    }

    StyleSheet ClaFiStyleSheet::fromBytes(const std::string_view bytes)
    {
        return contentOf<StyleSheet>(bytes);
    }

    Format ClaFiTextFormat::format()
    {
        return Format::Custom(std::wstring{ k_name });
    }

    std::string ClaFiTextFormat::toBytes(const TextFormat& textFormat)
    {
        return bytesOf(textFormat);
    }

    TextFormat ClaFiTextFormat::fromBytes(const std::string_view bytes)
    {
        return contentOf<TextFormat>(bytes);
    }

    // **The package holds a text over the framework's sheet**, each style it names beyond the
    // framework's one of the text's own, so nothing in it points into the sheet it was copied
    // from, which may go first. The same styles go beside it as a sheet, with the format's.
    void addText(Source& source, const Text& text, const TextFormat* format)
    {
        Text carried{ StyleSheet::framework() };
        carried << text;
        StyleSheet styles{ &StyleSheet::framework() };
        styles.takeStyles(carried.styles());
        if (format && !format->style().empty())
            carryStyle(styles, text.styles().find(format->style()));
        source.add<ClaFiText>(std::move(carried));
        if (!styles.empty())
            source.add<ClaFiStyleSheet>(std::move(styles));
        if (format && !format->empty())
            source.add<ClaFiTextFormat>(*format);
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
        Transfer::registerByteSpelling<Transfer::ClaFiTextFormat>();
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
