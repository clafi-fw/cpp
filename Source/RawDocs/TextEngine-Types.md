# TextEngine Types

The words that no longer fit above a declaration.

## TextStamp

WHICH text, and WHICH state of it. Two stamps are equal only for one ControlText that has not
been written to in between: an id is never reused, a copy being a new text with a new one,
and a revision only moves forward. What a stamp cannot say is that two texts SAY the same
thing - that is Text::operator==.

An id of zero is no stamp at all: what a plain Text answers, having no earlier self to be
told from. It matches nothing, itself included, so a reader tests named() before comparing.

## EditKind::Replace

A paste, or anything written over a selection. Stands on its own: the user drew the
boundary it acted on, so it is a step of its own to take back.

## TextLineColumn

Where a position stands in the text as a reader counts it, both numbers from one. A LINE is
a paragraph - what the newlines cut the text into - so a line broken across several rows
carries one number and the column is the offset along the whole of it. Which of those rows
the position fell on is a different question, asked of the layout - see TextLayout::rowStart.

## TextOp::PopScript

One pop for both, because one stack holds them: a script closes the nearest script
still open, whichever of the two opened it.

## TextRenderMode

What a control's text has to survive, stated as three cases rather than as raster settings.
The settings are the answer and live in textRasterParams below; this is the question, and it
is the only part of the decision a control is asked to make.

Static is text that cannot move. It gets everything that makes a glyph crisp, because nothing
will ever ask it to be anywhere but where it is.

Movable is text that can move, while it is not moving. It gives up the crispness, and it does
so permanently rather than at the moment movement starts. Every setting that buys crispness
buys it by letting the glyph's raster depend on sub pixel position, so turning them on and off
around a movement makes the text visibly change at both ends of it - at rest, where nothing is
moving to cover the change. A control whose text can move holds one appearance for its whole
life, and that is what Movable names.

Moving adds only what matters while the position is actually changing: outlines filled as
geometry instead of a glyph rasterized afresh at every frame's sub pixel position. That is the
expensive half - outlines bypass the glyph cache - so it is not paid by a control that is
merely capable of moving.

## ControlText

A text a control keeps: written once or written often, and asked about on every measurement
and every paint for as long as the control stands. It is a Text and is read as one; what it
adds is a stamp, so a reader holding the last one can tell an unchanged text from a new one
without reading either of them. A Text passed as an argument, built for one paint or read
out of a file has no earlier self to be told from and carries none of this.

EVERY ROUTE THAT WRITES IS HERE, and each one moves the stamp on. Reached as a Text it is
data like any other, so a caller that writes through such a reference writes behind the
stamp's back - which is why the machinery that edits a control's text, TextHistory above
all, takes a ControlText and not a Text.

## TextEdit

What one edit did, stated in the coordinates of the text BEFORE it: the range taken out and
how many characters went in. It is the whole of what a layout needs to tell which paragraph
moved and by how much, which is why it carries no text of its own.

## EditSelection

Where the caret stood, so that an undone step puts the user back where that step found
them. This is the caret half of EditProps and no more: targetX belongs to a run of
vertical keys and the hit list to the last search, and an undo restores neither.

## EditKind

What made an edit, which is what decides whether it joins the one before it. A run of
typing and a run of one delete key are each a single step to take back.

## TextHistory

The undo history of one editable text: what each step took out, what it put in,
and where the caret was standing.
A record is a delta. Nothing here copies the whole text, so a step costs the size
of the step and a long text is no dearer to edit than a short one.
Every edit goes through apply(), and that is what makes the history whole. A text
written to by any other route leaves every position in here naming a string that is
gone, so this is checked rather than trusted: the plain length is compared on the way
in, and a text that moved behind the history's back empties it instead of misplacing an
edit. A rewrite of exactly the same length is the case that comparison does not see.

## ColorOverlay

Colours drawn over a text's own, asked for by paragraph while the paragraph is drawn. The
overlay answers in the paragraph's coordinates, in order, without overlap, and everything the
answer leaves uncovered keeps the colour the text states - so an overlay that says nothing
about a paragraph changes nothing in it.

A DRAW-TIME QUESTION ALONE. The shaping never sees the overlay, which is what lets a layout
be handed one, or a different one, without shaping again; the cost is a repaint. It is also
what holds the overlay to colour: a weight or a face is a shaping input, and an overlay that
could state one would have to invalidate the lines it stands over.

The layout asks about the paragraphs it draws and no other, so a document taller than its
viewport pays for the paragraphs on screen. A CodeBox is the overlay the framework ships.

