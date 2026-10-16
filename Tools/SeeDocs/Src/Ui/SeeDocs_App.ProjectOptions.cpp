module SeeDocs_App.ProjectOptions;

import SeeDocs_App.Browser;
import SeeDocs_App.Editing;
import SeeDocs_App.FolderBox;
import SeeDocs_App.Workspace;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Stack;

import ClaFi.Icons.XMark;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        // **The widest a path may make the page.** The column is as wide as what it holds, so a
        // long path left to itself would stretch every row under it; past this it is trimmed, and
        // the tooltip over it shows the rest.
        constexpr float k_pathWidth = 440.0f;
        constexpr float k_rowSpacing = 10.0f;    // between the parts of a row
        constexpr float k_fieldWidth = 440.0f;   // the narrowest a field stands
        constexpr float k_noteWidth = 520.0f;    // the widest a note under a group runs
        constexpr float k_choiceIndent = 20.0f;  // a set of choices in from its caption
        constexpr float k_removeSize = 20.0f;
        constexpr float k_removeMark = 8.0f;
        // The project's own rows stand at the top as a group's body does: set in by as much, and
        // spaced as its rows are.
        constexpr Padding k_headPadding{ 12.0f, 0.0f };
        constexpr float k_headSpacing = 6.0f;
        constexpr std::wstring_view k_stubText = L"Add source";   // what the stub's cell reads
        constexpr std::wstring_view k_browseText = L"\u2026";
        constexpr std::wstring_view k_pickTitle = L"Pick a folder of sources";

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

        class SourceRow;

        // The folders of sources in a grid, a row each, over the stub a folder is added through.
        class SourceList : public Grids::Grid
        {
        public:
            SourceList(const CreateParams&, Workspace&, OpenProject&, Choose changed);
        public:
            // The grid's columns, as their tags name them.
            enum class SourceColumn : TagValue
            {
                Path,
                Browse,
                Subfolders,
                Remove
            };
        public:
            [[nodiscard]] Workspace& workspace() { return m_workspace; }
            [[nodiscard]] OpenProject& project() { return m_project; }
            [[nodiscard]] Grids::Column& column(SourceColumn tag)
                { return columnByTag(Tag{ tag }); }
            [[nodiscard]] bool readsSubfolders(std::wstring_view source) const;
            void changed() const { m_changed(); }   // after a change has been read back
            void addStub();
            // Stops reading the row's folder and takes the row out on the next tick, outside the
            // edit or the click that asked; the selection lands on the row under it.
            void removeLater(SourceRow&);
        protected:
            bool defaultCanMoveItem(Control&) override;   // the stub stays under the folders
            void itemsMoved(ItemsMoveEvent&) override;    // the folders are read in the rows' order
        private:
            using Rows = std::vector<SourceRow*>;
        private:
            void removePending();
        private:
            Workspace& m_workspace;
            OpenProject& m_project;
            Choose m_changed;
            Rows m_rows{};      // the rows standing, the stub last
            Rows m_pending{};   // the rows asked to go
            UiTimer m_removeTimer{};
        };

        // One folder of sources, or the stub under them: a row with no folder yet. See README
        class SourceRow : public Grids::RowContainer
        {
        public:
            SourceRow(const CreateParams&, Grids::GridDescriptor&, SourceList&,
                std::wstring source);
        public:
            [[nodiscard]] const std::wstring& source() const { return m_source; }
            [[nodiscard]] bool isStub() const { return m_source.empty(); }
            // Writes the source in the row's place, or as one more; the reason where refused.
            [[nodiscard]] std::wstring take(std::wstring_view written);
        protected:
            void getCellText(const Grids::Column&, Text&) override;   // the folder read, in full
            void getCellEditorText(Grids::GetCellEditorTextEvent&) override;   // the words written
        private:
            void addAnswers();   // the check and the cross, which the stub has not got
            void accept(Grids::AcceptCellTextEvent&);
            void browse();
            [[nodiscard]] bool isPath(const Grids::Column&) const;
        private:
            SourceList& m_list;
            std::wstring m_source;   // as written, empty on the stub
            FolderSuggester m_folders{};   // what the path's editor completes from
        };

        // **The age is worked out when the page is built**, and after a rescan. It does not tick
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
        [[nodiscard]] StackPanel& addChoiceSet(StackPanel& group)
        {
            return group.add<StackPanel>(Orientation::Vertical, Padding{ 0.0f });
        }

        // A caption in the group's own column, and under it the set of choices it names, set in
        // from it.
        [[nodiscard]] StackPanel& addChoiceSet(StackPanel& group, const std::wstring_view caption)
        {
            group.add<Label>(Text{ caption });
            return group.add<StackPanel>(Orientation::Vertical, Padding{ k_choiceIndent, 0.0f });
        }

        // One answer of a set, chosen while chosen() says so and greyed while enabled() says
        // not; a click runs choose() and redraws every answer of the set. The mark stands where
        // a label's text stands, as on the Settings page.
        void addChoice(StackPanel& set, const std::wstring_view caption, const std::wstring_view tooltip,
            Chosen chosen, Choose choose, Chosen enabled = {})
        {
            set.add<RadioButton>(
                caption,
                TooltipText{ tooltip },
                Padding{ 0.0f, set.themeMetrics().listItem.padding.y },
                HorizontalAlign::Left,
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
        void addNote(StackPanel& group, const std::wstring_view words)
        {
            group.add<Label>(
                Text{ words },
                TextFormat{ InkGrade::Muted },
                WordWrap::Yes,
                MaxSize{ k_noteWidth, k_maxFloat }
            );
        }
    }

    // **The page stands through a scan of its project.** A scan shows the surface again, and the
    // browser keeps the selected tab's options page while it does - see SurfaceBrowser - so a
    // button writes what came of it onto the page once the work is done. The project's own rows
    // stand under the page's title with no group of their own; the groups follow.
    void buildProjectOptions(OptionsPage& page, Workspace& workspace, OpenProject& project)
    {
        StackPanel& group = page.add<StackPanel>(
            Orientation::Vertical,
            k_headPadding,
            Spacing{ k_headSpacing }
        );
        Panel& nameField = group.add<Panel>(
            UiElement::Section,
            Radius{ group.themeMetrics().button.radius },
            group.themeMetrics().button.padding,
            MinSize{ k_fieldWidth, 0.0f }
        );
        nameField.setBorder(group.themeMetrics().button.border);
        NameBox& name = nameField.createBody<NameBox>(
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
            TooltipText{ L"The project's own folder: its properties, its database and its "
                L"footnotes" }
        );
        StackPanel& scanRow = group.add<StackPanel>(
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
            TooltipText{ L"Reads the folders of sources again" },
            Button::OnClick{ [&workspace, &project, rescanned](ClickEvent&) {
                workspace.rescan(project);
                rescanned();
            } }
        );

        StackPanel& sources = page.addGroup(L"Sources");
        sources.add<SourceList>(workspace, project, rescanned);

        // The edit choices are built after the hint choices, which grey one of them; the hints
        // reach them through this once they stand.
        const std::shared_ptr<StackPanel*> editSet = std::make_shared<StackPanel*>(nullptr);

        StackPanel& scan = page.addGroup(L"Scan options");
        StackPanel& readAs = addChoiceSet(scan, L"Read sources as");
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
        StackPanel& hints = addChoiceSet(scan, L"Hints format");
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

        StackPanel& edits = page.addGroup(L"Edit hints");
        StackPanel& editChoices = addChoiceSet(edits);
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
        // Set as the new project dialog's boxes are, in heading type. Emptied, it shows the
        // folder's name in its place - the name an empty one gives.
        NameBox::NameBox(const CreateParams& params, Workspace& workspace, OpenProject& project,
            PlaceHolderText folderName)
            :
            WordsBox{
                params,
                Interactivity::Focusable,
                TextFormat{ TextStyleId::Heading },
                WordWrap::No,
                Padding{ 0.0f },
                VerticalTextAnchor::Center,
                std::move(folderName),
                TooltipText{ L"Renames the project - left empty, it takes the folder's name" }
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
            Grids::Grid{
                params,
                params.themeMetrics().page,
                UiElement::Section,
                Grids::GridLines::Horizontal,
                ItemsOrder::Rearrangeable
            },
            m_workspace{ workspace },
            m_project{ project },
            m_changed{ std::move(changed) }
        {
            columns().add(Tag{ SourceColumn::Path }, Grids::ColumnWidthMode::Fill,
                EditorMode::Editable);
            columns().add(Tag{ SourceColumn::Browse }, Grids::CellHighlightMode::Control);
            columns().add(Tag{ SourceColumn::Subfolders });
            columns().add(Tag{ SourceColumn::Remove });
            m_removeTimer.onTick([this](TimerEvent&) {
                removePending();
            });
            for (const ProjectSource& source : m_project.project.sources)
                m_rows.push_back(&add<SourceRow>(*this, source.written));
            m_rows.push_back(&add<SourceRow>(*this, std::wstring{}));
        }

        bool SourceList::readsSubfolders(const std::wstring_view source) const
        {
            for (const ProjectSource& entry : m_project.project.sources)
            {
                if (entry.written == source)
                    return entry.subfolders;
            }
            return true;
        }

        void SourceList::addStub()
        {
            m_rows.push_back(&add<SourceRow>(*this, std::wstring{}));
            invalidateFormAlign();
        }

        void SourceList::removeLater(SourceRow& row)
        {
            if (std::ranges::find(m_pending, &row) != m_pending.end())
                return;
            m_pending.push_back(&row);
            m_removeTimer.start(MilliSeconds{ 0u });
        }

        // The stub is never removed, so a row always has one under it to land on.
        void SourceList::removePending()
        {
            Rows going{};
            std::swap(going, m_pending);
            for (SourceRow* row : going)
            {
                m_workspace.removeSource(m_project, row->source());
                const Rows::iterator next = m_rows.erase(std::ranges::find(m_rows, row));
                const bool held = row->containsNested(Input::focusedControl())
                    || descriptor().selectedRow() == row;
                row->deleteSelf();
                if (held)
                    selectCell(**next, &column(SourceColumn::Path));
            }
            invalidateFormAlign();
            changed();
        }

        bool SourceList::defaultCanMoveItem(Control& item)
        {
            if (!Grids::Grid::defaultCanMoveItem(item))
                return false;
            return !static_cast<const SourceRow&>(item).isStub();
        }

        void SourceList::itemsMoved(ItemsMoveEvent&)
        {
            Rows rows{};
            std::vector<std::wstring_view> order{};
            for (ControlPtr& control : controls())
            {
                const Rows::iterator found = std::ranges::find_if(m_rows,
                    [&control](const SourceRow* row) {
                        return row == &*control;
                    });
                if (found == m_rows.end())
                    continue;
                rows.push_back(*found);
                if (!(*found)->isStub())
                    order.push_back((*found)->source());
            }
            m_rows = std::move(rows);
            m_workspace.orderSources(m_project, order);
            changed();
        }
    }

    // SourceRow

    namespace
    {
        SourceRow::SourceRow(const CreateParams& params, Grids::GridDescriptor& descriptor,
            SourceList& list, std::wstring source)
            :
            Grids::RowContainer{ params, descriptor },
            m_list{ list },
            m_source{ std::move(source) }
        {
            m_folders.setBase(m_list.project().project.folder);
            addControl<Button>(
                m_list.column(SourceList::SourceColumn::Browse),
                k_browseText,
                VerticalAlign::Center,
                HorizontalTextAnchor::Center,
                VerticalTextAnchor::Center,
                Interactivity::MouseOnly,
                TooltipText{ L"Picks the folder in the desktop's own dialog" },
                Button::OnClick{ [this](ClickEvent&) {
                    browse();
                } }
            );
            onAcceptCellText([this](Grids::AcceptCellTextEvent& event) {
                accept(event);
            });
            if (!isStub())
                addAnswers();
        }

        void SourceRow::addAnswers()
        {
            addControl<CheckBox>(
                m_list.column(SourceList::SourceColumn::Subfolders),
                L"Include subfolders",
                VerticalAlign::Center,
                TooltipText{ L"Reads the folders under this one too" },
                themeMetrics().listItem.padding,
                CheckBox::OnGetState{ [this](GetStateEvent& event) {
                    event.state.selected = m_list.readsSubfolders(m_source);
                } },
                CheckBox::OnClick{ [this](ClickEvent& event) {
                    m_list.workspace().setSubfolders(m_list.project(), m_source,
                        !m_list.readsSubfolders(m_source));
                    event.control->invalidateState();
                    m_list.changed();
                } }
            );
            addControl<ToolButton>(
                m_list.column(SourceList::SourceColumn::Remove),
                L"Remove",
                FixedSize{ k_removeSize },
                ButtonViewMode::IconOnly,
                IconSize{ k_removeMark },
                VerticalAlign::Center,
                Interactivity::MouseOnly,
                ButtonBase::OnPaintIcon{ Icons::XMark::paint },
                TooltipText{ L"Stops reading this folder" },
                Button::OnClick{ [this](ClickEvent&) {
                    m_list.removeLater(*this);
                } }
            );
            invalidateFormAlign();
        }

        bool SourceRow::isPath(const Grids::Column& column) const
        {
            return column.tag().value == static_cast<TagValue>(SourceList::SourceColumn::Path);
        }

        void SourceRow::getCellText(const Grids::Column& column, Text& text)
        {
            if (!isPath(column))
            {
                Grids::RowContainer::getCellText(column, text);
                return;
            }
            if (isStub())
                text << InkGrade::Muted << k_stubText;
            else
                text << m_list.project().project.readSource(m_source).wstring();
        }

        void SourceRow::getCellEditorText(Grids::GetCellEditorTextEvent& event)
        {
            if (!isPath(event.column()))
            {
                Grids::RowContainer::getCellEditorText(event);
                return;
            }
            event.text = Text{ m_source };
            event.suggester = &m_folders;
            event.stopPropagation();
        }

        // An edit emptied stops reading the folder; the stub emptied stays the stub.
        void SourceRow::accept(Grids::AcceptCellTextEvent& event)
        {
            if (!isPath(event.column()))
                return;
            event.stopPropagation();
            const std::wstring typed{ trimmed(event.text().plainText()) };
            if (typed.empty())
            {
                if (!isStub())
                    m_list.removeLater(*this);
                return;
            }
            const std::wstring refusal = take(typed);
            if (!refusal.empty())
                event.refuse(refusal);
        }

        std::wstring SourceRow::take(const std::wstring_view written)
        {
            Workspace& workspace = m_list.workspace();
            OpenProject& project = m_list.project();
            const bool stub = isStub();
            std::wstring refusal = stub
                ? workspace.addSource(project, written)
                : workspace.replaceSource(project, m_source, written);
            if (!refusal.empty())
                return refusal;
            m_source = written;
            invalidate();
            if (stub)
            {
                addAnswers();
                m_list.addStub();
            }
            m_list.changed();
            return {};
        }

        // A folder refused - one read already - is said at the row, the way a box would say it.
        void SourceRow::browse()
        {
            const Project& project = m_list.project().project;
            std::filesystem::path start = project.folder;
            if (!isStub())
                start = project.readSource(m_source);
            else if (!project.sources.empty())
                start = project.readSource(project.sources.back().written);
            std::error_code error;
            while (!std::filesystem::is_directory(start, error))
            {
                const std::filesystem::path parent = start.parent_path();
                if (parent == start)
                    break;
                start = parent;
            }
            const std::optional<std::filesystem::path> picked =
                Platform::pickFolder(&form(), k_pickTitle, start);
            if (!picked)
                return;
            const std::wstring refusal = take(project.writtenSource(*picked));
            if (!refusal.empty())
                ContextMessage::show(*this, Text{ refusal });
        }
    }
}
