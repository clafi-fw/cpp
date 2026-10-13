module SeeDocs_App.ExternalEditors;

import SeeDocs_App.Surface;

import ClaFi.App.AppMenu;
import ClaFi.App.Notices;
import ClaFi.App.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.DropList;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TextItems;

import ClaFi.Icons.Chevron;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer, which is what makes the config's nodes readable as strings.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.CommandRun;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr std::wstring_view k_editorsNodeName = L"ExternalEditors";
        // What each language's command line is kept under, in the editors' section.
        namespace EditorKey
        {
            constexpr std::wstring_view cpp = L"Cpp";
            constexpr std::wstring_view delphi = L"Delphi";
        }
        constexpr std::wstring_view k_commandPlaceHolder = L"A command line with {file} and {line}";
        constexpr std::wstring_view k_filePlaceHolder = L"{file}";
        constexpr std::wstring_view k_linePlaceHolder = L"{line}";
        // The captions' shared width, so the fields line up as the Settings page's rows do.
        constexpr float k_captionWidth = 64.0f;
        constexpr float k_captionSpacing = 3.0f;
        // As wide as the Settings page's theme list may grow, so the group widens nothing.
        constexpr float k_fieldWidth = 360.0f;
        constexpr float k_dropGap = 4.0f;       // between a box and its drop button
        constexpr float k_dropWidth = 24.0f;
        constexpr float k_dropMarkBox = 12.0f;  // the box a combo box's mark is drawn in
        constexpr float k_commandColumn = 130.0f;   // where a listed command starts
        constexpr float k_noLineGap = 16.0f;        // the least room before the no line mark
        constexpr std::wstring_view k_noLineHint =
            L"Opens the file without going to the line - the editor has no switch for one";

        // An editor a box suggests: what it is called, and the command that opens a source in it.
        struct Suggestion
        {
            std::wstring_view name;
            std::wstring_view command;
            std::wstring_view note{};   // the hint beside its line, where it has one to give
        };

        using Suggestions = std::span<const Suggestion>;

        // Each editor once, as the lists suggest it.
        namespace Editors
        {
            constexpr Suggestion vsCode{ L"Visual Studio Code", LR"(code --goto "{file}:{line}")" };
            constexpr Suggestion visualStudio{
                L"Visual Studio",
                LR"(devenv /edit "{file}" /command "Edit.GoTo {line}")",
                L"A running Visual Studio opens the file alone; one started here goes to the line"
            };
            constexpr Suggestion clion64{ L"CLion", LR"(clion64 --line {line} "{file}")" };
            constexpr Suggestion clion{ L"CLion", LR"(clion --line {line} "{file}")" };
            constexpr Suggestion notepadPlusPlus{ L"Notepad++", LR"(notepad++ -n{line} "{file}")" };
            constexpr Suggestion sublimeText{ L"Sublime Text", LR"(subl "{file}:{line}")" };
            constexpr Suggestion cursor{ L"Cursor", LR"(cursor --goto "{file}:{line}")" };
            constexpr Suggestion qtCreator{
                L"Qt Creator",
                LR"(qtcreator -client "{file}:{line}")"
            };
            constexpr Suggestion zed{ L"Zed", LR"(zed "{file}:{line}")" };
            constexpr Suggestion kate{ L"Kate", LR"(kate --line {line} "{file}")" };
            constexpr Suggestion geany{ L"Geany", LR"(geany +{line} "{file}")" };
            constexpr Suggestion gedit{ L"gedit", LR"(gedit +{line} "{file}")" };
            constexpr Suggestion emacs{ L"Emacs", LR"(emacs +{line} "{file}")" };
            constexpr Suggestion gvim{ L"gVim", LR"(gvim +{line} "{file}")" };
            constexpr Suggestion radStudio{ L"RAD Studio", LR"(bds -pDelphi "{file}")" };
            constexpr Suggestion lazarus{ L"Lazarus", LR"(lazarus "{file}")" };
            constexpr Suggestion lazarusIde{ L"Lazarus", LR"(lazarus-ide "{file}")" };
        }

        // What each language suggests on Windows, the most used first.
        namespace WindowsSuggestions
        {
            constexpr std::array cpp{
                Editors::vsCode,
                Editors::visualStudio,
                Editors::clion64,
                Editors::notepadPlusPlus,
                Editors::sublimeText,
                Editors::cursor,
                Editors::qtCreator,
                Editors::zed,
                Editors::gvim
            };
            constexpr std::array delphi{
                Editors::radStudio,
                Editors::vsCode,
                Editors::lazarus,
                Editors::notepadPlusPlus,
                Editors::sublimeText
            };
        }

        // What each language suggests on Linux, the most used first.
        namespace LinuxSuggestions
        {
            constexpr std::array cpp{
                Editors::vsCode,
                Editors::clion,
                Editors::qtCreator,
                Editors::kate,
                Editors::sublimeText,
                Editors::cursor,
                Editors::zed,
                Editors::geany,
                Editors::gedit,
                Editors::emacs,
                Editors::gvim
            };
            constexpr std::array delphi{
                Editors::lazarusIde,
                Editors::vsCode,
                Editors::kate,
                Editors::sublimeText,
                Editors::geany
            };
        }

        // Windows is the desktop whose paths part with a backslash.
        constexpr bool k_onWindows = std::filesystem::path::preferred_separator == L'\\';

        // A language's command line, written to the config as typed, with editors to suggest.
        class CommandBox : public TextBox
        {
        public:
            CommandBox(const CreateParams&, std::wstring_view key, Suggestions, HintText);
        public:
            // Every suggestion under the field, the one the box holds current.
            void dropSuggestions() { list(Listing::Every); }
        protected:
            void charPress(CharPressEvent&) override;     // one line: Return types nothing
            void textEdited() override;
            void focusChanged() override;                 // leaving takes the list down
            void nestedKeyDown(KeyDownEvent&) override;   // the list's keys before the box's
        private:
            // What a drop lists.
            enum class Listing
            {
                Narrowed,   // the suggestions the text starts, none for an empty box
                Every
            };
        private:
            void list(Listing);
            void take(std::size_t line);   // the suggestion on that line, in place of the text
        private:
            std::wstring m_key;                    // under the editors' section
            Suggestions m_suggestions;
            std::vector<std::size_t> m_listed{};   // the suggestions listed, a line each
            DropList m_list;
        };

        // The mark at a field's end that drops its box's suggestions, and takes them down again.
        class DropButton : public ToolButton
        {
        public:
            DropButton(const CreateParams&, CommandBox&);
        public:
            // The box: a press with its list up takes it down. See Control-Foundation#popup-owner
            [[nodiscard]] Control& popupOwner() override { return m_box; }
        protected:
            void paintIcon(PaintIconEvent&) override;   // the mark a combo box's strip wears
        private:
            CommandBox& m_box;
        };

        [[nodiscard]] Dom::DomNodeBase& commandNode(AppContext& context,
            const std::wstring_view key)
        {
            return context.config() / k_editorsNodeName / key;
        }

        [[nodiscard]] Dom::DomNodeBase& commandNode(AppContext& context,
            const SourceLanguage language)
        {
            const std::wstring_view key = language == SourceLanguage::Pascal
                ? EditorKey::delphi
                : EditorKey::cpp;
            return commandNode(context, key);
        }

        void replaceEvery(std::wstring& text, const std::wstring_view what,
            const std::wstring_view replacement)
        {
            std::size_t at = text.find(what);
            while (at != std::wstring::npos)
            {
                text.replace(at, what.size(), replacement);
                at = text.find(what, at + replacement.size());
            }
        }

        // An exit code as it is usually written: in decimal, and in hex where it is large enough to
        // be a Windows status such as 0xC0000005.
        [[nodiscard]] std::wstring spellExitCode(const std::uint32_t exitCode)
        {
            constexpr std::uint32_t k_firstHexCode = 0x10000;
            if (exitCode < k_firstHexCode)
                return std::format(L"{}", exitCode);
            return std::format(L"0x{:08X}", exitCode);
        }

        // The program a command line starts, as a shell reads its first word: up to the closing
        // quote where it opens with one, else up to the first space.
        [[nodiscard]] std::wstring programOf(const std::wstring_view commandLine)
        {
            const std::size_t start = commandLine.find_first_not_of(L" \t");
            if (start == std::wstring_view::npos)
                return {};
            if (commandLine[start] == L'"')
            {
                const std::size_t close = commandLine.find(L'"', start + 1);
                return std::wstring{ commandLine.substr(start + 1, close - start - 1) };
            }
            const std::size_t end = commandLine.find_first_of(L" \t", start);
            return std::wstring{ commandLine.substr(start, end - start) };
        }

        [[nodiscard]] constexpr Suggestions onThisDesktop(const Suggestions windows,
            const Suggestions others)
        {
            return k_onWindows ? windows : others;
        }

        // What is typed starts the command or a word of the name, case aside.
        [[nodiscard]] bool startsSuggestion(const Suggestion& suggestion,
            const std::wstring_view typed)
        {
            if (startsWithFolded(suggestion.command, typed))
                return true;
            std::wstring_view name = suggestion.name;
            while (!startsWithFolded(name, typed))
            {
                const std::size_t space = name.find(L' ');
                if (space == std::wstring_view::npos)
                    return false;
                name.remove_prefix(space + 1);
            }
            return true;
        }

        // The name, the command muted in a column of its own, and a mark where no line is reached.
        [[nodiscard]] DropListLine lineOf(const Suggestion& suggestion)
        {
            DropListLine line;
            line.text << suggestion.name << TabTo{ k_commandColumn };
            line.text << InkGrade::Muted << suggestion.command << PopColor{};
            const bool reachesLine =
                suggestion.command.find(k_linePlaceHolder) != std::wstring_view::npos;
            if (!reachesLine)
            {
                line.text << FlexSpace{ k_noLineGap };
                line.text << InkGrade::Muted << L"no line" << PopColor{};
            }
            if (!suggestion.note.empty())
                line.hint = Text{ suggestion.note };
            else if (!reachesLine)
                line.hint = Text{ k_noLineHint };
            return line;
        }

        // A row: what the language is called, then its command line in a project options field.
        void addEditorRow(Stack& group, const std::wstring_view caption,
            const std::wstring_view key, const Suggestions suggestions, HintText hint)
        {
            Stack& row = group.add<Stack>(Orientation::Horizontal, Spacing{ k_captionSpacing });
            row.add<Label>(
                Text{ caption },
                TextFormat{ InkGrade::Strong },
                MinSize{ k_captionWidth, 0.0f },
                MaxSize{ k_captionWidth, k_maxFloat },
                WordWrap::No,
                VerticalTextAnchor::Center
            );
            Panel& field = row.add<Panel>(
                UiElement::Section,
                Radius{ row.themeMetrics().button.radius },
                row.themeMetrics().button.padding,
                Spacing{ k_dropGap },
                MinSize{ k_fieldWidth, 0.0f },
                MaxSize{ k_fieldWidth, k_maxFloat }
            );
            field.setBorder(row.themeMetrics().button.border);
            CommandBox& box = field.createBody<CommandBox>(key, suggestions, std::move(hint));
            field.createRightBar<DropButton>(box);
        }
    }

    Dom::Dt::Section createExternalEditorsSchema()
    {
        return Dom::Dt::Section{
            k_editorsNodeName,
            Dom::Dt::Value{ EditorKey::cpp, std::wstring{} },
            Dom::Dt::Value{ EditorKey::delphi, std::wstring{} }
        };
    }

    void buildExternalEditors(OptionsPage& page)
    {
        Stack& group = page.addGroup(L"External editors");
        addEditorRow(group, L"C++:", EditorKey::cpp,
            onThisDesktop(WindowsSuggestions::cpp, LinuxSuggestions::cpp),
            HintText{ L"Opens a C++ source at a line - {file} and {line} are filled in" });
        addEditorRow(group, L"Delphi:", EditorKey::delphi,
            onThisDesktop(WindowsSuggestions::delphi, LinuxSuggestions::delphi),
            HintText{ L"Opens a Delphi source at a line - {file} and {line} are filled in" });
    }

    bool hasEditorCommand(AppContext& context, const SourceLanguage language)
    {
        return !commandNode(context, language).get<std::wstring>().empty();
    }

    std::wstring_view editorLanguageName(const SourceLanguage language)
    {
        return language == SourceLanguage::Pascal ? L"Delphi" : L"C++";
    }

    EventConnection onEditorCommandChange(AppContext& context, const SourceLanguage language,
        std::function<void()> callback)
    {
        return commandNode(context, language).connectEvent(
            [callback = std::move(callback)](Dom::ChangeEvent&) {
                callback();
            });
    }

    // openInEditorTitle

    std::wstring openInEditorTitle(const std::filesystem::path& file)
    {
        return std::format(L"{}: {}", k_openInEditorCaption, file.filename().wstring());
    }

    // EditorRuns

    EditorRuns::EditorRuns(AppContext& context)
        :
        m_context{ context }
    {
        for (const SourceLanguage language : { SourceLanguage::Cpp, SourceLanguage::Pascal })
        {
            runOf(language).edited = onEditorCommandChange(m_context, language, [this, language]() {
                runOf(language).notice.withdraw();
            });
        }
    }

    EditorRuns::~EditorRuns() = default;

    // **The command line is the user's own** and runs as a terminal would run it, so a path is
    // quoted where the line quotes {file}, and nowhere else. The run before goes with what it
    // answered - a run is the newest attempt, and its notice speaks for it alone.
    void EditorRuns::open(const SourceLanguage language, const std::filesystem::path& file,
        const std::size_t line)
    {
        std::wstring command = commandNode(m_context, language).get<std::wstring>();
        if (command.empty())
            return;
        replaceEvery(command, k_filePlaceHolder, file.wstring());
        replaceEvery(command, k_linePlaceHolder, std::to_wstring(line));
        Run& run = runOf(language);
        run.notice.withdraw();
        run.program = programOf(command);
        run.title = openInEditorTitle(file);
        run.command = std::make_unique<CommandRun>(command);
        run.command->onDone([this, language](CommandRunEvent& event) {
            answered(language, event.sender());
        });
    }

    EditorRuns::Run& EditorRuns::runOf(const SourceLanguage language)
    {
        return language == SourceLanguage::Pascal ? m_delphi : m_cpp;
    }

    // **Headed with the button and the file**, since the answer may come long after the click. The
    // program is in the spot ink, and Settings is the answer: the command line is where every one
    // of these is put right.
    void EditorRuns::answered(const SourceLanguage language, const CommandRun& command)
    {
        Run& run = runOf(language);
        const std::wstring_view languageName = editorLanguageName(language);
        Text message{};
        switch (command.outcome())
        {
            case CommandOutcome::NotFound:
                message << PushThemeColor{ InkWell::spotInk() } << run.program << PopColor{}
                    << L" was not found. Write its full path in the " << languageName
                    << L" editor's command line, or put its folder on the PATH.";
                break;
            case CommandOutcome::NotRunnable:
                message << PushThemeColor{ InkWell::spotInk() } << run.program << PopColor{}
                    << L" was found, and the system would not run it.";
                break;
            case CommandOutcome::NotStarted:
                message << L"The shell that runs the " << languageName
                    << L" editor's command line could not be started.";
                break;
            case CommandOutcome::Failed:
                message << PushThemeColor{ InkWell::spotInk() } << run.program << PopColor{}
                    << L" ended with exit code " << PushThemeColor{ InkWell::spotInk() }
                    << spellExitCode(command.exitCode()) << PopColor{} << L".";
                break;
            case CommandOutcome::Running:
            case CommandOutcome::Ended:
                return;
        }
        run.notice = notices().post(Notice{
            .icon = MessageIcon::Error,
            .title = run.title,
            .message = std::move(message),
            .answers = {
                NoticeAnswer{
                    .caption = L"Settings",
                    .run = [](ClickEvent& event) {
                        openAppMenuPage(event.form, AppMenu::k_settingsPageCaption, event.stamp);
                    }
                }
            }
        });
    }

    // CommandBox

    namespace
    {
        // Set as the project options' fields are. The list is walked as a folder box's is:
        // nothing is current until Down, and Up from the first line goes back to the box.
        CommandBox::CommandBox(const CreateParams& params, const std::wstring_view key,
            const Suggestions suggestions, HintText hint)
            :
            TextBox{
                params,
                Interactivity::Focusable,
                WordWrap::No,
                Padding{ 0.0f },
                VerticalTextAnchor::Center,
                PlaceHolderText{ k_commandPlaceHolder },
                std::move(hint)
            },
            m_key{ key },
            m_suggestions{ suggestions },
            m_list{
                *this,
                [this](const DropListPick& picked) {
                    take(picked.line);
                },
                DropListOptions{ .upFromFirst = UpFromFirst::Leaves }
            }
        {
            text() = Text{ commandNode(appContext(), m_key).get<std::wstring>() };
        }

        void CommandBox::charPress(CharPressEvent& event)
        {
            if (event.character() == Keys::Return)
                return;
            TextBox::charPress(event);
        }

        // **Typing at the end narrows the list**, as an address bar does. An edit further in
        // corrects what stands, and a list dropped over the box would be in its way.
        void CommandBox::textEdited()
        {
            TextBox::textEdited();
            commandNode(appContext(), m_key).set(text().plainText());
            const TextRange selected = selection();
            const bool atEnd = selected.length == 0
                && selected.start == text().plainText().size();
            if (atEnd)
                list(Listing::Narrowed);
            else
                m_list.hide();
        }

        void CommandBox::focusChanged()
        {
            TextBox::focusChanged();
            if (!isFocused())
                m_list.hide();
        }

        void CommandBox::nestedKeyDown(KeyDownEvent& event)
        {
            switch (event.key)
            {
                case Keys::Return:
                {
                    if (m_list.pickCurrent(event.modifiers, event.stamp))
                    {
                        event.handled = true;
                        return;
                    }
                    m_list.hide();
                    break;
                }
                case Keys::Escape:
                {
                    if (m_list.shown())
                    {
                        event.handled = true;
                        m_list.hide();
                        return;
                    }
                    break;
                }
                case Keys::Up:
                case Keys::Down:
                {
                    const ScrollDirection direction = event.key == Keys::Up
                        ? ScrollDirection::ToBegin
                        : ScrollDirection::ToEnd;
                    if (m_list.moveCurrent(direction))
                    {
                        event.handled = true;
                        return;
                    }
                    // Down drops every suggestion, Alt held or not.
                    if (event.key == Keys::Down)
                    {
                        list(Listing::Every);
                        event.handled = true;
                        return;
                    }
                    break;
                }
                case Keys::Prior:
                case Keys::Next:
                {
                    const ScrollDirection direction = event.key == Keys::Prior
                        ? ScrollDirection::ToBegin
                        : ScrollDirection::ToEnd;
                    if (m_list.moveCurrentByPage(direction))
                    {
                        event.handled = true;
                        return;
                    }
                    break;
                }
                // **Down before the focus moves on**: a list going down hands the focus back to the
                // box, which would take back the step Tab is about to make.
                case Keys::Tab:
                {
                    m_list.hide();
                    break;
                }
                default:
                    break;
            }
            TextBox::nestedKeyDown(event);
        }

        // **A whole command is not narrowed**: the text standing as a suggestion's command lists
        // nothing, which is also what keeps a pick from dropping the list again. The list stands
        // off the field the box fills.
        void CommandBox::list(const Listing listing)
        {
            const std::wstring& typed = text().plainText();
            m_listed.clear();
            std::optional<std::size_t> current;
            for (std::size_t i = 0; i < m_suggestions.size(); ++i)
            {
                const Suggestion& suggestion = m_suggestions[i];
                const bool whole = suggestion.command == typed;
                if (listing == Listing::Narrowed
                    && (typed.empty() || whole || !startsSuggestion(suggestion, typed)))
                    continue;
                if (whole)
                    current = m_listed.size();
                m_listed.push_back(i);
            }
            if (m_listed.empty())
            {
                m_list.hide();
                return;
            }
            DropListLines lines;
            for (const std::size_t index : m_listed)
                lines.push_back(lineOf(m_suggestions[index]));
            m_list.show(std::move(lines), current, FormPlacement::BottomLeft, *parent());
        }

        // An edit as typing is, so Ctrl+Z takes it back and the config is written.
        void CommandBox::take(const std::size_t line)
        {
            if (line >= m_listed.size())
                return;
            selectAll();
            replaceSelectedText(m_suggestions[m_listed[line]].command);
        }
    }

    // DropButton

    namespace
    {
        // **As tall as the box**, filling the line it ends without making the field taller - the
        // least height is the mark's. It takes no focus: a click hands the focus to the box,
        // whose keys walk the list.
        DropButton::DropButton(const CreateParams& params, CommandBox& box)
            :
            ToolButton{
                params,
                L"Popular editors",
                ButtonViewMode::IconOnly,
                IconSize{ k_dropMarkBox },
                Padding{ 0.0f },
                MinSize{ k_dropWidth, 0.0f },
                Interactivity::MouseOnly,
                Button::OnClick{ [&box](ClickEvent&) {
                    box.setFocus();
                    box.dropSuggestions();
                } }
            },
            m_box{ box }
        {
        }

        // In the ink a combo box's mark wears at rest, so the two read as one kind of control.
        void DropButton::paintIcon(PaintIconEvent& event)
        {
            ToolButton::paintIcon(event);
            ChevronPainter::paint(
                event.canvas(),
                event.iconCenter(),
                event.textRgb(InkGrade::Muted),
                0.0f,
                event.scaleFactor()
            );
        }
    }
}
