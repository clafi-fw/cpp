module ClaFi.Controls.Grids;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Controls::Grids
{
    // Column

    Column::~Column()
    {
        if (m_subColumns)
            delete m_subColumns;
    }

    ColumnCollection& Column::createSubColumns()
    {
        if (!m_subColumns)
            m_subColumns = new ColumnCollection(m_descriptor, this);
        return *m_subColumns;
    }

    std::size_t Column::level() const
    {
        if (!m_parent)
            return 0;
        return m_parent->subColumns()->m_level;
    }

    bool Column::hasSubColumns() const
    {
        return m_subColumns && !m_subColumns->empty();
    }

    std::size_t Column::subColumnsCount() const
    {
        return m_subColumns ? m_subColumns->size() : 0;
    }

    // A column carries no hover or selection state of its own, and so occupies no slot in
    // the animation controller. Cell highlights are animated per cell by GridDescriptor, which
    // is the only way to express "this cell" rather than "this row" times "this column".

    void Column::takeOffWidth(float value)
    {
        value = std::min(value, m_calculatedWidth);   // never remove more than a leaf has
        Column* column = this;
        do {
            column->m_calculatedWidth -= value;
            column = column->m_parent;
            if (column)
                column->m_finalWidth -= value;
        } while (column);
    }

    void Column::traverseFittables(const FittableFunc func)
    {
        if (hasSubColumns())
            for (Column* col : *m_subColumns)
                col->traverseFittables(func);
        else
            func(*this);
    }

    void Column::addNeededWidth(const float w)
    {
        m_neededWidth = std::max(m_neededWidth, w);
    }

    float Column::subColumnsNeededWidth() const
    {
        float result = 0.0f;
        if (m_subColumns)
            for (const Column* col : *m_subColumns)
                result += col->m_neededWidth;
        return result;
    }

    void Column::clearCalculatedWidth()
    {
        m_left = 0;
        m_stretchedWidth = 0;
        m_stretchedCnt = 0;
        m_neededWidth = 0;
        m_calculatedWidth = 0;
        m_finalWidth = 0;
        // NOTE: do NOT invalidate the descriptor's cached layout here. This runs on the
        // hot path (preCalcRows calls it per column, per pass) and would wipe the value
        // calculateGrid() has just stored, disabling the cache entirely.
        // Invalidation belongs at the mutation site: ColumnCollection::invalidateLayout().
    }

    void Column::preCalcRows(GridBase& grid)
    {
        clearCalculatedWidth();

        if (m_subColumns)
            for (Column* column : *m_subColumns)
                column->preCalcRows(grid);

        // The mode decides, not whether a designWidth happens to be set. FitContent
        // ignores designWidth outright, and for Fill it is a stretch ratio spent in
        // stretchStretched() - never a width, so it must not be scaled as one.
        if (m_calcMode == ColumnWidthMode::Fixed)
            m_neededWidth = m_descriptor.scaler().scale(m_designWidth);
        else
            for (RowBase& row : grid.controlsAs<RowBase>())
                row.preCalcColumn(*this);

        if (m_calcMode != ColumnWidthMode::Fixed)
            addNeededWidth(subColumnsNeededWidth());
        m_calculatedWidth = m_neededWidth;
    }

    void Column::finishCalcStage1()
    {

        float subColumnsWidth = 0;
        // 1. writing fixed and calculated, accumulating stretched
        if (m_subColumns)
            for (Column* column : *m_subColumns)
            {
                column->finishCalcStage1();
                if (column->m_calcMode == ColumnWidthMode::Fill)
                {
                    ++m_stretchedCnt;
                    m_stretchedWidth += column->m_finalWidth;
                }
                subColumnsWidth += column->m_finalWidth;
            }
        m_subColumnsWidth = subColumnsWidth;              // new member
        m_finalWidth = std::max(m_calculatedWidth, subColumnsWidth);
    }

    void Column::stretchStretched(const float boundsW)
    {
        m_calculatedWidth = boundsW;
        m_calculatedSubColumnsWidth2 = m_finalWidth;
        if (!m_subColumns)
            return;

        // What the children divide: my width less the non-stretching children.
        // Picks up both parent-supplied slack and the internal hole left when my
        // own content is wider than my subcolumns.
        float remainWidth = std::max(boundsW, m_finalWidth) - m_subColumnsWidth + m_stretchedWidth;

        if (m_stretchedWidth && m_stretchedWidth < remainWidth)
        {
            // Fill children divide the slack in their declared ratios, not in proportion
            // to what they measured. With a single Fill child the ratio cannot matter -
            // it takes the whole remainder either way.
            float totalRatio = 0.0f;
            for (const Column* column : *m_subColumns)
                if (column->m_calcMode == ColumnWidthMode::Fill)
                    totalRatio += column->fillRatio();

            const float slack = remainWidth;
            for (Column* column : *m_subColumns)
                if (column->m_calcMode == ColumnWidthMode::Fill)
                {
                    --m_stretchedCnt;
                    // The last one absorbs the division remainder.
                    const float newColWidth = m_stretchedCnt
                        ? slack * column->fillRatio() / totalRatio
                        : remainWidth;
                    m_finalWidth += newColWidth - column->m_calculatedWidth;
                    column->stretchStretched(newColWidth);
                    remainWidth -= newColWidth;
                }
        }

        // Non-stretching children still need to lay out their own subtree,
        // which is where a nested Fill under a FitContent parent gets reached.
        for (Column* column : *m_subColumns)
            if (column->m_calcMode != ColumnWidthMode::Fill)
                column->stretchStretched(column->m_finalWidth);
    }

    void Column::calcLeft(float left)
    {
        m_left = left;
        if (m_subColumns)
            for (Column* column : *m_subColumns)
            {
                column->calcLeft(left);
                left += column->m_calculatedWidth;
            }
    }

    // ColumnCollection

    void ColumnCollection::invalidateLayout() const
    {
        // k_maxFloat, not 0: boundsW == 0 is reachable during early layout and would
        // read back as a valid cache hit.
        m_descriptor.m_lastBoundaryWidth = k_maxFloat;
    }

    ColumnCollection::ColumnCollection(GridDescriptor& descriptor, Column* parent) :
        m_descriptor{ descriptor },
        m_parent{ parent }
    {
        if (parent)
            m_level = parent->level() + 1;
        else
            m_level = 0;
    }

    ColumnCollection::~ColumnCollection()
    {
        for (const Column* column : *this)
            delete column;
    }

    Column* ColumnCollection::findByTag(Tag tag) const
    {
        for (Column* column : *this)
        {
            if (column->tag() == tag)
            {
                return column;
            }
            if (const ColumnCollection* subColumns = column->subColumns())
                if (Column* found = subColumns->findByTag(tag))
                {
                    return found;
                }
        }
        return nullptr;

    }

    Column& ColumnCollection::byTag(Tag tag) const
    {
        Column* result = findByTag(tag);
        if (!result)
            unreachable("Grid: no column carries the requested tag");
        return *result;
    }
}
