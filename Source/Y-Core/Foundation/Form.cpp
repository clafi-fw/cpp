module ClaFi.Core.Foundation;

import :Form;
import :Control;
import :Action;
import :Input;
import :Navigation;
import :Tooltip;

import ClaFi.Diagnostic.Log;
import ClaFi.Diagnostic.Options;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Scaler;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.ShadowPainter;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.StdLib;


namespace ClaFi
{
    // HOW MANY PASSES ONE FRAME MAY SPEND. A pass that asks for another converges on the next
    // one, which hands back the width it was measured against, and a box above a wrapping panel
    // can ask once more on top of that. Past this the layout is not converging, and the frame
    // goes out on what the last pass produced rather than the application stopping.
    constexpr int k_maxAlignPasses{ 3 };

    namespace
    {
        // The parts of rect outside hole - above it, below it, and beside it between those.
        [[nodiscard]] std::array<FloatRect, 4> partsOutside(const FloatRect& rect,
            const FloatRect& hole)
        {
            const FloatRect gap = {
                std::clamp(hole.left, rect.left, rect.right),
                std::clamp(hole.top, rect.top, rect.bottom),
                std::clamp(hole.right, rect.left, rect.right),
                std::clamp(hole.bottom, rect.top, rect.bottom)
            };
            return { {
                { rect.left, rect.top, rect.right, gap.top },
                { rect.left, gap.bottom, rect.right, rect.bottom },
                { rect.left, gap.top, gap.left, gap.bottom },
                { gap.right, gap.top, rect.right, gap.bottom }
            } };
        }
    }

    namespace
    {
        // ONE CROSSING TO A LINE, ONE PAIR TO A TICK: how long the frame was on the screen, and
        // how much of that the application spent making it. A frame that was on the screen for
        // two refreshes having worked for three milliseconds waited on something this process
        // does not do - the compositor, the driver; one that worked for twenty-five was busy.
        // Those two answers want opposite fixes, and the period alone cannot tell them apart.
        //
        // The forms of one tick paint back to back, so the work is the whole run of them: the
        // first form's start to the last form's end. Both numbers are therefore known only once
        // the NEXT tick begins, which is where the pair is written. The count of forms rides the
        // last pair of the line.
        //
        // A gathered line is written when the next crossing starts, because a line costs a
        // control and a layout of its own, which inside a crossing would be measuring the
        // measurement. A stall longer than the rest between two previews therefore starts a new
        // line, and a line too short to be a crossing is that stall.
        void logCrossingFrame(std::chrono::steady_clock::time_point startedAt)
        {
            using Clock = std::chrono::steady_clock;
            // The forms of one tick follow each other within a fraction of a millisecond, and
            // the shortest tick is four form paints long. Anything between the two is neither.
            constexpr double k_tickGap = 2.0;
            static Clock::time_point s_previous{};
            static Clock::time_point s_tickStartedAt{};
            static std::wstring s_line{};
            static std::size_t s_count{};
            static std::size_t s_forms{};

            // The end of the form before this one, which for the first form of a tick is the end
            // of the whole tick before it.
            const Clock::time_point lastFormEnded = s_previous;
            const Clock::time_point now = Clock::now();
            const double since = lastFormEnded == Clock::time_point{}
                ? 0.0
                : std::chrono::duration<double, std::milli>(startedAt - lastFormEnded).count();
            s_previous = now;

            // Nothing measurable since the last form is that form's own tick carrying on.
            if (since < k_tickGap and s_forms)
            {
                ++s_forms;
                return;
            }

            const double period = s_tickStartedAt == Clock::time_point{}
                ? 0.0
                : std::chrono::duration<double, std::milli>(startedAt - s_tickStartedAt).count();
            const double worked = s_tickStartedAt == Clock::time_point{}
                ? 0.0
                : std::chrono::duration<double, std::milli>(
                    lastFormEnded - s_tickStartedAt).count();
            s_tickStartedAt = startedAt;

            if (s_count and (period > 100.0 or s_count >= 40ull))
            {
                diagnosticLog(Text{ s_line + std::format(L"[{} forms]", s_forms) });
                s_line.clear();
                s_count = 0ull;
            }

            s_forms = 1ull;
            s_line += std::format(L"{:.0f}/{:.1f} ", period, worked);
            ++s_count;
        }
    }

    // Whether a control and every holder between it and the host listing it are shown. An
    // overlay entry stays registered while the holder in between is hidden, and the paint walk
    // reaches a control through its parent's children, so a rect taken from inside a hidden
    // holder stands where no frame has drawn anything.
    [[nodiscard]] static bool shownUpTo(const Control& control, const Control& host)
    {
        for (const Control* item = &control; item && item != &host; item = item->parent())
        {
            if (!item->visible())
                return false;
        }
        return true;
    }
    // FormPaintedEvent

    FormPaintedEvent::FormPaintedEvent(FormBase& form, const IntRect& dirtyRect, TimePoint startedAt,
                                       TimePoint finishedAt)
        :
        EventOf<FormBase>{ form },
        m_dirtyRect{ dirtyRect },
        m_startedAt{ startedAt },
        m_finishedAt{ finishedAt }
    {
    }

    // FormControlBase

    FormControlBase::~FormControlBase()
    {
        stopAnimations();
    }

    FloatPoint FormControlBase::textOrigin() const
    {
        return scaledPadding();
    }

    FormBase* FormControlBase::getForm()
    {
        return &m_form;
    }

    // FormBase

    FormBase::FormBase(AppContext& appContext, WindowRole windowRole, FormControlBase& content, Control* popupTarget,
                       FormPlacement placement)
        :
        FormBase{ appContext, windowRole, content, popupTarget ? &popupTarget->form() : nullptr, popupTarget,
                  placement }
    {
    }

    FormBase::FormBase(AppContext& appContext, WindowRole windowRole,
        FormControlBase& content, FormBase& ownerForm, FormPlacement placement)
        :
        FormBase{ appContext, windowRole, content, &ownerForm, nullptr, placement }
    {
    }

    FormBase::~FormBase()
    {
        // FIRST, and not left to the member teardown that follows this body. The tooltip is a form
        // of its own whose window this form's window OWNS, and the system destroys an owned window
        // along with its owner - so a tooltip taken down after m_window would be destroying a
        // handle that had already been taken. It also reads this form on its way down.
        m_tooltip.destroyForm();

        if (m_popupTargetForm && m_popupTargetForm->m_activePopup == this)
            m_popupTargetForm->m_activePopup = nullptr;
        // Both directions. A popup normally goes first, but nothing enforces it - a popup is a
        // value the caller holds - and a popup left behind reads its owner on the way down.
        if (m_activePopup)
        {
            m_activePopup->m_popupTargetForm = nullptr;
            m_activePopup->m_popupTarget = nullptr;
            m_activePopup->detachScaler(m_ownScaler);
            m_activePopup = nullptr;
        }
    }

    // THE PICTURE STANDS STILL AND STOPS FOLLOWING. The percents are copied over first, so what
    // this form is left drawn at is what it was already showing, and every route that reads the
    // scaler is pointed at the form's own.
    //
    // A form already on its own scaler is left alone rather than re-pointed at it, so asking for
    // a scale it is already holding costs nothing.
    void FormBase::holdScale()
    {
        if (m_scaler == &m_ownScaler)
            return;
        m_ownScaler.setSystemPercent(m_scaler->systemPercent());
        m_ownScaler.setAppPercent(m_scaler->appPercent());
        stateScaler(m_ownScaler);
    }

    // THE FORM UNDERNEATH OWNS THE SCALE AGAIN, which is the state every popup is built in - see
    // m_scaler. A form with nobody under it has nothing to follow and keeps its own.
    //
    // A PLACEMENT AND NOT AN ALIGNMENT. A popup's window is as big as what was in it when the
    // window was placed - see initPlacement, where the size asked for is the content's own - so
    // a scale that has to be measured again is a placement. An alignment lays the content out
    // inside the window that is already there, which cuts it to a size taken at the scale being
    // left, and nothing would ever ask for another: a popup is placed again when the control it
    // stands on moves, and the form underneath is standing still by the time this runs.
    //
    // ASKED FOR WHERE NOTHING IS BEING STEERED. The press point a drag measures its travel from
    // is not moved here, the way a scaler's own change moves it - see scaleFactorChanged - so a
    // form brought back under a held pointer would answer that pointer at the wrong scale.
    void FormBase::followScale()
    {
        if (!m_popupTargetForm)
            return;
        if (m_scaler == m_popupTargetForm->m_scaler)
            return;
        stateScaler(*m_popupTargetForm->m_scaler);
        stateFrame();
        initPlacement();
        invalidate();
    }

    float FormBase::contentHeight() const
    {
        return m_content.height();;
    }

    FloatRect FormBase::geometry() const
    {
        return FloatRect::fromDimensions(frameOrigin(), m_placed.size);
    }

    void FormBase::invalidateAlign()
    {
        m_aligned = false;
        invalidate();
    }

    void FormBase::scrollIntoViewOnAlign(Control& control)
    {
        m_scrollIntoViewOnAlign = &control;
        // The request needs a pass to answer it. Asking for one costs a flag and a repaint, and
        // an alignment already pending swallows it.
        invalidateAlign();
    }

    void FormBase::validateAlign()
    {
        m_aligned = true;
    }

    void FormBase::invalidate() const
    {
        // wrong: if the client is empty rect then paint and update never called
        //if (m_client)
        //  invalidateControl(&*m_client);

        //
        window().invalidateRect(nullptr);
    }

    void FormBase::contentMoved(const Control& movedControl) const
    {
        if (m_activePopup && movedControl.containsNested(m_activePopup->popupTarget()))
            m_activePopup->followPopupTarget2();
    }

