module;
#include "../System/EventBindings.h"

export module ClaFi.Core.Foundation :Action;

import :Control;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;

import ClaFi.StdLib;

namespace ClaFi
{
    export class Action;

    // A key and the modifiers held with it. See Control-Foundation
    export struct Shortcut
    {
        KeyCode key{};
        KeyModifiers modifiers{};
        [[nodiscard]] bool empty() const { return key == 0; }
        [[nodiscard]] bool matches(const KeyDownEvent&) const;
        // "Ctrl+S", for a menu's shortcut column and for the tooltip line. Empty for a key with
        // no name of its own, so that a caller never shows a modifier with nothing after it.
        [[nodiscard]] std::wstring text() const;
    };

    // What both action events carry. See Control-Foundation
    export class ActionEventBase : public Event
    {
    public:
        ActionEventBase(const Action&, Control*);
        const Action& action;
        // The control being asked, or null when the action is answering for itself.
        Control* control;
    };

    // Whether the action can be run, and by whom. See Control-Foundation
    export class GetActionStateEvent : public ActionEventBase
    {
    public:
        using ActionEventBase::ActionEventBase;
        // This handler answers for the action, and `value` is its answer. Claiming is a separate
        // statement from what the state says: a text box with nothing selected claims Copy and
        // reports it disabled, which is not the same as not being about Copy at all.
        //
        // The first control that claims is the subject, and it is the one the click goes to.
        void claim(ActionState value);
        [[nodiscard]] bool claimed() const { return m_claimed; }
        [[nodiscard]] ActionState state() const { return m_state; }
    private:
        ActionState m_state{};
        bool m_claimed{};
    };

    // Run the action. See Control-Foundation
    export class ActionClickEvent : public ActionEventBase
    {
    public:
        ActionClickEvent(const Action&, Control* subject, Control* presenter, FormBase&, InputStamp);
        FormBase& form;
        // WHAT SHOWED THE COMMAND that was run - the toolbar button, the menu item - or null for
        // a shortcut, which was shown by nothing. A handler raising a question of its own owns it
        // by this, so the question stands where the user pressed and on top of whatever the press
        // came from, rather than over a control the user is not looking at.
        Control* presenter;
        // WHAT THE USER DID TO RUN IT, as the display server states it. A handler making a
        // request the server has to authorise - taking the clipboard, raising a menu that takes
        // the pointer - names this. Empty where the command was not run by a person: an action
        // the application invoked for itself has no press and no key behind it.
        InputStamp stamp;
    };

    // What key runs a control's action, asked by whatever shows the key. See Control-Foundation
    export class GetShortcutEvent : public Event
    {
    public:
        GetShortcutEvent(const Control&);
        const Control& control;
        Shortcut shortcut{};
    };

    // A named command: what it says, what it draws, and what it does. See Control-Foundation
    export class Action : public EventComponent
    {
    public:
        Action() = default;
        template <typename... Args>
            requires (sizeof...(Args) > 0) && NotSelfCopy<Action, Args...>
        explicit Action(Args&&... args);
        ~Action();
        // Every connection an attachment holds captures this action's address, so it may not be
        // copied and may not move.
        Action(const Action&) = delete;
        Action(Action&&) = delete;
        Action& operator=(const Action&) = delete;
        Action& operator=(Action&&) = delete;
    public:
        DECLARE_EVENT(ActionClickEvent, OnClick, onClick) // run the action. See Control-Foundation
        // Whether the action can be run, and by whom. See Control-Foundation
        DECLARE_EVENT(GetActionStateEvent, OnGetState, onGetState)
        // An icon being painted, from a control's paint or from a text run. See Context
        DECLARE_EVENT(PaintIconEvent, OnPaintIcon, onPaintIcon)
    public:
        [[nodiscard]] Text& text() { return m_text; }
        [[nodiscard]] const Text& text() const { return m_text; }
        [[nodiscard]] TooltipText& tooltipText() { return m_tooltipText; }
        [[nodiscard]] const TooltipText& tooltipText() const { return m_tooltipText; }
        // Writes what this action says of itself into a tooltip, and does nothing to one that
        // already says something. Every presenter is given it; a caller showing the command
        // somewhere the action is not attached - a line drawn by hand - asks for it here.
        void getTooltip(GetTooltipEvent&) const;
        //
        [[nodiscard]] Shortcut shortcut() const { return m_shortcut; }
        void setShortcut(const Shortcut value) { m_shortcut = value; }
    public:
        // What answers for this action in that form, or null when nothing up the focus chain
        // claimed it and the action answers for itself. Asked, never stored: two calls a frame
        // apart may answer differently.
        // The presenter is what is ASKING, where something is - a button showing this command
        // wanting to know whether to draw itself available. It is the nearest answer there is to
        // where the command is about to act, and a caller with none - a shortcut - passes none.
        [[nodiscard]] Control* subject(FormBase&, Control* presenter = nullptr) const;
        [[nodiscard]] ActionState state(FormBase&, Control* presenter = nullptr) const;
        [[nodiscard]] bool enabled(FormBase& form, Control* presenter = nullptr) const
        {
            return state(form, presenter).enabled;
        }
        // Runs the action, on the subject if there is one. Answers whether it ran - a disabled
        // action refuses, and the key that reached it stays unhandled.
        // EVERY ARGUMENT IS NAMED. A defaulted stamp reads as an argument nobody has to think
        // about, and what it produces is a command that works from the keyboard and silently does
        // nothing from a menu on any display server that authorises the request.
        bool invoke(FormBase&, Control* presenter, InputStamp);
    public:
        // Takes a presenter: a control that shows this action and delivers its click. The
        // subject is not attached - it is found - so this is the only side that is wired.
        //
        // Callable from the presenter's own constructor, which is where a control given an
        // action as a property attaches itself. Connecting only inserts into the dispatcher the
        // control has already contributed, and nothing is asked of the control until its first
        // state query.
        void attach(Control& presenter, PresenterRole role = PresenterRole::Button);
        void detach(const Control& presenter);
        [[nodiscard]] std::size_t attachedCount() const { return m_attachments.size(); }
    public:
        // What the action reads has changed. Asks every attached presenter for its state again,
        // and animates the difference.
        void invalidateState();
        // Repaints every attached presenter, for an icon that draws something new.
        void invalidate() const;
        // The text changed, so the size every presenter asked for is stale.
        void invalidateText();
    private:
        using ConnectionCollection = std::vector<EventConnection>;
        using EntryControls = std::array<Control*, 4>;

