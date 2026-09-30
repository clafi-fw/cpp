export module ClaFi.Documents.Page;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.Utils;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.HistoryButton;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.StdActions;

import ClaFi.Core.DomEngine;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    // One document, edited in place: the file behind the derived page's editor. See Documents
    export class DocumentPage : public DocumentsBasePage
    {
    public:
        template<typename... Args>
        explicit DocumentPage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] bool hasUnsavedEdits() const override;
        [[nodiscard]] bool saveEdits(Control& initiator) const override;
        // The tab config node the document stands in, where every edit lands and stays until
        // Save - so a tab restored from settings comes back holding what was not saved.
        [[nodiscard]] Dom::DomNodeBase& documentNode() const;
        // The file this page's document stands in.
        [[nodiscard]] std::filesystem::path documentFile() const;
        // Reads the saved document into the node - what a tab arriving at this page wants, unless
        // it was restored onto it. Answers whether it could. See Documents#restored
        [[nodiscard]] bool loadDocument() const;
    protected:
        // Reads the saved side of the comparison behind hasUnsavedEdits into the node: the page's
        // own file, unless the derived page has another. Answers whether it could.
        [[nodiscard]] virtual bool readSavedDocument(Dom::DomNodeBase& into) const;
        // Names what Undo and Redo act on wherever the focus stands. See Documents#history
        void setEditHistory(IEditHistory&);
    protected:
        static constexpr IconSize k_toolButtonIconSize{ 18.0f };
    private:
        // Writes the page's work to its own file, and answers whether it got there.
        [[nodiscard]] bool writeDocument() const;
        // Writes this page's work to a name the user gives, and takes the tab there. The original
        // is left on the disk as it stands, which is what tells this from Save.
        void saveAs(Control& initiator);
        // Asks for a name and writes the work to it, and does no more than that. Answers the file
        // name it wrote, or nothing where the user would not give one.
        [[nodiscard]] std::wstring saveAsFile(Control& initiator) const;
        // Puts the name the user typed back to them where a file already stands under it, and
        // answers whether they said to write over it.
        [[nodiscard]] bool confirmReplacing(Control& initiator, std::wstring_view stem) const;
    private:
        // Save on the face, Save as behind the strip. Both are StdActions, so the page answers for
        // them once and the keys reach the same answer the button does. Mouse only, like every
        // button on this bar: a press leaves the caret in the editor.
        SplitButton& m_saveButton{ toolBar().add<SplitButton>(
            StdActions::save,
            ButtonViewMode::LeftIcon,
            k_toolButtonIconSize,
            Interactivity::MouseOnly
        ) };
        Divider& m_dividerAfterSave{ toolBar().add<Divider>(Padding{ 4.0f }) };
        // The strips list the steps the history holds. See Documents#history
        HistoryButton& m_undoButton{ toolBar().add<HistoryButton>(
            HistoryDirection::Undo,
            ButtonViewMode::IconOnly,
            k_toolButtonIconSize,
            Interactivity::MouseOnly
        ) };
        HistoryButton& m_redoButton{ toolBar().add<HistoryButton>(
            HistoryDirection::Redo,
            ButtonViewMode::IconOnly,
            k_toolButtonIconSize,
            Interactivity::MouseOnly
        ) };
        IEditHistory* m_editHistory{ nullptr }; // what Undo and Redo act on, where named
        // Set by Save as, which has just written this page's work under another name. What stands
        // in the view state differs from this page's own file and always will - and nothing is at
        // risk by it: the work is on the disk, and the tab is on its way to where it went.
        bool m_savedUnderAnotherName{ false };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    DocumentPage::DocumentPage(const CreateParams& params, Args&&... args)
        :
        DocumentsBasePage{ params, std::forward<Args>(args)... }
    {
        // The second half of the button IS Save as: behind the strip the command names itself in
        // the strip's tooltip and runs off one press.
        m_saveButton.dropdownAction(StdActions::saveAs);

        onGetActionState([this](GetActionStateEvent& event) {
            // Claiming says this page is what the command acts on. Neither is ever refused: a
            // page with no file of its own still has somewhere to put the work, and Save asks
            // where rather than saying no.
            if (&event.action == &StdActions::save || &event.action == &StdActions::saveAs)
                event.claim({});
        });

        onActionClick([this](ActionClickEvent& event) {
            const bool isSave = &event.action == &StdActions::save;
            if (!isSave && &event.action != &StdActions::saveAs)
                return;

            // Under whatever showed the command. The strip is half of the button, so the whole
            // button carries a question or a message about either half - which is also where the
            // shortcut, shown by nothing, puts it.
            Control& shownBy = event.presenter && !m_saveButton.containsNested(event.presenter)
                ? *event.presenter
                : m_saveButton;

            // SAVE ON A PAGE WITH NO FILE OF ITS OWN IS SAVE AS. The work has somewhere to go and
            // the press said to put it there, so the command that can reach the disk is the one
            // that runs.
            if (isSave && canSaveEdits())
            {
                // A write leaves nothing on screen to say it happened - the page looks exactly as
                // it did - so the command says so itself, beside the button that ran it.
                const std::wstring_view noun = folder().kind().noun;
                ContextMessage::show(shownBy, writeDocument()
                    ? messageText(MessageIcon::Ok, capitalized(noun).append(L" saved"))
                    : messageText(MessageIcon::Error,
                        std::wstring{ L"The " }.append(noun).append(L" could not be saved")));
                return;
            }

            // SAVE AS SAYS NOTHING HERE. It takes the tab to the file it wrote, and the page it
            // arrives at is the answer: a message hung off this button would be raised about a
            // control the rebuild is about to destroy - see saveAs.
            saveAs(shownBy);
        });

        // A page naming its history answers for Undo and Redo, and its buttons reach it first.
        onGetActionState([this](GetActionStateEvent& event) {
            if (!m_editHistory)
                return;
            if (&event.action == &StdActions::undo)
                event.claim({ .enabled = m_editHistory->undoDepth() != 0 });
            else if (&event.action == &StdActions::redo)
                event.claim({ .enabled = m_editHistory->redoDepth() != 0 });
        });

        onActionClick([this](ActionClickEvent& event) {
            if (!m_editHistory)
                return;
            if (&event.action == &StdActions::undo)
                m_editHistory->undo(1);
            else if (&event.action == &StdActions::redo)
                m_editHistory->redo(1);
        });

        connectEvent([this](GetEditHistoryEvent& event) {
            event.history = m_editHistory;
        });
    }
}
