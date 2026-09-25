module ClaFi.Controls.Base.SliderBase;

import ClaFi.Controls.Panel;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // SliderChangeEvent

    SliderChangeEvent::SliderChangeEvent(const SliderBase& slider, float newPosition, float previousPosition)
        :
        slider{ slider },
        newPosition{ newPosition },
        previousPosition{ previousPosition }
    {
    }

#pragma region SliderBase::ChildItem

    SliderBase::ChildItem::ChildItem(const CreateParams& params)
        :
        Control(params)
    {
    }

    ScaledDimensions SliderBase::ChildItem::calculateContent(AlignEvent& event)
    {
        float sz = event.scale(parent()->buttonSize(event.theme()));
        return { sz, sz };
    }

    void SliderBase::ChildItem::adjustPaint(AdjustPaintEvent& event)
    {
        Control::adjustPaint(event);
        event.setParentHoverAmount(0.33f);
    }

#pragma endregion

#pragma region SliderBase::Button

    SliderBase::ScrollButton::ScrollButton(const CreateParams& params, ScrollDirection direction)
        :
        ChildItem{ params },
        m_direction{ direction },
        m_clickRepeater{ [this](RepeatEvent& event) { repeat(event); } }
    {
    }

    void SliderBase::ScrollButton::startAutoScroll(float speed)
    {
        m_autoScrolling = true;
        m_autoScrollSpeed = speed;
        m_clickRepeater.startOrRepeatNow(k_autoScrollInterval);
    }

    void SliderBase::ScrollButton::stopAutoScroll()
    {
        m_autoScrolling = false;
        m_clickRepeater.stopAndReset();
    }

    void SliderBase::ScrollButton::adjustPaint(AdjustPaintEvent& event)
    {
        ChildItem::adjustPaint(event);
        parent()->adjustButtonPaint(event);
    }

    void SliderBase::ScrollButton::click(ClickEvent&)
    {
        if (Input::device() == InputDevice::Keyboard)
            doIt(); // mouse clicks handled in pressDown
    }

    void SliderBase::ScrollButton::pressDown(PressDownEvent& params)
    {
        m_autoScrolling = false;
        m_clickRepeater.start();
        params.stopPropagation();
    }

    void SliderBase::ScrollButton::pressUp(PressUpEvent&)
    {
        m_clickRepeater.stop();
    }

    void SliderBase::ScrollButton::paintSurface(PaintEvent& event)
    {
        parent()->paintButton(*this, event);
    }

    // A step is a step, so a click and a held button both move by one. Only a drag near an edge
    // asks for a speed, and that goes to autoScroll.
    void SliderBase::ScrollButton::doIt()
    {
        float steps = m_direction == ScrollDirection::ToBegin ? -1.0f : +1.0f;
        if (parent()->glidesToPosition())
            parent()->animatePositionBySteps(steps);
        else
            parent()->offsetPositionBySteps(steps);
    }

    void SliderBase::ScrollButton::repeat(RepeatEvent& event)
    {
        if (m_autoScrolling)
        {
            autoScroll(event.elapsedSeconds);
        }
        else
            doIt();
    }

    // A fixed rate, covered over however long the repeat actually took. The interval asked of the
    // timer is what a machine keeping up delivers; one that does not delivers fewer, longer moves,
    // and the content travels at the same speed either way.
    void SliderBase::ScrollButton::autoScroll(float elapsedSeconds)
    {
        float sign = m_direction == ScrollDirection::ToBegin ? -1.0f : 1.0f;
        float rate = scaler().scale(k_autoScrollUnitsPerSecond) * m_autoScrollSpeed;
        parent()->offsetPosition(sign * rate * elapsedSeconds);
    }
#pragma endregion

