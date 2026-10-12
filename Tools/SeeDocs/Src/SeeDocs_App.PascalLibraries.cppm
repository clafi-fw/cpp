export module SeeDocs_App.PascalLibraries;

import ClaFi.StdLib;

// **What SeeDocs knows of the libraries a Delphi tree builds on** - the VCL, FMX and the LCL - for
// a tree that does not hold their sources. See README
namespace SeeDocs_App
{
    // The library a unit's class names are read in, by the units it uses.
    export enum class ClassLibrary
    {
        Vcl,          // the VCL's, and the LCL's under a name the VCL leaves free
        FireMonkey    // FMX's, in a unit that uses one of its units
    };

    // Whether the unit is one of FireMonkey's - FMX.Controls, FMX.Types.
    export [[nodiscard]] bool isFireMonkeyUnit(std::wstring_view unit);
    // Whether the name is TControl in any case, the class every library control derives from.
    export [[nodiscard]] bool isControlRoot(std::wstring_view name);
    // Whether a class the library declares descends from TControl, its name read in any case.
    export [[nodiscard]] bool isLibraryControl(std::wstring_view name, ClassLibrary);
}
