module ThisApp.ApplyToControl;

import ClaFi.Controls.Checkbox;
import ClaFi.Controls.Label;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

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

        // A word framing a rule rather than naming its parts, gray and small.
        void writeFramingWord(Text& text, const std::wstring_view word)
        {
            text << InkGrade::Muted
                << TextStyleId::SubBody
                << word
                << PopTextStyle{}
                << PopColor{};
        }

        // A set's inputs joined as "or", in brackets when asked and there is more than one.
        void writeInputs(Text& text, const RuleInputs inputs, const bool bracketed)
        {
            Text result{};
            std::size_t count = 0ull;
            for (std::size_t i = 0ull; i != k_inputLabels.size(); ++i)
            {
                if (!inputs.has(static_cast<RuleInput>(i)))
                    continue;
                if (count != 0ull)
                    writeFramingWord(result, L" or ");
                result << k_inputLabels[i];
                ++count;
            }
            if (bracketed and count > 1ull)
                text << L'(' << result << L')';
            else
                text << result;
        }

        // When the rule applies: "at rest", "a or b", "(a or b) and c".
        void writeCondition(Text& text, const ColorRule2& rule)
        {
            const bool joined = !rule.andInputs.empty();
            if (rule.inputs.empty())
                writeFramingWord(text, L"at rest");
            else
                writeInputs(text, rule.inputs, joined);
            if (joined)
            {
                writeFramingWord(text, L" and ");
                writeInputs(text, rule.andInputs, joined);
            }
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

    // The outputs as radio buttons, then each of the two input sets as a column of checkboxes.
    class ApplyToPopup : public StackPanel
    {
    public:
        ApplyToPopup(const CreateParams&, ApplyToControl&);
    public:
        [[nodiscard]] const ApplyToControl& owner() const { return m_owner; }
        void setOutput(PaintChannel);
        void toggleInput(RuleClause, RuleInput);
    private:
        StackPanel& addColumn(std::wstring_view header);
        void addInputColumn(std::wstring_view header, RuleClause);
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
        void nestedClick(ClickEvent&) override;
    private:
        ApplyToPopup& m_popup;
        PaintChannel m_channel;
    };

    // An input in the popup, checked while the rule's set reads it.
    class InputItem : public Checkbox
    {
    public:
        InputItem(const CreateParams&, ApplyToPopup&, RuleClause, RuleInput);
    protected:
        void getControlState(GetStateEvent&) const override;
        void nestedClick(ClickEvent&) override;
    private:
        ApplyToPopup& m_popup;
        RuleClause m_clause;
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

    bool ApplyToControl::reads(const RuleClause clause, const RuleInput input) const
    {
        return (rule().*clause).has(input);
    }

    bool ApplyToControl::readsAny(const RuleClause clause) const
    {
        return !(rule().*clause).empty();
    }

    void ApplyToControl::setOutput(const PaintChannel value)
    {
        if (rule().output == value)
            return;
        rule().output = value;
        changed();
    }

    void ApplyToControl::toggleInput(const RuleClause clause, const RuleInput input)
    {
        RuleInputs& inputs = rule().*clause;
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

    // The channel the rule writes, and under it the states it reads, one grade down.
    void ApplyToControl::getMainText(GetTextEvent& event) const
    {
        if (!m_rules)
            return;
        const ColorRule2& value = rule();
        event.text << TextStyleId::SubHeading
            << itemLabel(k_channelLabels[static_cast<std::size_t>(value.output)])
            << PopTextStyle{};
        if (!value.inputs.empty())
            writeFramingWord(event.text, L", when");
        event.text << L'\n'
            << InkGrade::Strong;
        writeCondition(event.text, value);
        event.text << PopColor{};
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
        StackPanel& outputs = addColumn(L"Apply to");
        for (std::size_t i = 0ull; i != k_channelLabels.size(); ++i)
            m_items.push_back(&outputs.add<OutputItem>(*this, static_cast<PaintChannel>(i)));
        addInputColumn(L"When", &ColorRule2::inputs);
        addInputColumn(L"And", &ColorRule2::andInputs);
    }

    void ApplyToPopup::setOutput(const PaintChannel value)
    {
        m_owner.setOutput(value);
        refreshItems();
    }

    void ApplyToPopup::toggleInput(const RuleClause clause, const RuleInput input)
    {
        m_owner.toggleInput(clause, input);
        refreshItems();
    }

    StackPanel& ApplyToPopup::addColumn(const std::wstring_view header)
    {
        StackPanel& result = add<StackPanel>(
            Orientation::Vertical,
            Interactivity::ActiveContainer,
            VerticalAlign::Top
        );
        result.add<Label>(
            Text{ InkGrade::Muted, header },
            WordWrap::No
        );
        return result;
    }

    void ApplyToPopup::addInputColumn(const std::wstring_view header, const RuleClause clause)
    {
        StackPanel& column = addColumn(header);
        for (std::size_t i = 0ull; i != k_inputLabels.size(); ++i)
            m_items.push_back(&column.add<InputItem>(*this, clause, static_cast<RuleInput>(i)));
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

    void OutputItem::nestedClick(ClickEvent&)
    {
        m_popup.setOutput(m_channel);
    }

    // InputItem

    InputItem::InputItem(const CreateParams& params, ApplyToPopup& popup, const RuleClause clause,
        const RuleInput input)
        :
        Checkbox{ params,
            Text{ itemLabel(k_inputLabels[static_cast<std::size_t>(input)]) }
        },
        m_popup{ popup },
        m_clause{ clause },
        m_input{ input }
    {
    }

    // Stopped here, so the column's current item is not written over the check.
    void InputItem::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        event.state.selected = m_popup.owner().reads(m_clause, m_input);
        event.state.enabled = m_clause != &ColorRule2::andInputs
            or m_popup.owner().readsAny(&ColorRule2::inputs);
        event.stopPropagation();
    }

    void InputItem::nestedClick(ClickEvent&)
    {
        m_popup.toggleInput(m_clause, m_input);
    }
}
