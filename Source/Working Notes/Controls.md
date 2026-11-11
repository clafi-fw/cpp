# Controls

The author's side of the footnote of the same name.

## ComboBox measureText

Every pass that measures the face measures every item, answered by the text cache after the
first. `Control::measureOwnText` answers an empty text without calling `measureText`, so a face
with nothing picked and no placeholder measures no item at all - a combo box is built standing
on an item.

## GetLinkTooltipEvent

The tooltip asks a control for its tooltip once, when the pointer comes onto it, and a link is a
part of the box the pointer enters and leaves - so the box tells the tooltip when the link under the
pointer changes, through Tooltip::hoveredZoneChanged, the way a grid tells it its hovered cell did.
A tooltip already up is asked again where it stands, and one that is not starts the usual wait.

The Ctrl+click line is written after whatever answered, a handler's words included: it says how
this box follows a link, which a handler previewing the target knows nothing about.

## ScrollBars::Auto

**A root answers against its maximum**, in the measuring pass - see calculateContent. The window
reaches a root as that maximum, so a menu cut short by its placement is measured against the room it
got and puts the bar up in one pass. **A box inside another answers the vertical bar from its
slot.** Its maximum is its own, and a window cut short lowers no maximum below the root's. So the
align pass records whether the body stood past the slot it was laid out into, and asks for another
pass where the bar standing is the wrong one - see settleAutoVerticalBar. The measuring pass that
follows stands the bar that answer names. One pass settles it: a body laid out narrower is never
shorter, nor a wider one taller. Until the box has been laid out once, its maximum answers for it. A
pass measuring what to ask a placement for records nothing - it lays the box out at the content's
own size, so its slot is no viewport - but it reads the answer, and the size it asks for carries the
bar. **The horizontal bar is answered against the maximum alone.** A box whose width is its host's
answer has no maximum of its own there and states the horizontal bar it wants. **A bar takes its
strip from the other axis.** Content that fits beside no bar may not fit beside one, so once a bar
comes up the axis still without one is read again against what is left - see calculateContent. Only
a bar coming up is read then: one taken down would hand its strip back and ask the first question
over. A bar the second reading raises finds the other one up already, so a pass measures the box
again twice at most, and only where a bar changes.

## InPlaceEditRoot offerText

2026-11-06. Oj, with a recording of the Themes sizes grid: Return opened the editor on a size the
chain gave, and a second Return with nothing typed stated that size as the style's own. Every
ending but Escape offered the box's text, and the sizes' sink takes any value typed as the style's
own statement, which is right for a value typed and wrong for one never touched. Decided here, in
the editor rather than in that sink: an untouched box offers nothing, and the edit ends as Escape
ends it, Cancelled and the sink never asked. Untouched is the box's undo depth at zero, which the
keys that opened the editor already leave behind them, and which undoing every edit comes back to -
chosen over comparing the text against the one handed in, which would have read a value retyped
as the same as one never typed. A sink that wants a retyped value to count still gets it.

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

## TextBox scrollHotspot

2026-10-18. A PageDown landing on a SeeDocs footnote showed its first line at the bottom of the
view: the hotspot was the caret whenever one had been placed, and the caret of a box the keys
land on stands on its first line. Oj's rule: out of edit mode the caret is not in use, so the
hotspot is the caret in edit mode only. A press enters edit mode before the release asks for
the scroll, so a mouse click on a half-shown box still shows the caret alone.

## HexView

**Only the lines the viewport reaches are shaped.** The view states its whole height through
calculateContent, so the scroll box it stands in takes the range from the layout and
nothing holds a second copy of it. The paint then draws the lines its viewport covers and
touches no others, which is what lets the view hold a buffer larger than a shaped document
could be.

**Every column is a count of characters.** One measurement of the monospace style per layout
pass gives the cell, and a line is drawn as **one** text laid out on that same grid - so where a
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

## MenuItem

**A line shows its command's selection** (2026-11-11, found by the MillScene showcase's theme
list). MenuItem took ButtonBase's ShowSelectionOnSurface::No, so the line of a command answering
selected drew like every other line, and the picked theme could not be told in the list. No menu
had shown a selected command before: the Themes app presents its selected commands as radio
buttons. A group's line reads as selected while its list is up - Control's visual state counts a
dropped-down control as selected - so it keeps that surface while the pointer is in the list.

## MenuListBuilder

**The builder is the menu's, not the action's** (2026-11-11, for the MillScene showcase's theme
picker in a list beside its Theme line). An Action lives in ClaFi.Core.Foundation and cannot name a
Menu, so a list of controls could not ride an action the way its items do. A group line made from a
builder carries no action: its words and icon come as a line's from add do, and it is enabled as
any button is. Menu::openGroup asks every group line for its list the same way - a line made from
an action builds it from the action's items.

**Left asks the navigator first.** Menu::nestedKeyDown closed a list on Left before the navigator
saw the key, which was right for a column of lines and wrong for the picker's lanes, where Left
moves to the lane before. It now runs the navigator and closes the list only where the focus
stayed put.

## CompletionList

The reading is compared with the last before any row is rebuilt, so typing in a routine's body
rebuilds nothing.

## Tree

### One line for every row

2026-10-12. SeeDocs' tree over a project of 31,925 types, 3,864 modules and 718 folders holds
about 36,500 rows fully expanded, and every layout pass of the form measured each of them - an
item and a node header alike gathered its text and measured it unbroken for its width. The
layout cache keeps 4,096 entries, so a tree past that loses the rows it measured first before
the pass ends, and by the cache's own rule every pass shaped nearly every row again. Every
answer was one line of body text, inside the button's `MinSize` once the padding is on, and
the width was thrown away, the bar's `PreferredSize` standing in its place.

`Tree::calculateChildren` measures one line of body text while the tree's width is given, and
`TreeItem::measureText` and `TreeNodeHeader::measureText` answer it. The bar's width was not
"given" to the framework until `isWidthGivenFromOutside` learned that a stated preference ends
the walk as given - see Item-Containers.

`Expander` builds its header, so the node's header is a type the node names through the
protected constructor taking `std::type_identity<Header>`. The header reaches its tree through
its parent, the node that built it.

The tree states `WordWrap::No`. A scroll box holding a wrapping body in a given width caps the
body at the viewport and asks for another pass whenever the viewport moves, so a splitter drag
laid the whole tree out twice per move once the bar's width counted as given. No row breaks at
any width.

What a pass still costs over such a tree is the controls themselves: a node is four (the
expander, its header, the mark, the rows stack), each measured and placed on every pass.

## PictureView

**Only what is on screen is drawn, and sampled once.** The part of the picture on screen is
sampled at the zoom into a cache, and a paint copies the cache's rows into a staging view, so a
paint costs the size of the view and never the size of the picture or the zoom, a zoom costs one
sampling of the view, and a pan samples only the strip it uncovers.

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
