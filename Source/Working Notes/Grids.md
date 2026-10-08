# Grids

The author's side of the footnote of the same name.

## Picking

The row the press focuses records it, in `RowBase::nestedControlFocusing`, before
`selectColumnUnderMouse` moves the selection - the one moment both the old cell and the new one
are known. The record is one cell and what the press was to it, a `CellPressKind`, kept in
`GridDescriptor`. When the cell's control is about to drop a popup it asks, and
`RowContainer::nestedControlDroppingPopup` answers from that record and from
`DropPopupEvent::implicit` - see Control-Foundation#droppopupevent.
