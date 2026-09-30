module ClaFi.Browser.Control;

import ClaFi.Browser.Actions;
import ClaFi.Browser.BreadCrumbBar;
import ClaFi.Browser.Consts;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.TabStrip;

import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;

import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;

import ClaFi.StdLib;

namespace ClaFi::Browser
{
    using namespace Controls;

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

        using UrlSpellings = std::vector<std::wstring>;

        [[nodiscard]] BrowserTab::Urls urlsIn(const Dom::Section& tabEntry,
            const std::wstring_view key)
        {
            BrowserTab::Urls result{};
            for (const std::wstring& spelling : (tabEntry / key).get<UrlSpellings>())
                result.push_back(Url::parse(spelling));
            return result;
        }

        void storeUrls(Dom::Section& tabEntry, const std::wstring_view key,
            const BrowserTab::Urls& urls)
        {
            UrlSpellings spellings{};
            spellings.reserve(urls.size());
            for (const Url& url : urls)
                spellings.push_back(url.str());
            (tabEntry / key).set(spellings);
        }

        // Points each url at or under oldPath to newPath, and answers whether one moved.
        [[nodiscard]] bool renameUnder(BrowserTab::Urls& urls, const std::wstring_view oldPath,
            const std::wstring_view newPath)
        {
            bool renamed = false;
            for (Url& url : urls)
            {
                const std::wstring& path = url.path();
                const bool under = path.starts_with(oldPath)
                    && (path.size() == oldPath.size()
                        || path[oldPath.size()] == ConfigNames::pathSeparator);
                if (!under)
                    continue;

                std::wstring movedPath{ newPath };
                movedPath.append(path, oldPath.size());
                url = Url{ movedPath, url.anchor() };
                renamed = true;
            }
            return renamed;
        }

        // The step an action takes through a tab's history, and none for any other action.
        [[nodiscard]] std::ptrdiff_t historyStep(const Action& action)
        {
            if (&action == &Actions::back)
                return -1;
            if (&action == &Actions::forward)
                return 1;
            return 0;
        }

