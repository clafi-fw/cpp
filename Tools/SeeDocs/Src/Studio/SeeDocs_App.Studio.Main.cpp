module SeeDocs_App.Studio.Main;

import SeeDocs_App.Studio.OpenPage;
import SeeDocs_App.Studio.SurfaceView;
import SeeDocs_App.Studio.Workspace;

import ClaFi.App.Settings;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Icons.SideBar;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr std::wstring_view k_appName = L"SeeDocs Studio";
        constexpr std::wstring_view k_publisher = L"ClaFi Framework";
        constexpr std::wstring_view k_openPageCaption = L"Open";
        constexpr float k_minWidth = 820.0f;
        constexpr float k_minHeight = 520.0f;
        constexpr float k_preferredWidth = 1360.0f;
        constexpr float k_preferredHeight = 880.0f;

        // The window: the surface view in the body, the title bar put on by runStudio.
        class MainForm : public WithBody<Panel, SurfaceView>
        {
        public:
            explicit MainForm(const CreateParams&);
        };

        MainForm::MainForm(const CreateParams& params)
            :
            WithBody{
                params,
                HostProps{
                    FormPlacement::Default,
                    params.themeMetrics().primaryWindow,
                    params.themeMetrics().primaryWindowShadow,
                    UiElement::ToolBar,
                    Border{ Thickness::Thin },
                    Padding{ 0.0f },
                    Spacing{ 0.0f },
                    MinSize{ k_minWidth, k_minHeight },
                    PreferredSize{ k_preferredWidth, k_preferredHeight }
                },
                BodyProps{}
            }
        {
        }
    }

    AppParams studioParams()
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

    Dom::Dt::Section createStudioConfigSchema()
    {
        return Workspace::createConfigSchema();
    }

    int runStudio(ApplicationBase& application, const std::filesystem::path& stated)
    {
        const std::unique_ptr<Form<MainForm>> form = application.createDialog<MainForm>();
        form->setConfigName(AppContext::k_mainFormName);

        // The title is the app button's container, and its padding is what stands between the
        // mark and the corner of the window.
        auto& title = form->createTopBar<DialogTitle>(
            HorizontalTextAnchor::Left,
            application.name(),
            Padding{ 0.0f },
            Spacing{ 4.0f }
        );
        AppButton& appButton = title.createLeftBar<AppButton>(
            VerticalAlign::Center,
            AppButton::OnPaintIcon{ Icons::SideBar::paintPanelIcon }
        );

        // A question the workspace asks drops under the app button, and the page it fills stands
        // in the menu that button opens.
        Workspace workspace{ application, *form, title, appButton, form->body() };
        connectAppPage(k_openPageCaption, [&workspace](OptionsPage& page) {
            buildOpenPage(page, workspace);
        });
        workspace.start(stated);

        return form->execute();
    }
}
