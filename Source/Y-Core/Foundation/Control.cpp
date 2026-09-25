module ClaFi.Core.Foundation;

import :Form;
import :Input;
import :Action;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Layout;

import ClaFi.Diagnostic.Log;
import ClaFi.Diagnostic.Options;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Utils;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    static constexpr std::array<const AnimationSlot*, static_cast<std::size_t>(VisualStateIndex::Count)> k_animationVisualStateSlots{
        &AnimationSlots::hovered,
        &AnimationSlots::pressed,
        &AnimationSlots::focused,
        &AnimationSlots::selected,
        &AnimationSlots::enabled,
        &AnimationSlots::textHovered,
        &AnimationSlots::current
    };

    void attachAction(Action& action, Control& control, const PresenterRole role)
    {
        action.attach(control, role);
    }

    namespace
    {
        template <typename MainSize, typename CrossSize, typename AssignMain, typename AccumulateCross, typename TotalSpacingCross>
        ScaledDimensions calculateLanes(
            ControlSpan controls,
            const bool autoWrap,
            std::size_t wrapCount,
            MainSize mainSize,
            CrossSize crossSize,
            float mainSpacing,
            float maxConstraint,
            AssignMain assignMain,
            AccumulateCross accumulateCross,
            TotalSpacingCross totalSpacingCross)
        {
            ScaledDimensions result{ 0.0f, 0.0f };
            std::size_t lineCount = 0;

            float currentLineCross = 0.0f;
            float currentLineMain = 0.0f;
            std::size_t itemsOnLineNum = 0;

            for (ControlPtr& item : controls)
            {
                if (!item->visible())
                    continue;

                float sizeWithSpacing = mainSize(item);
                if (currentLineMain > 0.0f)
                    sizeWithSpacing += mainSpacing;

                currentLineMain += sizeWithSpacing;

                // A line holding nothing yet takes the item whatever it measures. Counted rather than
                // compared against the first CHILD: the line begins at the first VISIBLE one.
                const bool breakLine = autoWrap && (itemsOnLineNum == wrapCount ||
                    (currentLineMain > maxConstraint && itemsOnLineNum != 0));

                if (breakLine)
                {
                    ++lineCount;
                    assignMain(result, std::max(0.0f, currentLineMain - sizeWithSpacing));
                    accumulateCross(result, currentLineCross);

                    currentLineCross = crossSize(item);
                    currentLineMain = mainSize(item);
                    itemsOnLineNum = 1;
                }
                else
                {
                    ++itemsOnLineNum;
                    currentLineCross = std::max(currentLineCross, crossSize(item));
                }
            }

            // THE OPEN LINE IS COMMITTED WHERE THE WALK ENDS, not on the last child. A walk ending on
            // a hidden child leaves a line nothing closed, and a last child that wrapped opens a line
            // the break did not close - either one reports no size at all while holding items.
            if (itemsOnLineNum != 0)
            {
                ++lineCount;
                assignMain(result, currentLineMain);
                accumulateCross(result, currentLineCross);
            }

            accumulateCross(result, totalSpacingCross(lineCount));
            return result;
        }

        enum class LaneAxis
        {
            Horizontal,
            Vertical
        };
    }

    // WHAT THE CHILDREN OF A LANE CAN BE CUT TO, composed by the rule the lane composes their
    // sizes by: along the lane the floors add up, across it the largest one stands for all of
    // them. A LANE THAT WRAPS ADDS NOTHING ALONG ITSELF - everything past the first item can
    // fall onto a line of its own, so the lane is only as long as its longest item.
    //
    // Across a wrapping lane this is the largest single item rather than the lines that item
    // count breaks into, so a wrapping panel of items that each state a floor under-reports its
    // own across the wrap. What reads this gives way, so an under-report costs room rather than
    // losing anything.
    //
    // SPACING COUNTS ONLY BETWEEN ITEMS THAT STATE A FLOOR. An item cut to nothing takes the gap
    // beside it with it, and a lane of items that all give way gives way whole - which is what
    // keeps a count of items from standing as a minimum nobody set.
    static ScaledDimensions composeLaneMinSize(ControlSpan controls, const LaneAxis axis,
                                               const bool autoWrap, float mainSpacing)
    {
        const bool mainIsX = axis == LaneAxis::Horizontal;
        float main = 0.0f;
        float cross = 0.0f;

        for (const ControlPtr& item : controls)
        {
            if (!item->visible())
                continue;
            const ScaledDimensions itemMin = item->calculatedMinSize();
            const float itemMain = mainIsX ? itemMin.x : itemMin.y;
            const float itemCross = mainIsX ? itemMin.y : itemMin.x;
            cross = std::max(cross, itemCross);
            if (itemMain <= 0.0f)
                continue;
            if (autoWrap)
            {
                main = std::max(main, itemMain);
                continue;
            }
            if (main > 0.0f)
                main += mainSpacing;
            main += itemMain;
        }

        if (mainIsX)
            return { main, cross };
        return { cross, main };
    }

    // AdjustMetricsEvent

    AdjustMetricsEvent::AdjustMetricsEvent(const FormContext& env,
        const Control& control, ControlMetrics& metrics)
        :
        ControlEventBaseC{ env },
        control{ control },
        metrics{ metrics }
    {
    }

    // AdjustViewportEvent

    AdjustViewportEvent::AdjustViewportEvent(
        const FormContext& env,
        const Control& control,
        FloatRect& viewport)
        :
        ControlEventBaseC{ env },
        control{ control },
        viewport{ viewport }
    {
    }

    // AlignEvent

    AlignEvent::AlignEvent(FormContext& formContext, LayoutPass& pass,
        const ControlMetrics& designMetrics)
        :
        ControlEventBase{ formContext },
        m_pass{ pass }
    {
        calculateScaledMetrics(designMetrics);
    }

    AlignEvent::AlignEvent(AlignEvent& parentEvent, const ControlMetrics& designMetrics)
        :
        AlignEvent(parentEvent.formContext(), parentEvent.m_pass, designMetrics)
    {
    }

    void AlignEvent::calculateScaledMetrics(const ControlMetrics& designMetrics)
    {
        spacing = scale(designMetrics.spacing);
        padding = scale(designMetrics.padding);
        minSize = scale(designMetrics.minSize);
        preferredSize = scale(designMetrics.preferredSize);
        // No limit is k_maxFloat in both spaces: Scaler::scale hands it straight back, so a
        // maximum nothing stated stays the sentinel here rather than arriving as a number the
        // layout would take for a width. A padding subtracted from it is still it, and every
        // reader either compares it or tests it against k_maxFloat.
        maxSize = scale(designMetrics.maxSize);
    }

    float AlignEvent::minContentWidth() const
    {
        return std::max(0.0f, minSize.x - padding.x * 2.0f);
    }

    float AlignEvent::minContentHeight() const
    {
        return std::max(0.0f, minSize.y - padding.y * 2.0f);
    }

    float AlignEvent::maxContentWidth() const
    {
        return maxSize.x - padding.x * 2.0f;
    }

    float AlignEvent::maxContentHeight() const
    {
        return maxSize.y - padding.y * 2.0f;
    }

    float AlignEvent::totalSpacingX(std::size_t itemsNum) const
    {
        if (itemsNum > 0)
            return spacing.x * (itemsNum - 1);
        return 0;
    }

    float AlignEvent::totalSpacingY(std::size_t itemsNum) const
    {
        if (itemsNum > 0)
            return spacing.y * (itemsNum - 1);
        return 0;
    }

    // HoverEnterEvent

    HoverEnterEvent::HoverEnterEvent(Control& control)
        :
        control{ control }
    {
    }

    // HoverLeaveEvent

    HoverLeaveEvent::HoverLeaveEvent(Control& control)
        :
        control{ control }
    {
    }

    // ClickEventBase

    ClickEventBase::ClickEventBase(Control& control, FormBase& form)
        :
        ClickEventBase{ control, form, form.mouseDownStamp() }
    {
    }

    ClickEventBase::ClickEventBase(Control& control, FormBase& form, const InputStamp stamp)
        :
        control{ &control },
        form{ form },
        modifiers{ Platform::keyModifiers() },
        stamp{ stamp }
    {
    }

    void ClickEventBase::closeForm()
    {
        form.close();
        stopPropagation();
    }

    float ClickEventBase::scaleFactor() const
    {
        return form.scaler().factor();
    }

    PointInForm ClickEventBase::clickPos() const
    {
        return form.mouseDownPos();
    }

    const AppContext& ClickEventBase::appContext() const
    {
        return form.appContext();
    }

    // GetStateEvent

    GetStateEvent::GetStateEvent(const Control& control)
        :
        control{ control }
    {
    }

    const FormBase& GetStateEvent::form() const
    {
        return control.form();
    }

    // ContextPopupEvent

    ContextPopupEvent::ContextPopupEvent(Control& control, FormBase& form, FloatPoint* mousePos)
        :
        ClickEventBase{ control, form },
        mousePos{ mousePos }
    {

    }

    // EditContextPopupEvent

    EditContextPopupEvent::EditContextPopupEvent(Control& control, FormBase& form,
        ActionList& actions, FloatPoint* mousePos)
        :
        ContextPopupEvent{ control, form, mousePos },
        actions{ actions }
    {
    }

    // DestroyEvent

    DestroyEvent::DestroyEvent(const Control& control)
        :
        control{ control }
    {
    }

    // ControlEventBaseCC

    ControlEventBaseCC::ControlEventBaseCC(const FormContext& formContext, const Control& control)
        :
        ControlEventBaseC{ formContext },
        m_control{ control }
    {
    }

    // CharPressEvent

    CharPressEvent::CharPressEvent(const FormContext& formContext, const Control& control, wchar_t character)
        :
        ControlEventBaseCC{ formContext, control },
        m_character{ character }
    {
    }

    // Any access to the buffer at all, which is where a named text is put into it.
    Text& GetTextEventBase::Buffer::built() const
    {
        m_owner.materialise();
        return m_owner.m_buffer;
    }

    // GetTextEventBase

    GetTextEventBase::GetTextEventBase(const FormContext& formContext, const Control& control, Text& text, EventPhase phase)
        :
        ControlEventBaseCC{ formContext, control },
        m_buffer{ text },
        m_phase{ phase },
        m_source{ m_ownSource }
    {
    }

    // Not delegated to the constructor above: this one shares the other event's name slot as well
    // as its buffer, and delegating would bind the slot to its own.
    GetTextEventBase::GetTextEventBase(GetTextEventBase& other, bool /*dummy*/)
        :
        ControlEventBaseCC{ other.formContext(), other.control() },
        m_buffer{ other.m_buffer },
        m_phase{ other.m_phase },
        m_source{ other.m_source }
    {
    }

    void GetTextEventBase::contribute(const Text& contribution)
    {
        if (!m_source && m_buffer.blank())
        {
            m_source = &contribution;
            return;
        }

        materialise();
        m_buffer << contribution;
    }

    const Text& GetTextEventBase::result()
    {
        // Something wrote into the buffer after a text was named, so the answer is the two of them.
        if (m_source && !m_buffer.blank())
            materialise();

        return m_source ? *m_source : m_buffer;
    }

    // In FRONT of whatever the buffer already holds: a text is named before anything that reaches
    // the buffer afterwards, so that is the order the two go in.
    void GetTextEventBase::materialise()
    {
        if (!m_source)
            return;

        const Text& named = *m_source;
        m_source = nullptr;
        if (m_buffer.blank())
        {
            m_buffer << named;
            return;
        }

        Text combined{ named };
        combined << m_buffer;
        m_buffer = std::move(combined);
    }

    // GetTooltipEvent

    GetTooltipEvent::GetTooltipEvent(const FormContext& formContext, const Control& control, Text& text, EventPhase phase)
        :
        ControlEventBaseC{ formContext },
        control{ control },
        text{ text },
        anchorRect{ control.boundsInForm() },
        phase{ phase }
    {
    }

    // AdjustChildInsetEvent

    AdjustChildInsetEvent::AdjustChildInsetEvent(
        const FormContext& formContext,
        const Control& control,
        ScaledPadding contentPadding)
        :
        ControlEventBaseC{ formContext },
        control{ control },
        contentPadding{ contentPadding },
        inset{ contentPadding }
    {
    }

    // AdjustTextRectEvent

    AdjustTextRectEvent::AdjustTextRectEvent(const Control& control, PaintEvent& event)
        :
        ControlEventBaseC{ event.formContext() },
        control{ control },
        padding{ event.padding() },
        spacing{ event.spacing() },
        textBounds{ event.controlBounds(), -padding }
    {
    }

    AdjustTextRectEvent::AdjustTextRectEvent(
        const FormContext& formContext,
        const Control& control,
        const FloatRect& itemRect)
        :
        AdjustTextRectEvent{ formContext, control, control.designMetrics(), itemRect  }
    {
    }

    AdjustTextRectEvent::AdjustTextRectEvent(
        const FormContext& formContext,
        const Control& control,
        const ControlMetrics& metrics,
        const FloatRect& itemRect)
        :
        ControlEventBaseC{ formContext },
        control{ control },
        padding{ scale(metrics.padding) },
        spacing{ scale(metrics.spacing) },
        textBounds{ itemRect, -padding}
    {
    }

    // MouseInputEvent

    MouseInputEvent::MouseInputEvent(const FormBase& form, Control& control, PointInControl posOnControl, PointInForm posOnForm)
        :
        Event{},
        form{ form },
        control{ control },
        posOnControl{ posOnControl },
        posOnForm{ posOnForm }
    {
    }

    // DragEvent

    DragEvent::DragEvent(Control& control, FormBase& form, PointInForm startPos, PointInForm currentPos,
        InputStamp stamp)
        :
        m_control{ control },
        m_form{ form },
        m_startPos{ startPos },
        m_currentPos{ currentPos },
        m_stamp{ stamp }
    {
    }

    FloatRect DragEvent::selectionRect() const
    {
        FloatPoint currentPt = m_currentPos;
        FloatRect result;
        if (m_currentPos.x > m_startPos.x)
        {
            result.left = m_startPos.x;
            result.right = currentPt.x;
        }
        else
        {
            result.left = currentPt.x;
            result.right = m_startPos.x;
            if (result.right == result.left)
                ++result.right;
        }
        if (m_currentPos.y > m_startPos.y)
        {
            result.top = m_startPos.y;
            result.bottom = currentPt.y;
        }
        else
        {
            result.top = currentPt.y;
            result.bottom = m_startPos.y;
            if (result.bottom == result.top)
                ++result.bottom;
        }
        return result;
    }

    float DragEvent::scaledDistance() const
    {
        return distanceBetweenPoints(m_startPos, m_currentPos);
    }

    float DragEvent::unscaledDistance() const
    {
        return scaledDistance() / m_form.scaler().factor();
    }

    void DragEvent::offsetStartPos(FloatPoint value) const
    {
        m_form.offsetMouseDownPos(value);
    }

    // CreateParams

    CreateParams::CreateParams(Control& parent)
        :
        parent{ &parent },
        form{ parent.form() }
    {
    }

    CreateParams::CreateParams(FormBase& form)
        :
        parent{ nullptr },
        form{ form }
    {
    }

    const AppTheme& CreateParams::theme() const
    {
        return form.theme();
    }

    const ThemeMetrics& CreateParams::themeMetrics() const
    {
        return form.themeMetrics();
    }

    const BakedColors& CreateParams::bakedColors() const
    {
        return form.bakedColors();
    }

    const AppContext& CreateParams::appContext() const
    {
        return form.appContext();
    }

    Control::~Control()
    {
        // First, so that anything holding a pointer to this control drops it before the rest of
        // the teardown runs. A handler may not disconnect: the vector this emit walks is the one
        // disconnect erases from.
        emitEvent<DestroyEvent>(DestroyEvent{ *this });

        stopAnimations();

        Input::forgetControl(this);

        // A control with no parent announces nothing. An owner detaches its children before it
        // destroys them and announces them itself - see releaseChildren.
        if (Control* p = m_parent)
        {
            do p->nestedControlDeleted(this);
            while ((p = p->m_parent));
            if (visible())
                invalidateFormAlign();
        }

        Tooltip::forgetControl(this);
    }

    void Control::deleteSelf()
    {
        m_parent->deleteControl(*this);
    }

    void Control::deleteControl(Control&)
    {
        throw std::logic_error{ "deleteControl can only be used on containers" };
    }

    bool Control::isWidthGivenFromOutside() const
    {
        const Control* control = this;
        // A CONTROL THAT DOES NOT FILL KEEPS WHAT IT MEASURED, whatever its host was willing to
        // hand it - align gives the surplus back. Its width is its own content's answer then, and
        // so is the width of everything standing inside it, however freely the hosts between hand
        // one down.
        while (control->horizontalAlign() == HorizontalAlign::Fill
            && control->isWidthGivenByParent())
        {
            control = control->m_parent;
        }
        // Stopped short of the root, so this control was reached from one that is as wide as what
        // it contains - and everything under that one is inside that measurement.
        if (control->m_parent)
            return false;
        // The root is as wide as the window. A window a form asked for out of its content is the
        // content's own answer once more, and a form that does that is never done asking - see
        // FormBase::autoFit.
        const FormBase* hostForm = control->getForm();
        return hostForm && hostForm->autoFit() == AutoFit::No;
    }

    FormBase& Control::form()
    {
        return *getForm();
    }

    const FormBase& Control::form() const
    {
        Control* it = const_cast<Control*>(this);
        return it->form();
    }

    FormContext& Control::formContext()
    {
        return form().context();
    }

    const FormContext& Control::formContext() const
    {
        return form().context();
    }

    AppContext& Control::appContext()
    {
        return form().appContext();
    }

    const AppContext& Control::appContext() const
    {
        return form().appContext();
    }

    AnimationController* Control::animator()
    {
        FormBase* form = getForm();
        return form ? &form->appContext().animator() : nullptr;
    }

    void Control::animate(const AnimationSlot& slot, const float currentValue,
        const float endValue, const OnAnimate& onAnimate)
    {
        if (AnimationController* controller = animator())
        {
            controller->start(this, slot, currentValue, endValue, onAnimate);
            return;
        }

        AnimateParams params{ this, slot, endValue };
        onAnimate(params);
    }

    void Control::stopAnimation(const AnimationSlot& slot)
    {
        if (AnimationController* controller = animator())
            controller->stop(this, slot);
    }

    void Control::stopAnimations()
    {
        if (AnimationController* controller = animator())
            controller->stop(this);
    }

    const AppTheme& Control::theme() const
    {
        return form().theme();
    }

    const ThemeMetrics& Control::themeMetrics() const
    {
        return form().themeMetrics();
    }

    const BakedColors& Control::bakedColors() const
    {
        return form().bakedColors();
    }

    const Scaler& Control::scaler() const
    {
        return form().scaler();
    }

    ActionState Control::state() const
    {
        GetStateEvent event{ *this };

        auto p = this;
        do p->getControlState(event);
        while (!event.propagationStopped() && ((p = p->m_parent)));
        return event.state;
    }

    void Control::setStateFactor(VisualStateIndex index, float value, bool triggerInvalidate)
    {
        const std::size_t i = static_cast<std::size_t>(index);
        const bool changed = triggerInvalidate && !sameFactors(value, m_factors[i]);
        m_factors.set(i, value);
        if (changed)
            invalidate();
    }

    void Control::invalidateState(AnimationMode animationMode)
    {
        VisualState state = visualState();
        const bool painted = isPainted() && visible();
        const bool animates = painted && animationMode == AnimationMode::On;
        for (std::size_t el = 0ull; el != static_cast<std::size_t>(VisualStateIndex::Count); el++)
        {
            const float targetValue = state.asArray()[el] /* 1.0 if true */;
            if (animates)
            {
                const float currValue = factors()[el];
                animate(*k_animationVisualStateSlots[el], currValue, targetValue, s_onAnimateState);
            }
            else
                m_factors.set(el, targetValue);
        }
        if (painted && !animates)
            invalidate();
    }

    void Control::invalidateChildrenStates()
    {
        for (ControlPtr& it : controls())
            it->invalidateState();
    }

    void Control::invalidateSiblingStates() const
    {
        m_parent->invalidateChildrenStates();
    }

    void Control::update()
    {
        form().update();
    }

    void Control::invalidate() const
    {
        const FormBase* form = getForm();
        if (!form)
            // when we are in the construction stage
            return;

        // A dirty rect repaints every control the paint walk finds inside it, so an overlay
        // control drawn over this one is redrawn on top without being asked for - as long as the
        // walk still enters it. It enters a control by the bounds the traversal builds, which is
        // where the control is drawn, so a standard-clipped overlay is found by the rect it
        // covers and this control's own rect is the whole cost. An unbounded one has painted
        // outside those bounds, and no rect over what it drew reaches the bounds it is entered
        // by, so its host is the smallest rect that does.
        const Control* controlToInvalidate = this;
        Control* current = m_parent;
        while (current)
        {
            if (current->hasUnboundedOverlayControls())
                controlToInvalidate = current;
            current = current->m_parent;
        }

        form->invalidateControl(controlToInvalidate);
    }

    void Control::invalidateFormAlign()
    {
        getForm()->invalidateAlign();
    }

    bool Control::respondsToPointer() const
    {
        switch (interactivity())
        {
        case Interactivity::MouseOnly:
        case Interactivity::Focusable:
            return true;
        default:
            return false;
        }
    }

    bool Control::canTakeFocus() const
    {
        if (!enabled(true))
            return false;
        switch (interactivity())
        {
        case Interactivity::Focusable:
        case Interactivity::ActiveContainer:
            return true;
        default:
            return false;
        }
    }

    void Control::setFocus()
    {
        // A control reached by a key reads the way the one under the pointer does, so the hover
        // follows the focus while the keyboard drives. The mouse already put the hover where it
        // belongs.
        if (Input::device() == InputDevice::Keyboard)
            Input::setHoveredControl(this);
        FocusEvent event{ *this, form() };
        nestedControlFocusing(event);
        // Only a retarget needs this. When the focus lands here the write at the end of the walk
        // has already invalidated both sides of the change.
        if (event.control != this)
            invalidateState();
    }

    void Control::animatedClick(FormBase& form, const InputStamp stamp)
    {
        if (!canClick())
            return;
        ClickEvent event{ *this, form, stamp };
        doClick(event);
    }

    void Control::setVisible(const bool value)
    {
        if (visible() == value)
            return;
        setFlag(m_flags, cfHidden, !value);
        doVisibilityChanged();
        // A popup standing on this control, or on anything inside it, goes down with it.
        if (!value)
            form().controlHidden(*this);
        form().invalidateAlign();
    }

    void Control::scrollIntoView()
    {
        if (m_parent && m_dimensions.y && m_dimensions.x)
            scrollIntoView(scrollHotspot());
    }

    void Control::scrollIntoView(const FloatRect& rectInControl)
    {
        // A held control is in view because it is held, and the rect handed up from here is the
        // place it is held away from - see isHeldInView.
        if (!m_parent || isHeldInView())
            return;
        FloatRect rectInParent = rectInControl;
        rectInParent.offset(boundsInParent().topLeft());
        m_parent->scrollChildIntoView(*this, rectInParent);
    }

    void Control::scrollIntoViewOnAlign()
    {
        form().scrollIntoViewOnAlign(*this);
    }

    bool Control::enabled(const bool deep) const
    {
        bool result = state().enabled;
        if (deep && m_parent && result)
            result = m_parent->enabled(true);
        return result;
    }

    bool Control::isHovered() const
    {
        const Control* control = Input::hoveredControl();
        while (control)
        {
            if (control == this)
                return true;
            control = control->m_parent;
        }
        return false;
    }

    bool Control::isTextHovered() const
    {
        return isTextDrawn() && Input::hoveredControl() == this && Input::hoveredOverText();
    }

    bool Control::isPressed() const
    {
        bool blue = form().isKeyboardClick();
        return (containsNested(Input::hoveredControl(), CheckSelf::Yes) && (Input::isMouseDown() || blue));
    }

    bool Control::isFocused() const
    {
        return Input::focusedControl() && Input::focusedControl()->focusDelegate() == this;
    }

    bool Control::isDroppedDown() const
    {
        return m_parent && m_parent->isChildDroppedDown(*this);
    }

    // Not gated on an overlay entry naming this control. The two are independent: the parent says
    // where its child is drawn, and whichever container holds the entry says when it is painted
    // and what clips it. A grid answers for its own header while the entry naming that header
    // sits on the grid's parent, so that the header's shadow falls on what the grid stands on.
    FloatPoint Control::floatOffset() const
    {
        if (!m_parent)
            return {};
        return m_parent->overlayChildOffset(*this);
    }

    void Control::scrollViewBy(FloatPoint delta)
    {
        for (Control* control = this; control->m_parent; control = control->m_parent)
        {
            if (control->m_parent->controlIsOnScrollBox(*control))
                return control->m_parent->scrollBy(delta);
        }
    }

    FloatPoint Control::parentContentOrigin() const
    {
        if (!m_parent)
            return { 0.0f, 0.0f };

        Control* it = const_cast<Control*>(this);
        return it->form().contentOriginOfControl(m_parent);
    }

    FloatRect Control::boundsInForm() const
    {
        Control* it = const_cast<Control*>(this);
        return boundsInForm(it->form());
    }

    FloatRect Control::boundsInForm(const FormBase& form) const
    {
        return form.rectOfControl(this);
    }

    FloatRect Control::viewPort(FormBase& form) const
    {
        return boundsInForm(form);
    }

    FloatRect Control::visibleRectInForm() const
    {
        Control* it = const_cast<Control*>(this);
        return visibleRectInForm(it->form());
    }

    FloatRect Control::visibleRectInForm(FormBase& form) const
    {
        FloatRect result = boundsInForm(form);
        for (const Control* ancestor = parent(); ancestor; ancestor = ancestor->parent())
        {
            const FloatRect clippedRect = FloatRect::intersection(result, ancestor->viewPort(form));
            if (!clippedRect.empty())
                result = clippedRect;
        }
        return result;
    }

    FloatRect Control::windowInForm() const
    {
        Control* it = const_cast<Control*>(this);
        return windowInForm(it->form());
    }

    // The same walk visibleRectInForm makes, started one rect earlier: from the nearest viewport
    // rather than from this control's bounds. A control with nothing above it is its own window.
    FloatRect Control::windowInForm(FormBase& form) const
    {
        const Control* ancestor = parent();
        if (!ancestor)
            return boundsInForm(form);

        FloatRect result = ancestor->viewPort(form);
        for (ancestor = ancestor->parent(); ancestor; ancestor = ancestor->parent())
        {
            const FloatRect clippedRect = FloatRect::intersection(result, ancestor->viewPort(form));
            if (!clippedRect.empty())
                result = clippedRect;
        }
        return result;
    }

    // The walk stops at the body itself, which is the control the box scrolls: everything below
    // it moves with it, so it is the only one whose inset is measured from a view that holds
    // still. Answered by asking each parent in turn rather than by looking for a scroll box,
    // because being the scrolled body is the host's to say - see controlIsOnScrollBox.
    ScaledPadding Control::scrollContentInset() const
    {
        for (Control* control = const_cast<Control*>(this); control->m_parent; control = control->m_parent)
        {
            if (control->m_parent->controlIsOnScrollBox(*control))
                return control->childInset(formContext(), control->scaledPadding());
        }
        return {};
    }

    // The same walk scrollContentInset makes, and it stops in the same place: the body a box
    // scrolls is the control that moves, so the box holding it is the one with travel to name.
    FloatPoint Control::viewTravelRemaining() const
    {
        for (Control* control = const_cast<Control*>(this); control->m_parent; control = control->m_parent)
        {
            if (control->m_parent->controlIsOnScrollBox(*control))
                return control->m_parent->scrollTravelRemaining();
        }
        return {};
    }

    ControlMetrics Control::designMetrics() const
    {
        ControlMetrics metrics;
        AdjustMetricsEvent event{ formContext(), *this, metrics };
        doAdjustMetrics(event);
        return metrics;
    }

    Padding Control::designPadding() const
    {
        ControlMetrics metrics;
        AdjustMetricsEvent event{ formContext(), *this, metrics };
        doAdjustMetrics(event);
        return metrics.padding;
    }

    ScaledPadding Control::scaledPadding(const Scaler& scaler) const
    {
        return scaler.scale(designPadding());
    }

    ScaledPadding Control::scaledPadding() const
    {
        return scaledPadding(form().scaler());
    }

    ScaledPadding Control::childInset(const FormContext& formContext, ScaledPadding contentPadding) const
    {
        AdjustChildInsetEvent event{ formContext, *this, contentPadding };
        adjustChildInset(event);
        return event.inset;
    }

    HorizontalAlign Control::horizontalAlign() const
    {
        return static_cast<HorizontalAlign>(m_flags & cfHorizontalAlignMask);
    }

    void Control::setHorizontalAlign(const HorizontalAlign value)
    {
        if (horizontalAlign() == value)
            return;
        storeHorizontalAlign(value);
        invalidateFormAlign();
    }

    VerticalAlign Control::verticalAlign() const
    {
        return static_cast<VerticalAlign>(m_flags & cfVerticalAlignMask);
    }

    void Control::setVerticalAlign(const VerticalAlign value)
    {
        if (verticalAlign() == value)
            return;
        storeVerticalAlign(value);
        invalidateFormAlign();
    }

    VerticalTextAnchor Control::verticalTextAnchor() const
    {
        return static_cast<VerticalTextAnchor>(m_flags2 & cfVerticalTextAnchorMask);
    }

    void Control::setVerticalTextAnchor(const VerticalTextAnchor value)
    {
        m_flags2 = static_cast<ControlFlags>((m_flags2 & ~cfVerticalTextAnchorMask) | static_cast<FlagByte>(value));
    }

    HorizontalTextAnchor Control::horizontalTextAnchor() const
    {
        return static_cast<HorizontalTextAnchor>(m_flags2 & cfHorizontalTextAnchorMask);
    }

    void Control::setHorizontalTextAnchor(const HorizontalTextAnchor value)
    {
        m_flags2 = static_cast<ControlFlags>((m_flags2 & ~cfHorizontalTextAnchorMask) | static_cast<FlagByte>(value));
    }

    void Control::setWordWrap(const WordWrap value)
    {
        m_flags2 = static_cast<ControlFlags>((m_flags2 & ~cfNoWordWrap) | static_cast<FlagByte>(value));
    }

    TextAnchor Control::textAnchor() const
    {
        return { verticalTextAnchor(), horizontalTextAnchor() };
    }

    void Control::copyText(const TextRange range, const InputStamp stamp)
    {
        Text tmpText;
        GetTextEvent event{ formContext(), *this, tmpText, EventPhase::Paint };
        getText(event);
        const Text& text = event.result();
        const std::size_t cnt = (std::min)(text.plainText().size() - range.start, range.length);
        const std::wstring_view plainText{ &text.plainText()[range.start], cnt };

        Transfer::Source source{};
        source.add<Transfer::PlainText>(std::wstring{ plainText });
        formContext().clipboard().set(std::move(source), stamp);
    }

    bool Control::containsNested(const Control& potentialChild, const CheckSelf checkSelf) const
    {
        return containsNested(&potentialChild, checkSelf);
    }

    bool Control::containsNested(const Control* potentialChild, const CheckSelf checkSelf) const
    {
        // The `this &&` that used to open this line was undefined and clang removes it - see
        // getForm below. Of the seven call sites, three test their receiver already and three
        // cannot hold nothing - two call it on themselves, one holds a reference. The seventh
        // was ScrollBox::scrollChildIntoView, reaching through an empty body slot, and it now
        // asks first.
        if (checkSelf == CheckSelf::Yes || potentialChild != this)
            while (potentialChild)
            {
                if (potentialChild == this)
                    return true;
                potentialChild = potentialChild->m_parent;
            }
        return false;
    }

    ControlSpan::iterator Control::firstChildInViewport(const FloatRect& viewport)
    {
        ControlSpan span = controls();
        const TraversalOrder order = traversalOrder();
        // Without a sorted axis the children may sit in any order, and a binary search
        // over an unordered range returns an arbitrary position rather than a wrong one
        // that is merely conservative.
        //
        // A HIDDEN CHILD IS NOT ORDERED EITHER, and it is the same fault by another road: it was
        // never laid out, so it stands at the origin with no size, and its key answers "ends
        // before the view" from wherever in the collection it sits. One of those behind a child
        // that does not answer so leaves the range unpartitioned, and the search then reports a
        // position that is not merely early - it can report the end and take every child with it.
        // The walk skips what is not visible anyway, so the whole range is the safe answer.
        if (span.empty() || !(order.x || order.y) || (m_flags & cfChildHidden))
            return span.begin();

        // The children ending before the viewport form a prefix of the range, so
        // lower_bound finds where that prefix ends. The key is the far edge of the lane
        // a child occupies: its own trailing edge in a single lane container, and an
        // upper bound built from laneExtent in a wrapping one.
        // Visibility is deliberately absent from the key. It carries no ordering, so
        // folding it in would leave the range unpartitioned and break the search. The
        // callers walking the returned range skip hidden children themselves.
        const float rangeStart = order.x ? viewport.left : viewport.top;
        return{ std::lower_bound(
            span.begin(), span.end(), rangeStart,
            [order](const ControlPtr& candidate, const float rangeStart)->bool {
                const float leadingEdge = order.x ? candidate->left() : candidate->top();
                const float laneEnd = order.laneExtent > 0.0f
                    ? leadingEdge + order.laneExtent
                    : (order.x ? candidate->right() : candidate->bottom());
                return laneEnd <= rangeStart;
            }
        ) };
    }

    bool Control::isViewportEnd(int vRight, int vBottom) const
    {
        // detecting the end of the range
        return (m_topLeft.y > vBottom || m_topLeft.x > vRight);
    }

    bool Control::isViewportEnd(float vRight, float vBottom) const
    {
        // detecting the end of the range
        // TODO: this stops the scan on both axes at once, which only holds for a
        // container whose children are sorted on both - no container is. For a wrapping
        // layout the cross-axis half is wrong, because the next lane restarts at the
        // leading edge. Left as it stands until it can be changed without putting a
        // virtual call, or a null parent check, into this loop.
        return (m_topLeft.y > vBottom || m_topLeft.x > vRight);
    }

    // ControlBase

    Control::Control(Control* parent)
        :
        m_parent{ parent }
    {
    }

    void Control::adjustChildPaint(AdjustPaintEvent& event)
    {
        event.control().adjustPaint(event);
    }

    bool Control::hasOverlayControls() const
    {
        return !overlayControls().empty();
    }

    bool Control::hasUnboundedOverlayControls() const
    {
        for (const OverlayEntry& entry : overlayControls())
        {
            if (entry.clippingMode == ClippingMode::Unbounded)
                return true;
        }
        return false;
    }

    bool Control::hasInOverlayControls(const Control& control) const
    {
        auto overlayControls = this->overlayControls();
        for (const auto& entry : overlayControls)
        {
            if (entry.control == &control)
                return true;
        }
        return false;
    }

    ClippingMode Control::getOverlayClippingMode(const Control& control) const
    {
        auto overlayControls = this->overlayControls();
        for (const auto& entry : overlayControls)
        {
            if (entry.control == &control)
                return entry.clippingMode;
        }
        return ClippingMode::Standard;
    }

    Control* Control::overlayControlOf(const OverlayEntry& entry)
    {
        return static_cast<Control*>(const_cast<void*>(entry.control));
    }

    float Control::restLineInForm() const
    {
        Control* it = const_cast<Control*>(this);
        FormBase& form = it->form();
        float result = windowInForm(form).top;
        // The inset is laid from the slot of the box doing the scrolling, so it is added to the
        // slot's top and not to the window's: the window is narrowed by every container between,
        // and a container that has not yet reached the slot's top narrows it to its own edge,
        // which is where its content begins and no line to rest an inset below. The same walk
        // scrollContentInset makes, stopping where it stops.
        for (Control* control = it; control->m_parent; control = control->m_parent)
        {
            if (control->m_parent->controlIsOnScrollBox(*control))
            {
                const float slotTop = control->m_parent->viewPort(form).top;
                result = std::max(result, slotTop + control->childInset(formContext(), control->scaledPadding()).y);
                break;
            }
        }
        for (const Control* ancestor = m_parent; ancestor; ancestor = ancestor->m_parent)
        {
            if (const Control* header = ancestor->heldHeader())
                return std::max(result, header->boundsInForm(form).bottom);
        }
        return result;
    }

    // The walk restLineInForm makes, measured from the other end of the slot.
    float Control::footLineInForm() const
    {
        Control* it = const_cast<Control*>(this);
        FormBase& form = it->form();
        float result = windowInForm(form).bottom;
        for (Control* control = it; control->m_parent; control = control->m_parent)
        {
            if (control->m_parent->controlIsOnScrollBox(*control))
            {
                const float inset = control->childInset(formContext(), control->scaledPadding()).y;
                result = std::min(result, control->m_parent->viewPort(form).bottom - inset);
                break;
            }
        }
        return result;
    }

    FloatPoint Control::heldHeaderOffset(const Control& header, float headedBottom) const
    {
        const float restLine = restLineInForm() - header.parentContentOrigin().y;
        const float home = header.top();
        const float lastStand = std::max(home, headedBottom - header.height());
        return { 0.0f, std::clamp(restLine, home, lastStand) - home };
    }

    float Control::heldHeaderStrip() const
    {
        const Control* header = heldHeader();
        if (!header)
            return 0.0f;
        // The same walk scrollContentInset makes, stopping where it stops: a holder further out
        // than the scrolled body holds its header over a different scroll.
        for (Control* control = const_cast<Control*>(this); control->m_parent; control = control->m_parent)
        {
            if (control->m_parent->heldHeader())
                return header->height();
            if (control->m_parent->controlIsOnScrollBox(*control))
                break;
        }
        return scrollContentInset().y + header->height();
    }

    void Control::paintChildSurface(PaintEvent& event)
    {
        event.control().paintSurface(event);
    }

    void Control::scrollChildIntoView(Control& control, FloatRect controlRect)
    {
        // The hold reaches everything inside it: a child of a held control has no offset of its
        // own to say so, and its rect is measured from the same place its host is held away from.
        if (!m_parent || isHeldInView())
            return;
        controlRect.offset(topLeft());
        controlRect.offset(childInset(formContext(), scaledPadding()));
        m_parent->scrollChildIntoView(control, controlRect);
    }

    bool Control::isChildDroppedDown(const Control& control) const
    {
        FormBase* activePopup = form().activePopup();
        return activePopup && activePopup->popupTarget() == &control;
    }

    void Control::getChildText(GetChildTextEvent& event) const
    {
        GetTextEvent childEvent{ event, true };
        event.control().getText(childEvent);
        emitEvent(event);
    }

    FormBase* Control::getForm()
    {
        return m_parent ? m_parent->getForm() : nullptr;
    }

    // No `if (!this)` here. Calling a member function through a null pointer is undefined before
    // the body starts, so a compiler is entitled to assume the test cannot fail - and clang does
    // exactly that, deleting the branch at any optimisation level above none. The check was not
    // protecting anything; it was hiding whichever caller passes a null pointer, and it would
    // have stopped hiding it in a release build. No caller needs it today: every one of them
    // either calls this on itself or holds a pointer it has already checked. One that cannot
    // writes `ptr ? ptr->getForm() : nullptr`, which is a test the language defines.
    const FormBase* Control::getForm() const
    {
        return const_cast<Control*>(this)->getForm();
    }

    Control* Control::navigationContainer(CheckSelf checkSelf)
    {
        Control* control = checkSelf == CheckSelf::Yes ? this : m_parent;
        while (control && control->interactivity() != Interactivity::ActiveContainer)
            control = control->m_parent;
        return control;
    }

    Control* Control::focusDelegate()
    {
        return this;
    }

    void Control::nestedControlFocusing(FocusEvent& event)
    {
        if (Control* parent = this->parent())
            parent->nestedControlFocusing(event);
        else
            Input::setFocusedControl(*event.control);
    }

    void Control::nestedControlHovered(Control* hovered)
    {
        if (Control* parent = this->parent())
            parent->nestedControlHovered(hovered);
    }

    void Control::sizeChanged()
    {
    }

    void Control::setTopLeft(float x, float y)
    {
        m_topLeft = { x, y };
    }

    void Control::setDimensions(const ScaledDimensions value)
    {
        if (value != m_dimensions)
        {
            m_dimensions = value;
            sizeChanged();
        }
    }

    void Control::setTop(float value)
    {
        if (m_topLeft.y == value)
            return;
        invalidate();
        m_topLeft.y = value;
        invalidate();
    }

    void Control::setLeft(float value)
    {
        if (m_topLeft.x == value)
            return;
        invalidate();
        m_topLeft.x = value;
        invalidate();
    }

    void Control::setWidth(float value)
    {
        if (m_dimensions.x == value)
            return;
        invalidate();
        m_dimensions.x = value;
        invalidate();
    }

    void Control::setHeight(float value)
    {
        if (m_dimensions.y == value)
            return;
        invalidate();
        m_dimensions.y = value;
        invalidate();
    }

    void Control::getControlState(GetStateEvent& event) const
    {
        // The state event walks up the parent chain, so a control answers only for itself.
        if (&event.control != this)
            return;

        emitEvent(event);

        // A handler of its own makes the control the authority on its own state, and the walk
        // ends here. The only thing above that writes a descendant's state is a container
        // marking its current item, and that is a default for an item with no opinion: a radio
        // button's selected IS its dot, and no container has any business turning it off.
        if (hasEventListeners<GetStateEvent>())
            event.stopPropagation();
    }

    NavigationWrap Control::navigationWrap() const
    {
        if (Control* parent = this->parent())
            return parent->navigationWrap();
        else
            return { false, false };
    }

    FloatRect Control::scrollHotspot() const
    {
        return FloatRect::fromDimensions({ 0.0f, 0.0f }, m_dimensions);
    }

    void Control::mouseMove(MouseMoveEvent& event)
    {
        emitEvent(event);
    }

    void Control::drag(DragEvent&)
    {
    }

    void Control::pressDown(PressDownEvent& event)
    {
        emitEvent(event);
    }

    void Control::click(ClickEvent& event)
    {
        emitEvent(event);
    }

    void Control::doubleClick(DoubleClickEvent& event)
    {
        emitEvent(event);
    }

    void Control::tripleClick(TripleClickEvent& event)
    {
        emitEvent(event);
    }

    void Control::hoverEnter()
    {
        emitEvent<HoverEnterEvent>(HoverEnterEvent{ *this });
    }

    void Control::hoverLeave()
    {
        emitEvent<HoverLeaveEvent>(HoverLeaveEvent{ *this });
    }

    void Control::contextPopup(ContextPopupEvent& event)
    {
        emitEvent(event);
    }

    void Control::editContextPopup(EditContextPopupEvent& event)
    {
        emitEvent(event);
    }

    void Control::keyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
        case Keys::Escape:
            Tooltip::handleUserInput();
            break;

        case Keys::Return:
            break;
        }
    }

    void Control::adjustPaint(AdjustPaintEvent& event)
    {
        emitEvent(event);
    }

    void Control::getTooltip(GetTooltipEvent& event)
    {
        emitEvent(event);
        // A handler that produced text wins over the trimmed-text fallback below.
        if (!event.text.empty())
            return;
        // The fallback stands in for text this control has cut, so it starts where that text
        // is and nowhere else. A tooltip given to the control is about the control and starts
        // anywhere inside it, which is why the pointer test guards this branch alone.
        // isTextHovered names THIS control, so a parent reached by the walk up from the hovered
        // one answers with a tooltip of its own or with nothing.
        //
        // A scroll body is left out of it. Its text is reached by scrolling rather than
        // withheld, and one row cut against the body's width would be answered with the whole
        // of the text the body holds.
        if ((cfTextTrimmed & m_flags) && isTextHovered() && !isOnScrollBox())
        {
            event.placement = FormPlacement::OverText;
            event.wordWrap = wordWrap();
            // The tooltip reads its own text, so a named answer is put into it. Assigning what
            // the buffer already holds to itself is the gather having built one, and costs the
            // comparison in Text::operator= and nothing else.
            event.text = doGetText(event.formContext(), event.text, EventPhase::Paint);
            // THE HINT STANDS ON THE GLYPHS, NOT ON THE BOX THEY WERE GIVEN. An anchor moves the
            // block inside that box - a line centred in a title bar sits half the band below its
            // top - and the placement lands the hint's own first glyph on this rect's top left.
            // The width is the box's, which is the width the lines were broken at and what
            // TooltipForm breaks its own at.
            const FloatRect textRect = textBounds(event.formContext(), boundsInForm());
            const CalculatedDimensions drawn = s_textEngine.calculateText(
                event.formContext(),
                event.text,
                textRect.dimensions(),
                editProps() != nullptr,
                wordWrap()
            );
            event.anchorRect = FloatRect::fromDimensions(
                anchoredOrigin(textRect, drawn, textAnchor()),
                { textRect.width(), drawn.y }
            );
        }
    }

    void Control::getText(GetTextEvent& event) const
    {
        emitEvent(event);
    }

    TextRenderMode Control::textRenderMode() const
    {
        // Keyed on whether the control moves, not on whether it is interactive. Interactivity was
        // the wrong question: a check box responds to the pointer without its label ever leaving
        // the pixel it was laid out on, and asking it to give up snapping, grid fit and subpixel
        // antialiasing bought nothing and cost it every bit of crispness.
        if (!allowZAnimation())
            return TextRenderMode::Static;

        // Movable at rest rather than Static, so the antialiasing mode is a property of the control
        // and not of the moment. Text that fell back to subpixel while still would show its colour
        // fringes appear and vanish as the pointer crossed it - a change with no meaning behind it,
        // at rest, where no movement covers it.
        //
        // Moving is the expensive answer - outlines rebuilt from beziers, no glyph cache - so it is
        // held to the one control actually engaged. Given for the whole of a hover rather than only
        // while the scale is in flight: full hover is the far end of the movement, not a state
        // outside it, and a control sits there for as long as the pointer does. Dropping it there
        // would put a rasterizer change at the one moment nothing else is moving to cover it, which
        // is the artefact this whole path exists to avoid.
        //
        // What that leaves is two handovers per hover, both at zero, where the movement is just
        // starting or has just ended. Everything a glyph's raster could depend on beyond the
        // rasterizer itself - snapping, grid fit, antialiasing - is already the same on both sides
        // of them.
        return (hoveredFactor() > 0.0f || pressedFactor() > 0.0f)
            ? TextRenderMode::Moving
            : TextRenderMode::Movable;
    }

    FloatRect Control::textBounds(const FormContext& formContext, const FloatRect& itemRect) const
    {
        AdjustTextRectEvent event{ formContext, *this, itemRect  };
        adjustTextRect(event);
        return event.textBounds;
    }

    FloatRect Control::textBounds(PaintEvent& paintEvent) const
    {
        AdjustTextRectEvent event{ *this, paintEvent };
        adjustTextRect(event);
        return event.textBounds;
    }

    void Control::paintIcon(PaintIconEvent& event)
    {
        emitEvent(event);
    }

    void Control::paintSurface(PaintEvent& event)
    {
        event.defaultPaintSurface();

        // The hit zone the pointer stands in, outlined - see Diagnostic::Options::highlightTextAreas.
        if constexpr (Diagnostic::Options::highlightTextAreas)
        {
            float textFactor = m_factors.textHovered();
            if (textFactor)
            {
                FloatRect textBounds = this->textBounds(event);
                event.canvas().drawRectangle(
                    textBounds,
                    event.strokeRgb().withOpacity(textFactor),
                    event.scale(1.0f)
                );
            }
        }
        emitEvent(event);
    }

    void Control::paintChildren(PaintEvent& event)
    {
        event.paintChildren();
    }

    void Control::paintText(PaintEvent& event)
    {
        FloatRect txtRect = textBounds(event);
        if (txtRect.empty())
            return;
        Text text;
        const Text& gathered = doGetText(event.formContext(), text, EventPhase::Paint);
        DrawTextResult result = drawText(event, txtRect, gathered);
        if (result.drawn)
            m_flags2 |= cfTextDrawn;
        if (result.trimmed)
        {
            m_flags |= cfTextTrimmed;
        }
    }

    DrawTextResult Control::drawText(PaintEvent& event, const FloatRect& textBounds, const Text& text)
    {
        return s_textEngine.drawText(
            event.controlContext(),
            textBounds,
            text,
            textAnchor(),
            editProps(),
            textRenderMode(),
            wordWrap()
        );
    }

    void Control::calculate(FormBase& form)
    {
        ControlMetrics designMetrics;
        {
            AdjustMetricsEvent event{ form.context(), *this, designMetrics };
            doAdjustMetrics(event);
        }
        calculateChildren(form);

        if (designMetrics.maxSize == designMetrics.minSize)
        {
            m_dimensions = form.scaler().scale(designMetrics.minSize);
            m_minDimensions = m_dimensions;
        }
        else
        {
            AlignEvent event{ form.context(), form.layoutPass(), designMetrics };
            // THE BOX THE CONTENT CAME TO, ceiled and padded here rather than where it is read.
            // It is what this control is, unless a preference stands in its place.
            //
            // TODO: ceiling, padding and the min clamp run only on this calculated-content path,
            // not on the fixed-minSize one above. Should they apply to both?
            ScaledDimensions measured = calculateContent(event);
            if (measured.x != k_maxFloat)
            {
                measured.x = std::ceil(measured.x);
                measured.x += event.padding.x * 2.0f;
                measured.x = std::max(measured.x, event.minSize.x);
            }
            measured.x = std::min(measured.x, event.maxSize.x);
            if (measured.y != k_maxFloat)
            {
                measured.y = std::ceil(measured.y);
                measured.y += event.padding.y * 2.0f;
                measured.y = std::max(measured.y, event.minSize.y);
            }
            measured.y = std::min(measured.y, event.maxSize.y);

            // A STATED PREFERENCE STANDS IN PLACE OF THE MEASUREMENT, ON ITS OWN AXIS. The content
            // is measured either way - a control preferring a width still finds its height that
            // way, and its children are laid out whatever it comes to - and what a preference
            // states is the whole box, so the padding the measured value is grown by is already
            // inside it. The minimum still holds it up and the maximum still holds it down: a
            // preference is what the control would like, not what it is owed.
            m_dimensions.x = event.preferredSize.x > 0.0f
                ? std::min(std::max(event.preferredSize.x, event.minSize.x), event.maxSize.x)
                : measured.x;
            m_dimensions.y = event.preferredSize.y > 0.0f
                ? std::min(std::max(event.preferredSize.y, event.minSize.y), event.maxSize.y)
                : measured.y;

            // WHAT SOMEBODY SET, HERE OR BELOW - NEITHER THE MEASUREMENT NOR THE PREFERENCE. A
            // preference is what this control would LIKE to be, and a measurement is what it came
            // to holding what it holds today; the least it can be is the MinSize it was given,
            // held up against the floor its content composed out of its children's.
            //
            // A MEASUREMENT IS NOT A FLOOR. The range a drag is held inside is this number - see
            // FormBase::stateSizeRange - so a window whose minimum is what it measured can only
            // ever be grown: open enough tabs and the strip that got wide is the strip that pins
            // the frame, and the fit that would have cut those tabs back is never reached because
            // the box holding them is never made narrower.
            //
            // The content's floor carries this control's padding, the way the measured box does.
            // A floor of zero carries nothing: content that gives way whole leaves no box to pad.
            ScaledDimensions contentMin = event.calculatedMinSize;
            if (contentMin.x > 0.0f)
            {
                contentMin.x = std::ceil(contentMin.x);
                contentMin.x += event.padding.x * 2.0f;
            }
            if (contentMin.y > 0.0f)
            {
                contentMin.y = std::ceil(contentMin.y);
                contentMin.y += event.padding.y * 2.0f;
            }
            m_minDimensions.x = std::max(contentMin.x, event.minSize.x);
            m_minDimensions.y = std::max(contentMin.y, event.minSize.y);
            m_minDimensions.x = std::min(m_minDimensions.x, m_dimensions.x);
            m_minDimensions.y = std::min(m_minDimensions.y, m_dimensions.y);
        }
    }

    ScaledDimensions Control::calculateText(AlignEvent& event, ScaledDimensions constraints)
    {
        Text text;
        const Text& gathered = doGetText(event.formContext(), text, EventPhase::Calculate);
        if (gathered.empty() && !editProps())
            return {};
        // A text that does not wrap is measured against no width at all. What a horizontal scroll
        // has to be told is how wide the text is, and a width to break at is the one thing that
        // would stop it saying so.
        const ScaledDimensions asked = wordWrap()
            ? constraints
            : ScaledDimensions{ k_maxFloat, constraints.y };
        ScaledDimensions result = measureText(event, asked, gathered);
        return {
            result.x,
            std::min(result.y, event.maxSize.y)
        };
    }

    CalculatedDimensions Control::measureText(AlignEvent& event, ScaledDimensions asked,
        const Text& text)
    {
        return s_textEngine.calculateText(
            event.formContext(),
            text,
            asked,
            editProps() != nullptr,
            wordWrap()
        );
    }

    void Control::calculateChildrenSequentially(FormBase& form)
    {
        ControlSpan span = controls();
        for (ControlPtr& item : span)
            if (item->visible())
                item->calculate(form);
    }

    void Control::calculateChildren(FormBase& form)
    {
        calculateChildrenSequentially(form);
    }

    ScaledDimensions Control::calculateContent(AlignEvent& event)
    {
        return calculateText(event, { event.maxContentWidth(), event.maxContentHeight() });
    }

    void Control::alignContent(AlignEvent& event, ScaledPosition, ScaledDimensions& newDimensions)
    {
        // A wrapping text answers a different width with a different height, in both directions:
        // a narrower box breaks more lines out of a paragraph and a wider one joins them, and the
        // box a control is granted is not always the one it was measured against. So it is asked
        // again, without a guard here about whether the question has moved: asking is what holds
        // the answers - TextEngine's cache, and a TextBox's own layout and last measurement - and
        // a question already answered comes back from them without reading a text or shaping one.
        //
        // A text that does not wrap is the width it is whatever it is given, and measuring it
        // again would say so again.
        if (wordWrap())
        {
            // A FIXED SIZE IS NOT MEASURED, on this pass as on the calculate pass: a control
            // whose minimum and maximum agree is that size whatever its text comes to, so the
            // measurement below would be made and thrown away - and for a list of a million
            // fixed items it is a million shapings the cache cannot hold, on every pass.
            if (event.maxSize == event.minSize)
                return;

            // AN EMPTY MEASUREMENT IS NOT A SIZE. calculateText answers {0, 0} for a control
            // that holds no text, and this box is what the control's children are laid out in -
            // a grid row hands it to the control standing in every one of its cells. Taking the
            // empty answer gives each of them a slot of no height, and a control with no height
            // paints nothing at all.
            //
            // A control with no text reaches here because a calculated size is CEILED while the
            // size it is then given is not, so a box measured at a fractional width arrives a
            // fraction of a pixel narrower than the width it was measured at.
            const ScaledDimensions measuredText = calculateText(event, { newDimensions.x, k_maxFloat });
            if (measuredText.x || measuredText.y)
                newDimensions = measuredText;
        }
        else if (isOnScrollBox())
        {
            // A scrolled body keeps what it measured. The align pass cuts a control to the slot
            // it was given, and the slot here is the viewport - so a box scrolls to its content
            // only if the content states the size it came to and holds it.
            float calculatedContentHeight = m_dimensions.y - event.padding.y * 2;
            newDimensions.y = std::max(calculatedContentHeight, newDimensions.y);
            // The width the same way, for a text that is not broken to its box: it is as wide as
            // it is, and a viewport narrower than that is what the horizontal bar is for.
            if (!wordWrap())
            {
                float calculatedContentWidth = m_dimensions.x - event.padding.x * 2;
                newDimensions.x = std::max(calculatedContentWidth, newDimensions.x);
            }
        }

    }

    void Control::align(FormContext& formContext, LayoutPass& pass, ScaledPosition position,
        ScaledDimensions newDimensions)
    {
        ControlMetrics designMetrics;
        {
            AdjustMetricsEvent event{ formContext, *this, designMetrics };
            doAdjustMetrics(event);
        }
        AlignEvent event{ formContext, pass, designMetrics };

        HorizontalAlign adjustedAlignModeX = horizontalAlign();
        VerticalAlign adjustedAlignModeY = verticalAlign();
        bool wasStretchX = adjustedAlignModeX == HorizontalAlign::Fill;
        bool wasStretchY = adjustedAlignModeY == VerticalAlign::Fill;

        bool fitX = width() < newDimensions.x;
        bool fitY = height() < newDimensions.y;
        bool onScroll = isOnScrollBox();
        if (onScroll)
        {
            adjustedAlignModeX = HorizontalAlign::Left;
            adjustedAlignModeY = VerticalAlign::Top;
        }
        HorizontalAlign alignModeX = wasStretchX && fitX ? HorizontalAlign::Fill : adjustedAlignModeX;
        VerticalAlign alignModeY = wasStretchY && fitY ? VerticalAlign::Fill : adjustedAlignModeY;

        bool axisX = true;
        bool axisY = true;
        bool needSecondAlignX = (alignModeX != HorizontalAlign::Fill) && axisX && fitX;
        bool needSecondAlignY = (alignModeY != VerticalAlign::Fill) && axisY && fitY;

        ScaledDimensions alignedDimensions = newDimensions;
        if (needSecondAlignX)
            alignedDimensions.x = m_dimensions.x;
        if (needSecondAlignY)
            alignedDimensions.y = m_dimensions.y;

        ScaledPadding doublePadding = event.padding * 2;
        alignedDimensions -= doublePadding;
        // Children are positioned against the child padding, so alignContent is told where the
        // content box begins in that space. It is {0, 0} unless the control separates the two.
        ScaledPosition contentOrigin = event.padding - childInset(event.formContext(), event.padding);
        alignContent(event, contentOrigin, alignedDimensions);
        // Rounded the way Control::calculate rounds what IT measures, and at the same point: on
        // the content, before the padding goes back on. These are the two routes to one number,
        // and a control sized by one of them has to come out the size the other would give it.
        // Where they disagreed, a wrapping text measured to a fraction of a pixel was handed a box
        // a fraction of a pixel short of itself - a text that does not fit by a hundredth of a
        // pixel, which is a text that collapses.
        alignedDimensions.x = std::ceil(alignedDimensions.x);
        alignedDimensions.y = std::ceil(alignedDimensions.y);
        alignedDimensions += doublePadding;

        float x = 0.0f;
        float y = 0.0f;
        if (axisX && alignedDimensions.x < newDimensions.x)
        {
            switch (horizontalAlign())
            {
            case HorizontalAlign::Right:
                x = newDimensions.x - alignedDimensions.x;
                break;
            case HorizontalAlign::Center:
                x = (newDimensions.x - alignedDimensions.x) / 2.0f;
                break;
            case HorizontalAlign::Fill:
            case HorizontalAlign::Left:
                break;
            }
        }

        if (axisY && alignedDimensions.y < newDimensions.y)
        {
            switch (verticalAlign())
            {
            case VerticalAlign::Bottom:
                y = newDimensions.y - alignedDimensions.y;
                break;
            case VerticalAlign::Center:
                y = (newDimensions.y - alignedDimensions.y) / 2;
                break;
            case VerticalAlign::Fill:
            case VerticalAlign::Top:
                break;
            }
        }

        // A MAXIMUM ON A FILL AXIS IS A CALLER ERROR. Fill says take the whole lane and a maximum
        // says never be this big; nothing resolves the two, and no clamp here would make them
        // agree - it would only hide which of them was meant. Fill is what stands.
        //
        // The measuring pass does clamp to the maximum, so a control given both carries a size
        // measured against its cap inside a box that ignores the cap: a wrapping text broken to
        // the capped width, at the height that wrap came to, laid out at the width of the lane.
        if (horizontalAlign() == HorizontalAlign::Fill)
            alignedDimensions.x = std::max(newDimensions.x, alignedDimensions.x);
        if (verticalAlign() == VerticalAlign::Fill)
            alignedDimensions.y = std::max(newDimensions.y, alignedDimensions.y);

        m_topLeft = position + FloatPoint{ x, y };
        setDimensions(alignedDimensions);

        adjustPlacement(event.scaler(), m_topLeft, m_dimensions);
    }

    ScaledDimensions Control::calculateRows(AlignEvent& event, ControlSpan controls, const bool autoWrap,
        std::size_t wrapCount, const float maxContentWidth)
    {
        event.calculatedMinSize = composeLaneMinSize(controls, LaneAxis::Horizontal, autoWrap, event.spacing.x);
        return calculateLanes(
            controls, autoWrap, wrapCount,
            [](const auto& item) { return item->width(); },
            [](const auto& item) { return item->height(); },
            event.spacing.x,
            std::min(event.maxContentWidth(), maxContentWidth),
            [](auto& res, float val) { res.x = std::max(res.x, val); },
            [](auto& res, float val) { res.y += val; },
            [&event](std::size_t lines) { return event.totalSpacingY(lines); }
        );
    }

    ScaledDimensions Control::calculateColumns(AlignEvent& event, ControlSpan controls, const bool autoWrap, std::size_t wrapCount)
    {
        event.calculatedMinSize = composeLaneMinSize(controls, LaneAxis::Vertical, autoWrap, event.spacing.y);
        return calculateLanes(
            controls, autoWrap, wrapCount,
            [](const auto& item) { return item->height(); },
            [](const auto& item) { return item->width(); },
            event.spacing.y,
            event.maxContentHeight(),
            [](auto& res, float val) { res.y = std::max(res.y, val); },
            [](auto& res, float val) { res.x += val; },
            [&event](std::size_t lines) { return event.totalSpacingX(lines); }
        );
    }

    void Control::alignControl(Control* control, AlignEvent& parentEvent, ScaledPosition position, ScaledDimensions newDimensions)
    {
        control->align(parentEvent.formContext(), parentEvent.pass(), position, newDimensions);
    }

    // NOTHING IS HANDED OUT WHERE NOBODY ASKED, which is every lane that has not been told
    // otherwise: the walk runs, counts none, and the placement below is the one it always was.
    float Control::laneSurplusShare(ControlSpan::iterator begin, ControlSpan::iterator end,
        const float laneExtent, const float spacing, float (Control::*extent)() const)
    {
        float taken = 0.0f;
        std::size_t fillCount = 0;
        bool first = true;
        for (ControlSpan::iterator it = begin; it != end; ++it)
        {
            const Control& control = **it;
            if (!control.visible())
                continue;
            // Counted off the items rather than off their number: a filling item measures nothing
            // and the gap before it is real, so a count times the spacing would be a gap short.
            if (!first)
                taken += spacing;
            first = false;
            taken += (control.*extent)();
            if (control.fillsLane())
                ++fillCount;
        }
        if (fillCount == 0)
            return 0.0f;

        return std::max(0.0f, laneExtent - taken) / static_cast<float>(fillCount);
    }

    void Control::alignSingleRow(AlignEvent& event, ControlSpan::iterator rowBegin, ControlSpan::iterator rowEnd,
                                     ScaledPosition position, ScaledDimensions rowSize, ScaledDimensions& contentDimensions)
    {
        // Both extents are read off the placement, never off a total accumulated beside it.
        // A row's bottom edge is where this row was put plus the height it was given, and the
        // caller advances to the next row from that same position, so the last row's bottom is
        // the content's bottom however many rows precede it. A running sum of row heights is a
        // second walk over the same numbers, and float rounds each of its steps independently -
        // over hundreds of thousands of rows the two answers separate by pixels, and whichever
        // comes out larger hands the scroll bar a range no item reaches.
        contentDimensions.y = std::max(contentDimensions.y, position.y + rowSize.y);
        // The main axis extent is where the items ended, not the width the calculate pass
        // predicted for them. Both walk the same row, but calculateLanes adds
        // (width + spacing) to its running total while this adds the two separately, so
        // the results drift apart as the row grows. Reporting the larger of the two - as
        // taking rowSize.x here would - leaves a scroll range the last item never reaches.
        const float share = laneSurplusShare(rowBegin, rowEnd, rowSize.x, event.spacing.x,
            &Control::width);
        float lastRight = position.x;
        for (ControlSpan::iterator it2 = rowBegin; it2 != rowEnd; ++it2)
        {
            Control& control = **it2;
            if (!control.visible())
                continue;
            const float width = control.width() + (control.fillsLane() ? share : 0.0f);
            FloatRect itemRect = { position.x, position.y, position.x + width, position.y + rowSize.y };
            alignControl(&control, event, itemRect.topLeft(), itemRect.dimensions());
            lastRight = itemRect.right;
            position.x = itemRect.right + event.spacing.x;
        }
        contentDimensions.x = std::max(contentDimensions.x, lastRight);
    }

    void Control::alignSingleColumn(AlignEvent& event, ControlSpan::iterator colBegin, ControlSpan::iterator colEnd,
                                        ScaledPosition position, ScaledDimensions colSize, ScaledDimensions& contentDimensions)
    {
        // The right edge of this column, rather than a running total of column widths - see
        // alignSingleRow for why the two separate. The caller advances to the next column from
        // this same position, so the last column's right edge is the content's right edge.
        contentDimensions.x = std::max(contentDimensions.x, position.x + colSize.x);
        // The bottom of the last item, rather than colSize.y or the trailing position.
        // colSize.y is the height the calculate pass predicted, accumulated by a different
        // expression than the one placing items here, and over a long column the two no
        // longer agree - taking the larger hands the scroll bar a range that runs past the
        // last item. position.y carries a trailing spacing that nothing occupies.
        const float share = laneSurplusShare(colBegin, colEnd, colSize.y, event.spacing.y,
            &Control::height);
        float lastBottom = position.y;
        for (ControlSpan::iterator it2 = colBegin; it2 != colEnd; ++it2)
        {
            Control& control = **it2;
            if (!control.visible())
                continue;
            const float height = control.height() + (control.fillsLane() ? share : 0.0f);
            FloatRect itemRect = { position.x, position.y, position.x + colSize.x, position.y + height };
            Control::alignControl(&control, event, itemRect.topLeft(), itemRect.dimensions());
            lastBottom = control.bottom();
            position.y = lastBottom + event.spacing.y;
        }
        contentDimensions.y = std::max(contentDimensions.y, lastBottom);
    }

    void Control::placeControlToHorizontalCenter(Control& control, float boundsLeft, float boundsRight)
    {
        float delta = (boundsRight - control.right()) / 2.0f;
        control.setLeft(boundsLeft + std::max(delta, 0.0f));
    }

    void Control::placeControlToVerticalCenter(Control& control, float boundsTop, float boundsBottom)
    {
        float delta = (boundsBottom - control.bottom()) / 2.0f;
        control.setTop(boundsTop + std::max(delta, 0.0f));
    }

    void Control::setControlPlacement(Control& control, ScaledPosition topLeft, ScaledDimensions dimensions)
    {
        if (control.m_topLeft == topLeft && control.m_dimensions == dimensions)
            return;
        control.invalidate();
        control.m_topLeft = topLeft;
        control.m_dimensions = dimensions;
        control.invalidate();
    }

    void Control::offsetControl(Control* control, FloatPoint pt)
    {
        if (control)
            control->m_topLeft.offset(pt);
    }

    // THE SUBTREE'S ANIMATIONS ARE STOPPED HERE, WHILE THE FORM IS STILL REACHABLE. The children
    // go down detached, and a control with no parent cannot find the controller from its own
    // destructor - so it is found here, once, for the whole subtree. A container that is itself
    // detached by now finds none, and has nothing left running: whoever detached it stopped its
    // subtree the same way.
    void Control::releaseChildren()
    {
        if (AnimationController* controller = animator())
            stopNestedAnimations(*this, *controller);

        announceControlsDeleted(*this, m_parent);

        for (ControlPtr& control : controls())
            setControlParent(*control, nullptr);
    }

    VisualState Control::visualState() const
    {
        const ActionState actionState = state();

        // A grid expander depends on this: it wears GridRow and holds its section's rows,
        // so read as hovered it would light the whole section.
        // Only the visual state is gated - a tooltip still asks isHovered().
        const bool interactive = interactivity() != Interactivity::None;

        VisualState result{
            .hovered = interactive && isHovered(),
            .pressed = interactive && isPressed(),
            .focused = interactive && isFocused(),
            .selected = actionState.selected,
            .enabled = actionState.enabled,
            .textHovered = interactive && isTextHovered()
        };
        // A CONTROL WITH A POPUP OF ITS OWN OPEN IS STILL WHERE THE USER IS. The popup takes the
        // focus so that its items can be walked, and the control it was opened on would otherwise
        // read as abandoned the moment it opened: the ring goes out, the fill goes cold, and a
        // text box's caret goes dark - while every command in the popup names that control and
        // nothing else. Said here, once, so that a control needing it says nothing of its own:
        // TextBox draws its caret from this focused, and a dropped-down control keeps its ring.
        //
        // NO EXCEPTIONS, and the one worth naming is the control an in-place editor covers: it
        // reads live while the editor is over it, because being edited is the same relation. A
        // control that wants to read otherwise dims itself from its own popup's
        // adjustNestedControlVisualState, which runs after this.
        //
        // This is how the control READS. Where the focus RESTS is Input::focusedControl, and
        // that is untouched - the two answers differ for exactly as long as the popup is up.
        if (isDroppedDown())
        {
            result.focused = true;
            result.selected = true;
        }
        const Control* p = m_parent;
        while (p)
        {
            p->adjustNestedControlVisualState(*this, result);
            p = p->m_parent;
        }
        return result;
    }

    void Control::invalidateStateUp(Control* control, const Control* upTo, InvalidateEvent event)
    {
        while (control && control != upTo)
        {
            bool pass = true;
            switch (event)
            {
            case InvalidateEvent::HoverLeave:
                control->doHoverLeave();
                break;
            case InvalidateEvent::HoverEnter:
                control->doHoverEnter();
                break;
            case InvalidateEvent::PressDown:
            case InvalidateEvent::PressUp:
                pass = control->respondsToPointer();
                break;
            case InvalidateEvent::None:
            case InvalidateEvent::InputDevice:
                break;
            }
            if (pass)
                control->invalidateState();
            control = control->m_parent;
        }
    }

    const Text& Control::doGetText(const FormContext& formContext, Text& text, EventPhase phase) const
    {
        if (m_parent)
        {
            GetChildTextEvent event{ formContext, *this, text, phase };
            m_parent->getChildText(event);
            return event.result();
        }

        GetTextEvent event{ formContext, *this, text, phase };
        getText(event);
        return event.result();
    }

    FloatPoint Control::pressOrigin(const PaintEvent& event) const
    {
        const FloatPoint landmark = event.center();
        // Weighted by the press, and by nothing else. A control at rest is not unscaled - it sits
        // at pressRestScale - so its origin is live even when it is doing nothing, and an origin
        // taken unconditionally would re-anchor every resting control on the form. Hover earns
        // nothing either: a fully hovered control is at exactly 1.0, where no origin has any effect.
        //
        // This control's own press, deliberately, even where the scale itself takes a share of the
        // parent's - see AdjustPaintEvent::setParentZAmount. A control standing for its parent is
        // small and the press lands somewhere out on the parent, so leaning towards that point
        // would slide the whole thing sideways instead of pushing it in. Its own press stays at
        // zero, which leaves the origin on its own centre - what a small mark should turn about.
        float engagement = pressedFactor();
        if (engagement <= 0.0f)
            return landmark;

        // A keyboard press has no point to lean towards, and the stored position still holds
        // whatever the last mouse press left there - on some other control entirely. The landmark
        // is the answer for anything not driven by the mouse.
        if (Input::device() != InputDevice::Mouse)
            return landmark;

        // Where the press landed, not where the pointer is now. The point is fixed for the life of
        // the press, so nothing has to repaint as the mouse travels, and the origin cannot drift
        // out from under a control that is already animating.
        FloatPoint pressPoint = form().mouseDownPos();
        return {
            std::lerp(landmark.x, pressPoint.x, engagement),
            std::lerp(landmark.y, pressPoint.y, engagement)
        };
    }

    void Control::announceControlsDeleted(Control& container, Control* ancestors)
    {
        for (ControlPtr& control : container.controls())
        {
            announceControlsDeleted(*control, ancestors);
            for (Control* p = ancestors; p; p = p->m_parent)
                p->nestedControlDeleted(&*control);
        }
    }

    void Control::stopNestedAnimations(Control& container, AnimationController& controller)
    {
        for (ControlPtr& control : container.controls())
        {
            stopNestedAnimations(*control, controller);
            controller.stop(&*control);
        }
    }

    void Control::storeHorizontalAlign(const HorizontalAlign value)
    {
        m_flags = static_cast<ControlFlags>((m_flags & ~cfHorizontalAlignMask) | static_cast<FlagByte>(value));
    }

    void Control::storeVerticalAlign(const VerticalAlign value)
    {
        m_flags = static_cast<ControlFlags>((m_flags & ~cfVerticalAlignMask) | static_cast<FlagByte>(value));
    }

    void Control::doVisibilityChanged()
    {
        if (Control::m_parent)
        {
            // Ahead of the virtual, so no override can leave the flag behind by not chaining.
            Control::m_parent->noteChildVisibility(*this);
            Control::m_parent->childVisibilityChanged(*this);
        }
        visibilityChanged();
    }

    // WHAT THE SEARCH OVER THE CHILDREN STANDS ON - see firstChildInViewport, which reads this
    // flag rather than the children. Hiding one states it outright; showing one has to ask the
    // rest, since another may still be hidden. That walk is the price of a show, which is a
    // gesture rather than a frame.
    void Control::noteChildVisibility(const Control& child)
    {
        if (!child.visible())
        {
            m_flags |= cfChildHidden;
            return;
        }
        for (const ControlPtr& item : controls())
        {
            if (!item->visible())
                return;
        }
        m_flags &= ~cfChildHidden;
    }

    void Control::doPressUp(PressUpEvent& event)
    {
        Control* item = this;
        while (item && !event.handled.propagationStopped)
        {
            item->pressUp(event);
            item = item->m_parent;
        }
        Control::invalidateStateUp(this, nullptr, InvalidateEvent::PressUp);
    }

    void Control::doPressDown(PressDownEvent& event)
    {
        Control* item = this;
        while (item && !event.propagationStopped())
        {
            item->pressDown(event);
            item = item->m_parent;
        }
        Control::invalidateStateUp(this, nullptr, InvalidateEvent::PressDown);
    }

    void Control::doClick(ClickEvent& params)
    {
        invalidateState();
        Control* control = this;
        do control->click(params);
        while (!params.propagationStopped() && ((control = control->m_parent)));
        // don't place code here - we may be killed inside the click()
    }

    void Control::doDoubleClick(DoubleClickEvent& event, PointInForm)
    {
        Control* control = this;
        do control->doubleClick(event);
        while (!event.propagationStopped() && ((control = control->m_parent)));
        // don't place code here - we may be killed inside the click()
    }

    void Control::doTripleClick(TripleClickEvent& event, PointInForm)
    {
        Control* control = this;
        do control->tripleClick(event);
        while (!event.propagationStopped() && ((control = control->m_parent)));
        // don't place code here - we may be killed inside the click()
    }

    void Control::doHoverEnter()
    {
        hoverEnter();
        if (m_parent)
            m_parent->childHoverEnter(*this);
    }

    void Control::doHoverLeave()
    {
        hoverLeave();
        if (m_parent)
            m_parent->childHoverLeave(*this);
    }

    void Control::doContextPopup(ContextPopupEvent& event)
    {
        if (!event.propagationStopped())
            contextPopup(event);
    }

    // A ROOT HAS NO PARENT TO ADJUST IT, and the one thing standing over it is the form: the
    // window it is measuring for and the frame that window is wearing - see
    // FormBase::adjustRootMetrics. Applied here, so every reader of a root's metrics - the
    // measuring pass, the align, the paint, the padding the traversal walks by - reads the same
    // block. A control standing on its own, not yet in a tree, has no form to ask.
    void Control::doAdjustMetrics(AdjustMetricsEvent& event) const
    {
        if (m_parent)
        {
            m_parent->adjustChildMetrics(event);
            return;
        }
        adjustMetrics(event);
        if (const FormBase* form = getForm())
            form->adjustRootMetrics(event);
    }

    void Control::doPaintInitialization()
    {
        // first time initialization
        if ((m_flags2 & cfPainted) != cfPainted)
        {
            ActionState st = state();
            m_factors.setEnabled(static_cast<float>(st.enabled));
            m_factors.setSelected(static_cast<float>(st.selected));
            m_flags2 |= cfPainted;
        }
    }

    // THE TWO TEXT FLAGS STATE WHAT THE LAST PAINT OF THIS CONTROL'S OWN TEXT PRODUCED, which is
    // why they are cleared where that paint happens rather than on every visit. A control is
    // walked once per paint stage and paints itself in one of them - an unselected tab is walked
    // in the standard stage and again in the overlay stage its selected sibling is drawn in - so
    // a clear on the visit leaves the answer the drawing stage wrote wiped by the stage that
    // drew nothing.
    //
    // Ahead of paintText rather than inside it: an override is free to draw no text at all - see
    // ButtonBase::paintText in IconOnly mode - and both flags have to read false for it.
    void Control::doPaintTextInitialization()
    {
        m_flags &= ~cfTextTrimmed;
        m_flags2 &= ~cfTextDrawn;
    }

    void Control::doAdjustViewport(AdjustViewportEvent& event) const
    {
        if (m_parent)
            m_parent->adjustChildViewport(event);
        else
            adjustViewPort(event);
    }

    void Control::doAdjustPaint(AdjustPaintEvent& event)
    {
        if (m_parent)
            m_parent->adjustChildPaint(event);
        else
            adjustPaint(event);
    }

    void Control::doPaintSurface(PaintEvent& event)
    {
        if (m_parent)
            m_parent->paintChildSurface(event);
        else
            paintSurface(event);
    }

    void Control::doPainted(PaintEvent& event) const
    {
        if (m_parent)
            m_parent->childPainted(event);
        //else
        //  painted(event);
    }

    bool Control::isOnScrollBox()
    {
        return m_parent && m_parent->controlIsOnScrollBox(*this);
    }

    OnAnimate Control::s_onAnimateState{
        [](const AnimateParams& params) {
            const auto control = static_cast<Control*>(params.control);
            control->setStateFactor(static_cast<VisualStateIndex>(params.tag()), params.value);
        } };

}
