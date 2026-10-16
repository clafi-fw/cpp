# UI Types

The words that do not fit above a declaration in `Y-Core/System`.

## Cursor

A handle on Win32, a shape number on Wayland. Platform::cursor returns the one shown, and
Platform::setCursor puts it back. **Only the platform makes one.**

## InputStamp

**A request made for the user names the input event behind it** - moving a window by its title
bar, opening a menu that takes the pointer, writing the clipboard. A display server that asks for
this proof refuses a request with an empty stamp. Win32 asks for none, and the stamp stays empty.

**Pass a stamp along unchanged.** It is filled where an input event enters the framework, and the
call that needs it takes it back. The value is the server's serial for the event, not a timestamp.

## HitTest

**Transparent covers the children too.** A parent answering Transparent keeps mouse input from
every control inside it.

## Interactivity

**HitTest comes first.** It routes mouse input by the pointer position alone. Interactivity is read
once the pointer is over the control, and sets how it takes part in keyboard navigation and
answers the mouse - hover and click animations.

**A mouse press focuses the first control up the chain that can take the focus** - Focusable or
ActiveContainer. A mouse press on a label inside a tile focuses the tile.

**A MouseOnly control is pressed for itself**, and leaves the focus where it is. A tool bar's
button keeps the caret in the editor beside it. A part is the exception - see Control::isChildPart.

## Control isChildPart

**A part's mouse press is its owner's.** A part is a slider's thumb, a check box's mark, a split
button's strip or a control hosted in a grid cell. The focus walk passes over a MouseOnly part and
goes on from the owner. ButtonBase, SliderBase and Grids::RowContainer answer yes for every child.

## WindowRole

**The role is given where the form is created**, and sets what the window does with activation.
The root control's Interactivity is a separate answer, for how the control takes part in focus and
selection. A popup list whose root tracks a current item is `WindowRole::Menu` with an
`ActiveContainer` root.

## PresenterRole

Action::attach takes it, Button by default. A control built with an Action property reads the
role from its properties too.

- `Button` - a button or a toolbar item. Its tooltip falls back to the action's name and key.
- `Line` - a menu line, which writes the name and key itself, so the action adds no tooltip.
- `Display` - a split button's strip. It shows the command, and the button it belongs to delivers
  the click.

## ActionState

**Asked for, never stored.** Control::state walks up the parent chain for the answer.

**ActionState is what the application knows, and VisualState is what is drawn.** The drawn
enabled and selected animate through StateFactors.

## VisualStateIndex

**Current is a separate state from Focused.** A multi-select view anchors its Shift ranges on its
current item, and keeps it while the mouse drives the input. The items it selected are drawn
Selected.

## HorizontalTextAnchor

**Left, Center and Right make a block** as wide as the widest line. TextAlign aligns the lines
inside the block, and the anchor moves the block as a whole. A right-aligned paragraph anchored
Left ends at the right edge of its widest line.

**None, the default, makes no block.** The lines align across the whole zone, as a page or a grid
cell needs. A stated break width is where such a text is placed - see TextLayout::setBreakWidth.

**A wrapped text's block is at most its break width**, even where a line overflows it.

**An over-text tooltip repeats an anchored text inside its block.** GetTooltipEvent::textAnchor
comes back Left, and the anchor rect starts at the block.

## WordWrap

**WordWrap::No measures the text at its full width.** A horizontally scrolled control needs that
width, and a wrapped text is never wider than its box.

## TextAnchor

**Map with both axes.** Drawing a text, hit testing it and measuring a caret in it all map through
a TextAnchor. A mapping that reads one axis alone puts the caret where the text is not.

## FormPlacement

**Default and ScreenRight are for a toplevel**, and ignore the placement rect. Default is the
platform's own placement. ScreenRight takes the full height of the work area at its right edge, at
the width asked for. Where the display server places toplevels itself, only the size is answered.

**The rest are for child popups**, against a placement rect in the parent form's coordinates.

- OverText lays the window's own text over the text in the rect - an in-place editor, an
  over-text tooltip.
- Each of the others flips to the opposite side of the rect when the side asked for has no room.
  Placed Right on a rect spanning a list, a window flips to the list's own left edge.

## AutoFit

**Yes is for a popup that grows with what is typed into it**, No for a window the user sizes.
Either way the placement is clamped to the monitor.

**A dialog its content sizes leaves the user nothing to resize** - no edge to drag, no maximize.
Build its DialogTitle with `TitleButtons::Close`. Placed again, the dialog keeps its position and
takes the new size.

## DropdownWidth

**Under a top icon the strip sits below it** and carries the text, as tall as that text. This
width does not apply there.

## ExpanderViewMode

**The header shows the label and the button shows the chevron**, so picking a header and opening
it are separate targets.

**The header takes the focus, and the chevron answers the mouse alone.** On a focused header Right
opens and Left closes, and a press with nothing to open or close goes no further. `TreeNode` hands
that press to its tree, which climbs on Left and steps into the rows on Right.

