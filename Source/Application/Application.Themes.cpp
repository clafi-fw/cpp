module ClaFi.App.Themes;

import ClaFi.Application.ThemesManager;
import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.DomEngine;
// Dom::Value<std::wstring> picks its specialization off the scalar serializer declared here, so
// without this import the theme node is an incomplete type wherever it is read.
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // The one manager an application makes, so that a page reaches it without being handed it.
    // Null until connectAppThemes has run, which ApplicationBase does before any window exists.
    ThemesManager*& mutableAppThemes();

    // appThemes

    ThemesManager*& mutableAppThemes()
    {
        static ThemesManager* manager{ nullptr };
        return manager;
    }

    ThemesManager& appThemes()
    {
        if (!mutableAppThemes())
            unreachable("appThemes: no themes manager - connectAppThemes has not run.");
        return *mutableAppThemes();
    }

    // THE NODES ARE THE SETTING, AND THIS IS WHAT READS THEM. Every route that changes the theme
    // states a path or a mode - the Settings page, an application of its own - and the node
    // carries the change here, so a stored theme and a chosen one travel one path. Under Auto the
    // desktop's mode is part of the setting, so a change the desktop announces is read here too.
    //
    // Connected once and never taken down: the nodes outlive every window, and an application
    // that has stopped being able to wear a theme has stopped running.
    void connectAppThemes(AppContext& appContext, ThemesManager& manager)
    {
        mutableAppThemes() = &manager;
        static EventConnection themeChanged = appContext.themePath().connectEvent(
            [&appContext](Dom::ChangeEvent&) {
                applyStoredTheme(appContext);
            });
        static EventConnection modeChanged = appContext.colorMode().connectEvent(
            [&appContext](Dom::ChangeEvent&) {
                applyStoredTheme(appContext);
            });
        static EventConnection systemChanged = Platform::events().connect<SystemColorModeEvent>(
            [&appContext](SystemColorModeEvent&) {
                if (appContext.colorMode().get() == ColorModeSetting::Auto)
                    applyStoredTheme(appContext);
            });
        applyStoredTheme(appContext);
    }

    // A PATH NAMING NOTHING IS NOT AN APPLICATION WITHOUT A THEME. A user theme can be renamed or
    // deleted between two sessions, and the config still holds the path it had, so the built-in
    // the framework can always answer for stands in its place. The node is left as it is - the
    // theme may be back the next time the application runs.
    const AppTheme& storedTheme(AppContext& appContext)
    {
        const std::wstring path = appContext.themePath().get();
        if (const AppTheme* theme = appThemes().themeByPath(path))
            return *theme;
        return defaultTheme();
    }

    void applyStoredTheme(AppContext& appContext)
    {
        applyTheme(appContext, storedTheme(appContext));
    }

    void applyTheme(AppContext& appContext, const AppTheme& theme)
    {
        applyTheme(appContext, theme, wornColorMode(appContext));
    }

    // Baked here rather than in the core: what a slot and a palette hue stand for is the theme's
    // to say, and saying it is what leaves a set with a half way in every value.
    void applyTheme(AppContext& appContext, const AppTheme& theme, ColorMode mode)
    {
        appContext.takeTheme(theme, bake(theme.colors, mode));
    }

    void applyThemePath(AppContext& appContext, const std::wstring_view path)
    {
        appContext.themePath().set(std::wstring{ path });
    }

    ColorMode wornColorMode(const AppContext& appContext)
    {
        switch (appContext.colorMode().get())
        {
        case ColorModeSetting::Dark:
            return ColorMode::Dark;
        case ColorModeSetting::Light:
            return ColorMode::Light;
        case ColorModeSetting::Auto:
            break;
        }
        return Platform::systemColorMode();
    }

    void applyColorMode(AppContext& appContext, ColorModeSetting setting)
    {
        appContext.colorMode().set(setting);
    }
}
