export module ClaFi.Browser.Control;

import ClaFi.Browser.Actions;
import ClaFi.Browser.Consts;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;
import ClaFi.Browser.BreadCrumbBar;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.FormTitle;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Base.Container;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Divider;
import ClaFi.Controls.TabStrip;

import ClaFi.Icons.PlusMark;
import ClaFi.Icons.XMark;
import ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;

import ClaFi.StdLib;
import ClaFi.Core.Context.PaintIconEvent;


namespace ClaFi::Browser
{
    using namespace Controls;

    export class BrowserControl;
    export class BrowserPage;
    export template<typename T>
        concept IsBrowserPage = std::derived_from<T, BrowserPage>;

    // One tab of the browser, standing for a page it can return to.
    export class BrowserTab : public Tab
    {
        // The browser is what makes a tab and what shows a page on one, so it is what sets and
        // spends m_onRestoredPage.
        friend BrowserControl;
    public:
        explicit BrowserTab(const CreateParams& params)
            :
            Tab{ params, WordWrap::No, Spacing{ 4.0f }, Padding{ 8.0f, 4.0f }, MinSize{ 0.0f, 28.0f } }
        {
        }
    public:
        template<IsBrowserPage PageClass, typename... Args>
        PageClass& createPage(Args&&... args);
        BrowserControl& browserControl() const { return *m_browserControl; }
        Control& closeButton() const { return m_closeButton; }
        PageData* pageData() const { return m_pageData; }
        void setPageData(PageData*);
        // Asks the browser what this tab's icon is now and opens or closes the icon slot to match.
        // setPageData does this on its own; a browser calls it again when the answer changes
        // without the page data changing - a tab whose page has just been built, say.
        void updateIconMode();
        void setId(BrowserControl&, const std::wstring_view id);
        std::wstring_view id() const { return m_id; }
        // Whether this tab is standing on a page it was RESTORED onto and has not opened yet.
        // Only then is the view state under it the page's own - written by a previous run, edits
        // and all - and a browser that keeps a page's state there must leave it alone. A tab that
        // has navigated carries the state of where it came from, and one the user has just opened
        // carries none; both want whatever the page's own source holds.
        //
        // Spent by the first page shown on it - see BrowserControl::selectPage - so a restored
        // tab is an ordinary one from its second page onwards.
        [[nodiscard]] bool isOnRestoredPage() const { return m_onRestoredPage; }
    protected:
        virtual RichControl* visualPage(RichControl*) override;
        void getTooltip(GetTooltipEvent&) override;
        void getText(GetTextEvent&) const override;
        void paintIcon(PaintIconEvent&) override;
        void secondaryClicked(ClickEvent& event) override { closeThisTab(event); }
    private:
        void closeThisTab(ClickEvent& params);
    private:
        BrowserControl* m_browserControl{};
        std::wstring m_id{};
        PageData* m_pageData{};
        // False for a tab the user opened, which is every tab but the ones a run starts with.
        bool m_onRestoredPage{ false };
        // The close button is the tab's secondary part, so the base keeps a press on it from
        // reading as a press on the tab and routes the click to secondaryClicked().
        //
        // It takes Button's Focusable, and that is what keeps pressing it off the tab's
        // selection. The focus walk in Input::setMouseDown stops at the first control that can
        // take focus, so the focus lands here, and TabStripBase::defaultCanFocusItem then declines
        // it as a current item because it is not a tab. Made MouseOnly, the button would be
        // invisible to that walk, which would climb to the tab and select it on the way to
        // closing it. Focus is also how the keyboard reaches the button at all.
        Button& m_closeButton = createSecondaryPart<ToolButton>(
            FixedSize{ 20.0f },
            Radius{ 10.0f },
            ButtonViewMode::IconOnly,
            IconSize{ 8.0f },
            OnEvent{ Icons::XMark::paint },
            OnEvent{ [this](GetTooltipEvent& event) {
                event.text << L"Close " << InkWell::textInk(InkGrade::Muted);
                m_pageData->paintText(event.text);
                event.placement = FormPlacement::Mouse;
            } }
        );
    };

