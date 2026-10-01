module ClaFi.Documents.HomePage;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.List;
import ClaFi.Documents.Utils;

import ClaFi.Browser.Actions;
import ClaFi.Browser.Control;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.StackView;

import ClaFi.StdActions;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    void DocumentsHomePageBase::selectItemByPagePath(const std::wstring_view value)
    {
        m_tiles->setCurrentFileName(fileNameOfPage(value));
    }

    // THE TILE THE USER IS ON, which a right click has just moved the current item to, so the
    // browser's Open and Open in new tab are about the tile the menu was raised from as well as
    // about the one the keyboard stands on.
    std::wstring DocumentsHomePageBase::pathToOpen()
    {
        return pagePathOfDocument(m_tiles->currentFileName());
    }

    void DocumentsHomePageBase::connectTiles(IDocumentTiles& tiles)
    {
        m_tiles = &tiles;
        StackView& view = tiles.view();

        // Answered by the page, not by the button: the toolbar button, the Delete key and a menu
        // item carrying the same action all land here, and the selection they act on is this
        // page's.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = !m_tiles->view().selection().empty() });
        });
        onActionClick([this](ActionClickEvent& event) {
            // Under whatever presented the command, so a question about it stands where the user
            // is looking. The Delete key presents nothing, and the toolbar button is where the
            // command lives on this page.
            if (&event.action == &StdActions::del)
                deleteSelectedFiles(event.presenter ? *event.presenter : m_deleteButton);
        });

        view.onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
        });
        view.connectEvent([this](DocumentsRebuiltEvent&) {
            documentsRebuilt();
        });

        // A PRESS ON THE TILE OPENS THE DOCUMENT, AND THE KEYBOARD'S PRESS IS A PRESS. Return and
        // Space on the tile the keyboard stands on arrive as an ordinary click, so the mouse's
        // single click and the keyboard's press are one event, and the form is what tells them
        // apart. A modifier makes the press a selection command rather than an activation.
        view.onClick([this](ClickEvent& event) {
            DocumentTile* tile = m_tiles->currentTile();
            if (!tile || tile != event.control)
                return;
            if (!event.form.isKeyboardClick() || event.modifiers.ctrl || event.modifiers.shift)
                return;
            openDocument(*tile, event.stamp);
        });
        view.onDoubleClick([this](DoubleClickEvent& event) {
            DocumentTile* tile = m_tiles->currentTile();
            if (!tile || tile != event.control)
                return;
            // THE GESTURE IS SPENT HERE. A double click that nothing stopped is read by the form
            // as the press it also is, and that press would act on a tile the navigation is about
            // to take down.
            event.stopPropagation();
            openDocument(*tile, event.stamp);
        });
    }

    DocumentEditHandler DocumentsHomePageBase::editHandler()
    {
        return [this](DocumentTile& tile, AcceptEditEvent& event) {
            acceptTileEdit(tile, event);
        };
    }

    DocumentMenuHandler DocumentsHomePageBase::menuHandler()
    {
        return [this](DocumentTile& tile, ContextPopupEvent& event) {
            showTileMenu(tile, event);
        };
    }

    void DocumentsHomePageBase::documentsRebuilt()
    {
        if (!m_fileNameToRename.empty())
            renameAfterAlign(m_fileNameToRename);
    }

    // ASKED FOR ON A TICK, because the rebuild that created the tile is still on the stack: it
    // holds the list this tile is in, and clears it, and the editor runs a message loop of its
    // own. The tile has no layout yet either, so the editor waits for the form to settle by
    // itself - see WithInPlaceEdit::openEditor.
    void DocumentsHomePageBase::renameAfterAlign(const std::wstring_view fileName)
    {
        m_fileNameToRename = fileName;
        m_renameTimer.start(MilliSeconds{ 0u });
    }

    void DocumentsHomePageBase::renamePendingTile()
    {
        // Found again rather than kept as a pointer: another rebuild may have run in between and
        // taken the tile with it.
        DocumentTile* tile = m_tiles->tileByFileName(m_fileNameToRename);
        m_fileNameToRename.clear();
        if (!tile)
            return;
        // THE FOCUS, not just the current item: the focus is still on the button that made the
        // document, which is where it would go back to when the editor closes.
        tile->setFocus();
        tile->openEditor();
    }

    void DocumentsHomePageBase::acceptTileEdit(DocumentTile& tile, AcceptEditEvent& event)
    {
        const std::wstring newName = folder().renameFile(tile.path(), event);
        if (newName.empty())
            return;
        // THE FILE IS RENAMED AND THIS TILE STILL SAYS OTHERWISE until the folder reports, and
        // the watch behind that waits out a quiet period first. The rebuild replaces this tile
        // with one that reads the same.
        tile.text().clear();
        tile.text() << event.text.plainText();
        // A name is as many lines as it needs inside a fixed width, so a new one is a new height.
        tile.invalidateFormAlign();
        // A RENAME MOVES THE TILE THE LIST REMEMBERS. The file name is the whole of a tile's
        // identity here, so the one to come back to after the rebuild is under the new name.
        m_tiles->selectFileNameAfterRebuild(newName);
    }

    // THE MENU NAMES FOUR COMMANDS AND SETTLES NONE OF THEM. Rename is answered by the tile,
    // against the caption that is the file's name; Delete by the page, against the selection;
    // Open and Open in new tab by the browser, against the path this page names for whichever
    // tile is current.
    void DocumentsHomePageBase::showTileMenu(DocumentTile& tile, ContextPopupEvent& event)
    {
        Menu menu{ tile };
        menu.addToCommandBar(StdActions::rename);
        menu.addToCommandBar(StdActions::del);
        menu.add(Browser::Actions::open);
        menu.add(Browser::Actions::openInNewTab);
        // The tile has answered, so nothing above it raises a second menu behind this one. Said
        // before the menu runs, because a command is free to take this tile down with it.
        event.stopPropagation();
        menu.execute();
    }

    // OPEN IS THE COMMAND, AND THE BROWSER ANSWERS IT, on a wait rather than from inside the
    // press that asked. The tile is the presenter: it is where the command was given.
    void DocumentsHomePageBase::openDocument(Control& presenter, const InputStamp stamp)
    {
        Browser::Actions::open.invoke(form(), &presenter, stamp);
    }

    void DocumentsHomePageBase::createNewFile(const DocumentTemplate* documentTemplate)
    {
        const std::wstring fileName = folder().createFile(documentTemplate);
        // The folder may have been made along with the file, and Open folder reads whether it is
        // there.
        m_exploreButton.invalidateState();
        if (fileName.empty())
            return;

        // A document that has just been made is unnamed in every sense but the file system's, so
        // standing on it and opening an editor over it is the rest of creating it.
        m_tiles->selectFileNameAfterRebuild(fileName);
        m_fileNameToRename = fileName;
    }

    void DocumentsHomePageBase::showTemplates(DropdownEvent& event)
    {
        Menu menu{ event.button() };
        for (const DocumentTemplate& documentTemplate : folder().templates())
        {
            menu.add(documentTemplate.name, [this, &documentTemplate](Control&) {
                createNewFile(&documentTemplate);
            });
        }
        menu.executeUnder(event.button());
    }

    void DocumentsHomePageBase::deleteSelectedFiles(Control& initiator)
    {
        if (!confirmDeleting(initiator))
            return;

        // Taken after the question, so a selection the user kept is left standing as it was.
        m_tiles->selectFileNameAfterRebuild(fileNameAfterDeleting());
        for (Control* item : m_tiles->view().selection())
        {
            std::error_code errorCode;
            std::filesystem::remove(static_cast<DocumentTile*>(item)->path(), errorCode);
        }
    }

    std::wstring DocumentsHomePageBase::fileNameAfterDeleting()
    {
        std::wstring previous{};
        bool seenSelected = false;
        for (DocumentTile* tile : m_tiles->tiles())
        {
            if (m_tiles->view().selection().contains(tile))
            {
                seenSelected = true;
                continue;
            }
            if (seenSelected)
                return tile->fileName();
            previous = tile->fileName();
        }
        return previous;
    }

    bool DocumentsHomePageBase::confirmDeleting(Control& initiator)
    {
        // The one edited document where there is exactly one, so the question can name it.
        // Cleared by the second, which is where a count is all that can be said.
        const DocumentTile* soleEdited{};
        int selectedCount = 0;
        int editedCount = 0;
        for (Control* item : m_tiles->view().selection())
        {
            const DocumentTile* tile = static_cast<const DocumentTile*>(item);
            ++selectedCount;
            if (!folder().isEdited(tile->path()))
                continue;
            soleEdited = editedCount == 0 ? tile : nullptr;
            ++editedCount;
        }

        // Nothing selected carries any work, so there is nothing to lose and nothing to ask.
        if (editedCount == 0)
            return true;

        // ONE QUESTION FOR THE WHOLE SELECTION, AND IT SETTLES ALL OF IT. The first line is what
        // stands to be lost - the one document by its name, several by how many of the selection
        // they are. The second is the question itself. See Documents#questions
        const DocumentKind& kind = folder().kind();
        Text message{};
        if (soleEdited)
        {
            message << documentInQuestionText(soleEdited->stem()) << L" has been edited.";
        }
        else
        {
            const std::wstring subject = std::to_wstring(editedCount)
                .append(L" ")
                .append(kind.nounPlural);
            message << documentInQuestionText(subject)
                << L" of " << selectedCount << L" have been edited.";
        }
        message << k_endLine
            << L"Are you sure you want to permanently delete "
            << (soleEdited ? L"it?" : L"them?");

        // A warning, not a question: the triangle is what says the answer cannot be taken back.
        MessageDialog dialog{
            initiator,
            std::wstring{ L"Delete " }.append(soleEdited ? kind.noun : kind.nounPlural),
            message,
            MessageIcon::Warning
        };
        dialog.add(DialogAnswer::Yes);
        dialog.add(DialogAnswer::No);
        // Dismissed without an answer is no, which is what makes Escape the safe way out of a
        // question about something that cannot be brought back.
        return dialog.execute() == DialogAnswer::Yes;
    }
}
