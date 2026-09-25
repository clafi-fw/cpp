module ClaFi.Controls.Grids;

import :Row;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // Grid

    void Row::paintSurface(PaintEvent& event)
    {
        // The base paints no rect and says why. The cells are what this row adds, and they are
        // what carries its surface - and a group's span covers its group in the columns it
        // fills, so a row's own rect would run under it in any case.
        RowBase::paintSurface(event);
        doPaintColumns(event);
    }

}