    void FormBase::controlHidden(const Control& control)
    {
        // A HIDDEN TARGET IS AS GONE AS A DELETED ONE. The popup was placed on where the control
        // stood, and a hidden control stands nowhere: followPopupTarget2 has no rect to carry it
        // to, so the window stays over the place the control left, with its loop running. A
        // strip that hides the tab an editor is over - WhatsClip, when the copy made inside that
        // editor changes the clipboard - is the case that showed it.
        //
        // The whole subtree is asked, since hiding a container hides what is in it without any
        // of it being told. The same way down as a deletion: the target is cleared, nothing is
        // offered, and the focus goes to nobody - a hidden control cannot take it back.
        if (m_activePopup && control.containsNested(m_activePopup->m_popupTarget))
            dropActivePopup();
    }

    void FormBase::invalidateControl(const Control* control) const
    {
        if (m_layoutInProgress)
            return;
        FloatRect rectOnForm = rectOfControl(control, true);
        AdjustViewportEvent event{ m_context, *control, rectOnForm };
        control->doAdjustViewport(event);
        window().invalidateRect(rectOnForm.roundedOut());
    }

    FloatPoint FormBase::boundsOriginOfControl(const Control* control) const
    {
        const Control* controlParent = control->m_parent;
        if (!controlParent)
            return control->topLeft();

        // The one place a float offset enters form space, so every rect built from this
        // accumulation - the control's bounds, the part of it still visible, the rect an
        // invalidation covers - names where the control is drawn. See Control::floatOffset.
        return control->topLeft() + control->floatOffset() + contentOriginOfControl(controlParent);
    }

    FloatPoint FormBase::contentOriginOfControl(const Control* control) const
    {
        return boundsOriginOfControl(control)
            + control->childInset(m_context, control->scaledPadding(m_context.scaler()));
    }

    FloatRect FormBase::rectOfControl(const Control* control, bool clipIntoParents) const
    {
        FloatRect outRect = FloatRect::fromDimensions(boundsOriginOfControl(control), control->dimensions());

        // Clamping reads each parent's box in form space rather than in that parent's own
        // coordinates. The two say the same thing - a shift applies to a bound and to the value
        // compared against it alike - and this way every position in this function comes from the
        // one accumulation order.
        if (clipIntoParents)
        {
            for (const Control* controlParent = control->m_parent;
                 controlParent;
                 controlParent = controlParent->m_parent)
            {
                const FloatPoint origin = boundsOriginOfControl(controlParent);
                outRect.left = std::max(outRect.left, origin.x);
                outRect.top = std::max(outRect.top, origin.y);
                outRect.right = std::min(outRect.right, origin.x + controlParent->width());
                outRect.bottom = std::min(outRect.bottom, origin.y + controlParent->height());
            }
        }
        return outRect;
    }

    void FormBase::adjustRootMetrics(AdjustMetricsEvent& event) const
    {
        // THE WINDOW IS THE CEILING - the size the placement granted, or the size the user
        // dragged the frame to. A root measured against more than that is measured for a window
        // nobody has, and the overflow is what ScrollBars::Auto reads in the measuring pass,
        // which is why the window has to be stated here rather than in the align.
        //
        // A ceiling, not a maximum: this is what the form HAS, and MaxSize is what it was asked
        // to be. The two are different facts and the second is not the framework's to write.
        //
        // ON THE AXES THE WINDOW ACTUALLY BOUNDS, AND NO OTHERS.
        //
        // An axis the placement REFUSED bounds every pass that follows it, the pass measuring
        // what to ask for next included. That is the one that matters: a list too long for the
        // room is measured against the room, which is where ScrollBars::Auto sees the overrun and
        // puts the bar up - so the width that placement is then asked for is the width WITH the
        // bar. Leave the ask unbounded and the bar can only ever be discovered afterwards, by
        // which time the window has been sized without it, and the strip comes out of the body:
        // every item stands past the view by exactly that width with its tail under the bar.
        //
        // An axis it granted in full bounds nothing while the form is measuring what to ask for -
        // a form held to the window it is about to replace could never grow, and growing by a
        // scrollbar is exactly what it has to do. Once it is laying content into the window it
        // got, that window bounds both axes if it is the USER'S: a window the user dragged is not
        // an answer to the content, and the content has to fit it or show a bar.
        const bool userSized = m_autoFit == AutoFit::No && !m_measuringPlacement;
        const bool boundX = m_placedShortX || userSized;
        const bool boundY = m_placedShortY || userSized;
        const FloatPoint client = m_placed.size;
        const float factor = scaler().factor();
        if (boundX && client.x > 0.0f && client.x / factor < event.metrics.maxSize.x)
            event.metrics.maxSize.x = client.x / factor;
        if (boundY && client.y > 0.0f && client.y / factor < event.metrics.maxSize.y)
            event.metrics.maxSize.y = client.y / factor;

        // THE FLOOR THE WINDOW STANDS ON - the width a dropdown was told to be at least, which is
        // that of the control it fell from. A MinSize, because that is what the placement reads
        // as a floor and what Control::calculate holds the root up by; the ceiling above still
        // wins where the room is short of it. A root stating a wider floor of its own keeps it.
        if (m_minWidth > event.metrics.minSize.x)
            event.metrics.minSize.x = m_minWidth;

        // The corners the window wears: the root's own, or none where the platform squares them.
        event.metrics.radius = m_frame.radius / factor;
    }

    FormBase* FormBase::popupTargetForm()
    {
        return m_popupTargetForm;
    }

    FormBase& FormBase::rootForm()
    {
        if (m_popupTargetForm)
            return m_popupTargetForm->rootForm();
        return *this;
    }

    void FormBase::closeActivePopup()
    {
        if (!m_activePopup)
            return;

        // INNERMOST FIRST. A popup may have one of its own - a question raised from inside a menu
        m_activePopup->closeActivePopup();
        if (m_activePopup->activePopup())
            // A popup's popup refused to close, so we cannot either
            return;

        // The popup gets to refuse. Everything that closes one from outside comes through here
        if (!m_activePopup->readyToClose())
            return;

        // Read AFTERWARDS, both of them. readyToClose is allowed to act, and what it runs may
        // have taken the popup down already - a popup that hides itself clears this pointer on
        // its way out - or deleted the control the focus was to go back to.
        FormBase* popup = m_activePopup;
        if (!popup)
            return;

        // THE TARGET IS KEPT. The way down hands the focus back to it - see updateVisibility -
        // and that is the whole of what closing a popup from outside owes the form behind it.
        // A menu dismissed by a click on that form, or by the application being deactivated,
        // comes through here and through nothing else, so clearing the target here left the user
        // with nothing focused at all: the next key press had nowhere to go, and the
        // context-menu key had no control to be about.
        //
        // Cleared only where there is a reason to hand the focus to NOBODY, which is
        // forgetControl - the target there is being destroyed.
        Control* target = popup->m_popupTarget;
        popup->close();
        m_activePopup = nullptr;
        if (target)
            target->invalidateState();
    }

    void FormBase::close()
    {
        hide();
        switch (m_closeAction)
        {
        case CloseAction::Close:
            m_window->close();
            break;
        case CloseAction::Hide:
            break;
        default:
            break;
        }
        FormCloseEvent event{ *this };
        emitEvent(event);
    }

    int FormBase::execute()
    {
        const bool claimedRootLoop = appContext().claimRootLoop(*this);
        show();
        m_window->doLoop();
        if (claimedRootLoop)
            appContext().releaseRootLoop(*this);
        // to use as the exit code from the main func
        // TODO: add and pass an optional param to close()
        return 0;
    }

    bool FormBase::holdsRootLoop() const
    {
        return appContext().rootLoopForm() == this;
    }

    void FormBase::setPlacementRect(const FloatRect& value)
    {
        if (m_placementRect == value)
            return;
        m_placementRect = value;
        rememberPlacementTarget();
        // A FORM HOLDING ITS OWN SCALE HOLDS ITS PLACE. The rect goes on tracking the control
        // this form stands on, so the placement it comes back to is where that control is by
        // then - but the window is not moved while the hold stands. The hold is there so that a
        // gesture can steer the size of the form underneath without this one moving under the
        // pointer doing the steering, and a form that keeps its size while it walks across the
        // screen is half of that. See followScale, which places it again as the hold ends.
        if (m_placementValid && !holdingScale())
            initPlacement();
    }

    void FormBase::setPlacement(FormPlacement value)
    {
        if (m_placement == value)
            return;
        m_placement = value;
        if (m_placementValid)
            initPlacement();
    }

    void FormBase::setPlacement(FormPlacement placement, const FloatRect& placementRect)
    {
        if (m_placement == placement && m_placementRect == placementRect)
            return;
        m_placement = placement;
        m_placementRect = placementRect;
        rememberPlacementTarget();
        if (m_placementValid)
            initPlacement();
    }

    void FormBase::setMinWidth(const float value)
    {
        if (m_minWidth == value)
            return;
        m_minWidth = value;
        if (m_placementValid)
            initPlacement();
    }

    void FormBase::maximize()
    {
        // helps to determine the right monitor
        updatePlacement();
        m_window->maximize();
        
        // stored position is stale anyway
        // mouseTick(true);
    }

    void FormBase::restore()
    {
        m_window->restore();
        
        // stored position is stale anyway
        // mouseTick(true);
    }

    void FormBase::setWindowTitle(const std::wstring_view value)
    {
        m_windowTitle = value;
        m_window->setTitle(value);
    }

    void FormBase::update()
    {
        m_appContext.animator().externalTimerTick();
        m_window->update();
    }

    void FormBase::mouseTick(bool allowDrag)
    {
        // to realign stale layout
        wnd_beforePaint();

        // Whatever is held down, the pointer moving is the mouse driving.
        Input::setDevice(m_appContext.animator(), InputDevice::Mouse);

        if (allowDrag && Input::isMouseDown() && m_downItem)
        {
            DragEvent dragEvent{ *m_downItem, *this, m_mouseDownPos, m_mousePos, m_mouseDownStamp };
            Control* control = m_downItem;
            while (control && !dragEvent.propagationStopped())
            {
                control->drag(dragEvent);
                control = control->m_parent;
                m_appContext.animator().externalTimerTick();
            }
            m_mousePos = dragEvent.currentPos();
            if (dragEvent.hoveredControlLocked())
                return;
        };

        m_appContext.animator().externalTimerTick();
        if (SearchControlResult searhResult = controlAt(m_mousePos))
        {
            Input::setHoveredControl(
                searhResult.control,
                searhResult.hitZone,
                searhResult.overText
            );
            MouseMoveEvent event{ *this, *searhResult.control, searhResult.relativePt, m_mousePos };
            searhResult.control->mouseMove(event);
            Control* control = searhResult.control;
            do control->nestedMouseMove(event);
            while ((control = control->parent()));
        }
        // The control under the pointer names the shape. The arrow stands in when the pointer is
        // over nothing, which is also what resets it after leaving a control that asked for more.
        const Control* hoveredControl = Input::hoveredControl();
        Platform::setCursor(hoveredControl ? hoveredControl->cursor() : CursorShape::Arrow);
    }

