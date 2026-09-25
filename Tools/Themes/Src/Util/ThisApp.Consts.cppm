export module ThisApp.Consts;

import ClaFi.Application.ThemesManager;
import ClaFi.Browser;
import ClaFi;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    export constexpr TagValue k_homePageTag = 1ull;
    export constexpr TagValue k_themePageTag = 2ull;

    // used for both, user and built-in themes
    export constexpr std::wstring_view k_themePathSuffix = L"theme";

    export constexpr std::wstring_view k_defaultPageName = k_defaultThemeName;
    export constexpr std::wstring_view k_dark2PageName = k_dark2ThemeName;

    export constexpr std::wstring_view k_defaultPageTitle = BuiltInThemes::defaultTheme;
    export constexpr std::wstring_view k_dark2PageTitle = L"Not So Dark";

    // A built-in page's name without the extension, which is what a user types and what a theme
    // file is named by.
    constexpr std::wstring_view stemOf(const std::wstring_view themeName)
    {
        return themeName.substr(0, themeName.size() - k_builtInThemeExtension.size());
    }

    // THE NAMES A USER THEME MAY NOT TAKE: every built-in the application lists, without its
    // extension. ThemesManager answers these from the compiled-in colours whatever stands on the
    // disk, so a file of that name would never be read and the theme in it could never be opened.
    export constexpr std::array<std::wstring_view, 1> k_reservedThemeNames{
        stemOf(k_defaultPageName)
    };

    export constexpr std::wstring_view k_defaultPagePath = L"/Default.theme";
    export constexpr std::wstring_view k_dark2PagePath = L"/Dark2.theme";

    // One toggle for the whole application rather than one per tab, so it is named in the root
    // config section and every page reads the same node.
    export constexpr std::wstring_view k_previewInAppAttrName{ L"PreviewInApp" };
    // The mode every preview shows a theme in, application wide for the same reason.
    export constexpr std::wstring_view k_previewColorModeAttrName{ L"PreviewColorMode" };

    // The two choices a code page offers, application wide for the same reason: one answer for
    // every theme tab, so they are named in the root config section too.
    export constexpr std::wstring_view k_codeContentAttrName{ L"CodeContent" };
    export constexpr std::wstring_view k_codeScopeAttrName{ L"CodeScope" };

    // The folds a tab's design page has closed, one name each - see ThemePage::storeFolds.
    export constexpr std::wstring_view k_collapsedAttrName{ L"Collapsed" };

    // What a tab's icon is drawn from, kept in the tab's entry so that an unopened tab reads no
    // file for it - see ThemesBrowser::paintTabIcon.
    export constexpr std::wstring_view k_tabIconAttrName{ L"Icon" };

    export enum class ColumnTag : TagValue
    {
        StaticName,
            Name,
            State,
        Hue,
        SaturationGroup,
            SaturationAction,
            SaturationAmount,
        ElevationGroup,
            ElevationOperation,
            ElevationAmount,
        ElevationFlip,
    };

    export enum class RowTag : TagValue
    {
        Group,
        StaticElement,
        DynamicElement
    };

    // The five hue operations as the editor names them, indexed by ColorRuleHueOp. The names the
    // serializers use are a separate set, spelled to match the enumerators; these are labels and
    // are free to read as such.
    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(ColorRuleHueOp::Count)> k_hueOpNames{
        L"Palette 1",
        L"Palette 2",
        L"Palette 3",
        L"No change",
        L"Custom hue"
    };

    export enum class ComboboxTarget
    {
        Saturation,
        Elevation
    };

}
