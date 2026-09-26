export module ThisApp.ValueRuleControl;

import ClaFi.Controls.Base.DropdownControlBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What a value rule control calls after it has changed its value.
    export using OnValueRuleChanged = std::function<void()>;

    // A Saturation or Elevation cell: the operation a rule applies and by how much, in a dropdown.
    export class ValueRuleControl : public DropdownControlBase
    {
    public:
        template<typename... Args>
        explicit ValueRuleControl(const CreateParams&, Args&&...);
    public:
        // Takes the value by address, and what to call after changing it.
        void bind(ColorRuleValue&, OnValueRuleChanged);
        [[nodiscard]] ColorRuleOp operation() const;
        [[nodiscard]] float normalizedValue() const;
        void setOperation(ColorRuleOp);
        void setNormalizedValue(float);
    protected:
        [[nodiscard]] bool dropOnPrimaryPress() const override { return true; }
        void showDropdown(Control& initiator) override;
        void getMainText(GetTextEvent&) const override;
        void getTooltip(GetTooltipEvent&) override;
    private:
        void changed();
    private:
        ColorRuleValue* m_value{}; // null until bound
        OnValueRuleChanged m_onChanged{};
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ValueRuleControl::ValueRuleControl(const CreateParams& params, Args&&... args)
        :
        DropdownControlBase{ params,
            HorizontalTextAnchor::Center,
            VerticalTextAnchor::Center,
            WordWrap::No,
            std::forward<Args>(args)...
        }
    {
    }
}
