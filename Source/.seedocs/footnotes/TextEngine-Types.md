# TextEngine Types

The words that do not fit above a declaration.

## TextStamp

**Two stamps are equal only for one ControlText with no write in between.** Whether two texts say
the same thing is Text::operator==.

**Id zero is no stamp** - the answer for a plain Text. Two such stamps compare equal, so test
named() first.

## EditKind::Replace

**Always a step of its own.** The user drew the boundary it acted on, so it joins no run and none
joins it.

## EditKind::Automatic

The indent a line break brings, the block a paste is moved to, a closer word realigned.

**It joins the step the last edit made** and is undone with it. After a caret move, an undo or a
redo it is a step of its own.

## TextLineColumn

**A line is a paragraph** - what the newlines cut the text into. A line wrapped across several rows
has one number, and the column counts along the whole of it. For the row, see TextLayout::rowStart.

## FlexSpace

**minWidth is the gap's width on a full line**, one with no room to spare. To hold the two sides
apart there, state minWidth rather than writing a space beside the gap.

## LineFade

lineFadeSpan turns `overrun` into the width of the fade.

- **The collapse line** - the last line that fits the box's height - carries the lines below it
  along itself. Its overrun counts all of them, drawn or not.
- **A line its native layout trimmed** fades over the whole fade width, since the part it left out
  was never measured.

## lineFadeSpan

**The fade takes the line's colour to nothing at the edge.** It covers as much of the line as runs
past the edge, up to `fadeWidth` - the draw passes k_lineFadeWidth, scaled.

## TextRenderMode

The engine turns the case into raster settings - the case is all a control decides.

- **Static** - text that cannot move. It gets every setting that makes a glyph crisp.
- **Movable** - text that can move, at rest. It gives up those settings for its whole life, since
  switching them around a movement would visibly change the text as it starts and stops.
- **Moving** - text whose position is changing. As Movable, with outlines filled as geometry
  instead of cached glyphs. Outlines bypass the glyph cache, so only text in motion pays for them.

**Compiled in while CLAFI_TEXT_MOVING is 1**, in Switches.h. At 0 every run is drawn the way Static
asks.

## ControlText

**A Text with a stamp** - see TextStamp. A reader tells an unchanged text from a changed one
without reading it. A copy or a move is a new text with an id of its own.

**Every write through ControlText moves the stamp on.** A write through a `Text&` bypasses it,
which is why TextHistory takes a ControlText.

## TextFormat

**It holds the values a text starts from** - a colour, a style, a size, a family, bold, italic, an
alignment, an indent, a line spacing. A script is left out: how far it is raised depends on the
size in force where it opens, so its order among the other values would change what is drawn.

**The text's own markers override it.**

- A push the text makes is read after the format's.
- A paragraph property is read at the paragraph's newline. One the text states replaces the
  format's for its whole paragraph and every paragraph after it.
- A pop the text has no push for leaves the format's value in force - the format is the floor of
  every stack. For a text whose pushes and pops pair up, the format acts as if written at index 0.

**It is presentation, kept apart from the text.** selectedText, the clipboard, the history and a
document never see it. A control with a format and no text is blank.

**A layout keeps a copy**, so the caller's format may change or go. An empty format and none are
the same answer.

## TextFormat operator<<

**Values are kept in the order of their kinds**, whatever order they came in. Two formats saying
the same thing are equal and share a cache entry.

**Bold and italic stay once stated** - no later value takes them off.

**Values only.** A character, a pop, a link or an anchor is refused at compile time. A TextOp
other than PushBold or PushItalic throws std::logic_error.

## TextEdit

**It is all TextLayout::applyTextEdit needs** to tell which paragraphs moved and how far, so it
carries no text.

## EditSelection

**The selection part of EditProps** - the range, caretOnLeft and affinityTrailing. targetX stays
out: it belongs to a run of vertical keys, and an undo does not restore it.

## EditKind

**A run is one step to take back.** Typing, or presses of one delete key, join the open run of
their kind. A run ends where white space and a word meet.

## TextHistory

**A step is a delta**: what it took out, what it put in, and where the caret was. A long text costs
no more to edit than a short one.

**Every edit goes through apply().** Each apply, undo and redo checks the text's length, and the
history empties itself when another route changed it. A rewrite to the same length goes unnoticed.

**A step may state where it lands** - `after` on apply. A redo selects that range, and the step
joins no run and none joins it. A box moving several lines uses it, so the lines are selected
again after a redo.

### Step names

writeUndoStep and writeRedoStep write a step's name:

- A step given a name in apply's `what` shows that name - Paste, Cut, Complete [WriteLn],
  Indent [3 lines].
- An unnamed step is Type or Delete, then what went in or came out, quoted in the accent ink.

The quote is the first line of that text, cut at 24 characters, with an ellipsis where more
follows. A step of blanks alone is counted, since blanks draw nothing - space, 4 spaces, line
break, 2 tabs.

## ColorOverlay

**Colour only, asked at draw time.** The shaping never sees the overlay, so a layout takes a new
one without shaping again - a repaint is the cost. A weight or a face would change the shaping, so
an overlay cannot state one.

