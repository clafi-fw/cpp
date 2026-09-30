export module ClaFi.Core.Foundation :Hint;

import :Control;

import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    export class HintForm;

    // TODO: remove Hint from the facade.
    // One per form, built with it: the window a hint shows in. See Control-Foundation
    export class Hint
    {
        // Builds this with itself, and takes its window down at the top of ~FormBase.
        friend FormBase;
    public:
        explicit Hint(FormBase& ownerForm);
        ~Hint();
        Hint(const Hint&) = delete;
        Hint& operator=(const Hint&) = delete;
    public:
        // The widest line a hint breaks at, in design units. See Control-Foundation#hint
        static constexpr float k_lineWidth{ 480.0f };
    public:
        // Puts this form's hint up about one of this form's controls, without the wait.
        void showRightNow(Control&);
    public:
        // The pointer went somewhere. Heard while the mouse drives. The hint coming down
        // belongs to the form the pointer was in, which is not always the form it has arrived
        // in, so this is static.
        static void hoveredControlChanged();
        // The pointer crossed into another zone of the control it is on - into its text or out.
        static void hoveredZoneChanged();
        // A key landed the focus somewhere. Heard while the keyboard alone drives.
        static void focusedControlChanged();
        // The mouse took over from the keyboard: the hover has its say from here on.
        static void mouseTookOver();
        // The control is being destroyed and takes with it anything said about it.
        static void forgetControl(const Control*);
        // The user did something. A hint waits for them to stop, and this is them starting
        // again.
        static void handleUserInput();
        // Takes down whatever is up, wherever it is, and answers whether there was anything to
        // take down - Escape is handled by having dismissed one.
        static bool stopAndHide();
        // The control the hint on screen is about, or nullptr while none is on screen.
        [[nodiscard]] static Control* control();
    private:
        // The control a hint is asked of: the hovered one while the mouse drives, the item the
        // focus rests on while the keyboard alone does. See Control-Foundation#hint
        [[nodiscard]] static Control* pointedControl();
        // The control a hint is about has changed - to pointedControl(), or to nothing.
        static void pointedControlChanged();
        void showOrHide(Control* = nullptr);
        void startWaiting(MilliSeconds);
        void updatePosition(const FloatRect& anchorRect, bool forceRepaint);
        // The rect the control states for its hint - its bounds unless it names another.
        [[nodiscard]] FloatRect anchorOf(Control&) const;
        [[nodiscard]] bool stillVisible() const;
        void destroyForm();
    private:
        // The hint waiting or showing, and nothing while neither is happening.
        inline static Hint* s_current{ nullptr };
        FormBase& m_ownerForm;
        // A HINT HAS NO HINT OF ITS OWN: this is null on the one form whose role is Hint,
        // and that is where the recursion of every form building one ends. Nothing asks for it
        // there - a hint window takes no pointer and holds nothing that can be hovered.
        std::unique_ptr<HintForm> m_form;
        // The handler is given in the constructor rather than here: MSVC will not deduce OnEvent
        // from a lambda written in a default member initializer.
        UiTimer m_timer;
    };

}
