module SeeDocs_App.OpenPage;

import SeeDocs_App.Editing;
import SeeDocs_App.Workspace;
import SeeDocs_App.Icons;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Label;
import ClaFi.Controls.Spacer;
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
        constexpr float k_rowSpacing = 10.0f;   // between the scan line and Rescan

        // The open project's name, typed into where it stands and kept in the project's folder.
        class NameBox : public WordsBox
        {
        public:
            NameBox(const CreateParams&, Workspace&, PlaceHolderText folderName);
        protected:
            [[nodiscard]] std::wstring writeWords(std::wstring_view words) override;
        private:
            Workspace& m_workspace;
        };

        // The name a recent row shows: the project's where its folder still holds one, else the
        // folder's own.
        [[nodiscard]] std::wstring nameOf(const std::filesystem::path& folder)
        {
            if (const std::optional<Project> project = readProject(folder))
                return project->name;
            return folder.filename().wstring();
        }

        // THE AGE IS WORKED OUT WHEN THE PAGE IS BUILT, which is every time the menu opens, and
        // after a rescan. It does not tick while the menu stays open.
        [[nodiscard]] Text scanText(const Workspace& workspace)
        {
            const ScanStamp scanned = workspace.scanned();
            if (!scanned)
                return Text{ L"Last scan: not recorded" };
            const std::chrono::sys_seconds now =
                std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
            return Text{ L"Last scan: " + ageSpelling(*scanned, now) };
        }

        // The open project: its name to rename it by, its folder, when it was scanned with the
        // button that scans it again, how its tree is read, and under a line, the button that
        // opens another.
        void addProjectGroup(OptionsPage& page, Workspace& workspace)
        {
            Stack& group = page.addGroup(L"Project");
            if (const Project* open = workspace.project())
            {
                NameBox& name = group.add<NameBox>(
                    workspace,
                    PlaceHolderText{ open->folder.filename().wstring() }
                );
                name.showWords(Text{ open->name });
                group.add<Label>(
                    Text{ open->folder.wstring() },
                    TextFormat{ InkGrade::Muted },
                    WordWrap::No,
                    MaxSize{ k_pathWidth, k_maxFloat }
                );
                Stack& scanRow = group.add<Stack>(
                    Orientation::Horizontal,
                    Spacing{ k_rowSpacing }
                );
                Label& scanLine = scanRow.add<Label>(
                    scanText(workspace),
                    WordWrap::No,
                    VerticalAlign::Center
                );
                scanRow.add<FlexSpacer>();
                // THE MENU STAYS OPEN, and the line beside the button says the scan has run.
                scanRow.add<Button>(
                    L"Rescan",
                    VerticalAlign::Center,
                    HintText{ L"Reads the folder again" },
                    Button::OnClick{ [&workspace, &scanLine](ClickEvent&) {
                        workspace.rescan();
                        scanLine.text() = scanText(workspace);
                        scanLine.invalidateFormAlign();
                    } }
                );
                // THE MENU STAYS OPEN, as for Rescan: the switch reads the tree again, and the
                // scan line says so. The box stands where a label's text stands, as on the
                // Settings page.
                group.add<CheckBox>(
                    L"Limited mode",
                    HintText{ L"Reads a third-party library - its headers too, with nothing "
                        L"written into it and nothing checked" },
                    Padding{ 0.0f, group.themeMetrics().listItem.padding.y },
                    CheckBox::OnGetState{ [&workspace](GetStateEvent& event) {
                        const Project* project = workspace.project();
                        event.state.selected = project && project->mode == ScanMode::Limited;
                    } },
                    CheckBox::OnClick{ [&workspace, &scanLine](ClickEvent& event) {
                        const Project* project = workspace.project();
                        if (!project)
                            return;
                        const bool limited = project->mode == ScanMode::Limited;
                        workspace.setMode(limited ? ScanMode::Full : ScanMode::Limited);
                        scanLine.text() = scanText(workspace);
                        scanLine.invalidateFormAlign();
                        event.control->invalidateState();
                    } }
                );
                group.add<Divider>();
            }
            else
            {
                group.add<Label>(Text{ L"No project is open" }, TextFormat{ InkGrade::Muted });
            }
            // THE MENU CLOSES BEFORE THE DIALOG OPENS: the menu is a popup, and a popup goes down
            // when the dialog takes the focus - see Platform::pickFolder.
            group.add<Button>(
                L"Browse",
                EndIcon::OpensWindow,
                HorizontalAlign::Left,
                HintText{ L"Opens a folder as a project" },
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

    // NameBox

    namespace
    {
        // Emptied, the box shows the folder's name in its place - the name an empty one gives.
        NameBox::NameBox(const CreateParams& params, Workspace& workspace,
            PlaceHolderText folderName)
            :
            WordsBox{
                params,
                TextFormat{ TextStyleId::SubHeading },
                std::move(folderName),
                HintText{ L"Renames the project - left empty, it takes the folder's name" }
            },
            m_workspace{ workspace }
        {
        }

        // The box shows the name the project goes by afterwards, the folder's for an empty one.
        std::wstring NameBox::writeWords(const std::wstring_view words)
        {
            if (!edited())
                return {};
            showWords(Text{ m_workspace.rename(words) });
            return {};
        }
    }
}
