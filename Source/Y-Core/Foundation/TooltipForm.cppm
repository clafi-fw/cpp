export module ClaFi.Core.Foundation :TooltipForm;

// A partition rather than a module beside Foundation. Control.cppm says
// `friend class TooltipLabel;`, which declares TooltipLabel attached to
// ClaFi.Core.Foundation - so the class has to be defined in that same module, not in a
// second one that imports it. Nothing outside Foundation names TooltipForm: Tooltip.cpp
// is the only importer, and it is an implementation unit of this module.
import :Control;
import :Form;
import :WithTextLayout;
import :Tooltip;
import :ContextMessage;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Animation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export class TooltipForm;

    // Holding its own layout rather than sharing the engine's cache. A hint over a slider being
    // dragged says something different on every frame of the drag, and a text nothing says twice
    // is a cache entry nothing will ever hit again - see WithTextLayout.
    class TooltipLabel : public WithTextLayout<FormControlBase>
    {
        friend TooltipForm;
    public:
        using WithTextLayout<FormControlBase>::WithTextLayout;
    protected:
        void getText(GetTextEvent&) const override;
        [[nodiscard]] float lineBreakWidth() const override { return m_breakWidth; }
        // Asked no wider than Tooltip::k_lineWidth, unless it repeats lines broken elsewhere.
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&) override;
        ScaledDimensions calculateContent(AlignEvent& event) override { return FormControlBase::calculateContent(event); }
    private:
        Control* m_control{ nullptr };
        // The width the words this hint repeats were broken at, and zero for a hint repeating
        // none - see TooltipForm::setControl.
        float m_breakWidth{ 0.0f };
    };

    export class TooltipForm : public Form<TooltipLabel>
    {
        friend Tooltip;
        friend TooltipLabel;
    public:
        explicit TooltipForm(FormBase& ownerForm);
        ~TooltipForm() override;
        Control* control() { return content().m_control; }
        void setControl(Control* control);
        void setControl(Control&, const GetTooltipEvent&);
        void startAlphaAnimation();
        bool stillVisible() const;
        bool hideOnUserInput() const { return m_hideOnUserInput; }
        // The text the layout on screen was measured from, which is what the control answers in
        // the Calculate phase. A control is free to be measured from a line it never shows - a
        // hue slider asks for the room 360 degrees needs and shows the degree it is on - so this
        // is what says whether the answer has moved since that layout, and is not what the
        // window says.
        [[nodiscard]] const Text& measuredText() const { return m_measuredText; }
        // The words the last visible paint put on screen.
        //
        // THEY OUTLIVE THE CONTROL BEING CLEARED, because the pixels do. A window taken down goes
        // out over an alpha animation and is painted the whole way, and these are the words every
        // one of those paints repeats - see TooltipLabel::getText.
        [[nodiscard]] const Text& shownText() const { return m_shownText; }
    protected:
        void updateVisibility() override;
    private:
        static constexpr ColorByte m_alpha = 255; // a bit of transparency looks good only when bg is blurred
    private:
        bool m_hideOnUserInput{ true };
        Text m_measuredText{};
        // Written by TooltipLabel::getText, which is const because asking a control for its text
        // does not change the label. What that answer was is a record of the window rather than
        // state of the label, so the const does not propagate to it.
        mutable Text m_shownText{};
    };


    //-------------------------------------------------------------------------


    // TooltipLabel

    void TooltipLabel::getText(GetTextEvent& event) const
    {
        const TooltipForm& form = static_cast<const TooltipForm&>(this->form());

        // A WINDOW THAT HAS BEEN LET GO OF KEEPS THE WORDS IT WENT OUT WITH. It goes on being
        // painted for as long as its alpha runs down, and every one of those paints asks again -
        // so asking a control there would repaint the window part way through the fade as
        // whatever that control says for itself NOW. A message being taken down is exactly that
        // case: it has been forgotten by the time the fade starts, and what the user was reading
        // would turn into the control's own hint in front of them.
        //
        // HAVING NO CONTROL IS THE WHOLE TEST, and the form's own visibility is not part of it.
        // A hint stands over a control whose answer changes underneath it - a slider dragged with
        // its value on the thumb - and the visible flag says where the alpha is heading rather
        // than whether there is anything to say. Read here, it stops a window that is on screen
        // from ever asking again, and the value stands at whatever it last said for the length of
        // the drag. Every route that takes a hint down goes through setControl(nullptr), so the
        // control being gone is exactly the state these words are kept for.
        if (!m_control)
        {
            event.text = form.shownText();
            return;
        }

        GetTooltipEvent tooltipEvent{ form.context(), *m_control, event.text, event.phase() };
        tooltipEvent.hideOnUserInput = form.m_hideOnUserInput;
        tooltipEvent.placement = form.placement();
        event.text.clear();
        // Asked in the same order the tooltip asked it in when it decided there was something to
        // show. Asked of the control alone, a message standing about this control would be laid
        // out as the control's own tooltip and the window would show the wrong words.
        if (!ContextMessage::answer(*m_control, tooltipEvent))
            m_control->getTooltip(tooltipEvent);
        // WHAT A PAINT PRODUCED IS WHAT THE FADE REPEATS. A calculation is a measurement and is
        // answered with the room a later value will need, so recording that one would send the
        // window out saying a line it never showed.
        if (event.phase() == EventPhase::Paint)
            form.m_shownText = event.text;
    }

    CalculatedDimensions TooltipLabel::measureText(AlignEvent& event, ScaledDimensions asked,
        const Text& text)
    {
        // A hint the screen alone bounds runs a long sentence out as one line from the pointer to
        // the edge. The window is then sized to the widest line, so a short hint stays short.
        if (m_breakWidth == 0.0f)
            asked.x = std::min(asked.x, Tooltip::k_lineWidth * event.scaleFactor());
        return WithTextLayout<FormControlBase>::measureText(event, asked, text);
    }

    // TooltipForm

    TooltipForm::TooltipForm(FormBase& ownerForm)
        :
        Form{
            ownerForm.appContext(),
            // The pointer goes through the window to whatever is behind it.
            WindowRole::Tooltip,
            // THE FORM IT STANDS OVER, NAMED DIRECTLY. Every other popup names the CONTROL it was
            // opened on and its form is read off that; this one is a member of the form it belongs
            // to and is built with it, before it is about any control at all. Owning the window to
            // that form is what keeps it above that form - a menu's hint over the menu - and is
            // why the window carries no always-on-top bit of its own.
            ownerForm,
            // And the label under it is not pointed at either. Said in its own right rather than
            // taken from the window: the two are separate answers, and this one is the label's.
            Interactivity::None,
            UiElement::Tooltip,
            ownerForm.appContext().themeMetrics().secondaryWindow,
            ownerForm.appContext().themeMetrics().secondaryWindowShadow
        }
    {
        window().setAlpha(0);
        // A tooltip is sized by what is in it, and its text changes under it: the same control
        // answers differently as its state moves, and a control it never left can hand it a
        // longer line than the window it is already showing in. Without this, an alignment lays
        // that line out inside the old bounds and it is cut; with it, every alignment is a
        // placement and the bounds come from the content.
        setAutoFit(AutoFit::Yes);
    }

    TooltipForm::~TooltipForm()
    {
        appContext().animator().stop(this);
    }

    void TooltipForm::setControl(Control* control)
    {
        content().m_control = control;
        if (!control)
        {
            // The window is about nothing from here on, and it says nothing more: the label
            // stops asking once the form is not visible. The words it went out with are left
            // standing - see m_shownText - because the pixels they were laid out for are still on
            // screen for the whole of the fade, and wiping them empties the window in front of
            // whoever is reading it.
            hide();
            return;
        }
        content().invalidate();
    }

    void TooltipForm::setControl(Control& control, const GetTooltipEvent& event)
    {
        setControl(&control);
        m_measuredText = event.text;
        setPlacement(event.placement);
        m_hideOnUserInput = event.hideOnUserInput;
        // AN OVER-TEXT HINT IS THE CONTROL'S OWN LAYOUT, UNCUT. It stands on the words it repeats,
        // so it breaks its lines where they were broken - the anchor rect is the box that text was
        // drawn in - and its window is then sized to the whole of the text instead of to the part
        // that fitted. Every other placement is a hint in its own right and lays itself out.
        const bool overText = event.placement == FormPlacement::OverText;
        content().m_breakWidth = overText ? event.anchorRect.width() : 0.0f;
        const bool wrap = !overText or event.wordWrap;
        content().setWordWrap(wrap ? WordWrap::Yes : WordWrap::No);
        invalidateAlign();
    }

    void TooltipForm::startAlphaAnimation()
    {
        const float currentValue = static_cast<float>(window().alpha());
        const float targetValue = static_cast<float>(visible()) * m_alpha;
        appContext().animator().start(this, AnimationSlots::formAlpha, currentValue, targetValue,
            [this](const AnimateParams& params) {
                window().setAlpha(static_cast<ColorByte>(params.value));
            });
    }

    bool TooltipForm::stillVisible() const
    {
        return window().alpha();
    }

    void TooltipForm::updateVisibility()
    {
        // nope! Form::updateVisibility();
        if (visible())
            updatePlacement();
        startAlphaAnimation();
        window().show();
    }

}
