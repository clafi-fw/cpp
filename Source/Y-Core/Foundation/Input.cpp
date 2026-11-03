module ClaFi.Core.Foundation;

import :Input;
import :Control;
import :Form;
import :Tooltip;

import ClaFi.Diagnostic.Log;
import ClaFi.Diagnostic.Options;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    InputController::InputController(const AnimationSlot& slot, bool active)
        :
        m_slot{ slot },
        m_factor{ static_cast<float>(active) },
        m_active{ active }
    {
    }

    // Keyed on no control: the fade belongs to the application rather than to any control. The
    // slot is what tells one controller's fade from the other's, so the two never share a slot.
    bool InputController::setActive(AppContext& appContext, bool value)
    {
        if (m_active == value)
            return false;
        m_active = value;
        appContext.animator().start(nullptr, m_slot, m_factor, static_cast<float>(m_active),
            [this, &appContext](const AnimateParams& params) {
                m_factor = params.value;
                Input::controllerFactorChanged(appContext);
            });
        return true;
    }

    void Input::setHoveredControl(Control* value, HitTest hitZone, bool overText)
    {
        bool controlChanged = value != s_hoveredControl;
        bool hitZoneChanged = s_hoveredZone != hitZone;
        bool overTextChanged = s_hoveredOverText != overText;
        if (!(controlChanged || hitZoneChanged || overTextChanged))
            return;

        Control* previousControl = s_hoveredControl;
        s_hoveredZone = hitZone;
        s_hoveredOverText = overText;

        s_hoveredControl = value;
        if (controlChanged)
        {
            Control* upTo = commonParent(previousControl, value);
            Control::invalidateStateUp(previousControl, upTo, InvalidateEvent::HoverLeave);
            Control::invalidateStateUp(value, upTo, InvalidateEvent::HoverEnter);
            // The walks above stop at the common parent, because everything above it looks the
            // same before and after. A container that follows what is under the pointer does not:
            // it holds its items in groups as often as not, and then the common parent of two
            // items is the group and the container never hears. So the new control is carried the
            // whole way up as well - see Control::nestedControlHovered.
            if (value)
                value->nestedControlHovered(value);
        }
        else if ((hitZoneChanged || overTextChanged) && value)
        {
            value->invalidateState();
        }
        if (controlChanged)
            Tooltip::hoveredControlChanged();
        else
            Tooltip::hoveredZoneChanged();
    }

    Control* Input::focusDelegate()
    {
        return s_focusHolder ? s_focusHolder->focusDelegate() : nullptr;
    }

    void Input::setFocusHolder(Control* value)
    {
        if (s_focusHolder == value)
            return;
        Control* previousDelegate = focusDelegate();
        s_focusHolder = value;
        if constexpr (Diagnostic::Options::logFocusedItem())
            diagnosticLog(L"focus holder", s_focusHolder);
        focusMoved(previousDelegate, focusDelegate());
    }

    // The holder has not changed, so setFocusHolder does not run - and its delegate has. A
    // null on either side is the holder answering for itself.
    void Input::focusDelegateChanged(Control& holder, Control* previousDelegate,
        Control* currentDelegate)
    {
        if (s_focusHolder != &holder)
            return;
        focusMoved(previousDelegate ? previousDelegate : &holder,
            currentDelegate ? currentDelegate : &holder);
    }

    // **Every control whose isFocused() answer changed is told**, after the move, so that answer
    // is for the side it is on. The two walks stop where the paths meet, which answers the same
    // before and after - except a control that wears the ring on one side only: a focus scope the
    // focus moved between it and an item of its own. A popup's root stepping onto the control it
    // was opened on is no such move, since that control starts its window's part of the path on
    // both sides.
    void Input::focusMoved(Control* previousDelegate, Control* currentDelegate)
    {
        if (previousDelegate == currentDelegate)
            return;
        Control* upTo = focusPathMeeting(previousDelegate, currentDelegate);
        if (upTo)
        {
            Control* belowBefore = focusPathStepBelow(previousDelegate, *upTo);
            Control* belowAfter = focusPathStepBelow(currentDelegate, *upTo);
            if (upTo->containsNested(belowBefore) != upTo->containsNested(belowAfter))
                upTo = focusPathParent(*upTo);
        }
        invalidateFocusPath(previousDelegate, upTo, InvalidateEvent::FocusLeave);
        invalidateFocusPath(currentDelegate, upTo, InvalidateEvent::FocusEnter);

        if constexpr (Diagnostic::Options::logFocusedItem())
            diagnosticLog(L"focus delegate", currentDelegate);
    }

    // At a root the path goes on only from a popup that is up, which the form it stands on holds
    // as its active popup - and from there at what the control that held the focus when the popup
    // came up answers for, so a button that drops a menu without taking the focus takes none of it.
    // The control the popup was opened on stands in where none of that form's controls held it. A
    // dialog is a window of its own: its owner reads nothing through it.
    Control* Input::focusPathParent(Control& control)
    {
        if (Control* parent = control.parent())
            return parent;
        FormBase* form = control.getForm();
        if (!form)
            return nullptr;
        FormBase* opener = form->popupTargetForm();
        if (!opener || opener->activePopup() != form)
            return nullptr;
        Control* origin = opener->focusBeforePopup();
        Control* continuesAt = origin ? origin : form->popupTarget();
        return continuesAt ? continuesAt->focusDelegate() : nullptr;
    }

    Control* Input::focusPathMeeting(Control* first, Control* second)
    {
        for (Control* step = first; step; step = focusPathParent(*step))
        {
            for (Control* other = second; other; other = focusPathParent(*other))
            {
                if (other == step)
                    return step;
            }
        }
        return nullptr;
    }

    Control* Input::focusPathStepBelow(Control* start, const Control& control)
    {
        Control* below = nullptr;
        for (Control* step = start; step; step = focusPathParent(*step))
        {
            if (step == &control)
                return below;
            below = step;
        }
        return nullptr;
    }

    void Input::invalidateFocusPath(Control* from, const Control* upTo, const InvalidateEvent event)
    {
        for (Control* step = from; step && step != upTo; step = focusPathParent(*step))
            Control::invalidateStateUp(step, step->parent(), event);
    }

    void Input::mouseActed(AppContext& appContext)
    {
        const bool mouseTurnedOn = s_mouse.setActive(appContext, true);
        const bool keyboardTurnedOff = s_keyboard.setActive(appContext, false);
        if (!mouseTurnedOn && !keyboardTurnedOff)
            return;
        if (mouseTurnedOn)
            Tooltip::mouseTookOver();
        // The animations repaint from their first tick onwards; this is the frame before the first.
        invalidateForControllers(s_hoveredControl);
    }

    void Input::keyActed(AppContext& appContext)
    {
        const bool mouseTurnedOff = s_mouse.setActive(appContext, false);
        const bool keyboardTurnedOn = s_keyboard.setActive(appContext, true);
        if (!mouseTurnedOff && !keyboardTurnedOn)
            return;
        // The user is doing something, which is all the tooltip waits for.
        Tooltip::handleUserInput();
        invalidateForControllers(s_hoveredControl);
    }

    void Input::modifierActed(AppContext& appContext)
    {
        if (!s_keyboard.setActive(appContext, true))
            return;
        Tooltip::handleUserInput();
        invalidateForControllers(s_hoveredControl);
    }

    void Input::setMouseDown(FormBase& form, PressUpHandled& handled, bool forceUpdate)
    {
        if (!forceUpdate && s_isMouseDown)
            return;
        s_isMouseDown = true;
        if (s_hoveredControl)
        {
            Control::setFocusByPress(*s_hoveredControl);

            PressDownEvent event{ *s_hoveredControl, form };
            s_hoveredControl->doPressDown(event);
            if (event.propagationStopped())
            {
                handled.propagationStopped = true;
                handled.preventClick = true;
            }
            handled.downControlDirty = handled.downControlDirty || event.downControlDirty();
        }
    }

    void Input::setMouseUp(const FormBase& form, Control* control,
        PressUpHandled& handled, bool& scrollIntoView)
    {
        if (!s_isMouseDown)
            return;
        s_isMouseDown = false;
        if (control)
        {
            PressUpEvent event{
                .form = form,
                .control = *control,
                .handled = handled,
                .scrollIntoView = scrollIntoView,
                .modifiers = Platform::keyModifiers()
            };
            control->doPressUp(event);
        }
    }

    void Input::forgetControl(const Control* value)
    {
        if (value == s_focusHolder)
            s_focusHolder = nullptr;
        if (value == s_hoveredControl)
            s_hoveredControl = nullptr;
    }

    Control* Input::commonParent(Control* first, Control* second)
    {
        while (first)
        {
            Control* control = second;
            while (control)
            {
                if (control == first)
                    return control;
                control = control->parent();
            }
            first = first->parent();
        }
        return nullptr;
    }

    // Both factors reach the ring, so the control the focus is on has to be repainted as either
    // moves - and so does the hovered one, whose ring the same factors gate. A rule reading
    // Keyboard or Mouse can stand on any control of any window, so every window repaints as well.
    void Input::controllerFactorChanged(AppContext& appContext)
    {
        Control* delegate = focusDelegate();
        invalidateForControllers(s_hoveredControl);
        if (delegate && delegate != s_hoveredControl)
        {
            invalidateFocusPath(delegate, nullptr, InvalidateEvent::InputController);
            delegate->invalidate();
        }
        appContext.events().emit<InputSwitchEvent>();
    }

    // **Up to the root form**, through the control each popup between was opened on. The focused
    // control's chain is walked along the focus path instead - see controllerFactorChanged.
    //
    // The repaint is asked for **outright**, not left to invalidateState. These factors belong to
    // the application rather than to any control, so no control's own state has moved, and
    // invalidateState repaints only when one of the control's own factors has - it would leave
    // the ring at its last painted width until something else happened to repaint the control,
    // and then drop it in one step instead of fading.
    void Input::invalidateForControllers(Control* control)
    {
        if (!control)
            return;
        Control::invalidateStateToRoot(control, InvalidateEvent::InputController);
        control->invalidate();
    }

    InputController Input::s_mouse{ AnimationSlots::MouseActive, true };
    InputController Input::s_keyboard{ AnimationSlots::KeyboardActive, false };

}
