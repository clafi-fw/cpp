module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.CheckBox;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A button drawn with a check, always showing its indicator.
    export class CheckBox : public ButtonBase
    {
    public:
        template <typename... Args>
        explicit CheckBox(const CreateParams& params, Args&&... args)
            :
            ButtonBase{
                params,
                IndicatorVisibility::Always,
                IndicatorStyle::Check,
                Interactivity::Focusable,
                params.themeMetrics().listItem,
                std::forward<Args>(args)...
            }
        {}
    };

    // A check box that keeps its own checked state and toggles it on a click.
    export class CheckBox2 : public CheckBox
    {
    public:
        template <typename... Args>
        CheckBox2(const CreateParams& params, Args&&... args)
            :
            CheckBox{ params, std::forward<Args>(args)...},
            INIT_PROPERTY(checked)
        {}
    public:
        // Whether the box is checked.
        DECLARE_WRITABLE_PROPERTY(Checked, checked, setChecked, Checked::No)
    public:
        void setChecked(Checked value);
    protected:
        void nestedClick(ClickEvent&) override;
        void getControlState(GetStateEvent& event) const override
        {
            event.state.selected = m_checked == Checked::Yes;
            event.stopPropagation();
        }
    };

    void CheckBox2::setChecked(Checked value)
    {
        if (value == m_checked)
            return;
        m_checked = value;
        invalidateState();
    }

    void CheckBox2::nestedClick(ClickEvent&)
    {
        if (m_checked == Checked::No)
            m_checked = Checked::Yes;
        else
            m_checked = Checked::No;
        invalidateState();
    }
}
