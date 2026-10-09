export module SeeDocs_App.Scanner;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE READER. Every module unit under the tree is lexed with the framework's own
// C++ lexer and read declaration by declaration over the tokens: no template is followed, and a
// body is skipped - read only for the property binds written in it. A limited read takes headers
// as well, follows each conditional down its first branch and expands the tree's own macros.
namespace SeeDocs_App
{
    // Reads the tree, as it stands - the interfaces first, then the implementation units, or in
    // a limited read the headers.
    export [[nodiscard]] Surface scanTree(const std::filesystem::path& root, ScanMode);

    // Reads one file into a surface, placed under the tree's root, in full. Answers whether it
    // was read.
    export bool scanFile(Surface&, const std::filesystem::path& root,
        const std::filesystem::path& file);
}
