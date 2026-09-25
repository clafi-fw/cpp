module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Browser.BreadCrumbBar;

import ClaFi.Browser.PageData;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;
import ClaFi.Controls.StackPanel;

import ClaFi.Icons.Chevron;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;

import ClaFi.StdLib;

namespace ClaFi::Browser
{
    using namespace Controls;

    export class BreadCrumbBar;

    // A crumb was chosen; the browser is being asked to go to that page. See Browser
    export class SelectBrowserDataEvent : public Event
    {
    public:
        SelectBrowserDataEvent(PageData& pageData, Control& initiator)
            :
            pageData{ pageData },
            initiator{ initiator }
        {
        }
        PageData& pageData;
        Control& initiator;
    };

    // A crumb is about to drop its list, and asks what pages lie under this one. See Browser
    export class FetchSubItemsEvent : public Event
    {
    public:
        explicit FetchSubItemsEvent(PageData& pageData) : pageData{ pageData } {}
        PageData& pageData;
    };

    // What a page's icon is. See Browser
    export class GetPageIconSizeEvent : public Event
    {
    public:
        explicit GetPageIconSizeEvent(PageData& pageData) : pageData{ pageData } {}
        PageData& pageData;
        IconSize size{ 0.0f };
    };

    /// @brief Paints a page's icon - in a crumb's own slot, or beside a line of a crumb's list.
    export class PaintPageIconEvent : public PaintIconEvent
    {
    public:
        PaintPageIconEvent(PaintIconEvent& event, PageData& pageData)
            :
            PaintIconEvent{ event },
            pageData{ pageData }
        {
        }
        PageData& pageData;
    };

    // Whether this page's name is the user's to change. See Browser
    export class CanRenamePageEvent : public Event
    {
    public:
        explicit CanRenamePageEvent(PageData& pageData) : pageData{ pageData } {}
        PageData& pageData;
        bool canRename{ false };
    };

    // The name typed over a crumb, on its way to whatever the page stands for. See Browser
    export class RenamePageEvent : public Event
    {
    public:
        RenamePageEvent(PageData& pageData, AcceptEditEvent& accept)
            :
            pageData{ pageData },
            accept{ accept }
        {
        }
        PageData& pageData;
        AcceptEditEvent& accept;
    };

    // BreadCrumbBarItem

    /// @brief One page of the path: the title it is known by, and the pages under it behind a
    /// chevron.
    // A dropdown control: the mark, the strip it rides on, the way that strip is squared against
    // the crumb's outline, F4 and Alt+Down, and the rule about which half owns the popup all
    // belong to the base. The in-place editor over the crumb's own title, and the gestures that
    // open one, come from the mixin. What is left here is the page a crumb names.
    using BreadCrumbBarItemBase = WithInPlaceEdit<DropdownControlBase>;
    class BreadCrumbBarItem : public BreadCrumbBarItemBase
    {
    public:
        explicit BreadCrumbBarItem(const CreateParams&);
    public:
        std::wstring_view diagnosticText() const override { return L"BreadCrumbBarItem"; }
        /// The page this crumb names, and the page the path goes on to from it - nothing for the
        /// crumb at the end of the path. Settles the crumb's title, its icon, whether it has a
        /// strip to drop a list from, its selected look, and which line of its list names the page
        /// the user is already inside.
        void setData(PageData*, const PageData* pathSubItem);
    protected:
        void getMainText(GetTextEvent&) const override;
        void getEditorText(Text&) const override;
        [[nodiscard]] EditorMode editorMode() const override;
        [[nodiscard]] FloatPoint editorMaxTextSize(const FloatRect&) const override;
        [[nodiscard]] bool clickOpensEditor() const override;
        void paintIcon(PaintIconEvent&) override;
        void showDropdown(Control& initiator) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void getControlState(GetStateEvent&) const override;
        void click(ClickEvent&) override;
        // The mark sits on a crumb's face rather than in a field, so it carries the normal text
        // colour rather than the muted one a combobox uses.
        [[nodiscard]] Ink dropdownMarkInk() const override { return InkWell::textInk(); }
        // A crumb's mark points the way the path runs while its list is shut, and turns a quarter
        // clockwise to point into that list once it is down.
        [[nodiscard]] DropdownMarkTurn dropdownMarkTurn() const override
        {
            return { ChevronTurn::right, ChevronTurn::down };
        }
    private:
        // Whether this crumb names the page the browser is showing, which is the crumb the path
        // does not go on from.
        [[nodiscard]] bool isCurrentPage() const { return !m_pathSubItem; }
        void updateIconMode();
        void updateDropdownMode();
        PageData* m_data{};
        // The page under this one that the path goes on to. Told by the bar rather than worked
        // out here: the bar lays the path out and knows which sub-item of a page it went through.
        const PageData* m_pathSubItem{};
    };

