export module ClaFi.App.ApplicationProperties;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi
{
    // Which themes an application is offered. See Application
    export enum class ThemesOffered
    {
        StatingEveryElement, // those stating a set for every element the application names
        Every                // every theme, whatever it states - what a theme editor takes
    };

    // What an application states about itself. See Application
    export struct ApplicationProperties
    {
        Text name;
        std::wstring_view publisher;
        Text description{}; // what the application does, shown on its Information page
        std::wstring_view version{}; // major.minor.patch, shown on its Information page
        std::wstring_view site{}; // its own web address, shown as a link on its Information page
        UpdateSource updates{}; // where its Information page looks for a newer release
        // The sheet a text is built over where none is given; none is the framework's.
        const StyleSheet* styleSheet{ nullptr };
        // Its own elements, by name - the i-th worn as appElement(i). See Application
        AppElementNames elements{};
        // A theme file's text stating its own styles, which join the built-in theme at start.
        std::wstring_view builtInTheme{};
        ThemesOffered themesOffered{ ThemesOffered::StatingEveryElement };
    };
}
