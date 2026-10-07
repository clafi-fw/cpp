export module SeeDocs_App.Scanner;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE READER. Every module unit under the tree is lexed with the framework's own
// C++ lexer and read declaration by declaration over the tokens: no macro is expanded, no
// template is followed, and a body is skipped - read only for the property binds written in it.
namespace SeeDocs_App
{
    // Reads the tree, as it stands - the interfaces first, then the implementation units.
    export [[nodiscard]] Surface scanTree(const std::filesystem::path& root);

    // Reads one file into a surface, placed under the tree's root. Answers whether it was read.
    export bool scanFile(Surface&, const std::filesystem::path& root,
        const std::filesystem::path& file);
}
