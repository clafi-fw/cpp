export module ClaFi.Controls.Grids :RowNewItem;

import :Columns;
import :Descriptor;
import :Row;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // The grid whose new-item row has asked for an item. See Grids
    export class NewItemEvent : public Event
    {
    public:
        explicit NewItemEvent(GridBase&);
    public:
        [[nodiscard]] GridBase& grid() const { return m_grid; }
    private:
        GridBase& m_grid;
    };

    // The row a grid keeps after its last one, whose one cell asks for a new item. See Grids
    export class RowNewItem : public Row
    {
    public:
        RowNewItem(const CreateParams&, GridDescriptor&, PlaceHolderText);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"RowNewItem"; }
    protected:
        bool hasContent(const Column&) const override; // the root column, which spans every other
        void getCellText(const Column&, Text&) override;
        void nestedClick(ClickEvent&) override;
    private:
        PlaceHolderText m_placeHolderText;
    };
}