    // BreadCrumbBar

    /// @brief The path down to the page a tab is showing, one crumb per level.
    export class BreadCrumbBar : public StackPanel
    {
        friend BreadCrumbBarItem;
    public:
        explicit BreadCrumbBar(const CreateParams&);
    public:
        // A crumb was chosen; the browser is being asked to go to that page. See Browser
        DECLARE_EVENT(SelectBrowserDataEvent, OnSelectBrowserData, onSelectBrowserData)
        // A crumb is about to drop its list, and asks what pages lie under this one. See Browser
        DECLARE_EVENT(FetchSubItemsEvent, OnFetchSubItems, onFetchSubItems)
        // What a page's icon is. See Browser
        DECLARE_EVENT(GetPageIconSizeEvent, OnGetPageIconSize, onGetPageIconSize)
        // Paints a page's icon - in a crumb's own slot, or beside a line of a crumb's list.
        DECLARE_EVENT(PaintPageIconEvent, OnPaintPageIcon, onPaintPageIcon)
        // Whether this page's name is the user's to change. See Browser
        DECLARE_EVENT(CanRenamePageEvent, OnCanRenamePage, onCanRenamePage)
        // The name typed over a crumb, on its way to whatever the page stands for. See Browser
        DECLARE_EVENT(RenamePageEvent, OnRenamePage, onRenamePage)
    public:
        std::wstring_view diagnosticText() const override { return L"BreadCrumbBar"; }
        /// Lays the bar out as the path down to this page: the root first, the page itself last.
        /// A null page empties the bar. Crumbs already standing are reused, so the bar keeps the
        /// controls a path shares with the one before it.
        void createItems(PageData*);
    private:
        void selectBrowserData(PageData&, Control& initiator);
        void fetchSubItems(PageData&);
        [[nodiscard]] IconSize pageIconSize(PageData&);
        void paintPageIcon(PageData&, PaintIconEvent&);
        // Const because a crumb asks it while answering editorMode(), which is const. Raising an
        // event is a const operation - the answer is the handler's, and none of it is the bar's
        // own state.
        [[nodiscard]] bool canRenamePage(PageData&) const;
        void renamePage(PageData&, AcceptEditEvent&);
    };


    //-----------------------------------------------------------------------------


    // BreadCrumbBarItem

    BreadCrumbBarItem::BreadCrumbBarItem(const CreateParams& params)
        :
        BreadCrumbBarItemBase{
            params,
            // The base sits on ButtonBase so that a combobox can share it without being handed a
            // button's metrics, so a crumb asks for them here.
            Interactivity::Focusable,
            // THE FILLED SURFACE IS WHAT SAYS WHICH PAGE THE BROWSER IS SHOWING, and that is
            // the crumb's own answer to getControlState - so the selected state has to reach the
            // surface. A button keeps it off by default, which is right where an indicator or
            // a dot says the same thing, and a crumb has neither.
            ShowSelectionOnSurface::Yes,
            params.themeMetrics().button,
            Padding{ 4.0f },
            // A crumb is as narrow as its own title, so a path longer than the bar narrows its
            // crumbs instead of pushing the last of them off the end. Only the width is freed:
            // the height stays the one every button on the bar stands at.
            MinSize{ 0.0f, params.themeMetrics().button.minSize.y }
        }
    {
        // THE PAGE IS READ AS THE SINK RUNS, and that is sound because a crumb is given a
        // different page only by a rebuild of the bar, which every ending of an editor happens
        // before: an ending the user drove offers the text from inside the press, and the press
        // is what goes on to move the browser.
        onAcceptEdit([this](AcceptEditEvent& event){
            if (!m_data)
                return;
            parentAs<BreadCrumbBar>().renamePage(*m_data, event);
        });
    }

