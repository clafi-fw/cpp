module ClaFi.Controls.Grids;

import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Animation;
import ClaFi.Core.Foundation;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Diagnostic.Log;

namespace ClaFi::Controls::Grids
{
    // HighlightChannel

    void HighlightChannel::forgetRow(AnimationController* animator, const Control* value)
    {
        for (CellHighlight& cell : m_ring)
            if (cell.row == value)
            {
                if (animator)
                    animator->stop(&cell);
                cell = CellHighlight{};
            }
        if (m_row == value)
            m_row = nullptr;
    }

    void HighlightChannel::stopAll(AnimationController* animator)
    {
        if (!animator)
            return;
        for (CellHighlight& cell : m_ring)
            animator->stop(&cell);
    }

    bool HighlightChannel::setCell(Control* row, const Column* column)
    {
        if (row == m_row && column == m_column)
            return false;
        m_row = row;
        m_column = column;
        return true;
    }

    float HighlightChannel::cellFactor(const Control& row, const Column& column) const
    {
        for (const CellHighlight& cell : m_ring)
            if (cell.is(&row, &column))
                return cell.factor;
        return 0.0f;
    }

    CellHighlight& HighlightChannel::acquireCellHighlight(AnimationController* animator,
        const Control& row, const Column& column)
    {
        for (CellHighlight& cell : m_ring)
            if (cell.is(&row, &column))
                return cell;
        if (m_ring.full() && animator)
            animator->stop(&m_ring.front());
        return m_ring.emplace_back(&row, &column);
    }

    // GridDescriptor

    GridDescriptor::GridDescriptor(Grid& owner, ViewMode viewMode, GridLines gridLines)
        :
        m_owner{ owner },
        m_viewMode{ viewMode },
        m_gridLines{ gridLines },
        m_designCellMetrics{ m_owner.themeMetrics().gridCell },
        m_scaledCellMetrics{ m_designCellMetrics, m_owner.scaler() },
        // Cannot be a default member initialiser: capturing `this` there does not compile.
        m_onCellAnimate{ [this](AnimateParams& params) { onCellAnimated(params); } }
    {
    }

    // The fades run under the cells of the two rings, not under a control, so Control's teardown
    // does not reach them; they are stopped here, while the grid can still name its controller.
    GridDescriptor::~GridDescriptor()
    {
        AnimationController* animator = m_owner.animator();
        m_hoverChannel.stopAll(animator);
        m_selectChannel.stopAll(animator);
    }

    void GridDescriptor::setGridLines(GridLines value)
    {
        if (m_gridLines == value)
            return;
        m_gridLines = value;
        m_owner.invalidate();
    }

    bool GridDescriptor::drawsVerticalLines() const
    {
        return m_gridLines == GridLines::Both || m_gridLines == GridLines::Vertical;
    }

    bool GridDescriptor::drawsHorizontalLines() const
    {
        return m_gridLines == GridLines::Both || m_gridLines == GridLines::Horizontal;
    }

    float GridDescriptor::calculateGrid(const FormContext& formContext, float rightPadding, float boundsW)
    {
        m_scrollBarWidthAndSpacing = rightPadding;
        if (m_lastBoundaryWidth == boundsW)
            return m_rootColumn.finalWidth();
        m_lastBoundaryWidth = boundsW;

        m_scaledCellMetrics = ScaledCellMetrics{ m_designCellMetrics, formContext.scaler() };

        // recursively
        m_rootColumn.preCalcRows(formContext, m_owner);
        if (boundsW != k_maxFloat)
        {
            boundsW = std::max(0.0f, boundsW);
            // recursively
            m_rootColumn.finishCalcStage1();
            // before stretchStretched(...)!
            const float needToShrink = m_rootColumn.finalWidth() - boundsW;
            // recursively
            m_rootColumn.stretchStretched(boundsW);
            if (needToShrink > 0.0f)
            {
                m_rootColumn.shrinkBy(needToShrink);
                m_rootColumn.spanSubColumns();
            }
            m_rootColumn.calcLeft(0.0f);
        }
        return m_rootColumn.finalWidth();
    }

    bool GridDescriptor::isColumnHovered(const Column& column) const
    {
        const Column* current = m_hoverChannel.column();
        while (current)
        {
            if (current == &column)
                return true;
            current = current->parent();
        }
        return false;
    }

    float GridDescriptor::cellHoveredFactor(const Control& row, const Column& column) const
    {
        return m_hoverChannel.cellFactor(row, column);
    }

    float GridDescriptor::cellSelectedFactor(const Control& row, const Column& column) const
    {
        return m_selectChannel.cellFactor(row, column);
    }

