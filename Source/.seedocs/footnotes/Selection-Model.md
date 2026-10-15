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

## StackView setSelection

Items the view would refuse the user are left out - see CanSelectItemEvent. A view holding no set
ignores the call.

**The current item stays where it is.** A grid moves both with GridBase::selectCell.

## SelectionMode

**A Shift range runs from the anchor to the current item**, and every pick without Shift moves the
anchor. The anchor and the current item join the set only when something puts them there. So a
mouse press on a current item outside the set picks it.

Tracking a current item at all is what `Interactivity::ActiveContainer` means, and a StackView
always is one.

## DragMode

A drag from the surface sweeps a rectangle in either mode.

- `EasySelect` - for items that stay put and a selection that is only read.
- `EasyDrag` - for items that move. A drag from a selected item carries the selection - see
  ItemsOrder - and from any other item carries that item alone.

## CanSelectItemEvent

**Separate from whether an item may be current**, which is CanFocusItemEvent. A rubber band needs
answers for controls it never makes current.

canSelect arrives as the view's own answer - yes for a Focusable item in a Multi view.