    void FormBase::mouseTick(PointInForm pt, bool allowDrag)
    {
        m_mousePos = pt;
        mouseTick(allowDrag);
    }

    void FormBase::setVisible(bool value)
    {
        if (value == m_visible)
            return;
        m_visible = value;
        updateVisibility();
    }

    IPlatformWindow& FormBase::wnd_window()
    {
        return *m_window;
    }

    void FormBase::wnd_beforePaint()
    {
        // A FORM NOBODY CAN SEE HAS NO FRAME TO PREPARE, and a layout run for one is work for a
        // frame that will never be drawn. A window is sized before it is shown by
        // updateVisibility, which reaches the placement directly and not through here, so
        // nothing that has to happen first happens here.
        if (!m_visible)
            return;

        // A PASS THAT ASKED FOR ANOTHER IS ANSWERED BEFORE THIS FRAME. The request is raised while
        // the tree is being laid out - a wrapping panel broken at a width the measure never saw, a
        // body handed a viewport the measure ran ahead of - so painting on it shows the very
        // layout the request exists to correct. updateAlign returns at once on a form that is
        // already aligned, so the loop ends the moment nothing asks.
        for (int pass = 0; pass < k_maxAlignPasses; ++pass)
        {
            // A form sized by its content is PLACED again rather than aligned. updateAlign(false)
            // lays the content out inside the window that is already there, which is the wrong way
            // round here: text that has grown would be cut to the old window instead of moving it.
            // initPlacement takes the content's own size and sets the bounds from it.
            if (m_autoFit == AutoFit::Yes && !m_aligned)
            {
                m_placementValid = false;
                updatePlacement();
            }
            else
            {
                updateAlign(false);
            }

            if (m_aligned)
                break;
        }

        // Still asking after the passes allowed. The next frame answers it, which is what keeps a
        // request that cannot converge from holding this one.
        if (!m_aligned)
            invalidate();
    }

    // Timed only while something is connected to FormPaintedEvent - see the event.
    void FormBase::wnd_paint(void* nativeContext, IntRect& dirtyRect, Graphics::Bitmap*& outData)
    {
        // AHEAD OF THE TIMING BELOW. Building a backend is not what a frame costs, and the frame
        // charged with one would stand in the FPS page's worst reading for the rest of the run.
        if (m_backendPending)
            stateBackend();

        const EventDispatcher& appEvents = m_appContext.events();
        if (!appEvents.hasListeners<FormPaintedEvent>())
        {
            paintWindow(nativeContext, dirtyRect, outData);
            return;
        }

        using Clock = std::chrono::steady_clock;
        const Clock::time_point startedAt = Clock::now();
        paintWindow(nativeContext, dirtyRect, outData);
        FormPaintedEvent event{ *this, dirtyRect, startedAt, Clock::now() };
        appEvents.emit(event);
    }

    HitTest FormBase::wnd_hitTest(PointInForm pt)
    {
        SearchControlResult searchResult = controlAt(pt);
        return searchResult.hitZone;
    }

    void FormBase::wnd_mouseMove(PointInForm pt)
    {
        bool posChanged = m_mousePos != pt;
        m_mousePos = pt;
        if (posChanged)
            mouseTick();
    }

    void FormBase::wnd_ncMouseDown(PointInForm pt, InputStamp stamp)
    {
        bool handled{};
        wnd_mouseDown(pt, stamp, handled);
        //Tooltip::handleUserInput();
        //closeActivePopup();
    }

    // The system takes this press for itself - it drags the window by its caption and sizes it
    // from its frame - so the framework sees nothing else of it, and a popup standing over this
    // form learns here that the user has gone elsewhere. A menu left up while the window it is
    // about is dragged out from under it is a menu about nothing.
    //
    // A popup that will not go keeps the press, which is the rule a press in the client area
    // follows too - see wnd_mouseDown. What it is refusing to lose is a value the user has still
    // to correct, and moving or sizing the form underneath takes the question with it.
    bool FormBase::wnd_systemMouseDown()
    {
        Tooltip::handleUserInput();
        closeActivePopup();
        return !m_activePopup;
    }

    void FormBase::wnd_mouseDown(PointInForm pt, InputStamp stamp, bool& /*handled*/)
    {
        // Tricky: instead of capturing the mouse by ourself we let Windows do it
        // by not touching the handled arg here
        //::SetCapture(window().handle());
        PressUpHandled handled = {};

        m_mouseDownPos = pt;
        m_mouseDownStamp = stamp;
        m_downItem = controlAt(m_mouseDownPos).control;

        Tooltip::handleUserInput();

        mouseTick(pt);
        if (Control* firstHotItem = Input::hoveredControl())
        {
            if (m_activePopup)
                if (m_activePopup->m_popupTarget != firstHotItem)
                {
                    closeActivePopup();
                    // The popup would not go, so the click that would have closed it does
                    // nothing else either. A click that neither dismisses the popup nor leaves
                    // the form behind it alone reads as the popup being ignored, and the popup
                    // is refusing precisely because it has something to say.
                    //
                    // BOTH HALVES OF THE CLICK. Returning here drops the press; the item the
                    // press went down on has to go with it, or the button-up finds it under the
                    // pointer still and fires the click that the press never started.
                    if (m_activePopup)
                    {
                        m_downItem = nullptr;
                        return;
                    }
                }
            Input::setMouseDown(*this, handled);
            if (handled.propagationStopped)
                return;
            mouseTick(pt, false);
            if (firstHotItem != Input::hoveredControl())
            {
                if (handled.downControlDirty)
                    m_downItem = Input::hoveredControl();
                Input::setMouseDown(*this, handled, true);
            }
        }
    }

    void FormBase::wnd_mouseUp(PointInForm pt)
    {
        PressUpHandled handled{};
        bool scrollIntoView = true;
        Input::setMouseUp(*this, m_downItem, handled, scrollIntoView);

        mouseTick(pt);

        if (SearchControlResult searhResult = controlAt(m_mousePos))
        {
            if (searhResult.control == m_downItem)
            {
                if (m_activePopup && m_activePopup->m_popupTarget == m_downItem)
                {
                    // it's the case when we are closing dropped down window by clicking the same button second time
                    closeActivePopup();
                    return;
                }

                if (!handled.propagationStopped)
                {
                    // What the pointer addresses, and nothing else. A release brings the control
                    // the user has just acted on fully into view; a press that landed on decoration
                    // or on a container's own surface acted on nothing, and a label cut by the edge
                    // of a scroll box has no business moving the view under the click.
                    if (m_downItem && scrollIntoView && m_downItem->respondsToPointer())
                        m_downItem->scrollIntoView();
                }

                if (!handled.preventClick)
                    if (searhResult.control->canClick())
                    {
                        ClickEvent clickParams{ *searhResult.control, *this };
                        searhResult.control->doClick(clickParams);
                    }
            }
        }
        // do not place any code here because this may be killed in the click()
    }

    void FormBase::wnd_doubleClick(PointInForm pt, InputStamp stamp)
    {
        // The same question a single press asks, and for the same reason. A double click that
        // reached the form behind an open popup would act on a control the popup is sitting on
        // top of; one the popup refuses to go for is the whole of the gesture.
        if (m_activePopup)
        {
            closeActivePopup();
            if (m_activePopup)
                return;
        }

        bool handled = false;
        if (SearchControlResult searhResult = controlAt(pt))
            if ((searhResult.control == Input::hoveredControl()) && (searhResult.control->enabled(true)))
            {
                DoubleClickEvent event{ *searhResult.control, *this };
                searhResult.control->doDoubleClick(event, pt);
                handled = event.propagationStopped();
            }
        if (!handled)
            wnd_mouseDown(pt, stamp, handled);
    }

    void FormBase::wnd_tripleClick(PointInForm pt, InputStamp stamp)
    {
        // The third press is a press whatever else it is, so everything the second one settles is
        // settled here the same way - see wnd_doubleClick.
        if (m_activePopup)
        {
            closeActivePopup();
            if (m_activePopup)
                return;
        }

        bool handled = false;
        if (SearchControlResult searhResult = controlAt(pt))
            if ((searhResult.control == Input::hoveredControl()) && (searhResult.control->enabled(true)))
            {
                TripleClickEvent event{ *searhResult.control, *this };
                searhResult.control->doTripleClick(event, pt);
                handled = event.propagationStopped();
            }
        // Most controls have nothing to say about a third press, and one that does not read it
        // gets the press it also is - so a run of clicks on a button is a run of clicks.
        if (!handled)
            wnd_mouseDown(pt, stamp, handled);
    }

    void FormBase::wnd_mouseLeave()
    {
        if (Input::hoveredControl() && &Input::hoveredControl()->form() == this)
            Input::setHoveredControl(nullptr);
    }

