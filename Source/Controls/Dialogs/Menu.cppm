export module ClaFi.Controls.Menu;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Stack;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Divider;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Controls.Button;
import ClaFi.Core.Context.PaintIconEvent;

namespace ClaFi::Controls
{
    namespace
    {
        // How much of the screen a menu may take, top to bottom. Past this it scrolls. A share
        // rather than a number, because the same list is a short menu on one screen and the whole
        // of another.
        // TODO: nothing reads this. The screen is not known until the menu is placed, so where
        // does the share become a maximum - FormBase::monitorHeightShare, asked by the placement?
        constexpr float k_monitorHeightShare = 0.8f;

        // How far a line may run before it turns. This one is **not** a share of anything: it is
        // how far the eye carries along a line, the same distance on a small screen and on a large
        // one - the message box states its own reading width for the same reason.
        constexpr float k_maxItemWidth = 360.0f;

        // The least room left between an item's name and the key printed at the end of its line.
        constexpr float k_keyGap = 4.0f;
    }

    export class MenuItemBase: public Button
    {
    public:
        template<typename ...Args>
        explicit MenuItemBase(const CreateParams&, Args&&...);
    protected:
        void click(ClickEvent&) override;
    };

    // One line of a menu.
    export class MenuItem : public MenuItemBase
    {
    public:
        template<typename ...Args>
        explicit MenuItem(const CreateParams&, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"MenuItem"; }
    protected:
        void getText(GetTextEvent&) const override;
    };

    // A menu item in TopIcon mode on the top bar.
    export class MenuCommand : public MenuItemBase
    {
    public:
        template<typename ...Args>
        explicit MenuCommand(const CreateParams&, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"MenuCommand"; }
    protected:
        void nestedGetTooltip(GetTooltipEvent&) override;
    };

    export using BaseMenuForm = Form<ScrollBoxWith<StackPanel>>;

    export class Menu;

    // A line for a command that holds commands: a press opens them beside it, as a list of their
    // own, and the menu stays up. See Controls
    export class MenuGroupItem : public MenuItem
    {
    public:
        template<typename... Args>
        MenuGroupItem(const CreateParams&, Menu&, Action&, Args&&...);
    public:
        [[nodiscard]] Action& action() const { return m_action; }
        std::wstring_view diagnosticText() const override { return L"MenuGroupItem"; }
    protected:
        void click(ClickEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;   // Right opens the list
    private:
        Menu& m_menu;
        Action& m_action;
    };

