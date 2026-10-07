export module SeeDocs_App.Studio.Icons;

import SeeDocs_App.Surface;

import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    // What a row of the surface tree stands for, which is what its icon draws.
    export enum class RowIcon
    {
        Chapter,
        Module,
        Control,
        Class,
        Struct,
        Union,
        Enum,
        Alias,
        Concept
    };

    // The icon of a type's row - a control's, whatever else the type is.
    export [[nodiscard]] RowIcon rowIconOf(const Type&);
    // The icon a row kind is marked with, sized for a tree row, for the head of the row's text.
    export [[nodiscard]] InTextIcon rowIcon(RowIcon);
}