    void FormBase::wnd_contextMenu(PointInForm* pt, InputStamp stamp)
    {
        if (!pt)
        {
            Input::setDevice(m_appContext.animator(), InputDevice::Keyboard);
            if (m_activePopup)
            {
                m_activePopup->wnd_contextMenu(nullptr, stamp);
                return;
            }
        }
        closeActivePopup();
        // A popup that would not go keeps the form: opening a menu over it would overwrite the
        // registration that is the only way back to it, leaving it on screen and reachable by
        // nothing - not the next click, and not Escape.
        if (m_activePopup)
            return;

        if (pt)
        {
            m_mouseDownPos = *pt;
            // The stamp goes with the position: the press that raised this menu where the
            // platform names one, and nothing where it does not - never a press this one is not.
            m_mouseDownStamp = stamp;
            mouseTick(m_mouseDownPos);
        }
        update();

        // THE POINTER NAMES THE CONTROL A RIGHT-CLICK IS ABOUT; THE KEYBOARD NAMES THE FOCUSED
        // ONE. Reading the hover for both worked only by way of Input::setDevice carrying the
        // hover onto the focused control - and setDevice does that only when the device CHANGES,
        // so a second press with the keyboard already current asked whichever control the pointer
        // was last left over. The focus is read here instead, and a container answering for an
        // item hands the item over.
        Control* it;
        if (pt)
        {
            it = Input::hoveredControl();
            // A right click focuses what a left click would, before the control under it answers
            // - a control showing a menu of its own stops the walk, and nothing above it is told.
            Control* controlToFocus = it;
            while (controlToFocus && !controlToFocus->canTakeFocus())
                controlToFocus = controlToFocus->parent();
            if (controlToFocus)
                controlToFocus->setFocus();
        }
        else
        {
            it = Input::focusedControl();
            // One focus per application: a form that never took it sees a control in another
            // window, and that control is not what this form's key was about.
            if (it && &it->form() != this)
                it = nullptr;
            if (it)
                it = it->focusDelegate();
        }
        // Nothing to raise a menu for - an empty form, or a focus dropped with the control that
        // held it.
        if (!it)
            return;

        ContextPopupEvent event{ *it , *this, pt };
        while (it && !event.propagationStopped())
        {
            it->doContextPopup(event);
            it = it->parent();
        };

        // A RIGHT CLICK ON THE TITLE BAR THAT NOTHING CLAIMED IS THE WINDOW'S OWN MENU - the
        // system's, as on every title bar the system draws itself.
        if (pt && !event.propagationStopped() && wnd_hitTest(*pt) == HitTest::Title)
            m_window->showWindowMenu(*pt, stamp);
    }

    void FormBase::wnd_mouseWheel(PointInForm pt, const float wheelDelta)
    {
        mouseWheelOrHwheel(pt, wheelDelta, false);
    }

    void FormBase::wnd_mouseHWheel(PointInForm pt, const float wheelDelta)
    {
        mouseWheelOrHwheel(pt, wheelDelta, true);
    }

    void FormBase::mouseWheelOrHwheel(PointInForm, const float wheelDelta, bool h)
    {
        Tooltip::handleUserInput();
        Control* control = Input::hoveredControl();
        if (!control)
            return;
        MouseWheelEvent event{ *this, *control, wheelDelta };
        while (control && !event.handled)
        {
            if (h)
                control->mouseHWheel(event);
            else
                control->mouseWheel(event);
            control = control->parent();
        };
        //handleMouseMove(pt, false);
    }

    void FormBase::wnd_keyDown(KeyDownEvent& event)
    {
        // Cleared first, and unconditionally. The flag stands for one press; a press the system
        // translates to no character at all would otherwise leave it standing for the next one,
        // and that one would lose a character it was owed.
        m_popupTookKey = false;

        if (m_activePopup)
        {
            // SET BEFORE THE FORWARD, and never cleared after it. Two reasons, and each on its
            // own is enough:
            //
            // A NESTED PUMP MAY SPEND THE CHARACTER INSIDE THIS CALL. A menu item runs its
            // command before it closes the menu, and a command is free to open a dialog or an
            // in-place editor - that runs a message loop of its own, on this stack, and it
            // dispatches the character this press already queued. Set afterwards, the flag would
            // be set too late to be read.
            //
            // AND NOTHING HERE MAY READ `this` ONCE THE FORWARD RETURNS. A command is allowed to
            // take this form down with it, which is the same hazard the focused-control walk
            // below names.
            //
            // Left standing costs nothing. wnd_char spends it only after declining to forward
            // the character to a popup that is still up, and the next press clears it at the top
            // of this function.
            m_popupTookKey = true;
            m_activePopup->wnd_keyDown(event);
            return;
        }

        // A held key carries the animations this form is running, the same way a pointer move
        // carries them in mouseTick. UiTimer is WM_TIMER, the lowest priority message there
        // is, so it arrives only when nothing else is queued: a key repeating streams presses
        // and the repaint each one causes, and the timer is held off for as long as the key is
        // held. A glide already in flight - a caret scrolled into view by the previous press -
        // stands still until the key is let go without this.
        //
        // Ticked BEFORE the press is dispatched, so a handler that asks for a new glide is
        // measuring from where the last one has actually reached.
        m_appContext.animator().externalTimerTick();

        switch (event.key)
        {
        case Keys::Return:
        case Keys::Space:
            m_isKeyboardClick = true;
            break;
        }
        // Every key, modifiers included. Reaching for Ctrl or Shift is reaching for the keyboard
        // as much as reaching for a letter is, and the focus has to be visible by the time the key
        // that acts on it arrives - which for a modifier is the whole point of pressing it first.
        //
        // A key still down is not a reach for anything: the system repeats a held modifier many
        // times a second, and each repeat would take the device back from a pointer that is
        // moving, leaving the hover to swing between the focused item and the item under the
        // pointer for as long as the two are held together.
        //
        // A modifier does not carry the hover with it either. Only navigation has a reason to
        // move the hover, and a modifier names no destination - it qualifies the key that
        // follows, and that key moves the hover when it arrives.
        if (!event.isRepeat)
        {
            const bool isBareModifier = event.key == Keys::Shift
                || event.key == Keys::Ctrl
                || event.key == Keys::Alt;
            Input::setDevice(
                m_appContext.animator(),
                InputDevice::Keyboard,
                isBareModifier ? FocusTakesHover::No : FocusTakesHover::Yes);
        }

        if (!event.handled)
        {
            switch (event.key)
            {
            case Keys::Return:
                Tooltip::handleUserInput();
                break;
            }
        }

        // ESCAPE IS THE FOCUSED CONTROL'S BEFORE IT IS THE TOOLTIP'S. Dismissing a tooltip is
        // what Escape means when nothing else wants it, and the same test below says so once the
        // walk has passed. Answering it here as well took Escape away from any control with a
        // meaning of its own for it, but only while a tooltip happened to be up - an in-place
        // editor showing why a value was refused is exactly that state, and cancelling took two
        // presses.

        // processing focused item
        if (!event.handled)
        {
            Control* control = Input::focusedControl();
            if (!control)
                control = &m_content;
            if (control)
                control = control->focusDelegate();
            while (control && !event.handled)
            {
                // The next step is read BEFORE the handler runs. A key handler is allowed to do
                // something that outlives the press - open a menu, run an in-place editor - and
                // whatever it opens pumps messages, so the control the walk is standing on may
                // be deleted before it returns. Reading its parent afterwards would be reading a
                // control that has gone.
                //
                // TODO: the parent may go the same way, and this walk has no answer for that.
                // Should a handler that opens something modal be run after the walk unwinds
                // instead of inside it?
                Control* nextControl = control->parent();
                control->keyDown(event);
                control = nextControl;
            };
        }

        if (!event.handled)
        {
            switch (event.key)
            {
            case Keys::Space:
            case Keys::Return:
                Tooltip::handleUserInput();
                break;
            case Keys::Escape:
                event.handled = Tooltip::stopAndHide();
                break;
            }
        }

        // A shortcut is what a key means when nothing else wants it: the control holding the
        // focus has already had its walk, and the navigator has not seen the key yet. This form
        // answers first, then the application. An open popup never reaches here - the forward at
        // the top of this function hands the key to the popup, which runs the same pipeline and
        // so consults its own scope and then the application's.
        if (!event.handled)
            event.handled = m_actions.runShortcut(event, *this)
                || AppActions::get().runShortcut(event, *this);

        // ESCAPE CLOSES A WINDOW WHOSE CLOSING ENDS NOTHING. The form holding the root loop is
        // what the application stands on, so the key reaches the navigator there instead.
        if (!event.handled)
        {
            switch (event.key)
            {
            case Keys::Escape:
                if (!holdsRootLoop())
                {
                    if (!Tooltip::stopAndHide())
                        close();
                    event.handled = true;
                    return;
                }
                break;
            }

            FocusNavigator focusNavigator{ *this };
            focusNavigator.formKeyDown(event);
        }
    }

    void FormBase::wnd_keyUp()
    {
        // Cleared BEFORE the forward. A key that opened a popup on its way down has its way up
        // forwarded to that popup, and the flag would otherwise stay set here for as long as the
        // popup is open - Control::isPressed reads it, so every control on this form would read
        // pressed. Enter starting an in-place edit is exactly that key.
        m_isKeyboardClick = false;
        if (m_activePopup)
        {
            m_activePopup->wnd_keyUp();
            return;
        }

        Control* control = Input::focusedControl();
        if (!control)
            control = &m_content;
        if (!control)
            return;
        control = control->focusDelegate();

        while (control)
        {
            control->keyUp();
            if (control->interactivity() == Interactivity::Focusable)
                control->invalidateState();
            control = control->parent();
        };
    }

