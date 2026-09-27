export module ThisApp.ValueRuleControl;

import ThisApp.RuleSlider;

import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What a value rule control calls after it has changed its value.
    export using OnValueRuleChanged = std::function<void()>;

    using ValueRuleControlBase = WithInPlaceEdit<DropdownControlBase>;

    // A Saturation or Elevation cell: a rule's operation and value, typed on it or picked below it.
    export class ValueRuleControl : public ValueRuleControlBase
    {
    public:
        template<typename... Args>
        explicit ValueRuleControl(const CreateParams&, Args&&...);
    public:
        // Takes the value by address, the channel it drives, its ramp's base and what to call.
        void bind(ColorRuleValue&, RuleChannel, OnGetRuleBase, OnValueRuleChanged);
        [[nodiscard]] const ColorRuleValue& value() const { return *m_value; }
        [[nodiscard]] RuleChannel channel() const { return m_channel; }
        [[nodiscard]] const OnGetRuleBase& ruleBase() const { return m_ruleBase; }
        [[nodiscard]] ColorRuleOp operation() const;
        [[nodiscard]] float normalizedValue() const;
        void setOperation(ColorRuleOp);
        void setNormalizedValue(float);
    protected:
        [[nodiscard]] EditorMode editorMode() const override;
        // The face is typed over on any press the grid leaves it, and the strip drops the popup.
        [[nodiscard]] bool clickOpensEditor() const override { return true; }
        void showDropdown(Control& initiator) override;
        void getMainText(GetTextEvent&) const override;
        void nestedGetTooltip(GetTooltipEvent&) override;
        void getEditorText(Text&) const override;
        void acceptEditorText(AcceptEditEvent&) override;
        [[nodiscard]] FloatPoint editorMaxTextSize(const FloatRect& textRect) const override;
        void nestedKeyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
    private:
        void changed();
    private:
        ColorRuleValue* m_value{}; // null until bound
        RuleChannel m_channel{};
        OnGetRuleBase m_ruleBase{};
        OnValueRuleChanged m_onChanged{};
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ValueRuleControl::ValueRuleControl(const CreateParams& params, Args&&... args)
        :
        ValueRuleControlBase{ params,
            HorizontalTextAnchor::Center,
            VerticalTextAnchor::Center,
            WordWrap::No,
            std::forward<Args>(args)...
        }
    {
    }
}
