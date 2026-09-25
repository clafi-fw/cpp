module ClaFi.Core.Foundation;

import :Control;
import :Input;
import :Form;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    Action& appMenuAction()
    {
        // Built on first use, which is a button's constructor. Text is a string and its markers,
        // so nothing here needs the platform or the text engine.
        static Action result{ Text{ L"Application menu" } };
        return result;
    }
    // Shortcut

    bool Shortcut::matches(const KeyDownEvent& event) const
    {
        return !empty()
            && key == event.key
            && modifiers.shift == event.modifiers.shift
            && modifiers.ctrl == event.modifiers.ctrl
            && modifiers.alt == event.modifiers.alt;
    }

    std::wstring Shortcut::text() const
    {
        // Every part is asked of the layout, modifiers included, so the whole line reads the way
        // the keyboard in front of the user is labelled rather than the way this one is.
        const std::wstring name = Platform::keyName(key);
        if (name.empty())
            return {};
        std::wstring result;
        if (modifiers.ctrl)
            result += Platform::keyName(Keys::Ctrl) + L"+";
        if (modifiers.shift)
            result += Platform::keyName(Keys::Shift) + L"+";
        if (modifiers.alt)
            result += Platform::keyName(Keys::Alt) + L"+";
        result += name;
        return result;
    }

    // ActionEventBase

    ActionEventBase::ActionEventBase(const Action& action, Control* control)
        :
        action{ action },
        control{ control }
    {
    }

    // GetActionStateEvent

    void GetActionStateEvent::claim(const ActionState value)
    {
        m_state = value;
        m_claimed = true;
    }

    // ActionClickEvent

    ActionClickEvent::ActionClickEvent(const Action& action, Control* subject, Control* presenter,
        FormBase& form, InputStamp stamp)
        :
        ActionEventBase{ action, subject },
        form{ form },
        presenter{ presenter },
        stamp{ stamp }
    {
    }

    // GetShortcutEvent

    GetShortcutEvent::GetShortcutEvent(const Control& control)
        :
        control{ control }
    {
    }

    // Action

    Action::~Action()
    {
        for (const Attachment& attachment : m_attachments)
            for (const EventConnection& connection : attachment.connections)
                connection.disconnect();
    }

    void Action::getTooltip(GetTooltipEvent& event) const
    {
        // Whatever else answered wins. An action supplies the words for a presenter that has
        // none of its own, and a presenter that spells its own tooltip is stating an exception.
        if (!event.text.empty())
            return;

        // ITS OWN NAME WHERE IT SAYS NOTHING MORE. A command with a key has something to tell
        // even when its tooltip would only repeat its label, because the key is shown nowhere
        // else on a button. What the action says of itself is preferred where it says anything.
        if (m_tooltipText.empty())
            event.text << m_text;
        else
            event.text << m_tooltipText;
        // A nameless action contributes nothing at all, so that the trimmed-text fallback
        // Control::getTooltip ends with is left the way it was found.
        if (event.text.empty())
            return;

        const std::wstring shortcutText = m_shortcut.text();
        if (shortcutText.empty())
            return;

        event.text << L" (" << shortcutText << L")";
    }

    Control* Action::subject(FormBase& form, Control* presenter) const
    {
        Control* result = nullptr;
        resolve(form, result, presenter);
        return result;
    }

    ActionState Action::state(FormBase& form, Control* presenter) const
    {
        Control* subject = nullptr;
        return resolve(form, subject, presenter);
    }

    bool Action::invoke(FormBase& form, Control* presenter, InputStamp stamp)
    {
        Control* subject = nullptr;
        if (!resolve(form, subject, presenter).enabled)
            return false;

        ActionClickEvent event{ *this, subject, presenter, form, stamp };
        if (subject)
            subject->emitEvent(event);
        else
            emitEvent(event);
        return true;
    }

    void Action::attach(Control& presenter, const PresenterRole role)
    {
        m_attachments.push_back(Attachment{ .control = &presenter });
        connectPresenter(m_attachments.back(), role);
    }

    void Action::detach(const Control& presenter)
    {
        const auto it = findAttachment(presenter);
        if (it == m_attachments.end())
            return;
        for (const EventConnection& connection : it->connections)
            connection.disconnect();
        m_attachments.erase(it);
    }

    void Action::invalidateState()
    {
        for (const Attachment& attachment : m_attachments)
            attachment.control->invalidateState();
    }

    void Action::invalidate() const
    {
        for (const Attachment& attachment : m_attachments)
            attachment.control->invalidate();
    }

    void Action::invalidateText()
    {
        for (const Attachment& attachment : m_attachments)
            attachment.control->invalidateFormAlign();
    }

    ActionState Action::resolve(FormBase& form, Control*& subject, Control* presenter) const
    {
        subject = nullptr;

        // TODO: a container holding the focus answers for an item inside it, and
        // Control::focusDelegate names that item. It is protected, so the walk starts at the
        // holder and an item below it is not asked. Should Action be a friend of Control, or
        // should the delegate be reachable the way FormBase::currentItem already makes the
        // root's reachable?
        const EntryControls entries = entryControls(form, presenter);
        for (std::size_t index = 0; index != entries.size(); ++index)
        {
            Control* entry = entries[index];
            if (!entry)
                continue;
            // Two entries naming one control is ordinary - a form whose focus is on its current
            // item does - and walking it twice would only ask the same controls again.
            const auto walked = entries.begin() + index;
            if (std::find(entries.begin(), walked, entry) != walked)
                continue;

            for (Control* control = entry; control; control = control->parent())
            {
                // Stepping over a control that answers nothing costs a pointer chase, so a form
                // of ordinary controls is walked without an event being built at all.
                if (!control->hasEventListeners<GetActionStateEvent>())
                    continue;
                GetActionStateEvent event{ *this, control };
                control->emitEvent(event);
                if (!event.claimed())
                    continue;
                subject = control;
                return event.state();
            }
        }

        if (hasEventListeners<GetActionStateEvent>())
        {
            GetActionStateEvent event{ *this, nullptr };
            emitEvent(event);
            if (event.claimed())
                return event.state();
        }

        // Nothing claimed, and the action has no opinion of its own. It is available if there
        // is anything at all to run: an action that carries its work and says nothing about
        // when is available whenever nothing else answers for it.
        return { .enabled = hasEventListeners<ActionClickEvent>() };
    }

    Action::EntryControls Action::entryControls(FormBase& form, Control* presenter)
    {
        // Four answers to "where is the user", nearest first, because no single one of them is
        // right for every form a command can be raised from.
        //
        // THE PRESENTER IS THE FIRST, when there is one and it is the one asking. A control
        // showing a command stands, by construction, somewhere in the tree that command is about -
        // a page's own toolbar button stands in the page - and the focus need never have been
        // there. Without it a document command reads as unavailable until the user happens to
        // touch the document, and then turns available under its own press, which is the state
        // and the click disagreeing about the same question.
        //
        // It costs nothing where it is not the answer: a chain on which nothing claims falls
        // straight through to the ones below, which is what a presenter outside its subject - a
        // menu item over a text box - does every time.
        //
        // The focus is the second, and it is only usable when it is in this form: there is one
        // focus for the whole application, so a form that never took it sees a control in
        // another window. An in-place editor took it, and a command typed into one acts there.
        Control* focused = Input::focusedControl();
        if (focused && &focused->form() != &form)
            focused = nullptr;

        // What a popup was opened ON is the third, and for a menu it is THE answer: the focus
        // moves into the menu so that its items can be walked, and a menu item is never what the
        // command is about. It is the more precise answer anyway - a context menu acts on the
        // control it was raised from, whether or not the focus ever reached it.
        //
        // The form's current item is the last, for a form that is neither: the focus is in
        // another window, and this is what this one was last in.
        return { presenter, focused, form.popupTarget(), form.currentItem() };
    }

    void Action::connectPresenter(Attachment& attachment, const PresenterRole role)
    {
        Control& presenter = *attachment.control;
        Control* control = attachment.control;
        ConnectionCollection& connections = attachment.connections;
        connections.reserve(k_presenterChannels);

        // The control's own text wins. An action supplies the words for a presenter that has
        // none of its own, and a presenter that spells its own text is stating an exception.
        connections.push_back(presenter.onGetText([this](GetTextEvent& event) {
            if (event.text.empty())
                event.text << m_text;
        }));

        // A Line states the command in full by itself, so nothing is composed for it: a tooltip
        // there would say back the very line the pointer is standing on.
        if (role != PresenterRole::Line)
            connections.push_back(presenter.onGetTooltip([this](GetTooltipEvent& event) {
                getTooltip(event);
            }));

        connections.push_back(presenter.onPaintIcon([this](PaintIconEvent& event) {
            emitEvent(event);
        }));

        // A Display presenter is shown the command and does not deliver it. Its press belongs to
        // whatever routes it - a strip's goes to the button it is part of - and connecting here
        // as well would run the command twice off one press.
        if (role != PresenterRole::Display)
            connections.push_back(presenter.onClick([this, control](ClickEvent& event) {
                // Answered from the presenter, so the click acts on whatever its own state said
                // it would. Resolved from the focus alone the two can differ: pressing a
                // presenter takes the focus, so the click would ask a question the state was
                // never asked.
                invoke(event.form, control, event.stamp);
            }));

        // The presenter is captured rather than read off the event, which carries it as a
        // const reference. It is the same control either way, and this side already holds it.
        connections.push_back(presenter.onGetState([this, control, role](GetStateEvent& event) {
            // Asked here rather than in attach, because attach runs from the presenter's own
            // constructor: interactivity() answers for Control while the derived part is still
            // on its way up, and a control that hard-codes it - a Grid Row does - never puts it
            // in the property pack either. A state query is the earliest moment the answer is
            // the control's own, and it comes before the first paint.
            if (role != PresenterRole::Display && control->interactivity() == Interactivity::None)
                unreachable("An action was attached to a control that never delivers a click");
            event.state = state(control->form(), control);
        }));

        connections.push_back(presenter.onGetShortcut([this](GetShortcutEvent& event) {
            event.shortcut = m_shortcut;
        }));

        connections.push_back(presenter.onDestroy([this](DestroyEvent& event) {
            dropAttachment(event.control);
        }));
    }

    // Trailing return type on purpose: AttachmentCollection is private, and a return type
    // written ahead of the declarator-id is looked up before this definition is inside the
    // class.
    auto Action::findAttachment(const Control& presenter) -> AttachmentCollection::iterator
    {
        return std::ranges::find(m_attachments, &presenter, &Attachment::control);
    }

    void Action::dropAttachment(const Control& presenter)
    {
        const auto it = findAttachment(presenter);
        if (it != m_attachments.end())
            m_attachments.erase(it);
    }

    // Actions

    void Actions::add(Action& action)
    {
        m_actions.push_back(&action);
    }

    void Actions::remove(const Action& action)
    {
        std::erase(m_actions, &action);
        std::erase_if(m_owned, [&action](const std::unique_ptr<Action>& owned) {
            return owned.get() == &action;
        });
    }

    bool Actions::runShortcut(const KeyDownEvent& event, FormBase& form) const
    {
        for (Action* action : m_actions)
        {
            if (!action->shortcut().matches(event))
                continue;
            if (action->invoke(form, nullptr, event.stamp))
                return true;
        }
        return false;
    }

    // AppActions

    AppActions& AppActions::get()
    {
        // Built on first use, so nothing depends on the order the statics of a translation unit
        // are initialized in.
        static AppActions result{};
        return result;
    }

}
