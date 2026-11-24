export module ClaFi.App.ApplicationProperties;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi
{
    // Which themes an application is offered. See Application
    export enum class ThemesOffered
    {
        StatingEveryElement, // publisher themes stating a style for each application element
        Every                // all readable publisher themes
    };

    export struct NamedTheme
    {
        std::wstring name{};
        AppTheme theme{};
    };

    export using NamedThemes = std::vector<NamedTheme>;

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
        NamedTheme defaultTheme{};
        NamedThemes additionalThemes{};
        const AppTheme* legacyThemeFileDefaults{ nullptr }; // seed for sparse theme files
        ThemesOffered themesOffered{ ThemesOffered::StatingEveryElement };
        std::wstring_view linkScheme{}; // the scheme the framework registers for this application
        LinkHandler linkHandler{}; // handles application links outside the framework's routes
        bool usePublisherThemes{ false };
    };
}
