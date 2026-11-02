module ClaFi.Tools.WhatsClip.Encodings;

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

        // How many bytes the mark takes at the start - none where it is not there.
        [[nodiscard]] std::size_t bomLength(const std::string_view bytes, const Bom bom)
        {
            return startsWith(bytes, bom) ? bom.size() : 0;
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

        // A character's units, each starting at the character's first byte. A code point above the
        // basic plane needs a surrogate pair where wchar_t is 16 bits and fits a single unit where
        // it is 32.
        void appendCharacter(DecodedText& decoded, const char32_t codePoint,
            const std::size_t byteStart)
        {
            if constexpr (sizeof(wchar_t) == 2)
            {
                if (codePoint >= 0x10000)
                {
                    const char32_t offset = codePoint - 0x10000;
                    decoded.appendUnit(static_cast<wchar_t>(0xD800 + (offset >> 10)), byteStart);
                    decoded.appendUnit(static_cast<wchar_t>(0xDC00 + (offset & 0x3FF)), byteStart);
                    return;
                }
            }

            decoded.appendUnit(static_cast<wchar_t>(codePoint), byteStart);
        }

        // One UTF-8 sequence: what it spells - none where it breaks a rule - and its length.
        struct Utf8Character
        {
            std::optional<char32_t> codePoint{};
            std::size_t length{ 1 };
        };

        // The UTF-8 sequence that starts at that byte.
        [[nodiscard]] Utf8Character readUtf8Character(const std::string_view bytes,
            const std::size_t index)
        {
            const unsigned char lead = byteAt(bytes, index);
            if (lead < 0x80)
                return { lead, 1 };

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
                return { std::nullopt, 1 };
            }

            // A sequence cut short takes the continuations it had, and the byte that cut it is read
            // again as a lead - one replacement for one broken sequence.
            std::size_t length = 1;
            while (length != continuations + 1)
            {
                if (index + length == bytes.size())
                    return { std::nullopt, length };

                const unsigned char next = byteAt(bytes, index + length);
                if ((next & 0xC0) != 0x80)
                    return { std::nullopt, length };

                codePoint = (codePoint << 6) | (next & 0x3Fu);
                ++length;
            }

            // Spelled no longer than it needs to be, and naming neither a surrogate nor anything
            // past the last plane.
            if (codePoint < lowest || codePoint > 0x10FFFF || isSurrogate(codePoint))
                return { std::nullopt, length };

            return { codePoint, length };
        }

        // Whether every sequence in the bytes is one UTF-8 allows.
        [[nodiscard]] bool isUtf8(const std::string_view bytes)
        {
            std::size_t index = 0;
            while (index != bytes.size())
            {
                const Utf8Character character = readUtf8Character(bytes, index);
                if (!character.codePoint)
                    return false;

                index += character.length;
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

    void DecodedText::appendUnit(const wchar_t unit, const std::size_t byteStart)
    {
        text.push_back(unit);
        byteStarts.push_back(static_cast<std::uint32_t>(byteStart));
    }

    void DecodedText::finish(const std::size_t byteEnd)
    {
        byteStarts.push_back(static_cast<std::uint32_t>(byteEnd));
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
        DecodedText utf16Le(const std::string_view bytes)
        {
            DecodedText result{};
            result.text.reserve(bytes.size() / 2);
            result.byteStarts.reserve(bytes.size() / 2 + 1);
            std::size_t index = bomLength(bytes, k_utf16LeBom);
            while (index + 1 < bytes.size())
            {
                const std::size_t characterStart = index;
                const char32_t unit = byteAt(bytes, index) | (byteAt(bytes, index + 1) << 8);
                index += 2;

                if (isHighSurrogate(unit) && index + 1 < bytes.size())
                {
                    const char32_t low = byteAt(bytes, index) | (byteAt(bytes, index + 1) << 8);
                    if (isLowSurrogate(low))
                    {
                        index += 2;
                        appendCharacter(result,
                            0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00),
                            characterStart);
                        continue;
                    }
                }

                appendCharacter(result, isSurrogate(unit) ? k_replacement : unit, characterStart);
            }

            if (index != bytes.size())
                appendCharacter(result, k_replacement, index);

            result.finish(bytes.size());
            return result;
        }

        DecodedText utf8(const std::string_view bytes)
        {
            DecodedText result{};
            result.text.reserve(bytes.size());
            result.byteStarts.reserve(bytes.size() + 1);
            std::size_t index = bomLength(bytes, k_utf8Bom);
            while (index != bytes.size())
            {
                const Utf8Character character = readUtf8Character(bytes, index);
                appendCharacter(result, character.codePoint.value_or(k_replacement), index);
                index += character.length;
            }

            result.finish(bytes.size());
            return result;
        }

        DecodedText latin1(const std::string_view bytes)
        {
            DecodedText result{};
            result.text.reserve(bytes.size());
            result.byteStarts.reserve(bytes.size() + 1);
            for (std::size_t index = 0; index != bytes.size(); ++index)
                result.appendUnit(static_cast<wchar_t>(byteAt(bytes, index)), index);

            result.finish(bytes.size());
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
