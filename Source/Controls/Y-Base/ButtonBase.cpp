module ClaFi.Controls.Base.ButtonBase;

import ClaFi.Icons.CheckMark;
import ClaFi.Icons.DialogIcon;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Context.PaintIconEvent;

namespace ClaFi::Controls
{

    class SelectionIndicator : public Control
    {
    public:
        using Control::Control;
        Interactivity interactivity() const override { return Interactivity::MouseOnly; }
        // The indicator takes the depth its host does not. A check box or radio button is a row of
        // text that happens to be clickable, and pushing the whole row moves a label that has no
        // business moving; the mark is the part that reads as a thing being pressed. It carries no
        // text of its own, so the depth costs nothing in rasterization.
        [[nodiscard]] bool allowZAnimation() const override { return true; }
    protected:
        void doubleClick(DoubleClickEvent& event) override { event.stopPropagation(); }
    };

    // ButtonBase

    ButtonBase::~ButtonBase()
    {
        releaseChildren();
    }

    void ButtonBase::setIndicatorVisibility(const IndicatorVisibility value)
    {
        if (value == m_indicatorVisibility)
            return;

        m_indicatorVisibility = value;
        applyIndicatorVisibility();
    }

    void ButtonBase::setIndicatorStyle(const IndicatorStyle value)
    {
        if (value == m_indicatorStyle)
            return;

        m_indicatorStyle = value;
    }

    void ButtonBase::setShowSelectionOnSurface(const ShowSelectionOnSurface value)
    {
        if (m_showSelectionOnSurface == value)
            return;

        m_showSelectionOnSurface = value;
        if (selectedFactor())
            invalidate();
    }

    void ButtonBase::setShowSurfaceAtRest(const ShowSurfaceAtRest value)
    {
        if (m_showSurfaceAtRest == value)
            return;

        m_showSurfaceAtRest = value;
        // Unconditional, unlike the selected surface above: this is what the button looks like
        // with nothing happening to it, so there is no state it could be in that leaves the answer
        // off the screen.
        invalidate();
    }

    void ButtonBase::setIndicatorPlacement(const IndicatorPlacement value)
    {
        if (value == m_indicatorPlacement)
            return;

        m_indicatorPlacement = value;
        invalidateFormAlign();
    }

    void ButtonBase::setIconSize(IconSize value)
    {
        if (value == m_iconSize)
            return;

        m_iconSize = value;
        invalidateFormAlign();
    }

    void ButtonBase::setViewMode(ButtonViewMode value)
    {
        if (value == m_viewMode)
            return;

        m_viewMode = value;
        invalidateFormAlign();
    }

    void ButtonBase::setOpensWindow(OpensWindow value)
    {
        if (value == m_opensWindow)
            return;

        m_opensWindow = value;
        invalidateFormAlign();
    }

    ControlSpan ButtonBase::controls()
    {
        return m_childCount ? ControlSpan{ m_children.data(), m_childCount } : RichControl::controls();
    }

    MinSize ButtonBase::indicatorSize(const AppTheme& theme) const
    {
        return theme.metrics.checkMark.minSize;
    }

    void ButtonBase::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        if (&event.control != m_indicator)
        {
            RichControl::adjustChildMetrics(event);
            return;
        }