**Return follows the label's first link** where the label has one. Otherwise Return and Space
toggle the header in every look but `TreeNode`, where they pick the row as a mouse click on it does.

- `Section` - a filled page and header, the chevron at the right end.
- `Divider` - paints nothing of its own. The label is at the left, the chevron at the right, and a
  line runs between them.
- `Ellipsis` - paints nothing of its own, and has no line. The chevron follows the label, and a
  muted ellipsis trails it while collapsed. A held header spans the width of what scrolls under it.
- `TreeNode` - the chevron leads and the label follows. The row shows hover, focus and pick as a
  tool button does, and a mouse click opens the node on the chevron alone.

## MessageIcon

**Pick by what the dialog is about**, so dialogs about the same kind of thing lead with the same
picture. Question and Information share a hue, and their glyphs alone tell them apart.

## EditorMode

**ReadOnly opens a reader** - the same box, refusing every change - so a value too long for its
place can be read whole and copied. **An Editable value can be refused** with a reason, and the
editor stays up for a correction.

**A grid states it per column**, ReadOnly by default, since a cell trims what does not fit. An
Editable cell's text goes to RowBase::acceptCellText. A cell holding a control leaves editing to
that control.

**A control built on WithInPlaceEdit answers editorMode()** each time an editor would open -
Editable while anything listens for AcceptEditEvent, None otherwise. Override it to answer None
under conditions of the control's own. StdActions::rename is enabled by Editable alone.

## ComboBox editorMode

The default is None. **Editable makes the face a place to type:**

- a press on the face opens the editor, and the strip drops the list;
- Return opens the editor, and so does a typed character, which goes in.

A combo box with no strip drops the list from its face. F4 and Alt+Down drop it either way. The
typed text goes to ComboBoxAcceptTextEvent.

## DropdownMarkTurn

**The mark turns through the signed distance from closed to open**, so the pair sets the
direction as well. The values are ChevronTurn's fractions of a turn, clockwise from pointing down:

- the default, `down` to `up` - a half turn clockwise;
- a breadcrumb crumb, `right` to `down` - a quarter turn clockwise.

Override DropdownControlBase::dropdownMarkTurn to give a pair.

## ColorMode

**The mode is what is stored.** Where a number is needed, lightnessOf answers 0 for Dark and 1 for
Light.

## ColorModeSetting

Auto follows the desktop, and is what a config stating nothing holds. **Painting always reads a
ColorMode** - see wornColorMode - so nothing that paints holds Auto.

## Pigment

**Each pigment is a region of the hue wheel**, so it keeps the colour its name states whatever the
theme's anchor hue. It sets the hue only. The tone comes from `InkWell::pigmentTones` for an ink,
or from the painter for a pigment it draws directly.

## InkTone

**Saturation and luminosity travel together.** A hue needs a different amount of colour to read at
each luminosity, so either alone is half a colour.

## PigmentTones

**The painter's mode picks the tone**, not the theme's. A theme between the two sides is drawn
between the tones.

## InkColor

**The grade is a share of the gap from the surface to an ink**, and the InkColor picks the ink -
the chain's own, a palette hue, black or white. Accent and Spot lay one of the theme's effects over
the chain's ink.

**The effect goes on before the mix**, so an effect that sets its channels outright still fades
with the grade. It is read at the painter's lightness, so a spot run rises off its surface the way
a muted run does.

## Ink

**An Ink becomes a colour where it lands**, by the control it is drawn in and the theme in effect.
A Color stays as it is.

**Take one from InkWell** - one accessor per colour, each taking the grade alone, and the named
colours built from them. A constructor taking a grade could not say which colour was meant.

## DirWatchChangeEvent

**Rescan the directory in the handler.** Every backend may drop events under load - inotify's
IN_Q_OVERFLOW, Win32's truncated buffer. Changes close together raise one event, 60 ms after the
last.

## DirWatch

**Only entries directly inside the directory are watched.**

**A missing directory leaves the watch down** - watching() answers false, and making the directory
reports nothing. Call restart() once it exists. The watch is up when restart returns.

## WebFetchEvent

Raised once per fetch, on the UI thread. WebFetch::status() is 0 where no answer came - no
connection, a timeout, a body over the limit, or a fetch the platform could not make.

## WebFetch

**A redirect is answered as it comes**, not followed. A body over a megabyte is refused whole.

**Destroy a fetch outside its own WebFetchEvent handler** - the fetch's own timer raises the event.

### WebFetch on Windows

WinHTTP, blocking, on the fetch's thread. It uses the system's proxy settings and TLS, so an
application ships nothing for either. Each step may take ten seconds, and a cancel ends the call at
once.

### WebFetch on Linux

libcurl, loaded with dlopen when a fetch runs - libcurl.so.4, else libcurl-gnutls.so.4, the one a
fresh Ubuntu desktop carries. The build needs no libcurl, and a system without one answers every
fetch with nothing.

The connection may take ten seconds and the transfer thirty. A cancel takes effect within a second.

