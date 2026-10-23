module ClaFi.Core.Foundation;

import :ContextMessage;
import :Tooltip;
import :Control;
import :Form;
import :Input;

import :TooltipForm;

import ClaFi.StdLib;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Context.FormContext;

namespace ClaFi
{
    Tooltip::Tooltip(FormBase& ownerForm)
        :
        m_ownerForm{ ownerForm },
        // **A tooltip has no tooltip of its own.** Every form builds one with itself and a tooltip
        // window is a form like any other, so this is where that recursion ends.
        //
        // Built here rather than at the first hover, because a form owning its own window is what
        // this is for and there is nothing to wait for: the owner's window is standing by the time
        // its last member is initialized, which is what makes this the last member.
        m_form{
            ownerForm.windowRole() == WindowRole::Tooltip
                ? nullptr
                : std::make_unique<TooltipForm>(ownerForm)
        },
        m_timer{ OnEvent{ [this](TimerEvent&) { showOrHide(nullptr); } } }
    {
    }

    Tooltip::~Tooltip()
    {
        destroyForm();
    }

    // Called from the **top** of ~FormBase rather than left to the member teardown below it. The
    // window this one is owned by is about to go, and the system destroys an owned window along
    // with its owner - a tooltip taken down after that would be destroying a handle that had
    // already been taken. Reaching the owner form from here also reaches it while it is whole.
    void Tooltip::destroyForm()
    {
        m_timer.stop();
        if (s_current == this)
            s_current = nullptr;
        m_form.reset();
    }

    void Tooltip::forgetControl(const Control* value)
    {
        // A control that is going takes with it anything raised about it: a message names the
        // control it stands under, and this one is about to stop being anywhere.
        if (ContextMessage::control() == value)
            ContextMessage::forget();
        if (control() == value)
            stopAndHide();
    }

    // **Asked again in place**, as a tooltip is when the pointer crosses into another zone of its
    // control: a new answer is measured and placed afresh, and no answer at all takes it down.
    void Tooltip::controlTooltipChanged(Control& value)
    {
        if (control() != &value)
            return;
        s_current->showOrHide(&value);
    }

    // **The hover has its say while the mouse drives.** The hover moves without the pointer moving:
    // a window put up under a still pointer is handed a move for it and the form beneath a leave
    // - see FormBase::wnd_mouseMove - and a hit-test after a scroll or an edit lands wherever the
    // pointer was left. None of that is the user pointing at anything, so nothing standing comes
    // down for it and nothing starts waiting. A tooltip a key raised beside a list that opened
    // under the pointer went down the frame it came up, taken for the pointer leaving.
    void Tooltip::hoveredControlChanged()
    {
        if (!Input::mouse().active())
            return;
        pointedControlChanged();
    }

    void Tooltip::hoveredZoneChanged()
    {
        if (!Input::mouse().active())
            return;
        Control* hovered = Input::hoveredControl();
        const Control* shown = control();
        // A tooltip on screen about the hovered control, or about one it stands in, is asked again
        // in place. An unchanged answer keeps the window where it stands, and an answer the new
        // zone ends - the control's own text shown over it - takes it down.
        for (Control* it = hovered; it && shown; it = it->parent())
        {
            if (it == shown)
            {
                s_current->showOrHide(hovered);
                return;
            }
        }
        pointedControlChanged();
    }

    // **The focus has its say while the keyboard alone drives.** A click lands the focus as well,
    // and the pointer's tooltip is the one for that: the hover it reads is where the click went.
    void Tooltip::focusedControlChanged()
    {
        if (Input::mouse().active())
            return;
        pointedControlChanged();
    }

    // Wherever the pointer was left - unless it rests on the very item the focus is on, where
    // nothing has changed hands: a tooltip about that item is the hover's answer as much as the
    // focus's.
    void Tooltip::mouseTookOver()
    {
        Control* delegate = Input::focusDelegate();
        if (delegate && delegate == Input::hoveredControl())
            return;
        pointedControlChanged();
    }

    Control* Tooltip::pointedControl()
    {
        if (Input::mouse().active())
            return Input::hoveredControl();
        // The item the key landed on, which a container holds the focus for.
        return Input::focusDelegate();
    }

