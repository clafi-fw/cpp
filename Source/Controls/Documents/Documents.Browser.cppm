export module ClaFi.Documents.Browser;

import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.Page;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
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
        concept IsDocumentsHomePage = std::derived_from<T, DocumentsHomePage>;
    export template<typename T>
        concept IsDocumentPage = std::derived_from<T, DocumentPage>;

    // A browser over a folder of documents; DocumentsBrowser names its pages. See Documents#browser
    export class DocumentsBrowserBase : public BrowserControl
    {
    public:
        template<typename... Args>
        explicit DocumentsBrowserBase(const CreateParams&, Args&&...);
        ~DocumentsBrowserBase() override;
    public:
        [[nodiscard]] DocumentsFolder& folder() const { return m_folder; }
    protected:
        void initPageData(PageData&) override;
        void fetchSubItems(PageData&) override;
        IconSize pageIconSize(PageData&) override;
        void paintPageIcon(PageData&, PaintIconEvent&) override;
        bool canLeavePage(BrowserTab&, Control& initiator) override;
        bool canRenamePage(PageData&) override;
        std::wstring renamePage(PageData&, AcceptEditEvent&) override;
        void showPage(BrowserTab&) override;
        IconSize tabIconSize(BrowserTab&) override;
        void paintTabIcon(BrowserTab&, PaintIconEvent&) override;
        // The pages, built on the tab under the tag that tells them apart.
        [[nodiscard]] virtual DocumentsHomePage& createHomePage(BrowserTab&) = 0;
        [[nodiscard]] virtual DocumentPage& createDocumentPage(BrowserTab&) = 0;
        [[nodiscard]] static bool isHomePage(const PageData&);
        [[nodiscard]] bool isDocumentPage(const PageData&) const;
        // The document file a page stands for, and an empty path where there is none the user
        // may rename: the home page is not a file, and neither is a document whose file has gone.
        // Asked of the disk, not of the folder's list. See Documents#browser
        [[nodiscard]] std::filesystem::path documentFileOf(const PageData&) const;
    private:
        DocumentsFolder& m_folder;
    };

    export template<IsDocumentsHomePage HomePageType, IsDocumentPage DocumentPageType>
        // The browser with its two page types named. See Documents#browser
        class DocumentsBrowser : public DocumentsBrowserBase
    {
    public:
        using DocumentsBrowserBase::DocumentsBrowserBase;
    protected:
        DocumentsHomePage& createHomePage(BrowserTab&) override;
        DocumentPage& createDocumentPage(BrowserTab&) override;
    };


    //-------------------------------------------------------------------------


    // DocumentsBrowserBase

    template<typename... Args>
    DocumentsBrowserBase::DocumentsBrowserBase(const CreateParams& params, Args&&... args)
        :
        BrowserControl{ params, std::forward<Args>(args)... },
        m_folder{ *Props::get<DocumentsFolder*>(nullptr, args...) }
    {
    }

    // DocumentsBrowser

    template<IsDocumentsHomePage HomePageType, IsDocumentPage DocumentPageType>
    DocumentsHomePage& DocumentsBrowser<HomePageType, DocumentPageType>::createHomePage(
        BrowserTab& tab)
    {
        return tab.createPage<HomePageType>(Tag{ k_homePageTag }, &folder());
    }

    template<IsDocumentsHomePage HomePageType, IsDocumentPage DocumentPageType>
    DocumentPage& DocumentsBrowser<HomePageType, DocumentPageType>::createDocumentPage(
        BrowserTab& tab)
    {
        return tab.createPage<DocumentPageType>(Tag{ k_documentPageTag }, &folder());
    }
}
