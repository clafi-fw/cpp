export module ClaFi.Tools.WhatsClip.Encodings;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // Bytes read as text in one encoding. Never refuses: what is not in the encoding comes out
    // as replacement characters, which is itself an answer.
    export using EncodingDecode = std::wstring (*)(std::string_view bytes);

    // Whether bytes are in one encoding, told from the bytes and the name of their format.
    export using EncodingClaim = bool (*)(std::wstring_view formatName, std::string_view bytes);

    // One encoding a text page offers: its name, how bytes are read in it, and how bytes are
    // known to be in it.
    export struct EncodingEntry
    {
        std::wstring_view name;
        EncodingDecode decode{ nullptr };
        EncodingClaim claims{ nullptr };   // null for an encoding only ever picked by hand
    };

    // What a text page picks from, in this order, and what Auto asks in this order.
    export using EncodingList = std::vector<EncodingEntry>;

    // The encoding of bytes left to Auto: the first in the list that claims them, else none.
    export [[nodiscard]] std::optional<EncodingEntry> detectEncoding(const EncodingList&,
        std::wstring_view formatName, std::string_view bytes);

    // The claims the viewer makes for the encodings it reads.
    export namespace Claims
    {
        // A byte order mark, a name ending in W, or a control byte in the high half of most
        // units - which is what Latin and Cyrillic text look like in sixteen bits.
        [[nodiscard]] bool utf16Le(std::wstring_view formatName, std::string_view bytes);
        // A byte order mark, or bytes that are UTF-8 all the way through.
        [[nodiscard]] bool utf8(std::wstring_view formatName, std::string_view bytes);
        // Any bytes at all - the answer for what nothing before it claimed.
        [[nodiscard]] bool anyBytes(std::wstring_view formatName, std::string_view bytes);
    }

    // The readings the viewer does itself. Each skips its own byte order mark.
    export namespace Decode
    {
        [[nodiscard]] std::wstring utf16Le(std::string_view bytes);
        [[nodiscard]] std::wstring utf8(std::string_view bytes);
        // Byte for code point: the reading that cannot fail.
        [[nodiscard]] std::wstring latin1(std::string_view bytes);
    }

    // The encodings above with their claims, loosest last: UTF-16 LE, UTF-8, Latin-1.
    export [[nodiscard]] EncodingList defaultEncodings();
}
