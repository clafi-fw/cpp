module ClaFi.Documents.Browser;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.List;
import ClaFi.Documents.Page;
import ClaFi.Documents.Utils;

import ClaFi.Browser.Consts;
import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.Panel;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Icons.HomeIcon;

import ClaFi.StdActions;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Url;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;
    using namespace Browser;

    // Small enough to sit inside a tab's caption line or a crumb's.
    constexpr IconSize k_pageIconSize{ 16.0f };

    namespace
    {
        constexpr std::wstring_view k_storagePrefix = L"storage-";
        constexpr std::wstring_view k_primaryStorageId = L"storage-primary";
        constexpr std::wstring_view k_storages = L"DocumentStorages";
        constexpr std::wstring_view k_primaryRemoved = L"PrimaryStorageRemoved";
        constexpr std::wstring_view k_primaryReadOnly = L"PrimaryStorageReadOnly";
        constexpr std::wstring_view k_primaryName = L"PrimaryStorageName";
        constexpr std::wstring_view k_nextStorage = L"NextDocumentStorageId";
        constexpr std::wstring_view k_name = L"Name";
        constexpr std::wstring_view k_location = L"Location";
        constexpr std::wstring_view k_readOnly = L"ReadOnly";
        using StorageSettings = Dom::Sequence<Dom::Section>;
        using StoragePathValidator = std::function<std::wstring(std::filesystem::path&)>;
        using StoragePropertiesForm = Form<PanelBase>;

        PromptBox& addStorageField(Panel& panel, const std::wstring_view value)
        {
            return panel.createBody<PromptBox>(
                UiElement::Section,
                Border{ Thickness::Thin },
                Padding{ 6.0f, 4.0f },
                MinSize{ 340.0f, 0.0f },
                WordWrap::No,
                Text{ value }
            );
        }

        class StoragePropertiesDialog final : public StoragePropertiesForm
        {
        public:
            StoragePropertiesDialog(Control& owner, std::wstring_view title,
                const std::filesystem::path& location, std::wstring_view name,
                const std::filesystem::path& startFolder,
                bool readOnly, StoragePathValidator validate);
        public:
            [[nodiscard]] const std::filesystem::path& location() const { return m_acceptedLocation; }
            [[nodiscard]] const std::wstring& customName() const { return m_acceptedName; }
            [[nodiscard]] bool readOnly() const { return m_readOnly.checked() == Checked::Yes; }
            [[nodiscard]] bool readyToClose() override;
        protected:
            void nestedKeyDown(KeyDownEvent&) override;
        private:
            [[nodiscard]] bool hasLocation() const;
            void locationChanged();
            void browse();
            void accept();
        private:
            StoragePathValidator m_validate;
            std::filesystem::path m_startFolder;
            std::filesystem::path m_acceptedLocation{};
            std::wstring m_acceptedName{};
            bool m_useFolderName;
            Panel& m_content;
            Panel& m_nameField;
            PromptBox& m_name;
            Panel& m_locationField;
            Panel& m_locationRow;
            PromptBox& m_location;
            CheckBox2& m_readOnly;
            Button* m_ok{};
            bool m_browsing{ false };
        };

        enum class StorageRemoval
        {
            Cancel,
            Disconnect,
            DeleteFolder
        };

        class StorageRemovalDialog final : public MessageBoxBase
        {
        public:
            StorageRemovalDialog(Control& owner, const DocumentsFolder& storage, bool canDelete);
        public:
            [[nodiscard]] StorageRemoval execute();
        private:
            static void drawDeleteFolder(DrawIconEvent&);
            void select(bool deleteFolder);
        private:
            bool m_deleteFolder{ false };
            StorageRemoval m_answer{ StorageRemoval::Cancel };
            RadioButton* m_disconnect{};
            RadioButton* m_delete{};
        };

        StoragePropertiesDialog::StoragePropertiesDialog(Control& owner,
            const std::wstring_view title, const std::filesystem::path& location,
            const std::wstring_view name,
            const std::filesystem::path& startFolder, const bool readOnly,
            StoragePathValidator validate)
            :
            StoragePropertiesForm{
                owner.appContext(),
                WindowRole::Menu,
                &owner,
                UiElement::Menu,
                ClickTarget::Self,
                MouseFocus::Keep,
                Padding{ 0.0f },
                Spacing{ 0.0f }
            },
            m_validate{ std::move(validate) },
            m_startFolder{ startFolder },
            m_useFolderName{ name.empty() },
            m_content{ createBody<Panel>(UiElement::Page, Padding{ 12.0f }, Spacing{ 8.0f }) },
            m_nameField{ m_content.createTopBar<Panel>(Spacing{ 4.0f }) },
            m_name{ addStorageField(m_nameField, name.empty() ? storageName(location) : name) },
            m_locationField{ m_content.createBody<Panel>(Spacing{ 4.0f }) },
            m_locationRow{ m_locationField.createBody<Panel>(Spacing{ 8.0f }) },
            m_location{ addStorageField(m_locationRow, location.wstring()) },
            m_readOnly{ m_content.createBottomBar<CheckBox2>(
                L"Read-only", readOnly ? Checked::Yes : Checked::No) }
        {
            setAutoFit(AutoFit::Yes);
            setDropdownClearance(1.0f);
            setPlacement(FormPlacement::BottomLeft, owner.boundsInForm());
            setWindowTitle(title);
            createTopBar<DialogTitle>(TitleButtons::Close, Text{ title });
            m_nameField.createTopBar<Label>(L"Name");
            m_locationField.createTopBar<Label>(L"Location");
            m_locationRow.createRightBar<Button>(
                L"Browse...",
                OnClick{ [this](ClickEvent&) {
                    browse();
                } }
            );

            StackPanel& answers = createBottomBar<StackPanel>(
                Orientation::Horizontal,
                ItemSizing::Equal,
                Spacing{ 8.0f },
                Padding{ 12.0f, 8.0f }
            );
            m_ok = &answers.add<Button>(L"OK", HorizontalTextAnchor::Center);
            m_ok->onGetState([this](GetStateEvent& event) {
                event.state.enabled = hasLocation();
                event.stopPropagation();
            });
            m_ok->onClick([this](ClickEvent&) {
                accept();
            });
            answers.add<Button>(L"Cancel", HorizontalTextAnchor::Center,
                OnClick{ [](ClickEvent& event) {
                    event.closeForm();
                } });
            m_name.onTextEdit([this](TextEditEvent&) {
                m_useFolderName = m_name.text().plainText().find_first_not_of(L" \t\r\n")
                    == std::wstring_view::npos;
            });
            m_location.onTextEdit([this](TextEditEvent&) {
                locationChanged();
            });
            PromptBox& first = location.empty() ? m_location : m_name;
            first.makeCurrentItem();
            first.selectAll();
        }

        bool StoragePropertiesDialog::readyToClose()
        {
            return !m_browsing && StoragePropertiesForm::readyToClose();
        }

        void StoragePropertiesDialog::nestedKeyDown(KeyDownEvent& event)
        {
            if (event.key == Keys::Return && !event.modifiers.shift
                && (m_name.isFocused() || m_location.isFocused()))
            {
                event.handled = true;
                accept();
                return;
            }
            StoragePropertiesForm::nestedKeyDown(event);
        }

        bool StoragePropertiesDialog::hasLocation() const
        {
            return m_location.text().plainText().find_first_not_of(L" \t\r\n") != std::wstring_view::npos;
        }

        void StoragePropertiesDialog::locationChanged()
        {
            if (m_useFolderName)
            {
                const std::filesystem::path location{ m_location.text().plainText() };
                m_name.text() = Text{ storageName(location) };
                m_name.invalidateFormAlign();
            }
            m_ok->invalidateState();
        }

        void StoragePropertiesDialog::browse()
        {
            std::filesystem::path start{ m_location.text().plainText() };
            std::error_code error;
            if (!std::filesystem::is_directory(start, error))
            {
                start = m_startFolder;
            }
            start.make_preferred();
            // The native picker changes the owner window's focus while the popup is still in use.
            m_browsing = true;
            const std::optional<std::filesystem::path> picked = Platform::pickFolder(
                &rootForm(), L"Select storage folder", start);
            m_browsing = false;
            if (picked)
            {
                m_location.text() = Text{ picked->wstring() };
                m_location.setRefusal({});
                m_location.invalidateFormAlign();
                locationChanged();
            }
        }

        void StoragePropertiesDialog::accept()
        {
            if (!hasLocation())
            {
                return;
            }
            const std::wstring_view text = m_location.text().plainText();
            if (text.find_first_of(L"\r\n") != std::wstring_view::npos
                || text.find(L'\0') != std::wstring_view::npos)
            {
                m_location.setRefusal(L"Enter a single folder location.");
                m_location.showRefusal();
                return;
            }
            std::filesystem::path path{ text };
            const std::wstring error = m_validate(path);
            if (!error.empty())
            {
                m_location.setRefusal(error);
                m_location.showRefusal();
                return;
            }
            const std::wstring_view name = m_name.text().plainText();
            if (!m_useFolderName && (name.find_first_of(L"\r\n") != std::wstring_view::npos
                || name.find(L'\0') != std::wstring_view::npos))
            {
                m_name.setRefusal(L"Enter a name on one line.");
                m_name.showRefusal();
                return;
            }
            if (!m_useFolderName)
            {
                m_acceptedName = name;
            }
            m_acceptedLocation = std::move(path);
            close();
        }

        StorageRemovalDialog::StorageRemovalDialog(Control& owner, const DocumentsFolder& storage,
            const bool canDelete)
            :
            MessageBoxBase{ owner, L"Remove storage", Text{ storage.directory().wstring() },
                drawDeleteFolder }
        {
            StackPanel& choices = messagePanel().createBottomBar<StackPanel>(
                Orientation::Vertical, Spacing{ 4.0f });
            const std::wstring label = std::wstring{ L"Remove " }.append(storage.name());
            m_disconnect = &choices.add<RadioButton>(
                Text{ label, L" from ", owner.appContext().appName().plainText() }
            );
            m_delete = &choices.add<RadioButton>(
                Text{ label, L" and delete all its content from disk" }
            );
            m_disconnect->onGetState([this](GetStateEvent& event) {
                event.state.selected = !m_deleteFolder;
                event.stopPropagation();
            });
            m_disconnect->onClick([this](ClickEvent&) {
                select(false);
            });
            m_delete->onGetState([this, canDelete](GetStateEvent& event) {
                event.state.selected = m_deleteFolder;
                event.state.enabled = canDelete;
                event.stopPropagation();
            });
            m_delete->onClick([this, canDelete](ClickEvent&) {
                if (canDelete)
                {
                    select(true);
                }
            });
            addAnswer(DialogAnswer::Ok, [this](ClickEvent& event) {
                m_answer = m_deleteFolder ? StorageRemoval::DeleteFolder : StorageRemoval::Disconnect;
                event.closeForm();
            });
            addAnswer(DialogAnswer::Cancel, [](ClickEvent& event) {
                event.closeForm();
            });
            m_disconnect->makeCurrentItem();
        }

        StorageRemoval StorageRemovalDialog::execute()
        {
            runDialog();
            return m_answer;
        }

        void StorageRemovalDialog::drawDeleteFolder(DrawIconEvent& event)
        {
            drawStorageIcon(event);
            const FloatRect rect = event.iconRect();
            const float size = std::min(rect.width(), rect.height());
            const FloatPoint mark = { rect.left + size * 0.75f, rect.top + size * 0.73f };
            const float half = size * 0.18f;
            const float stroke = event.scaledStrokeWidth(Thickness::Thin);
            Graphics::PixelPath cross;
            cross.moveTo(mark.x - half, mark.y - half);
            cross.lineTo(mark.x + half, mark.y + half);
            cross.moveTo(mark.x + half, mark.y - half);
            cross.lineTo(mark.x - half, mark.y + half);
            event.canvas().drawPath(cross,
                { Graphics::PathDrawLayer::Stroke(event.surfaceRgb(), stroke * 3.0f) });
            event.canvas().drawPath(cross,
                { Graphics::PathDrawLayer::Stroke(event.inkRgb(InkWell::Red), stroke) });
        }

        void StorageRemovalDialog::select(const bool deleteFolder)
        {
            m_deleteFolder = deleteFolder;
            m_disconnect->invalidateState();
            m_delete->invalidateState();
        }

        class BrowserStorage final : public DocumentsFolder
        {
        public:
            BrowserStorage(DocumentsFolder& source, const std::filesystem::path&, bool readOnly);
        public:
            void checkNameShape(AcceptEditEvent&, std::wstring_view stem) const override;
            bool readDocument(const std::filesystem::path&, Dom::DomNodeBase&) const override;
            bool writeDocument(const Dom::DomNodeBase&, const std::filesystem::path&) const override;
            bool isEdited(const std::filesystem::path&) const override;
            void drawIcon(std::wstring_view fileName, DrawIconEvent&) override;
        private:
            DocumentsFolder& m_source;
        };

        BrowserStorage::BrowserStorage(DocumentsFolder& source, const std::filesystem::path& path,
            const bool readOnly)
            :
            DocumentsFolder{ path, source.kind() },
            m_source{ source }
        {
            setReadOnly(readOnly);
        }

        void BrowserStorage::checkNameShape(AcceptEditEvent& event, const std::wstring_view stem) const
        {
            m_source.checkNameShape(event, stem);
        }

        bool BrowserStorage::readDocument(const std::filesystem::path& path,
            Dom::DomNodeBase& document) const
        {
            return m_source.readDocument(path, document);
        }

        bool BrowserStorage::writeDocument(const Dom::DomNodeBase& document,
            const std::filesystem::path& path) const
        {
            return canWriteFile(path) && m_source.writeDocument(document, path);
        }

        bool BrowserStorage::isEdited(const std::filesystem::path& path) const
        {
            return m_source.isEdited(path);
        }

        void BrowserStorage::drawIcon(const std::wstring_view fileName, DrawIconEvent& event)
        {
            m_source.drawIcon(fileName, event);
        }

        [[nodiscard]] bool sameDirectory(const std::filesystem::path& left,
            const std::filesystem::path& right)
        {
            std::error_code error;
            return left == right || std::filesystem::equivalent(left, right, error);
        }

        [[nodiscard]] bool isStorageId(const std::wstring_view id)
        {
            if (id == k_primaryStorageId)
            {
                return true;
            }
            if (!id.starts_with(k_storagePrefix) || id.size() == k_storagePrefix.size())
            {
                return false;
            }
            return std::ranges::all_of(id.substr(k_storagePrefix.size()), [](const wchar_t digit) {
                return digit >= L'0' && digit <= L'9';
            });
        }
    }

    DocumentsBrowserBase::~DocumentsBrowserBase()
    {
        // The pages go while the browser still stands: a page reaches the tab it was built on
        // from its own destructor.
        clearControls();
    }

    void DocumentsBrowserBase::initialize(BrowserSettings& settings)
    {
        Dom::Section& config = settings.appConfig();
        const Dom::DomNodeBase* stored = config.child(k_storages);
        const bool primaryReadOnly = stored && (config / k_primaryReadOnly).get<bool>();
        const std::wstring primaryName = stored ? (config / k_primaryName).get<std::wstring>()
            : std::wstring{};
        m_storages.push_back({
            .id = std::wstring{ k_primaryStorageId },
            .folder = makeStorage(m_folder.directory(), primaryReadOnly || m_folder.readOnly(),
                primaryName)
        });
        m_missingStorage = makeStorage({}, true);
        m_missingStorage->detach();

        if (stored)
        {
            if ((config / k_primaryRemoved).get<bool>())
            {
                m_storages.front().folder->detach();
            }
            m_nextStorageId = (config / k_nextStorage).get<std::size_t>();
            for (const Dom::Section& entry : stored->as<StorageSettings>())
            {
                std::wstring id = (entry / ConfigNames::Id).get<std::wstring>();
                std::wstring location = (entry / k_location).get<std::wstring>();
                if (location.empty())
                {
                    location = (entry / ConfigNames::Path).get<std::wstring>();
                }
                const std::filesystem::path path{ location };
                if (!isStorageId(id) || storageEntry(id) || !path.is_absolute())
                {
                    continue;
                }
                const bool duplicatePath = std::ranges::any_of(m_storages, [&path](const StorageEntry& item) {
                    return item.folder->available() && sameDirectory(item.folder->directory(), path);
                });
                if (duplicatePath)
                {
                    continue;
                }
                m_storages.push_back({
                    .id = std::move(id),
                    .folder = makeStorage(path, (entry / k_readOnly).get<bool>(),
                        (entry / k_name).get<std::wstring>())
                });
            }
        }
        collectStorageTiles();
        m_storageRefresh.onTick([this](TimerEvent&) {
            refreshStoragePages();
        });
        BrowserControl::initialize(settings);
    }

    Dom::Dt::Section DocumentsBrowserBase::storageSettingsSchema()
    {
        using namespace Dom::Dt;
        return {
            Value{ k_primaryRemoved, false },
            Value{ k_primaryReadOnly, false },
            Value{ k_primaryName, L"" },
            Value{ k_nextStorage, std::size_t{ 1 } },
            Sequence{
                k_storages,
                Section{
                    Value{ ConfigNames::Id, L"" },
                    Value{ k_name, L"" },
                    Value{ k_location, L"" },
                    Value{ ConfigNames::Path, L"" },
                    Value{ k_readOnly, false }
                }
            }
        };
    }

    DocumentsFolder& DocumentsBrowserBase::storageFolder(const PageData& data) const
    {
        const PageData* root = &data;
        while (root->parent && root->parent->parent)
        {
            root = root->parent;
        }
        const StorageEntry* entry = storageEntry(isStoragePage(*root) ? root->name : k_primaryStorageId);
        return entry ? *entry->folder : *m_missingStorage;
    }

    DocumentsBrowserBase::WritableStorages DocumentsBrowserBase::writableStorages() const
    {
        WritableStorages result{};
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder->canWrite())
            {
                result.push_back(entry.folder.get());
            }
        }
        return result;
    }

    std::wstring DocumentsBrowserBase::documentPath(const DocumentsFolder& storage,
        const std::wstring_view fileName) const
    {
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder.get() == &storage && storage.available() && !fileName.empty())
            {
                return pagePathOfDocument(entry.id).append(L"/").append(fileName);
            }
        }
        return {};
    }

    void DocumentsBrowserBase::editStorage(Control& owner, DocumentsFolder* storage)
    {
        const std::filesystem::path path = storage ? storage->directory() : std::filesystem::path{};
        StoragePropertiesDialog dialog{
            owner,
            storage ? L"Storage properties" : L"Add storage",
            path,
            storage ? std::wstring_view{ storage->customName() } : std::wstring_view{},
            m_folder.directory(),
            storage && storage->readOnly(),
            [this, storage](std::filesystem::path& selected) {
                return storagePathError(selected, storage);
            }
        };
        dialog.execute();
        if (dialog.location().empty())
        {
            return;
        }

        if (storage && sameDirectory(storage->directory(), dialog.location()))
        {
            storage->setName(dialog.customName());
            storage->setReadOnly(dialog.readOnly());
            for (const StorageEntry& entry : m_storages)
            {
                if (entry.folder.get() == storage)
                {
                    if (PageData* page = homePageData().findSubItem(entry.id))
                    {
                        setPageTitle(*page, storage->name());
                    }
                    break;
                }
            }
            storageChanged({});
            return;
        }
        const std::wstring id = addStorage(dialog.location(), dialog.readOnly(), dialog.customName());
        if (storage)
        {
            storage->detach();
        }
        storageChanged(id);
    }

    void DocumentsBrowserBase::removeStorage(Control& owner, DocumentsFolder& storage)
    {
        if (!storage.available())
        {
            return;
        }
        StorageRemovalDialog dialog{ owner, storage, !storageToDelete(storage).empty() };
        const StorageRemoval answer = dialog.execute();
        if (answer == StorageRemoval::Cancel)
        {
            return;
        }
        if (answer == StorageRemoval::DeleteFolder)
        {
            const std::filesystem::path path = storageToDelete(storage);
            if (path.empty())
            {
                ContextMessage::show(owner, Text{ L"This folder is unavailable for deletion." });
                return;
            }
            std::error_code error;
            std::filesystem::remove_all(path, error);
            if (error)
            {
                ContextMessage::show(owner, Text{ L"Could not delete the folder and all its contents." });
                return;
            }
        }
        storage.detach();
        storageChanged({});
    }

    std::optional<std::wstring> DocumentsBrowserBase::canonicalPagePath(
        const std::wstring_view path) const
    {
        if (path == ConfigNames::HomePath)
        {
            return std::wstring{};
        }
        if (path.starts_with(ConfigNames::HomePath)
            && path.find(L'/', ConfigNames::HomePath.size()) == std::wstring_view::npos
            && path.ends_with(m_folder.kind().extension))
        {
            return pagePathOfDocument(k_primaryStorageId).append(path);
        }
        return std::nullopt;
    }

    void DocumentsBrowserBase::initPageData(PageData& data)
    {
        if (isHomePage(data))
        {
            data.title = appContext().appName().plainText();
            // Said here because this runs for every page as it is built, well before anything
            // asks for the list itself. It is what lets the home crumb carry a strip from the
            // start, while a document page carries none.
            data.fetchState = FetchState::HasChildren;
        }
        else if (isStoragePage(data))
        {
            const DocumentsFolder& storage = storageFolder(data);
            if (!storage.available())
            {
                data.title = L"Unavailable storage";
            }
            else
            {
                data.title = storage.name();
            }
            data.fetchState = FetchState::HasChildren;
        }
        else if (isDocumentPage(data))
        {
            // The stem, whatever the extension: a derived browser's documents may carry another.
            data.title = std::filesystem::path{ data.name }.stem().wstring();
        }
    }

    void DocumentsBrowserBase::fetchSubItems(PageData& data)
    {
        // A document page is a leaf, so the home page is the only one with a list to give.
        if (isHomePage(data))
        {
            for (const StorageTile& storage : m_storageTiles)
            {
                addSubItem(data, storage.id);
            }
            return;
        }
        else if (!isStoragePage(data))
        {
            return;
        }

        // The page is left unmarked, so this is asked again on every drop: the folder is written
        // to while the application is up, and a file added, renamed or deleted since the last
        // drop belongs in the answer. What is named here is the whole list.
        for (const std::filesystem::path& file : storageFolder(data).files())
            addSubItem(data, file.filename().wstring());
    }

    IconSize DocumentsBrowserBase::pageIconSize(PageData& data)
    {
        return isHomePage(data) || isStoragePage(data) || isDocumentPage(data)
            ? k_pageIconSize : IconSize{ 0.0f };
    }

    void DocumentsBrowserBase::drawPageIcon(PageData& data, DrawIconEvent& event)
    {
        if (isHomePage(data))
            Icons::HomeIcon::drawBlock(event);
        else if (isStoragePage(data))
            drawStorageIcon(event);
        else if (isDocumentPage(data))
            storageFolder(data).drawIcon(data.name, event);
    }

    bool DocumentsBrowserBase::canLeavePage(BrowserTab& tab, Control& initiator)
    {
        const DocumentsBasePage* page = static_cast<const DocumentsBasePage*>(tab.page());
        // A tab restored from settings and never opened has no page.
        if (!page || !page->hasUnsavedEdits())
            return true;

        Text message{};
        message << documentInQuestionText(tab.pageData()->displayTitle())
            << L" has changes that are not saved.";
        // Under whatever leaving was asked from - the crumb, the Up button, the tab being closed -
        // so the question stands where the user is looking.
        MessageDialog dialog{ initiator, L"Unsaved changes", message, MessageIcon::Question };
        dialog.add(DialogAnswer::Save);
        dialog.add(DialogAnswer::Discard);
        dialog.add(DialogAnswer::Cancel);
        // Answered while the dialog is still standing: a save that did not reach the disk leaves
        // the question up, with what is being left still on screen behind it.
        dialog.onAnswer([page](DialogAnswerEvent& event) {
            if (event.answer == DialogAnswer::Save && !page->saveEdits(event.button))
                event.keepOpen();
        });
        const OptionalDialogAnswer answer = dialog.execute();

        // Save got here only by having saved. Discard is the only other way out - Cancel and a
        // dialog dismissed without an answer at all both mean stay.
        return answer == DialogAnswer::Save || answer == DialogAnswer::Discard;
    }

    bool DocumentsBrowserBase::canRenamePage(PageData& data)
    {
        return storageFolder(data).canWriteFile(documentFileOf(data));
    }

    std::wstring DocumentsBrowserBase::renamePage(PageData& data, AcceptEditEvent& event)
    {
        // Asked again as the name is taken, rather than trusted from the crumb that offered the
        // editor: the seconds an editor stands open are seconds for the file to have gone.
        const std::filesystem::path file = documentFileOf(data);
        if (file.empty())
        {
            event.refuse(std::wstring{ L"That " }.append(m_folder.kind().noun)
                .append(L" is no longer there."));
            return {};
        }
        // A document page is named by its file, so the rename settles the page's new name as
        // well - the extension is the file's to give, and it is not what the user typed.
        return storageFolder(data).renameFile(file, event);
    }

    void DocumentsBrowserBase::showPage(BrowserTab& tab)
    {
        TagValue needTag = 0ull;
        const PageData& data = *tab.pageData();
        DocumentsBasePage* tabPage{};
        if (tab.page())
        {
            tabPage = static_cast<DocumentsBasePage*>(tab.page());
        }
        if (isDocumentPage(data))
        {
            const bool hasRetainedDocument = tab.isOnRestoredPage()
                || (tabPage && &tabPage->pageData() == &data && tabPage->tag().value == k_documentPageTag);
            const bool filePage = data.name.ends_with(m_folder.kind().extension);
            needTag = storageFolder(data).available() || !filePage || hasRetainedDocument
                ? k_documentPageTag : k_homePageTag;
        }
        else if (isHomePage(data) || isStoragePage(data))
        {
            needTag = k_homePageTag;
        }

        std::wstring pathToSelect{};
        if (tabPage)
        {
            const TagValue tabTag = tabPage->tag().value;
            // **A page is built for one page data and holds it by reference**, so a tab that has
            // moved to a different one needs a new page even where the kind of page is the same.
            // Kept, the editor would go on showing the document it was built for, and saving to
            // that document's file, while the caption, the path and the tab all named the one
            // chosen.
            const bool pageDataChanged = &tabPage->pageData() != tab.pageData();
            if (tabTag != needTag || pageDataChanged)
            {
                // Only the home page lands on an item, and only it reads this.
                if ((tabTag == k_documentPageTag || isStoragePage(tabPage->pageData()))
                    && needTag == k_homePageTag
                    && tabPage->pageData().parent == tab.pageData())
                    pathToSelect = tabPage->pageData().path();
                // The edits that were in the page have already been dealt with: canLeavePage put
                // them to the user before anything moved.
                tabPage->deleteSelf();
                tab.setPage(nullptr);
                tabPage = nullptr;
            }
        }

        if (!tab.page())
        {
            switch (needTag)
            {
                case k_homePageTag:
                {
                    DocumentsHomePageBase& homePage = createHomePage(tab);
                    // The page a tab is coming up from is the item to land on.
                    if (!pathToSelect.empty())
                        homePage.selectItemByPagePath(pathToSelect);
                    tabPage = &homePage;
                    break;
                }
                case k_documentPageTag:
                {
                    DocumentPage& documentPage = createDocumentPage(tab);
                    // The page reads its document out of the tab's view state, so the saved one
                    // is read into it first - unless the tab was restored onto this page, in
                    // which case the view state is the work that has not reached the file yet.
                    // See Documents#restored
                    if (!tab.isOnRestoredPage())
                        static_cast<void>(documentPage.loadDocument());
                    tabPage = &documentPage;
                    break;
                }
                default:
                    break;
            }
            if (tabPage)
                tabPage->restoreViewState();
        }

        tab.setPage(tabPage);
        // The page is what the icon comes from once there is one, and the tab last asked before
        // it existed.
        tab.updateIconMode();
        if (tabPage)
            tabPage->show();
    }

    IconSize DocumentsBrowserBase::tabIconSize(BrowserTab& tab)
    {
        const PageData* data = tab.pageData();
        // A tab with nothing to show keeps its caption against the tab's left edge.
        if (!data || !(isHomePage(*data) || isStoragePage(*data) || isDocumentPage(*data)))
            return IconSize{ 0.0f };
        return k_pageIconSize;
    }

    void DocumentsBrowserBase::drawTabIcon(BrowserTab& tab, DrawIconEvent& event)
    {
        const PageData* data = tab.pageData();
        if (!data)
            return;
        if (isHomePage(*data))
            Icons::HomeIcon::drawBlock(event);
        else if (isStoragePage(*data))
            drawStorageIcon(event);
        else if (isDocumentPage(*data))
            storageFolder(*data).drawIcon(data->name, event);
    }

    bool DocumentsBrowserBase::isHomePage(const PageData& data)
    {
        return data.parent == nullptr;
    }

    DocumentStorageView DocumentsBrowserBase::listingView(const PageData& data) const
    {
        if (isHomePage(data))
        {
            return { m_missingStorage.get(), true };
        }
        return { &storageFolder(data), false };
    }

    const StorageTiles* DocumentsBrowserBase::storageTilesFor(const PageData& data) const
    {
        return isHomePage(data) ? &m_storageTiles : nullptr;
    }

    bool DocumentsBrowserBase::isDocumentPage(const PageData& data) const
    {
        return data.parent && (isHomePage(*data.parent) || isStoragePage(*data.parent))
            && data.name.ends_with(m_folder.kind().extension);
    }

    std::filesystem::path DocumentsBrowserBase::documentFileOf(const PageData& data) const
    {
        if (!isDocumentPage(data))
            return {};
        const DocumentsFolder& storage = storageFolder(data);
        if (!storage.available())
        {
            return {};
        }
        std::filesystem::path result = storage.fileOf(data.name);
        std::error_code errorCode;
        if (!std::filesystem::is_regular_file(result, errorCode))
            return {};
        return result;
    }

    bool DocumentsBrowserBase::isStoragePage(const PageData& data)
    {
        return data.parent && isHomePage(*data.parent) && isStorageId(data.name);
    }

    const DocumentsBrowserBase::StorageEntry* DocumentsBrowserBase::storageEntry(
        const std::wstring_view id) const
    {
        const StorageEntries::const_iterator entry =
            std::ranges::find(m_storages, id, &StorageEntry::id);
        return entry == m_storages.end() ? nullptr : &*entry;
    }

    std::unique_ptr<DocumentsFolder> DocumentsBrowserBase::makeStorage(
        const std::filesystem::path& path, const bool readOnly, const std::wstring_view name) const
    {
        auto storage = std::make_unique<BrowserStorage>(m_folder, path, readOnly);
        storage->setName(name);
        return storage;
    }

    std::wstring DocumentsBrowserBase::addStorage(const std::filesystem::path& path,
        const bool readOnly, const std::wstring_view name)
    {
        std::wstring id{};
        do
        {
            id.assign(k_storagePrefix).append(std::to_wstring(m_nextStorageId++));
        }
        while (storageEntry(id));
        m_storages.push_back({ .id = id, .folder = makeStorage(path, readOnly, name) });
        return id;
    }

    void DocumentsBrowserBase::storeStorages()
    {
        Dom::Section& config = settings().appConfig();
        Dom::DomNodeBase* stored = config.child(k_storages);
        if (!stored)
        {
            return;
        }
        const DocumentsFolder& primary = *m_storages.front().folder;
        (config / k_primaryRemoved).set(!primary.available());
        (config / k_primaryReadOnly).set(primary.readOnly());
        (config / k_primaryName).set(primary.customName());
        (config / k_nextStorage).set(m_nextStorageId);
        StorageSettings& entries = stored->as<StorageSettings>();
        entries.clear();
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.id == k_primaryStorageId || !entry.folder->available())
            {
                continue;
            }
            Dom::Section& storedEntry = entries.add();
            (storedEntry / ConfigNames::Id).set(entry.id);
            (storedEntry / k_name).set(entry.folder->customName());
            (storedEntry / k_location).set(entry.folder->directory().wstring());
            (storedEntry / k_readOnly).set(entry.folder->readOnly());
        }
    }

    void DocumentsBrowserBase::collectStorageTiles()
    {
        m_storageTiles.clear();
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder->available() && !entry.folder->directory().empty())
            {
                m_storageTiles.push_back({ entry.folder.get(), entry.id });
            }
        }
    }

    void DocumentsBrowserBase::refreshStoragePages()
    {
        for (BrowserTab& tab : tabs())
        {
            if (tab.page() && tab.page()->tag().value == k_homePageTag)
            {
                auto& page = static_cast<DocumentsHomePageBase&>(*tab.page());
                page.rebuildStorageTiles(&page == currentPage() ? m_storageToSelect : L"");
            }
        }
        m_storageToSelect.clear();
        StdActions::save.invalidateState();
        StdActions::saveAs.invalidateState();
        StdActions::rename.invalidateState();
        StdActions::del.invalidateState();
    }

    void DocumentsBrowserBase::storageChanged(const std::wstring_view selection)
    {
        collectStorageTiles();
        storeStorages();
        m_storageToSelect = selection;
        m_storageRefresh.start(MilliSeconds{ 0u });
    }

    std::wstring DocumentsBrowserBase::storagePathError(std::filesystem::path& path,
        const DocumentsFolder* editedStorage) const
    {
        if (!path.is_absolute())
        {
            return L"Enter an absolute folder path.";
        }
        if (editedStorage && sameDirectory(editedStorage->directory(), path))
        {
            path = editedStorage->directory();
            return {};
        }
        std::error_code error;
        const std::filesystem::path selected = std::filesystem::canonical(path, error);
        if (error || !std::filesystem::is_directory(selected, error))
        {
            return L"Choose an existing folder.";
        }
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder->available() && sameDirectory(entry.folder->directory(), selected))
            {
                return L"That folder is already a storage.";
            }
        }
        path = selected;
        return {};
    }

    std::filesystem::path DocumentsBrowserBase::storageToDelete(const DocumentsFolder& storage) const
    {
        const std::filesystem::path target = storage.directoryForDeletion();
        if (target.empty())
        {
            return {};
        }
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder.get() == &storage || !entry.folder->available()
                || entry.folder->directory().empty())
            {
                continue;
            }
            std::error_code error;
            const std::filesystem::path other =
                std::filesystem::weakly_canonical(entry.folder->directory(), error);
            if (error || std::mismatch(target.begin(), target.end(), other.begin(), other.end()).first
                == target.end())
            {
                return {};
            }
        }
        return target;
    }
}
