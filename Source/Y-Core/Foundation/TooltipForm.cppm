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
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.System.Animation;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export class TooltipWindow;
    export class TooltipForm;
    export class OverTextForm;

    // Holding its own layout rather than sharing the engine's cache. A tooltip over a slider being
    // dragged says something different on every frame of the drag, and a text nothing says twice
    // is a cache entry nothing will ever hit again - see WithTextLayout.
    class TooltipLabel : public WithTextLayout<FormControlBase>
    {
        friend TooltipWindow;
    public:
        using WithTextLayout::WithTextLayout;
    protected:
        [[nodiscard]] float lineBreakWidth() const override { return m_breakWidth; }
        // Held to Tooltip::k_maxLines, and to k_lineWidth unless it repeats lines broken elsewhere.
        ScaledSize measureText(AlignEvent&, ScaledSize maxDimensions, const Text&) override;
        ScaledSize calculateContent(AlignEvent& event) override { return FormControlBase::calculateContent(event); }
    private:
        // The width the words this tooltip repeats were broken at, and zero for a tooltip repeating
        // none - see OverTextForm::setAnswer.
        float m_breakWidth{ 0.0f };
    };

    // The window a tooltip is shown in. A form builds two. See Control-Foundation#tooltip
    export class TooltipWindow : public Form<TooltipLabel>
    {
        friend Tooltip;
    public:
        explicit TooltipWindow(FormBase& ownerForm);
        ~TooltipWindow() override;
        [[nodiscard]] Control* control() { return m_control; }
        void setControl(Control* control);
        void startAlphaAnimation();
        [[nodiscard]] bool stillVisible() const;
        [[nodiscard]] bool hideOnUserInput() const { return m_hideOnUserInput; }
    protected:
        void setBreakWidth(const float value) { content().m_breakWidth = value; }
        void setHideOnUserInput(const bool value) { m_hideOnUserInput = value; }
        void updateVisibility() override;
        // The control the tooltip is about, which the window goes along with as a popup does.
        [[nodiscard]] const Control* placementTarget() const override { return m_control; }
        // Where the control comes to rest, which is where a tooltip about it is placed.
        [[nodiscard]] FloatRect placementTargetBounds(const Control&) const override;
    private:
        static constexpr ColorByte m_alpha = 255; // a bit of transparency looks good only when bg is blurred
    private:
        bool m_hideOnUserInput{ true };
        Control* m_control{ nullptr };
    };

    // The window a caption and a body are shown in, and the only one the tooltip event reaches.
    export class TooltipForm : public TooltipWindow
    {
        friend Tooltip;
    public:
        explicit TooltipForm(FormBase& ownerForm);
        using TooltipWindow::setControl;
        [[nodiscard]] const TooltipHeader& header() const { return m_header; }
        [[nodiscard]] const Text& body() const { return m_body; }
        void setControl(Control&, const AdjustTooltipEvent&); // the answer becomes the label's text
    private:
        [[nodiscard]] Text composeText(const AdjustTooltipEvent&) const;
        [[nodiscard]] Padding tooltipPadding() const;
    private:
        TooltipHeader m_header{};
        Text m_body{};
    };

    // The window a control's cut text is repeated in, over it. See Control-Foundation#tooltip
    export class OverTextForm : public TooltipWindow
    {
        friend Tooltip;
    public:
        explicit OverTextForm(FormBase& ownerForm);
        void setAnswer(Control&, const Text& text, const FloatRect& anchorRect,
            const TextFormat& format, HorizontalTextAnchor, bool wordWrap);
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

    // TooltipWindow

    TooltipWindow::TooltipWindow(FormBase& ownerForm)
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
            ClickTarget::Parent,
            UiElement::Tooltip
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

    TooltipWindow::~TooltipWindow()
    {
        appContext().animator().stop(this);
    }

    void TooltipWindow::setControl(Control* control)
    {
        m_control = control;
        if (!control)
        {
            // The window is about nothing from here on. The words stay in the label, since the
            // pixels they were laid out for are on screen for the whole of the fade.
            hide();
            return;
        }
        content().invalidate();
    }

    void TooltipWindow::startAlphaAnimation()
    {
        const float currentValue = static_cast<float>(window().alpha());
        const float targetValue = static_cast<float>(visible()) * m_alpha;
        appContext().animator().start(this, AnimationSlots::FormAlpha, currentValue, targetValue,
            [this](const AnimateParams& params) {
                window().setAlpha(static_cast<ColorByte>(params.value));
            });
    }

    bool TooltipWindow::stillVisible() const
    {
        return window().alpha();
    }

    void TooltipWindow::updateVisibility()
    {
        // nope! Form::updateVisibility();
        if (visible())
            updatePlacement();
        startAlphaAnimation();
        window().show();
    }

    FloatRect TooltipWindow::placementTargetBounds(const Control& target) const
    {
        // A glide carries the control there without moving the place, so the tooltip stands still.
        FloatRect bounds = target.boundsInForm();
        bounds.offset(-target.viewTravelRemaining());
        return bounds;
    }

    // TooltipForm

    TooltipForm::TooltipForm(FormBase& ownerForm)
        :
        TooltipWindow{ ownerForm }
    {
    }

    void TooltipForm::setControl(Control& control, const AdjustTooltipEvent& event)
    {
        TooltipWindow::setControl(&control);
        // Taken here too - a rect stated equal to the last one records nothing.
        rememberPlacementTarget();
        m_header = event.header;
        m_body = event.text;
        content().text() = composeText(event);
        content().setPadding(tooltipPadding());
        content().setWordWrap(WordWrap::Yes);
        setPlacement(event.placement);
        setHideOnUserInput(event.hideOnUserInput);
        invalidateAlign();
    }

    // **A tooltip says a caption and a body, and either may stand alone.** The caption and its key
    // are one bold line, the body follows after a short gap - see Control-Foundation#tooltip.
    Text TooltipForm::composeText(const AdjustTooltipEvent& event) const
    {
        if (event.header.empty())
            return event.text;

        Text headerLine{ event.header.caption };
        if (!event.header.shortcut.empty())
        {
            if (!event.header.caption.empty())
                headerLine << L" (";
            headerLine << event.header.shortcut;
            if (!event.header.caption.empty())
                headerLine << L")";
        }

        Text answer;
        answer << BoldText{ std::move(headerLine) };
        if (!event.text.empty())
        {
            answer << L"\n" << VSpace{ Tooltip::k_partSpacing } << L"\n" << event.text;
        }
        return answer;
    }

    // **The style's room, or the theme's default where the style states none.** A theme written
    // before the Tooltip style stated a padding still gets a window with room in it, and one that
    // states its own keeps it.
    Padding TooltipForm::tooltipPadding() const
    {
        const ControlStyle* style = theme().styles.of(L"Tooltip");
        if (style && style->metrics.padding)
            return *style->metrics.padding;
        return ThemeMetrics::TooltipPadding;
    }

    // OverTextForm

    OverTextForm::OverTextForm(FormBase& ownerForm)
        :
        TooltipWindow{ ownerForm }
    {
    }

    // **The control's own lines, broken where the control broke them.** The anchor rect is the box
    // that text was drawn in, so the window is sized to the whole of the text instead of to the part
    // that fitted, and its first glyph lands where the drawn text's did - see
    // FormControlBase::textOrigin.
    void OverTextForm::setAnswer(Control& control, const Text& text, const FloatRect& anchorRect,
        const TextFormat& format, const HorizontalTextAnchor anchor, const bool wordWrap)
    {
        setControl(&control);
        // Taken here too - a rect stated equal to the last one records nothing.
        rememberPlacementTarget();
        content().text() = text;
        content().setTextFormat(format);
        setBreakWidth(anchorRect.width());
        content().setWordWrap(wordWrap ? WordWrap::Yes : WordWrap::No);
        content().setHorizontalTextAnchor(anchor);
        setPlacement(FormPlacement::OverText);
        invalidateAlign();
    }

}
