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

`IconOnly` lays no text out, and its words serve as the hint where none is set.

## PanelBase

**A panel's own text in the body slot** - `TextPlacement::Body` - wraps at the slot's width. That
is the width it remembers for the next measure - see Control::textWidthLimit.

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
`MaxSize` still clamp it. A drag also stops where the body reaches its own floor. A
`PreferredSize` on the bar is where it starts.

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
