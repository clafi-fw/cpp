module ClaFi.Documents.TextFile;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Documents
{
    namespace
    {
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

    std::optional<std::wstring> readTextFile(const std::filesystem::path& path)
    {
        std::ifstream file{ path, std::ios::binary };
        if (!file)
            return std::nullopt;

        using ByteIterator = std::istreambuf_iterator<char>;
        const std::string bytes{ ByteIterator{ file }, ByteIterator{} };
        std::wstring text = withoutCarriageReturns(fromUtf8(bytes));
        constexpr wchar_t k_byteOrderMark = L'\xFEFF';
        if (text.starts_with(k_byteOrderMark))
            text.erase(0ull, 1ull);
        return text;
    }

    bool writeTextFile(const std::filesystem::path& path, const std::wstring_view text)
    {
        const std::string bytes = toUtf8(withCarriageReturns(text));
        std::ofstream file{ path, std::ios::binary | std::ios::trunc };
        if (!file)
            return false;
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(file);
    }
}