    void BreadCrumbBarItem::setData(PageData* value, const PageData* pathSubItem)
    {
        m_data = value;
        m_pathSubItem = pathSubItem;
        updateIconMode();
        updateDropdownMode();
        invalidateState();
    }

    void BreadCrumbBarItem::getMainText(GetTextEvent& event) const
    {
        // Where the mark goes, and whether it goes here or on the strip, is the base's business.
        if (m_data)
            m_data->paintText(event.text);
    }

    void BreadCrumbBarItem::getEditorText(Text& text) const
    {
        // The page's own title, which is the run of glyphs the editor is placed over. A crumb
        // keeps no text of its own - it writes its caption out of the page every time it is
        // asked to draw - so the base would open the editor empty.
        if (m_data)
            m_data->paintText(text);
    }

    EditorMode BreadCrumbBarItem::editorMode() const
    {
        // ONLY THE CRUMB NAMING THE PAGE THE BROWSER IS SHOWING. The bar is an address, and the
        // page at the end of it is the one the user is in; the crumbs before it name pages that
        // are only on the way there, and a name is changed where the thing it names is open.
        //
        // ASKED AS THE GESTURE IS ANSWERED, never kept. What a browser will let be renamed moves
        // under a bar that is standing still - a file appears, goes, or is renamed - and nothing
        // rebuilds the crumbs to tell them so. An answer settled when the crumb was given its page
        // would be the answer to a question about a state that has since changed.
        if (!isCurrentPage() || !m_data || !parentAs<BreadCrumbBar>().canRenamePage(*m_data))
            return EditorMode::None;
        return BreadCrumbBarItemBase::editorMode();
    }

    FloatPoint BreadCrumbBarItem::editorMaxTextSize(const FloatRect&) const
    {
        // A CRUMB IS ONE LINE AS WIDE AS ITS OWN TITLE, and a path longer than the bar narrows it
        // further, so a name being typed outgrows it almost at once. No ceiling on either axis:
        // the editor grows to the right over the crumbs after this one, and the monitor is the
        // only bound left - which is what lets a name be read whole while it is typed. The crumb's
        // own rect is still the floor, so what it was showing stays covered.
        return {};
    }

    bool BreadCrumbBarItem::clickOpensEditor() const
    {
        // THE FIRST PRESS ON THE CRUMB'S FACE OPENS THE EDITOR. The base's rule keeps a press that
        // PICKS a control from also editing it, and there is nothing to pick here: the only crumb
        // with an editor is the one naming the page already on screen, so that press has no other
        // meaning. A press on the strip never reaches this - the base stops the click there and
        // drops the list instead.
        //
        // The pointer test goes with the rule. A crumb's face IS its title: the icon slot is the
        // root crumb's alone, and editorMode is what settles which crumb this applies to.
        return true;
    }

    void BreadCrumbBarItem::paintIcon(PaintIconEvent& event)
    {
        BreadCrumbBarItemBase::paintIcon(event);
        if (m_data)
            parentAs<BreadCrumbBar>().paintPageIcon(*m_data, event);
    }

