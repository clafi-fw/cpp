module ClaFi.Tools.WhatsClip.Encodings;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    namespace
    {
        using Bom = std::span<const unsigned char>;

        constexpr char32_t k_replacement = 0xFFFD;
        constexpr auto k_utf8Bom = std::to_array<unsigned char>({ 0xEF, 0xBB, 0xBF });
        constexpr auto k_utf16LeBom = std::to_array<unsigned char>({ 0xFF, 0xFE });

        [[nodiscard]] unsigned char byteAt(const std::string_view bytes, const std::size_t index)
        {
            return static_cast<unsigned char>(bytes[index]);
        }

        [[nodiscard]] bool startsWith(const std::string_view bytes, const Bom bom)
        {
            if (bytes.size() < bom.size())
                return false;

            for (std::size_t index = 0; index != bom.size(); ++index)
            {
                if (byteAt(bytes, index) != bom[index])
                    return false;
            }

            return true;
        }

        // The bytes without the mark, where the mark is there.
        [[nodiscard]] std::string_view withoutBom(const std::string_view bytes, const Bom bom)
        {
            return startsWith(bytes, bom) ? bytes.substr(bom.size()) : bytes;
        }

        [[nodiscard]] bool isSurrogate(const char32_t codePoint)
        {
            return codePoint >= 0xD800 && codePoint <= 0xDFFF;
        }

        [[nodiscard]] bool isHighSurrogate(const char32_t unit)
        {
            return unit >= 0xD800 && unit <= 0xDBFF;
        }

        [[nodiscard]] bool isLowSurrogate(const char32_t unit)
        {
            return unit >= 0xDC00 && unit <= 0xDFFF;
        }

        // A code point above the basic plane needs a surrogate pair where wchar_t is 16 bits and
        // fits a single unit where it is 32.
        void appendWide(std::wstring& out, const char32_t codePoint)
        {
            if constexpr (sizeof(wchar_t) == 2)
            {
                if (codePoint >= 0x10000)
                {
                    const char32_t offset = codePoint - 0x10000;
                    out.push_back(static_cast<wchar_t>(0xD800 + (offset >> 10)));
                    out.push_back(static_cast<wchar_t>(0xDC00 + (offset & 0x3FF)));
                    return;
                }
            }

            out.push_back(static_cast<wchar_t>(codePoint));
        }

        // Whether every sequence is one UTF-8 allows: a lead with its continuations all there,
        // spelled no longer than it needs to be, and naming neither a surrogate nor anything
        // past the last plane. The same rules fromUtf8 replaces by, asked rather than applied.
        [[nodiscard]] bool isUtf8(const std::string_view bytes)
        {
            std::size_t index = 0;
            while (index != bytes.size())
            {
                const unsigned char lead = byteAt(bytes, index);
                if (lead < 0x80)
                {
                    ++index;
                    continue;
                }

                char32_t codePoint = 0;
                std::size_t continuations = 0;
                char32_t lowest = 0;
                if ((lead & 0xE0) == 0xC0)
                {
                    codePoint = lead & 0x1Fu;
                    continuations = 1;
                    lowest = 0x80;
                }
                else if ((lead & 0xF0) == 0xE0)
                {
                    codePoint = lead & 0x0Fu;
                    continuations = 2;
                    lowest = 0x800;
                }
                else if ((lead & 0xF8) == 0xF0)
                {
                    codePoint = lead & 0x07u;
                    continuations = 3;
                    lowest = 0x10000;
                }
                else
                {
                    return false;
                }

                if (index + continuations >= bytes.size())
                    return false;

                for (std::size_t k = 1; k <= continuations; ++k)
                {
                    const unsigned char next = byteAt(bytes, index + k);
                    if ((next & 0xC0) != 0x80)
                        return false;

                    codePoint = (codePoint << 6) | (next & 0x3Fu);
                }

                if (codePoint < lowest || codePoint > 0x10FFFF || isSurrogate(codePoint))
                    return false;

                index += 1 + continuations;
            }

            return true;
        }

        // A control byte other than the three that text is full of.
        [[nodiscard]] bool isControl(const unsigned char value)
        {
            return value < 0x20 && value != '\t' && value != '\n' && value != '\r';
        }

        // Whether the high byte of most units is a control byte. Text in a Latin or Cyrillic
        // script has 00 or 04 there, and a byte string that is not UTF-16 has control bytes
        // almost nowhere - tabs and line ends excepted, which is why those do not count. More
        // than half, strictly: a short ASCII string ending in its NUL has one in every two.
        [[nodiscard]] bool looksUtf16Le(const std::string_view bytes)
        {
            if (bytes.size() < 2 || bytes.size() % 2 != 0)
                return false;

            std::size_t controlHighs = 0;
            for (std::size_t index = 1; index < bytes.size(); index += 2)
            {
                if (isControl(byteAt(bytes, index)))
                    ++controlHighs;
            }

            return controlHighs * 2 > bytes.size() / 2;
        }
    }

    std::optional<EncodingEntry> detectEncoding(const EncodingList& encodings,
        const std::wstring_view formatName, const std::string_view bytes)
    {
        for (const EncodingEntry& entry : encodings)
        {
            if (entry.claims && entry.claims(formatName, bytes))
                return entry;
        }

        return std::nullopt;
    }

    namespace Claims
    {
        bool utf16Le(const std::wstring_view formatName, const std::string_view bytes)
        {
            // The W is the Windows convention for a format's sixteen-bit spelling - FileNameW,
            // UniformResourceLocatorW - and nothing else names a format that way.
            return startsWith(bytes, k_utf16LeBom)
                || formatName.ends_with(L'W')
                || looksUtf16Le(bytes);
        }

        bool utf8(std::wstring_view, const std::string_view bytes)
        {
            return startsWith(bytes, k_utf8Bom) || isUtf8(bytes);
        }

        bool anyBytes(std::wstring_view, std::string_view)
        {
            return true;
        }
    }

    namespace Decode
    {
        // A lone surrogate, and the odd byte a string of units has no room for, each become one
        // replacement character.
        std::wstring utf16Le(const std::string_view bytes)
        {
            const std::string_view body = withoutBom(bytes, k_utf16LeBom);

            std::wstring result{};
            result.reserve(body.size() / 2);
            std::size_t index = 0;
            while (index + 1 < body.size())
            {
                const char32_t unit = byteAt(body, index) | (byteAt(body, index + 1) << 8);
                index += 2;

                if (isHighSurrogate(unit) && index + 1 < body.size())
                {
                    const char32_t low = byteAt(body, index) | (byteAt(body, index + 1) << 8);
                    if (isLowSurrogate(low))
                    {
                        index += 2;
                        appendWide(result, 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00));
                        continue;
                    }
                }

                appendWide(result, isSurrogate(unit) ? k_replacement : unit);
            }

            if (index != body.size())
                appendWide(result, k_replacement);

            return result;
        }

        std::wstring utf8(const std::string_view bytes)
        {
            return fromUtf8(withoutBom(bytes, k_utf8Bom));
        }

        std::wstring latin1(const std::string_view bytes)
        {
            std::wstring result{};
            result.reserve(bytes.size());
            for (std::size_t index = 0; index != bytes.size(); ++index)
                result.push_back(static_cast<wchar_t>(byteAt(bytes, index)));

            return result;
        }
    }

    // UTF-16 before UTF-8, because ASCII in sixteen bits is UTF-8 all the way through: a NUL is
    // as valid a byte as any.
    EncodingList defaultEncodings()
    {
        return {
            { L"UTF-16 LE", Decode::utf16Le, Claims::utf16Le },
            { L"UTF-8", Decode::utf8, Claims::utf8 },
            { L"Latin-1", Decode::latin1, Claims::anyBytes },
        };
    }
}
