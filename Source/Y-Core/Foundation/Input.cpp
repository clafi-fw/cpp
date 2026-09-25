module ClaFi.Core.Foundation;

import :Input;
import :Control;
import :Form;
import :PaintEvent;
import :Tooltip;

import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
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

    void Input::setMouseDown(FormBase& form, PressUpHandled& handled, bool forceUpdate)
    {
        if (!forceUpdate && s_isMouseDown)
            return;
        s_isMouseDown = true;
        if (s_hoveredControl)
        {
            Control* controlToFocus = s_hoveredControl;
            while (controlToFocus && !controlToFocus->canTakeFocus())
                controlToFocus = controlToFocus->m_parent;
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

    // The focus ring is drawn through the factor, so the control holding the focus has to be
    // repainted as it moves - and so does the hovered one, whose ring the same factor gates.
    void Input::deviceFactorChanged(const AnimateParams& params)
    {
        PaintEvent::s_keyboardFactor = params.value;
        invalidateForDevice(s_hoveredControl);
        if (s_focusedControl != s_hoveredControl)
            invalidateForDevice(s_focusedControl);
    }

    // The repaint is asked for OUTRIGHT, not left to invalidateState. This factor belongs to the
    // application rather than to any control, so no control's own state has moved, and
    // invalidateState repaints only when one of the control's own factors has - it would leave
    // the ring at its last painted width until something else happened to repaint the control,
    // and then drop it in one step instead of fading.
    //
    // The control that HOLDS the focus and the item it answers for can be two controls, and the
    // ring is on the item.
    void Input::invalidateForDevice(Control* control)
    {
        if (!control)
            return;
        Control::invalidateStateUp(control, nullptr, InvalidateEvent::InputDevice);
        control->invalidate();
        if (Control* delegate = control->focusDelegate(); delegate != control)
            delegate->invalidate();
    }

    OnAnimate Input::s_onAnimateDevice{ &Input::deviceFactorChanged };

    void Input::setDevice(AnimationController& animator, const InputDevice value,
        const FocusTakesHover focusTakesHover)
    {
        if (s_device == value)
            return;
        s_device = value;
        if (value == InputDevice::Keyboard)
        {
            if (focusTakesHover == FocusTakesHover::Yes
                && s_focusedControl
                && s_focusedControl->enabled(true))
                setHoveredControl(s_focusedControl->focusDelegate());
            // The tooltip goes on any key, whether or not the hover moved: the user is doing
            // something, which is all it waits for.
            Tooltip::handleUserInput();
        }
        // One animation for the whole application rather than one per control, so it is keyed
        // on no control.
        animator.start(
            nullptr,
            AnimationSlots::inputDevice,
            PaintEvent::s_keyboardFactor,
            static_cast<float>(s_device),
            s_onAnimateDevice
        );
        // The animation repaints from its first tick onwards; this is the frame before it.
        invalidateForDevice(s_hoveredControl);
    }

}
