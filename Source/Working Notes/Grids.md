# Grids

The author's side of the footnote of the same name.

## Picking

The row the press focuses records it, in `RowBase::nestedControlFocusing`, before
`selectColumnUnderMouse` moves the selection - the one moment both the old cell and the new one
are known. The record is one cell and what the press was to it, a `CellPressKind`, kept in
`GridDescriptor`. When the cell's control is about to drop a popup it asks, and
`RowContainer::nestedControlDroppingPopup` answers from that record and from
`DropPopupEvent::implicit` - see Control-Foundation#droppopupevent.

A click from the keyboard is never refused a popup: `pressOn` reads the device at the click, and a
key reaches a cell's control only through the selected cell. A double click on a cell that was not
selected picks and opens in one gesture - the row refuses the editor on the first click, and the
second lands on the cell it picked.

## RowContainer isChildWidthGiven

Without it a combo box sized its own column: a pixel short after a scale change, "No change" broke
onto two lines and the column shrank to its longer word.

## GetCellTooltipEvent

The row tells the tooltip when the link under the pointer changes, through
Tooltip::hoveredZoneChanged, as TextBox does - see the Controls note. A move tells it and a leave
does not: the hover change has told the tooltip already, and telling it again would start a second
wait for whatever the pointer went to.

## MovingText

Set per column, because a column is what says where a cell's text comes from.

## GridBase moveCellSelection

2026-10-17. The field was searched by score: every stop scored by its distance ahead of the
source and by how far its lane stood from the kept line, on OrientedRect and Score. It now reads
the stops into a table first - GridBase::FieldTable, bands from the distinct tops and bottoms and
columns from the distinct lane edges, edges within half a pixel made one - and every key is index
arithmetic over it. The rule is the scoring's: on the line beats nearer, and off the line the
nearest band ahead, then the nearest column. Oj picked the table over a structural address - leaf
columns from the column tree and rows from the row order plus sections and span rules - because
that would restate the layout rules the cell walk keeps in one place.

Two things moved with it:

- Every arrow is answered at the field root. Up and Down used to start at the innermost grid and
  travel outward, which held only while nearest over part of the field was nearest over all of
  it. With the line winning over distance, a sub-grid's nearest stop on the line can stand where
  the whole field has a nearer one past the sub-grid's rows.
- A selection scrolled out of view is read around itself, a viewport each way. The focus
  navigator once took such a key and searched around the selected row; it now hands a grid's
  arrows back as nothing - see GridBase navigationNeighbour.

## RowExpander navigationEntry

2026-10-18. Up from a footnote box under a SeeDocs table landed on the section's header instead
of the last row. The navigator enters the grid at its last child holding a stop, the RowExpander,
and asked it where a step from below lands; Control::navigationEntry's default answers the first
stop in reading order whichever way the step comes, which is the header. The override states
the layout alignContent makes: header over body. PageDown then stopped on every section header:
the steps of a page press walk navigationNeighbour, and the default leaves the expander from its
header. The neighbour override states the same layout for a step inside.

## RowCell

Everything the walk knows of one cell and no more. A property of the row as a whole - whether the
row is the first of the grid - is the consumer's to carry.

## RowBase paintSharedLinePixels

**A line of odd width has a middle pixel that neither half owns.** Each cell draws its half of a
line inside its own bounds, which is what lets a line between two differently coloured cells follow
both colours. At an even width each half is whole pixels. At an odd one - 1 px at 100% and 125%, 3
px at 250% - the middle pixel would be covered half by each cell, and two half coverages laid on one
pixel come out at 75%, not 100%.

**The middle pixel is the last pixel of the cell above or to the left.** That cell paints it first,
opaque: its line laid over its own surface. The cell below or to the right paints its own line over
its own surface on top, at half opacity. The pixel comes out as the mean of the two halves, and as
the line itself where the cells agree. A cell whose line the theme leaves out paints nothing there,
and its fill stands for its half.

**Every cell gives up one pixel at its bottom and right.** The frame takes the bottom pixel of the
last row and the right pixel of the last column inside their layout boxes, and a line takes it
everywhere else. So GridDescriptor::cellInnerRect insets the bottom and right by the shared pixel on
top of the halves, and the text, a hosted control and the in-place editor all centre between the
lines a cell actually shows. The cell's own half and its highlight stop short of that pixel too, so
the pixel a neighbour paints over afterwards is never part of a highlight.

**A row reaches one pixel past its edges for it.** RowBase::adjustViewPort widens the row's viewport
by the shared pixel at the top, except on the grid's first row, which has the frame above it, and at
the bottom. The viewport is both the clip the row paints in and the rect its invalidation covers, so
the one hook does both jobs: the row below can lay its half over the row above, and repainting
either row enters the other. A nested grid widens its own viewport the same way, in
GridBase::adjustViewPort, or it would clip its rows back to its bounds. At an even width the shared
pixel is 0 and none of this changes anything.

**The mix rests on paint order.** Rows paint top to bottom and cells left to right, so the cell that
owns the pixel paints it before the other lays its half. Two orders break it, and the pixel then
carries the owner's line alone: the header paints after the rows under it, and the cell walk visits
a parent column's cell after the sub-column cells under it.

**Cell edges are whole pixels.** paintOneCell rounds the cell rect, and a cell that ends the row
takes the row's own bottom. Sections are measured in fractions while the row is laid out rounded up,
which left each cell's bottom up to a pixel short of the next row and put its half of the line
across two pixels.

**An expander header's line takes part.** RowExpanderHeader draws its full line inside its own
bottom, so at 1 px that line is the header's last pixel, and the first row of the body lays its half
over it like any row under a row.

Not covered by this:
- A RowGroup laying its body over its header has not been looked at.
