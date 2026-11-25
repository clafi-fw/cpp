module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Documents.Browser;

import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.List;
import ClaFi.Documents.Page;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;
    using namespace Browser;

    // The tags a browser tells its two kinds of page apart by.
    export constexpr TagValue k_homePageTag = 1ull;
    export constexpr TagValue k_documentPageTag = 2ull;

    export template<typename T>
        concept IsDocumentsHomePage = std::derived_from<T, DocumentsHomePageBase>;
    export template<typename T>
        concept IsDocumentPage = std::derived_from<T, DocumentPage>;

    // A browser over a folder of documents; DocumentsBrowser names its pages. See Documents
    export class DocumentsBrowserBase : public BrowserControl
    {
    public:
        using WritableStorages = std::vector<DocumentsFolder*>;
    public:
        template<typename... Args>
        explicit DocumentsBrowserBase(const CreateParams&, Args&&...);
        ~DocumentsBrowserBase() override;
    public:
        void initialize(BrowserSettings&);
        [[nodiscard]] DocumentsFolder& folder() const { return m_folder; }
        [[nodiscard]] static Dom::Dt::Section storageSettingsSchema();
        [[nodiscard]] DocumentsFolder& storageFolder(const PageData&) const;
        [[nodiscard]] WritableStorages writableStorages() const;
        [[nodiscard]] std::wstring documentPath(const DocumentsFolder&, std::wstring_view fileName) const;
        void editStorage(Control& owner, DocumentsFolder* storage);
        void removeStorage(Control& owner, DocumentsFolder& storage);
    protected:
        [[nodiscard]] std::optional<std::wstring> canonicalPagePath(std::wstring_view) const override;
        void initPageData(PageData&) override;
        void fetchSubItems(PageData&) override;
        IconSize pageIconSize(PageData&) override;
        void drawPageIcon(PageData&, DrawIconEvent&) override;
        bool canLeavePage(BrowserTab&, Control& initiator) override;
        bool canRenamePage(PageData&) override;
        std::wstring renamePage(PageData&, AcceptEditEvent&) override;
        void showPage(BrowserTab&) override;
        IconSize tabIconSize(BrowserTab&) override;
        void drawTabIcon(BrowserTab&, DrawIconEvent&) override;
        // The pages, built on the tab under the tag that tells them apart.
        [[nodiscard]] virtual DocumentsHomePageBase& createHomePage(BrowserTab&) = 0;
        [[nodiscard]] virtual DocumentPage& createDocumentPage(BrowserTab&) = 0;
        [[nodiscard]] static bool isHomePage(const PageData&);
        [[nodiscard]] DocumentStorageView listingView(const PageData&) const;
        [[nodiscard]] const StorageTiles* storageTilesFor(const PageData&) const;
        // Whether a page is a document: one named with the kind's extension, unless a derived
        // browser knows others.
        [[nodiscard]] virtual bool isDocumentPage(const PageData&) const;
        // The document file a page stands for, or an empty path where there is none to rename.
        [[nodiscard]] virtual std::filesystem::path documentFileOf(const PageData&) const;
    private:
        struct StorageEntry
        {
            std::wstring id;
            std::unique_ptr<DocumentsFolder> folder;
            bool userReadOnly{};
            bool fromDefaults{};
            bool inactiveDefault{};
        };
        using StorageEntries = std::vector<StorageEntry>;
    private:
        [[nodiscard]] static bool isStoragePage(const PageData&);
        [[nodiscard]] const StorageEntry* storageEntry(std::wstring_view id) const;
        [[nodiscard]] const StorageDefault* storageDefault(const std::filesystem::path&) const;
        [[nodiscard]] bool requiresReadOnly(const std::filesystem::path&) const;
        [[nodiscard]] std::unique_ptr<DocumentsFolder> makeStorage(
            const std::filesystem::path& path, bool readOnly, std::wstring_view name = {}) const;
        StorageEntry& addStorage(const std::filesystem::path& path, bool readOnly,
            std::wstring_view name);
        void storeStorages(Dom::Section& config) const;
        void collectStorageTiles();
        void refreshStoragePages();
        void storageChanged(std::wstring_view selection);
        [[nodiscard]] std::wstring storagePathError(std::filesystem::path& path,
            const DocumentsFolder* editedStorage) const;
        [[nodiscard]] std::filesystem::path storageToDelete(const DocumentsFolder&) const;
    private:
        DocumentsFolder& m_folder;
        StorageEntries m_storages{};
        std::unique_ptr<DocumentsFolder> m_missingStorage{};
        StorageTiles m_storageTiles{};
        std::wstring m_storageToSelect{};
        UiTimer m_storageRefresh{};
        std::size_t m_nextStorageId{ 1 };
    };

    export template<IsDocumentsHomePage HomePageType, IsDocumentPage DocumentPageType>
        // The browser with its two page types named. See Documents
        class DocumentsBrowser : public DocumentsBrowserBase
    {
    public:
        using DocumentsBrowserBase::DocumentsBrowserBase;
    protected:
        DocumentsHomePageBase& createHomePage(BrowserTab&) override;
        DocumentPage& createDocumentPage(BrowserTab&) override;
    };


    //-------------------------------------------------------------------------


    // DocumentsBrowserBase

    template<typename... Args>
    DocumentsBrowserBase::DocumentsBrowserBase(const CreateParams& params, Args&&... args)
        :
        BrowserControl{ params, std::forward<Args>(args)... },
        // The folder the browser shows, which outlives it.
        m_folder{ *REQUIRE_PROPERTY(DocumentsFolder*) }
    {
    }

    // DocumentsBrowser

    template<IsDocumentsHomePage HomePageType, IsDocumentPage DocumentPageType>
    DocumentsHomePageBase& DocumentsBrowser<HomePageType, DocumentPageType>::createHomePage(
        BrowserTab& tab)
    {
        return tab.createPage<HomePageType>(
            Tag{ k_homePageTag },
            &folder(),
            listingView(*tab.pageData()),
            StorageTilesView{ storageTilesFor(*tab.pageData()) }
        );
    }

    template<IsDocumentsHomePage HomePageType, IsDocumentPage DocumentPageType>
    DocumentPage& DocumentsBrowser<HomePageType, DocumentPageType>::createDocumentPage(
        BrowserTab& tab)
    {
        return tab.createPage<DocumentPageType>(
            Tag{ k_documentPageTag },
            &folder(),
            DocumentStorageView{ &storageFolder(*tab.pageData()), isHomePage(*tab.pageData()->parent) }
        );
    }
}
