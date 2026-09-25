# Selection Model

What StackView holds on top of the current item every StackPanel already tracks, and what the
two enums that shape it mean. Referenced from `Source/Controls/StackView.cppm`.

## StackView

StackView is a StackPanel that traps its children's focus, and that can hold several items
selected at once when it is given `SelectionMode::Multi`.

The two ideas stay apart. The focused item belongs to the StackPanel underneath and moves with
the keyboard and with plain clicks; the selection belongs to the view, and is spanned with
Shift, toggled with Ctrl and a click or Ctrl+Space, and swept by dragging a rubber band across
the items. Ctrl held over a navigation key moves the focused item alone and leaves the
selection standing. Items sitting in panels nested inside the view are selectable too - a range
and a rubber band both walk the whole subtree, not just the direct children.

An action decides nothing on its own wherever what it means depends on the action that follows
it - see `StackPanelBase::appliesGesture`.

## SelectionMode

Whether a view holds a set of selected items on top of the current item that every StackPanel
already tracks. A Shift range runs between the selection anchor and the current item, and the
anchor is recorded by every pick that is not a Shift. Neither the anchor nor the current item
is a member of the set unless something put it there.

This is the view's own axis, and the only one it has. Whether the container tracks a current
item at all is the other, and it is not a mode: it is what makes a container an
`Interactivity::ActiveContainer`, which a StackView always is.

## DragMode

What a drag that starts on an item does. A drag starting on the surface sweeps a selection
rectangle in either mode.

`EasySelect` - an item is one more place to start a rectangle from. For a view whose items stay
where they are and whose selection is only read.

`EasyDrag` - an item is the selection's handle: the drag carries what is selected, and a
rectangle is swept from the surface alone. For a view whose items are moved.

## CanSelectItemEvent

`CanSelectItemEvent` is the view's own question, and nothing but the view asks it - a rubber
band sweeping the subtree wants an answer for controls it never intends to make current.
Whether an item may be the CURRENT one is asked separately, a level down - see
`CanFocusItemEvent` in `StackPanelBase`.
