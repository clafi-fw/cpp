module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.WithEditMode;

import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
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

    // What turned the edit mode on. See Controls-Base
    export enum class EditModeEntry
    {
        Key,      // Return on the control out of the mode
        Press,    // a press, or a menu asked on the control
        Call      // setEditMode
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
        virtual void editModeEntered(EditModeEntry) {}
        // The mode has turned off; accepted is false for Escape alone.
        virtual void editModeLeft(bool /*accepted*/) {}
        // Whether Return is a line break the host types, which the mode then leaves to it.
        [[nodiscard]] virtual bool takesLineBreaks() const { return false; }
        // Whether a mouse press turns the mode on - yes where it places a caret, no where it drags.
        [[nodiscard]] virtual bool mousePressEntersEditMode() const { return true; }
        void adjustPaint(AdjustPaintEvent&) override;
        void paintFrame(PaintEvent&) override;
        // The mode's indicator, its tips in tipColor. A host overriding paintFrame calls it there.
        void paintEditModeIndicator(PaintEvent&, Color tipColor);
        void nestedKeyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedContextPopup(ContextPopupEvent&) override;
        void focusChanged() override;
        void enterEditMode(EditModeEntry);   // for a host's handler that turns the mode on itself
    private:
        void leaveEditMode(bool accepted);
        void animateEditModeIndicator();
    private:
        // The Return that turned the mode on has its character still to come.
        bool m_entryCharPending{ false };
        float m_editModeIndicatorFactor{ 0.0f }; // stays 0 under EditModeSwitching::No
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
        else if (isInEditMode())
            m_editModeIndicatorFactor = 1.0f;
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::setEditMode(const EditMode value)
    {
        if (value == EditMode::Yes)
            enterEditMode(EditModeEntry::Call);
        else
            leaveEditMode(true);
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::adjustPaint(AdjustPaintEvent& event)
    {
        HostClass::adjustPaint(event);
        event.replaceRing(m_editModeIndicatorFactor);
    }

    // Drawn after the host's frame, so the indicator covers the border along its edge.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::paintFrame(PaintEvent& event)
    {
        HostClass::paintFrame(event);
        paintEditModeIndicator(event, event.strokeRgb());
    }

    // As deep as an open tab's indicator: twice the border.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::paintEditModeIndicator(PaintEvent& event, const Color tipColor)
    {
        if (m_editModeIndicatorFactor <= 0.0f)
            return;
        const float tipOpacity = tipColor.alpha / 255.0f * m_editModeIndicatorFactor;
        event.canvas().fillEdgeIndicator(
            { .bounds = event.controlBounds(), .radii = event.cornerRadii() },
            RectSide::Bottom,
            2.0f * event.scaledStrokeWidth(ThemeMetrics::border) * m_editModeIndicatorFactor,
            event.indicatorRgb().withOpacity(m_editModeIndicatorFactor),
            tipColor.withOpacity(tipOpacity),
            1.0f
        );
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
                enterEditMode(EditModeEntry::Key);
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

    // A mouse press is the pointer's Return, for a host that takes it as one.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::nestedPressDown(PressDownEvent& event)
    {
        HostClass::nestedPressDown(event);
        if (mousePressEntersEditMode())
            enterEditMode(EditModeEntry::Press);
    }

    // A menu on the control is about what the mode shows. A host that answers the request ahead
    // of this turns the mode on itself - see TextBox::nestedContextPopup.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::nestedContextPopup(ContextPopupEvent& event)
    {
        enterEditMode(EditModeEntry::Press);
        HostClass::nestedContextPopup(event);
    }

    // Leaving accepts. A popup opened on the control - an edit menu, a link's menu - keeps it on
    // the focus path, and the mode with it.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::focusChanged()
    {
        HostClass::focusChanged();
        if (!HostClass::isFocused())
            leaveEditMode(true);
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::enterEditMode(const EditModeEntry entry)
    {
        if (isInEditMode())
            return;
        m_editMode = EditMode::Yes;
        animateEditModeIndicator();
        HostClass::invalidate();
        editModeEntered(entry);
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::leaveEditMode(const bool accepted)
    {
        if (!isInEditMode() || m_editModeSwitching == EditModeSwitching::No)
            return;
        m_editMode = EditMode::No;
        animateEditModeIndicator();
        HostClass::invalidate();
        editModeLeft(accepted);
    }

    // An indicator nobody has seen has nothing to grow from.
    template <IsControl HostClass>
    void WithEditMode<HostClass>::animateEditModeIndicator()
    {
        const float target = isInEditMode() ? 1.0f : 0.0f;
        if (!HostClass::isPainted() || !HostClass::visible())
        {
            m_editModeIndicatorFactor = target;
            return;
        }
        HostClass::animate(AnimationSlots::EditMode, m_editModeIndicatorFactor, target,
            [this](AnimateParams& params) {
                m_editModeIndicatorFactor = params.value;
                HostClass::invalidate();
            });
    }

}
