export module ClaFi.Documents.TextFile;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    // The encoding to try when an unmarked file contains invalid UTF-8.
    export enum class TextFileFallback
    {
        None,
        Windows1251,
#ifdef _WIN32
        WindowsAnsi,   // The active Windows ANSI code page.
#endif
    };

    // Whether a UTF-8 file starts with the three-byte encoding signature.
    export enum class Utf8Bom
    {
        Omit,
        Write
    };

    // Reads UTF-8, strips its signature and folds CRLF to LF.
    export [[nodiscard]] std::optional<std::wstring> readTextFile(const std::filesystem::path&,
        TextFileFallback = TextFileFallback::None);
    // Writes UTF-8 with CRLF line endings.
    export bool writeTextFile(const std::filesystem::path&, std::wstring_view text,
        Utf8Bom = Utf8Bom::Omit);
}
