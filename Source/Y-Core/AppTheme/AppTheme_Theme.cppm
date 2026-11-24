export module ClaFi.Core.AppTheme_Theme;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Styles;

import ClaFi.StdLib;

namespace ClaFi
{
    // A theme: its palette, and the style every element wears. See AppTheme
    export class AppTheme
    {
    public:
        // The styles as a document field reads and writes them.
        [[nodiscard]] const ControlStyleList& styleList() const { return styles.list; }
        void setStyleList(const ControlStyleList& value) { styles.list = value; }
    public:
        ThemeColors colors{};
        ThemeStyles styles{};
    };

    // The roots a theme path stands under, and what parts a root from the name. See AppTheme
    export namespace ThemeRoots
    {
        constexpr std::wstring_view App{ L"App" };
        constexpr std::wstring_view User{ L"User" };
        constexpr std::wstring_view AppConfig{ L"AppConfig" };
        constexpr std::wstring_view AppExecutable{ L"AppExecutable" };
        constexpr wchar_t Separator{ L'/' };
    }

    // The Settings path of the default theme declared by the application.
    export constexpr std::wstring_view k_defaultThemePath{ L"App/Default" };

    // A path out of the root a theme stands under and the name it stands there by. See AppTheme
    export [[nodiscard]] std::wstring themePathOf(std::wstring_view root, std::wstring_view name);
    // The root a path stands under, and nothing where the path names none.
    export [[nodiscard]] constexpr std::wstring_view themePathRoot(std::wstring_view path);
    // The name a path stands by under its root, and the whole of it where it names no root.
    export [[nodiscard]] constexpr std::wstring_view themePathName(std::wstring_view path);
    //-----------------------------------------------------------------------------


    std::wstring themePathOf(const std::wstring_view root, const std::wstring_view name)
    {
        std::wstring result{};
        result.reserve(root.size() + 1ull + name.size());
        result.append(root);
        result.push_back(ThemeRoots::Separator);
        result.append(name);
        return result;
    }

    constexpr std::wstring_view themePathRoot(const std::wstring_view path)
    {
        const std::size_t separator = path.find(ThemeRoots::Separator);
        if (separator == std::wstring_view::npos)
            return {};
        return path.substr(0ull, separator);
    }

    constexpr std::wstring_view themePathName(const std::wstring_view path)
    {
        const std::size_t separator = path.find(ThemeRoots::Separator);
        if (separator == std::wstring_view::npos)
            return path;
        return path.substr(separator + 1ull);
    }


}