        void invalidateHistoryState()
        {
            Actions::back.invalidateState();
            Actions::forward.invalidateState();
        }
    }


    // BrowserTab

    BrowserTab::BrowserTab(const CreateParams& params)
        :
        Tab{ params, WordWrap::No, Spacing{ 4.0f }, Padding{ 8.0f, 4.0f }, MinSize{ 0.0f, 28.0f } }
    {
        // Connected here rather than given to the button as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_closeButton.onGetTooltip([this](GetTooltipEvent& event) {
            event.text << L"Close " << InkWell::textInk(InkGrade::Muted);
            m_pageData->paintText(event.text);
            event.placement = FormPlacement::Mouse;
        });
    }

    Url BrowserTab::url() const
    {
        return Url{ m_pageData->path(), m_anchor };
    }

    const Url* BrowserTab::historyUrl(const std::ptrdiff_t offset) const
    {
        if (offset < 0)
        {
            const std::size_t steps = static_cast<std::size_t>(-offset);
            return steps <= m_backUrls.size() ? &m_backUrls[m_backUrls.size() - steps] : nullptr;
        }
        const std::size_t steps = static_cast<std::size_t>(offset);
        return steps != 0 && steps <= m_forwardUrls.size() ? &m_forwardUrls[steps - 1] : nullptr;
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

    void BrowserTab::nestedGetTooltip(GetTooltipEvent& event)
    {
        Tab::nestedGetTooltip(event);
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

    void BrowserTab::recordLeft(const Url& left, const HistoryEntry entry)
    {
        if (entry == HistoryEntry::Push)
        {
            m_backUrls.push_back(left);
            m_forwardUrls.clear();
            trimHistory();
            return;
        }

        // A replace may land on the url beside it, and an entry naming the url on screen is a
        // step that goes nowhere.
        const Url here = url();
        if (!m_backUrls.empty() && m_backUrls.back() == here)
            m_backUrls.pop_back();
        if (!m_forwardUrls.empty() && m_forwardUrls.front() == here)
            m_forwardUrls.erase(m_forwardUrls.begin());
    }

    // The urls passed over change sides along with the url left, keeping the order they were
    // visited in.
    void BrowserTab::recordTravel(const Url& left, const std::ptrdiff_t offset)
    {
        if (offset < 0)
        {
            const Urls::iterator target = m_backUrls.end() + offset;
            m_forwardUrls.insert(m_forwardUrls.begin(), left);
            m_forwardUrls.insert(m_forwardUrls.begin(), target + 1, m_backUrls.end());
            m_backUrls.erase(target, m_backUrls.end());
            return;
        }

        const Urls::iterator target = m_forwardUrls.begin() + (offset - 1);
        m_backUrls.push_back(left);
        m_backUrls.insert(m_backUrls.end(), m_forwardUrls.begin(), target);
        m_forwardUrls.erase(m_forwardUrls.begin(), target + 1);
        trimHistory();
    }

    void BrowserTab::trimHistory()
    {
        if (m_backUrls.size() <= k_historyDepth)
            return;

        const auto excess = static_cast<Urls::difference_type>(m_backUrls.size() - k_historyDepth);
        m_backUrls.erase(m_backUrls.begin(), m_backUrls.begin() + excess);
    }

    bool BrowserTab::renameInHistory(const std::wstring_view oldPath,
        const std::wstring_view newPath)
    {
        const bool backRenamed = renameUnder(m_backUrls, oldPath, newPath);
        const bool forwardRenamed = renameUnder(m_forwardUrls, oldPath, newPath);
        return backRenamed || forwardRenamed;
    }

    // BrowserPage

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

    void BrowserControl::initialize(BrowserSettings& settings)
    {
        m_settings = &settings;

        // The form is there by now, which is what puts this here rather than in the constructor.
        form().onClosing([this](FormClosingEvent& event) {
            if (!canCloseWindow())
                event.refuse();
        });

        initPageData(m_homePageData);

        m_tabsRestoring = true;
        const std::wstring selectedTabId = settings.selectedTab().get();
        // The stored one, and the first tab where no restored tab carries that id.
        BrowserTab* tabToSelect{};
        for (const Dom::Section& tabEntry : settings.openTabs())
        {
            const std::wstring tabId = (tabEntry / ConfigNames::id).get<std::wstring>();
            const std::wstring tabUrl = (tabEntry / ConfigNames::path).get<std::wstring>();
            BrowserTab* newTab = restoreTab(tabId, Url::parse(tabUrl));
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

    void BrowserControl::addTab(PageData& pageData, const std::wstring_view anchor)
    {
        // Generate new tab Id
        int n = 0;
        std::wstring newTabId;
        do
        {
            newTabId = L"Tab" + std::to_wstring(++n);
        }
        while (idExists(newTabId));

        BrowserTab& tab = m_tabs.add<BrowserTab>();
        settings().addTabEntry(newTabId);

        tab.setId(*this, newTabId);
        tab.setPageData(&pageData);
        tab.m_anchor = anchor;
        tab.select();

        // in case if there the same page is on both tabs.
        // But may be it worth it to move that into the item's constructor
        // (== notify the form every time if an item has been created)
        form().invalidateAlign();
    }

    void BrowserControl::addTab(const Url& url)
    {
        // The same assumption goTo(const Url&) makes: a path handed to the browser names a page
        // the tree can reach, and findOrAdd builds the ones it has not been asked for yet.
        addTab(*findPageData(url.path()), url.anchor());
    }

    BrowserTab* BrowserControl::restoreTab(const std::wstring_view id, const Url& url)
    {
        PageData* pageData = m_homePageData.findOrAdd(url.path(), m_addPageDataCallback);
        if (!pageData)
            return nullptr;
        BrowserTab& tab = m_tabs.add<BrowserTab>();
        tab.setId(*this, id);
        tab.setPageData(pageData);
        tab.m_anchor = url.anchor();
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
        goTo(Url{ combinedPath });
    }

    void BrowserControl::goTo(const Url& url, const HistoryEntry entry)
    {
        // The tab stands in for a caller that cannot name what was pressed - it is what the
        // question would be about in any case.
        goTo(*findPageData(url.path()), url.anchor(), *m_tabs.currentItem(), entry);
    }

    void BrowserControl::goTo(PageData& pageData)
    {
        goTo(pageData, *m_tabs.currentItem());
    }

    void BrowserControl::goTo(PageData& pageData, Control& initiator)
    {
        goTo(pageData, {}, initiator);
    }

    void BrowserControl::goTo(PageData& pageData, const std::wstring_view anchor,
        Control& initiator, const HistoryEntry entry)
    {
        BrowserTab& tab = *currentTab();
        readHistory(tab);
        const Url left = tab.url();
        const TabMove move = moveTab(tab, pageData, anchor, initiator);
        if (move == TabMove::Refused)
            return;

        if (move == TabMove::Moved)
        {
            tab.recordLeft(left, entry);
            storeHistory(tab);
        }
        storeTabSettings(tab);
        invalidateHistoryState();
    }

    void BrowserControl::goToLater(const Url& url)
    {
        m_goToUrl = url;
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
        (tabEntry / ConfigNames::path).set(tab.url().str());
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
            // The title bar alone. A page aligned twice reads the sizes its first align handed out.
            alignControl(&m_title, event, m_title.topLeft(), m_title.dimensions());
        }
    }

    // Whatever the pointer stands on inside the browser, a side button is its Back or Forward.
    void BrowserControl::nestedSideClick(SideClickEvent& event)
    {
        Panel::nestedSideClick(event);
        if (event.propagationStopped())
            return;

        event.stopPropagation();
        const std::ptrdiff_t offset = event.button == SideButton::Back ? -1 : 1;
        travel(offset, historyButton(offset));
    }

    bool BrowserControl::idExists(std::wstring value)
    {
        for (const BrowserTab& tab : m_tabs.controlsAs<BrowserTab>())
            if (tab.id() == value)
                return true;

        return false;
    }

    void BrowserControl::connectParts()
    {
        m_addPageDataCallback = [this](PageData& newPageData) {
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
        m_goToTimer.onTick([this](TimerEvent&) {
            goTo(m_goToUrl);
        });

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

        m_upButton.onClick([this](ClickEvent&) {
            goUp();
        });

        m_upButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = m_selectedPageData && m_selectedPageData->parent;
        });
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
                goToLater(Url{ path });
            // A new tab leaves the page it was asked from standing: the strip gains a tab and
            // the page behind it is hidden rather than freed, so there is nothing to wait for.
            else if (&event.action == &Actions::openInNewTab)
                addTab(Url{ path });
        });
    }

    void BrowserControl::connectHistory()
    {
        onGetActionState([this](GetActionStateEvent& event){
            const std::ptrdiff_t offset = historyStep(event.action);
            if (offset == 0)
                return;
            const BrowserTab* tab = currentTab();
            event.claim({ .enabled = tab && tab->historyUrl(offset) });
        });

        // Deferred for the reason Open is: a presenter standing on the page goes down with it.
        onActionClick([this](ActionClickEvent& event){
            const std::ptrdiff_t offset = historyStep(event.action);
            if (offset != 0)
                travelLater(offset);
        });

        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_travelTimer.onTick([this](TimerEvent&) {
            travel(m_travelOffset, historyButton(m_travelOffset));
        });

        m_backButton.onContextPopup([this](ContextPopupEvent& event) {
            event.stopPropagation();
            showHistoryMenu(-1);
        });

        m_forwardButton.onContextPopup([this](ContextPopupEvent& event) {
            event.stopPropagation();
            showHistoryMenu(1);
        });
    }

    // Everything in the strip was put there by addTab or restoreTab, which make BrowserTabs.
    // A WINDOW CLOSING WITH THE SETTINGS UNKEPT IS EVERY TAB LEAVING ITS PAGE. A page's work
    // waits in the tab's view state for the next run, and the view state reaches the disk only
    // where the user allowed storing - so where it will not, each page is asked about now, under
    // its own tab, and the first no keeps the window up.
    bool BrowserControl::canCloseWindow()
    {
        if (appContext().configFolderExists())
            return true;
        for (BrowserTab& tab : tabs())
            if (tab.page() && !canLeavePage(tab, tab))
                return false;
        return true;
    }

    BrowserTab* BrowserControl::currentTab()
    {
        return static_cast<BrowserTab*>(m_tabs.currentItem());
    }

    BrowserControl::TabMove BrowserControl::moveTab(BrowserTab& tab, PageData& pageData,
        const std::wstring_view anchor, Control& initiator)
    {
        const bool samePage = tab.pageData() == &pageData;
        // Going to the page already open is not leaving it, and must not raise the question.
        if (!samePage && !canLeavePage(tab, initiator))
            return TabMove::Refused;

        const bool urlChanged = !samePage || tab.anchor() != anchor;
        tab.setPageData(&pageData);
        if (urlChanged)
            tab.m_anchor = anchor;
        selectPage(&tab, urlChanged);
        return urlChanged ? TabMove::Moved : TabMove::Stayed;
    }

    void BrowserControl::travel(const std::ptrdiff_t offset, Control& initiator)
    {
        BrowserTab* tab = currentTab();
        const Url* target = tab ? tab->historyUrl(offset) : nullptr;
        if (!target)
            return;

        // Copied out of the stacks, which the page's handlers may rewrite during the move.
        const Url destination = *target;
        const Url left = tab->url();
        PageData& pageData = *findPageData(destination.path());
        if (moveTab(*tab, pageData, destination.anchor(), initiator) == TabMove::Refused)
            return;

        tab->recordTravel(left, offset);
        storeHistory(*tab);
        storeTabSettings(*tab);
        invalidateHistoryState();
    }

    void BrowserControl::travelLater(const std::ptrdiff_t offset)
    {
        m_travelOffset = offset;
        m_travelTimer.start(MilliSeconds{ 0u });
    }

    // A line per url, nearest first. The travel runs from inside the menu, whose lines stand
    // apart from the page it takes down.
    void BrowserControl::showHistoryMenu(const std::ptrdiff_t direction)
    {
        const BrowserTab* tab = currentTab();
        if (!tab)
            return;

        Button& button = historyButton(direction);
        Menu menu{ button };
        std::ptrdiff_t offset = direction;
        while (const Url* url = tab->historyUrl(offset))
        {
            PageData& pageData = *findPageData(url->path());
            std::wstring line{ pageData.displayTitle() };
            if (!url->anchor().empty())
                line.append(L" - ").append(url->anchor());
            menu.add(
                line,
                [this, &pageData](PaintIconEvent& event) {
                    paintPageIcon(pageData, event);
                },
                [this, offset](Control& item) {
                    travel(offset, item);
                }
            );
            offset += direction;
        }
        menu.executeUnder(button);
    }

    Button& BrowserControl::historyButton(const std::ptrdiff_t offset)
    {
        return offset < 0 ? m_backButton : m_forwardButton;
    }

    void BrowserControl::readHistory(BrowserTab& tab)
    {
        if (tab.m_historyRead)
            return;

        tab.m_historyRead = true;
        const Dom::Section& tabConfig = m_settings->tabConfig(tab.id());
        tab.m_backUrls = urlsIn(tabConfig, ConfigNames::backUrls);
        tab.m_forwardUrls = urlsIn(tabConfig, ConfigNames::forwardUrls);
    }

    void BrowserControl::storeHistory(const BrowserTab& tab) const
    {
        Dom::Section& tabConfig = m_settings->tabConfig(tab.id());
        storeUrls(tabConfig, ConfigNames::backUrls, tab.backUrls());
        storeUrls(tabConfig, ConfigNames::forwardUrls, tab.forwardUrls());
    }

    void BrowserControl::acceptPageName(PageData& pageData, AcceptEditEvent& event)
    {
        const std::wstring newName = renamePage(pageData, event);
        // A refusal, or a name the page already had. Either way nothing under the page has moved,
        // and the editor is left to whatever the refusal said.
        if (newName.empty())
            return;

        const std::wstring oldPath = pageData.path();
        pageData.rename(newName);
        // The title went with the name, and this is where a title is worked out - the same hook
        // that gave the page its first one.
        initPageData(pageData);
        const std::wstring newPath = pageData.path();

        // EVERY PAGE UNDER THIS ONE IS AT A NEW PATH: a path is walked from the name each page on
        // it carries, and one of those names has just changed. So every tab standing on such a
        // page, and every history entry naming one, is stored under a path that names nothing
        // until it is written out again.
        for (BrowserTab& tab : tabs())
        {
            // A tab not shown in this run has its file read here: its history names paths too.
            readHistory(tab);
            if (tab.renameInHistory(oldPath, newPath))
                storeHistory(tab);
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

    void BrowserControl::selectPage(BrowserTab* tab, const bool urlChanged)
    {
        m_selectedPageData = tab ? tab->pageData() : nullptr;
        bool anchorToShow = false;
        if (tab)
        {
            const RichControl* pageBefore = tab->page();
            showPage(*tab);
            // SPENT AFTER THE PAGE HAS BEEN SHOWN, which is what reads it. From here on the view
            // state under this tab is whatever the page just put there, so the next page shown on
            // it is an ordinary navigation.
            tab->m_onRestoredPage = false;
            // A page this show has just built has not been told its anchor, whatever the url did.
            anchorToShow = urlChanged || tab->page() != pageBefore;
        }
        else
            m_pageControl.setCurrentItem(nullptr);
        pagePathChanged();
        if (anchorToShow)
            showAnchor(*tab);
    }

    // Everything a tab shows was put there by BrowserTab::createPage, which takes an IsBrowserPage.
    void BrowserControl::showAnchor(BrowserTab& tab)
    {
        auto* page = static_cast<BrowserPage*>(tab.page());
        if (!page)
            return;

        const Url url = tab.url();
        ShowAnchorEvent event{ url };
        page->emitEvent(event);
    }

    void BrowserControl::handleTabSelect()
    {
        const auto tab = static_cast<BrowserTab*>(m_tabs.currentItem());
        // The tab's own anchor, so selecting a tab moves it nowhere.
        if (tab)
            goTo(*tab->pageData(), tab->anchor(), *tab);
        else
            selectPage(nullptr, false);

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
        // A FETCH THAT NAMES NOTHING LEAVES A HasChildren MARK STANDING. The mark says the page is
        // a list, and a list read from a folder is empty whenever the folder is - the crumb keeps
        // its strip, and drops the list's placeholder - see BreadCrumbBarItem::showDropdown.
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
