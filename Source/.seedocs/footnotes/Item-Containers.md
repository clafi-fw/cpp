# Item Containers

The words that no longer fit above a declaration in `ContainerBase.cppm`, `StackBase.cppm`,
`Stack.cppm` and `TabStrip.cppm`. Referenced from those files.

## ContainerBase

A control that holds other controls. It also hosts overlay entries: `addOverlayControl` affects
painting order and clipping alone. Overlay controls are painted in a second pass on top of the
other controls and are not clipped by their own bounds, though they are still clipped by this
container's bounds. An overlay control must belong to this container's control tree, and
otherwise `addOverlayControl` has no effect.

Which container holds the entry is therefore a choice about how far the second pass may reach,
and it need not be the control's own parent: a grid hands its header's entry to the control the
grid stands in, so that what the header draws around itself lands on the page instead of
stopping at the grid's edge. What hosting costs is that this container's children are walked
twice for every paint.

## StackBase

A base for the Stack and PageControl classes.

It owns the current item - the single item a container tracks. A PageControl shows its visible
page through it and a TabStrip its open tab, and neither of them can hold more than one.

Holding several items selected at once is a different idea, and it lives in StackView.

## CanFocusItemEvent

`CanFocusItemEvent` is asked of every candidate, and is answerable by a handler that knows
something the container does not.

Whether an item may join a SELECTION is a different question, asked separately and one level up
- see `CanSelectItemEvent` in StackView. Neither answer implies the other: a view sweeping a
rubber band asks only about selection, and a container with no selection at all still asks this
one.

## Stack

A container that places its items in lanes. `Orientation` says which way a lane runs and whether
the stack wraps into further lanes; `ItemSizing` says how a lane divides itself between the items
in it; `LaneSize` caps how many items one lane takes before the next begins.

### A stated lane is a break, not a hint

`LaneSize` breaks the lane wherever it is read, and BOTH passes read it. The measure counts the
items into lanes - `Control::calculateLanes`, which closes a lane on the count or on the maximum
the panel states - and the align closes the same lane on the same count, in `nextLane`. A stack
stated four to a lane stands four to a lane in a window of any size, and what a larger host gives
it is room around the lane, not a fifth item in it.

Hidden items are passed over by both, so a lane holds the number it was measured to hold whatever
has been hidden since. They are taken into the lane already open rather than left to start the next
one, which would otherwise begin on something that is never placed.

### Filling the lane, and filling across it

A WRAPPING STACK SHARES WHAT IT WAS GIVEN, both ways round.

Across the lanes: what is left once every lane stands at its own thickness goes to the lanes in
equal parts, so a list dropped into a window wider than its columns fills it - and a stack of rows
told to fill a taller box spreads its rows down it the same way.

Along the lane: what is left once every item stands at its own size goes to the items in equal
parts.

THE SURPLUS IS ONE NUMBER FOR THE WHOLE STACK, AND IT IS DIVIDED BY THE ITEM: what the length has
over the LONGEST lane, over the count of items on that lane. Every item in the stack then grows by
the same amount, the fullest lane comes out exactly at the length, and a short last lane simply
stays short.

ONE GATE: THE STACK'S OWN ALIGNMENT, read along the lane - `fillsMain`.

A lane holding something that ASKED for the surplus takes it either way, gate or no gate, which is
what a single lane already does: `alignContent` reaches for the granted length on `hasFillingItem`
alone. On a wrapping stack the question is asked per lane, off the count `nextLane` already made,
because walking a million items to answer it once would cost the pass more than the layout.

A lane holding something that ASKED for what the lane has over takes it whole - see
`Control::fillsLane` and `FlexSpacer`. Something that asked already says where the surplus goes, so
the equal parts are what happens where nothing asked.

What an item does with the part it is handed is the item's own to say: one that fills takes it, one
aligned to an edge keeps the size it measured and stands in it.

### Lanes that line up: ItemSizing::Equal

Shares divided among the items on one lane are as many as that lane holds, so a short last lane
makes larger parts than the lanes above it and the tracks go ragged. `ItemSizing::Equal` divides by
the LANE instead: every lane is cut into the same number of equal shares - four shares whether it
holds four items or two - so the short last lane leaves its tracks standing rather than spreading
two items over the room for four.

The share is a box, not a size. What stands in it is placed by its own alignment, the way
`Control::align` places anything handed more room than it kept: an item that fills takes the share
whole, one aligned to an edge keeps the size it measured and stands in it.

FILLING IS WHAT MAKES THE SHARES SHOW. An item that keeps its own size is only as wide as its own
content, so a set of them comes out ragged inside a set of equal shares - a `ThemeTile` told to
Center came out as wide as its NAME, and the picture inside it, drawn at a stated size and centred,
was then clipped to a tile narrower than itself: a theme called "1" showed a third of its picture
and one called "123456789" showed all of it. Filling the share is what makes every tile the same
width, and a name too long for it wraps under the picture instead of widening the tile.

Nothing about the MEASURE changes - the panel still asks for its natural lane length, and what it
asks for is what a host measured from its content still gives it.

