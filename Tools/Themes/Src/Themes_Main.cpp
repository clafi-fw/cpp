module ThisApp.Main;

import ThisApp.ThemeToCppCode;
import ThisApp.Consts;

import ClaFi.Application.ThemesManager;
import ClaFi.App.Application;
import ClaFi.App.ThemeIcon;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    AppParams themesParams()
    {
        return {
            .name = Text{ L"Themes" },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Makes and edits the color themes ClaFi applications wear, "
                L"and previews them live. Every theme also reads as C++, ClaFi, XML and JSON.",
                TextOp::EndLine,
                TextOp::EndLine,
                L"The UI is terrible now - that long grid of sliders is unbearable, and it will be "
                L"remade, as well as the internal data structures and file formats. "
                L"But it serves great as a showcase of grids, expanders and in-place editors."
            }
        };
    }

    Dom::Dt::Section rootConfigBlueprint()
    {
        return {
            Dom::Dt::Value{ k_previewInAppAttrName, false },
            Dom::Dt::Value{ k_previewColorModeAttrName, ColorMode::Dark },
            Dom::Dt::Value{ k_codeContentAttrName, CodeContent::Full },
            Dom::Dt::Value{ k_codeScopeAttrName, CodeScope::ClassDeclarations }
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
            Dom::Dt::Sequence{ k_collapsedAttrName, std::wstring{} },
            Dom::Dt::Value{ k_themeDataAttrName, AppTheme{} }
        };
    }
}
