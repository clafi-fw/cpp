module Themes_App.Main;

import Themes_App.ThemeToCppCode;
import Themes_App.Consts;

import ClaFi.Application.ThemesManager;
import ClaFi.App.Application;
import ClaFi.App.ThemeIcon;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ClaFi;

    AppParams themesParams()
    {
        return {
            .name = Text{ L"Themes" },
            .publisher = L"ClaFi Framework",
            .description = Text{ L"Edits the themes other ClaFi applications use." },
            .version = CLAFI_APP_VERSION,
            .updates = UpdateSource{
                .repository = L"clafi-fw/cpp",
                .tagPrefix = L"Themes-v"
            }
        };
    }

    Dom::Dt::Section rootConfigBlueprint()
    {
        return {
            Dom::Dt::Value{ k_previewInAppAttrName, false },
            Dom::Dt::Value{ k_previewColorModeAttrName, ColorMode::Dark },
            Dom::Dt::Value{ k_codeContentAttrName, CodeContent::Full },
            Dom::Dt::Value{ k_codeScopeAttrName, CodeScope::ClassMethod }
        };
    }

    Dom::Dt::Section tabEntryBlueprint()
    {
        return {
            Dom::Dt::Value{ k_tabIconAttrName, ThemeIconColors{} }
        };
    }

    Dom::Dt::Section tabConfigBlueprint()
    {
        return {
            Dom::Dt::Value{ L"Preview", true },
            Dom::Dt::Value{ k_viewAttrName, ThemeView::Design },
            Dom::Dt::Value{ k_themeDataAttrName, AppTheme{} }
        };
    }
}