    void Tooltip::pointedControlChanged()
    {
        Control* pointed = pointedControl();

        // **A message stands while what is pointed at is still the control it is about** - crossing
        // between two zones of one control is not the pointer going anywhere, and the window is
        // left exactly as it stands rather than taken down and put back up a moment later.
        //
        // Nor does a change arriving right after the message went up move it: the window the
        // message was raised from has gone down under a still pointer, and the move that hands
        // the pointer to this form says where it already was - see ContextMessage::justRaised.
        const Control* about = ContextMessage::control();
        if (about && (about == pointed || ContextMessage::justRaised()))
            return;

        // A tooltip already up about the control pointed at stands as it is: the mouse takes over
        // with the pointer resting on the item the focus is on, or the pointer comes to a control
        // whose tooltip was put up without the wait.
        if (pointed && control() == pointed)
            return;
        // Nor does one about a control the pointer has gone further into: crossing children that
        // leave the answer to their container is not leaving the container.
        if (pointed && answersFor(*pointed))
            return;

        // The user has gone to something else, so the window goes - and the message goes with
        // it rather than waiting to be pointed at again, which is what makes a message a thing
        // said once.
        ContextMessage::forget();

        // **The one coming down is not always the one about to wait.** Each form has a tooltip of
        // its own, and the pointer crosses from one form to another - a menu raised over the form
        // it was raised from is two windows. The one to take down is named by s_current rather than
        // looked for under the pointer, because it is the form the pointer has **left** that is
        // holding a window up and a timer running, and a timer left running there would put a
        // tooltip up over a control the pointer is nowhere near.
        //
        // s_current **is not cleared here.** A hidden window goes on being painted for the whole of
        // its fade, and while it has pixels it is still the tooltip on screen - which is what
        // Escape answers by dismissing. It stops being the one only when another takes over
        // below, or when stopAndHide finishes it off.
        //
        // The **control** is let go of, which is what hides the window and what tells the label to
        // keep the words it went out with rather than ask again - see TooltipLabel::getText. A
        // tooltip the pointer has left is about nothing from here on, exactly as one dismissed is.
        if (s_current)
        {
            s_current->m_timer.stop();
            s_current->m_form->setControl(nullptr);
        }

        if (!pointed)
            return;
        Tooltip& tooltip = pointed->form().tooltip();
        // The short wait is for a window still on screen: crossing a row of buttons reads as one
        // tooltip following the pointer rather than as a tooltip per button. It is this form's own
        // window that has to still be there - crossing from one window into another is not that
        // gesture, and answers with the full wait.
        tooltip.startWaiting(tooltip.stillVisible() ? MilliSeconds{ 110 } : MilliSeconds{ 1000 });
    }

    bool Tooltip::answersFor(Control& pointed)
    {
        const Control* shown = control();
        Control* it = pointed.parent();
        while (it && it != shown)
            it = it->parent();
        if (!it)
            return false;
        // Asked as showOrHide asks it. A control between them with a tooltip of its own says that
        // instead, and the tooltip's own control may answer differently from where the pointer is.
        const TooltipForm& form = *s_current->m_form;
        Text text{};
        for (it = &pointed; it; it = it->parent())
        {
            GetTooltipEvent event{ form.context(), *it, text, EventPhase::Calculate };
            if (!ContextMessage::answer(*it, event))
                it->nestedGetTooltip(event);
            if (it == shown || !text.empty())
                break;
        }
        return it == shown && text == form.measuredText();
    }

    void Tooltip::startWaiting(MilliSeconds delay)
    {
        s_current = this;
        m_timer.start(delay);
    }

    void Tooltip::updatePosition(const FloatRect& anchorRect, bool forceRepaint)
    {
        TooltipForm& form = *m_form;
        if (!form.control())
            return;
        // Where the control comes to rest rather than where a glide has carried it so far - see
        // TooltipForm::placementTargetBounds.
        FloatRect restingRect = anchorRect;
        restingRect.offset(-form.control()->viewTravelRemaining());
        form.setPlacementRect(restingRect);

        // TODO: move this OverText height adjustment into form.initPlacement().
        //if (form.placement() == FormPlacement::OverText)
        //{
        //    float d = anchorRect.height() - (form.contentHeight() - form.content().scaledPadding().y * 2.0f);
        //    if (d > 0.0f)
        //    {
        //        IntRect frmBounds = form.window().bounds();
        //        frmBounds.bottom += static_cast<int>(std::ceil(d));
        //        form.window().setBounds(frmBounds);
        //    }
        //}

        // A tooltip already on screen keeps the pixels it has until something invalidates them. A
        // caller whose text changes under a window nothing else invalidated asks for the repaint:
        // a thumb dragged along a slider with its value on the tooltip, in a window measured once
        // for the widest value.
        if (forceRepaint)
        {
            form.invalidate();
            form.update();
        }
    }

