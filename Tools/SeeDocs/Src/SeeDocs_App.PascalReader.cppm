export module SeeDocs_App.PascalReader;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// **The Delphi reader.** A unit's interface section is what the unit publishes, read declaration by
// declaration over the framework's own Pascal lexer: its types with their members, its routines,
// constants and variables. By loose rules its implementation section gives a routine the hint
// written under its header. A unit is a module, and its name is the namespace its declarations
// are named in, written the C++ way. See README
namespace SeeDocs_App
{
    // The conditional symbols a tree defines for itself, in lower case. A conditional on a
    // symbol neither the tree nor the compiler defines is read false; any other reads its first
    // branch. See README
    export using PascalSymbols = std::set<std::wstring>;

    // The symbols the units and include files of a tree define - {$DEFINE X} in a branch that
    // reads. A conditional reads by what any of them defines, so the gathering goes over them
    // until the symbols settle.
    export [[nodiscard]] PascalSymbols pascalSymbolsOf(std::span<const std::wstring_view> texts);
    // Whether the symbols are gathered from the text - whether it holds a {$DEFINE}.
    export [[nodiscard]] bool definesSymbols(std::wstring_view text);

    // Reads one unit into the surface, the file being the surface's own already, its hints by
    // the rules given.
    export void readPascalUnit(Surface&, std::size_t file, std::wstring_view text,
        const std::wstring& folder, const PascalSymbols&, HintRules);

    // Settles what the units say of each other once all are read: a property declared again
    // with no type takes the one it redeclares, and a property of a method type is an event.
    export void settlePascal(Surface&);
}
