export module ClaFi.Core.Foundation :Tooltip;

import :Control;

import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi
{
    export class TooltipForm;
    export class OverTextForm;

    // TODO: remove Tooltip from the facade.
    // One per form, built with it: the windows tooltips are shown in. See Control-Foundation
    export class Tooltip
    {
        // Builds this with itself, and takes its windows down at the top of ~FormBase.
        friend FormBase;
    public:
        explicit Tooltip(FormBase& ownerForm);
        ~Tooltip();
        Tooltip(const Tooltip&) = delete;
        Tooltip& operator=(const Tooltip&) = delete;
    public:
        // The widest line a tooltip breaks at, in design units.
        static constexpr float k_lineWidth{ 480.0f };
        // The air between a tooltip's caption and its body, in design units.
        static constexpr float k_partSpacing{ 6.0f };
        // The most lines a tooltip shows, its last faded when cut.
        static constexpr std::size_t k_maxLines{ 20 };
    public:
        // Puts this form's tooltip up about one of this form's controls, without the wait.
        void showRightNow(Control&);
    public:
        // The pointer went somewhere. Heard while the mouse drives. The tooltip coming down
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
        static void controlTooltipChanged(Control&); // the words its tooltip says have moved
        // The user did something. A tooltip waits for them to stop, and this is them starting
        // again.
        static void handleUserInput(const FormBase* source = nullptr);
        // Takes down whatever is up, wherever it is, and answers whether there was anything to
        // take down - Escape is handled by having dismissed one.
        static bool stopAndHide();
        // The control the tooltip on screen is about, or nullptr while none is on screen.
        [[nodiscard]] static Control* control();
    private:
        // The control a tooltip is asked of: the hovered one while the mouse drives, the item the
        // focus rests on while the keyboard alone does. See Control-Foundation#tooltip
        [[nodiscard]] static Control* pointedControl();
        // The control a tooltip is about has changed - to pointedControl(), or to nothing.
        static void pointedControlChanged(bool allowLinkHandoff = true);
        // Whether a tooltip up about a control this one stands in still answers for this one.
        [[nodiscard]] static bool answersFor(Control&);
        void showOrHide(Control* = nullptr);
        void startWaiting(MilliSeconds);
        void updatePosition(Control&, const FloatRect& anchorRect);
        [[nodiscard]] bool stillVisible() const;
        void destroyForm();
        // **The overflown text is a window of its own.** It is asked of the control the pointer is
        // on, shown over the words it repeats, and takes no part in the event the other window is
        // asked through. See Control-Foundation#tooltip
        static void overTextChanged();
        void startOverTextWaiting(Control&);
        void showOrHideOverText();
        void stopOverText();
    private:
        // The tooltip waiting or showing, and nothing while neither is happening.
        inline static Tooltip* s_current{ nullptr };
        // The over-text window waiting or showing, in the same way and apart from the tooltip.
        inline static Tooltip* s_overTextCurrent{ nullptr };
        FormBase& m_ownerForm;
        // A tooltip form builds neither window, which ends the recursion.
        std::unique_ptr<TooltipForm> m_form;
        std::unique_ptr<OverTextForm> m_overTextForm;
        // The handler is given in the constructor rather than here: MSVC will not deduce OnEvent
        // from a lambda written in a default member initializer.
        UiTimer m_timer;
        UiTimer m_linkHandoffTimer;
        UiTimer m_overTextTimer;
        Control* m_overTextControl{ nullptr };
        Text m_overTextText{};
    };

}
