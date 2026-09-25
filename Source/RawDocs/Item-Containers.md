# Item Containers

The words that no longer fit above a declaration in `ContainerBase.cppm`, `StackPanelBase.cppm`
and `StackPanel.cppm`. Referenced from those files.

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

## StackPanelBase

A base for the StackPanel and PageControl classes.

It owns the current item - the single item a container tracks. A PageControl shows its visible
page through it and a TabStrip its open tab, and neither of them can hold more than one.

PageControl does not need a stack alignment - that part lives in StackPanel - but it does need
the current item, so that part is here.

Holding several items selected at once is a different idea, and it lives in StackView.

## CanFocusItemEvent

`CanFocusItemEvent` is asked of every candidate, and is answerable by a handler that knows
something the container does not.

Whether an item may join a SELECTION is a different question, asked separately and one level up
- see `CanSelectItemEvent` in StackView. Neither answer implies the other: a view sweeping a
rubber band asks only about selection, and a container with no selection at all still asks this
one.

## StackPanel

A container that places its items in lanes. `Orientation` says which way a lane runs and whether
the stack wraps into further lanes; `ItemSizing` says how a lane divides itself between the items
in it; `LaneSize` caps how many items one lane takes before the next begins.

### One routine, read through main and cross

A wrapping stack does the same four things whichever way its lanes run: it cuts the lane on its
count or on the length it was given, it shares what the stack has over that length among the items
on each lane, it shares what the stack has over the thickness among the lanes, and it divides an
Equal lane by the count the lane states. `alignIntoLanes` does all four once, reading every extent
through `mainOf` and `crossOf` - MAIN IS THE WAY A LANE RUNS, CROSS THE WAY THE LANES STACK - the
way `Control::calculateLanes` already reads the measure side.

It was written twice before, and that is the whole history of this file: `alignIntoRows` was a
hand-written half of `alignIntoColumns`, and every defect found in a month was one path missing
what the other one had. Rows broke on the lane count and columns did not; columns shared the slack
between lanes and rows did not; rows skipped hidden items and columns did not; neither shared what
the stack had over ALONG the lane, so a wrapping stack told to fill the way its items run filled
nothing. Two routines that are supposed to be mirror images are two routines that will drift, so
there is one.

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
parts. A stack of rows told to fill a wider box was laid out at the width it measured and the
remainder stood at its right, so it accepted the width and left a strip of it empty; a stack of
columns told to fill a taller box did the same downwards.

THE SURPLUS IS ONE NUMBER FOR THE WHOLE STACK, AND IT IS DIVIDED BY THE ITEM: what the length has
over the LONGEST lane, over the count of items on that lane. Every item in the stack then grows by
the same amount, the fullest lane comes out exactly at the length, and a short last lane simply
stays short.

Neither of the two easier readings survives a short last lane. Normalising each lane to the length
- handing it the difference between itself and the limit - stretches a last row of two across the
room for five. Handing every lane the same TOTAL is not much better: two items sharing what four
shared above them come out half again as wide as the tracks they stand under. The per-item share
is the one that leaves the tracks alone. A lane still never takes more room than it has, whatever
the share comes to: one long item can make a lane that is already at the length.

NOT WHAT THE STACK HAS OVER WHAT IT MEASURED, which is the same number on the row axis and nonsense
on the column axis. `Control::calculateRows` measures against a maximum - the remembered width - so
`dimensions()` there really is the longest row; `calculateColumns` takes no maximum, so a wrapping
stack of columns measures ONE lane holding everything, a million items tall, and its difference
with what it was granted says nothing about the layout being placed. Reading the longest lane, which
this pass has just cut, is the same answer for rows and the only right one for columns.

ONE GATE: THE STACK'S OWN ALIGNMENT, read along the lane - `fillsMain`. It is tempting to let the
granted length answer for itself, since `Control::align` normally hands the surplus back to a stack
that was not told to fill. It only does so where the stack's measure is SHORTER than the slot it is
given. A wrapping stack on a scroll box is longer - it measured one lane holding everything - so it
is handed the viewport whatever its alignment says, and the length carries no answer. Every
VerticalAlign looked like Fill for exactly as long as this was read off the length.

A lane holding something that ASKED for the surplus takes it either way, gate or no gate, which is
what a single lane already does: `alignContent` reaches for the granted length on `hasFillingItem`
alone. On a wrapping stack the question is asked per lane, off the count `nextLane` already made,
because walking a million items to answer it once would cost the pass more than the layout.

