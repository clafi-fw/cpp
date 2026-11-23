export module ClaFi.Controls.Grids :Cell;

import :Columns;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine;
import ClaFi.Core.System.Events;
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

    class CellEventLocation
    {
    public:
        CellEventLocation(const RowBase&, const Column&);
        [[nodiscard]] Cell cell() const { return m_cell; }
        [[nodiscard]] const RowBase& row() const { return m_cell.row; }
        [[nodiscard]] const Column& column() const { return m_cell.column; }
    private:
        const Cell m_cell;
    };

    class CellEventBase : public Event, public CellEventLocation
    {
    public:
        using CellEventLocation::CellEventLocation;
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

    // The tooltip for a cell where the pointer is outside linked text. See Grids
    export class GetCellTooltipEvent : public CellEventBase
    {
    public:
        GetCellTooltipEvent(const RowBase&, const Column&, AdjustTooltipEvent&);
    public:
        [[nodiscard]] AdjustTooltipEvent& tooltip() { return m_tooltip; }
        [[nodiscard]] Text& text() { return m_tooltip.text; }
    private:
        AdjustTooltipEvent& m_tooltip;
    };

    // A link in one cell was followed. See Grids
    export class CellLinkClickEvent : public LinkClickEvent, public CellEventLocation
    {
    public:
        CellLinkClickEvent(RowBase&, const Column&, std::wstring target, InputStamp);
    };

    // The tooltip for a link in one cell is being asked for. See Grids
    export class GetCellLinkTooltipEvent : public GetLinkTooltipEvent, public CellEventLocation
    {
    public:
        GetCellLinkTooltipEvent(RowBase&, const Column&, std::wstring_view target, TextRange,
            AdjustTooltipEvent&, FloatRect anchorRect);
    };

    // The menu for a link in one cell is being asked for. See Grids
    export class CellLinkContextPopupEvent : public LinkContextPopupEvent, public CellEventLocation
    {
    public:
        CellLinkContextPopupEvent(RowBase&, const Column&, std::wstring target, TextRange,
            ContextPopupEvent&);
    };

    // Stored cell callbacks, not construction properties. A handler passed when a grid or a
    // row is built is an OnEvent and connects itself; these are the ones a cell spec holds on
    // to and moves around, so they need a concrete default-constructible type.
    export using CellTextFunc = std::function<void(GetCellTextEvent&)>;
    export using CellContentFunc = std::function<void(CellContentEvent&)>;
    export using AcceptCellTextFunc = std::function<void(AcceptCellTextEvent&)>;

    export struct ScaledCellMetrics : public ScaledControlMetrics
    {
        ScaledCellMetrics(const ControlMetrics&, const Scaler&);
        float border;
        float radius;
    };


    //-------------------------------------------------------------------------


    // CellEventLocation

    CellEventLocation::CellEventLocation(const RowBase& row, const Column& column)
        :
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

    GetCellTooltipEvent::GetCellTooltipEvent(const RowBase& row, const Column& column,
        AdjustTooltipEvent& tooltip)
        :
        CellEventBase{ row, column },
        m_tooltip{ tooltip }
    {
    }

    ScaledCellMetrics::ScaledCellMetrics(const ControlMetrics& desighMetrics, const Scaler& scaler)
        :
        ScaledControlMetrics{ desighMetrics, scaler },
        border{ scaler.scaledStrokeWidth(desighMetrics.border) },
        radius{ scaler.scaledStrokeWidth(desighMetrics.border) }
    {
    }

}
