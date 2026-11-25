module ClaFi.Documents.TextFile;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Documents
{
    namespace
    {
        constexpr std::string_view k_utf8Bom = "\xEF\xBB\xBF";

        [[nodiscard]] std::wstring fromWindows1251(const std::string_view bytes)
        {
            // Bytes 80-BF in code-page order. C0-FF run from Cyrillic capital A to small Ya.
            constexpr std::wstring_view k_symbols =
                L"\u0402\u0403\u201A\u0453\u201E\u2026\u2020\u2021\u20AC\u2030\u0409\u2039\u040A\u040C\u040B\u040F"
                L"\u0452\u2018\u2019\u201C\u201D\u2022\u2013\u2014\uFFFD\u2122\u0459\u203A\u045A\u045C\u045B\u045F"
                L"\u00A0\u040E\u045E\u0408\u00A4\u0490\u00A6\u00A7\u0401\u00A9\u0404\u00AB\u00AC\u00AD\u00AE\u0407"
                L"\u00B0\u00B1\u0406\u0456\u0491\u00B5\u00B6\u00B7\u0451\u2116\u0454\u00BB\u0458\u0405\u0455\u0457";
            std::wstring text;
            text.reserve(bytes.size());
            for (const unsigned char byte : bytes)
            {
                if (byte < 0x80)
                {
                    text.push_back(static_cast<wchar_t>(byte));
                }
                else if (byte < 0xC0)
                {
                    text.push_back(k_symbols[byte - 0x80]);
                }
                else
                {
                    text.push_back(static_cast<wchar_t>(L'\u0410' + byte - 0xC0));
                }
            }
            return text;
        }

        [[nodiscard]] std::wstring withoutCarriageReturns(const std::wstring_view text)
        {
            std::wstring result{};
            result.reserve(text.size());
            for (std::size_t i = 0ull; i != text.size(); ++i)
            {
                const bool foldedEnd = text[i] == L'\r'
                    && i + 1ull != text.size()
                    && text[i + 1ull] == L'\n';
                if (!foldedEnd)
                    result.push_back(text[i]);
            }
            return result;
        }

        [[nodiscard]] std::wstring withCarriageReturns(const std::wstring_view text)
        {
            std::wstring result{};
            const std::size_t lineEnds = static_cast<std::size_t>(std::ranges::count(text, L'\n'));
            result.reserve(text.size() + lineEnds);
            for (const wchar_t unit : text)
            {
                if (unit == L'\n')
                    result.push_back(L'\r');
                result.push_back(unit);
            }
            return result;
        }
    }

    std::optional<std::wstring> readTextFile(const std::filesystem::path& path,
        const TextFileFallback fallback)
    {
        std::ifstream file{ path, std::ios::binary };
        if (!file)
            return std::nullopt;

        using ByteIterator = std::istreambuf_iterator<char>;
        const std::string bytes{ ByteIterator{ file }, ByteIterator{} };
        const std::string_view body{ bytes };
        if (body.starts_with(k_utf8Bom))
        {
            return withoutCarriageReturns(fromUtf8(body.substr(k_utf8Bom.size())));
        }
        std::wstring text = fromUtf8(body);
        if (fallback == TextFileFallback::Windows1251 && toUtf8(text) != body)
        {
            text = fromWindows1251(body);
        }
        return withoutCarriageReturns(text);
    }

    bool writeTextFile(const std::filesystem::path& path, const std::wstring_view text, const Utf8Bom bom)
    {
        const std::string bytes = toUtf8(withCarriageReturns(text));
        std::ofstream file{ path, std::ios::binary | std::ios::trunc };
        if (!file)
            return false;
        if (bom == Utf8Bom::Write)
        {
            file.write(k_utf8Bom.data(), static_cast<std::streamsize>(k_utf8Bom.size()));
        }
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        file.close();
        return static_cast<bool>(file);
    }
}
