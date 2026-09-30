module ClaFi.Documents.Page;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.Utils;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.PromptDialog;
import ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.StdActions;

import ClaFi.Core.DomEngine;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Url;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    bool DocumentPage::hasUnsavedEdits() const
    {
        if (m_savedUnderAnotherName)
            return false;
        // THE SAVED DOCUMENT IS THE OTHER SIDE OF THE COMPARISON, not a flag kept as edits arrive.
        // A saved side that cannot be read is work the disk does not hold. See Documents#unsaved
        const std::unique_ptr<Dom::DomNodeBase> saved = documentNode().clone(nullptr);
        if (!readSavedDocument(*saved))
            return true;
        return !Dom::sameValue(documentNode(), *saved);
    }

    bool DocumentPage::saveEdits(Control& initiator) const
    {
        if (canSaveEdits())
        {
            // The leaving question stays up on a failed write, so the reason stands under the
            // answer that was pressed.
            if (writeDocument())
                return true;
            ContextMessage::show(initiator, notSavedText());
            return false;
        }
        // A PAGE WITH NO FILE OF ITS OWN SAVES AS. The work still has to land somewhere and the
        // user is being asked before it is lost, so the command that can keep it is the one to
        // offer. It writes the file and no more: the tab is already on its way somewhere else.
        return !saveAsFile(initiator).empty();
    }

    Dom::DomNodeBase& DocumentPage::documentNode() const
    {
        return tabConfig() / folder().kind().attrName;
    }

    std::filesystem::path DocumentPage::documentFile() const
    {
        return folder().fileOf(pageData().name);
    }

    bool DocumentPage::loadDocument() const
    {
        return readSavedDocument(documentNode());
    }

    bool DocumentPage::readSavedDocument(Dom::DomNodeBase& into) const
    {
        return folder().readDocument(documentFile(), into);
    }

    void DocumentPage::setEditHistory(IEditHistory& history)
    {
        m_editHistory = &history;
        StdActions::undo.invalidateState();
        StdActions::redo.invalidateState();
    }

    bool DocumentPage::writeDocument() const
    {
        // The folder can go while a page stands open on one of its files.
        folder().needDirectory();
        return folder().writeDocument(documentNode(), documentFile());
    }

    Text DocumentPage::notSavedText() const
    {
        return messageText(MessageIcon::Error,
            std::wstring{ L"The " }.append(folder().kind().noun).append(L" could not be saved"));
    }

    void DocumentPage::saveAs(Control& initiator)
    {
        const std::wstring fileName = saveAsFile(initiator);
        if (fileName.empty())
            return;
        // The name the page already has: the file was written over, which is Save, and the tab
        // stands where it stands.
        if (fileName == pageData().name)
            return;

        // Said before the tab moves, and read by the question canLeavePage puts. The work is on
        // the disk under the name that was just given, so there is nothing left to ask about.
        m_savedUnderAnotherName = true;

        // ONLY THE GOING WAITS: taking the tab to what it wrote rebuilds this page, and the frames
        // above the press go on reading the button that goes down with it. The browser holds that
        // wait, because it outlives what the going destroys.
        tab().browserControl().goToLater(Url{ pagePathOfDocument(fileName), tab().anchor() });
    }

    std::wstring DocumentPage::saveAsFile(Control& initiator) const
    {
        const DocumentKind& kind = folder().kind();
        // The prompt opens on the name this page already stands under - the name the user is
        // working from, and the one they are about to vary.
        const std::wstring currentStem = std::filesystem::path{ pageData().name }.stem().wstring();

        PromptDialog dialog{
            initiator,
            std::wstring{ L"Save " }.append(kind.noun).append(L" as"),
            currentStem,
            MessageIcon::Question
        };
        dialog.onAccept([this](AcceptEditEvent& event) {
            const std::wstring stem{ event.text.plainText() };
            folder().checkNameShape(event, stem);
            if (event.refused())
                return;

            // A NAME ALREADY TAKEN IS A QUESTION, NOT A REFUSAL. Writing over a document is a
            // thing the user may well have meant, and the one who typed the name is the only one
            // who can say whether they did.
            std::error_code errorCode;
            if (!std::filesystem::exists(folder().fileOf(folder().fileNameOf(stem)), errorCode))
                return;
            if (!confirmReplacing(event.askedBy, stem))
                event.refuse();
        });
        if (!dialog.execute())
            return {};

        const std::wstring fileName = folder().fileNameOf(dialog.text());
        folder().needDirectory();
        if (!folder().writeDocument(documentNode(), folder().fileOf(fileName)))
        {
            ContextMessage::show(initiator, notSavedText());
            return {};
        }
        return fileName;
    }

    bool DocumentPage::confirmReplacing(Control& initiator, const std::wstring_view stem) const
    {
        // Owned by the answer that asked for the name to be taken, so it stands on top of the
        // naming question with the name the user typed still on screen behind it.
        Text message{};
        message << documentInQuestionText(stem) << L" already exists. Replace it?";

        // A warning, not a question: the triangle is what says the answer cannot be taken back,
        // and what stands under this name now is about to stop existing.
        MessageDialog dialog{
            initiator,
            std::wstring{ L"Replace " }.append(folder().kind().noun),
            message,
            MessageIcon::Warning
        };
        dialog.add(DialogAnswer::Yes);
        dialog.add(DialogAnswer::No);
        // Dismissed without an answer is no, which is what makes Escape the safe way out.
        return dialog.execute() == DialogAnswer::Yes;
    }
}