    void BreadCrumbBarItem::showDropdown(Control& initiator)
    {
        if (!m_data)
            return;

        BreadCrumbBar& bar = parentAs<BreadCrumbBar>();
        bar.fetchSubItems(*m_data);
        // The fetch is what settles the question the strip was standing on, so the strip is put
        // right by what it left behind.
        updateDropdownMode();
        // Nothing under this page, so there is no list to drop. Answered before the menu is built
        // rather than by letting an empty one decline to open: a form holds a window from the
        // moment it is constructed, and building one to find out there is nothing to put in it is
        // work for nothing.
        if (m_data->items.empty())
            return;

        Menu menu{ initiator };
        for (const PageDataPtr& subItem : m_data->items)
        {
            PageData* target = subItem.get();
            MenuItem& line = menu.add(
                target->displayTitle(),
                [&bar, target](PaintIconEvent& event){
                    bar.paintPageIcon(*target, event);
                },
                // GONE TO FROM INSIDE THE MENU, and owned by the line that was pressed. A browser
                // with a question to put before it moves - work that is not saved - stands that
                // question on top of this menu rather than in place of it, so what is being left
                // is still on screen while it is answered.
                //
                // THIS CRUMB IS STILL STANDING AFTERWARDS: every page in the list is under the
                // page it names, so the path the bar rebuilds reaches at least as far as this
                // crumb, and createItems drops only the crumbs past the end of the new path. That
                // is what lets the going happen here rather than after the menu has closed.
                [&bar, target](Control& item){
                    bar.selectBrowserData(*target, item);
                }
            );
            if (target != m_pathSubItem)
                continue;

            // THE PAGE THE USER IS ALREADY INSIDE. It is one of the pages under this crumb, so it
            // is one of the lines of the list, and it is marked rather than left out - a list
            // missing the line the path went through reads as a list of somewhere else.
            //
            // Disabled with it, because there is nowhere for a press on it to go. A menu closes on
            // the command it ran, so a line that answered a press by doing nothing would still
            // take the list away.
            line.setShowSelectionOnSurface(ShowSelectionOnSurface::Yes);
            line.onGetState([](GetStateEvent& event){
                event.state.selected = true;
                event.state.enabled = false;
            });
        }
        // Anchored to the whole crumb, which is what the user reads as the thing being dropped,
        // even when the strip of it is what owns the popup.
        menu.executeUnder(*this);
    }

    void BreadCrumbBarItem::adjustPaint(AdjustPaintEvent& event)
    {
        BreadCrumbBarItemBase::adjustPaint(event);
        // The button's colours. A crumb has no surface at rest - its fill arrives with the
        // pointer the way a tool button's does - and that is the split button's default, so it is
        // not stated twice.
        event.setColorRules(UiElement::Button);
    }

    void BreadCrumbBarItem::getControlState(GetStateEvent& event) const
    {
        BreadCrumbBarItemBase::getControlState(event);
        if (&event.control != this)
            return;

        // A crumb answers for its own selected look, and the walk ends here. On this bar the
        // filled surface means the page the browser is showing, which is the last crumb of the
        // path. Left to the walk it would be the bar's current item - whichever crumb was last
        // reached - and where the keyboard is standing is what the focus ring already says.
        event.state.selected = isCurrentPage();
        event.stopPropagation();
    }

    void BreadCrumbBarItem::click(ClickEvent& event)
    {
        // WHICH HALF WAS PRESSED IS READ FIRST, and that is load bearing: the base runs the
        // dropdown from here, and a fetch that names nothing takes the strip away - after which
        // the press lands on no part at all and would read as a press on the crumb's own face.
        const bool pressedStrip = pressedSecondary(event);
        BreadCrumbBarItemBase::click(event);
        // A press on the strip is the base's: it stops the click there and runs the dropdown. What
        // is left is a press on the crumb's own face, which is the page the crumb names.
        if (pressedStrip || !m_data)
            return;

        // THE LAST CRUMB NAMES THE PAGE THE BROWSER IS ALREADY SHOWING, so there is nowhere for a
        // press on it to go. What that press means instead is an edit of the title, which the base
        // call above has already answered - see clickOpensEditor.
        if (isCurrentPage())
            return;

        parentAs<BreadCrumbBar>().selectBrowserData(*m_data, *this);
    }

