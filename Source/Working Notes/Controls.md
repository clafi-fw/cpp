# Controls

The author's side of the footnote of the same name.

## ComboBox

### The face is as wide as the widest item

Every pass that measures the face measures every item, answered by the text cache after the
first. `Control::calculateText` answers an empty text without calling `measureText`, so a face
with nothing picked and no placeholder measures no item at all - a combo box is built standing
on an item.

## Link hints

The hint asks a control for its hint once, when the pointer comes onto it, and a link is a part of
the box the pointer enters and leaves - so the box tells the hint when the link under the pointer
changes, through Hint::hoveredZoneChanged, the way a grid tells it its hovered cell did. A hint
already up is asked again where it stands, and one that is not starts the usual wait.

## ScrollBars::Auto

A ROOT ANSWERS AGAINST ITS MAXIMUM, in the measuring pass - see calculateContent. The window reaches
a root as that maximum, so a menu cut short by its placement is measured against the room it got and
puts the bar up in one pass. A BOX INSIDE ANOTHER ANSWERS THE VERTICAL BAR FROM ITS SLOT. Its
maximum is its own, and a window cut short lowers no maximum below the root's. So the align pass
records whether the body stood past the slot it was laid out into, and asks for another pass where
the bar standing is the wrong one - see settleAutoVerticalBar. The measuring pass that follows
stands the bar that answer names. One pass settles it: a body laid out narrower is never shorter,
nor a wider one taller. Until the box has been laid out once, its maximum answers for it. A pass
measuring what to ask a placement for records nothing - it lays the box out at the content's own
size, so its slot is no viewport - but it reads the answer, and the size it asks for carries the
bar. THE HORIZONTAL BAR IS ANSWERED AGAINST THE MAXIMUM ALONE. A box whose width is its host's
answer has no maximum of its own there and states the horizontal bar it wants. A BAR TAKES ITS STRIP
FROM THE OTHER AXIS. Content that fits beside no bar may not fit beside one, so once a bar comes up
the axis still without one is read again against what is left - see calculateContent. Only a bar
coming up is read then: one taken down would hand its strip back and ask the first question over. A
bar the second reading raises finds the other one up already, so a pass measures the box again twice
at most, and only where a bar changes.

## SuggestionList

Keys reach the list through the ordinary forward chain - the form under the editor forwards to the
editor, the editor to the list - and the list's walk starts from the box, which holds the focus. A
form whose popup is up but does not hold the focus keeps its characters; see wnd_char.

The list is asked for on every edit and answered on a 0 ms timer, once the input that made
the edit has been delivered - a held key writes characters faster than a window can be placed
for each. A request that finds the editor's layout unsettled waits on the pass that settles
it, so the list is placed on a box that has been laid out. It is hidden rather than
destroyed when nothing matches, and sized by what it lists: AutoFit, at least as wide as the
box, under it.

## HexView

ONLY THE LINES THE VIEWPORT REACHES ARE SHAPED. The view states its whole height through
calculateContent, so the scroll box it stands in takes the range from the layout and
nothing holds a second copy of it. The paint then draws the lines its viewport covers and
touches no others, which is what lets the view hold a buffer larger than a shaped document
could be.

EVERY COLUMN IS A COUNT OF CHARACTERS. One measurement of the monospace style per layout
pass gives the cell, and a line is drawn as ONE text laid out on that same grid - so where a
byte is drawn and where the view says it is cannot part. The style is one format both are
shaped under, so the run measured and the lines drawn cannot disagree about it either - see
Control-Foundation#text-format. This holds for as long as TextStyleId::Code resolves to a
font of one advance; a proportional fallback would slide the drawn line out from under the
rects.

## CodeBox

Beside the layout's shaping the box keeps a Syntax::LineStates - the state every line starts
in, which is all that has to be remembered, since a line's tokens are read off it and that
state in the time it takes to draw the line. The two are brought into step together, in
textTaken: an ordinary edit re-reads the lines from the one it reached until a line starts in
the state it already had, and a whole text stated anew is read from its start.

## CompletionList

The reading is compared with the last before any row is rebuilt, so typing in a routine's body
rebuilds nothing.

## PictureView

Each strip around the picture is staged on its own, so nothing the picture drew is staged twice.

The new size needs a pass before the bars range over it, so the view asks for the pass and sets the
bars when the form reports itself aligned - see FormAlignedEvent. Set, not left to glide: a bar
whose range shrank under it is on its way home, and setting it ends that where the placement says.
The point is kept to the fraction the placement floors off the origin: the picture is drawn on whole
device pixels and what the floor took is carried, so a slider dragged across its track, or an edge
dragged for a resize, does not walk the picture off its anchor a fraction of a pixel at a time.

What lies under the mark is worked out, never read back from the canvas: a backend that draws
through a staging buffer of its own hands back no pixels. A pixel the picture drew is read from the
cache, which holds every picture pixel on screen, and a square is known from where it stands.
