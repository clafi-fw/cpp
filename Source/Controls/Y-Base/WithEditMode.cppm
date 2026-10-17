module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.WithEditMode;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Whether the control's own keys apply, or the keys travel past it. See Controls-Base
    export enum class EditMode
    {
        No,
        Yes
    };

    // Whether the user switches the edit mode, or the control stays in it. See Controls-Base
    export enum class EditModeSwitching
    {
        No,    // in edit mode throughout - a box in a dialog, an editor
        Yes    // Return and a press turn it on, Escape and leaving turn it off
    };

    // A control whose own keys apply in edit mode only. See Controls-Base
    export template <IsControl HostClass>
        class WithEditMode : public HostClass
    {
    public:
        template <typename... Args>
        explicit WithEditMode(const CreateParams&, Args&&...);
    public:
        // The mode the control is in; EditModeSwitching::No holds it at Yes.
        DECLARE_WRITABLE_PROPERTY(EditMode, editMode, setEditMode, EditMode::No)
        // Whether the mode is the user's to switch.
        DECLARE_PROPERTY(EditModeSwitching, editModeSwitching, EditModeSwitching::No)
    public:
        void setEditMode(EditMode);
        [[nodiscard]] bool isInEditMode() const { return m_editMode == EditMode::Yes; }
    protected:
        // The host's keys, asked in edit mode only and ahead of the mode's own answer.
        virtual void editModeKeyDown(KeyDownEvent&) {}
        // The mode has turned on; what Escape puts back is read here.
        virtual void editModeEntered() {}
        // The mode has turned off; accepted is false for Escape alone.
        virtual void editModeLeft(bool /*accepted*/) {}
        // Whether Return is a line break the host types, which the mode then leaves to it.
        [[nodiscard]] virtual bool takesLineBreaks() const { return false; }
        void adjustPaint(AdjustPaintEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedContextPopup(ContextPopupEvent&) override;
        void focusChanged() override;
    private:
        void enterEditMode();
        void leaveEditMode(bool accepted);
    private:
        // The Return that turned the mode on has its character still to come.
        bool m_entryCharPending{ false };
    };


    //-------------------------------------------------------------------------


    // WithEditMode

    template <IsControl HostClass>
    template <typename... Args>
    WithEditMode<HostClass>::WithEditMode(const CreateParams& params, Args&&... args)
        :
        HostClass{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(editMode),
        INIT_PROPERTY(editModeSwitching)
    {
        if (m_editModeSwitching == EditModeSwitching::No)
            m_editMode = EditMode::Yes;
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::setEditMode(const EditMode value)
    {
        if (value == EditMode::Yes)
            enterEditMode();
        else
            leaveEditMode(true);
    }

    // Temporary edit mode indication - to be done right later.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::adjustPaint(AdjustPaintEvent& event)
    {
        if (m_editModeSwitching == EditModeSwitching::Yes && isInEditMode())
        {
            event.setColorRules(UiElement::Button);
            event.setSurfaceHsl({ 0.69f, 0.5f, 0.5f });
        }
        HostClass::adjustPaint(event);
    }

    // The host answers first, so Return on a link at the caret is the host's before it is the
    // mode's. A host that takes line breaks types Return as one, and the mode leaves it alone.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::nestedKeyDown(KeyDownEvent& event)
    {
        m_entryCharPending = false;
        HostClass::nestedKeyDown(event);
        if (event.handled || event.propagationStopped())
            return;
        if (!isInEditMode())
        {
            if (event.key == Keys::Return && event.modifiers.empty())
            {
                enterEditMode();
                m_entryCharPending = true;
                event.handled = true;
            }
            return;
        }
        editModeKeyDown(event);
        if (event.handled || event.propagationStopped())
            return;
        if (m_editModeSwitching == EditModeSwitching::No)
            return;
        switch (event.key)
        {
        case Keys::Return:
            if (takesLineBreaks())
                break;
            leaveEditMode(true);
            event.handled = true;
            break;
        case Keys::Escape:
            leaveEditMode(false);
            event.handled = true;
            break;
        default:
            break;
        }
    }

    // A character typed outside the mode is not the host's, and neither is the one the Return
    // that turned the mode on leaves behind.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::charPress(CharPressEvent& event)
    {
        if (m_entryCharPending)
        {
            m_entryCharPending = false;
            return;
        }
        if (!isInEditMode())
            return;
        HostClass::charPress(event);
    }

    // A press is the pointer's Return.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::nestedPressDown(PressDownEvent& event)
    {
        HostClass::nestedPressDown(event);
        enterEditMode();
    }

    // A menu on the control is about what the mode shows. A host that answers the request ahead
    // of this turns the mode on itself - see TextBox::nestedContextPopup.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::nestedContextPopup(ContextPopupEvent& event)
    {
        enterEditMode();
        HostClass::nestedContextPopup(event);
    }

    // Leaving accepts. A popup opened on the control - an edit menu, a link's menu - holds the
    // focus for a while and keeps the mode up.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::focusChanged()
    {
        HostClass::focusChanged();
        if (!HostClass::isFocused() && !HostClass::isDroppedDown())
            leaveEditMode(true);
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::enterEditMode()
    {
        if (isInEditMode())
            return;
        m_editMode = EditMode::Yes;
        HostClass::invalidate();
        editModeEntered();
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::leaveEditMode(const bool accepted)
    {
        if (!isInEditMode() || m_editModeSwitching == EditModeSwitching::No)
            return;
        m_editMode = EditMode::No;
        HostClass::invalidate();
        editModeLeft(accepted);
    }

}