    // A popup list of commands, owned by the control it was opened from. See Controls
    export class Menu : public BaseMenuForm
    {
        friend class MenuGroupItem;
    public:
        explicit Menu(Control& owner);
    public:
        // Every command as a line, a null as a separator, and one holding commands as a line
        // with its list beside it.
        void addActions(const ActionList&);
        void addSeparator();
        MenuCommand& addToCommandBar(Action&);
        /// Whether the strip already shows this command.
        [[nodiscard]] bool onCommandBar(const Action&) const;
        [[nodiscard]] bool empty() const { return !m_itemCount && m_commandBarActions.empty(); }
        void setPlaceHolderText(const Text&);
        int execute();
        /// Drops the menu under a control the way a dropdown list falls, and runs it. This is for
        /// a menu that belongs to a control's outline rather than to the point that was clicked.
        /// BottomLeft hangs it from the control's left end, BottomRight from its right end.
        /// Answers once it has closed, and a menu with nothing to show does not open.
        int executeUnder(const Control&, FormPlacement placement = FormPlacement::BottomLeft);
        /// Stands the menu beside a line of another menu, as the list of the command the line
        /// holds, and runs it. Left closes it; a command run in it closes the menu it stands beside.
        int executeBeside(const Control& line);
    protected:
        void nestedClick(NestedClickEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;
    private:
        // Nothing in it, and no placeholder to stand in for the nothing.
        [[nodiscard]] bool nothingToShow() const { return empty() && body().placeholderText().empty(); }
    public:
        // A line of the caller's own, built from what it is given, that closes the menu behind
        // its click.
        template<typename... Args>
        MenuItem& add(Args&&...);
        // A line that opens the command's list beside it. See Controls
        MenuGroupItem& addGroup(Action&);
    private:
        // One place for what every line shares: the separator waiting on it, the count that says
        // the menu has something in it, and the rest the pointer is timed on.
        template<typename Item, typename... Args>
        Item& place(Args&&...);
        // Opens the group's list beside it and runs it, then the list of whichever other group
        // the pointer settled on while that one was up, until none did. A command run in any of
        // them closes this menu too.
        void openGroup(MenuGroupItem&);
        // The pointer has rested on a line: a group's list opens, and another line's closes the
        // list up beside the group it left.
        void hoverSettled();
        [[nodiscard]] MenuGroupItem* groupOf(const Control&) const;
    private:
        // How long the pointer rests on a line before the list under it opens, or the list up
        // beside another line closes: long enough to cross a line on the way somewhere else. A
        // member, since place is a template the importers instantiate.
        static constexpr MilliSeconds k_listDelay{ 300u };

        // The strip is built when something is put in it. A menu with no commands in the strip
        // has no top bar at all, and measures exactly as it did before there was one.
        StackPanel* m_commandBar{ nullptr };
        // Held by address, which is how an action is recognised everywhere else.
        std::vector<const Action*> m_commandBarActions;
        std::vector<MenuGroupItem*> m_groups;
        std::size_t m_itemCount{ 0 };
        // A separator waits for an item to follow it, so one asked for at the end never lands.
        bool m_separatorPending{ false };
        bool m_isList{ false };       // stands beside a line of another menu
        bool m_commandRan{ false };   // a command was run from a line, or from a list under one
        MenuGroupItem* m_pendingGroup{ nullptr };   // whose list opens once the one up has closed
        UiTimer m_hoverTimer{};
    };


    //-----------------------------------------------------------------------------


    namespace
    {
        // The menu closes behind whichever of its controls ran the command. closeForm stops the
        // event, so closing ahead of the command would swallow the handler the control is there
        // for.
        void closeBehindCommand(ClickEvent& event)
        {
            if (!event.propagationStopped())
                event.closeForm();
        }
    }


    //-----------------------------------------------------------------------------


    // Menu

    Menu::Menu(Control& owner)
        :
        BaseMenuForm{
            // Takes the pointer, never the **activation** - which leaves the form that opened it
            // captioned and active behind it. The **focus** does move here, so that the arrows read
            // the items the way they read any other list. What the menu is about stays behind:
            // an item's action finds its subject through FormBase::popupTarget, and the owner
            // keeps its caret and its selected look through Control::isDroppedDown.
            owner.appContext(), WindowRole::Menu, &owner,
            HostProps{
                owner.themeMetrics().secondaryWindow,
                owner.themeMetrics().secondaryWindowShadow,
                UiElement::Menu,
                // A list longer than the screen has to be reachable, and a menu is the one
                // window that cannot be resized to reach it - so it scrolls. The bar is there
                // only when there is something under the edge to reach: Auto answers against the
                // ceiling that pass has, which is the window once there is one - and the window
                // is short of the list only when neither side of the owner held it. A menu of
                // four commands shows no bar at all.
                ScrollBars::Auto,
                Padding{ 4.0f },
                // Clicked and hovered, never focused. The items carry the focus themselves and
                // the frame around them is not a focus scope of its own.
                // TODO: Home, End and the page keys address the nearest ActiveContainer in
                // scope, and a menu has none, so those four do nothing in one. Should this root
                // be an ActiveContainer so they reach the items?
                Interactivity::MouseOnly,
            },
            BodyProps{
                Orientation::Vertical,
            }
        }
    {
        // The window is whatever the items came out as. A menu is never laid out into a size it
        // was given: it has no size until it has been filled.
        setAutoFit(AutoFit::Yes);
        m_hoverTimer.onTick([this](TimerEvent&) {
            hoverSettled();
        });
    }

