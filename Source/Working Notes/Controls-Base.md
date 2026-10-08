# Controls Base

The author's side of the footnote of the same name.

## ButtonBase

### A picture standing over the words sets the width

Asked against the box, the caption comes out on ONE LINE and the button is as wide as its name: a
button wider than the thing it is built around, and no two of them the same width. What that costs
shows up a level away, in a wrapping stack with `ItemSizing::Equal` - see Item-Containers - where
every share is as long as the largest item, so the single longest caption in the stack sizes every
share and can drop a whole column out of the grid. A row of theme tiles fell from four to two that
way, on one theme called "New Theme (10)".

### What stands beside the words comes off their width

Measured at the whole box, a caption one word longer than its room comes out a line short, and the
paint collapses the last line onto the one above.

## PanelBase

### bodySlotSettled

`ScrollBox` is the reason it exists. It states the viewport as a wrapping body's maximum, and it
used to write that width after `PanelBase::alignContent` returned - after the body had been laid
out against the PREVIOUS pass's viewport. Written here instead, a body that FILLS is right on the
pass that resized it, because the align hands it the slot and the slot is what it breaks its lanes
at.

It does not remove the box's extra pass, and cannot. A body that does not fill keeps the width it
MEASURED, and the measure ran before this hook, against the viewport of the pass before - the
viewport being this box's width less its bars, which this box does not have until its own parent
lays it out. So the width reaches a filling body at once and a non-filling one on the pass after.
Maximizing with the body aligned Left was where that showed: rows still broken at the width the
small window had, with the rest of the screen empty beside them.
