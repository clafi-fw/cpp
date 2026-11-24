# Selection Model

What StackView holds on top of the current item every Stack tracks, and the two enums that shape
it. Referenced from `Source/Controls/Controls.StackView.cppm`.

## StackView

With `SelectionMode::Multi` it holds several items selected.

**The current item and the selection are separate.** The current item belongs to the Stack
underneath, and follows the keyboard and plain mouse clicks. The selection belongs to the view:

- Shift spans a range.
- Ctrl with a mouse click, or Ctrl+Space, toggles an item.
- A rubber band sweeps the items it crosses.
- Ctrl with a navigation key moves the current item alone.

Ranges and rubber bands reach items in nested panels too.

**A mouse press that may start a drag waits for its click:**

- with Ctrl or Shift, it may become a rubber band;
- inside the selection, it may become a drag of the selection.

A plain press outside the selection selects at once.

**A view standing in a focus scope keeps no set of its own.** Built with FocusScope::No, it is
given no SelectionMode: the scope above holds the set, as it holds the current item, and the view
reads it through Control hold - the set a drag from one of its items carries, in the order the items
stand, and the held look of a grid's row. A page holding a Multi StackView over several grids is
one selection over all of them, and CanSelectItemEvent on the page says which items it lets in.

## StackView setSelection

**The selection becomes the items CanSelectItemEvent lets in**, and the rest are dropped without
a word - the same test a Ctrl+click or a rectangle passes. A view holding no set ignores the call.

**The current item stays where it is.** A grid moves both with GridBase::selectCell.

## SelectionMode

**A Shift range runs from the anchor to the current item**, and every pick without Shift moves the
anchor. The anchor and the current item join the set only when something puts them there. So a
mouse press on a current item outside the set picks it.

Tracking a current item at all is what `FocusScope::Yes` means, and a StackView is one unless it
is built otherwise.

## DragMode

A drag from the surface of a multi-selection view sweeps a selection rectangle.

- `EasySelect` starts a selection rectangle from an item.
- `LiveShuffle` moves items among their siblings as the pointer crosses them. See ItemsOrder.
- `ZoneDrop` offers Before in the top 30% of a row and Inside below it. At the end of a
  group, the bottom 15% offers Before on the following outer row, or After on the last row
  when there is no following row. An open header with visible children keeps its Inside zone.

**The view asks CanMoveItemEvent about the source.** ItemsOrder::Rearrangeable enables moving
visible items. A multi-selection view carries the selected siblings when the press lands on one.

**CanDropItemsEvent asks the application about a destination.** Set allowed to accept the zone.
Set hint to the text describing the drop. The pointer holds a translucent snapshot of the pressed
row at the original click point. The action badge sits below the row, with its left edge aligned
to the pointer. The window grows to fit the badge and its shadow. The action icon shows an arrow
entering a box for Inside or pointing between rows for insertion. An empty hint reads "Move here".
A rejected destination shows only the rejection icon.
The overlay highlights the row for Inside or draws a line for insertion. In a tree, the line
begins at the destination row's icon column. Holding over Inside for half a second opens the node.

**ItemsDropEvent delivers an accepted release.** Apply the edit to the model using items, target
and zone. Defer rebuilding the pressed controls until the input event returns. Escape cancels
the gesture.

## CanSelectItemEvent

**Separate from whether an item may be current**, which is CanFocusItemEvent. A rubber band needs
answers for controls it never makes current.

canSelect arrives as the view's own answer - yes in a Multi view for an item that answers
NavigationReach::Yes and MouseFocus::Self and is no focus scope itself.

**It is also what Control hold reads**, so an item refused here stands under no set - see ItemHold.
