export module Themes_App.Consts;

import ClaFi.Browser;
import ClaFi;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ClaFi;

    // One toggle for the whole application rather than one per tab, so it is named in the root
    // config section and every page reads the same node.
    export constexpr std::wstring_view k_previewInAppAttrName{ L"PreviewInApp" };
    // The mode every preview shows a theme in, application wide for the same reason.
    export constexpr std::wstring_view k_previewColorModeAttrName{ L"PreviewColorMode" };

    // The amount of theme data shown on the file format tabs.
    export constexpr std::wstring_view k_codeContentAttrName{ L"CodeContent" };

    // The view a tab's theme page shows - see ThemePage::storeView.
    export constexpr std::wstring_view k_viewAttrName{ L"View" };

    // The token the Palette page goes by.
    export constexpr std::wstring_view k_paletteToken{ L"Palette" };

    export constexpr std::wstring_view k_themesLinkScheme{ L"themes" };
    export constexpr std::wstring_view k_stylePrefix{ L"Style/" };

    export constexpr std::wstring styleAnchor(const std::wstring_view name)
    {
        std::wstring anchor{ k_stylePrefix };
        for (const wchar_t character : name)
        {
            if (character == L'%')
                anchor.append(L"%25");
            else
                anchor.push_back(character);
        }
        return anchor;
    }

    // **The application's own two elements** - test subjects nothing in the framework wears, for
    // trying rules out on a bare control. Only the rules a theme states for one reach it.
    export constexpr std::wstring_view k_testeeName{ L"Testee" };
    export constexpr std::wstring_view k_besteeName{ L"Bestee" };
    export constexpr std::array k_appElements{ k_testeeName, k_besteeName };
    export constexpr UiElement k_testee{ appElement(0) };
    export constexpr UiElement k_bestee{ appElement(1) };

    // What a tab's icon is drawn from, kept in the tab's entry so that an unopened tab reads no
    // file for it - see ThemesBrowser::drawTabIcon.
    export constexpr std::wstring_view k_tabIconAttrName{ L"Icon" };

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

    // The views a theme page switches between, one tab each.
    export enum class ThemeView
    {
        Design,
        Cpp,
        ClaFi,
        Xml,
        Json,
        Count
    };

    // The names a tab's config uses, spelled to match the enumerators.
    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(ThemeView::Count)>
        k_themeViewKeys{
            L"Design",
            L"Cpp",
            L"ClaFi",
            L"Xml",
            L"Json"
        };

    export constexpr auto enumNames(ThemeView) { return k_themeViewKeys; }

}