AN EVEN LANE IS CUT ON ITS SHARES, NOT ON THE ITEMS. Every share is as long as the largest item,
so a lane holds as many of those as the length has room for, capped by the lane's own count -
`sharesThatFit` - and every lane then holds the same number. Cutting on the items instead leaves
the lane of long names holding two while the one above it holds three, at shares that are all one
size, so the third slot stands empty for no reason.

### A wrapping panel and the length it is given

So a wrapping panel carries the length over: `alignIntoLanes` remembers the length it broke the
lanes at - the content width for rows, the content height for columns - and the next measure breaks
them at that same length - `wrapLengthLimit`, handed to `Control::calculateRows` or
`Control::calculateColumns` as a maximum. Where that length has CHANGED, the panel asks for one more
pass through `AlignEvent::invalidatePass` and the pass after it hands back the same number, which is
what ends it.

THE BOX ABOVE CARRIES ITS OWN. `ScrollBox` states the viewport as a wrapping body's maximum, and it
writes that width in `PanelBase::bodySlotSettled` - the moment the slot is settled and before the
body is laid into it - so a body that FILLS breaks its lanes at this pass's viewport. A body that
does not fill keeps the width it MEASURED, and the measure ran earlier in the pass, against the
viewport of the one before, so for that one the box asks for another pass and the pass after hands
back the same width.

BOTH AXES CARRY IT, in one member read along the lane. The length is the extent the measure runs
against - `Control::calculateRows` and `Control::calculateColumns` each take a maximum - and
whether it may be kept is asked the same way round: `isWidthGivenFromOutside` for a stack of rows,
`isHeightGivenFromOutside` for a stack of columns, which `isLaneLengthGivenFromOutside` picks
between. A change of orientation drops what was kept, since it was measured along the other axis.

Three things the remembered length is not allowed to do, and each is a guard:

- It is stored in DESIGN UNITS, at the scale that laid it out. Converting it at whatever scale
  reads it next latches a form taken to 250% and back at a fraction of the length it has.
- It is kept only where the length is given from outside, which asks the question ALL THE WAY UP
  and asks it of this control first. **A control that does not FILL keeps what it measured**,
  however freely its host hands a width down - align gives the surplus back - so its width is
  its own content's answer and a bound taken from it eats itself: the rows wrap, the panel comes
  out narrower than the width it wrapped at, the next pass wraps THAT, and a stack aligned Left
  walks from five items a row down to two. **An item of a row keeps what it measured too**, Fill or
  not: a row sizes its items from what they measured, and hands a width out only to an item that
  takes what the lane has over - `Stack::isChildWidthGiven`. Without that a crumb held its
  own width as a ceiling: a longer title broke onto more lines, and a scale round trip left its
  icon and strip, rounded per scale, a pixel short of the text. Above this control the same
  question is asked of every host in turn, and one level is never enough: the backstage hands a
  width down through five of them and is itself measured from the pages it holds. The walk ends at
  the first host that is as wide as what it contains, and at the root it asks the form: a window
  asked for out of the content is the content's answer once more, `AutoFit::Yes`.

  The height walk is the same walk down the other axis. A control that does not fill downwards
  keeps the height it measured. A column sizes its items from what they measured and hands a
  height out only to an item that takes what the lane has over - `Stack::isChildHeightGiven`.
  A panel's top and bottom bars are as tall as they measured, while its side bars and its body are
  handed the slot's height - `PanelBase::isChildHeightGiven`. A scroll box gives its body no height
  on an axis it scrolls - `ScrollBox::isChildHeightGiven`, and see the next section.
- It states nothing while the form is measuring to ASK for a window -
  `FormBase::isMeasuringPlacement`. That pass is where a form finds out what it wants, and one
  held to the size it was last given could never grow.

THE SAME THREE GOVERN THE VIEWPORT A SCROLL BOX STATES FOR ITS BODY, which is a remembered width of
the same kind - see ScrollBox::adjustChildMetrics, and Controls-Base for where it is written.

A first open therefore shows the lane count's answer, then the length's: the placement is asked
for off the first, and the pass that lays the content into the window is where the length
arrives. A page that converges SHORTER keeps the window it already asked for.

So a panel in a window fills the length it is given, and a panel in something sized by what it
holds stands at its lane count. Which is the right answer to each: there is no length to fill in
a host that has none of its own yet. A stack of columns in a side bar is the first kind: the
window decides the bar's height, the columns break at it, and the bar is as wide as the columns
that makes, with the body beside it giving up the room.

### A column on a box that scrolls

A COLUMN IS NOT BROKEN AT A HEIGHT ITS HOST SCROLLS. There `calculateColumns` breaks a column at the
stack's own maximum and nowhere else, and a scroll box hands its body the viewport whatever the body
measured.

So where `Control::isScrolledByParent` answers yes for the vertical axis, the column is granted
at least the height it measured, and the lanes the align cuts are the lanes the measure counted.
A `ScrollBox` answers from its bars: `Both`, `Vertical` and `Auto` scroll the body's height,
which is the same reading `ScrollBox::stateSizeGivenWay` makes of what the body can give up.

