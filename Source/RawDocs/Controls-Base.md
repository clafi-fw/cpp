# Controls Base

The words that no longer fit above a declaration.

## ButtonBase

### A picture standing over the words sets the width

`TopLeftIcon`, `TopCenterIcon` and `BottomIcon` put the picture and the caption on separate lines,
so the button's natural width is the picture's and the caption wraps under it - `measureText` asks
for the text at the icon's width rather than at the whole box.

Asked against the box, the caption comes out on ONE LINE and the button is as wide as its name: a
button wider than the thing it is built around, and no two of them the same width. What that costs
shows up a level away, in a wrapping stack with `ItemSizing::Equal` - see Item-Containers - where
every share is as long as the largest item, so the single longest caption in the stack sizes every
share and can drop a whole column out of the grid. A row of theme tiles fell from four to two that
way, on one theme called "New Theme (10)".

The other modes are unchanged. `LeftIcon` puts the caption BESIDE the picture, where the two
together are the width and the caption has no reason to wrap to the icon; `TextLabel` has no
picture; `IconOnly` lays no text out at all.

## PanelBase

### bodySlotSettled

A panel places its bars, settles the rect its body will stand in, and then lays the body into it.
`bodySlotSettled` is the moment between those two: the slot's width takes no more changes, and the
body has not been asked for anything yet - so whatever a panel wants to state ABOUT that slot
reaches the body on the pass that settled it, rather than one pass late.

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

Called whether or not a body stands in the slot - the slot is settled either way, and a panel
stating something about it should not have to ask whether anything is in it yet.

## SplitButtonBase

A button carrying a second target: a part that is pressed on its own account and answers
with something other than the button's primary action - a dropdown strip, a tab's close
button. What lives here is the part's ownership, the room it takes out of the content box,
and the rule that a press on the part is not a press on the button. What the part looks
like, where exactly it lands and what pressing it does belong to the derived class.

## MessageBoxBase

The shell every dialog of this family is: a title across the top, a middle carrying
an icon and the text, and a strip of answers across the bottom. What a derived dialog adds
is the answers it offers and what it does with the text - the shape is settled here.
THE THREE BARS ARE FULL BLEED. Each states its own colours and its own padding, and
the window keeps of itself exactly the border it draws. A strip inset further reads as a
card floating on the window rather than as a band of it.
THE TEXT IS A `Text`, not a run of characters. What a dialog is about is named in
the sentence it puts - a theme, a file, a page - and the sentence has to be able to mark
which words those are. A caller with nothing to mark writes `Text{ L"..." }`.
Built on the stack where it is asked, filled, then run: the derived execute() answers
once the dialog has closed, so nothing outlives the call that raised it.
A POPUP, not a dialog window. That is what makes it safe to raise one from inside a
click: a popup registers as its owner form's active popup, so the next press anywhere
underneath closes it and goes no further. A dialog window leaves the window beneath it live
- nothing here disables it - and the work that raised the question could be reached again,
and undone, while the question was still standing.

## DropdownControlBase

A control that drops a popup: the mark that says so, the optional strip that carries it,
the way that strip is shaped against the control's outline, the rule for who owns the
popup, and the keys that open it. The strip is SplitButtonBase's secondary part, so the
room it takes and the routing of a press onto it come from there. What the popup contains,
and what pressing the control does when it is not the strip, are left to the derived class.

## OpensWindow

Whether the command the button stands for opens a window of its own. The button says so with a
mark at the right-hand end of its line - a box with an arrow leaving its corner - and that mark
is what a trailing ellipsis in the caption otherwise stands for. A caption that carries the mark
does not also carry the three dots.

The mark rides in the button's own text, after a flex space, the way a menu item's shortcut key
does. A column of buttons stretched to one width lands every mark on the same edge without a
column to align them in; a button measured to its own caption leaves the space nothing to take,
and the mark then stands at the flex space's own floor - which is why that floor is stated and
no separate space is written beside it.

An item carrying both a mark and a shortcut key writes the mark first and the key at the very
end. Only one flex space may open a line - two would split the slack between them and strand
both in the middle - so the key takes a plain space of the same width when the mark has already
opened it.