    void Menu::addActions(const ActionList& actions)
    {
        for (Action* action : actions)
        {
            if (!action)
                addSeparator();
            else if (action->opensList())
                addGroup(*action);
            else if (!onCommandBar(*action))
                add(*action);
        }
    }

    MenuGroupItem& Menu::addGroup(Action& action)
    {
        MenuGroupItem& item = place<MenuGroupItem>(*this, action);
        m_groups.push_back(&item);
        return item;
    }

    void Menu::addSeparator()
    {
        // Held rather than added. A separator is a rule between two groups of commands, so one
        // with nothing before it, nothing after it, or another separator beside it is not one.
        // The strip counts as what is before: it stands above the list, so the rule under it is
        // a rule between two groups exactly as one inside the list is.
        m_separatorPending = m_itemCount != 0 || !m_commandBarActions.empty();
    }

    MenuCommand& Menu::addToCommandBar(Action& action)
    {
        if (!m_commandBar)
            m_commandBar = &createTopBar<StackPanel>(Orientation::Horizontal);
        m_commandBarActions.push_back(&action);
        // The rule under the strip. Held like any other, so it lands only once the list has
        // something under it and never on a menu that is strip alone.
        addSeparator();
        return m_commandBar->add<MenuCommand>(action);
    }

    bool Menu::onCommandBar(const Action& action) const
    {
        return std::ranges::find(m_commandBarActions, &action) != m_commandBarActions.end();
    }

    void Menu::setPlaceHolderText(const Text& value)
    {
        body().setPlaceHolderText(value);
    }

    int Menu::execute()
    {
        if (nothingToShow())
            return 0;
        // The pointer decides where a menu the **mouse** raised lands, and the rect below is not
        // read at all - see FormBase::initPlacement. This is the other case: raised from the
        // keyboard there is no pointer, and the menu drops under the part of its owner the
        // commands are about, which for a text box is the caret rather than the whole box.
        // The owner is tested rather than assumed: the constructor takes one, but FormBase
        // accepts a popup with no target at all, and a menu built over a control that went away
        // while it was being filled is that.
        if (const Control* owner = popupTarget())
            setPlacement(FormPlacement::ContextMenu, owner->contextMenuAnchor());
        else
            setPlacement(FormPlacement::ContextMenu);
        return BaseMenuForm::execute();
    }

    int Menu::executeUnder(const Control& control, const FormPlacement placement)
    {
        if (nothingToShow())
            return 0;
        // The gap a dropped list leaves between itself and what it fell from.
        setDropdownClearance(1.0f);
        setPlacement(placement, control.boundsInForm());
        // And at least as wide as it - a list narrower than the face it fell from reads as
        // belonging to something else. In design units, which a popup shares with its parent.
        setMinWidth(control.scaledWidth() / scaler().factor());
        return BaseMenuForm::execute();
    }

    int Menu::executeBeside(const Control& line)
    {
        if (nothingToShow())
            return 0;
        m_isList = true;
        setPlacement(FormPlacement::Right, line.boundsInForm());
        return BaseMenuForm::execute();
    }

    void Menu::nestedClick(NestedClickEvent& nestedClickEvent)
    {
        Form<ScrollBoxWith<StackPanel>>::nestedClick(nestedClickEvent);
        Platform::beep();
    }

    void Menu::nestedKeyDown(KeyDownEvent& event)
    {
        if (m_isList && event.key == Keys::Left && event.modifiers.empty())
        {
            close();
            event.handled = true;
            return;
        }
        BaseMenuForm::nestedKeyDown(event);
    }