    // The device is the key-down's to name, not the character's: a character arrives from the
    // key press that produced it, and that press has already been read. Naming it again here
    // would undo the repeat filter, because a held letter repeats its character too.
    void FormBase::wnd_char(wchar_t character)
    {
        // A menu-class popup never takes the activation, so the characters typed into it arrive
        // at the form that opened it, exactly as its key presses do. The event carries a
        // FormContext, and a control in the popup must be given the popup's own: every form owns
        // its Direct2D device, and a resource made against one form is not valid on another.
        //
        // The forward is conditional on where the focus actually is, which the walk below is
        // conditional on too. The focus is one per application rather than one per window, so a
        // popup can be up with the focus outside it - clicking the control a popup was opened
        // over leaves it open and moves the focus - and forwarding then hands the POPUP's
        // context to a control of this form, which is the thing the paragraph above forbids.
        //
        // THE WHOLE CHAIN, not this form's own popup alone. Popups stack - a prompt raised from
        // inside a dialog stands on it and registers on ITS form - and a key press reaches the
        // innermost by being forwarded at every level. A character has to travel the same way, or
        // the focus two levels down never matches here, the forward is declined, and the flag
        // below spends the character the press already handed on.
        Control* control = Input::focusedControl();
        if (m_activePopup && control && isNestedPopup(&control->form()))
        {
            m_activePopup->wnd_char(character);
            return;
        }

        // THE CHARACTER OF A PRESS IS QUEUED BEFORE THE PRESS IS ANSWERED, so a key this form
        // handed to a popup that the key then CLOSED produced nothing for the control behind
        // it: the Enter that chose Select All is the MENU's, and the box the focus has just
        // come back to must not be given a newline by it. Marking the press handled cannot say
        // this - TranslateMessage has already run by the time anything answers - so the press
        // leaves a flag instead.
        //
        // TESTED AFTER THE FORWARD, so that a popup still up and still holding the focus is
        // given its characters as usual. Reading the flag clears it, so exactly one character
        // is dropped - and only where the popup has gone. A popup still up that does not hold
        // the focus took nothing: its walk started from the control that holds it, which is a
        // control of this form, and the character belongs where the press already went. An
        // in-place editor with its suggestion list up is that shape on every key typed.
        if (m_popupTookKey)
        {
            m_popupTookKey = false;
            if (!m_activePopup)
                return;
        }

        if (!control)
            control = &m_content;
        if (!control)
            return;
        if ((control = control->focusDelegate()))
        {
            CharPressEvent event{ m_context, *control, character };
            control->charPress(event);
        }
    }

    // Whether that form is this form's popup, or one nested on it at any depth. Each level
    // registers on the form below it, so the chain is walked rather than compared.
    bool FormBase::isNestedPopup(const FormBase* form) const
    {
        for (const FormBase* popup = m_activePopup; popup; popup = popup->m_activePopup)
        {
            if (popup == form)
                return true;
        }
        return false;
    }

    void FormBase::wnd_resize(IntSize surface, const WindowFrame& frame)
    {
        const IntSize margins = frame.margins.total();
        const ScaledDimensions placed =
            IntSize{ surface.x - margins.x, surface.y - margins.y }.toFloat();

        // A RESIZE THAT MOVED NOTHING IS NOT A RESIZE. A window's position is stated whether or
        // not it has changed - see Window::place - and the message stating it arrives here all
        // the same, so a placement pass that agreed with the window would throw away the
        // alignment it had just made, and the loop that asked for it could never tell that it
        // had finished. The surface follows from the two compared below, and Canvas::resize
        // drops the brush cache and the kept images whether the size moved or not.
        if (frame == m_frame and placed == m_placed.size)
        {
            return;
        }

        m_frame = frame;
        m_placed.size = placed;
        m_canvas.resize(surface);

        // THE PASS IS LEFT TO THE FRAME, AND THE FRAME IS ASKED FOR AT ONCE. On Win32 update()
        // paints inside this call, which is inside SetWindowPos: a sizing loop takes pointer
        // input ahead of anything posted, so a frame left to the queue waits for the pointer to
        // stop. On Wayland update() only marks the window, and the idle drains every queued
        // configure before it paints, so a drag costs one pass at the last size rather than one
        // per pointer step for sizes already gone.
        invalidateAlign();
        update();
    }

    const Graphics::ShadowPainter& FormBase::wnd_shadowPainter() const
    {
        return m_shadowPainter;
    }

    void FormBase::wnd_posChanged()
    {
        FormPositionChangeEvent event{ *this };
        emitEvent(event);
    }

    void FormBase::wnd_focusChanged()
    {
        // TODO: is invalidateState() needed here?
        //m_client->invalidateState();
        FormFocusChangeEvent event{ *this };
        emitEvent(event);

        // it's needed when our app is deactivated,
        // but that also closes the active menu
        // when it is resizing by user, which is a bummer
        closeActivePopup();
    }

    void FormBase::wnd_minimize()
    {
    }

    void FormBase::wnd_maximize()
    {
    }

    void FormBase::wnd_restore()
    {
    }

    void FormBase::wnd_setScalePercent(int value)
    {
        // The platform tells every window its own DPI, and a form standing on another is not
        // free to take it: it is drawn at that form's scale - see m_scaler - and follows the
        // change through the scaler it shares when the form underneath takes it.
        if (m_popupTargetForm)
            return;
        m_ownScaler.setSystemPercent(value);
    }

    void FormBase::wnd_closeRequested()
    {
        close();
    }

    void FormBase::forgetControl(Control* item)
    {
        if (m_downItem == item)
            m_downItem = nullptr;
        if (m_scrollIntoViewOnAlign == item)
            m_scrollIntoViewOnAlign = nullptr;
        // THE POPUP GOES WITH THE CONTROL IT IS OVER. A list rebuilt under an open in-place
        // editor deletes the item being edited, and a popup is placed, sized and aimed by that
        // control: what would be left is a window standing over nothing, with a modal loop still
        // running underneath it. Nothing else would ever take it down - the user cannot reach it,
        // and every ending it has runs through a control that no longer exists. Each rebuild
        // would leave one more on screen.
        //
        // NOT THROUGH closeActivePopup: that asks readyToClose, and there is nothing left to
        // argue about. A popup refuses to close in order to keep a value the user must correct,
        // and the place that value was going has just gone.
        //
        // The target is cleared FIRST, so the way down hands the focus to nobody rather than to a
        // control being destroyed - the popup reads it there. m_popupTargetForm carries the
        // de-registration and is untouched by this.
        if (m_activePopup && m_activePopup->m_popupTarget == item)
            dropActivePopup();
    }

    void FormBase::updatePlacement()
    {
        if (m_placementValid)
            return;

        m_placementValid = true;
        // BEFORE THE FIRST PLACEMENT, which is what reads it - a platform that names a window to
        // the display server has to do so before that window is first placed.
        if (!m_placementRestored && !m_configName.empty())
        {
            m_placementRestored = true;
            m_appContext.restoreFormPlacement(m_configName, *this);
        }
        initPlacement();
    }

    void FormBase::updateVisibility()
    {
        FormBase* popupTargetForm = nullptr;
        if (m_windowRole == WindowRole::Menu)
            popupTargetForm = this->popupTargetForm();

        if (visible())
        {
            updatePlacement();

            m_window->show();

            // REGISTERED BEFORE THE FOCUS MOVES. Both sides of a focus change are told as it
            // happens, and what the control this popup was opened on answers there - its
            // selected look, and a text box's caret - is read off activePopup(). Told after the
            // focus had already left, it answered as though nothing had been opened on it and
            // was never asked again: the box stopped its blink timer on the way out and then
            // stood with a caret that never blinked for as long as the menu was up.
            if (popupTargetForm)
            {
                popupTargetForm->m_activePopup = this;
                // The target is tested apart from the form it lives in. A popup may outlive the
                // control that opened it, and the two links go down separately - see
                // m_popupTargetForm.
                if (m_popupTarget)
                    m_popupTarget->invalidateState();
            }

            switch (m_windowRole)
            {
            case WindowRole::Tooltip:
                // non activated windows
                break;
            
            default:
                // the owner of the dialog may want to focus a default button
                // before executing, so we need to check if it's already focused
                const bool alreadyFocused = Input::focusedControl() && (&Input::focusedControl()->form() == this);
                if (!alreadyFocused)
                    focusEntryPoint();
            }
        }
        else
        {
            // WHILE THE WINDOW IS STILL UP, which is the only time its placement can be read.
            if (!m_configName.empty() && m_placementRestored)
                m_appContext.storeFormPlacement(m_configName, *this);
            m_window->hide();
            if (popupTargetForm)
            {
                if (popupTargetForm->m_activePopup == this)
                    popupTargetForm->m_activePopup = nullptr;
                if (m_popupTarget)
                    m_popupTarget->invalidateState();
            }
            if (m_popupTarget)
                m_popupTarget->setFocus();
        }
    }

    FormBase::FormBase(AppContext& appContext, WindowRole windowRole, FormControlBase& content,
        FormBase* ownerForm, Control* popupTarget, FormPlacement placement)
        :
        m_content{ content },
        // THE FORM UNDERNEATH OWNS THE SCALE. Its own pointer is taken rather than its scaler,
        // so a popup opened from a popup is drawn at the same scale the whole stack is, and a
        // form with nobody underneath is drawn at its own.
        m_scaler{ ownerForm ? ownerForm->m_scaler : &m_ownScaler },
        // Connected to whichever that is: a change to the scale this form is drawn at is a
        // change to this form, whether or not the scaler belongs to it.
        m_scalerConnection{ m_scaler->connectEvent<ScaleFactorChangeEvent>(this, &FormBase::scaleFactorChanged) }, // red squiggles in declaration

        m_windowRole{ windowRole },

        // The backend type belongs to the application - see Application<Platform, Backend>. It
        // reaches here as a factory on the AppContext, so this stays a single non-template
        // construction whatever the application chose. m_window is declared before m_canvas and is
        // built by then; the parameter is used rather than m_appContext, which is declared after.
        m_canvas{ appContext.createBackend(*m_window, m_placed.size.roundOut()) },

        m_appContext{ appContext },
        m_context{ appContext.platform(), appContext.theme(), appContext.bakedColors(), m_canvas,
            *m_scaler },
        m_themeSwitchConnection{
            appContext.events().connect<ThemeSwitchEvent>(this, &FormBase::themeSwitched)
        },
        m_backendSwitchConnection{
            appContext.events().connect<BackendSwitchEvent>(this, &FormBase::backendSwitched)
        },
        m_scaleSwitchConnection{
            appContext.events().connect<ScaleSwitchEvent>(this, &FormBase::scaleSwitched)
        },
        m_popupTargetForm{ ownerForm },
        m_popupTarget{ popupTarget },
        m_placement{ placement },