**Only drawn paragraphs are asked**, so a document taller than its viewport pays for what is on
screen. CodeBox is the overlay the framework ships.

## Searcher

**The application keeps it** while any SearchResult points at it. One searcher can serve several
results, one per page. The count, the current match and the steps are SearchResult's.

**The paint asks it.** Every text painted inside a result's container is searched a paragraph at a
time as it is drawn, so text out of view costs nothing. The innermost result's searcher is in
force. The places show on the FoundText band, and the text is unchanged. A tooltip, a window of its
own, shows no places.

- The text is split as SearchSplit says, and case is folded in the user's locale.
- A word is found anywhere, inside a longer word too.
- Overlapping places merge into one.

**New words repaint nothing.** Whoever sets them invalidates what shows them, and revision tells a
result to read its texts again.

**Ask it from one thread** - findIn is const, but folds into a buffer the searcher keeps.

## Links

A link is the text between a PushLink and its PopLink. The target is whatever the push states - a
URL, a path, or a word the application answers.

**In a format string** the tags are `[link target]` and `[/link]`. The target is everything after
`link`, spaces included, so it cannot hold `]`. A document and the clipboard carry any target
whole.

**Typing at a link's end goes outside it**, where any other run takes it in. Typing over the last
word of a link replaces part of it, and the new word belongs to the link.

**Two links that meet and name one target are one** - pointed at and underlined whole, and typing
where they meet goes inside.

**A link is drawn in the accent ink** where no colour is pushed inside it. A colour pushed before
the link gives way to that ink, and one pushed inside it wins.

**The underline shows on hover.** The control holding the layout names the hovered link with
TextLayout::setHoveredLink, and the layout underlines it:

- one segment per line, under the glyphs that line shows, in the colour of its first glyph
- over the glyphs and any selection band
- with a gap wherever it would cross ink
- unfaded on a line the box cuts

**TextLayout::linkAt** names the link under a point. Past the end of a line, between lines and
below the text there is none.

## Anchors

An anchor is the text between a PushAnchor and its PopAnchor, named by the push. It draws nothing.
A link reaches it with # and the name:

    [anchor clue-17]Clue 17[/anchor]
    [link #clue-17]Clue 17[/link]

**A TextBox following the link selects the anchor's text** - see TextBox::goToAnchor. Of two
anchors with one name, the first in text order is reached.

**Edits treat it as a run**: an edit inside moves its end, deleting all of it removes it, an undo
puts it back, and text typed at its end joins it.

## MonoFontRequest

**`size` is the design size times the scale.** **`family` is what the text names**, generic
families included, resolved as for a native layout - so a paragraph on cells gets the same face.

## INativeMonoFont

**The platform's part of a monospace font.** Which characters have a cell, the columns, the tab
stops, the colours, the bands and the fade are MonoFont's, written once for both platforms.

**The cell is the space's advance**, the number the tab stops are measured from. glyphOf answers
a glyph only when its advance equals the cell exactly - on DirectWrite in the face's design units,
on FreeType as the same scaled float from the same design advance.

**A proportional face is asked once.** createNativeMonoFont answers null for it, the core keeps
that as a negative entry, and its paragraphs go straight to the native layout.

## MonoFont

**A character is one column.** A tab moves to the next multiple of k_tabStopSpaces columns, at
least one column on, as the native layouts place their stops.

**extent answers two widths.** `inkWidth` leaves out trailing spaces and tabs, and judges a line's
fit and fade. `width` keeps them, and is what the paragraph measures.

**glyphOf refuses what a cmap lookup alone cannot lay out**, whatever the face holds: control and
format characters, combining marks, the right-to-left blocks, the scripts that reorder or stack,
surrogates and everything past the basic plane. A refusal costs the paragraph its cells and
nothing more, so the list errs toward refusing.

## Mono paragraph

**A paragraph on cells is one row of columns**, shaped in one pass over its characters with no
native layout. A paragraph goes on cells when:

- one monospace font covers its whole range, with no change of family, size, weight or style
- it is plain text, with no inline object and no script
- its alignment is Left, Center or Right
- every character has a cell in that font, or CellFallback is MissingGlyph
- where the layout wraps, its ink fits the width the lines break at

Otherwise it is shaped natively. The choice is per paragraph, so one line of Arabic in a source
file costs that line alone.

**The pixels match the native layout's**, ligatures apart: a glyph per character applies none.
Consolas and DejaVu Sans Mono have none; Cascadia Code does.

**CellFallback decides a character without a cell** - set on a layout, or with
WithTextLayout::setCellFallback.

- `Native`, the default, shapes the paragraph natively, where another font can draw the character.
- `MissingGlyph` keeps it on cells and draws the font's missing glyph, so any text costs one pass
  over its characters - the answer for bytes that are not text.

## Paragraph placement

**Lines are placed across a width** - the box, a stated break width, or the block a
HorizontalTextAnchor moves. Placing shapes nothing. Only a justified paragraph of several lines
tells its native layout the new width, through setMaxWidth.
