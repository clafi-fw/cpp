# Controls Base

The words that do not fit above a declaration.

## ButtonBase

**A wrapping caption is measured in the box it is drawn in.** measureText takes off the selection
indicator, a `LeftIcon` and a SplitButtonBase side part, as adjustTextRect does. contentAroundText
adds the icon and the indicator back. So a check box or radio button laid out at its own width
keeps its caption whole.

**In `TopLeftIcon`, `TopCenterIcon` and `BottomIcon` the picture sets the width** - a wrapping
caption breaks at the icon's width. Set `WordWrap::No` to keep the caption on one line at its own
width, as MenuCommand does. At the icon's width, a long word would be cut.

`IconOnly` lays no text out, and its words serve as the tooltip where none is set.

## PanelBase

**A panel's own text in the body slot** - `TextPlacement::Body` - wraps at the slot's width. That
is the width it remembers for the next measure - see Control::textWidthLimit.

**A step moves between three rows** - the top bar, the side bars with the body between them, the
bottom bar - and sideways only along the middle one. The middle row is read at the step's line,
so Down from a top bar lands on whichever of the three stands under the item it left. A splitter
is no stop.

## PanelBase bodySlotSettled

**State what the body needs about its slot here** - it reaches the body on the same pass. The
slot's width is final by then. Called whether or not a body is in the slot.

## PanelBase bodyExtent

Asked right after bodySlotSettled, and only by a panel with a dictated height -
`VerticalAlign::Fill`, or hosted as a body. A panel growing to hold its body lays it out at the
slot's size.

ScrollBox states the room past its body's end here - see ScrollBox::bodyExtent.

## PanelBase createSplitter

Dragging the line resizes the bar. A double click fits the bar to its content, as fitBarToContent
does.

**Create the bar first** - one splitter per bar, and none beside the body. It shows and hides with
its bar.

**The bar keeps its metrics.** A drag or a fit sets only the bar's preferred size, so `MinSize` and
`MaxSize` still clamp it. A `PreferredSize` on the bar is where it starts.

**The body keeps its floor.** A drag stops where the body reaches its floor, and a panel made too
small for both cuts the bar instead, down to the bar's own floor. The extent stays what was
stated, so the bar comes back as the panel grows.

**Keep the split through OnBarResize.** Store the extent it carries, and hand it back with
setBarExtent.

The line sits in the spacing gap, and widens it only where the spacing is thinner than the line.

## Splitter

Made by PanelBase::createSplitter. **It answers the mouse only** - the focus skips it and no key
moves it. It is drawn in the Divider colours, and the pointer over it shows the resize arrow for
its direction.

## SplitButtonBase

The part has an action of its own - a dropdown strip, or a tab's close button.

**The base owns the part and its room.** A click on the part goes to secondaryClicked instead of
OnClick. The derived class creates the part and answers secondaryClicked.

**SecondaryPartPlacement puts the part right, below or left.** A part on the left moves the caption
and the icon along. A control with a part on the left carries no selection indicator - the
indicator stays where ButtonBase puts it.

## MessageBoxBase

A title across the top, an icon and the text in the middle, answers across the bottom. A derived
dialog adds the answers and what it does with the text.

**The bars are full bleed** - each states its own colours and padding.

**The text is a `Text`**, so it can mark the words it is about - a theme, a file. With nothing to
mark, write `Text{ L"..." }`.

**Build it on the stack, fill it, run it.** The derived execute() returns once the dialog has
closed, so nothing outlives the call.

**It is a popup**, so it is safe to raise from inside a click. A mouse press on the form behind
closes it before that press acts. A dialog window would leave that form live while the question
stands.

## DropdownControlBase

The strip is the SplitButtonBase part. **The derived class supplies the popup** in showDropdown.
dropOnPrimaryPress says whether a click outside the strip opens it too.

- F4 and Alt+Down open the popup while there is something to drop.
- A second mouse click on the control or its strip closes it.

## EndIcon

**OpensWindow marks a command that opens a window of its own** - a box with an arrow leaving its
corner. It replaces a trailing ellipsis, so a caption carrying it drops the three dots.

**The mark rides in the button's text after a flex space**, as a menu item's shortcut key does.
Buttons stretched to one width line their marks up on the right. A button sized to its caption
puts the mark after the words.

**One flex space per line** - two would split the slack. Text added after the mark, such as a
shortcut key, takes a plain space of the same width.

## WithBody

The body stays a separate child, which the host places, clips and hides on its own. A
Form<ControlClass> differs: its content is the whole client area, and the form is outside the
control tree.

**Each props bag holds a prop type once**, since props route by type.

## WithEditMode

**Outside the mode the control is an item of the page it stands on.** The keys travel past it to
the page - the arrows, Home, End, the page keys - and the mode's own look is off. Return, with no
modifier, a press or a menu asked on the control turns the mode on; Escape and the focus leaving
turn it off, and so does EditMode::No from code. A popup opened on the control - an edit menu, a
link's menu - takes the focus for a while and keeps the mode up.

**In the mode the host's keys apply**, through editModeKeyDown, and they are asked first: Return
on a link at the caret is the box's before it is the mode's. What the host leaves, the mode
answers: Return accepts and turns the mode off, Escape rejects and turns it off. A host that
takes line breaks - see WithEditMode takesLineBreaks - types Return as one, so the mode leaves it
alone there. The character the Return that turned the mode on leaves behind is not typing.

**Accepting and rejecting are the host's**, through editModeEntered and editModeLeft. A box typed
into keeps the text the mode opened on and puts it back on Escape, as an edit of its own; a slider
keeps its position. A read-only box has nothing to put back: its mode is a caret mode - arrows to
a link and Return to follow it, Return off a link or Escape to leave.

**A search showing a match in a box turns the mode on**, so the match stands selected, and takes
it off again as the search moves on.

## WithEditMode takesLineBreaks

**Its own answer, not wrapping.** A box wraps the words it is given whether or not it lets the
user type a line break, so the host says outright. The default is no: in every control the mode
switches, Return accepts.

## EditMode

The mode as it stands. Under EditModeSwitching::No it is Yes from construction and stays so.

## EditModeSwitching

- `No` - the control is in edit mode throughout: a box in a dialog, a code editor, a search
  field. The default for a box.
- `Yes` - the user turns the mode on and off - see WithEditMode. A box on a page walked with the
  keys, and every Slider unless the caller says No: a slider stands among other controls, and
  its arrows are the page's until Return.