        event.metrics = event.themeMetrics().checkMark;
        switch (m_indicatorStyle)
        {
            case IndicatorStyle::Check:
                break;
            case IndicatorStyle::Radio:
                event.metrics.radius = event.metrics.minSize.y / 2.0f;
                break;
            case IndicatorStyle::Custom:
                break;
        }
    }

    void ButtonBase::adjustChildPaint(AdjustPaintEvent& event)
    {
        RichControl::adjustChildPaint(event);
        if (&event.control() != m_indicator)
            return;

        // All below is for the indicator

        // The set arrives on the event by copy, so the on state is put into it here rather than
        // stated in the theme: an indicator's active rule IS the accent, and the accent is where
        // the framework states it once for the mark, the focus ring and the tab indicator alike.
        // See UiElement::InactiveIndicator.
        event.setColorRules(UiElement::InactiveIndicator);
        event.colorRules().active = event.bakedColors().rule(UiElement::Accent);

        event.setParentSelectedAmount(1.0f);
        event.setParentPressedAmount(1.0f);

        //
        event.setParentHoverAmount(1.0f);

        // The whole of the parent's, unlike the hover amount above. On a check box or radio button
        // the mark is not a target in its own right - the pointer is over the row and injectCtrl
        // forwards the click to the host - so its own factors barely move, and a share of them
        // would leave the depth almost never seen. Colour is half inherited because a mark has to
        // stay legible against a row that is only lit; movement has no such reason to hold back.
        //
        // A host whose indicator is separately clickable - a list item, where the mark toggles
        // selection on its own account - overrides this to a share, so the mark still answers to
        // being hovered by itself.
        event.setParentZAmount(1.0f);

        // The indicator is given no Focusable role. That role gates focus painting, which an
        // indicator never has, and a press is expressed by scaling the whole control about one
        // origin - see PaintEvent::pressScale() - rather than per element.

        if (m_indicatorVisibility == IndicatorVisibility::Hover)
        {
            event.dropSurfaceAtRest();
            //event.setParentHoverAmount(0.0f);
        }
    }

    void ButtonBase::paintChildSurface(PaintEvent& event)
    {
        if (&event.control() != m_indicator)
        {
            RichControl::paintChildSurface(event);
            return;
        }

        switch (m_indicatorStyle)
        {
            case IndicatorStyle::Check:
            {
                RichControl::paintChildSurface(event);

                // The mark draws as geometry rather than staged pixels. Staging is free on the
                // CPU backend, which hands back a window onto the back buffer, but on a GPU one
                // it is a scratch buffer, a CPU rasterization and a texture upload for every
                // mark on screen, every frame - which is what a list of selected items turns
                // into. As a path it goes to the GPU as a filled outline and to the CPU
                // rasterizer as it did before.
                //
                // The factors handed over are this button's, not the ones on the event. The
                // event belongs to the indicator child, which carries none of the selection,
                // and the mark is drawn by how far pressed and selected have travelled.
                PaintIconEvent iconEvent{
                    event.controlContext(),
                    event.controlBounds(),
                    event.control().tag(),
                    event.disabledAmount()
                };
                Icons::CheckMark::paint(iconEvent, factors());
                break;
            }
            case IndicatorStyle::Radio:
            {
                RichControl::paintChildSurface(event);
                // We already adjusted original surface radius to make it look like a circle,
                // so surface and border are already perfect as they are so now it acts as an
                // outer ring. Now we only need to paint the hole in the "donut".
                //
                // TODO: at 125% this is not quite right - it kind of works, but the indicator
                // bounds need to be snapped into the pixel grid, and the same holds for the
                // check mark. Work out where the snapping belongs.
                if (const float selK = selectedFactor())
                {
                    // hole in the "donut"
                    // The radius follows selection alone. A press is expressed by scaling the
                    // whole control, so it must not take a pixel off this as well.
                    event.canvas().fillCircle(
                        event.center(),
                        selK * event.scaleF(4.0f),
                        event.textRgb(InkGrade::Strongest));
                }
                break;
            }
            case IndicatorStyle::Custom:
                break;
        }
    }

    void ButtonBase::adjustPaint(AdjustPaintEvent& event)
    {
        RichControl::adjustPaint(event);
        // Both told to the event rather than taken out of the colour rules: Button assigns its
        // whole rule set after this runs, and a cleared rule would come straight back with it.
        if (m_showSurfaceAtRest == ShowSurfaceAtRest::No)
            event.dropSurfaceAtRest();
        if (m_showSelectionOnSurface == ShowSelectionOnSurface::No)
            event.dropSelectedSurface();
    }

    void ButtonBase::paintSurface(PaintEvent& event)
    {
        RichControl::paintSurface(event);
        paintIconLayer(event);
    }

    void ButtonBase::paintIconLayer(PaintEvent& event)
    {
        if (!showsIcon())
            return;

        // No press inset or fade here: the whole control, icon included, is scaled as one piece by
        // PaintEvent::pressScale(), about the origin pressOrigin() picks.
        FloatRect rect = iconRect(event);
        PaintIconEvent event2{
            event.controlContext(),
            rect,
            event.control().tag(),
            event.disabledAmount()
        };
        paintIcon(event2);
    }

    void ButtonBase::paintText(PaintEvent& event)
    {
        if (m_viewMode != ButtonViewMode::IconOnly)
            RichControl::paintText(event);
    }

    FloatRect ButtonBase::iconRect(const PaintEvent& event) const
    {
        FloatRect result = event.controlBounds().toFloat();
        result.inflate(-event.padding());
        float contentBottom = result.bottom;
        adjustForIndicator(event.spacing(), result);
        FloatPoint contentBounds = result.dimensions();
        ScaledDimensions scaledSize = event.scaleF(m_iconSize);
        result.setDimensions(scaledSize);
        switch (m_viewMode)
        {
            case ButtonViewMode::LeftIcon:
                result.offset(0, (contentBounds.y - result.height()) / 2.0f);
                break;
            case ButtonViewMode::TopCenterIcon:
                result.offset((contentBounds.x - scaledSize.x) / 2.0f, 0.0f);
                break;
            case ButtonViewMode::BottomIcon:
                result.offset(0.0f, contentBottom - result.bottom);
                break;
            case ButtonViewMode::IconOnly:
                // Center
                result.offset(
                    (contentBounds.x - scaledSize.x) / 2.0f,
                    (contentBounds.y - scaledSize.y) / 2.0f
                );
                break;
            case ButtonViewMode::TextLabel:
            case ButtonViewMode::TopLeftIcon:
                break;
        }
        return result;
    }

    void ButtonBase::adjustTextRect(AdjustTextRectEvent& event) const
    {
        adjustForIndicator(event.spacing, event.textBounds);
        switch (m_viewMode)
        {
            case ButtonViewMode::LeftIcon:
                event.textBounds.left += event.scale(m_iconSize.x);
                if (event.textBounds.width() > 0)
                    event.textBounds.left += event.spacing.x;
                break;
            case ButtonViewMode::TopLeftIcon:
            case ButtonViewMode::TopCenterIcon:
                event.textBounds.top += event.scale(m_iconSize.y);
                if (event.textBounds.height() > 0)
                    event.textBounds.top += event.spacing.y;
                break;
            case ButtonViewMode::BottomIcon:
                event.textBounds.bottom -= event.scale(m_iconSize.y);
                if (event.textBounds.height() > 0)
                    event.textBounds.bottom -= event.spacing.y;
                break;
            case ButtonViewMode::TextLabel:
            case ButtonViewMode::IconOnly:
                break;
        }
        RichControl::adjustTextRect(event);
    }

    // THE MARK STANDS AT THE LINE'S RIGHT-HAND END, which is what the flex space ahead of it
    // buys: a column of buttons stretched to one width lands every mark on the same edge, the
    // way a menu's keys do. A button measured to its own caption leaves the space nothing to
    // take, and the mark follows the words. See Controls-Base
    void ButtonBase::getText(GetTextEvent& event) const
    {
        RichControl::getText(event);

        if (m_opensWindow == OpensWindow::No)
            return;

        const InTextIcon mark{ k_openWindowMark, [](PaintIconEvent& iconEvent) {
            Icons::DialogIcon::paint(iconEvent);
        } };
        event.text << FlexSpace{ k_openWindowMarkGap } << mark;
    }

    void ButtonBase::getTooltip(GetTooltipEvent& event)
    {
        RichControl::getTooltip(event);
        if (event.text.empty() && m_viewMode == ButtonViewMode::IconOnly)
        {
            GetTextEvent textEvent(event.formContext(), *this, event.text, EventPhase::Paint);
            getText(textEvent);
            // The tooltip reads the buffer rather than the answer, so a named text is put in it.
            textEvent.materialise();
        }
    }

    // AN ICON-ONLY BUTTON DOES NOT LAY ITS TEXT OUT. Its words are what the tooltip says - see
    // getTooltip - and paintText draws none of them, so they take no space in either pass.
    // calculateContent answers for that mode with the icon alone, and the align pass re-measures a
    // wrapping text against the box the control was granted and takes that answer as its content:
    // a button measuring words it never shows comes out as tall as they are wrapped in its own
    // width - three lines for a three-word tooltip - and spills out of the bar it stands in.
    CalculatedDimensions ButtonBase::measureText(AlignEvent& event, ScaledDimensions asked,
        const Text& text)
    {
        if (m_viewMode == ButtonViewMode::IconOnly)
            return {};

        // A PICTURE STANDING OVER THE WORDS SETS THE WIDTH, and the words wrap under it. Asked
        // against the whole box instead, a caption longer than the picture comes out on ONE LINE
        // and the button is as wide as its name - a button wider than the thing it is built
        // around, and every name a different width. In a lane of equal shares that is worse than
        // untidy: the widest name in the stack sizes every share in it, so one long caption drops
        // a whole column out of the grid. See Controls-Base
        const float iconWidth = event.scale(m_iconSize).x;
        if (iconWidth > 0.0f
            && (hasTopIcon(m_viewMode) || m_viewMode == ButtonViewMode::BottomIcon))
        {
            asked.x = std::min(asked.x, iconWidth);
        }

        return RichControl::measureText(event, asked, text);
    }

    void ButtonBase::calculateChildren(FormBase& form)
    {
        if (m_indicator)
        {
            float sz = form.scaler().scale(form.themeMetrics().checkMark.minSize.x);
            setControlPlacement(*m_indicator, { 0, 0 }, { sz, sz });
        }
    }

    ScaledDimensions ButtonBase::calculateContent(AlignEvent& event)
    {
        ScaledDimensions scaledIconSize;
        ScaledDimensions zeroSize{ 0, 0 };
        scaledIconSize = m_viewMode == ButtonViewMode::TextLabel ? zeroSize : event.scale(m_iconSize);
        // Base text
        ScaledDimensions result =
            m_viewMode == ButtonViewMode::IconOnly ? scaledIconSize : RichControl::calculateContent(event);

        // Icon
        switch (m_viewMode)
        {
            case ButtonViewMode::LeftIcon:
            {
                if (result.x)
                    result.x += event.spacing.x;

                result.x += scaledIconSize.x;
                result.y = std::max(result.y, scaledIconSize.y);
                break;
            }
            case ButtonViewMode::TopLeftIcon:
            case ButtonViewMode::TopCenterIcon:
            case ButtonViewMode::BottomIcon:
            {
                if (result.y)
                    result.y += event.spacing.y;

                result.y += scaledIconSize.y;
                result.x = std::max(result.x, scaledIconSize.x);
                break;
            }
            case ButtonViewMode::TextLabel:
            case ButtonViewMode::IconOnly:
                break;
        }

        // Indicator
        if (m_indicatorVisibility != IndicatorVisibility::None)
        {
            ScaledDimensions indicatorSize = m_indicator->width();
            switch (m_indicatorPlacement)
            {
                case IndicatorPlacement::LeftCenter:
                case IndicatorPlacement::TopLeft:
                    if (result.x)
                        result.x += event.spacing.x;

                    result.x += indicatorSize.x;
                    break;
                case IndicatorPlacement::TopLeftIn:
                    result.x = std::max(result.x, indicatorSize.x);
                    break;
            }
            result.y = std::max(result.y, indicatorSize.y);
        }
        return result;
    }

    void ButtonBase::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& dimensions)
    {
        if (m_indicator && m_indicator->visible())
        {
            switch (m_indicatorPlacement)
            {
                case IndicatorPlacement::LeftCenter:
                    // The content box does not begin at {0, 0} when the control separates its child
                    // padding from its content padding, so the indicator follows the given position
                    // rather than the placeholder placement calculateChildren() left it at.
                    setControlPlacement(*m_indicator, { position.x, m_indicator->top() }, m_indicator->dimensions());
                    placeControlToVerticalCenter(*m_indicator, position.y, position.y + dimensions.y);
                    position.x += m_indicator->width();
                    if (dimensions.x)
                        position.x += event.spacing.x;

                    break;
                case IndicatorPlacement::TopLeft:
                case IndicatorPlacement::TopLeftIn:
                    break;
            }
        }
        RichControl::alignContent(event, position, dimensions);
    }

    void ButtonBase::pressDown(PressDownEvent& event)
    {
        injectCtrl(event);
        RichControl::pressDown(event);
    }

    void ButtonBase::pressUp(PressUpEvent& event)
    {
        // TODO: make PressUpEvent a ClickEventBase so this can go through injectCtrl
        if (&event.control == m_indicator)
        {
            //affects selection in BaseListView
            event.modifiers.ctrl = true;
        }
        RichControl::pressUp(event);
    }

    void ButtonBase::click(ClickEvent& event)
    {
        injectCtrl(event);
        RichControl::click(event);
    }

    void ButtonBase::nestedControlFocusing(FocusEvent& event)
    {
        injectCtrl(event);
        RichControl::nestedControlFocusing(event);
    }

    Control& ButtonBase::addChild(ControlPtr&& control)
    {
        if (!control)
            unreachable("ButtonBase::addChild was given an empty control");

        if (m_childCount == k_maxChildren)
            unreachable("ButtonBase has room for one child besides the selection indicator");

        Control& result = *control;
        m_children[m_childCount] = std::move(control);
        ++m_childCount;
        return result;
    }

    Control& ButtonBase::insertIndicator()
    {
        if (m_childCount == k_maxChildren)
            unreachable("ButtonBase has room for one child besides the selection indicator");

        // A derived class may have registered its child before the indicator was ever asked for,
        // so make room at the front rather than appending.
        for (std::size_t i = m_childCount; i != 0; --i)
            m_children[i] = std::move(m_children[i - 1]);

        ++m_childCount;
        m_children[0] = std::make_unique<SelectionIndicator>(CreateParams{ *this });
        return *m_children[0];
    }

    void ButtonBase::applyIndicatorVisibility()
    {
        if (m_indicatorVisibility == IndicatorVisibility::None)
        {
            if (m_indicator)
                m_indicator->hide();

            return;
        }
        if (!m_indicator)
            m_indicator = &insertIndicator();

        if (!m_indicator->visible())
            m_indicator->show();
        else
            m_indicator->invalidate();
    }

    void ButtonBase::injectCtrl(ClickEventBase& event)
    {
        if (!m_indicator)
            return;

        // A press on the indicator means the same as a press with Ctrl held: take this item
        // in or out of the selection and leave the rest of it alone.
        //
        // The event does not always say so by itself. The indicator is MouseOnly, so the focus
        // walk in setMouseDown climbs past it to the button before it builds the FocusEvent,
        // and that event arrives here already named after the button. What the mouse is over
        // is what still tells the two apart, and a hidden indicator cannot be hovered, so the
        // test needs no separate visibility check.
        if (event.control != m_indicator && !m_indicator->isHovered())
            return;

        // affects multi-selection behavior (StackView)
        event.modifiers.ctrl = true;
        // actual control to select
        event.control = this;
    }

    void ButtonBase::adjustForIndicator(ScaledSpacing spacing, FloatRect& rect) const
    {
        switch (m_indicatorVisibility)
        {
            case IndicatorVisibility::None:
                break;
            default:
                switch (m_indicatorPlacement)
                {
                    case IndicatorPlacement::LeftCenter:
                    case IndicatorPlacement::TopLeft:
                        rect.left += m_indicator->width();
                        if (rect.width() > 0)
                            rect.left += spacing.x;
                        break;
                    case IndicatorPlacement::TopLeftIn:
                        break;
                }
                break;
        }
    }

} // of namespace ClaFi
