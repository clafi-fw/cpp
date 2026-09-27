module ThisApp.ElementPage;

import ThisApp.ApplyToControl;
import ThisApp.HueRuleControl;
import ThisApp.RuleSlider;
import ThisApp.ValueRuleControl;

import ClaFi.Controls.Grids;
import ClaFi.Controls.StackView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    void ElementPage::bind(ColorRules2& rules, const ColorRules2& defaults,
        const ThemeColors& colors, OnGetListRuleBase ruleBase, OnRulesChanged onRulesChanged)
    {
        m_rules = &rules;
        m_defaults = &defaults;
        m_colors = &colors;
        m_ruleBase = std::move(ruleBase);
        m_onRulesChanged = std::move(onRulesChanged);
        rebuild();
    }

    // THE ROWS NAME RULES BY THEIR PLACE IN THE LIST, so they are built afresh whenever a place
    // moves.
    void ElementPage::rebuild()
    {
        for (Grids::RowContainer* row : m_ruleRows)
            row->deleteSelf();
        m_ruleRows.clear();
        if (!m_rules)
            return;
        for (std::size_t i = 0ull; i != m_rules->size(); ++i)
            addRow(i);
        m_resetButton.invalidateState();
        form().invalidateAlign();
    }

    // A new rule is appended, so it applies after the rest of the list.
    void ElementPage::addRule()
    {
        m_rules->push_back(ColorRule2{});
        // Appending may move the list, and the hue and value editors hold their rules by address.
        for (Grids::RowContainer* row : m_ruleRows)
            bindEditors(*row);
        addRow(m_rules->size() - 1ull);
        form().invalidateAlign();
        rulesChanged();
    }

    // The Delete key arrives through the grid, whose rows the deletion takes down, so the rules
    // go on the next tick. A second request before it is not taken: the places the first one
    // names have not moved yet.
    void ElementPage::deleteSelectedRules()
    {
        if (!m_pendingDeletes.empty())
            return;
        for (const Control* row : m_grid.selection())
            m_pendingDeletes.push_back(row->tag().value);
        if (!m_pendingDeletes.empty())
            m_deleteTimer.start(MilliSeconds{ 0u });
    }

    // From the last place to the first, so each erase leaves the places still to go where they
    // were.
    void ElementPage::deletePendingRules()
    {
        std::ranges::sort(m_pendingDeletes, std::ranges::greater{});
        for (const std::size_t index : m_pendingDeletes)
            m_rules->erase(m_rules->begin() + static_cast<std::ptrdiff_t>(index));
        m_pendingDeletes.clear();
        rebuild();
        rulesChanged();
    }

    // The button stands outside the grid, so the rows can go at once.
    void ElementPage::resetRules()
    {
        *m_rules = *m_defaults;
        rebuild();
        rulesChanged();
    }

    // Every cell of a rule's row holds its editor.
    void ElementPage::addRow(const std::size_t index)
    {
        Grids::RowContainer& row = m_grid.addControlRow(Tag{ index });
        const Padding padding = themeMetrics().listItem.padding;
        ApplyToControl& applyTo = row.addControl<ApplyToControl>(
            column(RuleColumn::ApplyTo),
            padding,
            VerticalAlign::Fill
        );
        applyTo.bind(*m_rules, index, m_onCellEdit);
        row.addControl<HueRuleControl>(
            column(RuleColumn::Hue),
            padding,
            VerticalAlign::Fill
        );
        row.addControl<ValueRuleControl>(
            column(RuleColumn::Saturation),
            padding,
            VerticalAlign::Fill
        );
        row.addControl<ValueRuleControl>(
            column(RuleColumn::Elevation),
            padding,
            VerticalAlign::Fill
        );
        bindEditors(row);
        m_ruleRows.push_back(&row);
    }

    // No change is always a hue a new rule may take, so Clear is offered on every row.
    void ElementPage::bindEditors(Grids::RowContainer& row)
    {
        const std::size_t index = row.tag().value;
        ColorRule& effect = (*m_rules)[index].effect;
        row.controlAtColumnAs<HueRuleControl>(column(RuleColumn::Hue))
            .bind(effect.hue, *m_colors, m_onCellEdit, true);
        row.controlAtColumnAs<ValueRuleControl>(column(RuleColumn::Saturation)).bind(
            effect.saturation,
            RuleChannel::Saturation,
            ruleBaseOf(index, RuleChannel::Saturation),
            m_onCellEdit
        );
        row.controlAtColumnAs<ValueRuleControl>(column(RuleColumn::Elevation)).bind(
            effect.elevation,
            RuleChannel::Elevation,
            ruleBaseOf(index, RuleChannel::Elevation),
            m_onCellEdit
        );
    }

    Grids::Column& ElementPage::column(const RuleColumn tag)
    {
        return m_grid.columnByTag(Tag{ tag });
    }

    // Asked as the ramp is painted, since every other rule of the theme can move it.
    OnGetRuleBase ElementPage::ruleBaseOf(const std::size_t index, const RuleChannel channel) const
    {
        if (!m_ruleBase)
            return {};
        return [this, index, channel]() {
            return m_ruleBase((*m_rules)[index], channel);
        };
    }

    void ElementPage::rulesChanged()
    {
        m_resetButton.invalidateState();
        if (m_onRulesChanged)
            m_onRulesChanged();
    }
}
