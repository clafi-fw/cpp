export module ClaFi.Core.AppTheme_Theme;

import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.StdLib;

namespace ClaFi
{
    export class AppTheme
    {
    public:
        ThemeMetrics metrics{};
        ThemeColors colors{};
    };

    // The roots a theme path stands under, and what parts a root from the name. See AppTheme
    export namespace ThemeRoots
    {
        constexpr std::wstring_view builtIn{ L"BuiltIn" };
        constexpr std::wstring_view user{ L"User" };
        constexpr wchar_t separator{ L'/' };
    }

    // The themes compiled into the framework, by the name they stand under. See AppTheme
    export namespace BuiltInThemes
    {
        constexpr std::wstring_view defaultTheme{ L"Default" };
    }

    // ThemeRoots::builtIn over BuiltInThemes::defaultTheme, which is what an application wears
    // where nothing else is stated. See AppTheme
    export constexpr std::wstring_view k_defaultThemePath{ L"BuiltIn/Default" };

    // A path out of the root a theme stands under and the name it stands there by. See AppTheme
    export [[nodiscard]] std::wstring themePathOf(std::wstring_view root, std::wstring_view name);
    // The root a path stands under, and nothing where the path names none.
    export [[nodiscard]] constexpr std::wstring_view themePathRoot(std::wstring_view path);
    // The name a path stands by under its root, and the whole of it where it names no root.
    export [[nodiscard]] constexpr std::wstring_view themePathName(std::wstring_view path);
    // The compiled-in theme of this name, or nullptr where no built-in goes by it.
    export [[nodiscard]] AppTheme* builtInTheme(std::wstring_view name);

    // The theme compiled into the framework, which BuiltInThemes::defaultTheme names.
    export [[nodiscard]] AppTheme& defaultTheme();


    //-----------------------------------------------------------------------------


    std::wstring themePathOf(const std::wstring_view root, const std::wstring_view name)
    {
        std::wstring result{};
        result.reserve(root.size() + 1ull + name.size());
        result.append(root);
        result.push_back(ThemeRoots::separator);
        result.append(name);
        return result;
    }

    constexpr std::wstring_view themePathRoot(const std::wstring_view path)
    {
        const std::size_t separator = path.find(ThemeRoots::separator);
        if (separator == std::wstring_view::npos)
            return {};
        return path.substr(0ull, separator);
    }

    constexpr std::wstring_view themePathName(const std::wstring_view path)
    {
        const std::size_t separator = path.find(ThemeRoots::separator);
        if (separator == std::wstring_view::npos)
            return path;
        return path.substr(separator + 1ull);
    }

    AppTheme* builtInTheme(const std::wstring_view name)
    {
        if (name == BuiltInThemes::defaultTheme)
            return &defaultTheme();
        return nullptr;
    }

    AppTheme& defaultTheme()
    {
        static AppTheme theme{};
        return theme;
    }

}