        // red squiggles in declaration
        m_placementRect{ m_popupTarget ? m_popupTarget->boundsInForm() : FloatRect{} }

    {
        setWindowTitle(appContext.appName().plainText());
        // The rect above and where the target stood when it was measured travel together from here
        // on - see followPopupTarget.
        rememberPlacementTarget();
    }

    void FormBase::initPlacement()
    {

        // A window standing on the pointer hangs off the point itself; one dropped from a control
        // stands clear of what it dropped from.
        const bool onPointer = Input::device() == InputDevice::Mouse &&
            (m_placement == FormPlacement::ContextMenu || m_placement == FormPlacement::Mouse);

        WindowPlacement request{};
        request.placement = m_placement;
        request.anchorRect = placementAnchor();

        // A FRESH NEGOTIATION EVERY TIME. What the last placement refused says nothing about what
        // this one will, and a form still held to it could not grow back when what is in it
        // shrank - see adjustRootMetrics, which is the only reader.
        m_placedShortX = false;
        m_placedShortY = false;

        // WHAT THIS PLACEMENT STARTS FROM: the scale the content is about to be measured at, and
        // the window standing from the last one. The two disagree wherever a form has been handed
        // a scale since it was placed - see adjustRootMetrics, which reads the window in the
        // factor's own units.
        if constexpr (Diagnostic::Options::logFormPlacement)
        {
            diagnosticLog(std::format(
                L"place enter  factor {:.3f}  window {:.0f} {:.0f}  autoFit {}",
                scaler().factor(), m_placed.size.x, m_placed.size.y,
                m_autoFit == AutoFit::Yes),
                &m_content);
        }

        // THREE AT MOST, AND EACH PASS IS A WHOLE ANSWER: measure what the content asks for,
        // place the window on it, lay the content out into what it got. Two things send it round
        // again, and neither is knowable before the step that finds it.
        //
        // THE WINDOW MOVING UNDER THE PASS. Setting its bounds resizes it, and the resize
        // invalidates the alignment before place() returns; a scale taken from the monitor it
        // landed on does the same. Only a form with nobody underneath sees a scale change at all:
        // a popup is drawn at its parent's scale wherever it lands - see m_scaler.
        //
        // WHAT THE LAST STEP MEASURED. Laying the content out into the window is where a
        // ScrollBox discovers it needs a bar, and a bar makes the content WIDER than the window
        // just granted for it. A form sized by its content is then a scrollbar too narrow: the
        // strip comes out of the body instead, and every Fill item in it stands past the view
        // with its tail under the bar. That pass cannot ask for another placement from inside
        // itself - updateAlign marks the form aligned before it runs, so a request raised in it
        // is lost - so it is asked here, where what was measured and what was granted can be
        // compared.
        for (int pass = 0; pass < 3; ++pass)
        {
            // THE ASK PASS HAS TO ACTUALLY RUN. updateAlign returns at once on a form that is
            // already aligned, and the pass before this one left it that way - so without this
            // the size asked for is not a measurement at all: it is what the last ALIGN left on
            // the root, which is the window it was laid out into. The placement is then handed
            // back the window it already granted, agrees with itself, and the content that no
            // longer fits stands past it.
            m_aligned = false;
            updateAlign(true);
            request.size = m_content.dimensions();
            // THE FLOOR THE CONTENT STATES - the MinSize set on the root or anywhere below it,
            // composed up the tree, and zero where nobody set one. The placement reads it instead
            // of asking whether this form may be shortened.
            request.minSize = m_content.calculatedMinSize();
            request.maxSize = scaler().scale(m_content.maxSize());
            request.textOrigin = m_content.textOrigin();
            request.clearance = onPointer ? 0.0f : scaler().scale(m_dropdownClearance);
            request.screenMargin = scaler().scaled4;
            m_placed = m_window->place(request);

            // WHAT THE PASS ASKED FOR AND WHAT IT GOT. An ask that matches the content and a
            // grant short of it is the placement refusing; an ask already short of the content
            // is a measure that was bounded before it ran.
            if constexpr (Diagnostic::Options::logFormPlacement)
            {
                diagnosticLog(std::format(
                    L"place pass {}  ask {:.0f} {:.0f}  got {:.0f} {:.0f}"
                    L"  refused {} {}  aligned {}",
                    pass,
                    request.size.x, request.size.y,
                    m_placed.size.x, m_placed.size.y,
                    m_placed.size.x < request.size.x,
                    m_placed.size.y < request.size.y,
                    m_aligned),
                    &m_content);
            }

            // THE PLACEMENT MOVED SOMETHING THIS PASS WAS MEASURED AGAINST. Setting the window's
            // bounds resizes it, and the resize invalidates the alignment before place() has even
            // returned; a scale taken from the monitor it landed on does the same. Either way what
            // was just placed was measured against something that no longer holds, so nothing is
            // laid out into it and the next pass measures again.
            if (!m_aligned)
                continue;

            // ONCE REFUSED, REFUSED FOR THE REST OF THE CYCLE. The next pass asks for the size
            // that refusal produced - a list measured against the room, with the bar it needs -
            // and being granted THAT is not the placement changing its mind. Recomputed instead
            // of accumulated, the axis would read as granted again, the ceiling would come off,
            // and the next measurement would go back to asking for the height it cannot have.
            m_placedShortX = m_placedShortX || m_placed.size.x < request.size.x;
            m_placedShortY = m_placedShortY || m_placed.size.y < request.size.y;

            // LAID OUT AGAINST THE RECT IT ACTUALLY GOT, which is not always the one it asked
            // for: the size above came from what the content wanted, and the placement is free to
            // give it less - the room the side it took had, or what the work area could hold. A
            // window that could not grow has to wrap its text rather than cut it, and one that
            // was made shorter has to show what fell past the cut, which is the scrollbar coming
            // up. Only a pass measured against the window does either - see adjustRootMetrics.
            // A grant past the ask is not laid out into - see contentExtent.
            //
            // The alignment is put back in question deliberately: the pass above left it valid,
            // and its own guard would otherwise return before doing anything.
            m_aligned = false;
            updateAlign(false);

            // A form whose window is the USER'S stands at whatever its content came to, and this
            // is the end of it. A form whose window came from its content asks again while what
            // it measured in that window differs from what it asked for - which is how the bar
            // that pass put up gets a window wide enough to hold it. A grant past the ask is the
            // platform's rounding and asks for nothing.
            if (m_autoFit == AutoFit::No || m_calculatedSize == request.size)
                break;
        }
        // Aligned unless the last pass asked for another, which the frame answers - see
        // wnd_beforePaint.
        m_aligned = m_layoutPass.valid();
    }

    void FormBase::initialize()
    {
        // THE SIZE THE APPLICATION IS AT, so a window opened after a change matches the windows
        // already up. A form standing on another takes nothing here: it is drawn at that form's
        // scale and follows through the scaler it shares - see m_scaler.
        if (!m_popupTargetForm)
            m_ownScaler.setAppPercent(m_appContext.scalePercent());
        stateFrame();
    }

    // A FORM IS ENTERED, NOT STEPPED THROUGH. Where the focus belongs the moment the window
    // appears is the form's entry - the item its root is on, which is a dropdown's selected
    // entry - and the first item in the form when the root is on none, which is every root that
    // is not a container in its own right.
    //
    // Those are two different moves, and only the second one is a search. Tab means "leave what
    // I am on and take the next", so a Tab emulated from the entry lands one item PAST it: a
    // combobox opened on its third entry comes up with the fourth lit. Landing on the item is
    // therefore said outright, and the walk is what answers the case with nothing to return to -
    // there the root answers with itself, the search starts from it, and the first item is what
    // it finds.
    //
    // An entry that cannot take the focus - disabled, or no longer focusable since it was
    // recorded - is no entry, and falls to the same walk.
    void FormBase::focusEntryPoint()
    {
        Control* entry = currentItem();
        if (entry && entry != &m_content && entry->canTakeFocus())
        {
            entry->setFocus();
            return;
        }

        FocusNavigator focusNavigator{ *this };
        bool dummy{};
        KeyDownEvent emulatedEvent{ Keys::Tab, {}, false, dummy };
        focusNavigator.formKeyDown(emulatedEvent);
    }

    WindowFrame FormBase::designFrame() const
    {
        const ControlMetrics& metrics = m_content.metrics();
        const WindowShadow shadow = m_content.windowShadow();
        const Scaler& scaler = this->scaler();

        WindowFrame result{};
        result.radius = scaler.scaleF(metrics.radius);

        // The margins are the painter's reach, read after stateFrame has handed it the design.
        if (shadow.opacity > 0.0f && shadow.blur > 0.0f)
        {
            const Graphics::ShadowPainter::Reach reach = m_shadowPainter.reach();
            result.margins.left = static_cast<int>(std::ceil(reach.left));
            result.margins.top = static_cast<int>(std::ceil(reach.top));
            result.margins.right = static_cast<int>(std::ceil(reach.right));
            result.margins.bottom = static_cast<int>(std::ceil(reach.bottom));
        }
        return result;
    }

    Graphics::ShadowPainter::Design FormBase::shadowDesign() const
    {
        const ControlMetrics& metrics = m_content.metrics();
        const WindowShadow shadow = m_content.windowShadow();
        const Scaler& scaler = this->scaler();
        const Color rgb = bakedColors().windowShadow(m_content.colorRules()).toColor();
        const ColorByte alpha = static_cast<ColorByte>(std::lround(shadow.opacity * 255.0f));
        return {
            .shadow = {
                .color = Color{ rgb.red, rgb.green, rgb.blue, alpha },
                .blur = scaler.scaleF(shadow.blur),
                .offset = scaler.scaleF(shadow.offset),
                .spread = scaler.scaleF(shadow.spread)
            },
            .radius = scaler.scaleF(metrics.radius)
        };
    }

    // WHETHER THE FRAMES START FROM NOTHING follows the window's answer, and the answer can change
    // with the design: a window is composited once it has a corner to round or a shadow to cast.
    void FormBase::stateFrame()
    {
        m_shadowPainter.setDesign(shadowDesign());
        m_frameDesign = designFrame();
        m_window->setFrame(m_frameDesign);
        if (Graphics::IBackend* backend = m_canvas.backend())
            backend->setTransparentBase(m_window->wantsAlphaChannel());
    }

