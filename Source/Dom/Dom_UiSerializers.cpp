module ClaFi.Core.Dom_UiSerializers;

import ClaFi.Core.DomEngine;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    namespace
    {
        constexpr wchar_t k_colorMark = L'#';
        constexpr std::wstring_view k_hexDigits = L"0123456789ABCDEF";
        constexpr std::size_t k_opaqueDigits = 6ull;
        constexpr std::size_t k_translucentDigits = 8ull;
        constexpr ColorByte k_opaqueAlpha = 0xFF;

        void appendHex(std::wstring& text, const ColorByte value)
        {
            text += k_hexDigits[value >> 4];
            text += k_hexDigits[value & 0x0F];
        }

        // The value of one hex digit in either case, and nothing for any other character.
        [[nodiscard]] std::optional<std::uint32_t> digitValueOf(const wchar_t digit)
        {
            if (digit >= L'0' && digit <= L'9')
                return static_cast<std::uint32_t>(digit - L'0');
            if (digit >= L'a' && digit <= L'f')
                return static_cast<std::uint32_t>(digit - L'a' + 10);
            if (digit >= L'A' && digit <= L'F')
                return static_cast<std::uint32_t>(digit - L'A' + 10);
            return std::nullopt;
        }

        [[nodiscard]] ColorByte byteAt(const std::uint32_t value, const std::uint32_t shift)
        {
            return static_cast<ColorByte>((value >> shift) & 0xFFu);
        }
    }

    // --- ScalarSerializer<Color> ---

    std::wstring ScalarSerializer<Color>::toWString(const Color& color)
    {
        std::wstring result{ k_colorMark };
        appendHex(result, color.red);
        appendHex(result, color.green);
        appendHex(result, color.blue);
        if (color.alpha != k_opaqueAlpha)
            appendHex(result, color.alpha);
        return result;
    }

    void ScalarSerializer<Color>::fromWString(std::wstring_view str, Color& color)
    {
        if (!str.starts_with(k_colorMark))
            return;
        const std::wstring_view digits = str.substr(1);
        if (digits.size() != k_opaqueDigits && digits.size() != k_translucentDigits)
            return;

        std::uint32_t value = 0u;
        for (const wchar_t digit : digits)
        {
            const std::optional<std::uint32_t> digitValue = digitValueOf(digit);
            if (!digitValue)
                return;
            value = (value << 4) | *digitValue;
        }

        if (digits.size() == k_opaqueDigits)
        {
            color = Color{
                byteAt(value, 16u),
                byteAt(value, 8u),
                byteAt(value, 0u),
                k_opaqueAlpha,
            };
            return;
        }
        color = Color{
            byteAt(value, 24u),
            byteAt(value, 16u),
            byteAt(value, 8u),
            byteAt(value, 0u),
        };
    }
}