        // One presenter, and what was connected on it. The connections are plain, never scoped:
        // the only path that drops them without disconnecting is a presenter being destroyed,
        // and unwiring a dispatcher from inside its own emit would invalidate the iteration that
        // is calling the handler.
        //
        // A handler may capture the presenter's address but never an attachment's: attachments
        // live in a vector, and taking another moves the ones already there.
        struct Attachment
        {
            Control* control{};
            ConnectionCollection connections{};
        };

        using AttachmentCollection = std::vector<Attachment>;
    private:
        // The state, and the control that gave it. One walk answers both, so the control asked
        // whether it could is the control that is asked to.
        ActionState resolve(FormBase&, Control*& subject, Control* presenter) const;
        // Where the walk may start, nearest first. Every one of them is tried, so a form that
        // is not what it looks like from any single one of them still answers.
        [[nodiscard]] static EntryControls entryControls(FormBase&, Control* presenter);
        void connectPresenter(Attachment&, PresenterRole);
        [[nodiscard]] AttachmentCollection::iterator findAttachment(const Control&);
        // Forgets a presenter without unwiring it, which is what a presenter being destroyed
        // needs: its dispatcher is inside the object running this, and disconnecting there would
        // erase from the vector that emit is walking.
        void dropAttachment(const Control&);
    private:
        static constexpr std::size_t k_presenterChannels = 7;
    private:
        Text m_text{};
        TooltipText m_tooltipText{};
        Shortcut m_shortcut{};
        AttachmentCollection m_attachments{};
    };


    // A set of actions a key can be looked up in. See Control-Foundation
    export class Actions
    {
    public:
        Actions() = default;
        // Takes an action of its own and hands it back, for the caller to attach to presenters.
        template <typename... Args>
        Action& add(Args&&... args);
        // Registers one owned elsewhere.
        void add(Action&);
        void remove(const Action&);
        // Runs the action this key is the shortcut of, and answers whether one ran. A disabled
        // action leaves the key alone rather than swallowing it, so the next scope still gets
        // to answer - and so does a second action sharing the shortcut.
        [[nodiscard]] bool runShortcut(const KeyDownEvent&, FormBase&) const;
    private:
        using ActionCollection = std::vector<Action*>;
        using OwnedActionCollection = std::vector<std::unique_ptr<Action>>;
    private:
        // Every entry, owned or not. An owned action is in both, and the owning list is only
        // what keeps it alive.
        ActionCollection m_actions{};
        OwnedActionCollection m_owned{};
    };

    // The application's own scope of commands. See Control-Foundation
    export class AppActions : public Actions
    {
    public:
        [[nodiscard]] static AppActions& get();
    private:
        AppActions() = default;
    };

    // The command every AppButton presents: opening the application's menu. See Control-Foundation
    export [[nodiscard]] Action& appMenuAction();


    //-------------------------------------------------------------------------


    // Actions

    template<typename ...Args>
    Action& Actions::add(Args&& ...args)
    {
        m_owned.push_back(std::make_unique<Action>(std::forward<Args>(args)...));
        Action& result = *m_owned.back();
        m_actions.push_back(&result);
        return result;
    }


    // Action
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface.

    template<typename ...Args>
        requires (sizeof...(Args) > 0) && NotSelfCopy<Action, Args...>
    Action::Action(Args&& ...args)
        :
        EventComponent{ args... }
    {
        auto appendText = [&](const auto& p) {
            m_text << p;
        };
        Props::ifThereIs<Text>(appendText, args...);
        Props::ifThereIs<std::wstring_view>(appendText, args...);
        Props::ifThereIs<const wchar_t*>(appendText, args...);
        Props::ifThereIs<Shortcut>([&](const Shortcut& p) {
            m_shortcut = p;
        }, args...);
        Props::ifThereIs<TooltipText>([&](const TooltipText& p) {
            m_tooltipText = p;
        }, std::forward<Args>(args)...);
    }

}