#pragma region SliderBase::Thumb

    void SliderBase::Thumb::getControlState(GetStateEvent& event) const
    {
        ChildItem::getControlState(event);
        // TODO: add the form environment to the event, so the scaler does not have to be
        // reached through the control.
        if (event.state.enabled)
            if (height() < scaler().scaled4)
                event.state.enabled = false;
    }

    void SliderBase::Thumb::adjustPaint(AdjustPaintEvent& event)
    {
        ChildItem::adjustPaint(event);
        parent()->adjustThumbPaint(event);
    }

    void SliderBase::Thumb::paintSurface(PaintEvent& params)
    {
        parent()->paintThumb(params);
    }

    void SliderBase::Thumb::getTooltip(GetTooltipEvent& event)
    {
        parent()->getThumbTooltip(event);
    }

    void SliderBase::Thumb::pressDown(PressDownEvent& event)
    {
        event.stopPropagation();
        // The pointer owns the position from here, and it takes it over at what is on screen.
        parent()->m_thumbPressed = true;
        parent()->stopPositionAnimation();
        if (parent()->m_axis == ScrollAxis::Vertical)
            m_startTop = top();
        else
            m_startTop = left();

        parent()->thumbPressDown();
    }

    void SliderBase::Thumb::drag(DragEvent& dp)
    {
        float delta;
        float myHeight;
        float areaHeight;
        float areaTop;
        float areaBottom;

        FloatRect area = parent()->bodyRect();
        area.inflate(-scaler().scaled2);

        bool reversePos = false;
        if (parent()->m_axis == ScrollAxis::Vertical)
        {
            delta = dp.currentPos().y - dp.startPos().y;
            myHeight = height();
            areaHeight = area.height();
            areaTop = area.top;
            areaBottom = area.bottom;
            reversePos = parent()->verticalDirection() == ScrollDirection::ToBegin;
        }
        else
        {
            delta = dp.currentPos().x - dp.startPos().x;
            myHeight = width();
            areaHeight = area.width();
            areaTop = area.left;
            areaBottom = area.right;
        }

        float newTop = m_startTop + delta;
        newTop = std::min(std::max(newTop, areaTop), areaBottom - myHeight);

        if (parent()->m_axis == ScrollAxis::Vertical)
            setTop(newTop);
        else
            setLeft(newTop);

        update();
        float newScrollPos = areaHeight - myHeight;
        if (newScrollPos)
        {
            // THE POSITION IS READ OFF WHERE THE THUMB HAS BEEN PUT, not off where it stood when
            // this step began. One drag event carries the whole of a movement, however far the
            // pointer went since the last one, and the thumb is not laid out from the position
            // while it is held - controlFeedBack leaves it alone - so the old place is exactly
            // one event behind the pointer, and a fast drag makes that a whole flick.
            newScrollPos = (newTop - areaTop) / newScrollPos;
            ScrollInfo si = parent()->controlScrollInfo();
            if (reversePos)
                newScrollPos = 1.0f - newScrollPos;

            newScrollPos = si.maxPos() * newScrollPos;
            parent()->setPosition(std::min(
                newScrollPos,
                si.maxPos()
            ));
        }
        parent()->invalidate();
        dp.lockHoveredControl();
        dp.stopPropagation();
    }

