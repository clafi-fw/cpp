module PascalScripts_App.Main;

import ClaFiApps.StarterTheme;

import PascalScripts_App.Consts;
import PascalScripts_App.Language;

import ClaFi.Documents.Folder;

import ClaFi.App.WindowApplication;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace PascalScripts_App
{
    using namespace ClaFi;

    ApplicationProperties scriptsParams()
    {
        return {
            .name = Text{ k_appName },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Keeps Pascal scripts in a folder of its own and edits them, "
                L"with the source colored as it is typed."
            },
            .version = CLAFI_APP_VERSION,
            .updates = UpdateSource{
                .repository = L"clafi-fw/cpp",
                .tagPrefix = L"PascalScripts-v"
            },
            .defaultTheme = { L"Default", ClaFiApps::starterTheme() },
            .legacyThemeFileDefaults = &ClaFiApps::starterTheme(),
            .usePublisherThemes = true
        };
    }

    std::filesystem::path scriptsDirectory(const Language& language)
    {
        const std::wstring documents = Platform::documentsPath();
        if (documents.empty())
            return {};
        const std::filesystem::path primary = std::filesystem::path{ documents } / k_appName;
        // Existing scripts and restored tabs keep their original storage.
        std::error_code error;
        if (language.defaultStorages().empty() || std::filesystem::is_directory(primary, error)
            || std::ranges::find(language.defaultStorages(), primary, &Documents::StorageDefault::location)
                != language.defaultStorages().end())
        {
            return primary;
        }
        return {};
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