    void BreadCrumbBarItem::updateIconMode()
    {
        // THE ROOT CRUMB IS THE ONLY ONE THAT CARRIES A PICTURE. It stands for the whole tree
        // rather than for a step through it, and a row of icons along a line of names reads as a
        // toolbar instead of as one address. The root is the crumb whose page has no parent,
        // which is where the bar starts every path it lays out.
        //
        // A crumb's list is the other way round - there a picture is what tells one line from the
        // next - so showDropdown asks per page.
        const IconSize size = m_data && !m_data->parent
            ? parentAs<BreadCrumbBar>().pageIconSize(*m_data)
            : IconSize{ 0.0f };
        const bool hasIcon = size.x > 0.0f && size.y > 0.0f;
        // A crumb with nothing to show keeps its title against its own left edge, rather than
        // holding a slot open beside it.
        setViewMode(hasIcon ? ButtonViewMode::LeftIcon : ButtonViewMode::TextLabel);
        if (hasIcon)
            setIconSize(size);
    }

    void BreadCrumbBarItem::updateDropdownMode()
    {
        // The strip is what says there is a list under this crumb, so a page with nothing under it
        // has none - and the mark, the two keys and the room all go with it, being the strip's.
        // Every crumb has a strip to show or hide: the constructor asks for one and never for
        // ArrowPlacement::InText.
        secondaryPart()->setVisible(m_data && m_data->hasSubItems());
    }

    // BreadCrumbBar

    BreadCrumbBar::BreadCrumbBar(const CreateParams& params)
        :
        StackPanel{ params, Interactivity::ActiveContainer }
    {
        const AppTheme& theme = params.theme();
        setMetrics(theme.metrics.page);
        setPadding(8.0f, 2.0f);
        setColorRules(UiElement::Page);
        setOrientation(Orientation::Horizontal);
    }

    void BreadCrumbBar::createItems(PageData* pathItem)
    {
        const std::size_t elemCount = pathItem ? pathItem->level() + 1ull : 0ull;

        reserve(std::max(controls().size(), elemCount));

        while (controls().size() > elemCount)
            controls().back()->deleteSelf();

        while (controls().size() < elemCount)
            add<BreadCrumbBarItem>();

        // Filled from the last crumb backwards, because a page names its parent and not its
        // children. The first one the walk reaches is the end of the path, which is the page the
        // browser is showing, and each crumb after it is handed the page the path goes on to -
        // which is the page the crumb before it in the walk was given.
        const PageData* pathSubItem = nullptr;
        for (ControlSpan::reverse_iterator it = controls().rbegin(); it != controls().rend(); ++it)
        {
            BreadCrumbBarItem& item = static_cast<BreadCrumbBarItem&>(**it);
            item.setData(pathItem, pathSubItem);
            pathSubItem = pathItem;
            pathItem = pathItem->parent;
        }
    }

    void BreadCrumbBar::selectBrowserData(PageData& value, Control& initiator)
    {
        SelectBrowserDataEvent event{ value, initiator };
        emitEvent(event);
    }

    void BreadCrumbBar::fetchSubItems(PageData& value)
    {
        FetchSubItemsEvent event{ value };
        emitEvent(event);
    }

    IconSize BreadCrumbBar::pageIconSize(PageData& value)
    {
        GetPageIconSizeEvent event{ value };
        emitEvent(event);
        return event.size;
    }

    void BreadCrumbBar::paintPageIcon(PageData& value, PaintIconEvent& iconEvent)
    {
        PaintPageIconEvent event{ iconEvent, value };
        emitEvent(event);
    }

    bool BreadCrumbBar::canRenamePage(PageData& value) const
    {
        CanRenamePageEvent event{ value };
        emitEvent(event);
        return event.canRename;
    }

    void BreadCrumbBar::renamePage(PageData& value, AcceptEditEvent& acceptEvent)
    {
        RenamePageEvent event{ value, acceptEvent };
        emitEvent(event);
    }
}
