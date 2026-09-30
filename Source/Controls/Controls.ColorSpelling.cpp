module ClaFi.Controls.ColorSpelling;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        constexpr std::wstring_view k_hexDigits = L"0123456789ABCDEF";

        // Which channel a run of digits belongs to.
        enum class Channel
        {
            Red,
            Green,
            Blue
        };

        [[nodiscard]] ColorByte valueOf(const Color color, const Channel channel)
        {
            switch (channel)
            {
                case Channel::Red:
                    return color.red;
                case Channel::Green:
                    return color.green;
                default:
                    return color.blue;
            }
        }

        // The theme's pigment for the channel, so the digits follow the theme.
        [[nodiscard]] InkColor inkOf(const Channel channel)
        {
            switch (channel)
            {
                case Channel::Red:
                    return InkColor::Red;
                case Channel::Green:
                    return InkColor::Green;
                default:
                    return InkColor::Blue;
            }
        }

        void appendHex(Text& text, const ColorByte value)
        {
            text << k_hexDigits[value >> 4];
            text << k_hexDigits[value & 0x0f];
        }

        void appendChannel(Text& text, const Color color, const Channel channel, const bool hex)
        {
            const ColorByte value = valueOf(color, channel);
            text << inkOf(channel);
            if (hex)
                appendHex(text, value);
            else
                text << static_cast<int>(value);
            text << PopColor{};
        }
    }

    std::wstring spell(const Color color, const ColorSpelling spelling)
    {
        switch (spelling)
        {
            case ColorSpelling::HexRgb:
                return std::format(L"{:02X}{:02X}{:02X}", color.red, color.green, color.blue);
            case ColorSpelling::HexBgr:
                return std::format(L"{:02X}{:02X}{:02X}", color.blue, color.green, color.red);
            case ColorSpelling::DecimalRgb:
                return std::format(L"{} {} {}", color.red, color.green, color.blue);
            case ColorSpelling::CssHex:
                return std::format(L"#{:02X}{:02X}{:02X}", color.red, color.green, color.blue);
        }
        unreachable("a colour spelling with no writer of its own");
    }

    std::wstring_view nameOf(const ColorSpelling spelling)
    {
        switch (spelling)
        {
            case ColorSpelling::HexRgb:
                return L"RGB hex";
            case ColorSpelling::HexBgr:
                return L"BGR hex";
            case ColorSpelling::DecimalRgb:
                return L"RGB decimal";
            case ColorSpelling::CssHex:
                return L"CSS";
        }
        unreachable("a colour spelling with no name of its own");
    }

    void appendSpelling(Text& text, const Color color, const ColorSpelling spelling)
    {
        switch (spelling)
        {
            case ColorSpelling::HexRgb:
                appendChannel(text, color, Channel::Red, true);
                appendChannel(text, color, Channel::Green, true);
                appendChannel(text, color, Channel::Blue, true);
                break;
            case ColorSpelling::HexBgr:
                appendChannel(text, color, Channel::Blue, true);
                appendChannel(text, color, Channel::Green, true);
                appendChannel(text, color, Channel::Red, true);
                break;
            case ColorSpelling::DecimalRgb:
                appendChannel(text, color, Channel::Red, false);
                text << L' ';
                appendChannel(text, color, Channel::Green, false);
                text << L' ';
                appendChannel(text, color, Channel::Blue, false);
                break;
            case ColorSpelling::CssHex:
                text << InkGrade::Muted << L'#' << PopColor{};
                appendChannel(text, color, Channel::Red, true);
                appendChannel(text, color, Channel::Green, true);
                appendChannel(text, color, Channel::Blue, true);
                break;
        }
    }
}
