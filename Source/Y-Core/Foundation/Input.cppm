export module ClaFi.Core.Foundation :Input;

import :Control;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    // Whether a change of device carries the hover to the focused control. Navigation does: a
    // control reached by a key reads the way the one under the pointer does, and the two never
    // light up at once. A modifier pressed on its own does not - it says the keyboard is about
    // to act rather than that it has, and the pointer is still sitting where the user left it.

    // Whether moving the focus moves the hover with it.
    export enum class FocusTakesHover
    {
        Yes,
        No
    };

    // The input state the whole framework reads. See Control-Foundation
    export class Input
    {
        friend Control;
        friend FormBase;
    public:
        // Where the focus rests. This may be a container answering for an item inside it -
        // Control::focusDelegate() names the leaf.
        [[nodiscard]] static Control* focusedControl() { return s_focusedControl; }
        [[nodiscard]] static Control* hoveredControl() { return s_hoveredControl; }
        [[nodiscard]] static HitTest hoveredZone() { return s_hoveredZone; }
        // Whether the pointer stands on the hovered control's own text. Apart from the zone: a
        // title bar answers Title for the press and still says where its text is.
        [[nodiscard]] static bool hoveredOverText() { return s_hoveredOverText; }
        [[nodiscard]] static InputDevice device() { return s_device; }
        [[nodiscard]] static bool isMouseDown() { return s_isMouseDown; }
        // Moves the hover with no pointer movement behind it. The keyboard drags the hover along
        // with the focus, so a control reached by a key reads the way the one under the pointer
        // does, and the two never light up at once.
        static void setHoveredControl(Control*, HitTest, bool overText);
        static void setHoveredControl(Control* control) { setHoveredControl(control, HitTest::Client, false); }
    private:
        // The single write to the focus holder. Both sides of the change are told afterwards -
        // see Control::focusChanged.
        static void setFocusedControl(Control* value);
        static void setFocusedControl(Control& value) { setFocusedControl(&value); }
        // The fade between the two devices' looks runs on the application's controller, which
        // the form calling this has in hand: this class owns nothing that has to be released.
        static void setDevice(AnimationController&, InputDevice,
            FocusTakesHover = FocusTakesHover::Yes);
        static void setMouseDown(FormBase&, PressUpHandled& handled, bool forceUpdate = false);
        static void setMouseUp(const FormBase&, Control*, PressUpHandled& handled, bool& scrollIntoView);
        // Called from ~Control. The pointers are dropped without telling anyone: the control is
        // being destroyed, so there is nothing left to invalidate and nothing to notify.
        static void forgetControl(const Control*);
        static Control* commonParent(Control*, Control*);
        static void deviceFactorChanged(const AnimateParams&);
        static void invalidateForDevice(Control*);
    private:
        static OnAnimate s_onAnimateDevice;
        inline static Control* s_focusedControl{ nullptr };
        inline static Control* s_hoveredControl{ nullptr };
        inline static HitTest s_hoveredZone{ HitTest::Client };
        inline static bool s_hoveredOverText{ false };
        inline static bool s_isMouseDown{ false };
        inline static InputDevice s_device{ InputDevice::Mouse };
    };

}
