export module ClaFi.Core.Foundation :TooltipForm;

// A partition rather than a module beside Foundation. Control.cppm says
// `friend class TooltipLabel;`, which declares TooltipLabel attached to
// ClaFi.Core.Foundation - so the class has to be defined in that same module, not in a
// second one that imports it. Nothing outside Foundation names TooltipForm: Tooltip.cpp
// and Form.cpp import it, and both are implementation units of this module.
import :Control;
import :Form;
import :WithTextLayout;
import :Tooltip;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Animation;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export class TooltipForm;

    // Holding its own layout rather than sharing the engine's cache. A tooltip over a slider being
    // dragged says something different on every frame of the drag, and a text nothing says twice
    // is a cache entry nothing will ever hit again - see WithTextLayout.
    class TooltipLabel : public WithTextLayout<FormControlBase>
    {
        friend TooltipForm;
    public:
        using WithTextLayout::WithTextLayout;
    protected:
        [[nodiscard]] float lineBreakWidth() const override { return m_breakWidth; }
        // Held to Tooltip::k_maxLines, and to k_lineWidth unless it repeats lines broken elsewhere.
        ScaledSize measureText(AlignEvent&, ScaledSize maxDimensions, const Text&) override;
        ScaledSize calculateContent(AlignEvent& event) override { return FormControlBase::calculateContent(event); }
    private:
        Control* m_control{ nullptr };
        // The width the words this tooltip repeats were broken at, and zero for a tooltip repeating
        // none - see TooltipForm::setControl.
        float m_breakWidth{ 0.0f };
    };

    // The window a tooltip is shown in, owned by the window of the form the tooltip belongs to.
    export class TooltipForm : public Form<TooltipLabel>
    {
        friend Tooltip;
    public:
        explicit TooltipForm(FormBase& ownerForm);
        ~TooltipForm() override;
        Control* control() { return content().m_control; }
        void setControl(Control* control);
        void setControl(Control&, const GetTooltipEvent&); // the answer becomes the label's text
        void startAlphaAnimation();
        bool stillVisible() const;
        bool hideOnUserInput() const { return m_hideOnUserInput; }
    protected:
        void updateVisibility() override;
        // The control the tooltip is about, which the window goes along with as a popup does.
        [[nodiscard]] const Control* placementTarget() const override
            { return TooltipLabel::m_control; }
        // Where the control comes to rest, which is where a tooltip about it is placed.
        [[nodiscard]] FloatRect placementTargetBounds(const Control&) const override;
    private:
        static constexpr ColorByte m_alpha = 255; // a bit of transparency looks good only when bg is blurred
    private:
        bool m_hideOnUserInput{ true };
    };


    //-------------------------------------------------------------------------


    // TooltipLabel

    ScaledSize TooltipLabel::measureText(AlignEvent& event, ScaledSize maxDimensions,
        const Text& text)
    {
        // A tooltip the screen alone bounds runs a long sentence out as one line from the pointer
        // to the edge. The window is then sized to the widest line, so a short tooltip stays short.
        if (m_breakWidth == 0.0f)
            maxDimensions.x = std::min(maxDimensions.x, Tooltip::k_lineWidth * event.scaleFactor());
        TextLayout& layout = syncedLayout(event.formContext(),
            FloatRect::FromDimensions({ 0.0f, 0.0f }, maxDimensions), text);
        maxDimensions.y = std::min(maxDimensions.y, layout.linesHeight(Tooltip::k_maxLines));
        return WithTextLayout<FormControlBase>::measureText(event, maxDimensions, text);
    }

    // TooltipForm

    TooltipForm::TooltipForm(FormBase& ownerForm)
        :
        Form{
            ownerForm.appContext(),
            // The pointer goes through the window to whatever is behind it.
            WindowRole::Tooltip,
            // **The form it stands over, named directly.** Every other popup names the **control**
            // it was opened on and its form is read off that; this one is a member of the form it
            // belongs to and is built with it, before it is about any control at all. Owning the
            // window to that form is what keeps it above that form - a menu's tooltip over the
            // menu - and is why the window carries no always-on-top bit of its own.
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
            // The window is about nothing from here on. The words stay in the label, since the
            // pixels they were laid out for are on screen for the whole of the fade.
            hide();
            return;
        }
        content().invalidate();
    }

    void TooltipForm::setControl(Control& control, const GetTooltipEvent& event)
    {
        setControl(&control);
        // Taken here too - a rect stated equal to the last one records nothing.
        rememberPlacementTarget();
        content().text() = event.text;
        content().setTextFormat(event.format);
        setPlacement(event.placement);
        m_hideOnUserInput = event.hideOnUserInput;
        // **An over-text tooltip is the control's own layout, uncut.** It stands on the words it
        // repeats, so it breaks its lines where they were broken - the anchor rect is the box that
        // text was drawn in - and its window is then sized to the whole of the text instead of to
        // the part that fitted. Every other placement is a tooltip in its own right and lays itself
        // out.
        const bool overText = event.placement == FormPlacement::OverText;
        content().m_breakWidth = overText ? event.anchorRect.width() : 0.0f;
        const bool wrap = !overText or event.wordWrap;
        content().setWordWrap(wrap ? WordWrap::Yes : WordWrap::No);
        content().setHorizontalTextAnchor(
            overText ? event.textAnchor : HorizontalTextAnchor::None);
        invalidateAlign();
    }

    void TooltipForm::startAlphaAnimation()
    {
        const float currentValue = static_cast<float>(window().alpha());
        const float targetValue = static_cast<float>(visible()) * m_alpha;
        appContext().animator().start(this, AnimationSlots::FormAlpha, currentValue, targetValue,
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

    FloatRect TooltipForm::placementTargetBounds(const Control& target) const
    {
        // A glide carries the control there without moving the place, so the tooltip stands still.
        FloatRect bounds = target.boundsInForm();
        bounds.offset(-target.viewTravelRemaining());
        return bounds;
    }

}