NOTHING IS REMEMBERED THERE EITHER. `ScrollBox::isChildHeightGiven` answers no for a body whose
height the box scrolls, so `isHeightGivenFromOutside` stops at the box and the measure has no
length to break the column at but the stack's own maximum. A remembered viewport would turn a list
that scrolls into one that flows sideways the moment it outgrew the height it was last laid out at.

A box that scrolls only across - a list of columns flowing sideways, `ScrollBars::Horizontal` -
still hands the viewport as the height, and the columns break at it. Rows take none of this: the
width is what a row stack is broken at by design, and the measure already runs against it.

## PreviewMode

Which item a stack raises its preview for - see `PreviewEvent`.

`Focus` is the item the container has settled on. What asks for a preview is the current item
moving, whichever gesture moved it, and a pointer merely crossing the stack asks for nothing. It
is the default, because previewing is a strong thing for a container to do and a pointer passing
over a list did not ask for it. A stack that wants the other reading says so.

`Hover` is the item under the pointer, and the current item whenever it moves - a key moving it
previews what it stops on, the same way Focus does. Leaving the stack returns the preview to the
current item, so what is previewed is always something the stack holds.

## PreviewEvent

The item a stack has settled on, which is not the item the user has picked. Nothing has been
chosen: this is for whatever a container shows about the item being looked at before the user
commits to anything - the theme browser wearing the theme of the tile in question is the case it
was built for. `PreviewMode` says which item that is.

Raised a moment after the item settles rather than for every item passed on the way, so a held
key and a pointer sweeping the stack each answer once, for the item they came to rest on. See
`Stack::startPreviewTimer`.

`item` is never null: a stack with nothing to settle on previews nothing at all.

## LaneSizing

How a lane's count is read. A CEILING IS SPREAD, A COUNT IS NOT.

`Exact` - every lane takes the count, and the remainder stands in a short last lane.

`UpTo` - the count is a ceiling: the items go into the fewest lanes it allows, and every lane but
the last is filled to the same depth, so eleven under a ceiling of ten stand as six and five
rather than ten and one. A hidden item is not one of the items counted.

## ItemSizing

How a lane sizes the items in it. `Equal` reads on every orientation. On a wrapping
one the divisor is how many shares of the largest item the length has room for - see above - and a
stated `LaneSize` caps that count rather than supplying it.

`Natural` - every item is the size it calculated for itself. A row is as wide as its items and
the gaps between them.

`Equal` - the lane divided evenly: every item is handed the same share of it, whatever it
calculated. A row of answers reads as one band this way rather than as buttons of several widths.
The lane is then AS LONG AS THE LARGEST ITEM TIMES THE COUNT, because a share has to hold every
item that could stand in it - taking the natural total instead would divide a row that fits Save,
Discard and Cancel into three widths none of which fits Discard.

## PageSizing

Which of its pages a page control answers for its own size with.

`CurrentPage` - the page that shows, and nothing about the ones that do not. The pages nobody is
looking at are never measured.

`WidestPage` - the largest of them all, in both extents, so turning to another page does not
resize what holds the page control: the application menu is as big as the largest page it can
show. The hidden pages are measured once, and again when a page is added or taken away or the
form's scale moves. A page that grows on its own while hidden, at the same scale, is not
measured again.

## TabStrip padding and spacing

Along the strip, the padding is the room the tab line runs on past the first and the last tab, and
the layout lays it out at both ends.

Across a `TabViewMode::Tab` strip, the padding stands on the outer side alone - the side away from
the page. The open tab's line runs along the strip's inner edge, and the page starts there, so a
padding on that side would lift the line off the page.

`TabStripBase::adjustMetrics` hands the layout no padding on that axis, and the strip lays the
stated value out itself. `calculateContent` adds it to the cross extent and to the calculated
minimum, the way `Control::calculate` adds a padding. `alignContent` places the tabs past it: below
it on `HorizontalTop`, right of it on `VerticalLeft`, and on the other two the room is left at the
far side. Everything that reads the layout's padding - the paint, the hit test, the line's origin,
`TabbedBox::tabLineX` - finds the inner edge.

So the two answers differ on that axis: `padding()` is the stated value, `scaledPadding()` the one
the layout sees.

Across a `TabViewMode::Tab` strip, the spacing is the gap an item that is not a tab keeps from the
line - a button or a check box standing among the tabs. A single lane has no use for the spacing
across it, so it is free for this. `adjustChildBox` cuts the gap off such an item's box on the
line side, and `calculateContent` counts it on top of the widest of them. Tabs reach the line
whatever the spacing is.

A `TabViewMode::ToolButton` strip joins nothing. The layout lays its padding out on every side,
and no item keeps a gap from a line.

A `TabbedBox` builds its strip with a padding and a spacing of its own for each mode.
`StripPadding` and `StripSpacing` stand in place of them, put on the strip by the box's
constructor body: the strip is a reference member, built by its initializer before any prop
can be bound, so the stated values go on after it stands rather than into its making.
