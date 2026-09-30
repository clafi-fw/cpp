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
        /// The split button as a whole. The inherited control is the part the press landed on.
        [[nodiscard]] SplitButton& button() const { return m_button; }
        /// Builds the popup, drops it under the button and runs it.
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
        /// Puts a command behind the strip. The strip takes the action's hint, its key and
        /// its availability - an unavailable command greys its own half of the button - and
        /// pressing the strip runs it, by mouse and by the dropdown keys alike.
        ///
        /// The strip is a DISPLAY presenter: what routes its press is this button, so the
        /// action is not wired to the strip's own click - see PresenterRole.
        void dropdownAction(Action&);
        [[nodiscard]] const Action* dropdownAction() const { return m_dropdownAction; }
    protected:
        // The strip is a command of its own where one stands behind it, and then it stands whether
        // or not the face's can be run. A strip handed to OnDropdown drops a list ABOUT the face's
        // command - the steps behind Undo - so it is half of that command and greys with it.
        [[nodiscard]] bool dropdownActsAlone() const override
        {
            return m_dropdownAction != nullptr;
        }
        virtual void dropdown(DropdownEvent&);
        void showDropdown(Control& initiator) override;
        void adjustPaint(AdjustPaintEvent&) override;
        // The mark sits on a button face rather than in a field, so it carries the normal text
        // colour rather than the muted one a combo box uses.
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
            // The base sits on ButtonBase so that a combo box can share it without being handed a
            // button's metrics, so a split button asks for a tool button's here.
            Interactivity::Focusable,
            params.themeMetrics().toolButton,
            std::forward<Args>(args)...
        }
    {
    }

    void SplitButton::dropdownAction(Action& action)
    {
        Control* part = secondaryPart();
        // Whatever stood here stops standing for anything. Left attached it would go on
        // answering for the strip's hint and its state behind the command that replaced it.
        if (m_dropdownAction && part)
            m_dropdownAction->detach(*part);

        m_dropdownAction = &action;
        if (!part)
            return;
        action.attach(*part, PresenterRole::Display);
        // The strip now acts alone - see dropdownActsAlone - so its availability is asked again.
        part->invalidateState();
    }

    void SplitButton::dropdown(DropdownEvent& event)
    {
        // The command's presenter is the strip, whichever way the dropdown was reached.
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
        // TWO TARGETS ON ONE FACE, so the surface arrives with the pointer: one at rest would
        // draw a single box around both halves, and the seam between them is what has to read.
        // An element passed in is read here or not at all - SplitButtonBase stops short of
        // RichControl.
        event.setColorRules(colorRules().value_or(UiElement::ToolButton));
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
        return m_button.dropPopup<ControlClass>(form, std::forward<Args>(args)...);
    }

}
