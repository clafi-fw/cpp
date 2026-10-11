export module SeeDocs_App.NewProject;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// **A new project is a name and a place**: the folder the project is kept in is the place with the
// name under it, made when the user creates it, and the project reads no sources until its
// options page is given some. See README
namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Makes the project in the folder under the name; the reason where it could not be made.
    export using ProjectMaker = std::function<std::wstring(const std::filesystem::path& folder,
        std::wstring_view name)>;

    using NewProjectForm = Form<PanelBase>;

    // **A window of its own** over the one it was raised from, standing until it is answered: it
    // asks for a new project's name and where to store it, and shows the folder the two make as
    // they are typed. Create hands the folder to the maker and closes; a refusal, the dialog's or
    // the maker's, keeps it up with the reason under the folder.
    export class NewProjectDialog : public NewProjectForm
    {
    public:
        NewProjectDialog(FormBase& owner, const std::filesystem::path& location,
            std::wstring_view name, ProjectMaker);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override
        {
            return L"NewProjectDialog";
        }
        // Shows the dialog, or brings it forward where it stands, on the input that asked.
        void showOrActivate(InputStamp);
        // Where the box says projects are stored, as it reads now.
        [[nodiscard]] std::filesystem::path location() const;
    protected:
        void nestedKeyDown(KeyDownEvent&) override;   // Return in a box creates
    private:
        // A question's caption and its answer, one group of the column.
        [[nodiscard]] static Stack& addGroup(Stack& column);
        [[nodiscard]] std::wstring typedName() const;
        void browse();
        void writeFolder();
        void create();
        // Why the project cannot be made under that name in that folder; empty where it can.
        [[nodiscard]] std::wstring refusalOf(std::wstring_view name,
            const std::filesystem::path& folder) const;
        void showRefusal(std::wstring_view reason);
    private:
        static constexpr float k_barPaddingX = 12.0f;
        static constexpr float k_barPaddingY = 8.0f;
        static constexpr float k_contentPadding = 12.0f;
        static constexpr float k_rowSpacing = 6.0f;      // between a caption and its answer
        static constexpr float k_groupSpacing = 16.0f;   // between one question and the next
        static constexpr float k_cellSpacing = 8.0f;
        static constexpr float k_answerSpacing = 8.0f;
        static constexpr float k_columnWidth = 460.0f;
    private:
        ProjectMaker m_maker;
        DialogTitle& m_title;
        Panel& m_content;
        Stack& m_column;
        Stack& m_nameGroup;
        Label& m_nameCaption;
        Panel& m_nameField;
        PromptBox& m_name;
        Stack& m_locationGroup;
        Label& m_locationCaption;
        Panel& m_locationRow;     // the field, and Browse at its end
        Panel& m_locationField;
        PromptBox& m_location;
        Stack& m_folderGroup;   // the folder, and a reason it cannot be made there
        Label& m_folderCaption;
        Label& m_folder;
        Label& m_refusal;
        Panel& m_answerBar;
        Stack& m_answers;
        Button& m_create;   // greyed while no name is typed
    };
}