    // One page of the browser: what it shows, and the crumbs that lead to it.
    export class BrowserPage : public Panel
    {
    public:
        template<typename... Args>
        explicit BrowserPage(const CreateParams&, Args&&...);
    public:
        BrowserSettings& settings() const;
        Dom::Section& tabConfig() const;
        const BrowserControl& browserControl() const { return m_browserControl; }
        const BrowserTab& tab() const { return m_tab; }
        std::wstring_view tabId() const { return m_tab.id(); }
        const PageData& pageData() const { return m_pageData; }
        void setPath(std::wstring_view) const;
        // What the user has picked inside this page, as the path a browser would open. Empty
        // where nothing is picked, which is what leaves Open and Open in new tab disabled.
        //
        // ASKED AS EACH COMMAND'S STATE IS, never kept: what is picked moves under a page that
        // is standing still, and nothing tells the commands so. A page that shows a list of
        // pages answers with the one the user is on; a page that is not a list of anything
        // answers with nothing and says so by leaving this alone.
        [[nodiscard]] virtual std::wstring pathToOpen() { return {}; }
    private:
        BrowserTab& m_tab;
        BrowserControl& m_browserControl;
        const PageData& m_pageData;
    };

    // A crumb bar over a page, with tabs across the top.
    export class BrowserControl : public Panel
    {
        friend BrowserPage;
        friend BrowserTab;
        friend BrowserTab;
    public:
        template<typename... Args>
        explicit BrowserControl(const CreateParams&, Args&&...);
    public:
        void initialize(BrowserSettings&);
        FormTitle& title() const { return m_title; }
        AppButton& appButton() const { return m_appButton; }
    public:
        // Opens a tab on the home page, which is where a browser with nothing said to it starts.
        void addTab();
        // Opens one on a page named outright. The new tab is selected, the way a tab the user
        // asked for is.
        void addTab(PageData&);
        void addTab(std::wstring_view path);
        BrowserTab* restoreTab(const std::wstring_view id, const std::wstring_view path);
        void goUp();
        void goTo(std::wstring_view path, std::wstring_view subPath);
        void goTo(std::wstring_view);
        void goTo(PageData&);
        void goTo(PageData&, Control& initiator);
        // Goes to a page once the press that asked for it has unwound. A COMMAND THAT NAVIGATES
        // FROM INSIDE A CLICK DESTROYS THE PAGE IT WAS PRESSED ON, and the frames above that click
        // go on reading controls that went down with it. The wait is held here rather than on the
        // page, because a page holding it would be destroying the very dispatcher delivering the
        // tick.
        void goToLater(std::wstring_view path);
        PageData& homePageData() { return m_homePageData; }
        const PageData& homePageData() const { return m_homePageData; }
        PageData* findPageData(std::wstring_view);
        BrowserSettings& settings() const { return *m_settings; }
    protected:
        ControlsAs<BrowserTab> tabs() const { return m_tabs.controlsAs<BrowserTab>(); }
        PageControl& pageControl() const { return m_pageControl; }
        // The page on screen, and nothing when no tab is selected.
        [[nodiscard]] BrowserPage* currentPage();
        virtual void initPageData(PageData&) {};
        // Names the pages under this one, with addSubItem() for each of them. Called before a
        // crumb drops its list, unless that page is already marked Fetched.
        //
        // ONLY FOR A PAGE THAT SAYS IT HAS A LIST. A crumb carries the strip that drops one only
        // where PageData::hasSubItems() answers yes, so a browser whose pages are enumerated
        // lazily has to mark them FetchState::HasChildren in initPageData - which runs for every
        // page as it is built. A page reached with no sub-items and no such mark reads as a leaf
        // and is never asked about here.
        //
        // Marking the page Fetched belongs to the answer, and says the set will not change while
        // the application is up. A browser reading a directory leaves it unmarked and is asked
        // again on every drop, which is what keeps a crumb current.
        //
        // A FETCH NAMES THE WHOLE SET, and what it does not name is freed: that is what lets a
        // crumb's list follow a source written to while the application is up. The one thing kept
        // unnamed is a page a tab stands on or under - a tab holds its page data by pointer, and
        // m_selectedPageData is one of those - which stays until that tab leaves it.
        virtual void fetchSubItems(PageData&) {}
        // Names a page under another one, answering the page already there when it has been
        // reached before. This is what a fetchSubItems override adds with: it carries the
        // initPageData callback, which a page built any other way never gets.
        //
        // THE ORDER A FETCH NAMES PAGES IN IS THE ORDER THEY ARE SHOWN. Each one is moved into
        // place as it is named, so a page walked into before the fetch ran does not keep wherever
        // it happened to land then. Anything the fetch does not name is left at the end of the
        // list, which is where ensureSubItems looks for the pages that have gone.
        //
        // A name given twice in one fetch answers with the same page both times, and that page
        // keeps the place its first naming gave it. So a browser whose sources overlap - a
        // built-in theme whose file has been saved into the directory it also lists - states them
        // as they come without having to sort the overlap out first.
        PageData& addSubItem(PageData& parent, std::wstring_view name);
        virtual void showPage(BrowserTab&) {};
        virtual void saveViewState(BrowserTab&) {};
        void storeSelectedTab(const BrowserTab*) const;
        void storeTabSettings(const BrowserTab&) const;
        void deleteTabSettings(BrowserTab&);
        // Whether this tab may leave the page it is on. Asked before anything moves, so a browser
        // holding work that is not saved can put the question to the user and answer no.
        //
        // Every route to another page goes through goTo(), and closing a tab asks it too.
        // Answering here rather than in showPage is what makes it refusable: by the time a page is
        // shown the tab's page data has already changed, and putting it back would have taken the
        // caption, the crumbs and the tab's icon with it and back again.
        //
        // The initiator is what the user pressed to leave, and it is what a question is dropped
        // under. Where a route cannot name one - a page navigating by path - it is the tab, which
        // is what the question is about either way.
        virtual bool canLeavePage(BrowserTab&, Control& /*initiator*/) { return true; }
        // Whether this page's name is the user's to change. Asked of the crumb that names the page
        // the browser is showing, as that crumb is given its page, and of no other crumb - a name
        // is changed where the thing it names is open.
        //
        // A browser that answers yes carries the editor and the gestures that open one, so this is
        // the whole of what it takes to make a page renameable from the bar.
        virtual bool canRenamePage(PageData&) { return false; }
        // Renames what this page stands for - the file behind it, the record it names - or refuses
        // the name and says why, which keeps the editor standing with the reason over it and the
        // value still there to be corrected.
        //
        // ANSWERS THE NAME THE PAGE IS KNOWN BY AFTERWARDS, which is what the browser writes into
        // the page and walks the new paths out of. A page that is a file is known by its file name
        // while the user typed a title, so the two are not one string. Nothing is the answer both
        // for a name that was refused and for one the page already had: in either case the page
        // stands where it stood, and nothing below has to be put right.
        virtual std::wstring renamePage(PageData&, AcceptEditEvent&) { return {}; }
        // What a tab shows in its icon slot. A tab reaches its icon by asking the browser rather
        // than by being a class of its own, so a browser can answer per tab - from the page, its
        // data, or nothing - without the strip needing a tab type per answer.
        //
        // A zero size is how a browser says this tab has no icon, and closes the slot. The size is
        // asked for whenever the tab's page data is set, which is before the page behind it is
        // built, so an answer that needs a page must have a second one for the tab that has none
        // yet - a tab restored from settings carries its icon before it is first opened.
        virtual IconSize tabIconSize(BrowserTab&) { return IconSize{ 0.0f }; }
        virtual void paintTabIcon(BrowserTab&, PaintIconEvent&) {}
        // What a page shows for itself, wherever the page rather than the tab is what is on
        // screen: the root crumb of the path, and every line of the list a crumb drops. Keyed by
        // page data rather than by control for that reason - one answer serves both.
        //
        // A zero size is how a browser says this page has no icon, and closes the slot. Only the
        // root crumb asks it; a list line sizes its own slot and asks only for the paint, so a
        // page with no icon there is one paintPageIcon leaves blank.
        virtual IconSize pageIconSize(PageData&) { return IconSize{ 0.0f }; }
        virtual void paintPageIcon(PageData&, PaintIconEvent&) {}
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        bool tabsRestoring() const { return m_tabsRestoring; }
    private:
        bool idExists(std::wstring value);
        // Claims Browser::Actions::open and openInNewTab, and answers them against whatever the
        // page on screen names. Connected from the constructor, the way TextBox connects the
        // edit actions.
        void connectPageActions();
        // Puts the name the user typed to the browser, and puts the tree right when it is taken.
        void acceptPageName(PageData&, AcceptEditEvent&);
        // Whether a page IS the one named or stands under it - which is every page whose path a
        // rename of that page has just changed.
        [[nodiscard]] static bool isPageUnder(const PageData&, const PageData& ancestor);
        void selectPage(BrowserTab*);
        void handleTabSelect();
        void tabClosing(BrowserTab&);
        void tabClosed();
        void pagePathChanged();
        void ensureSubItems(PageData&);
        // Frees the sub-items a fetch of this page did not name.
        void dropGoneSubItems(PageData&);
        // Whether anything still reaches this page, which is what keeps one the fetch did not name
        // standing in the tree.
        [[nodiscard]] bool isPageInUse(const PageData&) const;
    private:
        PageData m_homePageData{ nullptr, ConfigNames::homePage };
        PageData* m_selectedPageData{};
        // The page a fetch is running for, and how many of its sub-items that fetch has named so
        // far. addSubItem moves each one to that position - see the order it states.
        PageData* m_fetchingPage{};
        std::size_t m_fetchedCount{ 0 };
        UiTimer m_goToTimer{};
        std::wstring m_goToPath{};
        BrowserSettings* m_settings{};
        bool m_tabsRestoring{};
    private:
        AddPageDataCallback m_addPageDataCallback;
        FormTitle& m_title{ createTopBar<FormTitle>(
            WordWrap::No,
            MinSize{ 0.0f, 36.0f },
            Spacing{ 4.0f, 0.0f },
            OnEvent{ [](GetTextEvent& event) {
                // checking the calcOnly, because all the space should be available for tabs,
                // so the
                if (event.phase() == EventPhase::Paint)
                    event.text
                        << TextAlign::Center
                        << event.control().appContext().appName();
            } }
        ) };
        StackPanel& m_titleBox{ m_title.createLeftBar<StackPanel>(
            Orientation::Horizontal,
            Padding{ 4.0f, 0.0f },
            Spacing{ 4.0f, 0.0f }
        ) };
        AppButton& m_appButton{ m_titleBox.add<AppButton>(
            VerticalAlign::Center
        ) };
        TabStrip& m_tabs{ m_titleBox.add<TabStrip>(
        ) };
        //Separator& m_plusButtonSeparator{ m_titleBox.add<Separator>(Padding{ 0.0f, 8.0f }) };
        ToolButton& m_plusButton{ m_titleBox.add<ToolButton>(
            FixedSize{ 28.0f },
            Radius{ 12.0f },
            Padding{ 14.0f },
            VerticalAlign::Bottom,
            ButtonViewMode::IconOnly,
            IconSize{ 12.0f },
            ButtonBase::OnPaintIcon{ Icons::PlusMark::paint }
        ) };
        Panel& m_breadCrumbArea{ m_title.createBottomBar<Panel>(
            Padding{ 4.0f },
            Spacing{ 4.0f },
            Radius{ 0.0f },
            UiElement::Bar
        ) };
        BreadCrumbBar& m_breadCrumbBar{ m_breadCrumbArea.createBody<BreadCrumbBar>(
        ) };
        StackPanel& m_navigationBar{ m_breadCrumbArea.createLeftBar<StackPanel>(
            Orientation::Horizontal
        ) };
        Button& m_upButton{ m_navigationBar.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            ButtonBase::OnPaintIcon{ Icons::BrowseUpIcon::paint },
            L"Up"
        ) };
        PageControl& m_pageControl{ createBody<PageControl>() };
    };


    //-----------------------------------------------------------------------------


    namespace
    {
        // Holds the page a fetch is running for, and lets it go however that fetch ends. Cleared
        // by a statement after the call instead, a fetch that threw would leave the browser
        // believing one was still running, and the next page named under it would be ordered
        // against a count belonging to nothing.
        class ScopedFetch
        {
        public:
            ScopedFetch(PageData*& fetchingPage, PageData& pageData)
                :
                m_fetchingPage{ fetchingPage }
            {
                m_fetchingPage = &pageData;
            }
            // A guard whose whole business is holding a scope must not be copied out of it.
            ScopedFetch(const ScopedFetch&) = delete;
            ScopedFetch& operator=(const ScopedFetch&) = delete;
            ~ScopedFetch()
            {
                m_fetchingPage = nullptr;
            }
        private:
            PageData*& m_fetchingPage;
        };
    }


    // BrowserTab

    template<IsBrowserPage PageClass, typename ...Args>
    PageClass& BrowserTab::createPage(Args && ...args)
    {
        PageClass& result = m_browserControl->pageControl().add<PageClass>(this, std::forward<Args>(args)...);
        return result;
    }

    void BrowserTab::setPageData(PageData* value)
    {
        m_pageData = value;
        updateIconMode();
    }

    void BrowserTab::updateIconMode()
    {
        if (!m_browserControl)
            return;

        IconSize size = m_browserControl->tabIconSize(*this);
        bool hasIcon = size.x > 0.0f && size.y > 0.0f;
        setViewMode(hasIcon ? ButtonViewMode::LeftIcon : ButtonViewMode::TextLabel);
        if (hasIcon)
        {
            setIconSize(size);
        }
        // What this tab shows has just been asked again, which is only ever done because the
        // answer may have moved. A size that comes back the same still leaves a different picture
        // to draw, and neither setter invalidates for one that has not changed.
        invalidate();
    }

    void BrowserTab::setId(BrowserControl& browser, const std::wstring_view id)
    {
        m_browserControl = &browser;
        m_id = id;
    }

    RichControl* BrowserTab::visualPage(RichControl*)
    {
        return &m_browserControl->m_breadCrumbArea;
    }

    void BrowserTab::getTooltip(GetTooltipEvent& event)
    {
        Tab::getTooltip(event);
        event.placement = FormPlacement::Bottom;
    }

    void BrowserTab::getText(GetTextEvent& event) const
    {
        m_pageData->paintText(event.text);
    }

    void BrowserTab::paintIcon(PaintIconEvent& event)
    {
        Tab::paintIcon(event);
        if (m_browserControl)
        {
            m_browserControl->paintTabIcon(*this, event);
        }
    }

    void BrowserTab::closeThisTab(ClickEvent& event)
    {
        // Closing a tab loses whatever its page holds exactly as navigating away from it does, so
        // it asks the same question - and asks it before the tab beside this one is selected,
        // which is the first thing below that cannot be taken back.
        //
        // Under the tab rather than under the close button inside it. The button is the press, but
        // the tab is what the question is about, and it is the shape the user has been reading -
        // a question hung off a corner of it would read as being about the corner.
        if (m_browserControl && !m_browserControl->canLeavePage(*this, *this))
            return;

        ControlSpan tabs = parent().controls();
        if (tabs.size() == 1ull) // is last remaining tab
            // so the tab remains selected and will be reselected on the next initialization
            return event.closeForm();

        if (selected())
        {
            // Selecting the item that supposed to be selected after this one will be closed
            auto prevIterator = std::ranges::find_if(
                tabs,
                [&](const ControlPtr& aTab){
                    return this == &*aTab;
                }
            );
            if (prevIterator < std::prev(tabs.end()))
            {
                ++prevIterator;
                static_cast<Tab&>(**prevIterator).select();
            }
            else if (prevIterator != tabs.begin())
            {
                --prevIterator;
                static_cast<Tab&>(**prevIterator).select();
            }
        }

        BrowserControl* browser = m_browserControl;
        browser->tabClosing(*this);
        deleteSelf();
        browser->tabClosed();

        // Todo: to implement Chrome behavior (delayed tabs realigning),
        // first shift the tabs on right to the freshly vacant space,
        // without changing their size, then make invalidateAlign() call on hotLeave
        // (but make sure to do that only if user is using the mouse)
        // event.form.validateAlign(); // this cancels invalidateAlign() triggered by closing/reselecting the tabs and/or showing/hiding their pages

        // To highlight a tab, or its close button that moved under the mouse
        event.form.mouseTick();
    }

    // BrowserPage

    template<typename ...Args>
    BrowserPage::BrowserPage(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_tab{ *Props::get<BrowserTab*>(nullptr, std::forward<Args>(args)...) },
        m_browserControl{m_tab.browserControl() },
        // saving the original pageData, to be able
        // to select it in a folder after level up.
        m_pageData{ *m_tab.pageData() }
    {
    }

    BrowserSettings& BrowserPage::settings() const
    {
        return m_browserControl.settings();
    }

    Dom::Section& BrowserPage::tabConfig() const
    {
        return settings().tabConfig(tabId());
    }

    void BrowserPage::setPath(std::wstring_view newPath) const
    {
        PageData* newData = m_browserControl.homePageData().findOrAdd(newPath, m_browserControl.m_addPageDataCallback);
        m_tab.setPageData(newData);
        {
            // That's actually is right only if the page is selected
            m_browserControl.m_selectedPageData = newData;
            m_browserControl.pagePathChanged();
        }
    }

    // BrowserControl

    template<typename ...Args>
    BrowserControl::BrowserControl(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        m_addPageDataCallback = [this](PageData& newPageData)
            {
                initPageData(newPageData);
            };

        m_tabs.setOverlayHost(m_title);

        m_plusButton.onClick([this](ClickEvent& event) {
            addTab();
            event.form.mouseTick();
            });

        m_tabs.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            handleTabSelect();
            });

        m_breadCrumbBar.onSelectBrowserData([this](SelectBrowserDataEvent& event) {
            goTo(event.pageData, event.initiator);
            });

        m_breadCrumbBar.onFetchSubItems([this](FetchSubItemsEvent& event) {
            ensureSubItems(event.pageData);
            });

        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_goToTimer.onTick([this](TimerEvent&) { goTo(m_goToPath); });

        m_breadCrumbBar.onGetPageIconSize([this](GetPageIconSizeEvent& event) {
            event.size = pageIconSize(event.pageData);
            });

        m_breadCrumbBar.onPaintPageIcon([this](PaintPageIconEvent& event) {
            paintPageIcon(event.pageData, event);
            });

        m_breadCrumbBar.onCanRenamePage([this](CanRenamePageEvent& event) {
            event.canRename = canRenamePage(event.pageData);
            });

        m_breadCrumbBar.onRenamePage([this](RenamePageEvent& event) {
            acceptPageName(event.pageData, event.accept);
            });

        m_upButton.onClick([this](ClickEvent&) { goUp(); });

        m_upButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = m_selectedPageData && m_selectedPageData->parent;
            });

        connectPageActions();
    }

    void BrowserControl::initialize(BrowserSettings& settings)
    {
        m_settings = &settings;

        initPageData(m_homePageData);

        m_tabsRestoring = true;
        const std::wstring selectedTabId = settings.selectedTab().get();
        // The stored one, and the first tab where no restored tab carries that id.
        BrowserTab* tabToSelect{};
        for (const Dom::Section& tabEntry : settings.openTabs())
        {
            const std::wstring tabId = (tabEntry / ConfigNames::id).get<std::wstring>();
            const std::wstring tabPath = (tabEntry / ConfigNames::path).get<std::wstring>();
            BrowserTab* newTab = restoreTab(tabId, tabPath);
            if (!newTab)
                continue;
            if (!tabToSelect || tabId == selectedTabId)
                tabToSelect = newTab;
        }
        if (tabToSelect)
            tabToSelect->select();
        m_tabsRestoring = false;

        // A browser always has a tab, so a run that restores none starts on the home page.
        if (m_tabs.controls().empty())
            addTab();
    }

    void BrowserControl::addTab()
    {
        addTab(m_homePageData);
    }

    void BrowserControl::addTab(const std::wstring_view path)
    {
        // The same assumption goTo(std::wstring_view) makes: a path handed to the browser names
        // a page the tree can reach, and findOrAdd builds the ones it has not been asked for yet.
        addTab(*findPageData(path));
    }

    void BrowserControl::addTab(PageData& pageData)
    {
        // Generate new tab Id
        int n = 0;
        std::wstring newTabId;
        do newTabId = L"Tab" + std::to_wstring(++n);
        while (idExists(newTabId));

        BrowserTab& tab = m_tabs.add<BrowserTab>();
        settings().addTabEntry(newTabId);

        tab.setId(*this, newTabId);
        tab.setPageData(&pageData);
        tab.select();

        // in case if there the same page is on both tabs.
        // But may be it worth it to move that into the item's constructor
        // (== notify the form every time if an item has been created)
        form().invalidateAlign();
    }

    BrowserTab* BrowserControl::restoreTab(const std::wstring_view tabId, const std::wstring_view path)
    {
        PageData* pageData = m_homePageData.findOrAdd(path, m_addPageDataCallback);
        if (!pageData)
            return nullptr;
        BrowserTab& tab = m_tabs.add<BrowserTab>();
        tab.setId(*this, tabId);
        tab.setPageData(pageData);
        // The view state under this tab was written for this very page by the run that stored it,
        // so the first page shown here reads it rather than being handed the page's own source.
        tab.m_onRestoredPage = true;
        return &tab;
    }

    void BrowserControl::goUp()
    {
        goTo(*(m_selectedPageData->parent), m_upButton);
    }

    void BrowserControl::goTo(std::wstring_view path, std::wstring_view subPath)
    {
        std::wstring combinedPath{ path };
        combinedPath.append(subPath);
        goTo(combinedPath);
    }

    void BrowserControl::goTo(std::wstring_view path)
    {
        PageData& pageData = *m_homePageData.findOrAdd(path, m_addPageDataCallback);
        goTo(pageData);
    }

    void BrowserControl::goTo(PageData& pageData)
    {
        // The tab stands in for a caller that cannot name what was pressed - it is what the
        // question would be about in any case.
        goTo(pageData, *m_tabs.currentItem());
    }

    void BrowserControl::goTo(PageData& pageData, Control& initiator)
    {
        auto& tab = static_cast<BrowserTab&>(*m_tabs.currentItem());
        // Going to the page already open is not leaving it, and must not raise the question.
        if (tab.pageData() != &pageData && !canLeavePage(tab, initiator))
            return;

        tab.setPageData(&pageData);
        selectPage(&tab);
        storeTabSettings(tab);
    }

    void BrowserControl::goToLater(const std::wstring_view path)
    {
        m_goToPath = path;
        m_goToTimer.start(MilliSeconds{ 0u });
    }

    PageData* BrowserControl::findPageData(std::wstring_view path)
    {
        return m_homePageData.findOrAdd(path, m_addPageDataCallback);
    }

    // Everything in the page control was put there by BrowserTab::createPage, which takes an
    // IsBrowserPage, so there is nothing else a current item can be.
    BrowserPage* BrowserControl::currentPage()
    {
        return static_cast<BrowserPage*>(m_pageControl.currentItem());
    }

    PageData& BrowserControl::addSubItem(PageData& parent, const std::wstring_view name)
    {
        PageData* subItem = parent.findSubItem(name);
        if (!subItem)
            subItem = &parent.addSubItem(name, m_addPageDataCallback);

        // Ordered only while a fetch of this very page is running. A fetch names what is under the
        // page it was given, so a call about any other page is one this count says nothing about.
        if (&parent == m_fetchingPage)
        {
            // A page this fetch has already named stands where it put it, which is below the
            // count. Naming it twice must not spend a second place: a browser that lists one name
            // twice - a built-in whose file has been saved into the directory it lists - gets the
            // same page back both times, and the pages after it keep the places they were given.
            if (parent.subItemIndex(*subItem) >= m_fetchedCount)
            {
                parent.moveSubItemTo(*subItem, m_fetchedCount);
                ++m_fetchedCount;
            }
        }

        return *subItem;
    }

    void BrowserControl::storeSelectedTab(const BrowserTab* tab) const
    {
        Dom::DomNodeBase& valueNode = m_settings->selectedTab();
        valueNode.set(tab ? tab->id() : L"");
    }

    void BrowserControl::storeTabSettings(const BrowserTab& tab) const
    {
        Dom::Section& tabEntry = m_settings->tabEntry(tab.id());
        (tabEntry / ConfigNames::title).set(tab.pageData()->title);
        (tabEntry / ConfigNames::path).set(tab.pageData()->path());
    }

    // The view state is saved while the entry it writes to is still listed.
    void BrowserControl::deleteTabSettings(BrowserTab& tab)
    {
        if (tab.page())
        {
            saveViewState(tab);
            tab.page()->deleteSelf();
        }
        m_settings->deleteTabEntry(tab.id());
    }

    void BrowserControl::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& newDimensions)
    {
        Panel::alignContent(event, position, newDimensions);

        float clientWidth = m_title.bodyRect().width();
        if (clientWidth < 0.0f)
        {
            float delta = -clientWidth;
            m_tabs.fitTabs(delta);
            for (ControlPtr& ptr : m_tabs.controls())
            {
                BrowserTab& tab = static_cast<BrowserTab&>(*ptr);
                if (!tab.selected())
                {
                    if (tab.width() > event.scale(48.f))
                        break;
                    setControlWidth(tab.closeButton(), 0);
                }
            }
            setControlWidth(m_titleBox, m_titleBox.width() - delta);
            Panel::alignContent(event, position, newDimensions);
        }
    }

    bool BrowserControl::idExists(std::wstring value)
    {
        for (const BrowserTab& tab: m_tabs.controlsAs<BrowserTab>())
            if (tab.id() == value)
                return true;

        return false;
    }

    void BrowserControl::connectPageActions()
    {
        Actions::registerAll();

        // CLAIMED BY THE BROWSER, NAMED BY THE PAGE. Opening belongs to the browser - it owns the
        // tabs and the route between pages - and what to open belongs to the page, because only
        // the page knows what the user has picked in it. So the browser claims both wherever the
        // walk reaches it, and a page naming nothing is a state the commands are disabled in
        // rather than a reason to say nothing: the commands are still about this browser.
        onGetActionState([this](GetActionStateEvent& event){
            if (&event.action != &Actions::open && &event.action != &Actions::openInNewTab)
                return;
            BrowserPage* page = currentPage();
            event.claim({ .enabled = page && !page->pathToOpen().empty() });
        });

        onActionClick([this](ActionClickEvent& event){
            BrowserPage* page = currentPage();
            if (!page)
                return;
            const std::wstring path = page->pathToOpen();
            if (path.empty())
                return;

            // A COMMAND THAT NAVIGATES TAKES THE PAGE IT WAS RUN FROM DOWN WITH IT, and the
            // frames above it - a menu item, the press under that - go on reading controls that
            // went with the page. goToLater is the wait that answers it.
            if (&event.action == &Actions::open)
                goToLater(path);
            // A new tab leaves the page it was asked from standing: the strip gains a tab and
            // the page behind it is hidden rather than freed, so there is nothing to wait for.
            else if (&event.action == &Actions::openInNewTab)
                addTab(path);
        });
    }

    void BrowserControl::acceptPageName(PageData& pageData, AcceptEditEvent& event)
    {
        const std::wstring newName = renamePage(pageData, event);
        // A refusal, or a name the page already had. Either way nothing under the page has moved,
        // and the editor is left to whatever the refusal said.
        if (newName.empty())
            return;

        pageData.rename(newName);
        // The title went with the name, and this is where a title is worked out - the same hook
        // that gave the page its first one.
        initPageData(pageData);

        // EVERY PAGE UNDER THIS ONE IS AT A NEW PATH: a path is walked from the name each page on
        // it carries, and one of those names has just changed. So every tab standing on such a
        // page is stored under a path that names nothing, until it is written out again.
        for (BrowserTab& tab : tabs())
        {
            if (!tab.pageData() || !isPageUnder(*tab.pageData(), pageData))
                continue;

            storeTabSettings(tab);
            // A tab's caption is its page's title, and the title has just been worked out again.
            tab.invalidate();
        }

        // THE PAGE ITSELF IS STILL THE ONE THE BROWSER IS SHOWING, so this rebuilds the same
        // crumbs over the same pages and the crumb the editor stands on is one of them. It runs
        // while that editor is up - the sink is called from inside it - and dropping the crumb
        // from under it would take the edit down as well.
        pagePathChanged();
    }

    bool BrowserControl::isPageUnder(const PageData& pageData, const PageData& ancestor)
    {
        for (const PageData* walk = &pageData; walk; walk = walk->parent)
            if (walk == &ancestor)
                return true;

        return false;
    }

    void BrowserControl::selectPage(BrowserTab* tab)
    {
        m_selectedPageData = tab ? tab->pageData() : nullptr;
        if (tab)
        {
            showPage(*tab);
            // SPENT AFTER THE PAGE HAS BEEN SHOWN, which is what reads it. From here on the view
            // state under this tab is whatever the page just put there, so the next page shown on
            // it is an ordinary navigation.
            tab->m_onRestoredPage = false;
        }
        else
            m_pageControl.setCurrentItem(nullptr);
        pagePathChanged();
    }

    void BrowserControl::handleTabSelect()
    {
        const auto tab = static_cast<BrowserTab*>(m_tabs.currentItem());
        if (tab)
            goTo(*tab->pageData());
        else
            selectPage(nullptr);

        // Even if the page has not been changed, we still have to realign items,
        // because the close button on non-selected tabs can be hidden while aligning
        form().invalidateAlign();
        storeSelectedTab(tab);
    }

    void BrowserControl::tabClosing(BrowserTab& tab)
    {
        deleteTabSettings(tab);
    }

    void BrowserControl::tabClosed()
    {
    }

    void BrowserControl::pagePathChanged()
    {
        m_breadCrumbBar.createItems(m_selectedPageData);
        m_upButton.invalidateState();
        invalidateFormAlign();
    }

    void BrowserControl::ensureSubItems(PageData& pageData)
    {
        if (pageData.fetchState == FetchState::Fetched)
            return;

        m_fetchedCount = 0;
        {
            const ScopedFetch fetching{ m_fetchingPage, pageData };
            fetchSubItems(pageData);
        }
        dropGoneSubItems(pageData);

        // HasChildren is a promise made before anything under the page had been named. Having
        // asked and been given nothing, the promise is what was wrong: leaving it would keep
        // offering a list that opens on nothing. A browser that learns better says so by marking
        // the page again.
        if (pageData.items.empty() && pageData.fetchState == FetchState::HasChildren)
            pageData.fetchState = FetchState::Unfetched;
    }

    void BrowserControl::dropGoneSubItems(PageData& pageData)
    {
        // Every name the fetch gave was moved to the front as it was given, so the sub-items from
        // m_fetchedCount on are the ones it did not name - the pages that have gone from wherever
        // the browser reads. Read here, while the count still belongs to the fetch that just ran.
        pageData.dropSubItemsFrom(m_fetchedCount, [this](const PageData& subItem){
            return isPageInUse(subItem);
        });
    }

    bool BrowserControl::isPageInUse(const PageData& pageData) const
    {
        // A TAB HOLDS THE PAGE DATA IT STANDS ON BY POINTER, and reaches it through every page of
        // the path down to it, so a page on the way to an open tab is as much in use as the one
        // that tab is on. Freeing either would leave the tab, its crumbs and its page pointing at
        // nothing.
        for (const BrowserTab& tab : tabs())
            if (tab.pageData() && isPageUnder(*tab.pageData(), pageData))
                return true;

        return false;
    }

}
