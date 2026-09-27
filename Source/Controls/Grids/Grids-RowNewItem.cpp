module ClaFi.Controls.Grids;

import :Descriptor;
import :GridBase;
import :RowNewItem;

import ClaFi.Icons.PlusMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    namespace
    {
        // The plus the placeholder leads with, in the placeholder's own ink.
        void paintMutedPlus(PaintIconEvent& event)
        {
            Icons::PlusMark mark{
                .canvas = event.canvas(),
                .center = event.iconCenter(),
                .size = event.iconWidth(),
                .lineWidth = event.scaledStrokeWidth(Thickness::Thin),
                .color = event.textRgb(InkGrade::Muted)
            };
            mark.paintPlusOrMinus(true);
        }
    }

    // NewItemEvent

    NewItemEvent::NewItemEvent(GridBase& grid)
        :
        m_grid{ grid }
    {
    }

    // RowNewItem

    RowNewItem::RowNewItem(const CreateParams& params, GridDescriptor& descriptor,
        PlaceHolderText placeHolderText)
        :
        Row{ params, descriptor },
        m_placeHolderText{ std::move(placeHolderText) }
    {
    }

    bool RowNewItem::hasContent(const Column& column) const
    {
        return &column == &m_descriptor.rootColumn();
    }

    void RowNewItem::getCellText(const Column&, Text& text)
    {
        text << InkGrade::Muted << InTextIcon{ 12.0f, 12.0f, paintMutedPlus } << L" "
            << m_placeHolderText;
    }

    // Return and Space land here as well: the focus navigator presses the row the grid's focus
    // stands for, which is this one while its cell is selected.
    void RowNewItem::click(ClickEvent& event)
    {
        Row::click(event);
        parentAs<GridBase>().requestNewItem();
    }
}
