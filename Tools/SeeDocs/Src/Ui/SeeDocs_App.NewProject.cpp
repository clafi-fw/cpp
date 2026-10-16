module SeeDocs_App.NewProject;

import SeeDocs_App.FolderBox;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
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
        // What no folder's name holds, on any desktop SeeDocs runs on.
        constexpr std::wstring_view k_forbidden = L"\\/:*?\"<>|";
        constexpr std::wstring_view k_pickTitle = L"Where to store the project";
        constexpr std::wstring_view k_title = L"New project";
        constexpr std::wstring_view k_titleSeparator = L" - ";

        // The box one line is typed into, in the field that gives it a surface of its own.
        template<typename Box>
        [[nodiscard]] Box& addBox(Panel& field)
        {
            return field.createBody<Box>(
                Interactivity::Focusable,
                WordWrap::No,
                Padding{ 0.0f },
                VerticalTextAnchor::Center
            );
        }
    }

    // NewProjectDialog

    // **The shell is a window's**, framed as the window it stands over is, and the middle and the
    // answers are a message box's. The title is dragged to move it; its one button closes it, the
    // window being whatever its rows come out as and never a size the user picks.
    NewProjectDialog::NewProjectDialog(FormBase& owner, const std::filesystem::path& location,
        const std::wstring_view name, ProjectMaker maker)
        :
        NewProjectForm{
            owner.appContext(),
            WindowRole::Dialog,
            owner,
            owner.themeMetrics().primaryWindow,
            owner.themeMetrics().primaryWindowShadow,
            UiElement::ToolBar,
            Interactivity::MouseOnly,
            Border{ Thickness::Thin },
            Padding{ 0.0f },
            Spacing{ 0.0f }
        },
        m_maker{ std::move(maker) },
        m_title{ createTopBar<DialogTitle>(TitleButtons::Close, Text{ k_title }) },
        m_content{ createBody<Panel>(
            UiElement::Page,
            Radius{ 0.0f },
            Padding{ k_contentPadding }
        ) },
        m_column{ m_content.createBody<StackPanel>(
            Orientation::Vertical,
            Spacing{ k_groupSpacing },
            MinSize{ k_columnWidth, 0.0f }
        ) },
        m_nameGroup{ addGroup(m_column) },
        m_nameCaption{ m_nameGroup.add<Label>(Text{ L"Project name" }) },
        // A field is drawn as an answer is: a surface of its own, at a button's radius.
        m_nameField{ m_nameGroup.add<Panel>(
            UiElement::Section,
            Radius{ owner.themeMetrics().button.radius },
            owner.themeMetrics().button.padding
        ) },
        m_name{ addBox<PromptBox>(m_nameField) },
        m_locationGroup{ addGroup(m_column) },
        m_locationCaption{ m_locationGroup.add<Label>(Text{ L"Where to store the project?" }) },
        m_locationRow{ m_locationGroup.add<Panel>(Spacing{ k_cellSpacing }) },
        m_locationField{ m_locationRow.createBody<Panel>(
            UiElement::Section,
            Radius{ owner.themeMetrics().button.radius },
            owner.themeMetrics().button.padding
        ) },
        // Lists from a root only: the project's place is a path from one.
        m_location{ addBox<FolderBox<PromptBox>>(m_locationField) },
        m_folderGroup{ addGroup(m_column) },
        m_folderCaption{ m_folderGroup.add<Label>(
            Text{ L"The project files will be created in" }
        ) },
        m_folder{ m_folderGroup.add<Label>(WordWrap::Yes, MaxSize{ k_columnWidth, k_maxFloat }) },
        m_refusal{ m_folderGroup.add<Label>(WordWrap::Yes, MaxSize{ k_columnWidth, k_maxFloat }) },
        m_answerBar{ createBottomBar<Panel>(
            UiElement::ToolBar,
            Radius{ 0.0f },
            Padding{ k_barPaddingX, k_barPaddingY }
        ) },
        m_answers{ m_answerBar.createBody<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_answerSpacing },
            ItemSizing::Equal
        ) },
        m_create{ m_answers.add<Button>(
            L"Create",
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            Button::OnGetState{ [this](GetStateEvent& event) {
                event.state.enabled = !typedName().empty();
            } },
            Button::OnClick{ [this](ClickEvent&) {
                create();
            } }
        ) }
    {
        setAutoFit(AutoFit::Yes);
        std::wstring windowTitle{ k_title };
        windowTitle.append(k_titleSeparator);
        windowTitle.append(appContext().appName().plainText());
        setWindowTitle(windowTitle);
        const Thickness border = themeMetrics().button.border;
        m_nameField.setBorder(border);
        m_locationField.setBorder(border);
        m_locationRow.createRightBar<Button>(
            L"Browse",
            EndIcon::OpensWindow,
            VerticalAlign::Center,
            TooltipText{ L"Picks the folder in the desktop's own dialog" },
            Button::OnClick{ [this](ClickEvent&) {
                browse();
            } }
        );
        m_answers.add<Button>(
            L"Cancel",
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            Button::OnClick{ [](ClickEvent& event) {
                event.closeForm();
            } }
        );
        m_name.text() = Text{ name };
        m_location.text() = Text{ location.wstring() };
        m_name.onTextEdit([this](TextEditEvent&) {
            writeFolder();
            m_create.invalidateState();
        });
        m_location.onTextEdit([this](TextEditEvent&) {
            writeFolder();
        });
        m_refusal.setVisible(false);
        writeFolder();
        // The name is what is typed first, and a name offered arrives selected so that typing
        // replaces it.
        m_name.setFocus();
        m_name.selectAll();
    }

    void NewProjectDialog::showOrActivate(const InputStamp stamp)
    {
        if (!visible())
        {
            show();
            return;
        }
        if (isMinimized())
            restore();
        window().setFocus(stamp);
    }

    // A separator ending the folder is looked past, so that the folder and the name join once.
    std::filesystem::path NewProjectDialog::location() const
    {
        std::filesystem::path typed{ std::wstring{ trimmed(m_location.text().plainText()) } };
        typed = typed.lexically_normal();
        if (!typed.has_filename() && typed != typed.root_path())
            typed = typed.parent_path();
        return typed;
    }

    // Return creates from either box; on an answer it runs that answer, as a click would.
    void NewProjectDialog::nestedKeyDown(KeyDownEvent& event)
    {
        const bool inBox = m_name.isFocused() || m_location.isFocused();
        if (event.key == Keys::Return && !event.modifiers.shift && inBox)
        {
            event.handled = true;
            create();
            return;
        }
        NewProjectForm::nestedKeyDown(event);
    }

    StackPanel& NewProjectDialog::addGroup(StackPanel& column)
    {
        return column.add<StackPanel>(Orientation::Vertical, Spacing{ k_rowSpacing }, Padding{ 0.0f });
    }

    std::wstring NewProjectDialog::typedName() const
    {
        return std::wstring{ trimmed(m_name.text().plainText()) };
    }

    // The desktop's dialog opens on the folder the box names, or on the nearest one above it
    // that stands.
    void NewProjectDialog::browse()
    {
        std::filesystem::path start = location();
        std::error_code error;
        while (!start.empty() && !std::filesystem::is_directory(start, error))
        {
            const std::filesystem::path parent = start.parent_path();
            if (parent == start)
                break;
            start = parent;
        }
        const std::optional<std::filesystem::path> picked = Platform::pickFolder(this,
            k_pickTitle, start);
        if (!picked)
            return;
        m_location.text() = Text{ picked->wstring() };
        writeFolder();
    }

    // The folder in spot ink, the way SeeDocs quotes a path; with no name typed yet, the place
    // alone. A reason shown goes with the words it was about.
    void NewProjectDialog::writeFolder()
    {
        const std::wstring name = typedName();
        const std::filesystem::path place = location();
        Text text;
        text << PushThemeColor{ InkWell::spotInk() };
        text << (name.empty() ? place : place / name).wstring();
        text << PopColor{};
        m_folder.text() = text;
        m_refusal.setVisible(false);
        m_folder.invalidateFormAlign();
    }

    // Return asks no more of a dialog with no name typed than the greyed Create does.
    void NewProjectDialog::create()
    {
        const std::wstring name = typedName();
        if (name.empty())
            return;
        const std::filesystem::path folder = location() / name;
        std::wstring refusal = refusalOf(name, folder);
        if (refusal.empty())
            refusal = m_maker(folder, name);
        if (!refusal.empty())
        {
            showRefusal(refusal);
            return;
        }
        close();
    }

    // **An empty folder takes a project**, one standing there before it included; a folder holding
    // anything else is somebody's, and is left to them.
    std::wstring NewProjectDialog::refusalOf(const std::wstring_view name,
        const std::filesystem::path& folder) const
    {
        const bool forbidden = name == L"." || name == L".."
            || std::ranges::any_of(name, [](const wchar_t current) {
                return current < L' ' || k_forbidden.find(current) != std::wstring_view::npos;
            });
        if (forbidden)
        {
            return L"A project's name is its folder's, so it holds none of "
                L"\\ / : * ? \" < > |.";
        }
        const std::filesystem::path place = location();
        if (place.empty() || !place.is_absolute())
            return L"Say where to store the project: a folder on this computer.";
        std::error_code error;
        if (std::filesystem::is_regular_file(folder / k_projectFileName, error))
            return L"A project is stored there already. The Open page opens it.";
        if (std::filesystem::exists(folder, error) && !std::filesystem::is_directory(folder, error))
            return L"A file of that name stands there already.";
        const bool stands = std::filesystem::is_directory(folder, error);
        if (stands && !std::filesystem::is_empty(folder, error))
        {
            return L"That folder stands there already, with files in it. Name the project "
                L"otherwise, or store it elsewhere.";
        }
        return {};
    }

    void NewProjectDialog::showRefusal(const std::wstring_view reason)
    {
        Text text;
        text << InkWell::redInk(InkGrade::Strong) << reason << PopColor{};
        m_refusal.text() = text;
        m_refusal.setVisible(true);
        m_refusal.invalidateFormAlign();
    }
}
