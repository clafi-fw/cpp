# Controls Base

The author's side of the footnote of the same name.

## ButtonBase

### A picture standing over the words sets the width

Asked against the box, the caption comes out on **one line** and the button is as wide as its name:
a button wider than the thing it is built around, and no two of them the same width. What that costs
shows up a level away, in a wrapping stack with `ItemSizing::Equal` - see Item-Containers - where
every share is as long as the largest item, so the single longest caption in the stack sizes every
share and can drop a whole column out of the grid. A row of theme tiles fell from four to two that
way, on one theme called "New Theme (10)".

### What stands beside the words comes off their width

Measured at the whole box, a caption one word longer than its room comes out a line short, and the
paint collapses the last line onto the one above.

## PanelBase bodySlotSettled

`ScrollBox` is the reason it exists. It states the viewport as a wrapping body's maximum, and it
used to write that width after `PanelBase::alignContent` returned - after the body had been laid
out against the **previous** pass's viewport. Written here instead, a body that **fills** is right
on the pass that resized it, because the align hands it the slot and the slot is what it breaks its
lanes at.

It does not remove the box's extra pass, and cannot. A body that does not fill keeps the width it
**measured**, and the measure ran before this hook, against the viewport of the pass before - the
viewport being this box's width less its bars, which this box does not have until its own parent
lays it out. So the width reaches a filling body at once and a non-filling one on the pass after.
Maximizing with the body aligned Left was where that showed: rows still broken at the width the
small window had, with the rest of the screen empty beside them.

## PanelBase createSplitter

The split is the panel's and never the bar's. A drag writing the bar's `MinSize` or
`PreferredSize` would overwrite what the caller built it with, and nothing could say afterwards
which of the two values was true. So the dragged extent lives in `m_barSizing`, in design units so
a scale change keeps the proportion, and `adjustChildMetrics` hands it to the pass's copy of the
bar's metrics as the preference. `Control::calculate` clamps it between the bar's own minimum and
maximum, which is what makes a `MaxSize` on the bar a hard stop for the drag without the splitter
knowing of it.

`barExtent` answers what the bar **has**, read off its laid-out box, not what was stated: a stated
value past the bar's `MaxSize` would otherwise be the start of the next drag, and the pointer would
have to move through the part nobody can see. The drag's ceiling is the bar's extent plus the room
the body has over its floor, read off the laid-out rects: the sum of the two stays put while the
drag moves the line between them, so the ceiling holds through the drag.

The splitter is a direct child registered as the panel's own overlay. The hit test walks a panel's
children in collection order and stops at the first that contains the point, and the band has to
win over the bar and the body it lies on - overlays are met before the children and painted after
them. `navigationSlots` lists the slots only, so the splitter is never a stop.

The gap rule is there so that a splitter added to an existing panel moves nothing: a panel with
spacing already has a gap between the bar and the body, and the line goes into it. Only a panel
with no spacing grows, by the line.

`wnd_doubleClick` hands the second press on to `wnd_mouseDown` only where nothing stopped the
event, so stopping it in `nestedDoubleClick` is also what keeps that press from becoming the start
of a drag over the bar it has just fitted.

A fit is a snapshot, not a mode. Oj asked for "resize to fit content", and a bar that went on
following its content would move every time a tree node opened. The measurement is not kept
anywhere - `Control::calculate` throws it away once the preference stands in its place - so
`fitBarToContent` only marks the bar (`fitAsked`) and asks for a pass; `adjustChildMetrics` hands
that pass a zero preference, and `keepFittedExtents`, first thing in `calculateContent` with the
children measured, reads the bar's box, keeps it as the extent and raises `OnBarResize`. Raised
inside the measuring pass, which the framework does already for `GetTextEvent`; a handler writes
the number down and asks for nothing. The drag raises the same event from `dragBarTo` with the
value it stated, because `barExtent` would still read the box of the pass before.

SeeDocs keeps the width per tab, in the tab's own file beside the open nodes (`TreeWidth`, zero
for a bar never resized): `showPage` states it or clears it as the tab's tree comes up, and the
`OnBarResize` handler writes it under the shown page's tab.

## MessageBoxBase

The three bars are full bleed because a strip inset further than the window's border reads as a
card floating on the window rather than as a band of it.
