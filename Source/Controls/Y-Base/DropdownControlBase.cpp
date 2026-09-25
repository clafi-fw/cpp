module ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Controls.Button;

import ClaFi.Icons.Chevron;
import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // DropdownPart

    DropdownPart::DropdownPart(const CreateParams& params, DropdownControlBase& owner, ArrowPlacement placement)
        :
        ToolButton{ params,
            Interactivity::MouseOnly,
            // THE MARK IS THE STRIP'S ICON WHERE THE STRIP HAS NOTHING ELSE TO SHOW. IconOnly
            // centres it in the content box, which is the whole of the strip - carried as text it
            // would be placed by the text anchor instead, and a run narrower than the strip leaves
            // all of the slack on one side.
            //
            // A ribbon strip has the owner's text on it, so there the mark goes back into the run
            // as its last glyph and the two are centred as one line. The owner is complete this
            // far - its own constructor body is what creates the part - so its config answers.
            owner.textOnDropdown() ? ButtonViewMode::TextLabel : ButtonViewMode::IconOnly,
            IconSize{ DropdownControlBase::k_markBox },
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            placement == ArrowPlacement::Bottom ? Padding{ 4.0f } : Padding{ 0.0f }
        },
        m_owner{ owner }
    {
    }

    bool DropdownPart::enabled(const bool deep) const
    {
        if (!m_owner.dropdownActsAlone())
            return Control::enabled(deep);

        // Both depths are answered by the state, because it already carries what the strip
        // stands on as well as what it stands for - see getControlState. One question, one
        // answer, so that the strip never paints live and refuses the press.
        return state().enabled;
    }

    void DropdownPart::getControlState(GetStateEvent& event) const
    {
        ToolButton::getControlState(event);
        // A strip that is half of one command is as available as the control, and the walk that
        // reaches the owner is what says so.
        if (&event.control != this || !m_owner.dropdownActsAlone())
            return;

        // Otherwise answered here, and the walk ends here, so that it never reaches the owner and
        // reads the state of the command on its FACE. What the owner stands on is folded in
        // instead - and left alone is whatever already answered for the strip itself, which is
        // the command behind it where a dropdown action put one there.
        event.state.enabled = event.state.enabled && enabledAlone();
        event.stopPropagation();
    }

    void DropdownPart::getText(GetTextEvent& event) const
    {
        m_owner.getDropdownPartText(event);
    }

    void DropdownPart::paintIcon(PaintIconEvent& event)
    {
        ToolButton::paintIcon(event);
        m_owner.paintDropdownMark(event);
    }

    void DropdownPart::adjustPaint(AdjustPaintEvent& event)
    {
        ToolButton::adjustPaint(event);
        // The same division getControlState makes, on the side that paints. A paint event
        // takes its enabled factor from the one above it, which for the strip is the owner -
        // the command on the button's face, and a strip that acts alone does not stand for
        // that one. Its own factor is the whole answer here, because what the owner stands
        // on is already folded into it.
        if (m_owner.dropdownActsAlone())
            event.dropInheritedEnabled();
    }

    void DropdownPart::paintSurface(PaintEvent& event)
    {
        ToolButton::paintSurface(event);
        paintDivider(event);
    }

    /// Whether a strip that stands for a command of its own is available.
    ///
    /// @note THE OWNER IS STEPPED OVER, AND EVERYTHING ABOVE IT IS NOT. The owner is the one
    /// control on the chain whose answer is about the OTHER command - a Save with nothing to
    /// write over says nothing about the Save as behind the strip - so its answer is skipped and
    /// the climb resumes above it. A page turned off still turns this off.
    bool DropdownPart::enabledAlone() const
    {
        const Control* above = m_owner.parent();
        return !above || above->enabled(true);
    }

    void DropdownPart::paintDivider(PaintEvent& event) const
    {
        return;

        // The seam takes the theme's separator rule, resolved against whatever this half is
        // sitting on - the same way a Separator control resolves it. It then fades out once this
        // half is lit, because by then the surfaces already tell the two halves apart.
        const BakedColors& bakedColors = event.bakedColors();
        Hsl hsl = event.surfaceHsl();
        float visibility = bakedColors.element(UiElement::Divider).surface.applyTo(hsl, 1.0f,
            event.lightness());
        Color color = hsl.toColor();
        color.alpha = static_cast<ColorByte>(visibility * 255.0f * (1.0f - event.hoveredFactor()));
        if (!color.alpha)
            return;

        FloatRect bounds = event.controlBounds();
        float strokeWidth = event.scaledStrokeWidth(ThemeMetrics::border);
        if (m_owner.arrowPlacement() == ArrowPlacement::Bottom)
        {
            float inset = event.parentEvent()->padding().x;
            event.canvas().drawLine(
                { bounds.left + inset, bounds.top },
                { bounds.right - inset, bounds.top },
                color,
                strokeWidth
            );
            return;
        }
        float inset = event.parentEvent()->padding().y;
        event.canvas().drawLine(
            { bounds.left, bounds.top + inset },
            { bounds.left, bounds.bottom - inset },
            color,
            strokeWidth
        );
    }
    // DropdownControlBase

    void DropdownControlBase::setDropdownWidth(const float value)
    {
        if (m_config.dropdownWidth == value)
            return;

        m_config.dropdownWidth = value;
        invalidateFormAlign();
    }

    bool DropdownControlBase::canDropDown() const
    {
        const Control* part = secondaryPart();
        return !part || part->visible();
    }

    void DropdownControlBase::dropDown()
    {
        if (!canDropDown())
            return;

        Control* part = shownSecondaryPart();
        runDropdown(part ? *part : *this);
    }

    void DropdownControlBase::getMainText(GetTextEvent& event) const
    {
        RichControl::getText(event);
    }

    void DropdownControlBase::appendInTextMark(Text& text) const
    {
        // The flexible space pushes the mark to the far edge, where it reads as belonging to the
        // control rather than trailing the text.
        if (!text.plainText().empty())
            text << Space{ k_markSpacing };

        text << FlexSpace{};
        appendDropdownMark(text);
    }

    void DropdownControlBase::paintDropdownMark(PaintIconEvent& event) const
    {
        // The mark is lit while the popup is up: it is the one part of the control that says so
        // wherever the press landed, and a muted mark over an open popup reads as a control at
        // rest. A mark already taking the normal colour stays as it is.
        Color color = Color::blend(
            event.inkColor(dropdownMarkInk()),
            event.textRgb(InkGrade::Strongest),
            m_droppedDownFactor
        );
        const DropdownMarkTurn turn = dropdownMarkTurn();
        ChevronPainter::paint(
            event.canvas(),
            event.iconCenter(),
            color,
            std::lerp(turn.closed, turn.open, m_droppedDownFactor),
            event.scaleFactor()
        );
    }

    SecondaryEdge DropdownControlBase::secondaryEdge() const
    {
        if (m_config.placement == ArrowPlacement::Bottom)
            return SecondaryEdge::Bottom;

        return SecondaryEdge::Right;
    }

    void DropdownControlBase::placeSecondaryPart(ScaledDimensions childArea)
    {
        // The strip spans the whole of the cross axis rather than sitting at its own size on it:
        // it is one half of the control's face, and a gap along either side of it would read as
        // two controls that happen to touch.
        Control* part = shownSecondaryPart();
        ScaledDimensions size = part->dimensions();
        if (m_config.placement == ArrowPlacement::Bottom)
        {
            setControlPlacement(*part, { 0.0f, childArea.y - size.y }, { childArea.x, size.y });
            return;
        }
        setControlPlacement(*part, { childArea.x - size.x, 0.0f }, { size.x, childArea.y });
    }

    void DropdownControlBase::secondaryClicked(ClickEvent&)
    {
        runDropdown(*shownSecondaryPart());
    }

    void DropdownControlBase::adjustChildInset(AdjustChildInsetEvent& event) const
    {
        // Without a strip nothing needs to reach the edge, and the children stay where every other
        // control puts them.
        if (!shownSecondaryPart())
            return;

        // The strip runs out past the control's content padding, but not over its border: a
        // focused control thickens that stroke, and a strip laid across it would cover the focus
        // ring on the side it reaches. So the children begin at the widest border the control can
        // paint, and no further in.
        float border = event.scaledStrokeWidth(ThemeMetrics::focusedBorder);
        event.inset = { border, border };
    }

    void DropdownControlBase::getText(GetTextEvent& event) const
    {
        getMainText(event);
        // The mark rides in the control's own line only where it was asked to go. With a strip it
        // is the strip's, and a strip that is not shown is a control with nothing to drop - which
        // shows no mark at all, because one in the text would say there is something.
        if (m_config.placement != ArrowPlacement::InText)
            return;

        appendInTextMark(event.text);
    }

    void DropdownControlBase::calculateChildren(FormBase& form)
    {
        // Sizes the selection indicator and calculates the strip.
        SplitButtonBase::calculateChildren(form);
        Control* part = shownSecondaryPart();
        if (!part || m_config.placement == ArrowPlacement::Bottom)
            return;

        // A right hand strip is as wide as the caller asked for. Its own content would only size
        // it to the mark.
        float zoneWidth = form.scaler().scale(m_config.dropdownWidth);
        setControlPlacement(*part, part->topLeft(), { zoneWidth, part->height() });
    }

    void DropdownControlBase::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        SplitButtonBase::adjustChildMetrics(event);
        if (&event.control != secondaryPart())
            return;

        // The strip is part of this control's outline, so it rounds with it. Left to itself it
        // would take ToolButton's radius, which agrees only by coincidence.
        //
        // Concentric, not equal: the strip sits one inset inside the host, so matching the host's
        // radius would leave the two curves parallel along the straight edges and then spreading
        // apart around the corner. Taking the inset off keeps the gap between them the same the
        // whole way round, which is what the eye actually reads as even.
        event.metrics.radius = std::max(metrics().radius - strokeWidth(ThemeMetrics::focusedBorder), 0.0f);
    }

    void DropdownControlBase::adjustChildPaint(AdjustPaintEvent& event)
    {
        SplitButtonBase::adjustChildPaint(event);
        if (&event.control() != secondaryPart())
            return;

        // All below is for the dropdown strip. It owns the popup, so its hover, press and dropped
        // down states are all its own and none of them are inherited from the control.

        // The seam side is square, so the two halves meet flush and only the control's outer
        // corners stay rounded.
        if (m_config.placement == ArrowPlacement::Bottom)
        {
            event.setCornerRadius(Corner::TopLeft, 0.0f);
            event.setCornerRadius(Corner::TopRight, 0.0f);
        }
        else
        {
            event.setCornerRadius(Corner::TopLeft, 0.0f);
            event.setCornerRadius(Corner::BottomLeft, 0.0f);
        }
    }

    void DropdownControlBase::click(ClickEvent& event)
    {
        // A press on the strip is the base's business - it stops the click and calls
        // secondaryClicked(). What is left to decide here is a press anywhere else.
        if (!pressedSecondary(event) && dropOnPrimaryPress() && canDropDown())
        {
            event.stopPropagation();
            runDropdown(*this);
            return;
        }
        SplitButtonBase::click(event);
    }

    void DropdownControlBase::keyDown(KeyDownEvent& event)
    {
        // The two Windows conventions for dropping a list open. Alt+Down has to be claimed here,
        // ahead of FocusNavigator, or the arrow moves the focus instead.
        // A control with nothing to drop leaves both keys alone, so Alt+Down goes back to being
        // the arrow FocusNavigator reads.
        if (canDropDown() && (event.key == Keys::F4 || (event.key == Keys::Down && event.modifiers.alt)))
        {
            event.handled = true;
            dropDown();
            return;
        }
        SplitButtonBase::keyDown(event);
    }

    ButtonViewMode DropdownControlBase::mainViewMode(const Config& config)
    {
        // The text has moved to the strip, so the top half is left with the icon alone.
        if (config.textOnDropdown)
            return ButtonViewMode::IconOnly;

        return config.viewMode;
    }

    void DropdownControlBase::getDropdownPartText(GetTextEvent& event) const
    {
        // A STRIP THAT CARRIES NO TEXT STATES NONE. Its mark is its icon, and ButtonBase asks an
        // icon-only control for its text to fill an empty tooltip - a chevron is not what a
        // tooltip says.
        if (!m_config.textOnDropdown)
            return;

        // The mark is the last glyph of the run, so the label and the mark are centred together as
        // one line rather than the label being centred and the mark hung off its end. TextAlign
        // centres the line inside the layout; centring the layout inside the strip is the part's
        // horizontal text anchor, which is the other half of it.
        event.text << TextAlign::Center;
        getMainText(event);
        if (!event.text.plainText().empty())
            event.text << Space{ k_markSpacing };
        appendDropdownMark(event.text);
    }

    void DropdownControlBase::appendDropdownMark(Text& text) const
    {
        text << InTextIcon{
            k_markBox,
            k_markBox,
            k_markBox * 0.85f,
            [](PaintIconEvent& event)
            {
                // Reached through the tag rather than captured, because text layouts are cached.
                // A text run carries no state factors of its own, so everything the mark is drawn
                // from comes off the control it belongs to.
                event.tag().get<DropdownControlBase*>()->paintDropdownMark(event);
            },
            Tag{ this }
        };
    }

    void DropdownControlBase::runDropdown(Control& initiator)
    {
        // The mark answers for the control as a whole, so it turns for every route into the popup.
        // The popup itself cannot be asked which those are: it is owned by the control the press
        // landed on - the strip for a press on the strip, this control for a press on its face -
        // and a mark reading that would turn for one route and stay put for the other.
        setDroppedDown(true);
        showDropdown(initiator);
        setDroppedDown(false);
    }

    void DropdownControlBase::setDroppedDown(const bool value)
    {
        if (m_droppedDown == value)
            return;

        m_droppedDown = value;
        float target = value ? 1.0f : 0.0f;
        animate(AnimationSlots::dropdownMark, m_droppedDownFactor, target,
            [this](AnimateParams& params) {
                m_droppedDownFactor = params.value;
                markHolder().invalidate();
            });
    }

    const Control& DropdownControlBase::markHolder() const
    {
        const Control* part = shownSecondaryPart();
        return part ? *part : *this;
    }

}