    void Menu::openGroup(MenuGroupItem& group)
    {
        if (activePopup() && activePopup()->popupTarget() == &group)
            return;
        closeActivePopup();
        if (activePopup())
            return;
        for (MenuGroupItem* next = &group; next;)
        {
            m_pendingGroup = nullptr;
            Menu list{ *next };
            list.addActions(next->action().items());
            list.executeBeside(*next);
            if (list.m_commandRan)
            {
                m_commandRan = true;
                close();
                return;
            }
            next = m_pendingGroup;
        }
    }

    void Menu::hoverSettled()
    {
        Control* hovered = Input::hoveredControl();
        if (!hovered || &hovered->form() != this)
            return;
        MenuGroupItem* group = groupOf(*hovered);
        if (const FormBase* list = activePopup())
        {
            if (list->popupTarget() == group)
                return;
            // Closed here and opened by openGroup, whose loop this close returns into.
            m_pendingGroup = group;
            closeActivePopup();
            return;
        }
        if (group)
            openGroup(*group);
    }

    MenuGroupItem* Menu::groupOf(const Control& control) const
    {
        for (MenuGroupItem* group : m_groups)
        {
            if (group == &control)
                return group;
        }
        return nullptr;
    }

    template<typename ...Args>
    MenuItem& Menu::add(Args&&... args)
    {
        MenuItem& item = place<MenuItem>(std::forward<Args>(args)...);
        // Read once the handlers have run: a click they stopped keeps the menu up, and runs no
        // command - see closeBehindCommand.
        item.onClick([this](ClickEvent& event) {
            if (!event.propagationStopped())
                m_commandRan = true;
        });
        return item;
    }

    template<typename Item, typename... Args>
    Item& Menu::place(Args&&... args)
    {
        if (m_separatorPending)
        {
            body().add<Controls::Divider>(Padding{ 0.0f, 4.0f });
            m_separatorPending = false;
        }
        ++m_itemCount;
        Item& item = body().add<Item>(std::forward<Args>(args)...);
        item.onHoverEnter([this](HoverEnterEvent&) {
            m_hoverTimer.start(k_listDelay);
        });
        return item;
    }

    //-----------------------------------------------------------------------------


    // MenuItem

    template <typename ... Args>
    MenuItemBase::MenuItemBase(const CreateParams& params, Args&&... args)
        :
        Button{
            params,
            Interactivity::Focusable,
            params.themeMetrics().toolButton,
            UiElement::ToolButton,
            std::forward<Args>(args)...
        }
    {
        
    }

    template<typename ...Args>
    MenuItem::MenuItem(const CreateParams& params, Args&& ...args)
        :
        MenuItemBase{
            params,
            // The icon slot is kept whether or not this item has one, which is what puts every
            // item's text on the same left edge.
            ButtonViewMode::LeftIcon,
            HorizontalTextAnchor::Left,
            // Every item is as wide as the menu, and that is what the key at the end of the line
            // is measured against.
            HorizontalAlign::Fill,
            // How wide the line may be. Stated by the **item** rather than by the menu around it,
            // because an item aligned Fill keeps whatever width it measured when that is the
            // larger - see Control::align - so a maximum on the list would leave the long item
            // standing past the view with its key out of sight. Capped here, a long name turns
            // onto a second line inside its own item and the key stays on the menu's right edge.
            // TODO: ButtonBase measures its text against the whole maximum and adds the icon
            // slot to what comes back, so an item that reaches this width overruns it by the
            // icon and the space beside it. Should a button's text be given the width its own
            // icon leaves?
            MaxSize{ k_maxItemWidth, k_maxFloat },
            // An item writes the command's name and its key into its own line - see getText -
            // so an action attached here says nothing further in a tooltip.
            PresenterRole::Line,
            std::forward<Args>(args)...
        }
    {
    }

    void MenuItemBase::click(ClickEvent& clickEvent)
    {
        Button::click(clickEvent);
        closeBehindCommand(clickEvent);
    }

