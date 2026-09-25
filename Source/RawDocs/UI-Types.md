# UI Types

The words that no longer fit above a declaration in `Y-Core/System/UiTypes.cppm`. Referenced
from that file.

## InputStamp

WHAT THE USER DID THAT LED HERE, in whatever form the display server states it. A display server
answers a request made on the user's behalf - moving a window by its title bar, opening a menu
that takes the pointer, writing the clipboard - only when the request names the input event that
asked for it, by the number the server itself put on that event. A request naming no event is
refused.

It is not a timestamp. A timestamp says when; this is the server's own count of the events it
has sent, and the two are different numbers with different rules.

Nothing above the platform layer reads one, and nothing above it may invent one: a stamp is
filled where an input event enters the framework, carried by whatever is built out of that
event, and handed back by a call that needs it. A platform whose requests need no such proof
fills nothing here and reads nothing.

## HitTest

Hit testing is performed top-down from the parent through all nested controls. If a parent is
set to `HitTest::Transparent`, none of its child controls will receive mouse input.

## Interactivity

The difference between HitTest and Interactivity. HitTest filters and reroutes mouse input based
on the cursor position only. Interactivity determines how a control participates in keyboard
navigation (focusable items) and how it reacts to mouse move and click events - hover and click
animations. Interactivity is evaluated only after HitTest confirms the cursor is over the
control.

## WindowRole

What the platform layer switches on - window class, styles, activation, how it is shown.

NAMED AT THE CREATION SITE: a form is created with the role it is meant to have, and its root
control's `Interactivity` is a separate answer the root gives for itself. The two are about
different things - what the WINDOW does with activation, and how the CONTROL takes part in focus
and selection - so neither is read off the other. A popup list whose root tracks a current item
is the case that says so: `WindowRole::Menu`, root `ActiveContainer`.

## IPlatformServices

THE PLATFORM, AS A SERVICE STANDING BELOW THE CONTEXT SEES IT: a name and nothing else. The
clipboard is handed one at construction, because Transfer cannot import Context - Context
imports Transfer for the clipboard, and Transfer names nothing above itself - so the name
the clipboard holds the platform by has to be declared below both. What stands behind the
name is each platform layer's own statement: on Win32 the Windows IMessageWindowFactory,
which Win32Platform derives from and the clipboard's Windows half casts to for its listener
window; on Wayland the IDisplayAccess of the Wayland layer, which WaylandPlatform derives
from and the clipboard's Wayland half casts to for the display. The cast is what keeps the
neutral name empty: no platform answers for a thing it has no notion of.

## PresenterRole

What a control does for an action it presents. Read where the action is attached - see
`Action::attach` - and given as a construction property by a control that is not the plain case.

`Button` delivers the action's click and shows nothing of the command but its face: a button, a
toolbar item. Its tooltip falls back to the action's name and key, which a button has nowhere
else to state.

`Line` delivers the click and writes the name and the key itself: a menu line. Nothing is
composed for its tooltip, which would say back what the pointer is already standing on.

`Display` shows the command without delivering it - a split button's strip, whose press is
routed by the button it is part of. Tooltip as for a Button.

## ActionState

What the data says about a control: whether it may be acted on, and whether it is chosen. Both
are asked for and never stored - `Control::state` walks for the answer.

`VisualState` is the other half, and the two must not be confused: this one is what the
application knows, that one is what is drawn, and every field of it that shares a name here is
an animated factor rather than a fact.

## VisualStateIndex

`Current` is the one item a container hands the focus back to when it is entered again. Not the
same idea as `Focused`: a multi-select view keeps a current item to anchor its Shift ranges on
while the items it selected are the ones drawn `Selected`, and it keeps that item while the
mouse rather than the keyboard drives the input.

## InputDevice

Which device the user last acted with. `Mouse` is zero because the value doubles as the
animation target that fades the focus ring in as the keyboard takes over - see
`Input::setDevice`.

## HorizontalTextAnchor

Where the measured text block sits horizontally inside its designated zone. It carries the box
each paragraph is aligned within, so text naming its own `TextAlign` is displaced by the anchor
and by the alignment both - such text is anchored `Left` and placed by the `TextAlign` alone.

## WordWrap

Whether a text is broken to the box it is drawn in. A text that is not comes out as wide as it
is, which is what a horizontally scrolled control needs: the scroll has to be told how wide the
text is, and a text broken to the box can never be wider than one.

## TextAnchor

Both axes together, because they answer one question: where inside its zone the text block was
put. Every mapping between a control's coordinates and the text's own - drawing it, hit testing
it, measuring a caret in it - is made against this, and a mapping made against one axis alone
reports the caret where the text is not.

## FormPlacement

`Default` and `ScreenRight` are the two a TOPLEVEL takes, and neither reads the anchor.
`Default` is the platform's own placement, used for dialogs, and the placement rect is ignored.
`ScreenRight` puts the window against the right edge of the work area, the full height of it, at
the width asked for; a platform that does not place toplevels for its client answers the size
and leaves the position to the display server - see Wayland's `Window::place`.

