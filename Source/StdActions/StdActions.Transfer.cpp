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
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    namespace
    {
        // **A carried style gives way to one the reading sheet knows by its name.** The first
        // style of the look's chain that the reading sheet finds by name becomes the look's
        // style, and the fields of the styles before it in the chain become the look's own, the
        // inner winning.
        [[nodiscard]] TextLook adoptedLook(const TextLook& look)
        {
            TextLook result = look;
            result.style = nullptr;
            for (const TextStyle* style = look.style; style; style = style->base)
            {
                if (const TextStyle* known = StyleSheet::findReading(style->name))
                {
                    result.style = known;
                    return result;
                }
                if (!result.fontFamily && style->fontFamily)
                    result.fontFamily = std::wstring{ *style->fontFamily };
                if (!result.fontSize)
                    result.fontSize = style->fontSize;
                if (!result.fontWeight)
                    result.fontWeight = style->fontWeight;
                if (!result.fontStyle)
                    result.fontStyle = style->fontStyle;
                if (!result.ink)
                    result.ink = style->ink;
            }
            return result;
        }

        // The style of each name the text is set in, the first one met. NamedIn carries that one
        // under the name, meeting them in the same order: run by run, each run's chain from its
        // outermost style.
        [[nodiscard]] StyleSheet::StyleList firstOfEachName(const Text& text)
        {
            StyleSheet::StyleList result;
            text.forEachRun([&result](const TextRunView& run) {
                StyleSheet::StyleList chain;
                for (const TextStyle* style = run.look.style; style; style = style->base)
                {
                    if (!TextStyles::contains(style) && !style->name.empty())
                        chain.push_back(style);
                }
                for (const TextStyle* style : std::views::reverse(chain))
                {
                    if (std::ranges::find(result, style->name, &TextStyle::name) == result.end())
                        result.push_back(style);
                }
            });
            return result;
        }

        // **The text a package holds names the styles the package carries**, which stay as long
        // as the package does, and none of the sheet it was copied from, which may go first. A
        // style whose name another style took is folded into the look, the inner fields winning,
        // down to one the package carries or one of the framework's.
        [[nodiscard]] Text carriedText(const Text& text, const StyleSheet& carried)
        {
            const StyleSheet::StyleList taken = firstOfEachName(text);
            TextPieces pieces = text.pieces();
            for (TextPiece& piece : pieces)
            {
                TextLook& look = piece.look;
                const TextStyle* style = look.style;
                look.style = nullptr;
                for (; style; style = style->base)
                {
                    if (TextStyles::contains(style))
                    {
                        look.style = style;
                        break;
                    }
                    if (std::ranges::find(taken, style) != taken.end())
                    {
                        look.style = carried.findOwn(style->name);
                        break;
                    }
                    if (!look.fontFamily && style->fontFamily)
                        look.fontFamily = std::wstring{ *style->fontFamily };
                    if (!look.fontSize)
                        look.fontSize = style->fontSize;
                    if (!look.fontWeight)
                        look.fontWeight = style->fontWeight;
                    if (!look.fontStyle)
                        look.fontStyle = style->fontStyle;
                    if (!look.ink)
                        look.ink = style->ink;
                }
            }
            return Text{ std::move(pieces) };
        }
    }

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

    // The sheet is moved into the package and not copied, so the styles the text names there keep
    // their addresses.
    void addText(Source& source, const Text& text)
    {
        StyleSheet carried = StyleSheet::NamedIn(text);
        if (carried.empty())
        {
            source.add<ClaFiText>(text);
            return;
        }
        source.add<ClaFiText>(carriedText(text, carried));
        source.add<ClaFiStyleSheet>(std::move(carried));
    }

    // **The text is read through the carried sheet, then given to the sheet stated**: a style
    // named in both is the stated sheet's, and one it does not know becomes the run's own fields
    // over the nearest it does know. See StdActions#taketext
    std::optional<Text> takeText(Offer& offer, const StyleSheet* sheet)
    {
        const StyleSheet::ReadingScope statedScope{ sheet };
        const std::optional<StyleSheet> carried = take<ClaFiStyleSheet>(offer);
        if (!carried.has_value())
            return take<ClaFiText>(offer);

        std::optional<Text> text;
        {
            const StyleSheet::ReadingScope carriedScope{ &carried.value() };
            text = take<ClaFiText>(offer);
        }
        if (!text.has_value())
            return std::nullopt;

        TextPieces pieces = text->pieces();
        for (TextPiece& piece : pieces)
            piece.look = adoptedLook(piece.look);
        return Text{ std::move(pieces) };
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
