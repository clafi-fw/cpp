export module Themes_App.Consts;

import ClaFi.Application.ThemesManager;
import ClaFi.Browser;
import ClaFi;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ClaFi;

    // The built-in's page, named as its theme is: a document page with no file of its own.
    export constexpr std::wstring_view k_defaultPageName = k_defaultThemeName;

    // One toggle for the whole application rather than one per tab, so it is named in the root
    // config section and every page reads the same node.
    export constexpr std::wstring_view k_previewInAppAttrName{ L"PreviewInApp" };
    // The mode every preview shows a theme in, application wide for the same reason.
    export constexpr std::wstring_view k_previewColorModeAttrName{ L"PreviewColorMode" };

    // The two choices a code page offers, application wide for the same reason: one answer for
    // every theme tab, so they are named in the root config section too.
    export constexpr std::wstring_view k_codeContentAttrName{ L"CodeContent" };
    export constexpr std::wstring_view k_codeScopeAttrName{ L"CodeScope" };

    // The view a tab's theme page shows - see ThemePage::storeView.
    export constexpr std::wstring_view k_viewAttrName{ L"View" };

    // The token the Palette page goes by.
    export constexpr std::wstring_view k_paletteToken{ L"Palette" };

    // **The application's own two elements** - test subjects nothing in the framework wears, for
    // trying rules out on a bare control. Only the rules a theme states for one reach it.
    export constexpr std::wstring_view k_testeeName{ L"Testee" };
    export constexpr std::wstring_view k_besteeName{ L"Bestee" };
    export constexpr std::array k_appElements{ k_testeeName, k_besteeName };
    export constexpr UiElement k_testee{ appElement(0) };
    export constexpr UiElement k_bestee{ appElement(1) };

    // The two styles the built-in theme states for them, each a button in size with no rules.
    export constexpr std::wstring_view k_builtInStyles{ LR"(@ClaFi 1.0
@encoding utf-8

Styles=[
=[
Name=Testee
Base=AnyNonWindow
StandsOn=Section
Metrics=[
Border=Thin
Radius=4
Padding=8 4
MinSize=40 24
]
]
=[
Name=Bestee
Base=AnyNonWindow
StandsOn=Section
Metrics=[
Border=Thin
Radius=4
Padding=8 4
MinSize=40 24
]
]
]
)" };

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