Everything else applies only to child popups, and the placement rect must be in the parent
form's coordinates.

## AutoFit

Which way a form's size and its content settle against each other.

`No` - the content is laid out into the window, which is right for a window the user sized.
`Yes` - the window is placed again around whatever the content came out as, which is what a
popup that grows with what is typed into it needs.

The placement clamps to the monitor either way, and that clamp is the bound of last resort.

## ShowSurfaceAtRest

Whether a button carries a surface when nothing is happening to it. `No` is what a tool button
carries: the surface colour rule is not applied, so the fill exists only as far as the hovered,
selected and pressed states carry it, and grows in from `ThemeMetrics::surfaceGrowInScale` as it
arrives.

## ArrowPlacement

Where the dropdown mark sits, and with it whether the control has a strip that can be pressed on
its own.

## DropdownWidth

Design width of the dropdown strip. It applies to `ArrowPlacement::Right` - a bottom strip is as
tall as the text it carries, and an in-text mark has no strip to size.

## ExpanderViewMode

What an expander draws of itself, and what its header is built from. The header paints the label
in all three, and the button shows the mark alone, so picking a header and opening it are
separate acts on separate targets.

`Section` fills the page and its header, with the chevron at the right end of the strip: a group
that reads as a box with a heading.

`Divider` states no colours at all. The chevron keeps the right end, the label goes against the
left edge, and a line fills what lies between them, so the heading reads as a labeled divider
across whatever it stands on.

`TreeNode` states no colours either and has no line. The chevron leads the strip and the label
follows it, which is the shape a tree of expanders is built from: the click that opens a node is
the button's, and the rest of the row is left to whatever picks it.

## PanelSlot

Where a bar sits in a panel, in the order the panel is read: across the top, then the middle row
from left to right, then across the bottom.

## MessageIcon

The picture a dialog leads with. One name per kind of thing a dialog is about, so two dialogs
asking the same kind of question never lead with different pictures.

`Question` and `Information` carry the same hue - a question and a notice, not two degrees of one
thing - and their glyphs are the whole of what tells them apart.

## EditorMode

What an in-place editor is for, and whether one opens at all.

`None` - nothing opens, and the gesture goes on meaning whatever else it meant.
`ReadOnly` - a reader: the same box over the same text, selected whole and refusing every
change. It is how a value too long for the place it is shown in is read whole and copied out of,
and nothing is written back.
`Editable` - the value is typed, and offered back to its source when the edit ends. The source
may refuse it and say why, and the editor stays up for the value to be corrected.

A grid states it per COLUMN, because a column is what says where a cell's text comes from, and a
value that can be typed back is a property of that source rather than of the row that happens to
hold one. What an Editable cell is left with goes to RowBase::acceptCellText.

ReadOnly is a column's default because a cell TRIMS what does not fit its column: every text
cell therefore has a value worth being read whole and copied out of, and a column that wants
that says nothing. Only a cell drawn as plain text opens one - a cell holding a control is that
control's, whatever its column says.

A control built on WithInPlaceEdit answers editorMode() instead, asked each time an editor would
open, so a control with conditions of its own answers None while they hold. The default is
Editable while anything listens for AcceptEditEvent and None otherwise. StdActions::rename is
enabled by Editable alone - a reader shows a name and does not change it.

A combobox states it as a property, None by default. Editable makes the face where a value is
typed: a press on the face opens the editor and the strip alone drops the list, Return opens the
editor, and a character typed on the combobox opens it with that character in it. A combobox
with no strip keeps the list on its face, and F4 and Alt+Down drop it from the keyboard either
way. What the editor is left with goes to ComboboxAcceptTextEvent - see Controls.

## ChevronTurn

Where a mark points, as a fraction of a full turn clockwise from pointing down - which is how
the chevron path in `Icons/Chevron.cppm` is authored.

A control animating between two of these picks the pair that states the turn it wants. The mark
travels the signed distance between them, so right to down is a quarter turn clockwise while
down to up is a half turn the same way.

## DropdownMarkTurn

Where the dropdown mark points with the popup closed and with it open. The mark is animated
between the two and travels the signed distance, so the pair states the turn it makes as well as
the two rests.

## ColorMode

Which side of the theme something is drawn on. It is the stored fact; the sign below is
derived from it, because a mode is what anyone chooses and a sign is only how the arithmetic
spells it.

## ColorModeSetting

The mode the application is drawn in, as the user chose it: Auto, which follows the desktop, or
Dark or Light stated outright. Auto stands first and is what a config stating nothing holds. What
is drawn is always a ColorMode - a setting is read into one where a theme is put on, see
wornColorMode in Application - so nothing that paints ever holds Auto.

## ColorSlot

