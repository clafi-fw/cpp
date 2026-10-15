module Themes_App.ElementPage;

import Themes_App.ApplyToControl;
import Themes_App.HueRuleControl;
import Themes_App.RuleSlider;
import Themes_App.ValueRuleControl;

import ClaFi.Controls.Grids;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    void ElementPage::bind(ColorRules& rules, const ColorRules& defaults,
        const PaintChannels outputs, const ThemeColors& colors, OnGetListRuleBase ruleBase,
        OnRulesChanged onRulesChanged)
    {
        m_rules = &rules;
        m_defaults = &defaults;
        m_outputs = outputs;
        m_colors = &colors;
        m_ruleBase = std::move(ruleBase);
        m_onRulesChanged = std::move(onRulesChanged);
        rebuild();
    }

    // **The rows name rules by their place in the list**, so they are built afresh whenever a place
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

    // A row's tag is its rule's place, and only a rule row stands in m_ruleRows at its own place -
    // which is what tells one from the header or the new-item row, whose tags say nothing.
    RuleSelection ElementPage::selection() const
    {
        RuleSelection result{};
        const Grids::GridDescriptor& descriptor = m_grid.descriptor();
        if (const Control* row = descriptor.selectedRow())
        {
            const std::size_t index = row->tag().value;
            if (index < m_ruleRows.size() && m_ruleRows[index] == row)
            {
                result.cell = RuleSelection::Cell{ .rule = index };
                if (const Grids::Column* column = descriptor.selectedColumn())
                    result.cell->column = column->tag();
            }
        }
        for (const Control* row : m_grid.selection())
            result.held.push_back(row->tag().value);
        return result;
    }

    // The cell first and the held set last: a focus arriving on a row the set does not hold reads
    // as a pick, and a pick replaces the set. A cell past the rows - its rule undone or deleted -
    // lands on the last of them, so the keyboard stays in the grid.
    void ElementPage::select(const RuleSelection& at, const TakeFocus takeFocus)
    {
        if (at.cell && !m_ruleRows.empty())
        {
            const std::size_t last = m_ruleRows.size() - 1ull;
            Grids::RowContainer& row = *m_ruleRows[std::min(at.cell->rule, last)];
            const Grids::Column* column = at.cell->column
                ? m_grid.findColumnByTag(*at.cell->column)
                : nullptr;
            m_grid.selectCell(row, column);
            if (takeFocus == TakeFocus::Yes)
                row.setFocus();
        }
        std::vector<Control*> held{};
        for (const std::size_t index : at.held)
            if (index < m_ruleRows.size())
                held.push_back(m_ruleRows[index]);
        m_grid.setSelection(held);
    }

    // A new rule is appended, so it applies after the rest of the list.
    void ElementPage::addRule()
    {
        const PaintChannel output = m_outputs.empty()
            ? PaintChannel::Surface
            : m_outputs.front();
        m_rules->push_back(ColorRule{ .output = output });
        // Appending may move the list, and the hue and value editors hold their rules by address.
        for (Grids::RowContainer* row : m_ruleRows)
            bindEditors(*row);
        const std::size_t index = m_rules->size() - 1ull;
        addRow(index);
        form().invalidateAlign();
        Text what{};
        what << L"Add " << InkWell::accentInk() << L"rule " << m_rules->size() << PopColor{}
            << L" to " << InkWell::accentInk() << m_name << PopColor{};
        // Made on the new rule, whose leading cell the grid is about to select.
        rulesChanged(EditPhase::Settled, what, RuleSelection{
            .cell = RuleSelection::Cell{ .rule = index, .column = Tag{ RuleColumn::ApplyTo } },
            .held = { index }
        });
    }

    // The Delete key arrives through the grid, whose rows the deletion takes down, so the rules
    // go on the next tick. A second request before it is not taken: the places the first one
    // names have not moved yet.
    void ElementPage::deleteSelectedRules()
    {
        if (!m_pendingDelete.held.empty())
            return;
        RuleSelection at = selection();
        if (at.held.empty())
            return;
        m_pendingDelete = std::move(at);
        m_deleteTimer.start(MilliSeconds{ 0u });
    }

    // From the last place to the first, so each erase leaves the places still to go where they
    // were. The step is made on what Delete was asked on, which undo puts back held.
    void ElementPage::deletePendingRules()
    {
        RuleSelection at{};
        std::swap(at, m_pendingDelete);
        std::ranges::sort(at.held, std::ranges::greater{});
        for (const std::size_t index : at.held)
            m_rules->erase(m_rules->begin() + static_cast<std::ptrdiff_t>(index));
        Text what{};
        what << L"Delete " << InkWell::accentInk();
        if (at.held.size() == 1ull)
            what << L"rule " << (at.held.front() + 1);
        else
            what << at.held.size() << L" rules";
        what << PopColor{} << L" from " << InkWell::accentInk() << m_name << PopColor{};
        rebuild();
        rulesChanged(EditPhase::Settled, what, at);
    }

    // The button stands outside the grid, so the rows can go at once - the selection is read
    // before they do.
    void ElementPage::resetRules()
    {
        const RuleSelection at = selection();
        *m_rules = *m_defaults;
        rebuild();
        Text what{};
        what << L"Reset " << InkWell::accentInk() << m_name << PopColor{} << L" to defaults";
        rulesChanged(EditPhase::Settled, what, at);
    }

    // **The rows stand in their new order and the rules follow them.** Each row's tag still names
    // the rule it was built for, so the list is read off the rows and the tags restated as the
    // places the rules stand at now. The step is made on the rows as they stand.
    void ElementPage::rulesMoved(const ItemsMoveEvent& event)
    {
        ColorRules rules{};
        rules.reserve(m_rules->size());
        RuleRows rows{};
        rows.reserve(m_ruleRows.size());
        for (ControlPtr& control : m_grid.controls())
        {
            const RuleRows::iterator row = std::ranges::find_if(m_ruleRows,
                [&control](const Grids::RowContainer* candidate) {
                    return candidate == &*control;
                });
            if (row == m_ruleRows.end())
                continue;
            rules.push_back((*m_rules)[(*row)->tag().value]);
            rows.push_back(*row);
        }
        *m_rules = std::move(rules);
        m_ruleRows = std::move(rows);
        for (std::size_t i = 0ull; i != m_ruleRows.size(); ++i)
        {
            m_ruleRows[i]->setTag(Tag{ i });
            bindEditors(*m_ruleRows[i]);
        }
        Text what{};
        what << L"Move " << InkWell::accentInk();
        if (event.items.size() == 1ull)
        {
            // The place is read off the rows: the grid counts its header among its children.
            const RuleRows::iterator moved = std::ranges::find_if(m_ruleRows,
                [&event](const Grids::RowContainer* candidate) {
                    return candidate == event.items.front();
                });
            const std::size_t place = static_cast<std::size_t>(moved - m_ruleRows.begin());
            what << L"rule " << (place + 1ull);
        }
        else
            what << event.items.size() << L" rules";
        what << PopColor{} << L" in " << InkWell::accentInk() << m_name << PopColor{};
        rulesChanged(EditPhase::Settled, what, selection());
    }

    // Every cell of a rule's row holds its editor.
    void ElementPage::addRow(const std::size_t index)
    {
        Grids::RowContainer& row = m_grid.addControlRow(Tag{ index });
        const Padding padding = themeMetrics().listItem.padding;
        row.addControl<ApplyToControl>(
            column(RuleColumn::ApplyTo),
            padding,
            VerticalAlign::Fill
        );
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

    // A rule here is a change to a colour, so No change and every operation are offered on every
    // row.
    void ElementPage::bindEditors(Grids::RowContainer& row)
    {
        const std::size_t index = row.tag().value;
        ColorEffect& effect = (*m_rules)[index].effect;
        row.controlAtColumnAs<ApplyToControl>(column(RuleColumn::ApplyTo))
            .bind(*m_rules, index, m_outputs, m_onCellEdit, ruleStepName(index, L"Apply to"));
        row.controlAtColumnAs<HueRuleControl>(column(RuleColumn::Hue))
            .bind(effect.hue, *m_colors, m_onCellEdit, true, ruleStepName(index, L"Hue"));
        row.controlAtColumnAs<ValueRuleControl>(column(RuleColumn::Saturation)).bind(
            effect.saturation,
            RuleChannel::Saturation,
            ruleBaseOf(index, RuleChannel::Saturation),
            m_onCellEdit,
            ValueRuleOperations::Any,
            ruleStepName(index, L"Saturation")
        );
        row.controlAtColumnAs<ValueRuleControl>(column(RuleColumn::Elevation)).bind(
            effect.elevation,
            RuleChannel::Elevation,
            ruleBaseOf(index, RuleChannel::Elevation),
            m_onCellEdit,
            ValueRuleOperations::Any,
            ruleStepName(index, L"Elevation")
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

    // Places count from one, as a person reads the rows.
    Text ElementPage::ruleStepName(const std::size_t index, const std::wstring_view column) const
    {
        Text result{};
        result << InkWell::accentInk() << column << PopColor{} << L" change in "
            << InkWell::accentInk() << m_name << L", rule " << (index + 1) << PopColor{};
        return result;
    }

    void ElementPage::rulesChanged(const EditPhase phase, const Text& what,
        const RuleSelection& at)
    {
        m_resetButton.invalidateState();
        if (m_onRulesChanged)
            m_onRulesChanged(phase, what, at);
    }
}
