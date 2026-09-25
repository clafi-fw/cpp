module ClaFi.StdActions.Transfer;

import ClaFi.StdActions;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.Transfer.Bytes;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Dom_TextSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    Format ClaFiText::format()
    {
        return Format::custom(std::wstring{ k_name });
    }

    std::string ClaFiText::toBytes(const Text& text)
    {
        Dom::Value<Text> node{ nullptr, {} };
        node.set(text);

        std::wstringstream stream;
        Dom::FileFormat::ClaFi format;
        format.saveSectionToStream(node, stream);
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
        Transfer::registerConversion<Transfer::ClaFiText, Transfer::PlainText>();
        Transfer::registerConversion<Transfer::PlainText, Transfer::ClaFiText>();

        // EVERY PASTE PRESENTER IN EVERY WINDOW, from this one connection. A presenter is
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
