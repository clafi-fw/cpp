module Themes_App.SizesPage;

import ClaFi.Application.ThemesManager_Elements;
import ClaFi.Application.ThemesManager_Serializers;

import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Grids;
import ClaFi.Controls.TextItems;

import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
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
    }

    // SizesPage

    void SizesPage::bind(ThemeMetrics& metrics, const ThemeMetrics& defaults,
        OnSizesChanged onSizesChanged)
    {
        m_metrics = &metrics;
        m_defaults = &defaults;
        m_onSizesChanged = std::move(onSizesChanged);
        refresh();
    }

    // The cells read the theme as they are drawn, and the border's pick is put on from it.
    void SizesPage::refresh()
    {
        if (m_metrics)
            m_borderPick->setItemIndex(static_cast<std::size_t>(m_metrics->of(m_set).border));
        m_resetButton.invalidateState();
        m_grid.invalidate();
        form().invalidateAlign();
    }

    // **A size that follows from another has no row**: a primary window's padding is its border,
    // and a check mark and a scroll button are square, each sized by one number. A band has a row
    // where a control draws one.
    void SizesPage::addRows()
    {
        addBorderRow();
        if (ThemeMetrics::drawsBand(m_set))
            addRow(m_grid, SizeRow::Band);
        addRow(m_grid, SizeRow::Radius);
        if (m_set != MetricsSet::PrimaryWindow)
            addRow(m_grid, SizeRow::Padding);
        addRow(m_grid, SizeRow::Spacing);
        addRow(m_grid, SizeRow::MinSize);
        if (!hasSingleSize())
            addRow(m_grid, SizeRow::PreferredSize);
        addRow(m_grid, SizeRow::Depth);
        addRow(m_grid, SizeRow::GrowIn);
        if (ThemeMetrics::castsShadow(m_set))
        {
            addGroup(L"Shadow", {
                SizeRow::ShadowBlur,
                SizeRow::ShadowOffset,
                SizeRow::ShadowSpread,
                SizeRow::ShadowOpacity
            });
        }
    }

    void SizesPage::addBorderRow()
    {
        Grids::RowContainer& row = m_grid.addControlRow(Tag{ SizeRow::Border });
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
    }

    void SizesPage::addRow(Grids::GridBase& target, const SizeRow size)
    {
        connectRow(target.addRow(Tag{ size }), size);
    }

    void SizesPage::addGroup(const std::wstring_view name, const SizeRows sizes)
    {
        Grids::RowExpander& group = m_grid.addExpander();
        group.header().setHeaderText(Text{ name });
        for (const SizeRow size : sizes)
            addRow(group.body(), size);
    }

    void SizesPage::connectRow(Grids::Row& row, const SizeRow size)
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

    void SizesPage::writeCell(Grids::GetCellTextEvent& event, const SizeRow size)
    {
        event.stopPropagation();
        Text& text = event.text();
        if (isColumn(event.column(), SizeColumn::Name))
        {
            text << nameOf(size);
            return;
        }
        const float* number = numberOf(size, event.column());
        if (!number)
            return;
        text << numberText(*number);
        if (isLength(size))
            text << ColoredText{ InkGrade::Muted, L" pt" };
    }

    void SizesPage::writeEditorText(Grids::GetCellEditorTextEvent& event, const SizeRow size)
    {
        event.stopPropagation();
        if (const float* number = numberOf(size, event.column()))
            event.text = Text{ numberText(*number) };
    }

    void SizesPage::accept(Grids::AcceptCellTextEvent& event, const SizeRow size)
    {
        event.stopPropagation();
        float* number = numberOf(size, event.column());
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
        if (value == *number)
            return;
        *number = value;
        sizeEdited(size);
    }

    void SizesPage::borderPicked()
    {
        if (!m_metrics)
            return;
        const Thickness picked = static_cast<Thickness>(*m_borderPick->itemIndex());
        Thickness& border = m_metrics->of(m_set).border;
        if (picked == border)
            return;
        border = picked;
        sizeEdited(SizeRow::Border);
    }

    // A shadow's offset and spread may run either way; every other size counts from 0.
    std::wstring SizesPage::readNumber(const SizeRow size, const std::wstring_view typed,
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

    void SizesPage::sizeEdited(const SizeRow size)
    {
        Text what{};
        what << ColoredText{ InkWell::accentInk(), nameOf(size) } << L" change in ";
        what << ColoredText{ InkWell::accentInk(), setName() };
        changed(what);
    }

    void SizesPage::reset()
    {
        m_metrics->of(m_set) = m_defaults->of(m_set);
        if (WindowShadow* shadow = m_metrics->shadowOf(m_set))
            *shadow = *m_defaults->shadowOf(m_set);
        Text what{};
        what << L"Reset " << ColoredText{ InkWell::accentInk(), setName() };
        what << L" sizes to defaults";
        changed(what);
    }

    void SizesPage::changed(const Text& what)
    {
        m_metrics->settle();
        refresh();
        if (m_onSizesChanged)
            m_onSizesChanged(EditPhase::Settled, what);
    }

    bool SizesPage::differsFromDefaults() const
    {
        if (!m_metrics || !m_defaults)
            return false;
        if (m_metrics->of(m_set) != m_defaults->of(m_set))
            return true;
        const WindowShadow* shadow = m_metrics->shadowOf(m_set);
        return shadow && *shadow != *m_defaults->shadowOf(m_set);
    }

    float* SizesPage::numberOf(const SizeRow size, const Grids::Column& column)
    {
        if (!m_metrics)
            return nullptr;
        if (isPair(size))
        {
            FloatPoint& pair = pairOf(size);
            if (isColumn(column, SizeColumn::Across))
                return &pair.x;
            if (isColumn(column, SizeColumn::Down))
                return &pair.y;
            return nullptr;
        }
        if (!isColumn(column, SizeColumn::Value))
            return nullptr;
        ControlMetrics& set = m_metrics->of(m_set);
        WindowShadow* shadow = m_metrics->shadowOf(m_set);
        switch (size)
        {
            case SizeRow::Band:
                return &set.band;
            case SizeRow::Radius:
                return &set.radius;
            case SizeRow::MinSize:
                return &set.minSize.x; // a square's, its height following - see ThemeMetrics
            case SizeRow::Depth:
                return &set.zDepthFactor;
            case SizeRow::GrowIn:
                return &set.surfaceGrowInScale;
            case SizeRow::ShadowBlur:
                return &shadow->blur;
            case SizeRow::ShadowSpread:
                return &shadow->spread;
            case SizeRow::ShadowOpacity:
                return &shadow->opacity;
            case SizeRow::Border:
            case SizeRow::Padding:
            case SizeRow::Spacing:
            case SizeRow::PreferredSize:
            case SizeRow::ShadowOffset:
                break;
        }
        return nullptr;
    }

    FloatPoint& SizesPage::pairOf(const SizeRow size)
    {
        ControlMetrics& set = m_metrics->of(m_set);
        switch (size)
        {
            case SizeRow::Padding:
                return set.padding;
            case SizeRow::Spacing:
                return set.spacing;
            case SizeRow::MinSize:
                return set.minSize;
            case SizeRow::PreferredSize:
                return set.preferredSize;
            case SizeRow::ShadowOffset:
                return m_metrics->shadowOf(m_set)->offset;
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
        noReach("SizesPage: a size of one number has no pair");
    }

    std::wstring_view SizesPage::nameOf(const SizeRow size) const
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

    std::wstring_view SizesPage::setName() const
    {
        return metricsSetOf(m_set).name;
    }

    bool SizesPage::hasSingleSize() const
    {
        return m_set == MetricsSet::CheckMark || m_set == MetricsSet::ScrollButton;
    }

    bool SizesPage::isPair(const SizeRow size) const
    {
        if (size == SizeRow::MinSize)
            return !hasSingleSize();
        return size == SizeRow::Padding
            || size == SizeRow::Spacing
            || size == SizeRow::PreferredSize
            || size == SizeRow::ShadowOffset;
    }

    // A pair's numbers stand under Across and Down, and a single number spans the two.
    bool SizesPage::hasCell(const SizeRow size, const Grids::Column& column) const
    {
        if (isColumn(column, SizeColumn::Name))
            return true;
        if (isColumn(column, SizeColumn::Value))
            return !isPair(size);
        return isPair(size);
    }

    // Depth is a share of the theme's depth, and grow-in and opacity are fractions.
    bool SizesPage::isLength(const SizeRow size)
    {
        return size != SizeRow::Depth && size != SizeRow::GrowIn && size != SizeRow::ShadowOpacity;
    }
}
