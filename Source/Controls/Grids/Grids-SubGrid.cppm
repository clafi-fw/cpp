export module ClaFi.Controls.Grids :SubGrid;

import :GridBase;

import ClaFi.Controls.StackView;
import ClaFi.Core.Foundation;

namespace ClaFi::Controls::Grids
{
    // The body of a group or a section, filling the columns of the grid it stands in. See Grids
    export class SubGrid : public GridBase
    {
    public:
        using GridBase::GridBase;
    protected:
        // THE GRID ELEMENT IS STATED ONCE, BY THE GRID. A body inherits the surface that
        // statement arrived at, so stating it again applies grid.surface over a surface already
        // carrying it. PaintEvent gives a surface its alpha from how much the rule changed, so
        // the second application is what makes the body paint a fill of its own - over the
        // header a group lays across it, taking that header's cells and the controls standing
        // in them off the screen - and puts the grid's own frame inside the lattice.
        //
        // A body adds nothing of its own: its rows state what they need - see
        // RowBase::adjustPaint - and the surface they stand on is the group's.
        void adjustPaint(AdjustPaintEvent& event) override { StackView::adjustPaint(event); }
    };
}
