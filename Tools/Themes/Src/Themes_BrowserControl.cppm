export module ThisApp.ThemesBrowser;

import ThisApp.AppIcon;
import ThisApp.BasePage;
import ThisApp.HomePage;
import ThisApp.ThemePage;
import ThisApp.Consts;
import ThisApp.Utils;

import ClaFi.Application.ThemesManager;
import ClaFi.App.Themes;
import ClaFi.App.ThemeIcon;

import ClaFi.Browser;
import ClaFi.Browser.PageData;

import ClaFi.Controls.Base.ButtonBase;

import ClaFi.Dom;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.MessageDialog;

import ClaFi.Icons.HomeIcon;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;


    // Small enough to sit inside a tab's caption line or a crumb's, and large enough that the
    // palette inset in the theme's surface still shows three parts.
    constexpr IconSize k_pageIconSize{ 16.0f };

    export class ThemesBrowser : public BrowserControl
    {
    public:
        template<typename... Args>
        ThemesBrowser(const CreateParams&, Args&&...);
        ~ThemesBrowser() override;
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
        void saveViewState(BrowserTab&) override;
    private:
        static bool isHomeTab(const BrowserTab&);
        static bool isThemeTab(const BrowserTab&);
        static bool isHomePage(const PageData&);
        // The theme of the page on this tab, and nothing while the tab has no page.
        const AppTheme* tabTheme(BrowserTab&);
        // What an unopened tab's icon is drawn from, out of the tab's entry.
        [[nodiscard]] ThemeIconColors tabIconColors(BrowserTab&);
        // The theme a page stands for, straight out of the manager. This is the answer for a page
        // no tab is open on - a line of a crumb's list - where there is no view state to read and
        // the file is the whole of what is known.
        const AppTheme* pageTheme(const PageData&);
        // The theme file a page stands for, and an empty path where the page has none the user may
        // rename: the home page is not a theme, and a built-in is answered from the compiled-in
        // colours whatever stands on the disk - so a file of that name is not what the page shows.
        //
        // ASKED OF THE DISK, not of the manager's list. That list is re-read when the directory
        // watch reports, and the watch waits out a quiet period first, so between a rename and
        // that report it still holds the name the file no longer has - and this is asked again the
        // moment after a rename, to say whether the page can be renamed once more.
        [[nodiscard]] std::filesystem::path themeFileOf(const PageData&);
    };


    template<typename ...Args>
    ThemesBrowser::ThemesBrowser(const CreateParams& params, Args&&... args)
        :
        BrowserControl{
            params,
            //params.themeColors().dialog,
            std::forward<Args>(args)...}
    {
        // The mark itself is in AppIcon, so the same drawing answers the button and writes
        // the .ico. The button states its own view mode and icon size; nothing here restates
        // them.
        appButton().onPaintIcon(AppIcon::paintIcon);
    }

    ThemesBrowser::~ThemesBrowser()
    {
        for (BrowserTab& tab : tabs())
            if (tab.page())
                ThemesBrowser::saveViewState(tab);

        // The pages go while the browser still stands: a page reaches the themes and the tab it
        // was built on from its own destructor.
        clearControls();
    }

    void ThemesBrowser::initPageData(PageData& data)
    {
        if (data.name == ConfigNames::homePage)
        {
            data.title = L"Themes";
            // Said here because this runs for every page as it is built, well before anything asks
            // for the list itself. It is what lets the home crumb carry a strip from the start,
            // while a theme page carries none.
            data.fetchState = FetchState::HasChildren;
        }
        else if (data.name == k_defaultPageName)
            data.title = k_defaultPageTitle;
        else if (data.name == k_dark2PageName)
            data.title = k_dark2PageTitle;
        else if (data.name.ends_with(k_themeFileExtension))
            data.title = data.name.substr(0, data.name.size() - k_themeFileExtension.size());
    }

    void ThemesBrowser::fetchSubItems(PageData& data)
    {
        // A theme page is a leaf, so the home page is the only one with a list to give.
        if (!isHomePage(data))
            return;

        // The same themes the home page shows, in the order it shows them: the order they are
        // named here is the order they are listed in, and userThemes() is sorted - so a crumb and
        // the page it drops from read the same either way.
        //
        // The page is left unmarked, so this is asked again on every drop: the directory is
        // written to while the application is up, and a theme added, renamed or deleted since the
        // last drop belongs in the answer. What is named here is the whole list - a theme whose
        // file has gone is not named, and the browser drops it.
        addSubItem(data, k_defaultPageName);
        for (const UserThemePtr& userTheme : appThemes().userThemes())
        {
            const std::wstring fileName = userTheme->path().filename().wstring();
            addSubItem(data, fileName);
        }
    }

    IconSize ThemesBrowser::pageIconSize(PageData& data)
    {
        // A page with nothing to show keeps its title against the crumb's left edge.
        return isHomePage(data) || pageTheme(data) ? k_pageIconSize : IconSize{ 0.0f };
    }

    void ThemesBrowser::paintPageIcon(PageData& data, PaintIconEvent& event)
    {
        if (isHomePage(data))
        {
            Icons::HomeIcon::paintBlock(event);
            return;
        }
        if (const AppTheme* theme = pageTheme(data))
            paintThemeIcon(event, theme->colors);
    }

    bool ThemesBrowser::canLeavePage(BrowserTab& tab, Control& initiator)
    {
        const BasePage* page = static_cast<const BasePage*>(tab.page());
        // A tab restored from settings and never opened has no page, and a page with nothing the
        // file is missing has nothing to ask about.
        if (!page || !page->hasUnsavedEdits())
            return true;

        Text message{};
        message << themeInQuestionText(tab.pageData()->displayTitle())
            << L" has changes that are not saved.";
        // Under whatever leaving was asked from - the crumb, the Up button, the tab being closed -
        // so the question stands where the user is looking.
        MessageDialog dialog{ initiator, L"Unsaved changes", message, MessageIcon::Question };
        dialog.add(DialogButton::Save);
        dialog.add(DialogButton::Discard);
        dialog.add(DialogButton::Cancel);
        // Answered while the dialog is still standing. A page with no file of its own asks WHERE
        // the work should go, and that question is owned by the Save button just pressed - so it
        // stands on top of this one, with what is being left still on screen behind both. Told no
        // there, nothing was written, so this question was never settled either.
        dialog.onAnswer([page](DialogAnswerEvent& event) {
            if (event.answer == DialogButton::Save && !page->saveEdits(event.button))
                event.keepOpen();
            });
        const DialogAnswer answer = dialog.execute();

        // Save got here only by having saved. Discard is the only other way out - Cancel and a
        // dialog dismissed without an answer at all both mean stay, which is the safe reading of
        // a question left unanswered.
        return answer == DialogButton::Save || answer == DialogButton::Discard;
    }

    bool ThemesBrowser::canRenamePage(PageData& data)
    {
        return !themeFileOf(data).empty();
    }

    std::wstring ThemesBrowser::renamePage(PageData& data, AcceptEditEvent& event)
    {
        // Asked again as the name is taken, rather than trusted from the crumb that offered the
        // editor: the directory is written to while the application is up, and the seconds an
        // editor stands open are seconds for the file to have gone.
        const std::filesystem::path themeFile = themeFileOf(data);
        if (themeFile.empty())
        {
            event.refuse(L"That theme is no longer there.");
            return {};
        }

        // A theme page is named by its file, so the rename settles the page's new name as well -
        // the extension is the file's to give, and it is not what the user typed.
        return renameThemeFile(themeFile, event);
    }

    void ThemesBrowser::showPage(BrowserTab& tab)
    {
        TagValue needTag = 0ull;
        if (tab.pageData()->name.ends_with(k_themePathSuffix))
            needTag = k_themePageTag;
        else if (tab.pageData()->name == ConfigNames::homePage)
            needTag = k_homePageTag;

        BasePage* tabPage{};

        std::wstring pathToSelect{};
        if (tab.page())
        {
            tabPage = static_cast<BasePage*>(tab.page());
            TagValue tabTag = tabPage->tag().value;
            // A PAGE IS BUILT FOR ONE PAGE DATA AND HOLDS IT BY REFERENCE, so a tab that has moved
            // to a different one needs a new page even where the kind of page is the same. Theme to
            // theme is that case, and reaching it needs a list of themes to pick from - a crumb's
            // dropdown is the first thing to offer one. Kept, the editor would have gone on showing
            // the theme it was built for, and saved to that theme's file, while the caption, the
            // path and the tab icon all named the one that was chosen.
            const bool pageDataChanged = &tabPage->pageData() != tab.pageData();
            if (tabTag != needTag || pageDataChanged)
            {
                // Only the home page lands on an item, and only it reads this.
                if (tabTag == k_themePageTag && needTag == k_homePageTag)
                    pathToSelect = tabPage->pageData().path();
                // The edits that were in the page have already been dealt with: canLeavePage put
                // them to the user before anything moved, and the page below overwrites the view
                // state they lived in with the theme it is opening.
                saveViewState(tab);
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
                HomePage& homePage = tab.createPage<HomePage>(Tag{ needTag });
                // The page a tab is coming up from is the item to land on. The scroll that
                // brings it into view is answered by the alignment pass, which runs once
                // restoreViewState below has settled the preview panel - the list's width
                // is what decides the row the item is on.
                if (!pathToSelect.empty())
                    homePage.selectItemByPagePath(pathToSelect);
                tabPage = &homePage;
                break;
            }
            case k_themePageTag:
                tabPage = &tab.createPage<ThemePage>(Tag{ needTag });
                // The page reads its theme out of the tab's view state, so the file is copied there
                // first. THE ONE TAB THAT MUST NOT HAVE IT is one standing on a page it was
                // restored onto: its view state already holds this theme, edits and all, and those
                // are exactly what has not reached the file yet. Every other tab wants the file -
                // one arriving from another page carries the last page's theme there, and one the
                // user has just opened carries nothing at all.
                //
                // The tab answers that itself. Asking whether it was showing a page cannot: a tab
                // opened straight onto a theme is showing none either, and it would come up on the
                // defaults an empty view state reads as, under the right name, reading as edited
                // against a file it had never been handed.
                // TODO: ThemesManager::saveTheme answers nothing and leaves the node alone when it
                // does not know the name, which a crumb's list can still offer for a theme whose
                // file has gone. The page then comes up editing the last theme's data under the
                // chosen theme's name, and saving it recreates the deleted file with that data.
                // Should saveTheme report whether it found the theme, and what should a page do
                // with a theme that is no longer there - open empty, or refuse to open?
                if (!tab.isOnRestoredPage())
                    appThemes().saveTheme(
                        tab.pageData()->name,
                        (tabPage->tabConfig() / k_themeDataAttrName).as<Dom::Value<AppTheme>>()
                    );
                break;
            default: ;
            }
            if (tabPage)
                tabPage->restoreViewState();
        }

        tab.setPage(tabPage);

        // The page is what the icon comes from once there is one, and the tab last asked before it
        // existed.
        tab.updateIconMode();

        if (tabPage)
            tabPage->show();
    }

    IconSize ThemesBrowser::tabIconSize(BrowserTab& tab)
    {
        // A tab with nothing to show keeps its caption against the tab's left edge.
        return isHomeTab(tab) || isThemeTab(tab) ? k_pageIconSize : IconSize{ 0.0f };
    }

    void ThemesBrowser::paintTabIcon(BrowserTab& tab, PaintIconEvent& event)
    {
        if (isHomeTab(tab))
        {
            Icons::HomeIcon::paintBlock(event);
            return;
        }
        if (!isThemeTab(tab))
            return;
        // The page's theme, edits and all, while there is a page. A tab restored from settings has
        // none until it is first opened, and its entry carries what the icon is drawn from, written
        // when the page was last left - so the tab's own file is not read for the icon.
        if (const AppTheme* theme = tabTheme(tab))
            paintThemeIcon(event, theme->colors);
        else
            paintThemeIcon(event, tabIconColors(tab));
    }

    // Writes into the entry what the icon is drawn from, for the runs in which this tab is
    // restored and never opened.
    void ThemesBrowser::saveViewState(BrowserTab& tab)
    {
        if (const AppTheme* theme = tabTheme(tab))
        {
            const ThemeIconColors iconColors{ theme->colors };
            (settings().tabEntry(tab.id()) / k_tabIconAttrName).set(iconColors);
        }
    }

    bool ThemesBrowser::isHomeTab(const BrowserTab& tab)
    {
        return tab.pageData() && tab.pageData()->name == ConfigNames::homePage;
    }

    bool ThemesBrowser::isThemeTab(const BrowserTab& tab)
    {
        return tab.pageData() && tab.pageData()->name.ends_with(k_themePathSuffix);
    }

    bool ThemesBrowser::isHomePage(const PageData& data)
    {
        return data.name == ConfigNames::homePage;
    }

    const AppTheme* ThemesBrowser::tabTheme(BrowserTab& tab)
    {
        BasePage* page = static_cast<BasePage*>(tab.page());
        return page ? page->tabTheme() : nullptr;
    }

    ThemeIconColors ThemesBrowser::tabIconColors(BrowserTab& tab)
    {
        return (settings().tabEntry(tab.id()) / k_tabIconAttrName).get<ThemeIconColors>();
    }

    const AppTheme* ThemesBrowser::pageTheme(const PageData& data)
    {
        // The list is asked for first, so that a theme file reaches themeByName even when nothing
        // has read the directory yet. Settled it is a bool test - but the directory watcher clears
        // that flag, so the first icon painted after the directory changes re-reads it and parses
        // every theme file, here, inside a paint.
        // TODO: should the browser hold a listener of its own and reload off the watcher, the way
        // HomePage does, so a paint never waits on the disk?
        appThemes().userThemes();
        return appThemes().themeByName(data.name);
    }

    std::filesystem::path ThemesBrowser::themeFileOf(const PageData& data)
    {
        if (isHomePage(data))
            return {};
        // The same test that keeps a user theme from taking a built-in's name. The extension
        // cannot answer it: a file the user drops in the directory called Foo.theme carries the
        // built-ins' extension and is not one of them.
        if (isReservedThemeName(std::filesystem::path{ data.name }.stem().wstring()))
            return {};

        std::filesystem::path result = appThemes().directory() / data.name;
        std::error_code errorCode;
        if (!std::filesystem::exists(result, errorCode))
            return {};

        return result;
    }
}
