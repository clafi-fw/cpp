module SeeDocs_App.Main;

import SeeDocs_App.Browser;
import SeeDocs_App.ExternalEditors;
import SeeDocs_App.Icons;
import SeeDocs_App.OpenPage;
import SeeDocs_App.ProjectOptions;
import SeeDocs_App.Workspace;

import ClaFi.App.AppMenu;
import ClaFi.App.Settings;

import ClaFi.App.Application;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;

    namespace
    {
        constexpr std::wstring_view k_appName = L"SeeDocs";
        constexpr std::wstring_view k_publisher = L"ClaFi Framework";
        constexpr std::wstring_view k_openPageCaption = L"Open";
        constexpr std::wstring_view k_newProjectCaption = L"New project";
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

    // The workspace's section is unnamed, so its keys stand at the root beside the editors'.
    Dom::Dt::Section rootConfigBlueprint()
    {
        return Dom::Dt::Section{
            Workspace::createConfigSchema(),
            createExternalEditorsSchema()
        };
    }

    Dom::Dt::Section tabEntryBlueprint()
    {
        return {};
    }

    Dom::Dt::Section tabConfigBlueprint()
    {
        return SurfaceBrowser::createTabConfigSchema();
    }

    // A question the workspace asks drops under the browser's app button. The Open page it fills
    // and its New project command stand in the menu that button opens, with the External editors
    // group on its Settings page, and each project's options page in the browser.
    int runSeeDocs(ApplicationBase& application, Form<SurfaceBrowser>& form,
        const std::filesystem::path& stated)
    {
        Workspace workspace{ application, form, form.content() };
        connectAppPage(k_openPageCaption, paintOpenIcon, [&workspace](OptionsPage& page) {
            buildOpenPage(page, workspace);
        });
        connectAppCommand(k_newProjectCaption, paintNewProjectIcon, EndIcon::OpensWindow,
            [&workspace](ClickEvent& event) {
                event.closeForm();
                workspace.createNew({}, event.stamp);
            });
        connectAppSettings(buildExternalEditors);
        form.content().connectProjectPage([&workspace](OptionsPage& page, OpenProject& project) {
            buildProjectOptions(page, workspace, project);
        });
        workspace.start(stated);

        return form.execute();
    }
}
