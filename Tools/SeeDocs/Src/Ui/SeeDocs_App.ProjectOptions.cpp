module SeeDocs_App.ProjectOptions;

import SeeDocs_App.Browser;
import SeeDocs_App.Editing;
import SeeDocs_App.Workspace;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Label;
import ClaFi.Controls.RadioButton;
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
        // THE WIDEST A PATH MAY MAKE THE PAGE. The column is as wide as what it holds, so a long
        // path left to itself would stretch every row under it; past this it is trimmed, and the
        // hint over it shows the rest.
        constexpr float k_pathWidth = 440.0f;
        constexpr float k_rowSpacing = 10.0f;    // between a row's label and its buttons
        constexpr float k_listSpacing = 2.0f;    // between the rows of the sources
        constexpr float k_noteWidth = 520.0f;    // the widest a note under a group runs
        constexpr float k_choiceIndent = 20.0f;  // a set of choices in from its caption
        // The project's own rows stand at the top as a group's body does: set in by as much, and
        // spaced as its rows are.
        constexpr Padding k_headPadding{ 12.0f, 0.0f };
        constexpr float k_headSpacing = 6.0f;

        // What a choice's state is read from, and what a click on it does.
        using Chosen = std::function<bool()>;
        using Choose = std::function<void()>;

        // The project's name, typed into where it stands and kept in the project's folder.
        class NameBox : public WordsBox
        {
        public:
            NameBox(const CreateParams&, Workspace&, OpenProject&, PlaceHolderText folderName);
        protected:
            [[nodiscard]] std::wstring writeWords(std::wstring_view words) override;
        private:
            Workspace& m_workspace;
            OpenProject& m_project;
        };

        // THE PROJECT'S FOLDERS OF SOURCES, a row each: the folder, whether the folders under it
        // are read, and Remove. The rows are built again from the project after each change,
        // what stands being what was read back. A row given way is hidden, and goes at the next
        // change: one of them holds the button that was pressed.
        class SourceList : public Stack
        {
        public:
            SourceList(const CreateParams&, Workspace&, OpenProject&, Choose changed);
        public:
            void rebuild();
        private:
            void addRow(const SourceFolder&);
            [[nodiscard]] bool readsSubfolders(const std::filesystem::path& source) const;
        private:
            Workspace& m_workspace;
            OpenProject& m_project;
            Choose m_changed;                  // after a change has been read back
            std::vector<Control*> m_rows{};
            std::vector<Control*> m_goneRows{};
        };

        // THE AGE IS WORKED OUT WHEN THE PAGE IS BUILT, and after a rescan. It does not tick
        // while the page stands.
        [[nodiscard]] Text scanText(const OpenProject& project)
        {
            const ScanStamp scanned = Workspace::scanned(project);
            if (!scanned)
                return Text{ L"Last scan: not recorded" };
            const std::chrono::sys_seconds now =
                std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
            return Text{ L"Last scan: " + ageSpelling(*scanned, now) };
        }

        // The column a set of choices stands in, whose answers redraw each other.
        [[nodiscard]] Stack& addChoiceSet(Stack& group)
        {
            return group.add<Stack>(Orientation::Vertical, Padding{ 0.0f });
        }

        // A caption in the group's own column, and under it the set of choices it names, set in
        // from it.
        [[nodiscard]] Stack& addChoiceSet(Stack& group, const std::wstring_view caption)
        {
            group.add<Label>(Text{ caption });
            return group.add<Stack>(Orientation::Vertical, Padding{ k_choiceIndent, 0.0f });
        }

        // One answer of a set, chosen while chosen() says so and greyed while enabled() says
        // not; a click runs choose() and redraws every answer of the set. The mark stands where
        // a label's text stands, as on the Settings page.
        void addChoice(Stack& set, const std::wstring_view caption, const std::wstring_view hint,
            Chosen chosen, Choose choose, Chosen enabled = {})
        {
            set.add<RadioButton>(
                caption,
                HintText{ hint },
                Padding{ 0.0f, set.themeMetrics().listItem.padding.y },
                RadioButton::OnGetState{ [chosen, enabled](GetStateEvent& event) {
                    event.state.selected = chosen();
                    if (enabled)
                        event.state.enabled = enabled();
                } },
                RadioButton::OnClick{ [chosen, choose](ClickEvent& event) {
                    if (!chosen())
                        choose();
                    event.control->invalidateSiblingStates();
                } }
            );
        }

        // A muted paragraph under a group's options, saying what they do not.
        void addNote(Stack& group, const std::wstring_view words)
        {
            group.add<Label>(
                Text{ words },
                TextFormat{ InkGrade::Muted },
                WordWrap::Yes,
                MaxSize{ k_noteWidth, k_maxFloat }
            );
        }
    }

    // THE PAGE STANDS THROUGH A SCAN OF ITS PROJECT. A scan shows the surface again, and the
    // browser keeps the selected tab's options page while it does - see SurfaceBrowser - so a
    // button writes what came of it onto the page once the work is done. The project's own rows
    // stand under the page's title with no group of their own; the groups follow.
    void buildProjectOptions(OptionsPage& page, Workspace& workspace, OpenProject& project)
    {
        Stack& group = page.add<Stack>(
            Orientation::Vertical,
            k_headPadding,
            Spacing{ k_headSpacing }
        );
        NameBox& name = group.add<NameBox>(
            workspace,
            project,
            PlaceHolderText{ project.project.folderName() }
        );
        name.showWords(Text{ project.project.name });
        group.add<Label>(
            Text{ project.project.folder.wstring() },
            TextFormat{ InkGrade::Muted },
            WordWrap::No,
            MaxSize{ k_pathWidth, k_maxFloat },
            HintText{ L"The project's own folder: its properties, its database and its "
                L"footnotes" }
        );
        Stack& scanRow = group.add<Stack>(
            Orientation::Horizontal,
            Spacing{ k_rowSpacing }
        );
        Label& scanLine = scanRow.add<Label>(
            scanText(project),
            WordWrap::No,
            VerticalAlign::Center
        );
        // What every change that reads the sources again ends with.
        const Choose rescanned = [&project, &scanLine]() {
            scanLine.text() = scanText(project);
            scanLine.invalidateFormAlign();
        };
        scanRow.add<FlexSpacer>();
        scanRow.add<Button>(
            L"Rescan",
            VerticalAlign::Center,
            HintText{ L"Reads the folders of sources again" },
            Button::OnClick{ [&workspace, &project, rescanned](ClickEvent&) {
                workspace.rescan(project);
                rescanned();
            } }
        );

        Stack& sources = page.addGroup(L"Sources");
        SourceList& list = sources.add<SourceList>(workspace, project, rescanned);
        sources.add<Button>(
            L"Add folder",
            EndIcon::OpensWindow,
            HorizontalAlign::Left,
            HintText{ L"Adds a folder of sources the project reads, with the folders under it" },
            Button::OnClick{ [&workspace, &project, &list, rescanned](ClickEvent&) {
                if (!workspace.addSource(project))
                    return;
                list.rebuild();
                rescanned();
            } }
        );

        // The edit choices are built after the hint choices, which grey one of them; the hints
        // reach them through this once they stand.
        const std::shared_ptr<Stack*> editSet = std::make_shared<Stack*>(nullptr);

        Stack& scan = page.addGroup(L"Scan options");
        Stack& readAs = addChoiceSet(scan, L"Read sources as");
        addChoice(readAs, L"Any C++ and Delphi",
            L"Anyone's C++ - headers too - and Delphi units, with nothing checked",
            [&project]() {
                return project.project.mode == ScanMode::Limited;
            },
            [&workspace, &project, rescanned]() {
                workspace.setMode(project, ScanMode::Limited);
                rescanned();
            });
        addChoice(readAs, L"ClaFi modules",
            L"Module interfaces and their implementation units, written to the declaration "
                L"routine and checked against it",
            [&project]() {
                return project.project.mode == ScanMode::Full;
            },
            [&workspace, &project, rescanned]() {
                workspace.setMode(project, ScanMode::Full);
                rescanned();
            });
        Stack& hints = addChoiceSet(scan, L"Hints format");
        const Choose hintsChanged = [rescanned, editSet]() {
            rescanned();
            if (*editSet)
                (*editSet)->invalidateChildrenStates();
        };
        addChoice(hints, L"Every recognizable comment",
            L"A hint is the brief of whatever comment a declaration carries; none is owed",
            [&project]() {
                return project.project.hints == HintRules::Loose;
            },
            [&workspace, &project, hintsChanged]() {
                workspace.setHints(project, HintRules::Loose);
                hintsChanged();
            });
        addChoice(hints, L"Strict ClaFi rules",
            L"A hint is the comment on its declaration's line, else the first line above it; "
                L"a declaration with none is marked",
            [&project]() {
                return project.project.hints == HintRules::Strict;
            },
            [&workspace, &project, hintsChanged]() {
                workspace.setHints(project, HintRules::Strict);
                hintsChanged();
            });

        Stack& edits = page.addGroup(L"Edit hints");
        Stack& editChoices = addChoiceSet(edits);
        *editSet = &editChoices;
        addChoice(editChoices, L"Not allowed",
            L"Hints are read, and never written",
            [&project]() {
                return project.project.edits == HintEdits::None;
            },
            [&workspace, &project]() {
                workspace.setEdits(project, HintEdits::None);
            });
        addChoice(editChoices, L"Directly to the source tree (no undo)",
            L"A hint edited on a page is written into its source file at once",
            [&project]() {
                return project.project.edits == HintEdits::Direct;
            },
            [&workspace, &project]() {
                workspace.setEdits(project, HintEdits::Direct);
            },
            [&project]() {
                return project.project.hints == HintRules::Strict;
            });
        addNote(edits, L"A hint is written back only where hints follow the strict ClaFi rules, "
            L"and only into C++ sources. Footnotes are always editable, and are kept in the "
            L"project's folder.");
    }

    // NameBox

    namespace
    {
        // Emptied, the box shows the folder's name in its place - the name an empty one gives.
        NameBox::NameBox(const CreateParams& params, Workspace& workspace, OpenProject& project,
            PlaceHolderText folderName)
            :
            WordsBox{
                params,
                TextFormat{ TextStyleId::SubHeading },
                std::move(folderName),
                HintText{ L"Renames the project - left empty, it takes the folder's name" }
            },
            m_workspace{ workspace },
            m_project{ project }
        {
        }

        // The box shows the name the project goes by afterwards, the folder's for an empty one.
        std::wstring NameBox::writeWords(const std::wstring_view words)
        {
            if (!edited())
                return {};
            showWords(Text{ m_workspace.rename(m_project, words) });
            return {};
        }
    }

    // SourceList

    namespace
    {
        SourceList::SourceList(const CreateParams& params, Workspace& workspace,
            OpenProject& project, Choose changed)
            :
            Stack{
                params,
                Orientation::Vertical,
                Padding{ 0.0f },
                Spacing{ k_listSpacing }
            },
            m_workspace{ workspace },
            m_project{ project },
            m_changed{ std::move(changed) }
        {
            rebuild();
        }

        void SourceList::rebuild()
        {
            for (Control* row : m_goneRows)
                row->deleteSelf();
            m_goneRows = std::exchange(m_rows, {});
            for (Control* row : m_goneRows)
                row->setVisible(false);
            if (m_project.project.sources.empty())
            {
                m_rows.push_back(&add<Label>(
                    Text{ L"No folder of sources yet" },
                    TextFormat{ InkGrade::Muted }
                ));
            }
            for (const SourceFolder& source : m_project.project.sources)
                addRow(source);
            invalidateFormAlign();
        }

        // The row names its folder by path, which outlasts the list it was built from.
        void SourceList::addRow(const SourceFolder& source)
        {
            const std::filesystem::path path = source.path;
            Stack& row = add<Stack>(
                Orientation::Horizontal,
                Spacing{ k_rowSpacing }
            );
            row.add<Label>(
                Text{ path.wstring() },
                WordWrap::No,
                VerticalAlign::Center,
                MaxSize{ k_pathWidth, k_maxFloat },
                HintText{ path.wstring() }
            );
            row.add<FlexSpacer>();
            row.add<CheckBox>(
                L"Include subfolders",
                VerticalAlign::Center,
                HintText{ L"Reads the folders under this one too" },
                Padding{ 0.0f, row.themeMetrics().listItem.padding.y },
                CheckBox::OnGetState{ [this, path](GetStateEvent& event) {
                    event.state.selected = readsSubfolders(path);
                } },
                CheckBox::OnClick{ [this, path](ClickEvent& event) {
                    m_workspace.setSubfolders(m_project, path, !readsSubfolders(path));
                    event.control->invalidateState();
                    m_changed();
                } }
            );
            row.add<Button>(
                L"Remove",
                VerticalAlign::Center,
                HintText{ L"Stops reading this folder" },
                Button::OnClick{ [this, path](ClickEvent&) {
                    m_workspace.removeSource(m_project, path);
                    rebuild();
                    m_changed();
                } }
            );
            m_rows.push_back(&row);
        }

        bool SourceList::readsSubfolders(const std::filesystem::path& source) const
        {
            for (const SourceFolder& folder : m_project.project.sources)
            {
                if (folder.path == source)
                    return folder.subfolders;
            }
            return true;
        }
    }
}
