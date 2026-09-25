export module ClaFi.Controls.Menu;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Divider;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
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

        // How far a line may run before it turns. This one is NOT a share of anything: it is how
        // far the eye carries along a line, the same distance on a small screen and on a large
        // one - the message box states its own reading width for the same reason.
        constexpr float k_maxItemWidth = 360.0f;

        // The least room left between an item's name and the key printed at the end of its line.
        constexpr float k_keyGap = 4.0f;
    }

    /// @brief One command in a menu.
    /// @note A button, and deliberately: the icon, the text, the enabled and selected states and
    /// the press all belong to a button already. What a menu adds is the key printed at the
    /// right-hand end of the line, and closing behind the command it ran.
    // Button for its colours, its metrics and its z animation - UiElement::Button,
    // themeMetrics().button, and the press depth that allowZAnimation() turns on - and the item
    // states nothing of its own about any of them. NOT ToolButton: a line of a menu is not a tool,
    // and all it wanted from that class was the one property it now states itself.
    using MenuItemBase = Button;
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
        void click(ClickEvent&) override;
    };

    /// @brief One command in the strip across the top of a menu.
    /// @note A large icon with the command's name under it. A command belongs up here when its
    /// picture says what it does at a glance, which is what buys the strip its width: four of
    /// them stand side by side in what the list would have spent on two lines.
    /// @note There is no room beside the name for the key, so the tooltip states both.
    // The same base as an item, for the same reasons.
    using MenuCommandBase = Button;
    // A menu line that runs an action, showing its name and its key.
    export class MenuCommand : public MenuCommandBase
    {
    public:
        template<typename ...Args>
        explicit MenuCommand(const CreateParams&, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"MenuCommand"; }
    protected:
        void getTooltip(GetTooltipEvent&) override;
        void click(ClickEvent&) override;
    };

    export using BaseMenuForm = Form<ScrollBoxWith<StackPanel>>;

    /// @brief What a text item does when it is chosen.
    /// @note It is handed the ITEM that was pressed. A handler raising a question of its own owns
    /// it by that: the question then stands on top of this menu, which stays on screen behind it,
    /// rather than replacing the thing the user was reading.
    export using MenuItemCallback = std::function<void(Control& item)>;

    /// @brief What a text item paints in its icon slot.
    export using MenuItemIcon = std::function<void(PaintIconEvent&)>;

    // A popup list of commands, owned by the control it was opened from. See Controls
    export class Menu : public BaseMenuForm
    {
    public:
        explicit Menu(Control& owner);
    public:
        /// Takes an action as an item. The item shows the action's text and icon, prints its
        /// shortcut, and closes the menu when it is chosen.
        MenuItem& add(Action&);
        /// Takes a list of them. A null entry is a separator, and one that would lead, trail or
        /// double up is dropped - so a list an application has adjusted needs no tidying. An
        /// action the strip already shows is passed over, so a list naming every command a
        /// control answers can be handed over whole whatever went up into the strip.
        void add(const ActionList&);
        /// Takes a line of text as an item, with what to do when it is chosen. This is for a
        /// list built out of data - the pages under a breadcrumb, a set of recent files - where
        /// each line stands for something that exists only while the menu is up. A command an
        /// application answers from more than one place is an Action, and goes through the
        /// overload above.
        MenuItem& add(std::wstring_view text, MenuItemCallback callback);
        /// The same, with a picture beside the line. Every item reserves the icon slot whether or
        /// not it fills one, so items with and without a picture still share a left edge.
        MenuItem& add(std::wstring_view text, MenuItemIcon icon, MenuItemCallback callback);
        void addSeparator();
        /// Takes an action as a button in the strip across the top: its icon, no caption, named
        /// by its tooltip. The strip appears with the first command put in it and reads left to
        /// right in the order they arrive.
        MenuCommand& addToCommandBar(Action&);
        /// Whether the strip already shows this command.
        [[nodiscard]] bool onCommandBar(const Action&) const;
        [[nodiscard]] bool empty() const { return !m_itemCount && m_commandBarActions.empty(); }
        /// Drops the menu at the pointer, or under the part of its owner the commands are about
        /// when the keyboard raised it, and runs it. Answers once it has closed. A menu with
        /// nothing in it does not open.
        int execute();
        /// Drops the menu under a control the way a dropdown list falls, and runs it. This is for
        /// a menu that belongs to a control's outline rather than to the point that was clicked.
        /// Answers once it has closed, and a menu with nothing in it does not open.
        int executeUnder(const Control&);
    private:
        // One place for what every item shares: the separator waiting on it, and the count that
        // says the menu has something in it. What differs between an action item and a text item
        // is only what the button is built from.
        template<typename... Args>
        MenuItem& addItem(Args&&...);
    private:
        // The strip is built when something is put in it. A menu with no commands in the strip
        // has no top bar at all, and measures exactly as it did before there was one.
        StackPanel* m_commandBar{ nullptr };
        // Held by address, which is how an action is recognised everywhere else.
        std::vector<const Action*> m_commandBarActions;
        std::size_t m_itemCount{ 0 };
        // A separator waits for an item to follow it, so one asked for at the end never lands.
        bool m_separatorPending{ false };
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
            // Takes the pointer, never the ACTIVATION - which leaves the form that opened it
            // captioned and active behind it. The FOCUS does move here, so that the arrows read
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
    }

    MenuItem& Menu::add(Action& action)
    {
        return addItem(action);
    }

    MenuItem& Menu::add(const std::wstring_view text, MenuItemCallback callback)
    {
        return addItem(text, OnEvent{ [callback = std::move(callback)](ClickEvent& event){
            callback(*event.control);
        } });
    }

    MenuItem& Menu::add(const std::wstring_view text, MenuItemIcon icon, MenuItemCallback callback)
    {
        return addItem(
            text,
            MenuItem::OnPaintIcon{ [icon = std::move(icon)](PaintIconEvent& event){
                icon(event);
            } },
            OnEvent{ [callback = std::move(callback)](ClickEvent& event){
                callback(*event.control);
            } }
        );
    }

    void Menu::add(const ActionList& actions)
    {
        for (Action* action : actions)
        {
            if (!action)
                addSeparator();
            else if (!onCommandBar(*action))
                add(*action);
        }
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

    int Menu::execute()
    {
        if (empty())
            return 0;
        // The pointer decides where a menu the MOUSE raised lands, and the rect below is not
        // read at all - see FormBase::initPlacement. This is the other case: raised from the
        // keyboard there is no pointer, and the menu drops under the part of its owner the
        // commands are about, which for a text box is the caret rather than the whole box.
        // The owner is tested rather than assumed: the constructor takes one, but FormBase
        // accepts a popup with no target at all, and a menu built over a control that went away
        // while it was being filled is that.
        if (Control* owner = popupTarget())
            setPlacement(FormPlacement::ContextMenu, owner->contextMenuAnchor());
        else
            setPlacement(FormPlacement::ContextMenu);
        return BaseMenuForm::execute();
    }

    int Menu::executeUnder(const Control& control)
    {
        if (empty())
            return 0;
        // The gap a dropped list leaves between itself and what it fell from.
        setDropdownClearance(1.0f);
        setPlacement(FormPlacement::Bottom, control.boundsInForm());
        // And at least as wide as it - a list narrower than the face it fell from reads as
        // belonging to something else. In design units, which a popup shares with its parent.
        setMinWidth(control.width() / scaler().factor());
        return BaseMenuForm::execute();
    }

    template<typename ...Args>
    MenuItem& Menu::addItem(Args&&... args)
    {
        if (m_separatorPending)
        {
            body().add<Controls::Divider>(Padding{ 0.0f, 4.0f });
            m_separatorPending = false;
        }
        ++m_itemCount;
        return body().add<MenuItem>(std::forward<Args>(args)...);
    }

    //-----------------------------------------------------------------------------


    // MenuItem

    template<typename ...Args>
    MenuItem::MenuItem(const CreateParams& params, Args&& ...args)
        :
        MenuItemBase{
            params,
            Interactivity::Focusable,
            // Not a tool, so not a ToolButton - but it wears the same look, and the property is
            // the whole of that look.
            ShowSurfaceAtRest::No,
            // The icon slot is kept whether or not this item has one, which is what puts every
            // item's text on the same left edge.
            ButtonViewMode::LeftIcon,
            HorizontalTextAnchor::Left,
            // Every item is as wide as the menu, and that is what the key at the end of the line
            // is measured against.
            HorizontalAlign::Fill,
            // How wide the line may be. Stated by the ITEM rather than by the menu around it,
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
        if (opensWindow() == OpensWindow::No)
            event.text << FlexSpace{ k_keyGap };
        else
            event.text << Space{ k_keyGap };

        event.text << InkWell::textInk(InkGrade::Muted) << shortcutText;
    }

    void MenuItem::click(ClickEvent& event)
    {
        MenuItemBase::click(event);
        closeBehindCommand(event);
    }



    //-----------------------------------------------------------------------------


    // MenuCommand

    template<typename ...Args>
    MenuCommand::MenuCommand(const CreateParams& params, Args&& ...args)
        :
        MenuCommandBase{
            params,
            Interactivity::Focusable,
            // Not a tool, so not a ToolButton - but it wears the same look, and the property is
            // the whole of that look.
            ShowSurfaceAtRest::No,
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
            std::forward<Args>(args)...
        }
    {
    }

    void MenuCommand::getTooltip(GetTooltipEvent& event)
    {
        MenuCommandBase::getTooltip(event);
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

    void MenuCommand::click(ClickEvent& event)
    {
        MenuCommandBase::click(event);
        closeBehindCommand(event);
    }

}