    void MenuItem::getText(GetTextEvent& event) const
    {
        MenuItemBase::getText(event);

        // The key goes at the right-hand end of the item's own line. A menu is a column of items
        // stretched to one width, so a flex space ahead of it lands every key on the same edge
        // without a column to align them in.
        GetShortcutEvent shortcutEvent{ *this };
        emitEvent(shortcutEvent);
        const std::wstring shortcutText = shortcutEvent.shortcut.text();
        if (shortcutText.empty())
            return;
        // Only one flex space may open the line: a second would split the slack and strand both
        // of the things on the right in the middle. The mark ButtonBase writes for a command that
        // opens a window has already taken it, and the key then stands at the very end.
        if (endIcon() == EndIcon::None)
            event.text << FlexSpace{ k_keyGap };
        else
            event.text << Space{ k_keyGap };

        event.text << InkWell::textInk(InkGrade::Muted) << shortcutText;
    }


    //-----------------------------------------------------------------------------


    // MenuGroupItem

    template<typename... Args>
    MenuGroupItem::MenuGroupItem(const CreateParams& params, Menu& menu, Action& action,
        Args&&... args)
        :
        MenuItem{
            params,
            EndIcon::OpensList,
            // Shown the command and left to open it: the action connects no click of its own.
            PresenterRole::Opener,
            action,
            std::forward<Args>(args)...
        },
        m_menu{ menu },
        m_action{ action }
    {
    }

    // The list opens, and the menu stays: nothing closes behind this press.
    void MenuGroupItem::click(ClickEvent& event)
    {
        Button::click(event);
        m_menu.openGroup(*this);
    }

    void MenuGroupItem::nestedKeyDown(KeyDownEvent& event)
    {
        if (event.key == Keys::Right && event.modifiers.empty())
        {
            m_menu.openGroup(*this);
            event.handled = true;
            return;
        }
        MenuItem::nestedKeyDown(event);
    }


    //-----------------------------------------------------------------------------


    // MenuCommand

    template<typename ...Args>
    MenuCommand::MenuCommand(const CreateParams& params, Args&& ...args)
        :
        MenuItemBase{
            params,
            // The icon over the name rather than beside it, which is what stands a row of
            // commands in the width one of them would take as a line.
            ButtonViewMode::TopCenterIcon,
            // The name sits under the middle of its own icon. TopCenterIcon centres the picture
            // and this centres the word: an item's Left anchor is for a column of lines sharing
            // one left edge, which a strip is not.
            HorizontalTextAnchor::Center,
            // Larger than the 16 an item's line carries: up here the picture is read first and
            // the word under it settles which command it is.
            IconSize{ 24.0f },
            // A floor rather than a fixed size, so a long name widens its own button while the
            // short ones still stand as targets the same size as the rest of the strip.
            MinSize{ 56.0f },
            // One line at its own width, which is what lets a long name widen the button.
            WordWrap::No,
            std::forward<Args>(args)...
        }
    {
    }

    void MenuCommand::nestedGetTooltip(GetTooltipEvent& event)
    {
        MenuItemBase::nestedGetTooltip(event);
        // An action says its own name and key - see Action::getTooltip - and this is what a
        // command with none says instead. The strip writes the name under each icon and has
        // nowhere to put the key beside it, so this is where the two are said together.
        if (!event.text.empty())
            return;

        GetTextEvent textEvent{ event.formContext(), *this, event.text, EventPhase::Paint };
        getText(textEvent);
        // The tooltip reads the buffer rather than the answer, so a named text is put in it.
        textEvent.materialise();
        if (event.text.empty())
            return;
        GetShortcutEvent shortcutEvent{ *this };
        emitEvent(shortcutEvent);
        const std::wstring shortcutText = shortcutEvent.shortcut.text();
        if (shortcutText.empty())
            return;
        event.text << L" (" << shortcutText << L")";
    }

}
