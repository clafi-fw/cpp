module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Checkbox;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A button drawn with a check, always showing its indicator.
    export class Checkbox : public ButtonBase
    {
    public:
        template <typename... Args>
        explicit Checkbox(const CreateParams& params, Args&&... args)
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

    // A checkbox that keeps its own checked state and toggles it on a click.
    export class Checkbox2 : public Checkbox
    {
    public:
        template <typename... Args>
        Checkbox2(const CreateParams& params, Args&&... args)
            :
            Checkbox{ params, std::forward<Args>(args)...},
            INIT_PROPERTY(checked)
        {}
    public:
        // Whether the box is checked.
        DECLARE_WRITABLE_PROPERTY(Checked, checked, setChecked, Checked::No)
    public:
        void setChecked(Checked value);
    protected:
        void click(ClickEvent&) override;
        void getControlState(GetStateEvent& event) const override
        {
            event.state.selected = m_checked == Checked::Yes;
            event.stopPropagation();
        }
    };

    void Checkbox2::setChecked(Checked value)
    {
        if (value == m_checked)
            return;
        m_checked = value;
        invalidateState();
    }

    void Checkbox2::click(ClickEvent&)
    {
        if (m_checked == Checked::No)
            m_checked = Checked::Yes;
        else
            m_checked = Checked::No;
        invalidateState();
    }
}
