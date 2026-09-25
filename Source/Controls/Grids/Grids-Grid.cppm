module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Grids :Grid;

// Partitions are not visible to one another - each one imports what it names.
// These came free while this file was the primary interface and did the
// `export import :X;` for the whole module; as a partition it must ask.
import :Cell;
import :Columns;
import :Descriptor;
import :GridBase;
import :RowBase;
import :GridHeader;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

import ClaFi.Diagnostic.Log;

namespace ClaFi::Controls::Grids
{
    export class Grid : public GridBase
    {
        friend RowBase;
        friend GridHeader;
    public:
        template<typename... Args>
        Grid(const CreateParams& params, Args&&... args);
    public:
        EventConnection onCellContent(auto&& callback) {
            return connectEvent<CellContentEvent>(std::forward<decltype(callback)>(callback));
        }
        EventConnection onGetCellText(auto&& callback) {
            return connectEvent<GetCellTextEvent>(std::forward<decltype(callback)>(callback));
        }
        EventConnection onAcceptCellText(auto&& callback) {
            return connectEvent<AcceptCellTextEvent>(std::forward<decltype(callback)>(callback));
        }
    protected:
        // Cell Content
        virtual void hasCellContent(CellContentEvent&) const;
        virtual void getCellText(GetCellTextEvent&);
        virtual void acceptCellText(AcceptCellTextEvent&);
        virtual void getCellTooltip(GetCellTooltipEvent&);
        virtual void getHeaderCellText(const Cell&, Text&);
        //
        void calculateChildren(FormBase&) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void paintChildren(PaintEvent&) override;
    private:
        GridDescriptor m_descriptor;
        float m_calculateLimit{ k_maxFloat };
    };


    //-------------------------------------------------------------------------


    template <typename ... Args>
    Grid::Grid(const CreateParams& params, Args&&... args)
        :
        GridBase{
            params,
            m_descriptor,
            Interactivity::ActiveContainer,
            std::forward<Args>(args)...
        },
        m_descriptor{ *this,
            READ_PROPERTY(ViewMode, ViewMode::Sheet), // how the grid lays its rows out
            READ_PROPERTY(GridLines, GridLines::Both) } // which of the grid's inner lines are drawn
    {
    }


    //-------------------------------------------------------------------------


    // Grid

    void Grid::hasCellContent(CellContentEvent& event) const
    {
        emitEvent(event);
    }

    void Grid::getCellText(GetCellTextEvent& event)
    {
        emitEvent(event);
    }

    void Grid::acceptCellText(AcceptCellTextEvent& event)
    {
        emitEvent(event);
    }

    void Grid::getCellTooltip(GetCellTooltipEvent& event)
    {
        emitEvent(event);
    }

    void Grid::getHeaderCellText(const Cell& cell, Text& text)
    {
        GridHeader::defaultGetCellText(cell, text);
    }

    void Grid::calculateChildren(FormBase& form)
    {
        m_descriptor.calculateGrid(0 /*scrollBarWidthAndSpacing*/, m_calculateLimit);
        GridBase::calculateChildren(form);
    }

    ScaledDimensions Grid::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result = GridBase::calculateContent(event);
        float border = m_descriptor.scaledCellMetrics().border;
        result.x += border;
        result.y += border;
        // The border stands outside the cells, so a floor the rows state carries it too. A floor
        // of zero carries nothing: a grid nothing inside holds up gives way whole.
        if (event.calculatedMinSize.x > 0.0f)
            event.calculatedMinSize.x += border;
        if (event.calculatedMinSize.y > 0.0f)
            event.calculatedMinSize.y += border;
        return result;
    }

    void Grid::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& contentSize)
    {
        // TODO: only if calculated width > contentSize.x
        float tmp = event.maxContentWidth();
        //if (tmp != k_maxFloat)
        //    tmp -= m_descriptor.scaledCellMetrics().border;
        float border = m_descriptor.scaledCellMetrics().border;
        contentSize.x = std::min(contentSize.x, tmp) - border;
        m_calculateLimit = contentSize.x;
        calculate(form());
        m_calculateLimit = k_maxFloat;
        position += border;
        GridBase::alignContent(event, position, contentSize);
        contentSize.x += border;
    }

    // The base, not StackView: a floating header is painted by GridBase and by nothing else, and
    // the ordered child range stops reaching it one header's height into the scroll.
    void Grid::paintChildren(PaintEvent& event)
    {
        m_descriptor.setPaintGridEvent(event);
        GridBase::paintChildren(event);
    }

}
