module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.WithEditMode;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;

namespace ClaFi::Controls
{
    export enum class EditMode
    {
        No,
        Yes
    };

    export template <IsControl HostClass>
        class WithEditMode : public HostClass
    {
    public:
        template <typename... Args>
        explicit WithEditMode(const CreateParams&, Args&&...);
    public:
        DECLARE_WRITABLE_PROPERTY(EditMode, editMode, setEditMode, EditMode::No)
    public:
        void setEditMode(EditMode);
        bool isInEditMode() const { return m_editMode == EditMode::Yes; }
    protected:
        // Allows editors to intercept navigation keys, only is called in edit mode
        virtual void editModeKeyDown(KeyDownEvent&) {};

        void adjustPaint(AdjustPaintEvent&) override;
        void nestedKeyDown(KeyDownEvent&) override;
    };

    template <IsControl HostClass>
    template <typename ... Args>
    WithEditMode<HostClass>::WithEditMode(const CreateParams& params, Args&&... args)
        :
        HostClass{ params, std::forward<Args>(args)... }
    {
        
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::setEditMode(EditMode value)
    {
        if (value == m_editMode)
            return;
        m_editMode = value;
        HostClass::invalidate();
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::adjustPaint(AdjustPaintEvent& event)
    {
        // Temporarily edit mode indication - to be done right later
        if (m_editMode == EditMode::Yes)
        {
            event.setColorRules(UiElement::Button);
            event.setSurfaceHsl({ 0.69f, 0.5f, 0.5f }); // !!!
        }
        HostClass::adjustPaint(event);
    }

    template <IsControl HostClass>
    void WithEditMode<HostClass>::nestedKeyDown(KeyDownEvent& event)
    {
        HostClass::nestedKeyDown(event);
        if (event.propagationStopped())
            return;

        if (isInEditMode())
        {
            switch (event.key)
            {
            case Keys::Return:
                if (!HostClass::wordWrap())
                {
                    // TODO: accept edit here
                    setEditMode(EditMode::No);
                    return;
                }
            case Keys::Escape:
                // TODO: reject edit here
                setEditMode(EditMode::No);
                return;
            }
        }
        else
        {
            switch (event.key)
            {
            case Keys::Return:
                setEditMode(EditMode::Yes);
                break;
            }
            return;
        }

        editModeKeyDown(event);
    }
}
