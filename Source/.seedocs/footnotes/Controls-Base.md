# Controls Base

The words that no longer fit above a declaration.

## ButtonBase

### A picture standing over the words sets the width

`TopLeftIcon`, `TopCenterIcon` and `BottomIcon` put the picture and the caption on separate lines,
so the button's natural width is the picture's and the caption wraps under it - `measureText` asks
for the text at the icon's width rather than at the whole box.

A caption stated `WordWrap::No` has nothing to wrap under the picture, and is measured on one line
at its own width. `MenuCommand` in the menu strip is built that way, because the strip has to show
every name whole. Measured at the icon's width, a word longer than the icon cannot break: the text
engine reports it at the width it was asked about, and the paint cuts it.

The other modes are unchanged. `LeftIcon` puts the caption **beside** the picture, where the two
together are the width and the caption has no reason to wrap to the icon; `TextLabel` has no
picture; `IconOnly` lays no text out at all.

### What stands beside the words comes off their width

A wrapping caption is broken in the box it is drawn in, and the box `measureText` is handed is the
content's. The indicator and a `LeftIcon` stand beside the caption, so `measureText` takes them off
the way `adjustTextRect` does - `adjustForSideParts` states it for both - and `SplitButtonBase`
takes a part at either side off as well.

## PanelBase

### bodySlotSettled

A panel places its bars, settles the rect its body will stand in, and then lays the body into it.
`bodySlotSettled` is the moment between those two: the slot's width takes no more changes, and the
body has not been asked for anything yet - so whatever a panel wants to state **about** that slot
reaches the body on the pass that settled it, rather than one pass late.

Called whether or not a body stands in the slot - the slot is settled either way, and a panel
stating something about it should not have to ask whether anything is in it yet.

### bodyExtent

The box the body is laid out into: the slot, unless the panel states it larger. Asked right
after bodySlotSettled, so the body is laid out once, at the size the panel wants, in the pass
that settled the slot. Only a panel whose height is dictated - Fill, or hosted as a body - is
asked: one growing to hold its body would grow by whatever it stated past the slot. ScrollBox
states the room past its body's end here - see Controls#scrollmetrics.

### The text in the body slot

A panel's own text in the body slot - `TextPlacement::Body` - is broken at the width of that slot,
so that is the width it remembers once the bars are placed: the remembered text width every control
carries - see Control-Foundation#remembered-text-width. `PanelBase::alignContent` states it in
place of `Control::alignContent`, which would remember the whole content width.

### createSplitter

Puts a splitter between a bar and the body, and from then on the pointer sizes the bar: dragging
the line moves the edge the bar and the body share, and a double click on it fits the bar to what
its content measures. The bar is created first - a splitter stands beside a bar that is already in
its slot, and the body has none.

The bar keeps every metric it was built with. What a drag or a fit states is the bar's preference
for the passes that follow - `MinSize` holds it up and `MaxSize` holds it down, as for any
preference - and a drag is held between the bar's floor and the room the body has over its own. A
`PreferredSize` on the bar is where the bar starts, and a fit sets it aside for the one pass that
measures the content. `barExtent` reads what the bar stands at, in design units; `setBarExtent`
states it, `resetBarExtent` clears it and `fitBarToContent` does what the double click does.
`OnBarResize` is raised with what the bar comes to whenever the pointer resizes it, so an
application that keeps the split writes the event's extent down and states it back with
`setBarExtent`.

The splitter stands in the gap the panel's spacing leaves between the bar and the body, and widens
that gap only where the spacing is thinner than its line. The band the pointer takes it by is
wider than the line and reaches over the bar and the body either side, which is why the splitter
is an overlay of its panel: it is met before the two and drawn after them.

## Splitter

The line between a bar and the body that the pointer drags to resize the bar, made by
`PanelBase::createSplitter` and by nothing else. It is a mouse target and nothing more: the focus
never rests on it and no key moves it. The line is drawn in the Divider's colours, upright beside
a left or right bar and across the panel beside a top or bottom one, and the pointer over it wears
the resize arrow of that direction.

## SplitButtonBase

A button carrying a second target: a part that is pressed on its own account and answers
with something other than the button's primary action - a dropdown strip, a tab's close
button. What lives here is the part's ownership, the room it takes out of the content box,
and the rule that a press on the part is not a press on the button. What the part looks
like, where exactly it lands and what pressing it does belong to the derived class.

**The part stands at the content's right, below it or at its left**, as the control's
`SecondaryPartPlacement` says. A part on the left moves the caption and the icon along by the
room it takes. The selection indicator stays where `ButtonBase` puts it, so a control with a part
on the left carries no indicator.

## MessageBoxBase

The shell every dialog of this family is: a title across the top, a middle carrying
an icon and the text, and a strip of answers across the bottom. What a derived dialog adds
is the answers it offers and what it does with the text - the shape is settled here.
**The three bars are full bleed.** Each states its own colours and its own padding, and
the window keeps of itself exactly the border it draws. A strip inset further reads as a
card floating on the window rather than as a band of it.
**The text is a** `Text`, not a run of characters. What a dialog is about is named in
the sentence it puts - a theme, a file, a page - and the sentence has to be able to mark
which words those are. A caller with nothing to mark writes `Text{ L"..." }`.
Built on the stack where it is asked, filled, then run: the derived execute() answers
once the dialog has closed, so nothing outlives the call that raised it.
**A popup**, not a dialog window. That is what makes it safe to raise one from inside a
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

## EndIcon

The icon a button carries at the right-hand end of its line. OpensWindow says the command the
button stands for opens a window of its own - a box with an arrow leaving its corner - and that
mark is what a trailing ellipsis in the caption otherwise stands for. A caption that carries the
mark does not also carry the three dots.

The mark rides in the button's own text, after a flex space, the way a menu item's shortcut key
does. A column of buttons stretched to one width lands every mark on the same edge without a
column to align them in; a button measured to its own caption leaves the space nothing to take,
and the mark then stands at the flex space's own floor - which is why that floor is stated and
no separate space is written beside it.

An item carrying both a mark and a shortcut key writes the mark first and the key at the very
end. Only one flex space may open a line - two would split the slack between them and strand
both in the middle - so the key takes a plain space of the same width when the mark has already
opened it.

## WithBody

Names the type of a host's body slot and builds that body from its own props bag. The body stays
a distinct child - the host positions, clips and hides it independently - so this puts a static
type over the slot rather than folding the two into one control. That is what separates it from
Form<ControlClass>, where the content is the whole of the client area and the form is not itself
a node in the control tree. Each props bag routes by type, so a bag must not carry the same prop
type twice.
