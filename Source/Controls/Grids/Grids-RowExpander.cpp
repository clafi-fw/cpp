module ClaFi.Controls.Grids;

import :RowExpander;
import :RowGroupBase;

import ClaFi.Core.Foundation;

namespace ClaFi::Controls::Grids
{
    // RowExpander

    void RowExpander::getControlState(GetStateEvent& event) const
    {
        // The base is what ends the walk, so StackPanelBase above still does not get to answer
        // with its current item. Only the selection is declined.
        RowGroupBase::getControlState(event);
        if (&event.control == this)
            event.state.selected = false;
    }

}
