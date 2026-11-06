module Themes_App.MetricsGrid;

import ClaFi.Application.ThemesManager_Elements;
import ClaFi.Application.ThemesManager_Serializers;

import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Grids;
import ClaFi.Controls.TextItems;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
        constexpr std::array k_thicknessNames = enumNames(Thickness{});

        [[nodiscard]] std::wstring_view trimmed(const std::wstring_view text)
        {
            const std::size_t first = text.find_first_not_of(L" \t");
            if (first == std::wstring_view::npos)
                return {};
            const std::size_t last = text.find_last_not_of(L" \t");
            return text.substr(first, last - first + 1);
        }

        // The shortest spelling that reads back as the same number.
        [[nodiscard]] std::wstring numberText(const float value)
        {
            return std::format(L"{}", value);
        }

        template <typename Tagged>
        [[nodiscard]] bool isColumn(const Grids::Column& column, const Tagged tag)
        {
            return column.tag().value == static_cast<TagValue>(tag);
        }

        [[nodiscard]] bool isStyle(const std::wstring_view name, const UiElement element)
        {
            return name == uiElementOf(element).token;
        }
    }

    // MetricsGrid

    void MetricsGrid::bind(ThemeStyles& styles, const std::wstring_view style,
        OnGetStyleName styleName, OnMetricsChanged onMetricsChanged)
    {
        m_styles = &styles;
        m_style = style;
        m_styleName = std::move(styleName);
        m_onMetricsChanged = std::move(onMetricsChanged);
        refresh();
    }

    // The rows are built again rather than refreshed: which rows there are follows the chain,
    // and a base picked on the page may bring a window's shadow with it or take it away.
    void MetricsGrid::refresh()
    {
        for (Grids::RowBase* row : m_rows)
            row->deleteSelf();
        m_rows.clear();
        m_sizeRows.clear();
        m_borderPick = nullptr;
        if (!m_styles)
            return;
        addRows();
        form().invalidateAlign();
    }

    bool MetricsGrid::canClear() const
    {
        const std::optional<SizeRow> row = currentRow();
        if (!row || !m_styles)
            return false;
        const ControlStyle* stated = m_styles->of(m_style);
        return stated && states(stated->metrics, *row);
    }

    void MetricsGrid::clearCurrent()
    {
        const std::optional<SizeRow> row = currentRow();
        if (!row || !canClear())
            return;
        clear(statedMetrics(), *row);
        Text what{};
        what << L"Clear " << ColoredText{ InkWell::accentInk(), nameOf(*row) };
        what << L" in " << ColoredText{ InkWell::accentInk(), m_styleName(m_style) };
        changed(what);
    }

    // **A size that follows from another has no row**: a primary window's padding is its border,
    // and an indicator and a scroll button are square, each sized by one number. A shadow is a
    // window's, so its rows stand where the chain reaches Any Window.
    void MetricsGrid::addRows()
    {
        addBorderRow();
        addRow(m_grid, SizeRow::Band);
        addRow(m_grid, SizeRow::Radius);
        if (!paddingFollowsBorder())
            addRow(m_grid, SizeRow::Padding);
        addRow(m_grid, SizeRow::Spacing);
        addRow(m_grid, SizeRow::MinSize);
        if (!hasSingleSize())
            addRow(m_grid, SizeRow::PreferredSize);
        addRow(m_grid, SizeRow::Depth);
        addRow(m_grid, SizeRow::GrowIn);
        if (castsShadow())
        {
            addGroup(L"Shadow", {
                SizeRow::ShadowBlur,
                SizeRow::ShadowOffset,
                SizeRow::ShadowSpread,
                SizeRow::ShadowOpacity
            });
        }
    }

    void MetricsGrid::addBorderRow()
    {
        Grids::RowContainer& row = m_grid.addControlRow(Tag{ SizeRow::Border });
        m_rows.push_back(&row);
        m_sizeRows.push_back({ .row = &row, .size = SizeRow::Border });
        connectRow(row, SizeRow::Border);
        TextItems weights{};
        for (const wchar_t* name : k_thicknessNames)
            weights.push_back(TextItem{ Text{ name } });
        m_borderPick = &row.addControl<ComboBox>(
            m_grid.columnByTag(Tag{ SizeColumn::Value }),
            std::move(weights),
            VerticalAlign::Fill,
            ComboBox::OnChange{ [this](ComboBoxChangeEvent&) {
                borderPicked();
            } }
        );
        const Thickness border = m_styles->resolvedMetrics(m_style).border;
        m_showingBorder = true;
        m_borderPick->setItemIndex(static_cast<std::size_t>(border));
        m_showingBorder = false;
    }

    void MetricsGrid::addRow(Grids::GridBase& target, const SizeRow size)
    {
        Grids::Row& row = target.addRow(Tag{ size });
        m_rows.push_back(&row);
        m_sizeRows.push_back({ .row = &row, .size = size });
        connectRow(row, size);
    }

    void MetricsGrid::addGroup(const std::wstring_view name, const SizeRows sizes)
    {
        Grids::RowExpander& group = m_grid.addExpander();
        m_rows.push_back(&group);
        group.header().setHeaderText(Text{ name });
        for (const SizeRow size : sizes)
        {
            Grids::Row& row = group.body().addRow(Tag{ size });
            m_sizeRows.push_back({ .row = &row, .size = size });
            connectRow(row, size);
        }
    }

    void MetricsGrid::connectRow(Grids::Row& row, const SizeRow size)
    {
        row.onCellContent([this, size](Grids::CellContentEvent& event) {
            event.hasContent = hasCell(size, event.column());
            event.stopPropagation();
        });
        row.onGetCellText([this, size](Grids::GetCellTextEvent& event) {
            writeCell(event, size);
        });
        row.onGetCellEditorText([this, size](Grids::GetCellEditorTextEvent& event) {
            writeEditorText(event, size);
        });
        row.onAcceptCellText([this, size](Grids::AcceptCellTextEvent& event) {
            accept(event, size);
        });
    }

    // A value the style states is written plain; one a base gives it, or nobody states, is muted,
    // and the From cell says which.
    void MetricsGrid::writeCell(Grids::GetCellTextEvent& event, const SizeRow size)
    {
        event.stopPropagation();
        Text& text = event.text();
        if (isColumn(event.column(), SizeColumn::Name))
        {
            text << nameOf(size);
            return;
        }
        const Source source = sourceOf(size);
        if (isColumn(event.column(), SizeColumn::From))
        {
            if (!source.stated)
                text << ColoredText{ InkGrade::Muted, L"default" };
            else if (source.level != 0ull)
                text << ColoredText{ InkGrade::Muted,
                    m_styleName(m_styles->chainOf(m_style)[source.level]->name) };
            return;
        }
        const std::optional<float> number = numberOf(size, event.column());
        if (!number)
            return;
        const bool own = source.stated && source.level == 0ull;
        if (own)
            text << numberText(*number);
        else
            text << ColoredText{ InkGrade::Muted, numberText(*number) };
        if (isLength(size))
            text << ColoredText{ InkGrade::Muted, L" pt" };
    }

    void MetricsGrid::writeEditorText(Grids::GetCellEditorTextEvent& event, const SizeRow size)
    {
        event.stopPropagation();
        if (const std::optional<float> number = numberOf(size, event.column()))
            event.text = Text{ numberText(*number) };
    }

    void MetricsGrid::accept(Grids::AcceptCellTextEvent& event, const SizeRow size)
    {
        event.stopPropagation();
        const std::optional<float> number = numberOf(size, event.column());
        if (!number)
            return;
        const std::wstring typed = event.text().plainText();
        float value = 0.0f;
        const std::wstring refusal = readNumber(size, trimmed(typed), value);
        if (!refusal.empty())
        {
            event.refuse(refusal);
            return;
        }
        // A typed value is this style's own even where it equals what the chain gave.
        const Source source = sourceOf(size);
        if (value == *number && source.stated && source.level == 0ull)
            return;
        stateNumber(size, event.column(), value);
    }

    void MetricsGrid::borderPicked()
    {
        if (!m_styles || m_showingBorder)
            return;
        const Thickness picked = static_cast<Thickness>(*m_borderPick->itemIndex());
        const Source source = sourceOf(SizeRow::Border);
        const bool own = source.stated && source.level == 0ull;
        if (own && picked == m_styles->resolvedMetrics(m_style).border)
            return;
        statedMetrics().border = picked;
        sizeEdited(SizeRow::Border);
    }

    // A shadow's offset and spread may run either way; every other size counts from 0.
    std::wstring MetricsGrid::readNumber(const SizeRow size, const std::wstring_view typed,
        float& value)
    {
        try
        {
            std::size_t read = 0;
            value = std::stof(std::wstring{ typed }, &read);
            if (read != typed.size())
                return L"A number is expected";
        }
        catch (...)
        {
            return L"A number is expected";
        }
        if (!std::isfinite(value))
            return L"A number is expected";
        const bool signedSize = size == SizeRow::ShadowOffset || size == SizeRow::ShadowSpread;
        if (!signedSize && value < 0.0f)
            return L"This size is a number from 0 up";
        const bool fraction = size == SizeRow::GrowIn || size == SizeRow::ShadowOpacity;
        if (fraction && value > 1.0f)
            return L"This size is a fraction, from 0 to 1";
        return {};
    }

    // A pair is stated whole: the other axis takes the value the chain gave it.
    void MetricsGrid::stateNumber(const SizeRow size, const Grids::Column& column,
        const float value)
    {
        const ControlMetrics resolved = m_styles->resolvedMetrics(m_style);
        StatedMetrics& stated = statedMetrics();
        if (isPair(size))
        {
            FloatPoint pair = pairOf(size, resolved);
            if (isColumn(column, SizeColumn::Across))
                pair.x = value;
            else
                pair.y = value;
            switch (size)
            {
                case SizeRow::Padding:
                    stated.padding = Padding{ pair };
                    break;
                case SizeRow::Spacing:
                    stated.spacing = Spacing{ pair };
                    break;
                case SizeRow::MinSize:
                    stated.minSize = MinSize{ pair };
                    break;
                case SizeRow::PreferredSize:
                    stated.preferredSize = PreferredSize{ pair };
                    break;
                case SizeRow::ShadowOffset:
                    stated.shadowOffset = pair;
                    break;
                default:
                    break;
            }
            sizeEdited(size);
            return;
        }
        switch (size)
        {
            case SizeRow::Band:
                stated.band = value;
                break;
            case SizeRow::Radius:
                stated.radius = value;
                break;
            case SizeRow::MinSize:
                stated.minSize = MinSize{ value, value }; // a square's, the height following
                break;
            case SizeRow::Depth:
                stated.depth = value;
                break;
            case SizeRow::GrowIn:
                stated.growIn = value;
                break;
            case SizeRow::ShadowBlur:
                stated.shadowBlur = value;
                break;
            case SizeRow::ShadowSpread:
                stated.shadowSpread = value;
                break;
            case SizeRow::ShadowOpacity:
                stated.shadowOpacity = value;
                break;
            default:
                break;
        }
        sizeEdited(size);
    }

    void MetricsGrid::sizeEdited(const SizeRow size)
    {
        Text what{};
        what << ColoredText{ InkWell::accentInk(), nameOf(size) } << L" change in ";
        what << ColoredText{ InkWell::accentInk(), m_styleName(m_style) };
        changed(what);
    }

    // **The rows go on the next tick.** The border's pick and a cell's editor are rows of the
    // grid, and the change arrives through them: a rebuild here would take down the control
    // that is still raising it. The style already holds the change, so the step is recorded now.
    void MetricsGrid::changed(const Text& what)
    {
        if (m_onMetricsChanged)
            m_onMetricsChanged(EditPhase::Settled, what);
        m_pendingRow = currentRow();
        m_refreshTimer.start(MilliSeconds{ 0u });
    }

    void MetricsGrid::selectRow(const std::optional<SizeRow> size)
    {
        if (!size)
            return;
        for (const SizeRowEntry& entry : m_sizeRows)
        {
            if (entry.size == *size)
            {
                m_grid.selectCell(*entry.row, m_grid.descriptor().selectedColumn());
                return;
            }
        }
    }

    StatedMetrics& MetricsGrid::statedMetrics()
    {
        if (ControlStyle* stated = m_styles->of(m_style))
            return stated->metrics;
        return m_styles->add(ControlStyle{ .name = m_style }).metrics;
    }

    std::optional<float> MetricsGrid::numberOf(const SizeRow size,
        const Grids::Column& column) const
    {
        if (!m_styles)
            return std::nullopt;
        const ControlMetrics resolved = m_styles->resolvedMetrics(m_style);
        if (isPair(size))
        {
            const FloatPoint pair = pairOf(size, resolved);
            if (isColumn(column, SizeColumn::Across))
                return pair.x;
            if (isColumn(column, SizeColumn::Down))
                return pair.y;
            return std::nullopt;
        }
        if (!isColumn(column, SizeColumn::Value))
            return std::nullopt;
        switch (size)
        {
            case SizeRow::Band:
                return resolved.band;
            case SizeRow::Radius:
                return resolved.radius;
            case SizeRow::MinSize:
                return resolved.minSize.x; // a square's, its height following - see settle
            case SizeRow::Depth:
                return resolved.zDepthFactor;
            case SizeRow::GrowIn:
                return resolved.surfaceGrowInScale;
            case SizeRow::ShadowBlur:
                return resolved.shadow.blur;
            case SizeRow::ShadowSpread:
                return resolved.shadow.spread;
            case SizeRow::ShadowOpacity:
                return resolved.shadow.opacity;
            case SizeRow::Border:
            case SizeRow::Padding:
            case SizeRow::Spacing:
            case SizeRow::PreferredSize:
            case SizeRow::ShadowOffset:
                break;
        }
        return std::nullopt;
    }

    FloatPoint MetricsGrid::pairOf(const SizeRow size, const ControlMetrics& resolved) const
    {
        switch (size)
        {
            case SizeRow::Padding:
                return resolved.padding;
            case SizeRow::Spacing:
                return resolved.spacing;
            case SizeRow::MinSize:
                return resolved.minSize;
            case SizeRow::PreferredSize:
                return resolved.preferredSize;
            case SizeRow::ShadowOffset:
                return resolved.shadow.offset;
            case SizeRow::Border:
            case SizeRow::Band:
            case SizeRow::Radius:
            case SizeRow::Depth:
            case SizeRow::GrowIn:
            case SizeRow::ShadowBlur:
            case SizeRow::ShadowSpread:
            case SizeRow::ShadowOpacity:
                break;
        }
        noReach("MetricsGrid: a size of one number has no pair");
    }

    // The nearest style of the chain stating the size, the style's own first.
    MetricsGrid::Source MetricsGrid::sourceOf(const SizeRow size) const
    {
        const StyleChain chain = m_styles->chainOf(m_style);
        for (std::size_t level = 0ull; level != chain.size(); ++level)
        {
            if (states(chain[level]->metrics, size))
                return { .stated = true, .level = level };
        }
        return {};
    }

    bool MetricsGrid::states(const StatedMetrics& metrics, const SizeRow size)
    {
        switch (size)
        {
            case SizeRow::Border:
                return metrics.border.has_value();
            case SizeRow::Band:
                return metrics.band.has_value();
            case SizeRow::Radius:
                return metrics.radius.has_value();
            case SizeRow::Padding:
                return metrics.padding.has_value();
            case SizeRow::Spacing:
                return metrics.spacing.has_value();
            case SizeRow::MinSize:
                return metrics.minSize.has_value();
            case SizeRow::PreferredSize:
                return metrics.preferredSize.has_value();
            case SizeRow::Depth:
                return metrics.depth.has_value();
            case SizeRow::GrowIn:
                return metrics.growIn.has_value();
            case SizeRow::ShadowBlur:
                return metrics.shadowBlur.has_value();
            case SizeRow::ShadowOffset:
                return metrics.shadowOffset.has_value();
            case SizeRow::ShadowSpread:
                return metrics.shadowSpread.has_value();
            case SizeRow::ShadowOpacity:
                return metrics.shadowOpacity.has_value();
        }
        return false;
    }

    void MetricsGrid::clear(StatedMetrics& metrics, const SizeRow size)
    {
        switch (size)
        {
            case SizeRow::Border:
                metrics.border.reset();
                break;
            case SizeRow::Band:
                metrics.band.reset();
                break;
            case SizeRow::Radius:
                metrics.radius.reset();
                break;
            case SizeRow::Padding:
                metrics.padding.reset();
                break;
            case SizeRow::Spacing:
                metrics.spacing.reset();
                break;
            case SizeRow::MinSize:
                metrics.minSize.reset();
                break;
            case SizeRow::PreferredSize:
                metrics.preferredSize.reset();
                break;
            case SizeRow::Depth:
                metrics.depth.reset();
                break;
            case SizeRow::GrowIn:
                metrics.growIn.reset();
                break;
            case SizeRow::ShadowBlur:
                metrics.shadowBlur.reset();
                break;
            case SizeRow::ShadowOffset:
                metrics.shadowOffset.reset();
                break;
            case SizeRow::ShadowSpread:
                metrics.shadowSpread.reset();
                break;
            case SizeRow::ShadowOpacity:
                metrics.shadowOpacity.reset();
                break;
        }
    }

    std::wstring_view MetricsGrid::nameOf(const SizeRow size) const
    {
        switch (size)
        {
            case SizeRow::Border:
                return L"Border";
            case SizeRow::Band:
                return L"Band";
            case SizeRow::Radius:
                return L"Radius";
            case SizeRow::Padding:
                return L"Padding";
            case SizeRow::Spacing:
                return L"Spacing";
            case SizeRow::MinSize:
                return hasSingleSize() ? L"Size" : L"Minimum size";
            case SizeRow::PreferredSize:
                return L"Preferred size";
            case SizeRow::Depth:
                return L"Depth";
            case SizeRow::GrowIn:
                return L"Grow-in";
            case SizeRow::ShadowBlur:
                return L"Blur";
            case SizeRow::ShadowOffset:
                return L"Offset";
            case SizeRow::ShadowSpread:
                return L"Spread";
            case SizeRow::ShadowOpacity:
                return L"Opacity";
        }
        return {};
    }

    // A group's header is a row of no size.
    std::optional<MetricsGrid::SizeRow> MetricsGrid::currentRow() const
    {
        const Control* row = m_grid.descriptor().selectedRow();
        for (const SizeRowEntry& entry : m_sizeRows)
            if (entry.row == row)
                return entry.size;
        return std::nullopt;
    }

    bool MetricsGrid::castsShadow() const
    {
        for (const ControlStyle* link : m_styles->chainOf(m_style))
            if (link->name == BaseElements::AnyWindow)
                return true;
        return false;
    }

    bool MetricsGrid::hasSingleSize() const
    {
        return isStyle(m_style, UiElement::SelectionIndicator)
            || isStyle(m_style, UiElement::HoverIndicator)
            || isStyle(m_style, UiElement::ScrollButton);
    }

    bool MetricsGrid::paddingFollowsBorder() const
    {
        return isStyle(m_style, UiElement::Dialog);
    }

    bool MetricsGrid::isPair(const SizeRow size) const
    {
        if (size == SizeRow::MinSize)
            return !hasSingleSize();
        return size == SizeRow::Padding
            || size == SizeRow::Spacing
            || size == SizeRow::PreferredSize
            || size == SizeRow::ShadowOffset;
    }

    // A pair's numbers stand under Across and Down, and a single number spans the two.
    bool MetricsGrid::hasCell(const SizeRow size, const Grids::Column& column) const
    {
        if (isColumn(column, SizeColumn::Name) || isColumn(column, SizeColumn::From))
            return true;
        if (isColumn(column, SizeColumn::Value))
            return !isPair(size);
        return isPair(size);
    }

    // Depth is a share of the theme's depth, and grow-in and opacity are fractions.
    bool MetricsGrid::isLength(const SizeRow size)
    {
        return size != SizeRow::Depth && size != SizeRow::GrowIn && size != SizeRow::ShadowOpacity;
    }
}
