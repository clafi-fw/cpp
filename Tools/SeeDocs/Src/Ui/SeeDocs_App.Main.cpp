module SeeDocs_App.Main;

import SeeDocs_App.Browser;
import SeeDocs_App.OpenPage;
import SeeDocs_App.Workspace;

import ClaFi.App.Settings;

import ClaFi.App.Application;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    namespace
    {
        constexpr std::wstring_view k_appName = L"SeeDocs";
        constexpr std::wstring_view k_publisher = L"ClaFi Framework";
        constexpr std::wstring_view k_openPageCaption = L"Open";
    }

    AppParams seeDocsParams()
    {
        return {
            .name = Text{ k_appName },
            .publisher = k_publisher,
            .description = Text{
                L"Shows the design surface the scanner read out of the tree, "
                L"as the documentation it is to become."
            }
        };
    }

    Dom::Dt::Section rootConfigBlueprint()
    {
        return Workspace::createConfigSchema();
    }

    Dom::Dt::Section tabEntryBlueprint()
    {
        return {};
    }

    Dom::Dt::Section tabConfigBlueprint()
    {
        return {};
    }

    // A question the workspace asks drops under the browser's app button, and the page it fills
    // stands in the menu that button opens.
    int runSeeDocs(ApplicationBase& application, Form<SurfaceBrowser>& form,
        const std::filesystem::path& stated)
    {
        Workspace workspace{ application, form, form.content() };
        connectAppPage(k_openPageCaption, [&workspace](OptionsPage& page) {
            buildOpenPage(page, workspace);
        });
        workspace.start(stated);

        return form.execute();
    }
}
