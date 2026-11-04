module Themes_App.ElementPage;

import Themes_App.ApplyToControl;
import Themes_App.Consts;
import Themes_App.HueRuleControl;
import Themes_App.RuleSlider;
import Themes_App.RuleText;
import Themes_App.Utils;
import Themes_App.ValueRuleControl;

import ClaFi.Application.ThemesManager_Serializers;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Controls.TextItems;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
        // What the Restore link on a dropped rule's row is written with.
        constexpr std::wstring_view k_restoreLink{ L"restore" };

        // Who has dropped a rule a base gives the set, as its row says.
        enum class DropState
        {
            Kept,           // nobody: the rule reaches the set
            DroppedHere,    // the set itself, which may take it back
            DroppedBetween  // a set between the two, named on the row
        };

        // A rule the set takes from a base: its words in the muted ink, struck through once a
        // set below the base has dropped it. The cells are read for measuring and painting alike.
        class InheritedRuleRow : public Grids::Row
        {
        public:
            template<typename... Args>
            InheritedRuleRow(const CreateParams&, Grids::GridDescriptor&, Args&&...);
        public:
            // Takes the rule, the palette its hue is read from, and who has dropped it.
            void bind(const ColorRule&, const ThemeColors&, DropState, std::wstring_view droppedBy);
        protected:
            void getCellText(const Grids::Column&, Text&) override;
            // The strike runs over the rule's words and leaves the Restore link alone.
            void paintCell(PaintEvent&, const FloatRect& cellRect, const Grids::Column&) override;
        private:
            using Lines = std::vector<Text>;
            // A cell's words line by line, the rule's own and not what follows them. The hue
            // cell's is the room its blob takes, which paintHue fills.
            [[nodiscard]] Lines ruleLines(const Grids::Column&) const;
            [[nodiscard]] bool namesHue() const; // whether the rule's hue cell shows a blob
            // The blob and its mark, as an editor's face draws them - centred in the room a face
            // leaves beside its dropdown strip, so the blobs of a column stand in one line.
            void paintHue(PaintEvent&, const FloatRect& textBounds);
            // A line through the middle of a struck run, on whole pixels.
            static void strike(PaintEvent&, float left, float right, float y);
        private:
            const ColorRule* m_rule{};
            const ThemeColors* m_colors{};
            DropState m_dropState{ DropState::Kept };
            std::wstring m_droppedBy{};
        };

        // InheritedRuleRow

        template<typename... Args>
        InheritedRuleRow::InheritedRuleRow(const CreateParams& params,
            Grids::GridDescriptor& descriptor, Args&&... args)
            :
            Grids::Row{ params, descriptor, std::forward<Args>(args)... }
        {
        }

        void InheritedRuleRow::bind(const ColorRule& rule, const ThemeColors& colors,
            const DropState dropState, const std::wstring_view droppedBy)
        {
            m_rule = &rule;
            m_colors = &colors;
            m_dropState = dropState;
            m_droppedBy = droppedBy;
            invalidate();
        }

        // The rule's words, and after them on the second line of its Apply to cell who dropped
        // it: a link that takes it back where the set did, the set's name where one between did.
        void InheritedRuleRow::getCellText(const Grids::Column& column, Text& text)
        {
            if (!m_rule)
                return;
            const Lines lines = ruleLines(column);
            for (std::size_t i = 0ull; i != lines.size(); ++i)
            {
                if (i != 0ull)
                    text << L'\n';
                text << lines[i];
            }
            if (column.tag().value != static_cast<TagValue>(RuleColumn::ApplyTo))
                return;
            if (lines.size() == 1ull)
                text << L'\n';
            if (m_dropState == DropState::DroppedHere)
                text << L"   " << LinkText{ std::wstring{ k_restoreLink }, L"Restore" };
            else if (m_dropState == DropState::DroppedBetween)
                text << L"   " << ColoredText{ InkGrade::Muted,
                    StyledText{ TextStyles::SubBody, L"dropped by ", m_droppedBy } };
        }

        // Measured the way the base measures the cell, so the lines land where it drew them: a
        // line's run starts where the column's alignment puts it in the box the text was laid
        // out in.
        void InheritedRuleRow::paintCell(PaintEvent& event, const FloatRect& cellRect,
            const Grids::Column& column)
        {
            Grids::Row::paintCell(event, cellRect, column);
            if (!m_rule)
                return;
            FloatRect textBounds = cellRect;
            textBounds.inflate(-descriptor().scaledCellMetrics().padding);
            textBounds.left += cellLead(column).x;
            const bool struck = m_dropState != DropState::Kept;
            const bool hueColumn = column.tag().value == static_cast<TagValue>(RuleColumn::Hue);
            if (hueColumn && namesHue())
            {
                paintHue(event, textBounds);
                return;
            }
            if (!struck)
                return;
            const TextFormat& format = cellTextFormat(column);
            const TextAnchor anchor = { column.verticalTextAnchor(), HorizontalTextAnchor::None };
            Text whole{};
            getCellText(column, whole);
            const ScaledSize block = textEngine().measureText(formContext(), whole,
                textBounds.dimensions(), false, true, &format);
            const TextAlign align = format.paragraph().align.value_or(TextAlign::Left);
            float top = anchoredOrigin(textBounds, block, anchor).y;
            for (const Text& line : ruleLines(column))
            {
                const ScaledSize lineSize = textEngine().measureText(formContext(), line,
                    textBounds.dimensions(), false, true, &format);
                float left = textBounds.left;
                if (align == TextAlign::Right)
                    left = textBounds.right - lineSize.x;
                else if (align == TextAlign::Center)
                    left = textBounds.left + (textBounds.width() - lineSize.x) / 2.0f;
                if (lineSize.x > 0.0f)
                    strike(event, left, left + lineSize.x, top + lineSize.y / 2.0f);
                top += lineSize.y;
            }
        }

        InheritedRuleRow::Lines InheritedRuleRow::ruleLines(const Grids::Column& column) const
        {
            Lines result{};
            switch (static_cast<RuleColumn>(column.tag().value))
            {
                case RuleColumn::ApplyTo:
                    writeApplyToChannel(result.emplace_back(), *m_rule, RuleFace::Inherited);
                    if (!m_rule->atRest())
                        writeApplyToCondition(result.emplace_back(), *m_rule,
                            RuleFace::Inherited);
                    break;
                case RuleColumn::Hue:
                    if (namesHue())
                        result.emplace_back() << InTextIcon{ k_blobSize, k_blobSize,
                            DrawIconFunc{} };
                    else
                        writeNoChange(result.emplace_back(), k_hueOpNames[static_cast<std::size_t>(
                            ColorRuleHueOp::NoChange)]);
                    break;
                case RuleColumn::Saturation:
                    writeValueRule(result.emplace_back(), m_rule->effect.saturation,
                        RuleFace::Inherited);
                    break;
                case RuleColumn::Elevation:
                    writeValueRule(result.emplace_back(), m_rule->effect.elevation,
                        RuleFace::Inherited);
                    break;
            }
            return result;
        }

        bool InheritedRuleRow::namesHue() const
        {
            return m_rule->effect.hue.operation() != ColorRuleHueOp::NoChange;
        }

        void InheritedRuleRow::paintHue(PaintEvent& event, const FloatRect& textBounds)
        {
            FloatRect room = textBounds;
            room.right -= event.scaleF(DropdownWidth{}.value);
            const FloatRect blob = blobBounds(room, event.scaleF(k_blobSize));
            paintHueBlob(event, blob, hueBlobColor(m_rule->effect.hue, *m_colors));
            Text glyph{};
            writeBlobGlyph(glyph, hueMark(m_rule->effect.hue.operation()));
            textEngine().drawText(
                event.controlContext(),
                blob,
                Text{ StyledText{ k_blobGlyphStyle, glyph } },
                { VerticalTextAnchor::Center, HorizontalTextAnchor::Center }
            );
            if (m_dropState != DropState::Kept)
                strike(event, blob.left, blob.right, blob.centerY());
        }

        void InheritedRuleRow::strike(PaintEvent& event, const float left, const float right,
            const float y)
        {
            const float middle = std::round(y) + 0.5f;
            event.canvas().drawLine({ left, middle }, { right, middle },
                event.textRgb(InkGrade::Muted), event.scaleF(1.0f));
        }

        // How many of a base's rules the set takes, as the base's group is headed.
        void writeRuleTally(Text& text, const std::size_t kept, const std::size_t count)
        {
            Text tally{};
            if (kept != count)
                tally << kept << L" of ";
            tally << count << (count == 1ull ? L" rule" : L" rules");
            text << L"  ";
            text << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, tally } };
        }
    }

    // ElementPage

    void ElementPage::bind(PageBinding binding)
    {
        m_binding = std::move(binding);
        m_deleteSetButton.setVisible(static_cast<bool>(m_binding.onDeleteSet));
        rebuild();
    }

    // **The rows name rules by their place in the list**, so they are built afresh whenever a place
    // moves. The names are read once per build: a page is built after every change to the sets.
    void ElementPage::rebuild()
    {
        for (Grids::RowContainer* row : m_ruleRows)
            row->deleteSelf();
        m_ruleRows.clear();
        for (Grids::RowExpander* group : m_groups)
            group->deleteSelf();
        m_groups.clear();
        m_inheritedRows.clear();
        if (!m_binding.rules)
            return;
        m_names = m_binding.setNames ? m_binding.setNames() : SetNames{};
        showFacts();
        if (const ColorRules* list = rules())
        {
            for (std::size_t i = 0ull; i != list->size(); ++i)
                addRow(i);
        }
        addInheritedGroups();
        m_resetButton.invalidateState();
        form().invalidateAlign();
    }

    // A row's tag is its rule's place, and only a rule row stands in m_ruleRows at its own place -
    // which is what tells one from the header, whose tag says nothing. A base's row is known by
    // its address.
    RuleSelection ElementPage::selection() const
    {
        RuleSelection result{};
        const Grids::GridDescriptor& descriptor = m_grid.descriptor();
        if (const Control* row = descriptor.selectedRow())
        {
            const std::size_t index = row->tag().value;
            if (index < m_ruleRows.size() && m_ruleRows[index] == row)
                result.cell = RuleSelection::Cell{ .rule = index };
            for (const InheritedRow& inherited : m_inheritedRows)
            {
                if (inherited.row == row)
                    result.cell = RuleSelection::Cell{
                        .level = inherited.level,
                        .rule = inherited.rule
                    };
            }
            if (result.cell)
            {
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
        if (at.cell)
        {
            Grids::RowBase* row = nullptr;
            for (const InheritedRow& inherited : m_inheritedRows)
            {
                if (inherited.level == at.cell->level && inherited.rule == at.cell->rule)
                    row = inherited.row;
            }
            if (!row && !m_ruleRows.empty())
            {
                const std::size_t last = m_ruleRows.size() - 1ull;
                const std::size_t index = at.cell->level == 0ull ? at.cell->rule : last;
                row = m_ruleRows[std::min(index, last)];
            }
            if (row)
            {
                const Grids::Column* column = at.cell->column
                    ? m_grid.findColumnByTag(*at.cell->column)
                    : nullptr;
                m_grid.selectCell(*row, column);
                if (takeFocus == TakeFocus::Yes)
                    row->setFocus();
            }
        }
        std::vector<Control*> held{};
        for (const std::size_t index : at.held)
            if (index < m_ruleRows.size())
                held.push_back(m_ruleRows[index]);
        m_grid.setSelection(held);
    }

    bool ElementPage::canDelete() const
    {
        const RuleSelection at = selection();
        if (!at.held.empty())
            return true;
        if (!at.cell || at.cell->level == 0ull)
            return false;
        for (const InheritedRow& inherited : m_inheritedRows)
        {
            if (inherited.level == at.cell->level && inherited.rule == at.cell->rule)
                return !inherited.dropped;
        }
        return false;
    }

    ElementRules* ElementPage::set() const
    {
        return m_binding.rules ? m_binding.rules->of(m_binding.set) : nullptr;
    }

    ColorRules* ElementPage::rules() const
    {
        ElementRules* stated = set();
        return stated ? &stated->rules : nullptr;
    }

    // Adding a set moves every set, so every page is built again before the edit goes on - this
    // one too, whose rows then stand on the set as it is now.
    ElementRules& ElementPage::statedSet()
    {
        if (ElementRules* stated = set())
            return *stated;
        m_binding.rules->add(ElementRules{ .name = m_binding.set });
        if (m_binding.onSetsChanged)
            m_binding.onSetsChanged();
        return *set();
    }

    // A new rule is appended, so it applies after the rest of the list.
    void ElementPage::addRule()
    {
        const PaintChannel output = m_binding.outputs.empty()
            ? PaintChannel::Surface
            : m_binding.outputs.front();
        ColorRules& list = statedSet().rules;
        list.push_back(ColorRule{ .output = output });
        // Appending may move the list, and the hue and value editors hold their rules by address.
        for (Grids::RowContainer* row : m_ruleRows)
            bindEditors(*row);
        const std::size_t index = list.size() - 1ull;
        addRow(index);
        form().invalidateAlign();
        Text what{};
        what << L"Add " << ColoredText{ InkWell::accentInk(), L"rule ", list.size() };
        what << L" to " << ColoredText{ InkWell::accentInk(), m_name };
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
        if (m_pendingDelete.cell || !m_pendingDelete.held.empty())
            return;
        if (!canDelete())
            return;
        m_pendingDelete = selection();
        m_deleteTimer.start(MilliSeconds{ 0u });
    }

    // Own rules held go, from the last place to the first, so each erase leaves the places still
    // to go where they were. With none held, the cell on a base's rule drops that rule. The step
    // is made on what Delete was asked on, which undo puts back held.
    void ElementPage::deletePendingRules()
    {
        RuleSelection at{};
        std::swap(at, m_pendingDelete);
        if (at.held.empty())
        {
            if (at.cell && at.cell->level != 0ull)
                dropRule(at.cell->level, at.cell->rule);
            return;
        }
        std::ranges::sort(at.held, std::ranges::greater{});
        ColorRules& list = *rules();
        for (const std::size_t index : at.held)
            list.erase(list.begin() + static_cast<std::ptrdiff_t>(index));
        Text rules{};
        if (at.held.size() == 1ull)
            rules << L"rule " << (at.held.front() + 1);
        else
            rules << at.held.size() << L" rules";
        Text what{};
        what << L"Delete " << ColoredText{ InkWell::accentInk(), rules };
        what << L" from " << ColoredText{ InkWell::accentInk(), m_name };
        rebuild();
        rulesChanged(EditPhase::Settled, what, at);
    }

    // The button stands outside the grid, so the rows can go at once - the selection is read
    // before they do. A set the built-in states goes back to that whole, facts and all; a set of
    // the theme's own keeps its facts and loses its rules and what it dropped.
    void ElementPage::resetRules()
    {
        const RuleSelection at = selection();
        ElementRules& stated = statedSet();
        if (m_binding.defaults)
        {
            stated = *m_binding.defaults;
        }
        else
        {
            stated.rules.clear();
            stated.dropped.clear();
        }
        rebuild();
        Text what{};
        what << L"Reset " << ColoredText{ InkWell::accentInk(), m_name } << L" to defaults";
        rulesChanged(EditPhase::Settled, what, at);
    }

    // **The rows stand in their new order and the rules follow them.** Each row's tag still names
    // the rule it was built for, so the list is read off the rows and the tags restated as the
    // places the rules stand at now. The step is made on the rows as they stand. A row can land
    // among a base's groups, so the rows are built again in order on the next tick.
    void ElementPage::rulesMoved(const ItemsMoveEvent& event)
    {
        ColorRules& list = *rules();
        ColorRules moved{};
        moved.reserve(list.size());
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
            moved.push_back(list[(*row)->tag().value]);
            rows.push_back(*row);
        }
        list = std::move(moved);
        m_ruleRows = std::move(rows);
        for (std::size_t i = 0ull; i != m_ruleRows.size(); ++i)
        {
            m_ruleRows[i]->setTag(Tag{ i });
            bindEditors(*m_ruleRows[i]);
        }
        Text movedRules{};
        if (event.items.size() == 1ull)
        {
            // The place is read off the rows: the grid counts its header among its children.
            const RuleRows::iterator movedRow = std::ranges::find_if(m_ruleRows,
                [&event](const Grids::RowContainer* candidate) {
                    return candidate == event.items.front();
                });
            const std::size_t place = static_cast<std::size_t>(movedRow - m_ruleRows.begin());
            movedRules << L"rule " << (place + 1);
        }
        else
            movedRules << event.items.size() << L" rules";
        Text what{};
        what << L"Move " << ColoredText{ InkWell::accentInk(), movedRules };
        what << L" in " << ColoredText{ InkWell::accentInk(), m_name };
        m_pendingSelection = selection();
        rulesChanged(EditPhase::Settled, what, m_pendingSelection);
        m_rebuildTimer.start(MilliSeconds{ 0u });
    }

    // Every cell of a rule's row holds its editor. The row goes after the own rows before it and
    // ahead of the groups of the bases; the header stands first among the grid's children.
    void ElementPage::addRow(const std::size_t index)
    {
        Grids::RowContainer& row = m_grid.addControlRow(Tag{ index });
        m_grid.moveControl(row, m_ruleRows.size() + 1ull);
        const Padding padding = themeMetrics().listItem.padding;
        const Spacing spacing = themeMetrics().listItem.spacing;
        row.addControl<ApplyToControl>(
            column(RuleColumn::ApplyTo),
            padding,
            spacing,
            VerticalAlign::Fill
        );
        row.addControl<HueRuleControl>(
            column(RuleColumn::Hue),
            padding,
            spacing,
            VerticalAlign::Fill
        );
        row.addControl<ValueRuleControl>(
            column(RuleColumn::Saturation),
            padding,
            spacing,
            VerticalAlign::Fill,
            HorizontalTextAnchor::Right
        );
        row.addControl<ValueRuleControl>(
            column(RuleColumn::Elevation),
            padding,
            spacing,
            VerticalAlign::Fill,
            HorizontalTextAnchor::Right
        );
        bindEditors(row);
        m_ruleRows.push_back(&row);
    }

    // A rule here is a change to a colour, so No change and every operation are offered on every
    // row.
    void ElementPage::bindEditors(Grids::RowContainer& row)
    {
        const std::size_t index = row.tag().value;
        ColorRules& list = *rules();
        ColorEffect& effect = list[index].effect;
        row.controlAtColumnAs<ApplyToControl>(column(RuleColumn::ApplyTo))
            .bind(list, index, m_binding.outputs, m_onCellEdit, ruleStepName(index, L"Apply to"));
        row.controlAtColumnAs<HueRuleControl>(column(RuleColumn::Hue))
            .bind(effect.hue, *m_binding.colors, m_onCellEdit, true, ruleStepName(index, L"Hue"));
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

    bool ElementPage::isOwnRow(const Control& row) const
    {
        return std::ranges::any_of(m_ruleRows, [&row](const Grids::RowContainer* candidate) {
            return candidate == &row;
        });
    }

    // The chain is walked from the set's own base up, so the nearest base's group stands first,
    // headed by the base's name and how many of its rules reach the set.
    void ElementPage::addInheritedGroups()
    {
        const ElementChain chain = m_binding.rules->chainOf(m_binding.set);
        for (std::size_t level = 1ull; level < chain.size(); ++level)
        {
            const ElementRules& base = *chain[level];
            std::size_t kept = 0ull;
            for (const ColorRule& rule : base.rules)
                if (!droppingOf(chain, level, rule).dropped)
                    ++kept;
            Text header{};
            header << ColoredText{ InkGrade::Muted, L"from " } << nameOf(base.name);
            writeRuleTally(header, kept, base.rules.size());
            Grids::RowExpander& group = m_grid.addExpander();
            group.header().setHeaderText(header);
            m_groups.push_back(&group);
            for (std::size_t rule = 0ull; rule != base.rules.size(); ++rule)
                addInheritedRow(group.body(), chain, level, rule);
            // Shut again where the user shut it, before the toggles are listened to.
            if (m_shutBases.contains(base.name))
                group.header().setExpanded(false);
            group.header().connectEvent<ToggleExpandedEvent>(
                [this, name = base.name](ToggleExpandedEvent& event) {
                    if (event.expanded())
                        m_shutBases.erase(name);
                    else
                        m_shutBases.insert(name);
                });
        }
    }

    void ElementPage::addInheritedRow(Grids::GridBase& body, const ElementChain& chain,
        const std::size_t level, const std::size_t rule)
    {
        const ColorRule& inherited = chain[level]->rules[rule];
        const Dropping dropping = droppingOf(chain, level, inherited);
        DropState state = DropState::Kept;
        std::wstring_view droppedBy{};
        if (dropping.dropped)
        {
            state = dropping.level == 0ull ? DropState::DroppedHere : DropState::DroppedBetween;
            droppedBy = nameOf(chain[dropping.level]->name);
        }
        InheritedRuleRow& row = body.add<InheritedRuleRow>(Tag{ rule });
        row.bind(inherited, *m_binding.colors, state, droppedBy);
        row.onCellLinkClick([this, level, rule](Grids::CellLinkClickEvent& event) {
            if (event.target == k_restoreLink)
                restoreRule(level, rule);
        });
        m_inheritedRows.push_back(InheritedRow{
            .level = level,
            .rule = rule,
            .row = &row,
            .dropped = dropping.dropped
        });
    }

    ElementPage::Dropping ElementPage::droppingOf(const ElementChain& chain,
        const std::size_t level, const ColorRule& rule) const
    {
        for (std::size_t below = 0ull; below != level; ++below)
        {
            for (const RuleSelector& selector : chain[below]->dropped)
                if (selector.matches(rule))
                    return { .dropped = true, .level = below };
        }
        return {};
    }

    // What is dropped is the rule's selector, so every rule of the chain above answering to it
    // is left out with it. The step is made on the row, which the rebuild keeps in its place.
    void ElementPage::dropRule(const std::size_t level, const std::size_t rule)
    {
        const ElementChain chain = m_binding.rules->chainOf(m_binding.set);
        if (level >= chain.size() || rule >= chain[level]->rules.size())
            return;
        const ColorRule& dropped = chain[level]->rules[rule];
        const RuleSelector selector{
            .inputs = dropped.inputs,
            .andInputs = dropped.andInputs,
            .output = dropped.output
        };
        RuleSelectors& list = statedSet().dropped;
        if (std::ranges::find(list, selector) != list.end())
            return;
        list.push_back(selector);
        Text what{};
        what << L"Drop " << inheritedRuleName(chain, level, rule);
        what << L" from " << ColoredText{ InkWell::accentInk(), m_name };
        const RuleSelection at = selection();
        rebuild();
        rulesChanged(EditPhase::Settled, what, at);
    }

    // Asked by the Restore link on the row, so the row is where the step is made - and the rows
    // are built again on the next tick, outside the click the row is answering.
    void ElementPage::restoreRule(const std::size_t level, const std::size_t rule)
    {
        const ElementChain chain = m_binding.rules->chainOf(m_binding.set);
        if (level >= chain.size() || rule >= chain[level]->rules.size())
            return;
        const ColorRule& restored = chain[level]->rules[rule];
        RuleSelectors& list = statedSet().dropped;
        const std::size_t before = list.size();
        std::erase_if(list, [&restored](const RuleSelector& selector) {
            return selector.matches(restored);
        });
        if (list.size() == before)
            return;
        Text what{};
        what << L"Restore " << inheritedRuleName(chain, level, rule);
        what << L" to " << ColoredText{ InkWell::accentInk(), m_name };
        m_pendingSelection = RuleSelection{
            .cell = RuleSelection::Cell{ .level = level, .rule = rule }
        };
        rulesChanged(EditPhase::Settled, what, m_pendingSelection);
        m_rebuildTimer.start(MilliSeconds{ 0u });
    }

    // The base may be any set whose chain does not reach this one, so no chain turns in a circle;
    // what the set stands on, any set whose standing does not. The picks are put on with the
    // handlers held off: the lists changing is no edit.
    void ElementPage::showFacts()
    {
        const ElementRules* stated = set();
        m_showingFacts = true;
        m_baseItems.clear();
        m_baseTokens.clear();
        m_standsOnItems.clear();
        m_standsOnTokens.clear();
        m_baseItems.push_back(TextItem{ PlaceHolderText{ L"none" } });
        m_baseTokens.emplace_back();
        m_standsOnItems.push_back(TextItem{ PlaceHolderText{ L"none" } });
        m_standsOnTokens.emplace_back();
        for (const SetName& name : m_names)
        {
            if (name.token == m_binding.set)
                continue;
            bool reachesThis = false;
            for (const ElementRules* link : m_binding.rules->chainOf(name.token))
                if (link->name == m_binding.set)
                    reachesThis = true;
            if (!reachesThis)
            {
                m_baseItems.push_back(TextItem{ Text{ name.name } });
                m_baseTokens.push_back(name.token);
            }
            bool standsOnThis = false;
            std::vector<const ElementRules*> walked{};
            for (const ElementRules* step = m_binding.rules->of(name.token); step;
                step = m_binding.rules->of(step->standsOn))
            {
                if (step->name == m_binding.set || std::ranges::find(walked, step) != walked.end())
                {
                    standsOnThis = step->name == m_binding.set;
                    break;
                }
                walked.push_back(step);
            }
            if (!standsOnThis)
            {
                m_standsOnItems.push_back(TextItem{ Text{ name.name } });
                m_standsOnTokens.push_back(name.token);
            }
        }
        if (m_seedItems.empty())
        {
            m_seedItems.push_back(TextItem{ PlaceHolderText{ L"none" } });
            for (const std::wstring_view pigment : k_pigmentKeys)
                m_seedItems.push_back(TextItem{ Text{ pigment } });
        }
        const std::wstring base = stated ? stated->base : std::wstring{};
        const std::wstring standsOn = stated ? stated->standsOn : std::wstring{};
        const auto baseItem = std::ranges::find(m_baseTokens, base);
        const auto standsOnItem = std::ranges::find(m_standsOnTokens, standsOn);
        m_basePick.setItemIndex(baseItem == m_baseTokens.end()
            ? 0ull
            : static_cast<std::size_t>(baseItem - m_baseTokens.begin()));
        m_standsOnPick.setItemIndex(standsOnItem == m_standsOnTokens.end()
            ? 0ull
            : static_cast<std::size_t>(standsOnItem - m_standsOnTokens.begin()));
        m_seedPick.setItemIndex(stated && stated->seed
            ? static_cast<std::size_t>(*stated->seed) + 1ull
            : 0ull);
        m_showingFacts = false;
        // A list is measured as its widest face, and the lists have changed under the faces.
        for (ComboBox* pick : { &m_basePick, &m_standsOnPick, &m_seedPick })
        {
            pick->invalidate();
            pick->invalidateFormAlign();
        }
    }

    void ElementPage::basePicked()
    {
        if (m_showingFacts || !m_basePick.itemIndex())
            return;
        const std::wstring& picked = m_baseTokens[*m_basePick.itemIndex()];
        ElementRules& stated = statedSet();
        if (stated.base == picked)
            return;
        stated.base = picked;
        const RuleSelection at = selection();
        rebuild();
        rulesChanged(EditPhase::Settled, changeStepName(L"Base"), at);
    }

    void ElementPage::standsOnPicked()
    {
        if (m_showingFacts || !m_standsOnPick.itemIndex())
            return;
        const std::wstring& picked = m_standsOnTokens[*m_standsOnPick.itemIndex()];
        ElementRules& stated = statedSet();
        if (stated.standsOn == picked)
            return;
        stated.standsOn = picked;
        rulesChanged(EditPhase::Settled, changeStepName(L"Stands on"), selection());
    }

    void ElementPage::seedPicked()
    {
        if (m_showingFacts || !m_seedPick.itemIndex())
            return;
        const std::size_t item = *m_seedPick.itemIndex();
        const OptionalPigment picked = item == 0ull
            ? OptionalPigment{}
            : OptionalPigment{ static_cast<Pigment>(item - 1ull) };
        ElementRules& stated = statedSet();
        if (stated.seed == picked)
            return;
        stated.seed = picked;
        rulesChanged(EditPhase::Settled, changeStepName(L"Seed"), selection());
    }

    Grids::Column& ElementPage::column(const RuleColumn tag)
    {
        return m_grid.columnByTag(Tag{ tag });
    }

    // Asked as the ramp is painted, since every other rule of the theme can move it.
    OnGetRuleBase ElementPage::ruleBaseOf(const std::size_t index, const RuleChannel channel) const
    {
        if (!m_binding.ruleBase)
            return {};
        return [this, index, channel]() {
            return m_binding.ruleBase((*rules())[index], channel);
        };
    }

    std::wstring_view ElementPage::nameOf(const std::wstring_view token) const
    {
        for (const SetName& name : m_names)
            if (name.token == token)
                return name.name;
        return token;
    }

    Text ElementPage::changeStepName(const std::wstring_view what) const
    {
        Text result{};
        result << ColoredText{ InkWell::accentInk(), what } << L" change in ";
        result << ColoredText{ InkWell::accentInk(), m_name };
        return result;
    }

    // Places count from one, as a person reads the rows.
    Text ElementPage::ruleStepName(const std::size_t index, const std::wstring_view column) const
    {
        Text result{};
        result << ColoredText{ InkWell::accentInk(), column } << L" change in ";
        result << ColoredText{ InkWell::accentInk(), m_name, L", rule ", index + 1 };
        return result;
    }

    Text ElementPage::inheritedRuleName(const ElementChain& chain, const std::size_t level,
        const std::size_t rule) const
    {
        Text result{};
        result << ColoredText{ InkWell::accentInk(), L"rule ", rule + 1 };
        result << L" of " << ColoredText{ InkWell::accentInk(), nameOf(chain[level]->name) };
        return result;
    }

    // An edit can change how wide a cell's text is, and the columns are fitted to their cells.
    void ElementPage::rulesChanged(const EditPhase phase, const Text& what,
        const RuleSelection& at)
    {
        m_resetButton.invalidateState();
        form().invalidateAlign();
        if (m_binding.onRulesChanged)
            m_binding.onRulesChanged(phase, what, at);
    }
}