Under a pixel is nothing over: the measure ceils what it answers and the align does not, so
`surplusOver` reads the difference as zero rather than moving every item by a fraction on a pass
that changed nothing.

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

WHAT A WRAPPING STACK LACKED WAS A DIVISOR KNOWN BEFORE THE WRAP: a lane's own count is settled by
the sizes the items came to, and dividing by that is dividing by the answer. `sharesThatFit` is
that divisor without anyone stating it - how many shares of the largest item the length has room
for, both of them known before a lane is cut - so Equal reads on a wrapping stack whether or not a
`LaneSize` is stated, and a stated lane is a cap on the count rather than the only way to have
one.

AN EVEN LANE IS CUT ON ITS SHARES, NOT ON THE ITEMS. Every share is as long as the largest item,
so a lane holds as many of those as the length has room for, capped by the lane's own count -
`sharesThatFit` - and every lane then holds the same number. Cutting on the items instead leaves
the lane of long names holding two while the one above it holds three, at shares that are all one
size, so the third slot stands empty for no reason.

THE ALIGN PASS ASKS THAT QUESTION AND THE MEASURE DOES NOT. The measure has no length of its own to
ask it against: the only one it could use is the width the last pass laid out - see the remembered
width below - and a count taken from that is a count derived from its own last answer. A narrow
pass shrinks what the panel asks for, a host measured from the panel grants that, and the lane
walks down one item per pass and never comes back. The largest item, on the other hand, is safe to
read during the align: `calculateChildren` re-measures every child before the parent measures
itself, and `Control::calculate` always writes what it came to, so a width read there is this
pass's measure and not the size the last align handed out.

Dividing by the LANE'S stated count instead, on a lane the length cut short, makes shares narrower
than the items measured, and an item overrunning its share is clipped rather than wrapped - a width
its text was never broken at is not a width its text fits. A theme tile called "New Theme" lost its
last letter that way while the tile beside it, called "1", left half its share empty.

### A wrapping panel and the width it is given

WITHOUT A LANE COUNT the two passes have nothing in common. The measure runs bottom-up against
the maximum the panel states; the width the panel will be handed is settled top-down, in the
align pass that follows. A panel stretched by its host therefore wraps at a width the measure
never saw, and the rows it does not use stand under the items as dead space with the same dead
travel on the bar beside them.

So a wrapping panel carries the width over: `alignIntoLanes` remembers the content width it broke
the rows at, and the next measure breaks them at that same width - `wrapWidthLimit`. Where that
width has CHANGED, the panel asks for one more pass through `AlignEvent::invalidatePass` and
the pass after it hands back the same number, which is what ends it. Asking on anything else
cannot end: comparing the height the rows came to against the height they measured looks like the
sharper test and is a program that lays itself out for ever, because `Control::calculate` ceils
the content it measured and the align pass does not - a fraction of a unit that never agrees.
Rows are monotone in width besides - a narrower panel never wraps into fewer of them - so the one
loop that could feed back, a bar appearing and narrowing the viewport, settles rather than rings.

THE BOX ABOVE CARRIES ITS OWN. `ScrollBox` states the viewport as a wrapping body's maximum, and
it writes that width in `PanelBase::bodySlotSettled` - the moment the slot is settled and before
the body is laid into it - so a body that FILLS breaks its lanes at this pass's viewport. A body
that does not fill keeps the width it MEASURED, and the measure ran earlier in the pass, against
the viewport of the one before, so for that one the box asks for another pass and the pass after
hands back the same width. A window maximized with the body aligned Left is where the miss showed
- rows still broken at the width the small window had, with the rest of the screen empty beside
them - while the same window with the body aligned Fill re-wrapped at once, because the align
overrides the measure there and the panel never reads the stale answer.

The panel cannot ask in the box's place: the stale viewport reaches it as its own maximum, so the
width it remembers and the width it is handed agree, and it sees nothing to ask about.

THE ROWS ALONE CARRY THIS, and it is the one thing that is not mirrored. The width is the extent
the measure runs against - `Control::calculateRows` takes a maximum and `calculateColumns` does not
- so a stack of columns has no answer to carry back and nothing to ask another pass about.

Three things the remembered width is not allowed to do, and each is a guard:

