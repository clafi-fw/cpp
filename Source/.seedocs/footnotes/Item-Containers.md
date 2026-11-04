# Item Containers

The words that do not fit above a declaration.

## ContainerBase

**Overlay entries change paint order and clipping only.** addOverlayControl lists a control at
any depth below this container, to paint in a second pass over the other children. Listing one
outside its tree has no effect.

ClippingMode::Standard clips the control to its own bounds, and Unbounded lets it paint past them.
Both stay inside this container's clip.

**The host need not be the parent**: it sets how far the second pass reaches. A grid lists its
header with a container outside the grid, so the header's shadow falls on the page.

Each paint walks a hosting container's children twice.

## ContainerBase currentItem

**The item a show enters the container at.** When a container is shown with the focus going into
it - its form's window coming up, or a page control turning to it while the page control holds the
focus - the focus goes into the current item, and to the first stop in reading order where there
is none. Each container on the way is entered by the same rule, so a page whose current item is a
panel lands inside that panel, at the panel's own current item - see FocusNavigator enterShown.

- A form built on a panel whose current item is its left bar opens with the focus on the left
  bar's first stop.
- A dialog opens on its default answer, which is its root's current item - see MessageBoxBase.

**A focus scope moves its current item with the focus**, so a show brings the focus back to the
item the user left. Any other container keeps the item it was given. Tab and the arrows follow a
focus scope's current item alone - see FocusScope.

The item is any control inside the container, or none, and it is dropped when it is deleted.

## ContainerBase takeCurrentItem

**A window's root takes what no container below it took**, as the item a show enters the window at.
Control::makeCurrentItem on a control with no focus scope above it names where its window opens,
so code says it before the window is shown - the focus itself lands only in a window that is up.
A container that is not a root answers no, and the question goes on up.

## StackBase

**One current item**, the one every container keeps - see ContainerBase currentItem. A PageControl
shows its page through it, and a TabStrip its open tab. For several items selected at once, see
StackView.

**Current is not focused.** The item stays current while the focus is anywhere else, so its cue,
VisualStateIndex::Current, is not gated on the keyboard.

**A focus scope moves the current item itself**, following whatever the user acts on - a click,
or the focus arriving from the keyboard - holds the focus for it, and answers for which of its
items is selected - see FocusScope. A container whose current item is settled elsewhere is no
scope: PageControl's is whichever page was made visible, and the tab strip is what makes one
visible.

**A container standing in a focus scope is no scope of its own.** A grid on a page that keeps the
current item is built with FocusScope::No, NavigationReach::No and MouseFocus::Parent: its rows are
the page's items, one selection stands on the whole page, and the arrows and the page keys travel
over every item the page holds - see Grid. Code that picks an item calls Control::makeCurrentItem on
it, which reaches whichever container keeps the current item.

**What the current item leaves is told** - see Control nestedCurrentItemChange - so a grid the
item has left drops its selected cell. The focus a container holds moves with its item, and the
move is told as any other - see Input focusDelegateChanged.

## Expander

**Shut over the container's current item, its header stands for the item.** A section on a page
that keeps the current item is shut from a row inside it, and the header is what the next key
moves on from.

## CanFocusItemEvent

Asked of every item that could become current. `canFocus` starts as the container's answer, and a
handler that knows more can change it.

**Selection is a separate question** - see CanSelectItemEvent. A rubber band asks only that one,
and a container with no selection asks only this one.

## ItemsOrder

Under `Rearrangeable` a drag from an item carries it along the lane, and the other items make
room as it passes: the order shown is the order the release leaves. Escape puts the run back.

**The item is the stack's own child** the press landed in. In a stack following the user it has
to be the item the press names, so an item inside a group moves nothing. A drag some control
inside the item has taken for its own - a text box selecting - is no carry.

- A `Horizontal` or `Vertical` stack only. A wrapping stack carries nothing.
- An item lands among the movable items around it. One the stack or a handler keeps in place -
  a grid's header, its new-item row - bounds the run.
- A StackView under SelectionMode::Multi and DragMode::EasyDrag carries the selection, as one
  block - see DragMode.

## CanMoveItemEvent

Asked of the pressed item and of every sibling around it, so a handler that keeps one item in
place also bounds where the others may go. `canMove` starts as the stack's answer.

## ItemsMoveEvent

