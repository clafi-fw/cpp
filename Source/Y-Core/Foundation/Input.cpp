module ClaFi.Core.Foundation;

import :Input;
import :Control;
import :Form;
import :Tooltip;

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

    void Input::setFocusedControl(Control* value)
    {
        if (s_focusedControl == value)
            return;

        Control* previousControl = s_focusedControl;

        s_focusedControl = value;

        // Told after s_focusedControl has moved, so isFocused() answers for the control being
        // told whichever side of the change it is on. The control that HOLDS the focus and the
        // one it delegates to can be two controls - a container answers focusDelegate() with the
        // item inside it - and each is told once. A control that answers with itself is one
        // control and is told once, which is what the identity test is for.
        auto notifyFocusChange = [](Control* control) {
            if (!control)
                return;
            control->invalidateState();
            control->focusChanged();
            Control* delegate = control->focusDelegate();
            if (!delegate || delegate == control)
                return;
            delegate->invalidateState();
            delegate->focusChanged();
        };

        notifyFocusChange(previousControl);
        notifyFocusChange(s_focusedControl);
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
            Control* controlToFocus = s_hoveredControl;
            while (controlToFocus && !controlToFocus->canTakeFocus())
            {
                Control* parent = controlToFocus->m_parent;
                // A MOUSE-ONLY CONTROL IS PRESSED FOR ITSELF, and its press leaves the focus where
                // it is: a tool bar's button beside an editor keeps the caret in the editor. A
                // part is the exception - a slider's thumb, a check box's mark, a control hosted
                // in a grid cell - there the press is the owner's, and the walk goes on from it.
                // See UI-Types#interactivity
                const bool mouseOnly = controlToFocus->interactivity() == Interactivity::MouseOnly;
                const bool pressedForItself = mouseOnly
                    && !(parent && parent->isChildPart(*controlToFocus));
                controlToFocus = pressedForItself ? nullptr : parent;
            }
            if (controlToFocus)
                controlToFocus->setFocus();

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
        if (value == s_focusedControl)
            s_focusedControl = nullptr;
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

    // Both factors reach the ring, so the control holding the focus has to be repainted as either
    // moves - and so does the hovered one, whose ring the same factors gate. A rule reading
    // Keyboard or Mouse can stand on any control of any window, so every window repaints as well.
    void Input::controllerFactorChanged(AppContext& appContext)
    {
        invalidateForControllers(s_hoveredControl);
        if (s_focusedControl != s_hoveredControl)
            invalidateForControllers(s_focusedControl);
        appContext.events().emit<InputSwitchEvent>();
    }

    // The repaint is asked for OUTRIGHT, not left to invalidateState. These factors belong to the
    // application rather than to any control, so no control's own state has moved, and
    // invalidateState repaints only when one of the control's own factors has - it would leave
    // the ring at its last painted width until something else happened to repaint the control,
    // and then drop it in one step instead of fading.
    //
    // The control that HOLDS the focus and the item it answers for can be two controls, and the
    // ring is on the item.
    void Input::invalidateForControllers(Control* control)
    {
        if (!control)
            return;
        Control::invalidateStateUp(control, nullptr, InvalidateEvent::InputController);
        control->invalidate();
        if (Control* delegate = control->focusDelegate(); delegate != control)
            delegate->invalidate();
    }

    InputController Input::s_mouse{ AnimationSlots::mouseActive, true };
    InputController Input::s_keyboard{ AnimationSlots::keyboardActive, false };

}
