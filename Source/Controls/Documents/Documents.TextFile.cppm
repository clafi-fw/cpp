export module ClaFi.Documents.TextFile;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    // A text file as the framework holds text: read as UTF-8, a leading byte order mark dropped,
    // and CRLF folded to the LF the framework cuts lines on. Nothing where the file cannot be read.
    export [[nodiscard]] std::optional<std::wstring> readTextFile(const std::filesystem::path&);
    // Writes text as UTF-8 with CRLF line ends, and answers whether it got there.
    export bool writeTextFile(const std::filesystem::path&, std::wstring_view text);
}