**The stack has moved its children**; whoever keeps the order they show is told where the items
stand now, and reads the rest off the children. Raised as the press ends, inside the release, so
a handler that would delete the items defers that.

## StackPanelBase::moveItems

The items go in the order given, as one block from `to` on. ItemsMoveEvent follows.

## Stack

**LaneSize breaks the lane**, in the measure and in the align alike. A stack of four to a lane
never takes a fifth, however much room its host gives - the room goes around the lane. A length
too short for four breaks the lane sooner. Hidden items take no place in a lane.

ItemSizing::Equal cuts the lanes into equal shares - see ItemSizing.

### Filling the lane, and filling across it

A wrapping stack hands out the room it was given beyond what its lanes take.

- **Across the lanes** - the room left once every lane is at its own thickness goes to the lanes
  in equal parts. A list in a window wider than its columns fills the window.
- **Along the lanes** - only where the stack itself fills that way: HorizontalAlign::Fill for
  rows, VerticalAlign::Fill for columns. The room goes to the items that fill the same way. What
  the length has over the longest lane is divided by the most such items any lane holds, so every
  one of them grows by the same amount, the lane holding the most ends at the length, and a short
  last lane stays short.
- **An item aligned to an edge takes none of it.** It keeps the size it measured and its place in
  the lane, and a lane of such items leaves the room at its end - the items stand packed.
- **A lane holding an item that fills the lane** hands all its room to that item, whatever the
  stack's alignment - see Control::fillsLane and FlexSpacer. Several such items share it evenly.

### A wrapping stack and the length it is given

**The lanes break at the length the host gives** - the width for rows, the height for columns -
where that length is the host's to give. The stack remembers the length its last layout broke the
lanes at, and the next measure breaks them there. A changed length asks for one more pass through
AlignEvent::invalidatePass, and that pass returns the same length.

- **In a window**, a stack that fills its host breaks its lanes at the length the window gives.
- **In a host sized by what it holds**, the stack keeps its lane count. That host has no length
  of its own to fill yet.

A stack of columns in a side bar is the first kind: the window sets the bar's height, the columns
break at it, and the bar is as wide as the columns that makes.

**A first open shows the lane count's answer, then the length's.** The window is placed from the
first answer, and the pass that lays the content into it brings the length. A page that comes out
shorter keeps the window it asked for.

**Any bound carried over from the last pass needs three guards** - this length, and the viewport a
ScrollBox states for its body (see ScrollBox::adjustChildMetrics):

- **Stored in design units**, with the scale that laid it out. Kept in pixels, it would latch a
  form taken to 250% and back at a fraction of its length.
- **Kept only where the length is given from outside**, asked all the way up - see
  Control::isHeightGivenFromOutside. Where a control keeps the size it measured, the bound comes
  from the thing it bounds: the rows wrap, the stack narrows, the next pass wraps that, and a
  Left-aligned stack walks from five items a row down to two.
- **Set aside while the form measures for a window** - FormBase::isMeasuringPlacement. A form
  held to the size it was last given could never grow.

### A column on a box that scrolls

**A stack of columns on a box that scrolls vertically breaks only at its own maximum.** The box
hands its body the viewport whatever the body measured, so the stack is granted at least the
height it measured, and the align cuts the lanes the measure counted. It remembers no length
either - see ScrollBox::isChildHeightGiven. A list that scrolls never turns into one that flows
sideways.

- ScrollBars Both, Vertical and Auto scroll the height.
- ScrollBars::Horizontal hands the viewport as the height, and the columns break at it: a list of
  columns flowing sideways.
- A stack of rows takes none of this: rows break at the width by design, and the measure already
  runs against it.

### Navigation

**A step along the lanes goes to the next item in the order**, so in a wrapping stack it runs on
from the end of one lane to the start of the next. **A step across them goes to the item of the
next lane nearest the line** - see NavigationStep. A stack of one lane hands a step across it to
its host.

**The lanes are read off where the align put the items.** An item starts a lane where it goes back
along the lanes and stands past the whole of the item before it across them. Hidden items are not
read.

## PreviewMode

- `Focus` - the current item, whatever moved it. A pointer crossing the stack previews nothing.
- `Hover` - the item under the pointer, and the current item whenever it moves. Leaving the stack
  returns the preview to the current item.

**Focus is the default**: a pointer passing over a list has not asked for a preview.

## PreviewEvent

