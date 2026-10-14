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

## GetCellHintEvent

The row tells the hint when the link under the pointer changes, through Hint::hoveredZoneChanged,
as TextBox does - see the Controls note. A move tells it and a leave does not: the hover change has
told the hint already, and telling it again would start a second wait for whatever the pointer went
to.

## MovingText

Set per column, because a column is what says where a cell's text comes from.

## RowCell

Everything the walk knows of one cell and no more. A property of the row as a whole - whether the
row is the first of the grid - is the consumer's to carry.