## Links

A link is a run of text between a PushLink and its PopLink, and the target is whatever the push
states - a URL, a path, or a word an application answers. The tags are `[link target]` and
`[/link]`, the target being everything after the command, spaces included. Fmt reads them like
any other tag, so a target holding `]` cannot be written in a format string; a document and the
clipboard carry it whole.

A LINK DOES NOT GROW AT ITS END. Every other run takes in what is typed at its end - a
character typed after a bold word is bold - and a link is the exception, because a word typed
after it is not part of where it points. Text::replaceText keeps a PopLink standing at the
insertion point in front of what goes in, when nothing is replaced. A replaced range keeps the
standing rule: typing over the last word of a link replaces part of the link, and the new word
belongs to it. The start needs nothing of its own, since a push at the insertion point already
moves behind what goes in.

TWO LINKS THAT MEET AND NAME ONE TARGET ARE ONE. An undo puts a link's last characters back as
a link of their own, standing against the rest of it. BakedText joins the two into one span, so
the link is pointed at and underlined whole, and replaceText lets what is typed where they meet
go inside, as it would anywhere else in the link. The markers are left as they stand: a later
undo is recorded against them, and a pair taken out would put a word back into the wrong link.

A LINK IS DRAWN IN THE ACCENT INK, where no colour is pushed inside it. A colour pushed before
the link opened gives way to that ink, and one pushed inside it is the link's own. The ink is
k_linkInk in TextEngine.BakedText, and goes into the theme with the other text inks.

THE UNDERLINE IS A HOVER STATE. The control holding the layout names the link under the
pointer through TextLayout::setHoveredLink, and the layout draws a line under it - one segment
per line the link reaches, under the glyphs that line shows, in the colour the segment's first
glyph was drawn in. It is drawn over the glyphs, so a selection band does not cover it. Its
place under the baseline is a share of the room the line has below the baseline, and its
thickness is the thin stroke; the font's own underline metrics are not read. It does not fade
with the glyphs of a line cut by the box.

THE LINE STANDS CLEAR OF THE INK IT WOULD CROSS, the way a browser draws an underline. The
native layout answers where the link's glyphs put ink between the line's own top and bottom -
INativeTextLayout::appendInkAcross, and INativeMonoFont::inkAcross for a paragraph on cells -
and each of those stretches is widened by the line's thickness on both sides and cut out. A
piece left no longer than the line is thick would read as a dot between two descenders, and is
not drawn. The ink is read off the glyph coverage the CPU path draws from, a row at a time,
counting a pixel covered past a fifth; on Direct2D the glyphs of a hovered link are rasterized
into that cache for the purpose. It is asked on every paint while a link is hovered and kept
nowhere. On Linux a raised or lowered run inside a link is measured where its line places it,
since a layout is told about scripts only when it draws.

WHICH LINK IS UNDER A POINT is TextLayout::linkAt. A hit test names the nearest character even
past the end of a line, so the point is held to the box that character's glyph stands in: past
the end of a line, between lines, or below the text there is no link.

## Anchors

An anchor is a run of text between a PushAnchor and its PopAnchor, named by the push, and a link
reaches it with # and the name: `[anchor clue-17]Clue 17[/anchor]` and
`[link #clue-17]Clue 17[/link]`. It draws nothing. Where two anchors share a name the first
in text order is the one answered, and Text::anchorRange walks the markers to find it - an
anchor is asked for when a link to it is followed, so nothing is kept for it in between.

A RUN RATHER THAN A POINT, so it rides the pair machinery every other run does. An edit inside
it moves its end, a deletion of the whole of it takes the pair out in the cleanup, and an undo
puts it back where it stood, as it does for a link. A point would need rules of its own for all
three. What the run also buys is text to land on: the box going to an
anchor selects it. It grows at its end the way bold does, since what is typed there is part of
what the anchor holds.

## MonoFontRequest

One face at one size, the size in the units a layout works in - the design size times the
scale, which is what the native layouts are built with. The family is what the text names,
a generic one included - the platform resolves it the way it resolves a layout's, so a
paragraph on cells is set in the face the layout would have set it in.

## INativeMonoFont

What the platform answers about a monospace face at one size, and no more: the cell width,
the height and baseline of a line set in it, the glyph a code point maps to where that glyph
is one cell wide, and drawing a run of glyphs a cell apart from a baseline origin through a
brush. Everything above that - which characters have a cell, where the columns fall, the tab
stops, the colour runs, the bands, the fade - is written once in MonoFont for both platforms.

