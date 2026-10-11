export module SeeDocs_App.Icons;

import SeeDocs_App.Surface;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    // What a row of the surface tree stands for, which is what its icon draws - and the root
    // of the browser, which has no row.
    export enum class RowIcon
    {
        Home,      // the root, listing the projects
        Project,
        Options,   // the project's options page
        Folder,
        Module,
        Control,
        Class,
        Struct,
        Union,
        Enum,
        Alias,
        Concept,
        Search     // what holds the words a search was given
    };

    // How wide a row's icon stands, which a row with none leaves blank to line up with the rest.
    export constexpr float k_rowIconSize = 14.0f;

    // The icon of a type's row - a control's, whatever else the type is.
    export [[nodiscard]] RowIcon rowIconOf(const Type&);
    // The icon a row kind is marked with, sized for a tree row, for the head of the row's text.
    export [[nodiscard]] InTextIcon rowIcon(RowIcon);
    // The same mark sized for a page's title, for the head of the title's text.
    export [[nodiscard]] InTextIcon titleIcon(RowIcon);
    // Draws a row kind's mark into the icon rect of the event - a crumb's or a tab's slot.
    export void paintRowIcon(PaintIconEvent&, RowIcon);
    // What a tree row of a type ends with: its kind word at the row's far end; nothing for a
    // control, whose icon says what it is.
    export void writeRowKind(Text&, const Type&);
    // The application menu's Open page: an open folder, its front flap in the accent.
    export void paintOpenIcon(PaintIconEvent&);
    // The application menu's New project command: a folder with a spark at its corner.
    export void paintNewProjectIcon(PaintIconEvent&);
}
