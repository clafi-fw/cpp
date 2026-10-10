module SeeDocs_App.OpenPage;

import SeeDocs_App.Workspace;
import SeeDocs_App.Icons;
import SeeDocs_App.Project;

import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        // THE WIDEST A PATH MAY MAKE THE PAGE. The menu is measured from its pages, so a long path
        // left to itself would widen the whole backstage; past this it is trimmed, and the hint
        // over it shows the rest.
        constexpr float k_pathWidth = 440.0f;
        constexpr float k_iconGap = 5.0f;   // between a row's icon and its text
        constexpr float k_nameGap = 12.0f;  // between a row's name and its folder

        // The name a recent row shows: the project's where the folder still stands for one, else
        // the folder's own.
        [[nodiscard]] std::wstring nameOf(const std::filesystem::path& folder)
        {
            if (const std::optional<Project> project = readProject(folder))
                return project->name;
            return folder.filename().wstring();
        }

        // The button that opens a project in a new tab, under a line saying none is open while
        // none is. A new one is made by the menu's own command, and an open project's options
        // stand on its page, under its node in the tree.
        void addProjectGroup(OptionsPage& page, Workspace& workspace)
        {
            Stack& group = page.addGroup(L"Project");
            if (!workspace.anyOpen())
                group.add<Label>(Text{ L"No project is open" }, TextFormat{ InkGrade::Muted });
            // THE MENU CLOSES BEFORE THE DIALOG OPENS: the menu is a popup, and a popup goes down
            // when a dialog takes the focus - see Platform::pickFolder.
            group.add<Button>(
                L"Browse",
                EndIcon::OpensWindow,
                HorizontalAlign::Left,
                HintText{ L"Opens a project's folder, or a tree that keeps one" },
                Button::OnClick{ [&workspace](ClickEvent& event) {
                    event.closeForm();
                    workspace.browse();
                } }
            );
        }

        void addRecentGroup(OptionsPage& page, Workspace& workspace)
        {
            const Workspace::Folders recent = workspace.recentFolders();
            if (recent.empty())
                return;
            Stack& group = page.addGroup(L"Recent");
            for (const std::filesystem::path& folder : recent)
            {
                Text row{ rowIcon(RowIcon::Chapter), Space{ k_iconGap }, nameOf(folder) };
                row << Space{ k_nameGap } << InkGrade::Muted << folder.wstring() << PopColor{};
                group.add<ToolButton>(
                    row,
                    HorizontalTextAnchor::Left,
                    WordWrap::No,
                    MaxSize{ k_pathWidth, k_maxFloat },
                    ToolButton::OnClick{ [&workspace, folder](ClickEvent& event) {
                        event.closeForm();
                        workspace.openFolder(folder);
                    } }
                );
            }
        }
    }

    void buildOpenPage(OptionsPage& page, Workspace& workspace)
    {
        addProjectGroup(page, workspace);
        addRecentGroup(page, workspace);
    }
}