    void Tooltip::showRightNow(Control& target)
    {
        if (control() == &target)
        {
            updatePosition(anchorOf(target), true);
        }
        else
        {
            // Whatever is up goes first, and it is not always this form's: a message is raised by
            // something the user did, and a tooltip may stand over another window at the time.
            stopAndHide();
            showOrHide(&target);
        }
    }

    FloatRect Tooltip::anchorOf(Control& target) const
    {
        // Asked as showOrHide asks it, so a message about the control keeps the rect it states.
        Text text{};
        GetTooltipEvent event{ m_form->context(), target, text, EventPhase::Calculate };
        if (!ContextMessage::answer(target, event))
            target.nestedGetTooltip(event);
        return event.anchorRect;
    }

    bool Tooltip::stopAndHide()
    {
        if (!s_current)
            return false;
        Tooltip& tooltip = *s_current;
        s_current = nullptr;
        tooltip.m_timer.stop();
        const bool result = tooltip.m_form->stillVisible();
        tooltip.m_form->setControl(nullptr);
        return result;
    }

    void Tooltip::handleUserInput()
    {
        if (!s_current)
            return;
        if (!s_current->m_form->hideOnUserInput())
            return;
        // The message was raised by the last thing the user did, and this is the next one.
        ContextMessage::forget();
        stopAndHide();
    }

    Control* Tooltip::control()
    {
        if (!s_current)
            return nullptr;
        TooltipForm& form = *s_current->m_form;
        if (!form.visible())
            return nullptr;
        return form.control();
    }

    void Tooltip::showOrHide(Control* it)
    {
        if (!it)
            it = pointedControl();
        if (!it)
            return;

        // **The control is one of this form's.** The timer runs only while this tooltip is the one
        // in play, and the one in play is the pointed control's form's - see pointedControlChanged;
        // the other way in is showRightNow, which a control reaches through its own form.
        TooltipForm& form = *m_form;

        Text tmpText{};

        while (it)
        {
            tmpText.clear();
            GetTooltipEvent event{ form.context(), *it, tmpText, EventPhase::Calculate };
            // A message raised about this control stands in front of whatever it would say for
            // itself, and is the whole of the answer wherever it stands.
            if (!ContextMessage::answer(*it, event))
                it->nestedGetTooltip(event);
            if (!tmpText.empty())
            {
                // A control is free to answer differently from one hover to the next - a toggle
                // names what a press would do now, and that changes when it is pressed. The form
                // holds the layout it was given, not the one it would be given, so a moved answer
                // has to re-place it exactly as a moved control does: the text is what the size
                // was measured from.
                const bool answerMoved = form.control() != it or form.measuredText() != tmpText;
                if (answerMoved)
                    form.setControl(*it, event);
                // **Shown before it is measured or painted.** While a form is not visible its label
                // answers for it with the words the window went out with - see
                // TooltipLabel::getText, which is how a window keeps them for the whole of its
                // fade - and a placement measures the label. Taken before this, the window is
                // sized to the **previous** answer and the paint forced inside it puts that answer
                // back on the screen: a box cut short of the words in it, or one left as wide as
                // a longer tooltip that stood there before, holding a value the user has gone past.
                form.show();
                if (answerMoved)
                    // The new answer is on its way to the screen: setControl invalidated the
                    // whole window, and a placement paints a window for its rect before the
                    // window takes it. Nothing is forced here.
                    updatePosition(event.anchorRect, false);
                s_current = this;
                return;
            }
            else
                it = it->parent();
        }
        form.setControl(nullptr);
    }

    bool Tooltip::stillVisible() const
    {
        return m_form->stillVisible();
    }

}
