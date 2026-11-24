module ClaFi.Documents.Browser;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.Page;
import ClaFi.Documents.Utils;

import ClaFi.Browser.Consts;
import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;
import ClaFi.Controls.MessageDialog;

import ClaFi.Icons.HomeIcon;

import ClaFi.StdActions;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
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
        constexpr std::wstring_view k_storages = L"DocumentStorages";
        constexpr std::wstring_view k_primaryRemoved = L"PrimaryStorageRemoved";
        constexpr std::wstring_view k_primaryReadOnly = L"PrimaryStorageReadOnly";
        constexpr std::wstring_view k_nextStorage = L"NextDocumentStorageId";
        constexpr std::wstring_view k_readOnly = L"ReadOnly";
        using StorageSettings = Dom::Sequence<Dom::Section>;

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
        m_storages.push_back({
            .folder = makeStorage(m_folder.directory(), primaryReadOnly || m_folder.readOnly())
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
                const std::filesystem::path path{ (entry / ConfigNames::Path).get<std::wstring>() };
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
                    .folder = makeStorage(path, (entry / k_readOnly).get<bool>())
                });
            }
        }
        BrowserControl::initialize(settings);
    }

    Dom::Dt::Section DocumentsBrowserBase::storageSettingsSchema()
    {
        using namespace Dom::Dt;
        return {
            Value{ k_primaryRemoved, false },
            Value{ k_primaryReadOnly, false },
            Value{ k_nextStorage, std::size_t{ 1 } },
            Sequence{
                k_storages,
                Section{
                    Value{ ConfigNames::Id, L"" },
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
        const StorageEntry* entry = storageEntry(isStoragePage(*root) ? root->name : L"");
        return entry ? *entry->folder : *m_missingStorage;
    }

    DocumentsFolder* DocumentsBrowserBase::saveAsStorage(const PageData& data) const
    {
        DocumentsFolder& current = storageFolder(data);
        if (current.canWrite())
        {
            return &current;
        }
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder->canWrite())
            {
                return entry.folder.get();
            }
        }
        return nullptr;
    }

    std::wstring DocumentsBrowserBase::documentPath(const DocumentsFolder& storage,
        const std::wstring_view fileName) const
    {
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder.get() == &storage && storage.available() && !fileName.empty())
            {
                if (entry.id.empty())
                {
                    return pagePathOfDocument(fileName);
                }
                return pagePathOfDocument(entry.id).append(L"/").append(fileName);
            }
        }
        return {};
    }

    void DocumentsBrowserBase::showStorages(const PageData& data, Control& owner)
    {
        DocumentsFolder& current = storageFolder(data);
        enum class Command
        {
            Open,
            Add,
            Edit,
            ToggleReadOnly,
            Remove
        };
        std::optional<Command> command{};
        std::wstring selectedPath{};
        Menu menu{ owner };
        for (const StorageEntry& entry : m_storages)
        {
            if (!entry.folder->available() || entry.folder->directory().empty())
            {
                continue;
            }
            std::wstring caption = entry.folder->directory().wstring();
            if (entry.folder->readOnly())
            {
                caption.append(L" (read-only)");
            }
            const std::wstring path = pagePathOfDocument(entry.id);
            menu.add(Text{ caption }, OnClick{ [&command, &selectedPath, path](ClickEvent&) {
                command = Command::Open;
                selectedPath = path;
            } });
        }

        menu.addSeparator();
        menu.add(L"Add storage...", OnClick{ [&command](ClickEvent&) {
            command = Command::Add;
        } });
        if (current.available() && !current.directory().empty())
        {
            menu.add(L"Change storage folder...", OnClick{ [&command](ClickEvent&) {
                command = Command::Edit;
            } });
            menu.add(current.readOnly() ? L"Make storage writable" : L"Make storage read-only",
                OnClick{ [&command](ClickEvent&) {
                    command = Command::ToggleReadOnly;
                } });
            menu.add(L"Remove storage", OnClick{ [&command](ClickEvent&) {
                command = Command::Remove;
            } });
        }
        menu.executeUnder(owner);
        if (!command)
        {
            return;
        }

        switch (*command)
        {
            case Command::Open:
                goToLater(Url{ selectedPath });
                break;
            case Command::Add:
                editStorage(owner, nullptr);
                break;
            case Command::Edit:
                editStorage(owner, &current);
                break;
            case Command::ToggleReadOnly:
                current.setReadOnly(!current.readOnly());
                storeStorages();
                break;
            case Command::Remove:
                current.detach();
                storeStorages();
                goToLater(Url{ ConfigNames::HomePage });
                break;
        }
        StdActions::del.invalidateState();
        StdActions::save.invalidateState();
        StdActions::saveAs.invalidateState();
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
                data.title = storage.directory().filename().wstring();
                if (data.title.empty())
                {
                    data.title = storage.directory().wstring();
                }
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
            for (const StorageEntry& entry : m_storages)
            {
                if (!entry.id.empty() && entry.folder->available())
                {
                    addSubItem(data, entry.id);
                }
            }
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
        if (isHomePage(data) || isStoragePage(data))
            Icons::HomeIcon::drawBlock(event);
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
                if (tabTag == k_documentPageTag && needTag == k_homePageTag
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
        if (isHomePage(*data) || isStoragePage(*data))
            Icons::HomeIcon::drawBlock(event);
        else if (isDocumentPage(*data))
            storageFolder(*data).drawIcon(data->name, event);
    }

    bool DocumentsBrowserBase::isHomePage(const PageData& data)
    {
        return data.name == ConfigNames::HomePage;
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
        const std::filesystem::path& path, const bool readOnly) const
    {
        return std::make_unique<BrowserStorage>(m_folder, path, readOnly);
    }

    void DocumentsBrowserBase::addStorage(const std::filesystem::path& path, const bool readOnly)
    {
        std::wstring id{};
        do
        {
            id.assign(k_storagePrefix).append(std::to_wstring(m_nextStorageId++));
        }
        while (storageEntry(id));
        m_storages.push_back({ .id = id, .folder = makeStorage(path, readOnly) });
        goToLater(Url{ pagePathOfDocument(id) });
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
        (config / k_nextStorage).set(m_nextStorageId);
        StorageSettings& entries = stored->as<StorageSettings>();
        entries.clear();
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.id.empty() || !entry.folder->available())
            {
                continue;
            }
            Dom::Section& storedEntry = entries.add();
            (storedEntry / ConfigNames::Id).set(entry.id);
            (storedEntry / ConfigNames::Path).set(entry.folder->directory().wstring());
            (storedEntry / k_readOnly).set(entry.folder->readOnly());
        }
    }

    void DocumentsBrowserBase::editStorage(Control& owner, DocumentsFolder* current)
    {
        std::filesystem::path start = current ? current->directory() : m_folder.directory();
        start.make_preferred();
        const std::optional<std::filesystem::path> chosen = Platform::pickFolder(&owner.form(),
            current ? L"Change storage folder" : L"Add document storage", start);
        if (!chosen)
        {
            return;
        }
        std::error_code error;
        std::filesystem::path path = std::filesystem::canonical(*chosen, error);
        if (error || !std::filesystem::is_directory(path, error))
        {
            ContextMessage::show(owner, Text{ L"The folder could not be opened." });
            return;
        }
        for (const StorageEntry& entry : m_storages)
        {
            if (entry.folder->available() && sameDirectory(entry.folder->directory(), path))
            {
                if (entry.folder.get() != current)
                {
                    ContextMessage::show(owner, Text{ L"That folder is already a storage." });
                }
                return;
            }
        }

        addStorage(path, current && current->readOnly());
        if (current)
        {
            current->detach();
        }
        storeStorages();
    }
}
