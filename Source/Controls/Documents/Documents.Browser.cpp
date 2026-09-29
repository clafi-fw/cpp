module ClaFi.Documents.Browser;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.Page;
import ClaFi.Documents.Utils;

import ClaFi.Browser.Consts;
import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.MessageDialog;

import ClaFi.Icons.HomeIcon;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;
    using namespace Browser;

    // Small enough to sit inside a tab's caption line or a crumb's.
    constexpr IconSize k_pageIconSize{ 16.0f };

    DocumentsBrowserBase::~DocumentsBrowserBase()
    {
        // The pages go while the browser still stands: a page reaches the tab it was built on
        // from its own destructor.
        clearControls();
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
        else if (isDocumentPage(data))
        {
            // The stem, whatever the extension: a derived browser's documents may carry another.
            data.title = std::filesystem::path{ data.name }.stem().wstring();
        }
    }

    void DocumentsBrowserBase::fetchSubItems(PageData& data)
    {
        // A document page is a leaf, so the home page is the only one with a list to give.
        if (!isHomePage(data))
            return;

        // The page is left unmarked, so this is asked again on every drop: the folder is written
        // to while the application is up, and a file added, renamed or deleted since the last
        // drop belongs in the answer. What is named here is the whole list.
        for (const std::filesystem::path& file : m_folder.files())
            addSubItem(data, file.filename().wstring());
    }

    IconSize DocumentsBrowserBase::pageIconSize(PageData& data)
    {
        return isHomePage(data) || isDocumentPage(data) ? k_pageIconSize : IconSize{ 0.0f };
    }

    void DocumentsBrowserBase::paintPageIcon(PageData& data, PaintIconEvent& event)
    {
        if (isHomePage(data))
            Icons::HomeIcon::paintBlock(event);
        else if (isDocumentPage(data))
            m_folder.paintIcon(data.name, event);
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
        dialog.add(DialogButton::Save);
        dialog.add(DialogButton::Discard);
        dialog.add(DialogButton::Cancel);
        // Answered while the dialog is still standing: a save that did not reach the disk leaves
        // the question up, with what is being left still on screen behind it.
        dialog.onAnswer([page](DialogAnswerEvent& event) {
            if (event.answer == DialogButton::Save && !page->saveEdits(event.button))
                event.keepOpen();
        });
        const DialogAnswer answer = dialog.execute();

        // Save got here only by having saved. Discard is the only other way out - Cancel and a
        // dialog dismissed without an answer at all both mean stay.
        return answer == DialogButton::Save || answer == DialogButton::Discard;
    }

    bool DocumentsBrowserBase::canRenamePage(PageData& data)
    {
        return !documentFileOf(data).empty();
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
        return m_folder.renameFile(file, event);
    }

    void DocumentsBrowserBase::showPage(BrowserTab& tab)
    {
        TagValue needTag = 0ull;
        if (isDocumentPage(*tab.pageData()))
            needTag = k_documentPageTag;
        else if (isHomePage(*tab.pageData()))
            needTag = k_homePageTag;

        DocumentsBasePage* tabPage{};
        std::wstring pathToSelect{};
        if (tab.page())
        {
            tabPage = static_cast<DocumentsBasePage*>(tab.page());
            const TagValue tabTag = tabPage->tag().value;
            // A PAGE IS BUILT FOR ONE PAGE DATA AND HOLDS IT BY REFERENCE, so a tab that has moved
            // to a different one needs a new page even where the kind of page is the same. Kept,
            // the editor would go on showing the document it was built for, and saving to that
            // document's file, while the caption, the path and the tab all named the one chosen.
            const bool pageDataChanged = &tabPage->pageData() != tab.pageData();
            if (tabTag != needTag || pageDataChanged)
            {
                // Only the home page lands on an item, and only it reads this.
                if (tabTag == k_documentPageTag && needTag == k_homePageTag)
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
        if (!data || !(isHomePage(*data) || isDocumentPage(*data)))
            return IconSize{ 0.0f };
        return k_pageIconSize;
    }

    void DocumentsBrowserBase::paintTabIcon(BrowserTab& tab, PaintIconEvent& event)
    {
        const PageData* data = tab.pageData();
        if (!data)
            return;
        if (isHomePage(*data))
            Icons::HomeIcon::paintBlock(event);
        else if (isDocumentPage(*data))
            m_folder.paintIcon(data->name, event);
    }

    bool DocumentsBrowserBase::isHomePage(const PageData& data)
    {
        return data.name == ConfigNames::homePage;
    }

    bool DocumentsBrowserBase::isDocumentPage(const PageData& data) const
    {
        return data.name.ends_with(m_folder.kind().extension);
    }

    std::filesystem::path DocumentsBrowserBase::documentFileOf(const PageData& data) const
    {
        if (!isDocumentPage(data))
            return {};
        std::filesystem::path result = m_folder.fileOf(data.name);
        std::error_code errorCode;
        if (!std::filesystem::is_regular_file(result, errorCode))
            return {};
        return result;
    }
}
