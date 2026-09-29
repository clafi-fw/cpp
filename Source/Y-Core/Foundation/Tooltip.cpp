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
        // A TOOLTIP HAS NO TOOLTIP OF ITS OWN. Every form builds one with itself and a tooltip
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

    // Called from the TOP of ~FormBase rather than left to the member teardown below it. The
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

    // THE HOVER HAS ITS SAY WHILE THE MOUSE DRIVES. The hover moves without the pointer moving:
    // a window put up under a still pointer is handed a move for it and the form beneath a leave
    // - see FormBase::wnd_mouseMove - and a hit-test after a scroll or an edit lands wherever the
    // pointer was left. None of that is the user pointing at anything, so nothing standing comes
    // down for it and nothing starts waiting. A hint a key raised beside a list that opened under
    // the pointer went down the frame it came up, taken for the pointer leaving.
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
        // A hint on screen about the hovered control, or about one it stands in, is asked again
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

    // THE FOCUS HAS ITS SAY WHILE THE KEYBOARD ALONE DRIVES. A click lands the focus as well, and
    // the pointer's hint is the one for that: the hover it reads is where the click went.
    void Tooltip::focusedControlChanged()
    {
        if (Input::mouse().active())
            return;
        pointedControlChanged();
    }

    // Wherever the pointer was left - unless it rests on the very item the focus is on, where
    // nothing has changed hands: a hint about that item is the hover's answer as much as the
    // focus's.
    void Tooltip::mouseTookOver()
    {
        Control* focused = Input::focusedControl();
        if (focused && focused->focusDelegate() == Input::hoveredControl())
            return;
        pointedControlChanged();
    }

    Control* Tooltip::pointedControl()
    {
        if (Input::mouse().active())
            return Input::hoveredControl();
        // The holder answers for the item inside it - a container holds the focus for its current
        // item, and the item is what the key landed on.
        Control* focused = Input::focusedControl();
        return focused ? focused->focusDelegate() : nullptr;
    }

    void Tooltip::pointedControlChanged()
    {
        Control* pointed = pointedControl();

        // A MESSAGE STANDS WHILE WHAT IS POINTED AT IS STILL THE CONTROL IT IS ABOUT - crossing
        // between two zones of one control is not the pointer going anywhere, and the window is
        // left exactly as it stands rather than taken down and put back up a moment later.
        //
        // Nor does a change arriving right after the message went up move it: the window the
        // message was raised from has gone down under a still pointer, and the move that hands
        // the pointer to this form says where it already was - see ContextMessage::justRaised.
        const Control* about = ContextMessage::control();
        if (about && (about == pointed || ContextMessage::justRaised()))
            return;

        // A hint already up about the control pointed at stands as it is: the mouse takes over
        // with the pointer resting on the item the focus is on, or the pointer comes to a control
        // whose hint was put up without the wait.
        if (pointed && control() == pointed)
            return;

        // The user has gone to something else, so the window goes - and the message goes with
        // it rather than waiting to be pointed at again, which is what makes a message a thing
        // said once.
        ContextMessage::forget();

        // THE ONE COMING DOWN IS NOT ALWAYS THE ONE ABOUT TO WAIT. Each form has a tooltip of its
        // own, and the pointer crosses from one form to another - a menu raised over the form it
        // was raised from is two windows. The one to take down is named by s_current rather than
        // looked for under the pointer, because it is the form the pointer has LEFT that is
        // holding a window up and a timer running, and a timer left running there would put a
        // hint up over a control the pointer is nowhere near.
        //
        // s_current IS NOT CLEARED HERE. A hidden window goes on being painted for the whole of
        // its fade, and while it has pixels it is still the tooltip on screen - which is what
        // Escape answers by dismissing. It stops being the one only when another takes over
        // below, or when stopAndHide finishes it off.
        //
        // The CONTROL is let go of, which is what hides the window and what tells the label to
        // keep the words it went out with rather than ask again - see TooltipLabel::getText. A
        // hint the pointer has left is about nothing from here on, exactly as one dismissed is.
        if (s_current)
        {
            s_current->m_timer.stop();
            s_current->m_form->setControl(nullptr);
        }

        if (!pointed)
            return;
        Tooltip& tooltip = pointed->form().tooltip();
        // The short wait is for a window still on screen: crossing a row of buttons reads as one
        // hint following the pointer rather than as a hint per button. It is this form's own
        // window that has to still be there - crossing from one window into another is not that
        // gesture, and answers with the full wait.
        tooltip.startWaiting(tooltip.stillVisible() ? MilliSeconds{ 110 } : MilliSeconds{ 1000 });
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

        // A tooltip already on screen keeps the pixels it has: a resize paints only the part of
        // the window that was newly exposed, so whatever stood there before is still standing
        // under the new text. Every caller whose content moves under a tooltip that is already
        // up asks for the repaint - a thumb dragged along a slider with its value on the hint,
        // and a control that answers with different text than it did last time.
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
            // something the user did, and a hint may be standing over another window at the time.
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

        // THE CONTROL IS ONE OF THIS FORM'S. The timer runs only while this tooltip is the one in
        // play, and the one in play is the pointed control's form's - see pointedControlChanged;
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
                // SHOWN BEFORE IT IS MEASURED OR PAINTED. While a form is not visible its label
                // answers for it with the words the window went out with - see
                // TooltipLabel::getText, which is how a window keeps them for the whole of its
                // fade - and a placement measures the label. Taken before this, the window is
                // sized to the PREVIOUS answer and the paint forced inside it puts that answer
                // back on the screen: a box cut short of the words in it, or one left as wide as
                // a longer hint that stood there before, holding a value the user has gone past.
                form.show();
                if (answerMoved)
                    // Forced wherever the window still has pixels of its own up, which is the
                    // whole of what the force is for: a window keeps what it has through a move
                    // and through a resize - only what is newly exposed is painted - so the
                    // answer it went out with would stand under the one coming in.
                    updatePosition(event.anchorRect, form.stillVisible());
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
