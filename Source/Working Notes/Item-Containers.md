# Item Containers

The author's side of the footnote of the same name.

## StackBase

PageControl does not need a stack alignment - that part lives in Stack - but it does need
the current item, so that part is here.

## Stack

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

### Filling the lane, and filling across it

A stack of rows told to fill a wider box was laid out at the width it measured and the remainder
stood at its right, so it accepted the width and left a strip of it empty; a stack of columns told
to fill a taller box did the same downwards.

Neither of the two easier readings survives a short last lane. Normalising each lane to the length
- handing it the difference between itself and the limit - stretches a last row of two across the
room for five. Handing every lane the same TOTAL is not much better: two items sharing what four
shared above them come out half again as wide as the tracks they stand under. The per-item share
is the one that leaves the tracks alone. A lane still never takes more room than it has, whatever
the share comes to: one long item can make a lane that is already at the length.

NOT WHAT THE STACK HAS OVER WHAT IT MEASURED, which is a number only where the measure ran against
the length the align cuts at. Where it did not - a stack of columns on a box that scrolls, measured
against no maximum, or any stack on the pass before it has a length to remember - it measured ONE
lane holding everything, a million items tall, and its difference with what it was granted says
nothing about the layout being placed. Reading the longest lane, which this pass has just cut, is
the right answer in every case.

It is tempting to let the granted length answer for itself, since `Control::align` normally hands
the surplus back to a stack that was not told to fill. It only does so where the stack's measure is
SHORTER than the slot it is given. A wrapping stack on a scroll box is longer - it measured one lane
holding everything - so it is handed the viewport whatever its alignment says, and the length
carries no answer. Every VerticalAlign looked like Fill for exactly as long as this was read off the
length.

Under a pixel is nothing over: the measure ceils what it answers and the align does not, so
`surplusOver` reads the difference as zero rather than moving every item by a fraction on a pass
that changed nothing.

### Lanes that line up: ItemSizing::Equal

WHAT A WRAPPING STACK LACKED WAS A DIVISOR KNOWN BEFORE THE WRAP: a lane's own count is settled by
the sizes the items came to, and dividing by that is dividing by the answer. `sharesThatFit` is
that divisor without anyone stating it - how many shares of the largest item the length has room
for, both of them known before a lane is cut - so Equal reads on a wrapping stack whether or not a
`LaneSize` is stated, and a stated lane is a cap on the count rather than the only way to have
one.

THE ALIGN PASS ASKS THAT QUESTION AND THE MEASURE DOES NOT. The measure has no length of its own to
ask it against: the only one it could use is the length the last pass laid out - see the remembered
length below - and a count taken from that is a count derived from its own last answer. A narrow
pass shrinks what the panel asks for, a host measured from the panel grants that, and the lane
walks down one item per pass and never comes back. The largest item, on the other hand, is safe to
read during the align: `calculateChildren` re-measures every child before the parent measures
itself, and `Control::calculate` always writes what it came to, so a width read there is this
pass's measure and not the size the last align handed out.

Dividing by the LANE'S stated count instead, on a lane the length cut short, makes shares narrower
than the items measured, and an item overrunning its share is clipped rather than wrapped - a width
its text was never broken at is not a width its text fits. A theme tile called "New Theme" lost its
last letter that way while the tile beside it, called "1", left half its share empty.

### A wrapping panel and the length it is given

WITHOUT A LANE COUNT the two passes have nothing in common. The measure runs bottom-up against
the maximum the panel states; the length the panel will be handed is settled top-down, in the
align pass that follows. A panel stretched by its host therefore wraps at a length the measure
never saw. Where the measure counted more lanes than the align cuts, the ones it does not use
stand beside the items as dead space, with the same dead travel on the bar beside them. Where it
counted fewer, the lanes past the counted ones stand outside the box the host measured for the
panel: a stack of columns in a side bar is as wide as the one column it measured and as tall as
the slot, so the column breaks at the slot and the next one stands past the bar's edge, clipped.

Asking on anything else cannot end: comparing the extent the lanes came to across against the extent
they measured looks like the sharper test and is a program that lays itself out for ever, because
`Control::calculate` ceils the content it measured and the align pass does not - a fraction of a
unit that never agrees. Lanes are monotone in length besides - a shorter length never wraps into
fewer of them - so the one loop that could feed back, a bar appearing and narrowing the viewport,
settles rather than rings. A side bar has no such loop: its height is the slot's, whatever width its
columns take.

A window maximized with the body aligned Left is where the miss showed - rows still broken at the
width the small window had, with the rest of the screen empty beside them - while the same window
with the body aligned Fill re-wrapped at once, because the align overrides the measure there and the
panel never reads the stale answer.

The panel cannot ask in the box's place: the stale viewport reaches it as its own maximum, so the
width it remembers and the width it is handed agree, and it sees nothing to ask about.

The placement guard is needed on the READ as well as the write: what a form does when it is handed a
scale is lay itself out once inside the window it still has, at the new factor, and the body
squeezed by that pass is what the writer records. It is an ordinary align, so the writer's guard
lets it through, and a menu taken from 165% to 227% arrives at its placement holding a viewport of
104 design units where its own page needs 374. The measure then collapses onto whatever MinSize the
root states and the window comes out at that floor.

### A column on a box that scrolls

Cut at the viewport, the columns are ones the measure never counted. A dropdown cut short by its
placement would show it: `ScrollBars::Auto` puts the bar up for the one column the list measured,
the cut makes two, the body comes out exactly as tall as the viewport so the bar has nothing to
carry, and the second column stands past the width the window was measured for, clipped at the slot.
