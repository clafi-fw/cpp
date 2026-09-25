export module ClaFi.Controls.Grids :Cell;

import :Columns;

import ClaFi.Core.System.Events;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{

    export class Cell
    {
    public:
        Cell(const RowBase& row, const Column& column) : row{ row }, column{ column } {}
        const RowBase& row;
        const Column& column;
    };

    class CellEventBase : public Event
    {
    public:
        CellEventBase(const RowBase&, const Column&);
        Cell cell() const { return m_cell; }
        const RowBase& row() const { return m_cell.row; }
        const Column& column() const { return m_cell.column; }
    private:
        const Cell m_cell;
    };

    // The text one cell is about to be written with.
    export class GetCellTextEvent : public CellEventBase
    {
    public:
        GetCellTextEvent(const RowBase&, const Column&, Text&);
        Text& text() { return m_text; }
    private:
        Text& m_text;
    };

    // The text an in-place edit was left with, on its way back to the cell's source. See Grids
    export class AcceptCellTextEvent : public CellEventBase
    {
    public:
        AcceptCellTextEvent(const RowBase&, const Column&, const Text&);
        const Text& text() const { return m_text; }
        void refuse(std::wstring_view why) { m_reason = why; }
        [[nodiscard]] const std::wstring& reason() const { return m_reason; }
    private:
        const Text& m_text;
        std::wstring m_reason{};
    };

    // The control one cell holds, for a cell that carries more than text.
    export class CellContentEvent : public CellEventBase
    {
    public:
        using CellEventBase::CellEventBase;
        bool hasContent{ true };
    };

    // The hint for one cell, asked while the pointer rests on it. See Grids
    export class GetCellTooltipEvent : public CellEventBase
    {
    public:
        GetCellTooltipEvent(const RowBase&, const Column&, GetTooltipEvent&);
        [[nodiscard]] GetTooltipEvent& tooltip() { return m_tooltip; }
        [[nodiscard]] Text& text() { return m_tooltip.text; }
    private:
        GetTooltipEvent& m_tooltip;
    };

    // Stored cell callbacks, not construction properties. A handler passed when a grid or a
    // row is built is an OnEvent and connects itself; these are the ones a cell spec holds on
    // to and moves around, so they need a concrete default-constructible type.
    export using CellTextFunc = std::function<void(GetCellTextEvent&)>;
    export using CellContentFunc = std::function<void(CellContentEvent&)>;
    export using AcceptCellTextFunc = std::function<void(AcceptCellTextEvent&)>;

    export struct ScaledCellMetrics : public AlignEvent
    {
        using AlignEvent::AlignEvent;
        float border;
        float radius;
    };


    //-------------------------------------------------------------------------


    // CellEventBase

    CellEventBase::CellEventBase(const RowBase& row, const Column& column)
        :
        Event{},
        m_cell{ row, column }
    {
    }

    // GetCellTextEvent

    GetCellTextEvent::GetCellTextEvent(const RowBase& row, const Column& column, Text& text)
        :
        CellEventBase{ row, column },
        m_text{ text }
    {
    }

    // AcceptCellTextEvent

    AcceptCellTextEvent::AcceptCellTextEvent(const RowBase& row, const Column& column, const Text& text)
        :
        CellEventBase{ row, column },
        m_text{ text }
    {
    }

    // GetCellTooltipEvent

    GetCellTooltipEvent::GetCellTooltipEvent(const RowBase& row, const Column& column, GetTooltipEvent& tooltip)
        :
        CellEventBase{ row, column },
        m_tooltip{ tooltip }
    {
    }

}
