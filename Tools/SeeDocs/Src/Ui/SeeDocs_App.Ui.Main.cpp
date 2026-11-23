module SeeDocs_App.Ui.Main;

import SeeDocs_App.Ui.Browser;
import SeeDocs_App.Ui.ExternalEditors;
import SeeDocs_App.Ui.Icons;
import SeeDocs_App.Ui.OpenPage;
import SeeDocs_App.Ui.ProjectOptions;
import SeeDocs_App.Ui.Workspace;

import ClaFi.App.AppMenu;
import ClaFi.App.ApplicationProperties;
import ClaFi.App.Settings;

import ClaFi.App.WindowApplication;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;

import ClaFi.StdLib;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;

    namespace
    {
        constexpr std::wstring_view k_appName = L"SeeDocs";
        constexpr std::wstring_view k_publisher = L"ClaFi Framework";
        constexpr std::wstring_view k_openPageCaption = L"Open";
        constexpr std::wstring_view k_newProjectCaption = L"New project";

        [[nodiscard]] MainForm* mainFormOf(FormBase& form)
        {
            FormBase& root = form.rootForm();
            if (&root != root.appContext().rootLoopForm())
                return nullptr;
            return &static_cast<Form<MainForm>&>(root).content();
        }
    }

    ApplicationProperties seeDocsParams()
    {
        return {
            .name = Text{ k_appName },
            .publisher = k_publisher,
            .description = Text{ L"A code documentation tool" },
            .version = CLAFI_APP_VERSION,
            .site = L"https://seedocs.app",
            .updates = UpdateSource{
                .repository = L"clafi-fw/cpp",
                .tagPrefix = L"SeeDocs-v"
            },
            .linkScheme = SurfaceBrowser::k_pageLinkScheme,
            .linkHandler = [](const Url& url, IForm* source, InputStamp) {
                MainForm* mainForm = source ? mainFormOf(static_cast<FormBase&>(*source)) : nullptr;
                return mainForm && mainForm->openPageLink(url);
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

    MainForm::MainForm(const CreateParams& params)
        :
        SurfaceBrowser{
            params,
            UiElement::ToolBar,
            Radius{ params.styleMetrics(UiElement::Dialog).radius },
            params.styleMetrics(UiElement::Dialog).shadow,
            Border{ Thickness::Thin },
            Padding{ 0.0f },
            Spacing{ 0.0f },
            MinSize{ 820.0f, 520.0f },
            PreferredSize{ 1360.0f, 880.0f }
        }
    {
    }

    void MainForm::connectMenus()
    {
        connectAppPage(k_openPageCaption, drawOpenIcon, [](AppMenuPage& page) {
            MainForm* mainForm = mainFormOf(page.form());
            if (mainForm && mainForm->m_workspace)
            {
                buildOpenPage(page, *mainForm->m_workspace);
            }
        });
        connectAppCommand(k_newProjectCaption, drawNewProjectIcon, EndIcon::OpensWindow,
            [](ClickEvent& event) {
                MainForm* mainForm = mainFormOf(event.form);
                if (mainForm && mainForm->m_workspace)
                {
                    event.closeForm();
                    mainForm->m_workspace->createNew({}, event.stamp);
                }
            });
        connectAppSettings(buildExternalEditors);
    }

    void MainForm::start(WindowApplicationBase& application, const std::filesystem::path& stated)
    {
        m_workspace.emplace(application, form(), *this);
        connectProjectPage([this](AppMenuPage& page, OpenProject& project) {
            buildProjectOptions(page, *m_workspace, project);
        });
        m_workspace->start(stated);
    }
}