#pragma endregion

    // SliderBase

    void SliderBase::setScrollAxis(ScrollAxis value)
    {
        if (m_axis == value)
            return;

        m_axis = value;
        // The buttons land in the slots barSlotFor names for the axis now in force. A begin button
        // at the left goes to the top when the top is the begin end, and to the bottom when it is
        // not; the end button takes the other, so one rotation moves both.
        switch (verticalDirection())
        {
            case ScrollDirection::ToEnd:
                swapBarsClockWise();
                break;
            case ScrollDirection::ToBegin:
                swapBarsCounterClockWise();
                break;
        }
    }

    void SliderBase::setScrollButtons(ScrollButtons value)
    {
        m_beginButton.setVisible(value == ScrollButtons::Yes);
        m_endButton.setVisible(value == ScrollButtons::Yes);
    }

    // The position itself, now. A glide is a bid for where the position is going to be, so
    // naming the position outright ends it.
    void SliderBase::setPosition(float value, bool triggerChange)
    {
        stopPositionAnimation();
        applyPosition(std::clamp(value, 0.0f, controlScrollInfo().maxPos()), triggerChange);
        m_positionTarget = m_position;
    }

    // The same destination reached over AnimationSlots::scroll. Each tick's value is applied as it
    // comes: a position sitting outside the range is what a glide back into the range starts from,
    // and clamping the way in would cover it in one step.
    void SliderBase::animatePosition(float value)
    {
        value = std::clamp(value, 0.0f, controlScrollInfo().maxPos());
        if (sameFactors(m_positionTarget, value))
            return;
        m_positionTarget = value;
        animate(AnimationSlots::scroll, m_position, value, [this](AnimateParams& params) {
            applyPosition(params.value, true);
        });
    }

    // One step on from wherever the position is heading, rather than from where it has reached.
    // A held button repeats faster than a glide lasts, so the position trails the target for as
    // long as the button is down, and a step measured from the position would lose that trail on
    // every repeat.
    void SliderBase::animatePositionBySteps(float steps)
    {
        animatePosition(m_positionTarget + steps * stepSize());
    }

    void SliderBase::stopPositionAnimation()
    {
        stopAnimation(AnimationSlots::scroll);
        m_positionTarget = m_position;
    }

    void SliderBase::offsetPosition(float value)
    {
        float newPosition = m_position + value;
        setPosition(newPosition);
    }

    void SliderBase::offsetPositionBySteps(float steps)
    {
        offsetPosition(steps * stepSize());
    }

    void SliderBase::paintButton(ScrollButton& btn, PaintEvent& event)
    {
        // The button's own parent, reached through the paint event, which is the only route that
        // cannot climb past it. parent() here is the slider's parent, not the button's - this is a
        // slider method, and the button is the slider's child - so a scroll bar sitting in a scroll
        // box grew its marks whenever the pointer was anywhere over the box's body.
        const PaintEvent& sliderEvent = *event.parentEvent();
        float phf = sliderEvent.hoveredFactor() * sliderEvent.enabledFactor();

        event.defaultPaintSurface(buttonInset());
        float size = 6.0f - pressedFactor() + phf * 2.0f;
        size = event.scaleF(size);
        if (enabledFactor() || event.disabledBlendAmount() < 1.0f)
            paintButtonMark(event, size, btn.direction());
    }

    void SliderBase::changed(SliderChangeEvent& event)
    {
        emitEvent(event);
    }

    // A range that has shrunk under the position leaves the target beyond its end, and the glide
    // to the new end is the movement that closes the gap the shrinking opened. A target already
    // inside the range is untouched, so an alignment pass costs nothing here.
    void SliderBase::revalidatePosition()
    {
        float maxPos = controlScrollInfo().maxPos();
        if (m_positionTarget <= maxPos)
            return;
        animatePosition(maxPos);
    }

    //BaseSlider2::Button::Button(ContainerControl* parent, const ScrollDirection dir)
    //  :
    //  ChildItem{ parent },
    //  m_direction{ dir }
    //{
    //}

    void SliderBase::adjustNestedControlVisualState(const Control& control, VisualState& state) const
    {
        SliderBaseClass::adjustNestedControlVisualState(control, state);
        //if (Input::device() == InputDevice::Keyboard)
        //{
        //  ScrollButton* button = nullptr;
        //  if (&control == &m_beginButton)
        //      button = &m_beginButton;
        //  else if (&control == &m_endButton)
        //      button = &m_endButton;
        //  else
        //      return;
        //  if (button->m_clickRepeater.active())
        //      state.pressed = true;
        //}
    }

    void SliderBase::controlFeedBack()
    {
        // without checking Pressed it flickers crazy while dragging
        //
        if (!m_thumb.isPressed())
        {
            alignThumb();
            invalidateChildrenStates();
        }
    }

    void SliderBase::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& contentSize)
    {
        SliderBaseClass::alignContent(event, position, contentSize);
        setControlDimensions(m_bodySpacer, { 0, 0 });
        alignThumb();
    }

    void SliderBase::alignThumb() const
    {
        const FloatRect area = slotArea();

        const ScrollInfo si = controlScrollInfo();
        const bool isVertical = (m_axis == ScrollAxis::Vertical);
        const bool isScrollBar = (viewMode() == SliderViewMode::ScrollBar);

        // Eliminate verticalDirection(): Determine if direction is inverted (ToBegin)
        // Sliders usually invert vertically (bottom-to-top), while ScrollBars do not.
        float dblPos = position();
        if (isVertical && !isScrollBar)
        {
            dblPos = si.maxPos() - dblPos;
        }

        const float slotSize = isVertical ? area.height() : area.width();
        float thumbStart = isVertical ? area.top : area.left;

        // Calculate thumb size with boundaries
        float thumbSize = slotSize;
        if (si.max > 0)
        {
            thumbSize = std::round(slotSize * si.page / si.max);
        }
        const float minThumbSize = scaler().scale(k_thumbSize);
        thumbSize = std::clamp(thumbSize, std::min(minThumbSize, slotSize), slotSize);

        // Calculate thumb position along the axis
        if (const float scrollRange = si.max - si.page; scrollRange > 0.0f)
        {
            // A position gliding back into a range that has just shrunk is outside that range
            // until the glide lands, and the thumb stays in its slot the whole way.
            const float ratio = std::clamp(dblPos / scrollRange, 0.0f, 1.0f);
            thumbStart += ratio * (slotSize - thumbSize);
        }

        // Determine cross-axis size based on ViewMode
        const float slotCrossSize = isVertical ? area.width() : area.height();
        const float crossSize = isScrollBar ? slotCrossSize : thumbSize;

        // Construct types matching setControlPlacement's signature
        const ScaledPosition position = isVertical
            ? ScaledPosition{ area.centerX() - crossSize / 2.0f, thumbStart }
        : ScaledPosition{ thumbStart, area.centerY() - crossSize / 2.0f };

        const ScaledDimensions dimensions = isVertical
            ? ScaledDimensions{ crossSize, thumbSize }
        : ScaledDimensions{ thumbSize, crossSize };

        // This handles all old/new invalidations safely and returns early if unmoved
        Control::setControlPlacement(m_thumb, position, dimensions);
    }

    void SliderBase::getControlState(GetStateEvent& event) const
    {
        SliderBaseClass::getControlState(event);
        if (event.state.enabled)
            event.state.enabled = controlScrollInfo().maxPos() > 0;

        event.stopPropagation();
    }

    void SliderBase::pressDown(PressDownEvent& event)
    {
        FloatPoint pt = slotPoint(form().mouseDownPos());
        m_slotPressed = slotArea().contains(pt);
        if (!m_slotPressed)
            return;

        float clickScrollTarget = slotPosition(pt);
        if (glidesToPosition())
            animatePosition(clickScrollTarget);
        else
            setPosition(clickScrollTarget);
        // A slot press that lands the thumb under the pointer hands the drag to the thumb, which
        // is the route a slider takes. A scroll bar sends the content on a journey instead, so
        // the hit test finds the slot again and the drag below is what steers.
        event.updateDownControl();
    }

    void SliderBase::pressUp(PressUpEvent&)
    {
        m_slotPressed = false;
        m_thumbPressed = false;
    }

    // The press sent the position to the point under the pointer, and the pointer goes on naming
    // it for as long as it is held. A point outside the slot names the end it is past, the same
    // way a thumb dragged off the end stops at the end.
    void SliderBase::drag(DragEvent& event)
    {
        if (!m_slotPressed)
            return;
        setPosition(slotPosition(slotPoint(event.currentPos())));
        event.lockHoveredControl();
        event.stopPropagation();
    }

    void SliderBase::applyPosition(float value, bool triggerChange)
    {
        if (m_position == value)
            return;

        float prevPosition = m_position;
        m_position = value;
        controlFeedBack();
        if (triggerChange)
        {
            SliderChangeEvent event{ *this, m_position, prevPosition };
            changed(event);
        }
    }

    FloatRect SliderBase::slotArea() const
    {
        return bodyRect();
    }

    FloatPoint SliderBase::slotPoint(FloatPoint pointOnForm) const
    {
        const FormBase& frm = form();
        FloatRect rect = frm.rectOfControl(this);
        return pointOnForm - (rect.topLeft() + frm.scaler().scale(padding()));
    }

    float SliderBase::slotPosition(FloatPoint point) const
    {
        FloatRect area = slotArea();
        bool reversePos = false;
        float travel;
        float travelStart;
        float pointOnAxis;
        switch (m_axis)
        {
            case ScrollAxis::Vertical:
                area.inflate(0.0f, -m_thumb.height() / 2.0f);
                travel = area.height();
                travelStart = area.top;
                pointOnAxis = point.y;
                reversePos = verticalDirection() == ScrollDirection::ToBegin;
                break;
            case ScrollAxis::Horizontal:
            default:
                area.inflate(-m_thumb.width() / 2.0f, 0.0f);
                travel = area.width();
                travelStart = area.left;
                pointOnAxis = point.x;
                break;
        }

        if (travel <= 1.0f)
            return position();

        ScrollInfo si = controlScrollInfo();
        float maxPos = si.maxPos();
        if (!maxPos)
        {
            // it's a slider
            maxPos = si.max;
        }
        float ratio = (pointOnAxis - travelStart) / travel;
        if (reversePos)
            ratio = 1.0f - ratio;
        return std::clamp(maxPos * ratio, 0.0f, maxPos);
    }

    // Vertical travel runs the way verticalDirection() names, so the top of the slider is the end
    // that direction does not name. A scroll bar counts downward and takes its begin button at the
    // top; a slider counts upward and takes it at the bottom.
    PanelSlot SliderBase::barSlotFor(ScrollDirection direction) const
    {
        if (m_axis == ScrollAxis::Horizontal)
            return direction == ScrollDirection::ToBegin ? PanelSlot::Left : PanelSlot::Right;

        return direction == verticalDirection() ? PanelSlot::Bottom : PanelSlot::Top;
    }

    float SliderBase::calcButtonSize(const AppTheme& theme, const Scaler& scaler)
    {
        return scaler.scale(buttonSize(theme));
    }

}
