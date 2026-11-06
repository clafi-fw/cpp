export module Themes_App.PageKinds;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;

    // What a page of the design holds, which its icon draws and its kind word names.
    export enum class PageKind
    {
        Palette, // the theme-wide settings
        Folder,  // a root or a category, its page listing what it holds
        Style    // a control style: its colour rules and its sizes
    };

    export constexpr float k_titleIconSize = 20.0f; // beside a title's SubTitle words
    export constexpr float k_titleIconGap = 8.0f;   // between a title's icon and its words

    // Draws a page kind's icon into the icon rect of the event.
    export void drawPageIcon(DrawIconEvent&, PageKind);
    // A page kind's icon at the stated size, for the head of a row's or a title's text.
    export [[nodiscard]] InTextIcon pageIcon(PageKind, float size);
    // The word a row or a title of the kind ends with, and nothing for a kind that has none.
    export [[nodiscard]] std::wstring_view kindWordOf(PageKind);
    // Writes a page's title: its kind's icon, its name in SubTitle and its kind word, muted.
    export void writePageTitle(Text&, PageKind, std::wstring_view name);
}
