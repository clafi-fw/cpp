export module ClaFi.Core.Foundation :Tooltip;

import :Control;

import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    export class TooltipForm;

    // TODO: remove Tooltip from the facade.
    // One per form, built with it: the window a hint shows in. See Control-Foundation
    export class Tooltip
    {
        // Builds this with itself, and takes its window down at the top of ~FormBase.
        friend FormBase;
    public:
        explicit Tooltip(FormBase& ownerForm);
        ~Tooltip();
        Tooltip(const Tooltip&) = delete;
        Tooltip& operator=(const Tooltip&) = delete;
    public:
        // The widest line a hint breaks at, in design units. See Control-Foundation#tooltip
        static constexpr float k_lineWidth{ 480.0f };
    public:
        // Puts this form's tooltip up about one of this form's controls, without the wait.
        void showRightNow(Control&);
    public:
        // The pointer went somewhere. The tooltip coming down belongs to the form the pointer was
        // in, which is not always the form it has arrived in, so this is static.
        static void hoveredControlChanged();
        // The pointer crossed into another zone of the control it is on - into its text or out.
        static void hoveredZoneChanged();
        // The control is being destroyed and takes with it anything said about it.
        static void forgetControl(const Control*);
        // The user did something. A tooltip waits for them to stop, and this is them starting
        // again.
        static void handleUserInput();
        // Takes down whatever is up, wherever it is, and answers whether there was anything to
        // take down - Escape is handled by having dismissed one.
        static bool stopAndHide();
        // The control the tooltip on screen is about, or nullptr while none is on screen.
        [[nodiscard]] static Control* control();
    private:
        void showOrHide(Control* = nullptr);
        void startWaiting(MilliSeconds);
        void updatePosition(const FloatRect& anchorRect, bool forceRepaint);
        [[nodiscard]] bool stillVisible() const;
        void destroyForm();
    private:
        // The tooltip waiting or showing, and nothing while neither is happening.
        inline static Tooltip* s_current{ nullptr };
        FormBase& m_ownerForm;
        // A TOOLTIP HAS NO TOOLTIP OF ITS OWN: this is null on the one form whose role is Tooltip,
        // and that is where the recursion of every form building one ends. Nothing asks for it
        // there - a tooltip window takes no pointer and holds nothing that can be hovered.
        std::unique_ptr<TooltipForm> m_form;
        // The handler is given in the constructor rather than here: MSVC will not deduce OnEvent
        // from a lambda written in a default member initializer.
        UiTimer m_timer;
    };

}
