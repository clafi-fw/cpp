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
        // the hint over it shows the rest.
        constexpr float k_pathWidth = 440.0f;
        constexpr float k_rowSpacing = 10.0f;    // between the parts of a row
        constexpr float k_fieldWidth = 440.0f;   // the narrowest a field stands
        constexpr float k_listSpacing = 2.0f;    // between the rows of the sources
        constexpr float k_noteWidth = 520.0f;    // the widest a note under a group runs
        constexpr float k_choiceIndent = 20.0f;  // a set of choices in from its caption
        constexpr float k_browseGap = 4.0f;      // between a source's box and its browse button
        constexpr float k_browsePadding = 6.0f;  // the browse button's sides
        constexpr float k_removeSize = 20.0f;
        constexpr float k_removeMark = 8.0f;
        // The project's own rows stand at the top as a group's body does: set in by as much, and
        // spaced as its rows are.
        constexpr Padding k_headPadding{ 12.0f, 0.0f };
        constexpr float k_headSpacing = 6.0f;
        // What an empty source box shows: the stub's, and an entry's emptied.
        namespace SourcePlaceHolder
        {
            constexpr std::wstring_view stub = L"Add source";
            constexpr std::wstring_view entry = L"A folder on this computer";
        }
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

        // The project's folders of sources, a row each, over the stub a folder is added through.
        class SourceList : public Stack
        {
        public:
            SourceList(const CreateParams&, Workspace&, OpenProject&, Choose changed);
        public:
            [[nodiscard]] Workspace& workspace() { return m_workspace; }
            [[nodiscard]] OpenProject& project() { return m_project; }
            [[nodiscard]] bool readsSubfolders(std::wstring_view source) const;
            void changed() const { m_changed(); }   // after a change has been read back
            void addStub();
            // Hides an entry's row and hands its focus on; the row is deleted at the next change.
            void remove(SourceRow&);
        private:
            using Rows = std::vector<SourceRow*>;
        private:
            void deleteGoneRows();
        private:
            Workspace& m_workspace;
            OpenProject& m_project;
            Choose m_changed;
            Rows m_rows{};                        // the rows standing, the stub last
            std::vector<Control*> m_goneRows{};   // each may hold the control that hid it
        };

        // An entry's answers; the stub carries them unseen, its field ending where an entry's does.
        class RowAnswers : public Stack
        {
        public:
            RowAnswers(const CreateParams&, bool shown);
        public:
            void reveal();
        protected:
            void getControlState(GetStateEvent&) const override;   // unseen, no focus reaches them
            void hitTest(HitTestEvent&) const override;            // unseen, the pointer passes
            void paintChildren(PaintEvent&) override;
        private:
            bool m_shown;
        };

        // A source's box: the folder read while it is left, and the words written while edited.
        class SourceBox : public FolderBox<WordsBox>
        {
        public:
            SourceBox(const CreateParams&, SourceRow&, OpenProject&);
        public:
            void showSource();
        protected:
            [[nodiscard]] std::wstring writeWords(std::wstring_view words) override;
            void focusChanged() override;
        private:
            SourceRow& m_row;
            OpenProject& m_project;
        };

        // One folder of sources, or the stub under them: a row with no folder yet. See README
        class SourceRow : public Panel
        {
        public:
            SourceRow(const CreateParams&, SourceList&, std::wstring source);
        public:
            [[nodiscard]] const std::wstring& source() const { return m_source; }
            [[nodiscard]] bool isStub() const { return m_source.empty(); }
            [[nodiscard]] SourceBox& box() { return m_box; }
            // Writes the source in the row's place, or as one more; the reason where refused.
            [[nodiscard]] std::wstring take(std::wstring_view written);
            void remove();   // the project stops reading the folder, and the row goes
        private:
            void browse();
        private:
            SourceList& m_list;
            std::wstring m_source;   // as written, empty on the stub
            Panel& m_field;
            SourceBox& m_box;
            RowAnswers& m_answers;
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

    // **The page stands through a scan of its project.** A scan shows the surface again, and the
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
        sources.add<SourceList>(workspace, project, rescanned);

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
            deleteGoneRows();
            m_rows.push_back(&add<SourceRow>(*this, std::wstring{}));
            invalidateFormAlign();
        }

        void SourceList::remove(SourceRow& row)
        {
            deleteGoneRows();
            const bool heldFocus = row.containsNested(Input::focusedControl());
            row.box().showWords(Text{});   // or the focus leaving it writes what it holds
            row.setVisible(false);
            m_goneRows.push_back(&row);
            const Rows::iterator next = m_rows.erase(std::ranges::find(m_rows, &row));
            // The stub is never removed, so an entry always has a row under it.
            if (heldFocus)
                (*next)->box().setFocus();
            invalidateFormAlign();
        }

        void SourceList::deleteGoneRows()
        {
            for (Control* row : m_goneRows)
                row->deleteSelf();
            m_goneRows.clear();
        }
    }

    // RowAnswers

    namespace
    {
        RowAnswers::RowAnswers(const CreateParams& params, const bool shown)
            :
            Stack{
                params,
                Orientation::Horizontal,
                VerticalAlign::Center,
                Spacing{ k_rowSpacing },
                Padding{ 0.0f }
            },
            m_shown{ shown }
        {
        }

        void RowAnswers::reveal()
        {
            m_shown = true;
            invalidateState();
            invalidateChildrenStates();
            invalidate();
        }

        void RowAnswers::getControlState(GetStateEvent& event) const
        {
            Stack::getControlState(event);
            if (&event.control == this && !m_shown)
                event.state.enabled = false;
        }

        void RowAnswers::hitTest(HitTestEvent& event) const
        {
            if (m_shown)
                Stack::hitTest(event);
            else
                event.zone = HitTest::Transparent;
        }

        void RowAnswers::paintChildren(PaintEvent& event)
        {
            if (m_shown)
                Stack::paintChildren(event);
        }
    }

    // SourceBox

    namespace
    {
        // Set as the new project dialog's boxes are.
        SourceBox::SourceBox(const CreateParams& params, SourceRow& row, OpenProject& project)
            :
            FolderBox<WordsBox>{
                params,
                Interactivity::Focusable,
                WordWrap::No,
                Padding{ 0.0f },
                VerticalTextAnchor::Center,
                PlaceHolderText{ row.isStub() ? SourcePlaceHolder::stub : SourcePlaceHolder::entry }
            },
            m_row{ row },
            m_project{ project }
        {
            setBaseFolder(m_project.project.folder);
        }

        // **The words are replaced only where they differ**, so a menu opened over a selection in
        // the box keeps the selection.
        void SourceBox::showSource()
        {
            std::wstring shown;
            if (!m_row.isStub())
            {
                const bool left = !isFocused() && !isDroppedDown();
                shown = left
                    ? m_project.project.readSource(m_row.source()).wstring()
                    : m_row.source();
            }
            if (edited() || text().plainText() != shown)
                showWords(Text{ shown });
        }

        std::wstring SourceBox::writeWords(const std::wstring_view words)
        {
            if (!edited())
                return {};
            const std::wstring typed{ trimmed(words) };
            if (!typed.empty())
                return m_row.take(typed);
            if (m_row.isStub())
                showWords(Text{});
            else
                m_row.remove();
            return {};
        }

        // Leaving writes what was typed. An edit written, or none, shows the folder again, and
        // one refused keeps the words typed.
        void SourceBox::focusChanged()
        {
            FolderBox<WordsBox>::focusChanged();
            if (!edited())
                showSource();
        }
    }

    // SourceRow

    namespace
    {
        // The field is the new project dialog's, as the name's is.
        SourceRow::SourceRow(const CreateParams& params, SourceList& list,
            std::wstring source)
            :
            Panel{ params, Spacing{ k_rowSpacing } },
            m_list{ list },
            m_source{ std::move(source) },
            m_field{ createBody<Panel>(
                UiElement::Section,
                Radius{ themeMetrics().button.radius },
                themeMetrics().button.padding,
                Spacing{ k_browseGap },
                MinSize{ k_fieldWidth, 0.0f }
            ) },
            m_box{ m_field.createBody<SourceBox>(*this, list.project()) },
            m_answers{ createRightBar<RowAnswers>(!isStub()) }
        {
            m_field.setBorder(themeMetrics().button.border);
            // As tall as the line it ends, so the field stands as high as the name's.
            m_field.createRightBar<ToolButton>(
                k_browseText,
                VerticalAlign::Center,
                HorizontalTextAnchor::Center,
                VerticalTextAnchor::Center,
                Padding{ k_browsePadding, 0.0f },
                MinSize{ 0.0f },
                Interactivity::MouseOnly,
                HintText{ L"Picks the folder in the desktop's own dialog" },
                Button::OnClick{ [this](ClickEvent&) {
                    browse();
                } }
            );
            m_answers.add<CheckBox>(
                L"Include subfolders",
                VerticalAlign::Center,
                HintText{ L"Reads the folders under this one too" },
                Padding{ 0.0f, themeMetrics().listItem.padding.y },
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
            m_answers.add<ToolButton>(
                L"Remove",
                FixedSize{ k_removeSize },
                ButtonViewMode::IconOnly,
                IconSize{ k_removeMark },
                VerticalAlign::Center,
                Interactivity::MouseOnly,
                ButtonBase::OnPaintIcon{ Icons::XMark::paint },
                HintText{ L"Stops reading this folder" },
                Button::OnClick{ [this](ClickEvent&) {
                    remove();
                } }
            );
            m_box.showSource();
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
            m_box.showSource();
            if (stub)
            {
                m_answers.reveal();
                m_list.addStub();
            }
            m_list.changed();
            return {};
        }

        void SourceRow::remove()
        {
            m_list.workspace().removeSource(m_list.project(), m_source);
            m_list.remove(*this);
            m_list.changed();
        }

        void SourceRow::browse()
        {
            const Project& project = m_list.project().project;
            const std::wstring typed{ trimmed(m_box.text().plainText()) };
            std::filesystem::path start = project.folder;
            if (!typed.empty())
                start = project.readSource(typed);
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
            const std::wstring written = project.writtenSource(*picked);
            const std::wstring refusal = take(written);
            if (refusal.empty())
                return;
            m_box.text() = Text{ written };
            m_box.setRefusal(refusal);
            m_box.showRefusal();
        }
    }
}