    CornerRadii GridDescriptor::cornerRadiiOf(const PaintEvent& gridEvent)
    {
        const FloatRect bounds = gridEvent.controlBounds();
        const FloatRect visible = gridEvent.viewport();
        using CornerPoints = std::array<FloatPoint, k_cornersNum>;
        const CornerPoints ownCorners = { bounds.topLeft(), bounds.topRight(), bounds.bottomRight(), bounds.bottomLeft() };
        const CornerPoints corners = { visible.topLeft(), visible.topRight(), visible.bottomRight(), visible.bottomLeft() };
        CornerRadii result = gridEvent.cornerRadii();
        for (std::size_t i = 0; i < k_cornersNum; ++i)
        {
            if (!(corners[i] == ownCorners[i]))
                result.set(i, std::max(result.get(i), gridEvent.radius()));
        }
        return result;
    }

    FloatRect GridDescriptor::cellInnerRect(const FloatRect& cellRect) const
    {
        // Every cell gives up the pixel an odd width shares on its bottom and right: a line's or,
        // in the last row and column, the frame's.
        const float shared = sharedLinePixel();
        FloatRect result = cellRect.inflated(-(m_scaledCellMetrics.border - shared) / 2.0f);
        result.right -= shared;
        result.bottom -= shared;
        return result;
    }

    float GridDescriptor::sharedLinePixel() const
    {
        // The scaler hands out stroke widths in whole pixels, so the width is odd or even.
        return static_cast<int>(m_scaledCellMetrics.border) % 2 ? 1.0f : 0.0f;
    }

    void GridDescriptor::setHoveredCell(Control* row, const Column* column, bool initiateHint)
    {
        if (!setHighlightedCell(m_hoverChannel, row, column))
            return;
        if (initiateHint)
        {
            Hint::stopAndHide();
            Hint::hoveredControlChanged();
        }
    }

    void GridDescriptor::setSelectedCell(Control* row, const Column* col)
    {
        // Read before the channel moves: the row being left is the other half of what the state
        // change reaches, and nothing else keeps it.
        Control* previousRow = m_selectChannel.row();
        if (!setHighlightedCell(m_selectChannel, row, col))
            return;
        if (previousRow != row)
            m_owner.selectedRowChanged(previousRow, row);
        if (m_selectionCnt)
            m_selectionChanged = true;
        else
            selectedCellChanged();
    }

    void GridDescriptor::beginCellSelection()
    {
        if (!m_selectionCnt)
            m_selectionChanged = false;
        ++m_selectionCnt;
    }

    void GridDescriptor::endCellSelection()
    {
        --m_selectionCnt;
        if (!m_selectionCnt && m_selectionChanged)
            selectedCellChanged();
    }

    void GridDescriptor::selectedCellChanged()
    {
        // TODO: emit the owner event.
        //
        // m_owner.selectedCellChanged();
    }

    void GridDescriptor::forgetRow(const Control* row)
    {
        for (HighlightChannel* hl : { &m_hoverChannel, &m_selectChannel })
        {
            hl->forgetRow(m_owner.animator(), row);
        }
        // A row built later at this address is a different row, and no press has landed on it.
        if (m_press.row == row)
            m_press = {};
    }

    bool GridDescriptor::setHighlightedCell(HighlightChannel& hl, Control* row, const Column* column) const
    {
        if (!hl.setCell(row, column))
            return false;
        // Everything that is not the new cell fades out, including the one we came from.
        for (CellHighlight& cell : hl.ring())
            if (!cell.vacant() && !cell.is(row, column))
                animateCell(hl, cell, 0.0f);
        if (row && column)
            animateCell(hl, hl.acquireCellHighlight(m_owner.animator(), *row, *column), 1.0f);
        return true;
    }

    // A grid standing in no form has no controller to run the fade on, and takes the end value
    // outright - the same answer Control::animate gives a control in no form.
    void GridDescriptor::animateCell(HighlightChannel& hl, CellHighlight& cell, float to) const
    {
        if (AnimationController* animator = m_owner.animator())
        {
            animator->start(&cell, hl.slot(), cell.factor, to, m_onCellAnimate);
            return;
        }

        AnimateParams params{ &cell, hl.slot(), to };
        m_onCellAnimate(params);
    }

    void GridDescriptor::onCellAnimated(AnimateParams& params) const
    {
        static_cast<CellHighlight*>(params.control)->factor = params.value;   // no slot branch
        m_owner.invalidate();
    }

    void GridDescriptor::setSelectedRow(Control* value)
    {
        setSelectedCell(value, m_selectChannel.column());
    }

    void GridDescriptor::recordPress(const Control& row, const Column* column,
        const CellPressKind cellPressKind)
    {
        m_press = { &row, column, cellPressKind };
    }

}