- It is stored in DESIGN UNITS, at the scale that laid it out. Converting it at whatever scale
  reads it next latches a form taken to 250% and back at a fraction of the width it has.
- It is kept only where `isWidthGivenFromOutside()`, which asks the question ALL THE WAY UP and
  asks it of this control first. **A control that does not FILL keeps what it measured**, however
  freely its host hands a width down - align gives the surplus back - so its width is its own
  content's answer and a bound taken from it eats itself: the rows wrap, the panel comes out
  narrower than the width it wrapped at, the next pass wraps THAT, and a stack aligned Left walks
  from five items a row down to two. Above this control the same question is asked of every host
  in turn, and one level is never enough: the backstage hands a width down through five of them
  and is itself measured from the pages it holds. The walk ends at the first host that is as wide
  as what it contains, and at the root it asks the form: a window asked for out of the content is
  the content's answer once more, `AutoFit::Yes`.
- It states nothing while the form is measuring to ASK for a window -
  `FormBase::isMeasuringPlacement`. That pass is where a form finds out what it wants, and one
  held to the width it was last given could never grow.

THE SAME THREE GOVERN THE VIEWPORT A SCROLLBOX STATES FOR ITS BODY, which is a remembered width
of the same kind - see ScrollBox::adjustChildMetrics, and Controls-Base for where it is written.
The placement guard is needed on the READ as well as the write: what a form does when it is handed
a scale is lay itself out once inside the window it still has, at the new factor, and the body
squeezed by that pass is what the writer records. It is an ordinary align, so the writer's guard
lets it through, and a menu taken from 165% to 227% arrives at its placement holding a viewport of
104 design units where its own page needs 374. The measure then collapses onto whatever MinSize
the root states and the window comes out at that floor.

A first open therefore shows the lane count's answer, then the width's: the placement is asked
for off the first, and the pass that lays the content into the window is where the width arrives.
A page that converges SHORTER keeps the window it already asked for.

So a panel in a window fills the width it is given, and a panel in something sized by what it
holds stands at its lane count. Which is the right answer to each: there is no width to fill in
a host that has no width of its own yet.

### A column on a box that scrolls

A COLUMN IS NOT BROKEN AT A HEIGHT ITS HOST SCROLLS. `calculateColumns` breaks a column at the
stack's own maximum and nowhere else, and a scroll box hands its body the viewport whatever the
body measured. Cut at the viewport, the columns are ones the measure never counted. A dropdown
cut short by its placement would show it: `ScrollBars::Auto` puts the bar up for the one column
the list measured, the cut makes two, the body comes out exactly as tall as the viewport so the
bar has nothing to carry, and the second column stands past the width the window was measured
for, clipped at the slot.

So where `Control::isScrolledByParent` answers yes for the vertical axis, the column is granted
at least the height it measured, and the lanes the align cuts are the lanes the measure counted.
A `ScrollBox` answers from its bars: `Both`, `Vertical` and `Auto` scroll the body's height,
which is the same reading `ScrollBox::stateSizeGivenWay` makes of what the body can give up.

A box that scrolls only across - a list of columns flowing sideways, `ScrollBars::Horizontal` -
still hands the viewport as the height, and the columns break at it. Rows take none of this: the
width is what a row stack is broken at by design, and the measure already runs against it.

## PreviewMode

Which item a stack raises its preview for - see `PreviewEvent`.

`Focus` is the item the container has settled on. What asks for a preview is the current item
moving, whichever gesture moved it, and a pointer merely crossing the stack asks for nothing. It
is the default, because previewing is a strong thing for a container to do and a pointer passing
over a list did not ask for it. A stack that wants the other reading says so.

`Hover` is the item under the pointer. Leaving the stack returns the preview to the current item,
so what is previewed is always something the stack holds.

## PreviewEvent

The item a stack has settled on, which is not the item the user has picked. Nothing has been
chosen: this is for whatever a container shows about the item being looked at before the user
commits to anything - the theme browser wearing the theme of the tile in question is the case it
was built for. `PreviewMode` says which item that is.

Raised a moment after the item settles rather than for every item passed on the way, so a held
key and a pointer sweeping the stack each answer once, for the item they came to rest on. See
`StackPanel::startPreviewTimer`.

`item` is never null: a stack with nothing to settle on previews nothing at all.

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
