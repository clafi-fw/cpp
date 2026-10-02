module ThisApp.Main;

import ThisApp.Consts;

import ClaFi.App.Application;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    AppParams scriptsParams()
    {
        return {
            .name = Text{ k_appName },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Keeps Pascal scripts in a folder of its own and edits them, "
                L"with the source coloured as it is typed."
            },
            .version = CLAFI_APP_VERSION
        };
    }

    std::filesystem::path scriptsDirectory()
    {
        const std::wstring documents = Platform::documentsPath();
        if (documents.empty())
            return {};
        return std::filesystem::path{ documents } / k_appName;
    }

    Dom::Dt::Section rootConfigBlueprint()
    {
        return {};
    }

    Dom::Dt::Section tabEntryBlueprint()
    {
        return {};
    }

    Dom::Dt::Section tabConfigBlueprint()
    {
        return {
            Dom::Dt::Value{ k_scriptKind.attrName, std::wstring{} }
        };
    }
}
