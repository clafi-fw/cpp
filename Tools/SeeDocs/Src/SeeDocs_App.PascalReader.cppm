export module SeeDocs_App.PascalReader;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// THE DELPHI READER. A unit's interface section is what the unit publishes, read declaration by
// declaration over the framework's own Pascal lexer: its types with their members, its routines,
// constants and variables. A unit is a module, and its name is the namespace its declarations
// are named in, written the C++ way. See README
namespace SeeDocs_App
{
    // The conditional symbols a tree defines for itself, in lower case. A conditional on a
    // symbol neither the tree nor the compiler defines is read false; any other reads its first
    // branch. See README
    export using PascalSymbols = std::set<std::wstring>;

    // Adds the symbols a unit or an include file defines - {$DEFINE X}.
    export void gatherPascalSymbols(std::wstring_view text, PascalSymbols&);

    // Reads one unit into the surface, the file being the surface's own already.
    export void readPascalUnit(Surface&, std::size_t file, std::wstring_view text,
        const std::wstring& category, const PascalSymbols&);

    // Settles what the units say of each other once all are read: a property declared again
    // with no type takes the one it redeclares, and a property of a method type is an event.
    export void settlePascal(Surface&);
}
