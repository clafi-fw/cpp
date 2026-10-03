module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Browser.Control;

import ClaFi.Browser.Actions;
import ClaFi.Browser.Consts;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;
import ClaFi.Browser.BreadCrumbBar;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.TabStrip;

import ClaFi.Icons.PlusMark;
import ClaFi.Icons.XMark;
import ClaFi.Icons.BrowseUpIcon;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;

import ClaFi.Core.Context.PaintIconEvent;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Url;

import ClaFi.StdLib;


namespace ClaFi::Browser
{
    using namespace Controls;

    export class BrowserControl;
    export class BrowserPage;
    export template<typename T>
        concept IsBrowserPage = std::derived_from<T, BrowserPage>;

    // How a move keeps the url it leaves. See Browser
    export enum class HistoryEntry
    {
        Push,       // the url left goes on the back stack, and the forward stack is dropped
        Replace     // the url left is forgotten, and the forward stack stays
    };

    // One tab of the browser, standing for a page it can return to.
    export class BrowserTab : public Tab
    {
        // The browser is what makes a tab and what shows a page on one, so it is what sets
        // m_anchor and the history, and sets and spends m_onRestoredPage.
        friend BrowserControl;
    public:
        using Urls = std::vector<Url>;
    public:
        explicit BrowserTab(const CreateParams&);
    public:
        template<IsBrowserPage PageClass, typename... Args>
        PageClass& createPage(Args&&... args);
        BrowserControl& browserControl() const { return *m_browserControl; }
        Control& closeButton() const { return m_closeButton; }
        PageData* pageData() const { return m_pageData; }
        [[nodiscard]] const std::wstring& anchor() const { return m_anchor; } // empty for none
        [[nodiscard]] Url url() const; // the page's path with this tab's anchor
        [[nodiscard]] const Urls& backUrls() const { return m_backUrls; } // nearest last
        [[nodiscard]] const Urls& forwardUrls() const { return m_forwardUrls; } // nearest first
        // The url this many steps through the history, back for a negative count, or nothing.
        [[nodiscard]] const Url* historyUrl(std::ptrdiff_t offset) const;
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
        void nestedGetHint(GetHintEvent&) override;
        void getText(GetTextEvent&) const override;
        void paintIcon(PaintIconEvent&) override;
        void secondaryClicked(ClickEvent& event) override { closeThisTab(event); }
    private:
        void closeThisTab(ClickEvent& params);
        // Keeps the url this tab has just moved from, the way the move asked.
        void recordLeft(const Url& left, HistoryEntry);
        // Moves the stacks past a travel of this many steps from the url this tab has just left.
        void recordTravel(const Url& left, std::ptrdiff_t offset);
        void trimHistory(); // drops the oldest entries past k_historyDepth
        // Points every entry at or under a renamed page to its new path. Answers whether one moved.
        bool renameInHistory(std::wstring_view oldPath, std::wstring_view newPath);
    private:
        static constexpr std::size_t k_historyDepth{ 50 };
        BrowserControl* m_browserControl{};
        std::wstring m_id{};
        PageData* m_pageData{};
        std::wstring m_anchor{};
        Urls m_backUrls{};
        Urls m_forwardUrls{};
        bool m_historyRead{ false }; // the stacks hold what the tab's file had
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
            OnEvent{ Icons::XMark::paint }
        );
    };

    // A tab has arrived at a url on this page, which shows the anchor it names. See Browser
    export class ShowAnchorEvent : public Event
    {
    public:
        explicit ShowAnchorEvent(const Url& url) : url{ url } {}
        const Url& url;
    };

    // One page of the browser: what it shows, and the crumbs that lead to it.
    export class BrowserPage : public Panel
    {
    public:
        template<typename... Args>
        explicit BrowserPage(const CreateParams&, Args&&...);
    public:
        // A tab has arrived at a url on this page, which shows the anchor it names. See Browser
        DECLARE_EVENT(ShowAnchorEvent, OnShowAnchor, onShowAnchor)
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
        DialogTitle& title() const { return m_title; }
        AppButton& appButton() const { return m_appButton; }
    public:
        // Opens a tab on the home page, which is where a browser with nothing said to it starts.
        void addTab();
        // Opens one on a page named outright. The new tab is selected, the way a tab the user
        // asked for is.
        void addTab(PageData&, std::wstring_view anchor = {});
        void addTab(const Url&);
        BrowserTab* restoreTab(std::wstring_view id, const Url&);
        void goUp();
        void goTo(std::wstring_view path, std::wstring_view subPath);
        void goTo(const Url&, HistoryEntry = HistoryEntry::Push);
        void goTo(PageData&);
        void goTo(PageData&, Control& initiator);
        void goTo(PageData&, std::wstring_view anchor, Control& initiator,
            HistoryEntry = HistoryEntry::Push);
        // Goes to a page once the press that asked for it has unwound. A COMMAND THAT NAVIGATES
        // FROM INSIDE A CLICK DESTROYS THE PAGE IT WAS PRESSED ON, and the frames above that click
        // go on reading controls that went down with it. The wait is held here rather than on the
        // page, because a page holding it would be destroying the very dispatcher delivering the
        // tick.
        void goToLater(const Url&);
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
        void nestedSideClick(SideClickEvent&) override;
        bool tabsRestoring() const { return m_tabsRestoring; }
    private:
        // What a move made of the tab it was asked of.
        enum class TabMove
        {
            Refused,    // canLeavePage said no, and the tab stands where it stood
            Stayed,     // the tab already stood on the url asked for
            Moved       // the tab stands on the url asked for
        };
    private:
        bool idExists(std::wstring value);
        void connectParts(); // wires the tabs, crumbs, buttons and timer, from the constructor
        // Claims Browser::Actions::open and openInNewTab, and answers them against whatever the
        // page on screen names. Connected from the constructor, the way TextBox connects the
        // edit actions.
        void connectPageActions();
        // Claims Browser::Actions::back and forward, and wires the buttons' history menus.
        void connectHistory();
        [[nodiscard]] BrowserTab* currentTab(); // nothing when no tab is selected
        // Whether the window may close now: yes where the settings will keep every page's work,
        // and otherwise what every page with work of its own answers. See Browser#closing
        [[nodiscard]] bool canCloseWindow();
        // Moves a tab to a url and answers what came of it. The history is the caller's to keep.
        TabMove moveTab(BrowserTab&, PageData&, std::wstring_view anchor, Control& initiator);
        // Moves the current tab this many steps through its history, back for a negative count.
        void travel(std::ptrdiff_t offset, Control& initiator);
        // Travels once the press that asked for it has unwound, for the reason goToLater waits.
        void travelLater(std::ptrdiff_t offset);
        // Drops the current tab's history one way, as a list under the button for that way.
        void showHistoryMenu(std::ptrdiff_t direction);
        [[nodiscard]] Button& historyButton(std::ptrdiff_t offset); // Back for a negative offset
        // Reads a tab's history from its file, once - goTo asks it of every tab it selects.
        void readHistory(BrowserTab&);
        void storeHistory(const BrowserTab&) const; // into the document of the tab's file
        // Puts the name the user typed to the browser, and puts the tree right when it is taken.
        void acceptPageName(PageData&, AcceptEditEvent&);
        // Whether a page IS the one named or stands under it - which is every page whose path a
        // rename of that page has just changed.
        [[nodiscard]] static bool isPageUnder(const PageData&, const PageData& ancestor);
        void selectPage(BrowserTab*, bool urlChanged);
        void showAnchor(BrowserTab&); // raises ShowAnchorEvent on the tab's page
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
        Url m_goToUrl{};
        UiTimer m_travelTimer{};
        std::ptrdiff_t m_travelOffset{ 0 };
        BrowserSettings* m_settings{};
        bool m_tabsRestoring{};
    private:
        AddPageDataCallback m_addPageDataCallback;
        DialogTitle& m_title{ createTopBar<DialogTitle>(
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
            Padding{ 0.0f, 4.0f }
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
            UiElement::ToolBar
        ) };
        BreadCrumbBar& m_breadCrumbBar{ m_breadCrumbArea.createBody<BreadCrumbBar>(
        ) };
        StackPanel& m_navigationBar{ m_breadCrumbArea.createLeftBar<StackPanel>(
            Orientation::Horizontal
        ) };
        Button& m_backButton{ m_navigationBar.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            Actions::back,
            Interactivity::MouseOnly
        ) };
        Button& m_forwardButton{ m_navigationBar.add<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            Actions::forward,
            Interactivity::MouseOnly
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


    // BrowserTab

    template<IsBrowserPage PageClass, typename ...Args>
    PageClass& BrowserTab::createPage(Args && ...args)
    {
        PageClass& result = m_browserControl->pageControl().add<PageClass>(this, std::forward<Args>(args)...);
        return result;
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

    // BrowserControl

    template<typename ...Args>
    BrowserControl::BrowserControl(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        connectParts();
        connectPageActions();
        connectHistory();
    }
}
