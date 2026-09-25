export module ClaFi.Controls.Grids :GridHeader;

import :RowBase;
import :Columns;
import :Cell;
import :Descriptor;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Foundation;


namespace ClaFi::Controls::Grids
{
    // The row of column names across the top of a grid.
    export class GridHeader : public RowBase
    {
    public:
        using RowBase::RowBase;
    public:
        static void defaultGetCellText(const Cell&, Text&);
        // The shape the header's cells fill: the row's rect with the corners a grid's first row
        // turns. Everything laid behind a held header is put on this rather than on the plain
        // rect - a square backdrop would fill the notch outside a rounded corner with the grid's
        // own colour and cut the shadow off there. See PaintEvent::paintHeldBackdrop.
        [[nodiscard]] RoundedRectangleParts silhouette(const PaintEvent& gridEvent, const FloatRect& headerRect) const;
    protected:
        bool hasContent(const Column& column) const override { return column.showInHeader() == ShowInHeader::Yes; }
        [[nodiscard]] bool alwaysDrawGridLines() const override { return true; }
        void getCellText(const Column&, Text&) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
    };
}
