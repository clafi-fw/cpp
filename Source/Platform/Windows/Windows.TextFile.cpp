module;
#include "Windows.Headers.h"

module ClaFi.Documents.TextFile;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    [[nodiscard]] std::optional<std::wstring> fromWindowsAnsi(const std::string_view bytes)
    {
        if (bytes.empty())
        {
            return std::wstring{};
        }
        if (!std::in_range<int>(bytes.size()))
        {
            return std::nullopt;
        }
        const int byteCount = static_cast<int>(bytes.size());
        const int length = ::MultiByteToWideChar(CP_ACP, 0, bytes.data(), byteCount, nullptr, 0);
        if (length == 0)
        {
            return std::nullopt;
        }
        std::wstring text(static_cast<std::size_t>(length), L'\0');
        if (::MultiByteToWideChar(CP_ACP, 0, bytes.data(), byteCount, text.data(), length) != length)
        {
            return std::nullopt;
        }
        return text;
    }
}
