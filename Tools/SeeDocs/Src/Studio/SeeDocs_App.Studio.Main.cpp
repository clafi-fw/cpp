module SeeDocs_App.Studio.Main;

import SeeDocs_App.Studio.SurfaceView;
import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Icons.SideBar;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
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
        constexpr std::wstring_view k_databasePath = L"Tools/SeeDocs/Surface.cfg";
        constexpr float k_minWidth = 820.0f;
        constexpr float k_minHeight = 520.0f;
        constexpr float k_preferredWidth = 1360.0f;
        constexpr float k_preferredHeight = 880.0f;

        [[nodiscard]] bool holdsSource(const std::filesystem::path& folder)
        {
            std::error_code error;
            return std::filesystem::is_directory(folder / k_sourceFolder, error);
        }

        // The nearest folder holding Source, from the one given upwards.
        [[nodiscard]] std::filesystem::path treeAbove(std::filesystem::path folder)
        {
            while (!folder.empty())
            {
                if (holdsSource(folder))
                    return folder;
                const std::filesystem::path parent = folder.parent_path();
                if (parent == folder)
                    break;
                folder = parent;
            }
            return {};
        }

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

        // What the page shows where there is no database to show.
        [[nodiscard]] Text noDatabaseText(const std::filesystem::path& tree,
            const std::filesystem::path& database)
        {
            Text text;
            text << TextStyleId::Title << k_appName << PopTextStyle{} << k_endLine << k_endLine;
            if (tree.empty())
            {
                text << L"No folder holding Source stands at or above the working directory. "
                    L"Start the studio with the tree's folder as its argument." << k_endLine;
                return text;
            }
            text << L"No database at " << TextStyleId::Code << database.wstring()
                << PopTextStyle{} << L". Run " << TextStyleId::Code << L"seedocs scan"
                << PopTextStyle{} << L" over the tree first." << k_endLine;
            return text;
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

    std::filesystem::path findTree(const std::filesystem::path& stated)
    {
        std::error_code error;
        if (!stated.empty())
        {
            const std::filesystem::path absolute = std::filesystem::absolute(stated, error);
            return holdsSource(absolute) ? absolute : std::filesystem::path{};
        }
        const std::filesystem::path fromWorkingDirectory =
            treeAbove(std::filesystem::current_path(error));
        if (!fromWorkingDirectory.empty())
            return fromWorkingDirectory;
        const std::wstring executableDirectory = Platform::executableDirectory();
        if (executableDirectory.empty())
            return {};
        return treeAbove(std::filesystem::path{ executableDirectory });
    }

    int runStudio(ApplicationBase& application, const std::filesystem::path& tree)
    {
        const std::filesystem::path database = tree / k_databasePath;
        std::optional<Surface> surface;
        if (!tree.empty())
            surface = readDatabase(database);
        // Declared before the form, which holds it by reference for as long as it runs.
        Notes notes{ tree / k_sourceFolder / k_notesFolder };

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
        title.createLeftBar<AppButton>(
            VerticalAlign::Center,
            AppButton::OnPaintIcon{ Icons::SideBar::paintPanelIcon }
        );

        if (surface)
            form->body().bind(*surface, notes);
        else
            form->body().showText(noDatabaseText(tree, database));

        return form->execute();
    }
}
