module ClaFi.Documents.Page;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Browser;
import ClaFi.Documents.Folder;
import ClaFi.Documents.List;
import ClaFi.Documents.Utils;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Icons.MessageIcons;

import ClaFi.Controls.Button;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.Panel;
import ClaFi.Controls.PromptDialog;
import ClaFi.Controls.TextItems;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Url;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    namespace
    {
        constexpr float k_maxSaveAsFieldWidth = 520.0f;

        class SaveAsDialog final : public PromptDialog
        {
        public:
            SaveAsDialog(Control& owner, std::wstring_view title, std::wstring_view name,
                TextItems&& destinations, ItemIndexValue initialDestination);
        public:
            [[nodiscard]] ItemIndexValue destination() const { return m_destination.itemIndex(); }
        private:
            Panel& m_fields;
            ComboBox& m_destination;
        };

        SaveAsDialog::SaveAsDialog(Control& owner, const std::wstring_view title,
            const std::wstring_view name, TextItems&& destinations,
            const ItemIndexValue initialDestination)
            :
            PromptDialog{ owner, title, name },
            m_fields{ messagePanel().createTopBar<Panel>(Spacing{ 4.0f }) },
            m_destination{ m_fields.createBody<ComboBox>(
                std::move(destinations),
                UiElement::Button,
                ButtonViewMode::LeftIcon,
                ItemsViewMode{ ButtonViewMode::LeftIcon },
                IconSize{ 24.0f },
                ItemsIconSize{ 24.0f },
                ItemIndex{ initialDestination },
                PlaceHolderText{ ColoredText{ InkGrade::Muted, L"Choose a storage" } },
                MinSize{ 300.0f, 0.0f },
                MaxSize{ k_maxSaveAsFieldWidth, k_maxFloat },
                DropdownMaxSize{ 680.0f, 240.0f }
            ) }
        {
            box().setMaxSize({ k_maxSaveAsFieldWidth, k_maxFloat });
            m_fields.createTopBar<Label>(L"Save in");
            m_fields.createBottomBar<Label>(L"Name");
            m_destination.onDrawItemIcon([](DrawItemIconEvent& event) {
                drawStorageIcon(event);
            });
            acceptButton().onGetState([this](GetStateEvent& event) {
                event.state.enabled = destination().has_value();
                event.stopPropagation();
            });
            m_destination.onChange([this](ComboBoxChangeEvent&) {
                acceptButton().invalidateState();
            });
            if (!initialDestination)
            {
                m_destination.makeCurrentItem();
            }
        }
    }

    bool DocumentPage::hasUnsavedEdits() const
    {
        if (m_savedUnderAnotherName)
            return false;
        // **The saved document is the other side of the comparison**, not a flag kept as edits
        // arrive. A saved side that cannot be read is work the disk does not hold.
        // See Documents#documentpage-hasunsavededits
        const std::unique_ptr<Dom::DomNodeBase> saved = documentNode().clone(nullptr);
        if (!readSavedDocument(*saved))
            return true;
        return !Dom::sameValue(documentNode(), *saved);
    }

    bool DocumentPage::saveEdits(Control& initiator) const
    {
        if (canWriteBack())
        {
            // The leaving question stays up on a failed write, so the reason stands under the
            // answer that was pressed.
            if (writeDocument())
                return true;
            ContextMessage::show(initiator, notSavedText());
            return false;
        }
        // Save As writes the copy without navigating because the tab is already leaving this page.
        return !saveAsFile(initiator).empty();
    }

    Dom::DomNodeBase& DocumentPage::documentNode() const
    {
        return tabConfig() / folder().kind().attrName;
    }

    std::filesystem::path DocumentPage::documentFile() const
    {
        return storageFolder().fileOf(pageData().name);
    }

    bool DocumentPage::loadDocument() const
    {
        return readSavedDocument(documentNode());
    }

    bool DocumentPage::readSavedDocument(Dom::DomNodeBase& into) const
    {
        return storageFolder().readDocument(documentFile(), into);
    }

    void DocumentPage::setEditHistory(IEditHistory& history)
    {
        m_editHistory = &history;
        StdActions::undo.invalidateState();
        StdActions::redo.invalidateState();
    }

    bool DocumentPage::canWriteBack() const
    {
        return canSaveEdits() && storageFolder().canWriteFile(documentFile());
    }

    bool DocumentPage::canSaveAs() const
    {
        const auto& browser = static_cast<const DocumentsBrowserBase&>(browserControl());
        return !browser.writableStorages().empty();
    }

    bool DocumentPage::writeDocument() const
    {
        return canWriteBack() && storageFolder().saveDocument(documentNode(), documentFile());
    }

    Text DocumentPage::notSavedText() const
    {
        return Icons::messageText(MessageIcon::Error,
            std::wstring{ L"The " }.append(folder().kind().noun).append(L" could not be saved"));
    }

    void DocumentPage::saveAs(Control& initiator)
    {
        const std::wstring path = saveAsFile(initiator);
        if (path.empty())
            return;
        // The name the page already has: the file was written over, which is Save, and the tab
        // stands where it stands.
        if (path == pageData().path())
            return;

        // Said before the tab moves, and read by the question canLeavePage puts. The work is on
        // the disk under the name that was just given, so there is nothing left to ask about.
        m_savedUnderAnotherName = true;

        // **Only the going waits**: taking the tab to what it wrote rebuilds this page, and the
        // frames above the press go on reading the button that goes down with it. The browser holds
        // that wait, because it outlives what the going destroys.
        tab().browserControl().goToLater(Url{ path, tab().anchor() });
    }

    std::wstring DocumentPage::saveAsFile(Control& initiator) const
    {
        const auto& browser = static_cast<const DocumentsBrowserBase&>(browserControl());
        const DocumentsBrowserBase::WritableStorages storages = browser.writableStorages();
        if (storages.empty())
        {
            ContextMessage::show(initiator, Text{ L"Add or enable a writable storage to save a copy." });
            return {};
        }
        const DocumentKind& kind = folder().kind();
        // The prompt opens on the name this page already stands under - the name the user is
        // working from, and the one they are about to vary.
        const std::wstring currentStem = std::filesystem::path{ pageData().name }.stem().wstring();
        const std::wstring title = std::wstring{ L"Save " }.append(kind.noun).append(L" as");
        TextItems destinations{};
        ItemIndexValue initialDestination{};
        for (DocumentsFolder* storage : storages)
        {
            const std::wstring path = storage->directory().wstring();
            if (storage == &storageFolder() || storages.size() == 1)
            {
                initialDestination = destinations.size();
            }
            destinations.emplace_back(
                Text{ storage->name(), k_endLine, ColoredText{ InkGrade::Muted, path } },
                TooltipText{ path }
            );
        }
        DocumentsFolder* storage{};
        SaveAsDialog dialog{
            initiator,
            title,
            currentStem,
            std::move(destinations),
            initialDestination
        };
        dialog.onAccept([this, &dialog, &storages, &storage](AcceptEditEvent& event) {
            const ItemIndexValue destination = dialog.destination();
            if (!destination)
            {
                event.refuse(L"Choose a storage for the copy.");
                return;
            }
            const std::wstring stem{ event.text.plainText() };
            folder().checkNameShape(event, stem);
            if (event.refused())
                return;

            DocumentsFolder& selected = *storages[*destination];
            const std::filesystem::path path = selected.fileOf(selected.fileNameOf(stem));
            if (!selected.canWriteFile(path))
            {
                event.refuse(L"This file cannot be written.");
                return;
            }

            // **A name already taken is a question, not a refusal.** Writing over a document is a
            // thing the user may well have meant, and the one who typed the name is the only one
            // who can say whether they did.
            std::error_code errorCode;
            if (std::filesystem::exists(path, errorCode) && !confirmReplacing(event.askedBy, stem))
            {
                event.refuse();
                return;
            }
            storage = &selected;
        });
        if (!dialog.execute())
            return {};

        const std::wstring fileName = storage->fileNameOf(dialog.text());
        if (!storage->saveDocument(documentNode(), storage->fileOf(fileName)))
        {
            ContextMessage::show(initiator, notSavedText());
            return {};
        }
        return browser.documentPath(*storage, fileName);
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