Which of the theme's semantic hues an ink is drawn from. Each is a region of the wheel
rather than a place in an order, so a slot carries the colour its name states whatever the
anchor is, and one harmony fills all four.

Only a hue is named here. What a slot is drawn as - how colourful, and how light - is stated
once per colour mode: by InkWell's tones for an ink colour, and by the painter for a slot it
draws directly. See InkTone.

## InkTone

How colourful a colour is and where it sits. The two travel together because a hue needs a
different amount of colour to read at one luminosity than at another, so stating either
without the other names half a colour - which is why a mode-pair is a pair of these rather
than a pair of luminosities.

## SlotTones

One colour stated once for each side of the theme, rather than one elevation the mode would
orient. Which of the two is read is the painter's mode rather than the theme's, and a theme
standing between the sides is drawn between the tones.

## InkColor

How an ink's grade becomes a colour. Every ink is read the same way: its grade is a share of
the gap between the surface and an ink, and the colour names the ink at the far end - the
chain's own, one of the palette's hues, pure black or white, or for Accent and Spot one of the
theme's rules laid over the chain's ink. The theme owns those rules, so what is written here is
which one and nothing about what it does.

The rule goes on before the mix. A rule that sets its channels outright states where the colour
lands, so laid over the mixed ink it would answer the same colour at every grade.

The theme's rule is read at the lightness the painter stands at. On an element that states a
flip that is the far side of the theme, so a spot run in a hint rises off the hint's dark
surface the way a muted run beside it does.

## Ink

A colour named rather than stated. A text run or an icon holding one is recoloured by the
control it lands in and by the theme in effect, which a resolved Color cannot be.

Nothing constructs one directly. Every colour is built from the same number - a share of the
ladder - so a constructor could not say which was meant. InkWell holds one accessor per colour,
each taking the grade and nothing else, and the named colours built from them.

## DirWatchChangeEvent

Carries no description of what changed. Every backend may drop events under load -
inotify reports IN_Q_OVERFLOW, Win32 truncates its buffer - so a correct handler rescans
the directory rather than acting on a delta it cannot trust to be complete.

## DirWatch

Watches a directory and reports that its contents changed. The watch is not recursive:
only entries directly inside the directory are observed.

A directory that does not exist cannot be watched. The watch is then not established,
`watching()` answers false, and nothing is reported when the directory is made. `restart()`
is what puts the watch over it once it is there, and it does so before returning: a change
made right after `restart()` is reported.

## RepeatEvent::elapsedSeconds

Time since the previous repeat. An action that moves at a speed multiplies by this
instead of counting repeats, so a machine that misses ticks covers the same ground in
fewer, longer moves.

## InkGrade

A shade of the ink, stated as a share of the gap between the surface an ink is drawn on and
the ink itself: 0 lies flush with the surface and cannot be seen, 1 is the ink. Stated as a
share rather than as a luminosity so that a theme with quiet contrast keeps it - every step
scales with what the theme itself declared readable.

These five are the steps the framework draws in. A caller wanting one of its own states the
share instead; nothing rounds a stated grade to the nearest of these.

## InkColor::Text

A shade of the ink the paint chain arrived at, and the colour an Ink starts with. Both ends of
the mix come from the chain, so a text ink owns no colour, no hue and no tone of its own.

## InkColor::Accent

The hue the theme's own live states are drawn from - a button under the pointer, a
check, the band behind selected text - and so the ink for anything asking to belong to
the controls around it.

## InkColor::Spot

The second accent, for what stands apart from the interface rather than answers to it:
a brand mark, a run of emphasised text, the tint a tooltip carries. Held apart from the
accent so that spending it sparingly is the theme's decision and not each caller's.

## InkColor::Yellow, Green, Blue, Red

One of the palette's semantic hues, drawn at the tone InkWell's slotTones states for the
colour mode the painter stands in. The grade fades it toward the surface the way it fades
the text ink, so at Strongest the hue stands at its own tone.

## InkColor::Black, White

Pure black and white - saturation 0 at luminosity 0 and 1 - the same on either side of the
theme. The grade fades one toward the surface the way it fades the text ink.

## Hsl

A colour held on three channels, each a fraction of its own range, over the Oklab
colour space. What the channels buy over an RGB cylinder is that each one means one
thing wherever it is read:

- luminosity is perceptual, matching CIE L* for greys. Half way up the channel is half
  way up to the eye, so one offset is the same visible step at every starting point and
  under every hue, and comparing two luminosities compares what a person sees.
- saturation is chroma as a fraction of the most the sRGB gamut holds at that hue and
  luminosity. Every triple in the unit cube therefore names a colour that exists, which
  is what lets a rule set a channel outright without asking whether the result can be
  drawn.
- hue is a fraction of a turn, and holds its appearance as the other two move. Equal
  steps around it are equal steps to the eye, so colours picked at even spacings look
  evenly spread.

## UiTimer

Class is called UiTimer, since it's WM_TIMER based,
and no timer ticks ever happens while there are active user input messages