    // THE TOP OF A FRAME IS THE ONE PLACE A BACKEND MAY BE CHANGED. One frame is up to five
    // beginPaint/endPaint pairs - the dirty rect, and each corner square it touches painted again
    // under a clip - so a backend put in between two of them would leave the passes drawing into
    // different surfaces and hand back a bitmap only one of them wrote.
    //
    // Everything the old backend held goes with it - its brushes, its geometries, the images it
    // kept - which is what a resize already drops, so nothing above the canvas has to be told.
    // The whole surface is dirty by the time this runs, backendSwitched having asked for it, so
    // the frame this stands at the top of draws all of it again.
    void FormBase::stateBackend()
    {
        m_backendPending = false;
        m_canvas.setBackend(m_appContext.createBackend(*m_window,
            { m_canvas.width(), m_canvas.height() }));
    }

    ScaledPosition FormBase::frameOrigin() const
    {
        return {
            static_cast<float>(m_frame.margins.left),
            static_cast<float>(m_frame.margins.top)
        };
    }

    ScaledDimensions FormBase::contentExtent() const
    {
        if (m_autoFit == AutoFit::No)
            return m_placed.size;
        return {
            std::min(m_calculatedSize.x, m_placed.size.x),
            std::min(m_calculatedSize.y, m_placed.size.y)
        };
    }

    FloatRect FormBase::rootRect() const
    {
        return FloatRect::fromDimensions(frameOrigin(), m_content.dimensions());
    }

    // A CORNER IS PAINTED TWICE. A paint clears the rect it covers, and a rect reaching into a
    // corner square clears the notch outside the arc with it - the root's own rect does, on any
    // change of its state - while the main pass lays no shadow inside the root's rectangle. So
    // each square the dirty rect touches is cleared, given its shadow back and painted again with
    // the root's content under a clip shaped like that corner of the frame - a square with its
    // outer corner rounded - so outside the arc the shadow alone stands, whatever a child draws
    // there. A primary window's children stand right inside the border ring and reach into the
    // squares; a popup's padding keeps its children inside the arc, and it pays the pass for the
    // notch alone. The ring is drawn over the content and outside the clip, so its own edge is not
    // cut by the clip's. The clip is that small on purpose: a path clip costs a mask the size of
    // its bounds, and a mask the size of the window would be paid on every resize.
    void FormBase::paintWindow(void* nativeContext, const IntRect& dirtyRect, Graphics::Bitmap*& outData)
    {
        paintPass(nativeContext, dirtyRect, nullptr, outData);
        if (m_frame.radius <= 0.0f)
            return;

        const float radius = m_frame.radius;
        const FloatRect windowRect = rootRect();
        const IntRect bounds = windowRect.roundedOut();
        const int side = static_cast<int>(std::ceil(radius)) + 1;
        struct Corner
        {
            FloatPoint point;
            FloatPoint direction;
            IntRect square;
        };
        const std::array<Corner, 4> corners{ {
            { windowRect.topLeft(), { 1.0f, 1.0f },
                { bounds.left, bounds.top, bounds.left + side, bounds.top + side } },
            { windowRect.topRight(), { -1.0f, 1.0f },
                { bounds.right - side, bounds.top, bounds.right, bounds.top + side } },
            { windowRect.bottomRight(), { -1.0f, -1.0f },
                { bounds.right - side, bounds.bottom - side, bounds.right, bounds.bottom } },
            { windowRect.bottomLeft(), { 1.0f, -1.0f },
                { bounds.left, bounds.bottom - side, bounds.left + side, bounds.bottom } }
        } };

        // A quarter circle as one cubic - the usual 0.5523 of the radius on each handle.
        constexpr float k_handle = 0.5522847f;
        for (const Corner& corner : corners)
        {
            const IntRect rect = IntRect::intersection(corner.square, dirtyRect);
            if (rect.empty())
                continue;

            const FloatPoint at = corner.point;
            const FloatPoint d = corner.direction;
            const float extent = static_cast<float>(side);
            Graphics::PixelPath clip;
            clip.moveTo({ at.x, at.y + d.y * radius });
            clip.cubicTo(
                { at.x, at.y + d.y * (radius - k_handle * radius) },
                { at.x + d.x * (radius - k_handle * radius), at.y },
                { at.x + d.x * radius, at.y });
            clip.lineTo({ at.x + d.x * extent, at.y });
            clip.lineTo({ at.x + d.x * extent, at.y + d.y * extent });
            clip.lineTo({ at.x, at.y + d.y * extent });
            clip.close();
            paintPass(nativeContext, rect, &clip, outData);
        }
    }

    void FormBase::paintPass(void* nativeContext, const IntRect& rect, const Graphics::PixelPath* contentClip,
        Graphics::Bitmap*& outData)
    {
        m_canvas.beginPaint(nativeContext, rect);
        const FloatRect floatRect = rect.toFloat();
        paintShadow(floatRect, contentClip != nullptr);
        paintContent(floatRect, contentClip);
        m_canvas.endPaint(nativeContext, rect, outData);
    }

    // THE SHADOW LIVES IN THE MARGINS AND IN THE CORNER NOTCHES. The children cover the root's
    // rectangle, notches included, so a paint lays the shadow only on the parts of its rect outside
    // the rectangle, a strip at a time. A paint inside the rectangle lays none, and that is every
    // paint but a resize. The corner pass is the exception: it clears its square and paints only
    // inside the arc, so it lays the shadow over the square first, and the notch outside the arc
    // is shadow again. The shadow is cast around the whole silhouette, corners included, which is
    // why it goes on before any clip.
    void FormBase::paintShadow(const FloatRect& dirtyRect, bool cornerPass)
    {
        if (m_frame.margins.empty())
            return;
        const FloatRect windowRect = rootRect();
        // The painter skips the tiles the clip does not reach and reads the clip off the canvas,
        // whose own stack stands at the whole surface until a part is pushed.
        auto paintPart = [&](const FloatRect& part) {
            if (part.empty())
                return;
            m_canvas.pushClip(part);
            m_shadowPainter.paint(m_canvas, windowRect);
            m_canvas.popClip();
        };
        if (cornerPass)
        {
            paintPart(dirtyRect);
            return;
        }
        for (const FloatRect& part : partsOutside(dirtyRect, windowRect))
            paintPart(part);
    }

    void FormBase::scaleFactorChanged(ScaleFactorChangeEvent& event)
    {
        // Important: use the event's scaler and not m_scaler,
        // because it's called from constructor when no vmt yet expanded.
        // mouseDownPos adjusting is needed when dragging the window between displays,
        // so the cursor stays at the same point of the form
        const Scaler& scaler = event.sender();
        float k = scaler.factor() / scaler.prevFactor();
        m_mouseDownPos.x *= k;
        m_mouseDownPos.y *= k;
        stateFrame();
        invalidateAlign();
    }

    // AN ANIMATION IS NOT AN INVALIDATION: every tick of a crossing is a frame someone is
    // meant to see, so a form that is on the screen paints the tick it was given rather
    // than scheduling one. A scheduled paint is one posted message for any number of
    // invalidations - see FormWindow::schedulePaint - and a moving pointer's own messages
    // are dispatched ahead of it, so every tick but the last is swallowed and the window
    // arrives at the end of the crossing in one step. A preview is asked for by the
    // pointer, so it is the case that always meets a full queue.
    //
    // A SET TAKEN OUTRIGHT WAITS FOR THE SCHEDULED PAINT. It is raised inside
    // AppContext::takeTheme, and so inside whatever called that - a page being built, a view
    // state being restored - where a frame made on the spot shows that work half done.
    //
    // The invalidation comes first either way: a paint takes the dirty rect the invalidation
    // left and does nothing without one. A form nobody can see has no frame worth making.
    void FormBase::themeSwitched(ThemeSwitchEvent& event)
    {
        if (!visible())
            return;

        // Taken before the work rather than around it, so what is measured is the frame this
        // call makes and not the call. See Diagnostic::Options::logThemeCrossing
        const std::chrono::steady_clock::time_point startedAt =
            Diagnostic::Options::logThemeCrossing
                ? std::chrono::steady_clock::now()
                : std::chrono::steady_clock::time_point{};

        stateFrame();
        invalidate();
        if (event.kind() == ThemeSwitchKind::Taken)
            return;

        update();

        if constexpr (Diagnostic::Options::logThemeCrossing)
            logCrossingFrame(startedAt);
    }

    // MARKED HERE AND TAKEN AT THE NEXT FRAME - see stateBackend, which is where the canvas is
    // allowed to change what it draws through. A window nobody can see keeps the mark and swaps
    // on the paint its next showing asks for.
    void FormBase::backendSwitched(BackendSwitchEvent&)
    {
        m_backendPending = true;
        invalidate();
    }

    // TAKEN AT ONCE, and by a form with nobody under it alone. Every other form is drawn at the
    // scale of the form it stands on and follows through the scaler they share - except one
    // holding a scale of its own, which is gated out here by still naming the form under it.
    // See holdScale
    void FormBase::scaleSwitched(ScaleSwitchEvent&)
    {
        if (m_popupTargetForm)
            return;
        m_ownScaler.setAppPercent(m_appContext.scalePercent());
    }

    // A theme change is one set of colours coming over another rather than replacing it, and the
    // coming over is made in the colours themselves: every control resolves its own once per theme
    // still on the screen and mixes what each comes to - see ControlPaintContext. So the tree is
    // laid out and painted ONCE a frame, in whatever the crossing has reached by then, and what
    // the window shows part way through is live rather than a still of the frame before it.
    void FormBase::paintContent(const FloatRect& dirtyRect, const Graphics::PixelPath* contentClip)
    {
        ControlTreePainter::paint(m_content, &dirtyRect, contentClip);
    }

