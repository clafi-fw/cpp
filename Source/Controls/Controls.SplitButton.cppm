module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.SplitButton;

export import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;

import ClaFi.StdLib;
import ClaFi.Core.System.Events;


namespace ClaFi::Controls
{
    export class SplitButton;

    // DropdownEvent

    // Raised when the dropdown of a SplitButton is activated. See Controls
    export class DropdownEvent : public ClickEventBase
    {
    public:
        DropdownEvent(SplitButton&, Control& initiator, FormBase&);
        /// The split button as a whole. The inherited control member is what the press landed on,
        /// and that is what the popup must be owned by.
        [[nodiscard]] SplitButton& button() const { return m_button; }
        /// Builds the popup, drops it under the button and runs it. A handler that needs to
        /// configure the form itself can call form.createPopup() - see the note on
        /// DropdownControlBase::dropPopup about which control has to own it.
        template <ClassOfFormControl ControlClass, typename... Args>
        int executeDropdown(Args&&...);
    private:
        SplitButton& m_button;
    };

    // SplitButton

    // A button whose dropdown is a second action alongside the primary one. See Controls
    export class SplitButton : public DropdownControlBase
    {
        // Reaches the inherited dropPopup() on the button's behalf.
        friend DropdownEvent;
    public:
        template <typename... Args>
        SplitButton(const CreateParams&, Args&&...);
    public:
        // Raised when the dropdown of a SplitButton is activated. See Controls
        DECLARE_EVENT(DropdownEvent, OnDropdown, onDropdown)
    public:
        std::wstring_view diagnosticText() const override { return L"SplitButton"; }
        /// Puts a command behind the strip. The strip takes the action's tooltip, its key and
        /// its availability - an unavailable command greys its own half of the button - and
        /// pressing the strip runs it, by mouse and by the dropdown keys alike.
        ///
        /// The strip is a DISPLAY presenter: what routes its press is this button, so the
        /// action is not wired to the strip's own click - see PresenterRole.
        void dropdownAction(Action&);
        [[nodiscard]] const Action* dropdownAction() const { return m_dropdownAction; }
    protected:
        // The two halves are two commands, which is the whole of what a split button is. So the
        // strip stands whether or not the face's command can be run.
        [[nodiscard]] bool dropdownActsAlone() const override { return true; }
        virtual void dropdown(DropdownEvent&);
        void showDropdown(Control& initiator) override;
        void adjustPaint(AdjustPaintEvent&) override;
        // The mark sits on a button face rather than in a field, so it carries the normal text
        // colour rather than the muted one a combobox uses.
        [[nodiscard]] Ink dropdownMarkInk() const override { return InkWell::textInk(); }
    private:
        Action* m_dropdownAction{ nullptr };
    };


    //-------------------------------------------------------------------------


    // SplitButton

    template<typename ...Args>
    SplitButton::SplitButton(const CreateParams& params, Args && ...args)
        :
        DropdownControlBase{
            params,
            // The base sits on ButtonBase so that a combobox can share it without being handed a
            // button's metrics, so a button asks for them here.
            Interactivity::Focusable,
            params.themeMetrics().button,
            std::forward<Args>(args)...
        }
    {
    }

    void SplitButton::dropdownAction(Action& action)
    {
        Control* part = secondaryPart();
        // Whatever stood here stops standing for anything. Left attached it would go on
        // answering for the strip's tooltip and its state behind the command that replaced it.
        if (m_dropdownAction && part)
            m_dropdownAction->detach(*part);

        m_dropdownAction = &action;
        if (part)
            action.attach(*part, PresenterRole::Display);
    }

    void SplitButton::dropdown(DropdownEvent& event)
    {
        // control is what the press landed on - the strip, whichever way the dropdown was
        // reached - and handing it over is what puts the command's own popup under it.
        if (m_dropdownAction)
        {
            m_dropdownAction->invoke(event.form, event.control, event.stamp);
            return;
        }

        emitEvent(event);
    }

    void SplitButton::showDropdown(Control& initiator)
    {
        DropdownEvent event{ *this, initiator, form() };
        dropdown(event);
    }

    void SplitButton::adjustPaint(AdjustPaintEvent& event)
    {
        DropdownControlBase::adjustPaint(event);
        event.setColorRules(UiElement::Button);
    }

    // DropdownEvent

    DropdownEvent::DropdownEvent(SplitButton& button, Control& initiator, FormBase& form)
        :
        ClickEventBase{ initiator, form },
        m_button{ button }
    {
    }

    template<ClassOfFormControl ControlClass, typename ...Args>
    int DropdownEvent::executeDropdown(Args&&... args)
    {
        // control is what the press landed on, which is what has to own the popup.
        return m_button.dropPopup<ControlClass>(form, *control, std::forward<Args>(args)...);
    }

}