The cell is the space's advance, the same number the tab stops are measured from, and a
glyph qualifies when its advance is that number exactly: on DirectWrite compared in the
design units the face states, on FreeType as the same scaled float from the same design
advance. A face that is not monospaced is answered with no font at all by
createNativeMonoFont, which the core keeps as a negative entry, so a proportional font is
asked once and every paragraph set in it goes straight to the native layout.

## MonoFont

A monospace font with the glyph of every code point asked so far, and the cell arithmetic of
a line set in it. A character is a column; a tab reaches the next multiple of
k_tabStopSpaces columns, always a whole one, as the native layouts place their stops. The
ink width leaves out trailing spaces and tabs, which is the width a line's fit and fade are
judged by, and the full width keeps them, which is what the paragraph measures.

glyphOf refuses what a cmap lookup alone cannot lay out, whatever the face has for it: the
controls and the format characters, combining marks, the right to left blocks, the scripts
that reorder or stack, surrogates and everything past the basic plane. A refusal costs the
paragraph the native layout and nothing else, so the list errs toward refusing. ASCII is
answered from a table and everything else from a map, so a document of source pays one
lookup per character.

The draw gathers glyphs into a run for as long as the colour holds and no tab breaks the
cells, and hands each run to the platform with the colour's brush - a gradient to
transparent over the box's right edge where the line fades. A run standing past the box, or
ending before what is on screen, is not handed over at all, which is what a long line
scrolled sideways costs.

The fonts live in one table for the application, found by family, size, weight and style;
a request is resolved once, negative answers included.

## Mono paragraph

A paragraph set in one monospace font over its whole range, carrying no inline object and no
script, every character of which has a cell, is one row of columns times cell - and holds no
native layout. shapeParagraph decides it before building anything, from the spans the walk's
cursors already stand on: a span of family, size, weight or style that ends inside the
paragraph makes it a paragraph in two fonts, and that is shaped natively, as is anything
else that fails the rule. The fallback is per paragraph, so one line of Arabic in a source
file costs that line and no other.

A wrapping layout keeps a paragraph on cells while its ink fits the width it breaks at: a
line that fits is never broken, so nothing about line breaking is decided here, and a line
that does not fit is broken natively. Justified is shaped natively; Left, Center and Right
are an offset inside the placement box, computed as the paragraph is drawn or hit rather
than told to a layout, so a box that moves costs nothing.

What it buys is the shaping: a document of source is one pass over its characters instead of
a native layout per paragraph, and its footprint is a paragraph record per line. The pixels
are the ones the native layout would draw - DirectWrite's natural measuring mode places a
monospace run at column times advance already, the tab stops are the same, and the glyphs
reach the same draw primitive - with one exception: a glyph per character means a face's
ligatures are not applied, which Consolas and DejaVu Sans Mono do not have and Cascadia Code
does.

## HeldLayouts

Which paragraphs of a TextLayout hold their native layout while k_releaseNativeLayouts is on -
the switch at the top of TextEngine.Layout.cpp - and which of them are given back first.

EVERY PARAGRAPH IS STILL SHAPED ONCE, when the text is. Its bounds and its lines go into the
paragraph record and the pool, and those are what the fit to the box, the extent the bars range
over and a caret's line and column are read from. What is given back is the native layout behind
them: the object that draws the paragraph and answers a hit test or a character's rect.

A PARAGRAPH IS BUILT AGAIN WHEN IT IS DRAWN OR ASKED ABOUT, by shapeParagraph from what it was
first built from - its slice of the baked text, the span cursors found by bisection at its start,
the width the text was broken at and the event phase it was shaped in, a change of which
reshapes the text. The box ensureBoxWidth told the held layouts about is stated to the rebuilt
one after the build, the order a held one was told in.

WHAT IS KEPT is the paragraphs nearest what the last draw showed, counted in paragraphs, up to
k_heldLayoutBudget. A shaping keeps the first ones: all a text shorter than the budget has, so a
label is never built twice, and what the top of a longer one shows. An edit keeps the last
paragraphs it shaped, where the caret is, and takes the last of them for the view until the next
draw shows where the view went. The drawn range is kept whatever the budget says, so a
view taller than the budget holds what it shows. A draw gives nothing back while it runs, since
a row it has not reached yet could be the one to go, and trims once it is over; a question
outside a draw makes room before it builds, so the paragraph it answers about is never the one
given back.

WHAT IT COSTS is a native build, with the allocations inside it, for every paragraph that comes
into view: a wheel notch builds a few, a thumb dragged across the document a screenful per frame.
A paint that shows what the last one showed builds nothing.
