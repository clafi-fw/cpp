export module SeeDocs_App.Scanner;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE READER. Every module unit in the folders of sources is lexed with the framework's own
// C++ lexer and read declaration by declaration over the tokens: no template is followed, and a
// body is skipped - read only for the property binds written in it. A limited read takes headers
// as well, follows each conditional down its first branch and expands the sources' own macros.
namespace SeeDocs_App
{
    // Reads the folders as they stand, into one surface - the interfaces first, then the
    // implementation units, or in a limited read the headers - a file under the folder of
    // its place in its source, so that a folder of one name under two sources is one.
    export [[nodiscard]] Surface scanSources(const SourceFolders&, ScanMode, HintRules);
}
