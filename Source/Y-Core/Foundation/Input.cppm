export module ClaFi.Core.Foundation :Input;

import :Control;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    export class Input;

    // The mouse or the keyboard, and whether it drives the input. See Control-Foundation
    export class InputController
    {
        friend Input;
    public:
        [[nodiscard]] bool active() const { return m_active; }
        [[nodiscard]] float factor() const { return m_factor; }   // active() through its fade
    private:
        InputController(const AnimationSlot&, bool active);
        [[nodiscard]] bool setActive(AppContext&, bool value);   // whether it changed
    private:
        const AnimationSlot& m_slot;
        float m_factor;
        bool m_active;
    };

    // The input state the whole framework reads. See Control-Foundation
    export class Input
    {
        friend Control;
        friend FormBase;
        friend InputController;
    public:
        // Where the focus rests. This may be a container answering for an item inside it.
        [[nodiscard]] static Control* focusHolder() { return s_focusHolder; }
        // focusHolder(), or the item it answers for. See Control-Foundation#input
        [[nodiscard]] static Control* focusDelegate();
        [[nodiscard]] static Control* hoveredControl() { return s_hoveredControl; }
        [[nodiscard]] static HitTest hoveredZone() { return s_hoveredZone; }
        // Whether the pointer stands on the hovered control's own text. Apart from the zone: a
        // title bar answers Title for the press and still says where its text is.
        [[nodiscard]] static bool hoveredOverText() { return s_hoveredOverText; }
        [[nodiscard]] static const InputController& mouse() { return s_mouse; }
        [[nodiscard]] static const InputController& keyboard() { return s_keyboard; }
        [[nodiscard]] static bool isMouseDown() { return s_isMouseDown; }
        // A control holding the focus changed the item it answers for.
        static void focusDelegateChanged(Control& holder, Control* previousDelegate,
            Control* currentDelegate);
    private:
        // The single write to the focus holder.
        static void setFocusHolder(Control* value);
        static void setFocusHolder(Control& value) { setFocusHolder(&value); }
        // The focus moved from one delegate to another, and the path each stands on is told.
        static void focusMoved(Control* previousDelegate, Control* currentDelegate);
        // The next control up the focus path. See Control-Foundation#control-isfocused
        [[nodiscard]] static Control* focusPathParent(Control&);
        // Where the focus paths up from two controls meet, null where they never do.
        [[nodiscard]] static Control* focusPathMeeting(Control* first, Control* second);
        // What the focus path from start passes just before control, null where there is none.
        [[nodiscard]] static Control* focusPathStepBelow(Control* start, const Control& control);
        // Each control on the focus path from `from` up to upTo, upTo left out, told of the event.
        static void invalidateFocusPath(Control* from, const Control* upTo, InvalidateEvent);
        // The single write to the hover: what the pointer is over, as the form hit-tests it.
        static void setHoveredControl(Control*, HitTest, bool overText);
        static void setHoveredControl(Control* control) { setHoveredControl(control, HitTest::Client, false); }
        static void mouseActed(AppContext&);      // mouse on, keyboard off
        static void keyActed(AppContext&);        // keyboard on, mouse off
        static void modifierActed(AppContext&);   // keyboard on, mouse as it is
        static void setMouseDown(FormBase&, PressUpHandled& handled, bool forceUpdate = false);
        static void setMouseUp(const FormBase&, Control*, PressUpHandled& handled, bool& scrollIntoView);
        // Called from ~Control. The pointers are dropped without telling anyone: the control is
        // being destroyed, so there is nothing left to invalidate and nothing to notify.
        static void forgetControl(const Control*);
        static Control* commonParent(Control*, Control*);
        static void controllerFactorChanged(AppContext&);
        static void invalidateForControllers(Control*);
    private:
        static InputController s_mouse;
        static InputController s_keyboard;
        inline static Control* s_focusHolder{ nullptr };
        inline static Control* s_hoveredControl{ nullptr };
        inline static HitTest s_hoveredZone{ HitTest::Client };
        inline static bool s_hoveredOverText{ false };
        inline static bool s_isMouseDown{ false };
    };

}
