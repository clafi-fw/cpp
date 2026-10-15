export module ClaFi.Controls.Grids :Cell;

import :Columns;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.System.Events;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine;
import ClaFi.Core.System.UiTypes;
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

    // The value an in-place editor opens on for one cell, where it is not what the cell shows.
    export class GetCellEditorTextEvent : public CellEventBase
    {
    public:
        using CellEventBase::CellEventBase;
        std::optional<Text> text{};   // left empty, the editor opens on what the cell shows
        Suggester* suggester{ nullptr };   // what the editor completes from, where the cell has one
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
    export class GetCellHintEvent : public CellEventBase
    {
    public:
        GetCellHintEvent(const RowBase&, const Column&, GetHintEvent&, std::wstring_view link);
        [[nodiscard]] GetHintEvent& hint() { return m_hint; }
        [[nodiscard]] Text& text() { return m_hint.text; }
        // What the link under the pointer names, as the cell's text states it; empty off a link.
        [[nodiscard]] std::wstring_view link() const { return m_link; }
    private:
        GetHintEvent& m_hint;
        std::wstring_view m_link;
    };

    // A link in one cell's text was clicked. It goes to the row, then to the grid. See Grids
    export class CellLinkClickEvent : public CellEventBase
    {
    public:
        CellLinkClickEvent(const RowBase&, const Column&, std::wstring target, InputStamp);
    public:
        const std::wstring target; // what the link was written with
        const InputStamp stamp;    // the click the link answers
    };

    // The menu for a link in one cell's text is being asked for; the row, then the grid. See Grids
    export class CellLinkContextPopupEvent : public CellEventBase
    {
    public:
        CellLinkContextPopupEvent(const RowBase&, const Column&, std::wstring target,
            ContextPopupEvent&);
    public:
        const std::wstring target;   // what the link was written with
        ContextPopupEvent& popup;    // the request; a handler that has shown a menu stops it
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

    // GetCellHintEvent

    GetCellHintEvent::GetCellHintEvent(const RowBase& row, const Column& column, GetHintEvent& hint,
        const std::wstring_view link)
        :
        CellEventBase{ row, column },
        m_hint{ hint },
        m_link{ link }
    {
    }

    // CellLinkClickEvent

    CellLinkClickEvent::CellLinkClickEvent(const RowBase& row, const Column& column,
        std::wstring linkTarget, const InputStamp inputStamp)
        :
        CellEventBase{ row, column },
        target{ std::move(linkTarget) },
        stamp{ inputStamp }
    {
    }

    // CellLinkContextPopupEvent

    CellLinkContextPopupEvent::CellLinkContextPopupEvent(const RowBase& row, const Column& column,
        std::wstring linkTarget, ContextPopupEvent& popup)
        :
        CellEventBase{ row, column },
        target{ std::move(linkTarget) },
        popup{ popup }
    {
    }

}
