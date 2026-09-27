module ThisApp.ApplyToControl;

import ClaFi.Controls.Checkbox;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        // The words a rule is read in - labels, where a theme file's names are the serializers'.
        constexpr std::array<std::wstring_view, static_cast<std::size_t>(PaintChannel::Count)>
            k_channelLabels{
                L"surface",
                L"stroke",
                L"text",
                L"shadow"
            };

        constexpr std::array<std::wstring_view, static_cast<std::size_t>(RuleInput::Count)>
            k_inputLabels{
                L"hovered",
                L"pressed",
                L"focused",
                L"selected",
                L"disabled",
                L"text hovered",
                L"current",
                L"window focused"
            };

        [[nodiscard]] std::wstring inputsText(const RuleInputs inputs)
        {
            if (inputs.empty())
                return L"at rest";
            std::wstring result{};
            for (std::size_t i = 0ull; i != k_inputLabels.size(); ++i)
            {
                if (!inputs.has(static_cast<RuleInput>(i)))
                    continue;
                if (!result.empty())
                    result.append(L" or ");
                result.append(k_inputLabels[i]);
            }
            return result;
        }

        // A label that starts what it is shown in, with a capital.
        [[nodiscard]] std::wstring itemLabel(const std::wstring_view label)
        {
            std::wstring result{ label };
            if (!result.empty())
                result.front() = static_cast<wchar_t>(std::towupper(result.front()));
            return result;
        }
    }

    // The outputs as radio buttons in one column and the inputs as checkboxes in the next.
    class ApplyToPopup : public StackPanel
    {
    public:
        ApplyToPopup(const CreateParams&, ApplyToControl&);
    public:
        [[nodiscard]] const ApplyToControl& owner() const { return m_owner; }
        void setOutput(PaintChannel);
        void toggleInput(RuleInput);
    private:
        StackPanel& addColumn();
        void refreshItems();
    private:
        ApplyToControl& m_owner;
        std::vector<Control*> m_items{}; // every radio button and checkbox
    };

    // An output in the popup, marked while the rule writes to it.
    class OutputItem : public RadioButton
    {
    public:
        OutputItem(const CreateParams&, ApplyToPopup&, PaintChannel);
    protected:
        void getControlState(GetStateEvent&) const override;
        void click(ClickEvent&) override;
    private:
        ApplyToPopup& m_popup;
        PaintChannel m_channel;
    };

    // An input in the popup, checked while the rule reads it.
    class InputItem : public Checkbox
    {
    public:
        InputItem(const CreateParams&, ApplyToPopup&, RuleInput);
    protected:
        void getControlState(GetStateEvent&) const override;
        void click(ClickEvent&) override;
    private:
        ApplyToPopup& m_popup;
        RuleInput m_input;
    };

    // ApplyToControl

    void ApplyToControl::bind(ColorRules2& rules, const std::size_t index, OnRuleChanged onChanged)
    {
        m_rules = &rules;
        m_index = index;
        m_onChanged = std::move(onChanged);
        invalidate();
    }

    PaintChannel ApplyToControl::output() const
    {
        return rule().output;
    }

    bool ApplyToControl::reads(const RuleInput input) const
    {
        return rule().inputs.has(input);
    }

    void ApplyToControl::setOutput(const PaintChannel value)
    {
        if (rule().output == value)
            return;
        rule().output = value;
        changed();
    }

    void ApplyToControl::toggleInput(const RuleInput input)
    {
        RuleInputs& inputs = rule().inputs;
        if (inputs.has(input))
            inputs.remove(input);
        else
            inputs.add(input);
        changed();
    }

    void ApplyToControl::showDropdown(Control& initiator)
    {
        if (!m_rules)
            return;
        dropPopup<ApplyToPopup>(form(), initiator, *this);
    }

    // The rule read the way it is stated - "Surface hovered or selected", "Stroke at rest".
    void ApplyToControl::getMainText(GetTextEvent& event) const
    {
        if (!m_rules)
            return;
        const ColorRule2& value = rule();
        event.text << itemLabel(k_channelLabels[static_cast<std::size_t>(value.output)]) << L" "
            << inputsText(value.inputs);
    }

    const ColorRule2& ApplyToControl::rule() const
    {
        return (*m_rules)[m_index];
    }

    ColorRule2& ApplyToControl::rule()
    {
        return (*m_rules)[m_index];
    }

    // The face's text changes length, so the row is laid out again.
    void ApplyToControl::changed()
    {
        invalidate();
        invalidateFormAlign();
        if (m_onChanged)
            m_onChanged();
    }

    // ApplyToPopup

    ApplyToPopup::ApplyToPopup(const CreateParams& params, ApplyToControl& owner)
        :
        StackPanel{ params,
            Orientation::Horizontal,
            Interactivity::ActiveContainer,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            Padding{ 4.0f },
            Spacing{ 12.0f }
        },
        m_owner{ owner }
    {
        StackPanel& outputs = addColumn();
        for (std::size_t i = 0ull; i != k_channelLabels.size(); ++i)
            m_items.push_back(&outputs.add<OutputItem>(*this, static_cast<PaintChannel>(i)));
        StackPanel& inputs = addColumn();
        for (std::size_t i = 0ull; i != k_inputLabels.size(); ++i)
            m_items.push_back(&inputs.add<InputItem>(*this, static_cast<RuleInput>(i)));
    }

    void ApplyToPopup::setOutput(const PaintChannel value)
    {
        m_owner.setOutput(value);
        refreshItems();
    }

    void ApplyToPopup::toggleInput(const RuleInput input)
    {
        m_owner.toggleInput(input);
        refreshItems();
    }

    StackPanel& ApplyToPopup::addColumn()
    {
        return add<StackPanel>(
            Orientation::Vertical,
            Interactivity::ActiveContainer,
            VerticalAlign::Top
        );
    }

    // The popup stays up through a pick, so an item that lost its mark is still in view.
    void ApplyToPopup::refreshItems()
    {
        for (Control* item : m_items)
            item->invalidateState();
    }

    // OutputItem

    OutputItem::OutputItem(const CreateParams& params, ApplyToPopup& popup,
        const PaintChannel channel)
        :
        RadioButton{ params,
            Text{ itemLabel(k_channelLabels[static_cast<std::size_t>(channel)]) }
        },
        m_popup{ popup },
        m_channel{ channel }
    {
    }

    // Stopped here, so the column's current item is not written over the mark.
    void OutputItem::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        event.state.selected = m_popup.owner().output() == m_channel;
        event.stopPropagation();
    }

    void OutputItem::click(ClickEvent&)
    {
        m_popup.setOutput(m_channel);
    }

    // InputItem

    InputItem::InputItem(const CreateParams& params, ApplyToPopup& popup, const RuleInput input)
        :
        Checkbox{ params,
            Text{ itemLabel(k_inputLabels[static_cast<std::size_t>(input)]) }
        },
        m_popup{ popup },
        m_input{ input }
    {
    }

    // Stopped here, so the column's current item is not written over the check.
    void InputItem::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        event.state.selected = m_popup.owner().reads(m_input);
        event.stopPropagation();
    }

    void InputItem::click(ClickEvent&)
    {
        m_popup.toggleInput(m_input);
    }
}