## WebFetch ~WebFetch

**The wait is short** - cancelling ends the request before the thread is joined.

## CommandOutcome

**A run is Running until the shell answers**, and then one of the other five for good. Failed is
every nonzero exit, from the program or from the shell - CommandRun::exitCode gives the code.

## CommandRunEvent

Raised once per run, on the UI thread, when the outcome leaves Running.

## CommandRun

The shell is ShellExecuteEx on Windows and /bin/sh on Linux. The program runs apart from the
application, and goes on after the application ends.

**The answer comes at once for a program that is not found**, and when it ends for one that runs -
which may be never in the life of the application.

**The object is the answer's lifetime.** Whoever holds the run hears the answer while it holds it.
An editor started from a page goes on after the page has gone, and nothing is raised for the page.

**Destroy a run outside its own CommandRunEvent handler** - the run's own timer raises the event.

### CommandRun on Windows

**The line is a program and its parameters** for ShellExecuteEx, with no cmd syntax - no `&&`, pipe
or `start`. The program is the first word, or the quoted run. The shell finds it as the Run box
does, on the PATH or under App Paths, with environment variables expanded. A .cmd or .bat opens a
console window.

- NotFound - no program answers to the name.
- NotRunnable - any other refusal, such as a declined elevation.
- Ended - exit code zero, or the line went to a program already running.
- Failed - any other exit code.

### CommandRun on Linux

**`/bin/sh -c` runs the line**, so shell syntax works. The program runs detached, and the
application has no child to reap.

- 0 - Ended.
- 127 - NotFound, and 126 - NotRunnable.
- Any other code - Failed. A program a signal killed answers 128 plus the signal's number.
- NotStarted - the shell could not be forked.

## CommandRun::exitCode

**Zero** while Running, for NotStarted, and on Windows wherever the shell refused the line.

- Linux: a program killed by a signal answers 128 plus the signal's number, as a shell does.
- Windows: a program that crashed answers the status it went down with, such as 0xC0000005.

## RepeatEvent::elapsedSeconds

**Multiply a speed by it** rather than counting repeats. A machine that misses ticks then covers
the same ground in fewer, longer moves.

## InkGrade

0 is flush with the surface, and 1 is the ink. **A grade is a share, not a luminosity**, so the
steps scale with the contrast the theme declared readable.

The framework draws in the five steps. A caller can state any share, and nothing rounds it to a
step.

**A grade is stated as a dark surface reads it.** Over a light surface the painter reads it
through lightGradeOf, and between the two sides it reads a share between the two.

## lightGradeOf

**A light surface counts the steps from the ink's end.** Each step lands as far from the surface as
its mirror is from the ink - Faint mirrors Strong, Subtle mirrors Muted:

- Faint at 0.20, Subtle at 0.40, Muted at 0.73, Strong at 0.88, each times
  `1 - luminosityFloor`;
- Strongest at 1, the ink itself;
- a share between two steps lands between theirs.

**The floor puts both sides in one set of greys.** Each step below Strongest stops short of the ink
as far as the dark side's surface is above black.

## InkColor::Text

**An Ink starts with Text.** Both ends of the mix come from the paint chain, so a text ink has no
colour, hue or tone of its own.

## InkColor::Accent

The theme draws its live states from it - a hovered button, a check, the band behind selected
text. **Use it for what belongs with the controls around it.**

## InkColor::Spot

**Spot is for what stands apart from the interface** - a brand mark, emphasised text, a tooltip's
tint. The theme decides how sparingly it shows.

## InkColor::Yellow, Green, Blue, Red

The tone is the one `InkWell::pigmentTones` states for the painter's side of the theme. Strongest
draws the hue at that tone, and a lower grade fades it toward the surface.

## InkColor::Black, White

Saturation 0, at luminosity 0 and 1. A lower grade fades it toward the surface.

## Hsl

**Each channel means one thing wherever it is read:**

- luminosity is perceptual, so one offset is the same visible step from any start;
- saturation is a share of the most chroma sRGB holds at that hue and luminosity, so every triple
  in the unit cube can be drawn;
- hue is a fraction of a turn, and equal steps around it look equal.

## UiTimer

**A timer fires once.** Each start arms one tick, and the timer is inactive by the time OnTick is
raised. Start it again from the handler to repeat, or use EventRepeater.

## Url

The place a browser tab is on, spelled `path#anchor`, or `path` alone. **`%23` is the one
escape** - str() writes a `#` inside either part as `%23`, so a theme file named `C#.clafitheme`
survives a round trip.

Url::parse reads back that escape alone. A web address's `%20` passes through as written, and a
name holding `%23` reads back with a `#` there.

## NoInitAllocator

**An element holds whatever the memory held.** Skipping the zeroing of a bitmap's worth of Color
is the point.

**The type must be trivially copyable**, since an element is destroyed and overwritten without
being constructed. A default constructor of its own is fine. A string, a vector or a unique_ptr
would be destroyed through a garbage pointer.

**Write every element before it is read.**