    void FormBase::reAlign(bool initPlacementMode)
    {
        m_aligned = true;
        m_layoutInProgress = true;
        m_layoutPass.revalidate();
        // What this pass is FOR, which the root reads through adjustRootMetrics: a form measuring
        // to ask for a window is bounded by nothing, a form laying out into the window it has is
        // bounded by that window. A measuring pass stays one through its align: that lays the
        // content out at its own size, and no width in it is one the window gave.
        m_measuringPlacement = initPlacementMode;
        // calculate() must be ALWAYS called – the align() implementations depend on it.
        m_content.calculate(*this);
        // What the content ASKED FOR, taken before the align gives it its extent instead.
        m_calculatedSize = m_content.dimensions();
        const ScaledDimensions newSize = initPlacementMode ? m_calculatedSize : contentExtent();
        m_content.align(m_context, m_layoutPass, frameOrigin(), newSize);
        m_measuringPlacement = false;
        m_aligned = true;
        m_layoutInProgress = false;
        stateSizeRange();
        contentMoved(m_content);
        // Cleared before the scroll rather than after it: scrolling moves controls and can ask
        // for another alignment, and a request left standing would be answered again by the pass
        // it caused.
        if (Control* scrollTarget = m_scrollIntoViewOnAlign)
        {
            m_scrollIntoViewOnAlign = nullptr;
            scrollTarget->scrollIntoView();
        }
        // A CONTROL LAID OUT AT A WIDTH THE PASS DID NOT MEASURE IT AGAINST. Put back in question
        // here, where the alignment has already been marked valid - a request raised inside the
        // pass is wiped by the line above the align. The window is not invalidated with it: the
        // caller runs the pass this asks for before the frame goes out, so a repaint asked for
        // here would be a second frame drawing what the first one already shows. See
        // AlignEvent::invalidatePass
        if (!m_layoutPass.valid())
            m_aligned = false;
        // WHAT A REQUEST FOR A SETTLED LAYOUT WAITS ON. Only a pass that leaves the form aligned
        // says so: a scroll asked for above may have asked for another pass, and a pass measuring
        // what to ask a window for lays nothing out into one.
        if (m_aligned && !initPlacementMode)
        {
            FormAlignedEvent event{ *this };
            emitEvent(event);
        }
    }

    void FormBase::updateAlign(bool initPlacementMode)
    {
        if (m_aligned)
            return;
        ScopedWaitCursor waitCursor{};
        reAlign(initPlacementMode);
    }

    FloatRect FormBase::placementAnchor() const
    {
        // A MENU THE POINTER RAISED STANDS ON THE POINTER, not on a control: an empty rect where
        // the pointer is, held clear of the arrow by what the cursor takes below its hot spot so
        // the first line is not underneath it. Read here rather than by the placement, which runs
        // again every time the target moves, by which time the pointer has gone elsewhere.
        if (Input::device() == InputDevice::Mouse &&
            (m_placement == FormPlacement::ContextMenu || m_placement == FormPlacement::Mouse))
        {
            FloatPoint pt = m_popupTargetForm->m_mousePos;
            if (m_placement == FormPlacement::Mouse)
            {
                const CursorInfo cursorInfo = Platform::getCursorInfo(CursorShape::Arrow);
                pt.offset(0.0f, static_cast<float>(cursorInfo.size.y - cursorInfo.hotSpot.y));
            }
            return FloatRect::fromDimensions(pt, {});
        }
        return m_placementRect;
    }

    void FormBase::stateSizeRange()
    {
        // WHAT THE USER MAY DRAG THIS WINDOW TO is the floor its content states and the maximum
        // it was given - the same range the placement gives way inside. A window is held up by
        // the MinSizes set inside it and by nothing else, so one that states none is held only by
        // whatever default the system keeps for a window nothing was stated for.
        const ScaledDimensions minSize = m_content.calculatedMinSize();
        const ScaledDimensions maxSize = scaler().scale(m_content.maxSize());
        if (minSize == m_statedMinSize && maxSize == m_statedMaxSize)
            return;
        m_statedMinSize = minSize;
        m_statedMaxSize = maxSize;
        m_window->setSizeRange(minSize, maxSize);
    }

    void FormBase::detachScaler(const Scaler& goingDown)
    {
        // A form drawn at a scaler further up the stack keeps it: the one going down is not the
        // one it reads.
        if (m_scaler != &goingDown)
            return;
        holdScale();
    }

    // Both the connection and the context named the scaler being left: the assignment drops the
    // old connection, and the context reads through a pointer for this one case.
    void FormBase::stateScaler(Scaler& scaler)
    {
        m_scaler = &scaler;
        m_scalerConnection = m_scaler->connectEvent<ScaleFactorChangeEvent>(
            this, &FormBase::scaleFactorChanged);
        m_context.setScaler(*m_scaler);
    }

    void FormBase::followPopupTarget2()
    {
        if (!m_popupTarget)
            return;
        if (rectOfControl(m_popupTarget, true).empty())
            return;
        if (m_placement == FormPlacement::Mouse || m_placement == FormPlacement::ContextMenu)
            return;

        // THE RECT FOLLOWS THE TARGET'S BOX, both where it is and how big it is. A placement
        // reads the rect's edges - Bottom drops the form below it - so a rect that tracks the
        // top-left and keeps its first size anchors to an edge the target no longer has: a
        // control that grew leaves the form over it, one that shrank leaves a gap. A scale
        // change moves both at once, which is where it shows.
        //
        // CARRIED AS A CHANGE rather than taken from the target, because the rect is not always
        // the target's bounds - a caller is free to state one - and what it states should move
        // and grow with the control rather than be replaced by it.
        const FloatRect targetBounds = m_popupTarget->boundsInForm();
        const FloatPoint step = targetBounds.topLeft() - m_placementTargetBounds.topLeft();
        const FloatPoint grew = {
            targetBounds.width() - m_placementTargetBounds.width(),
            targetBounds.height() - m_placementTargetBounds.height()
        };
        if (step == FloatPoint{} and grew == FloatPoint{})
            return;

        FloatRect rect = m_placementRect;
        rect.offset(step);
        rect.right += grew.x;
        rect.bottom += grew.y;
        setPlacementRect(rect);
        if (m_activePopup)
            m_activePopup->followPopupTarget2();
    }

    void FormBase::rememberPlacementTarget()
    {
        m_placementTargetBounds = m_popupTarget ? m_popupTarget->boundsInForm() : FloatRect{};
    }

    void FormBase::dropActivePopup()
    {
        FormBase* popup = m_activePopup;
        popup->m_popupTarget = nullptr;
        popup->close();
        m_activePopup = nullptr;
    }

    SearchControlResult FormBase::controlAt(Control& currentControl, PointInControl pt0, FloatRect clipRect)
    {
        // Only the child padding is needed here - the search descends into children and never
        // touches the control's own text.
        const ScaledPadding padding = currentControl.childInset(m_context, currentControl.scaledPadding(scaler()));
        // What the parent leaves open for this control, which for a scrolled child is a window
        // narrower than the child. Asked before the bounds are met, so a point the parent does
        // not show this control through belongs to whatever else lies under it.
        if (const Control* controlParent = currentControl.m_parent)
            controlParent->adjustChildClip(currentControl, clipRect);
        // Where the control is drawn, which is where the pointer meets it. See
        // Control::floatOffset - the search runs against the same rect the paint used.
        FloatRect baseRect = currentControl.boundsInParent();
        baseRect.offset(currentControl.floatOffset());
        clipRect.intersectWith(baseRect);
        if (!clipRect.contains(pt0))
            return { nullptr, {} };
        HitTestEvent hit{ .formContext = m_context, .point = pt0 };
        currentControl.hitTest(hit);
        if (hit.zone == HitTest::Transparent)
            return { nullptr, {}, HitTest::Transparent };

        const PointInControl parentPt = pt0;
        pt0 = pt0 - baseRect.topLeft();
        if (ControlSpan children = currentControl.controls(); !children.empty())
        {
            clipRect.offset(-padding);
            clipRect.offset(-baseRect.topLeft());
            PointInControl pt = pt0 - padding;

            // An overlay control is painted over its siblings, so the pointer meets it first. It
            // is also the only control that may be drawn away from where it was laid out, and the
            // range below is entered by the laid-out leading edge, so it is reached by hand: a
            // floating control is not in that range at all once the view has scrolled past where
            // it belongs.
            //
            // An entry may name a control at any depth, so the point is carried from this
            // control's content space into the one that control's own topLeft is measured in.
            // Both are translations of form space, so the step between them is the difference of
            // the two content origins - and those already carry any float offset above.
            for (const OverlayEntry& entry : currentControl.overlayControls())
            {
                Control& item = *Control::overlayControlOf(entry);
                if (!item.m_parent || !shownUpTo(item, currentControl))
                    continue;
                const FloatPoint step = contentOriginOfControl(item.m_parent) - contentOriginOfControl(&currentControl);
                FloatRect itemClip = clipRect;
                itemClip.offset(-step);
                if (SearchControlResult result = controlAt(item, pt - step, itemClip))
                    return result;
            }

            const ControlSpan::iterator rangeBegin = currentControl.firstChildInViewport(clipRect);
            for (ControlSpan::iterator it = rangeBegin; it != children.end(); ++it)
            {
                if (Control& item = **it; item.visible())
                {
                    if (item.isViewportEnd(clipRect.right, clipRect.bottom))
                        break;
                    if (SearchControlResult result = controlAt(item, pt, clipRect))
                        return result;
                }
            }
        }
        // Asked here and nowhere else on the way down: a text rect costs an AdjustTextRectEvent,
        // and only the control the search comes to rest on is asked about its own text.
        // baseRect is where the control was drawn, which is what parentPt was met against above.
        const bool overText = currentControl.textBounds(m_context, baseRect).contains(parentPt);
        return { &currentControl, pt0, hit.zone, overText };
    }

    SearchControlResult FormBase::controlAt(PointInForm pt)
    {
        return controlAt(
            m_content,
            pt, // conversion not needed, assuming that m_content->boundsInParent() is at {0, 0}
            m_content.boundsInParent()
        );
    }

}
