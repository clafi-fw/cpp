module ThisApp.ElementPage;

import ThisApp.Consts;

import ClaFi.Icons.DeleteIcon;

import ClaFi.Controls.Button;
import ClaFi.Controls.Grids;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        // The words a rule is read in. The names a theme file uses are the serializers'; these
        // are labels, and free to read as such.
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

        // A channel as the rule moves it - +0.05 an offset, x0.80 a scale, =0.50 a set - and
        // nothing where the rule leaves it alone.
        [[nodiscard]] std::wstring valueText(const ColorRuleValue& value)
        {
            switch (value.operation())
            {
                case ColorRuleOp::NoChange:
                    return {};
                case ColorRuleOp::Offset:
                    return std::format(L"{:+.2f}", value.value());
                case ColorRuleOp::Scale:
                    return std::format(L"\u00D7{:.2f}", value.value());
                case ColorRuleOp::Set:
                    return std::format(L"={:.2f}", value.value());
            }
            return {};
        }

        [[nodiscard]] std::wstring hueText(const ColorRuleHue& hue)
        {
            switch (hue.operation())
            {
                case ColorRuleHueOp::NoChange:
                    return {};
                case ColorRuleHueOp::ExactValue:
                    return std::format(L"={:.2f}", hue.exactValue());
                default:
                    return std::wstring{ k_hueOpNames[static_cast<std::size_t>(hue.operation())] };
            }
        }
    }

    void ElementPage::bind(ColorRules2& rules, OnRulesChanged onRulesChanged)
    {
        m_rules = &rules;
        m_onRulesChanged = std::move(onRulesChanged);
        rebuild();
    }

    // THE ROWS NAME RULES BY THEIR PLACE IN THE LIST, so they are built afresh whenever a place
    // moves.
    void ElementPage::rebuild()
    {
        for (Control* row : m_ruleRows)
            row->deleteSelf();
        m_ruleRows.clear();
        if (!m_rules)
            return;
        for (std::size_t i = 0ull; i != m_rules->size(); ++i)
            if ((*m_rules)[i].subject == m_element)
                addRow(i);
        form().invalidateAlign();
    }

    // A new rule is appended, so it comes last among this element's and applies after them.
    void ElementPage::addRule()
    {
        m_rules->push_back(ColorRule2{ .subject = m_element });
        addRow(m_rules->size() - 1ull);
        form().invalidateAlign();
        rulesChanged();
    }

    // A second press before the tick is not taken: the rule the first one names has not gone yet,
    // and every place after it is about to move.
    void ElementPage::deleteRule(const std::size_t index)
    {
        if (m_pendingDelete)
            return;
        m_pendingDelete = index;
        m_deleteTimer.start(MilliSeconds{ 0u });
    }

    void ElementPage::deletePendingRule()
    {
        const std::size_t index = *m_pendingDelete;
        m_pendingDelete.reset();
        m_rules->erase(m_rules->begin() + static_cast<std::ptrdiff_t>(index));
        rebuild();
        rulesChanged();
    }

    void ElementPage::addRow(const std::size_t index)
    {
        Grids::RowContainer& row = m_grid.addControlRow(Tag{ index });
        ToolButton& deleteButton = row.addControl<ToolButton>(
            m_grid.columnByTag(Tag{ RuleColumn::Delete }),
            IconSize{ 14.0f },
            ButtonViewMode::IconOnly,
            Button::OnPaintIcon{ Icons::DeleteIcon::paint },
            L"Delete rule"
        );
        deleteButton.onClick([this, index](ClickEvent&) {
            deleteRule(index);
        });
        m_ruleRows.push_back(&row);
    }

    void ElementPage::ruleCellText(Grids::GetCellTextEvent& event) const
    {
        const ColorRule2& rule = (*m_rules)[event.row().tag().value];
        switch (event.column().tag().get<RuleColumn>())
        {
            case RuleColumn::Output:
                event.text() << k_channelLabels[static_cast<std::size_t>(rule.output)];
                break;
            case RuleColumn::Inputs:
                event.text() << inputsText(rule.inputs);
                break;
            case RuleColumn::Hue:
                event.text() << hueText(rule.effect.hue);
                break;
            case RuleColumn::Saturation:
                event.text() << valueText(rule.effect.saturation);
                break;
            case RuleColumn::Elevation:
                event.text() << valueText(rule.effect.elevation);
                break;
            case RuleColumn::Delete:
                break;
        }
    }

    void ElementPage::rulesChanged() const
    {
        if (m_onRulesChanged)
            m_onRulesChanged();
    }
}
