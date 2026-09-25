export module ClaFi.App.Themes;

import ClaFi.Application.ThemesManager;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // The themes this application can wear, over its themes directory. See Application
    export [[nodiscard]] ThemesManager& appThemes();

    // Names the manager everything reaches through appThemes, and puts the application in the
    // theme its config states. Once per application. See Application
    export void connectAppThemes(AppContext&, ThemesManager&);

    // The theme the config names, and the default one where it names nothing that stands.
    // See Application
    export [[nodiscard]] const AppTheme& storedTheme(AppContext&);

    // Puts the application in this theme: the set every window is painted from, and the theme
    // itself for whatever asks what is worn rather than what it looks like. See Application
    export void applyTheme(AppContext&, const AppTheme&);

    // The same, in the mode given rather than the one the config states. See Application
    export void applyTheme(AppContext&, const AppTheme&, ColorMode);

    // The mode the config states, or the desktop's where it states Auto. See Application
    export [[nodiscard]] ColorMode wornColorMode(const AppContext&);

    // Puts the application in the theme the config names, crossing to it. See Application
    export void applyStoredTheme(AppContext&);

    // States this path in the config, which is what puts the application in that theme and keeps
    // it there between sessions. See Application
    export void applyThemePath(AppContext&, std::wstring_view path);

    // States this mode in the config, the way applyThemePath states a path. See Application
    export void applyColorMode(AppContext&, ColorModeSetting);
}