**A look, not a pick.** The event lets a container show something about the item being looked at
before the user commits - the theme browser wears the theme of the tile in question. PreviewMode
says which item that is.

**Raised once the item settles**, a moment after it is reached. A held key or a pointer sweeping
the stack raises one preview, for the item it stops on.

## LaneSizing

**A ceiling is spread, a count is not.** Under `UpTo` every lane but the last holds the same
number: eleven items under a ceiling of ten make lanes of six and five, where `Exact` makes ten and
one. Hidden items are not counted.

## ItemSizing

**Equal hands every item the same share of the lane**, whatever it measured - a row of answer
buttons reads as one band.

- **A single lane** measures as the largest item times the count, with the gaps. Divided from the
  natural total instead, Save, Discard and Cancel would get three shares too small for Discard.
- **A wrapping stack** cuts every lane into the same number of shares: as many shares of the
  largest item as the length holds, capped by LaneSize, and at least one. A short last lane keeps
  the tracks above it and leaves its spare shares empty. The measure is unchanged.

**The share is a box, not a size.** An item that fills takes the whole share; one aligned to an
edge or centred keeps its own size inside it. Filled items all come out the same width.

## PageControl

**The focus goes with the switch.** A page control holding the focus - a container holds what its
child holds, so this is a focus in the page that shows - enters the page it turns to as a show
enters it: at the page's current item, else at its first stop - see ContainerBase currentItem. A
focus elsewhere, in the tab strip or a sidebar, stays where it is.

**A replaced page is shown before its predecessor goes.** A page deleted while it holds the focus
drops it, and nothing is told; a page hidden by the switch still holds it when the page control
reads where the focus is. So the successor is added and shown first, and the page it replaces is
deleted after.

## PageSizing

- `CurrentPage` - hidden pages are never measured.
- `WidestPage` - the largest page in both extents, so turning to another page does not resize what
  holds the page control. The application menu is as big as its largest page.

**Under WidestPage the hidden pages are measured once**, and again when a page is added or removed
or the form's scale changes. A hidden page that grows on its own at the same scale keeps the size
it was measured at.

## Tab

**A press opens the tab and leaves the focus where it is.** A focus in the page goes on to the page
the tab opens - see PageControl - and a focus anywhere else stays there. The keys still reach the
tab.

**Every tab is its strip's item, whatever MouseFocus it answers.** The strip takes each tab it holds
as an item, and a press makes the pressed tab current. A tab that is to take the focus on a mouse
press states MouseFocus::Self.

## TabStripBase::adjustMetrics

**A Tab strip pads its outer side only**, away from the page. The open tab's line runs along the
inner edge, where the page starts, and a padding there would lift the line off the page.

- **Along the strip** - the padding is the room the tab line runs on past the first and the last
  tab, at both ends.
- **Across a Tab strip** - the padding is on the outer side. The spacing is the gap an item other
  than a tab keeps from the line - a button or a check box among the tabs. Tabs reach the line
  whatever the spacing.
- **A ToolButton strip** - the padding is on every side, and no item keeps a gap from a line.

**The strip lays the outer padding out itself**: `padding()` is the stated value, while
`scaledPadding()` is the layout's, zero across a Tab strip. The paint, the hit test and the tab
line all find the inner edge.

On a TabbedBox, StripPadding and StripSpacing replace the padding and spacing the box gives its
strip for each mode.

## ScrollBox::isChildHeightGiven

The box scrolls the body's height under ScrollBars Both, Vertical and Auto. A walk up from inside
the body then ends at the body, so nothing in it takes the viewport height as a bound - see
Control::isHeightGivenFromOutside. A wrapping stack of columns in it breaks only at its own
maximum, and the box scrolls it.

## Control::isHeightGivenFromOutside

**The walk climbs while each control fills downwards and its parent hands it its height.** It
answers yes only when it reaches the root of a form that lays its content into its window -
AutoFit::No. Anywhere else the height is the content's own answer.

A parent reads the height off a child, rather than handing one down, in three places:

- a column stack, for an item that does not fill the lane - see Stack::isChildHeightGiven;
- a panel, for its top and bottom bars - the side bars and the body get the slot's height;
- a ScrollBox, for a body it scrolls vertically.

**isWidthGivenFromOutside is the same walk across**, with one more stop: a control whose width a
PreferredSize states ends the walk as given - a bar the pointer sized. A fit states none for the
pass that measures the bar's content.
