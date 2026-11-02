# TextEngine Types

The author's side of the footnote of the same name.

## FlexSpace

**The room is taken less the fit tolerance.** The width is stated by building the line a second
time, and a line filled to the box exactly can come back from that build a hair wider than the
box - which a wrapping text breaks at the gap, and a one-line box then collapses.

**The gap stands in the text as U+FFFC**, a contingent break, which both breakers may cut on either
side of. Cut after, the gap trails its line and fills nothing, and the words land at the start of
the next line; cut before, it leads the next line and fills it. A greedy breaker cuts after it
while the gap's floor fits and before it once the line is full, so the words moved between the two
edges as a cell narrowed. Each backend states the rule to its breaker: DirectWrite through
CustomInlineObject::GetBreakConditions - can break before, may not break after - and the Linux
layout by writing LINEBREAK_ALLOWBREAK before the gap and LINEBREAK_NOBREAK on it over what
set_linebreaks_utf32 answered.

**A stated break width fills in the Calculate phase** because TooltipLabel's own layout never
enters the Paint one - WithTextLayout states no phase, and the over-text hint is measured and
drawn from the lines broken at the cell's width. Filled only in the paint, the hint measured the
gap at its floor and showed the words at the left.

## LineFade

The collapse line's overrun is its own width and everything appended to it, less the room its
paragraph has, whatever its alignment. A centred or right aligned collapse line moves left by the
collapse offset to make room for what follows, and no further than the leading edge, so either
the whole run fits or its start is held at that edge and all of it past the room is overrun. The
appended width is the layout's own sum - line widths without their trailing whitespace, and a
quarter of a line's height between paragraphs - so it can come out a space per lifted line short
of what DirectWrite and HarfBuzz draw.

## TextRenderMode

**The whole path stands on** CLAFI_TEXT_MOVING, in Source/Y-Core/System/Switches.h, and it is 0.
At 0 this type, Control::textRenderMode, the mode parameter of TextEngine::drawText and
TextLayout::draw, and everything under them down to the backends' raster state are left out
of the build, and every run is drawn the way Static asks. Nothing moves text at 0 either: the
press animation scales a control's surface and children, and PaintEvent draws every control's
text without it, where the text was laid out.

## Text

Runs over the plain text, each a length and four indexes: the style in the text's own sheet, the
link and the anchor in its table of names, and the object. Index 0 is none in all four, so a text
written in no style caches none. The sheet and the names only grow - an edit that drops the last
run in a style leaves the style cached, and comparing and hashing read what each run draws rather
than its indexes. A run is found by walking from the nearer end; a wrapper closing walks back from
the end of the text it is writing, so writing stays linear.

Until 2026-11 a text was its plain characters and a vector of push and pop markers. It allowed an
unclosed marker, every edit ran pair matching and annihilation over the markers, and baking walked
a stack per category with the format as each stack's floor. Step 1 of the move put the wrappers on
the marker stream; step 2 put runs behind them. The project doc claude/text-runs.md holds the
decisions.

**An undo has to be exact**, and laying a text under the look where it goes is not: a plain word
taken from between two red ones would come back red. So replaceText nests what goes in, and
restoreText - which TextHistory reverts with - puts every run and paragraph back as it was. A redo
replays replaceText in the surroundings the undo restored, so it lands where the edit did.

**A link's ink is not a field of the run's style.** BakedText reads it between the run's ink, its
own or its base's, and the format's, so text typed at a link's end, taking the run's style and not
the link, does not come out in the link's ink. A wrapper's ink does not reach a linked run. The one
exception is a wrapper's base taken by a linked run with no base of its own: where that base states
an ink, the link's ink is written into the run's style in its place, and words typed at the link's
end take it with the style.

**A paragraph's look survives an edit inside it** - a join keeps the upper look, and a text put in
brings looks only for the paragraphs it starts. So restorableText, what TextHistory records, carries
no look for the paragraph a range starts in, and restoring has none to put back. A look for it would
have to mean two things: the paragraph the range stands in, and the paragraph that starts where two
removals are joined - merging forward deletes into one undo step appends the second removal at a
paragraph start, and Text's operator<< states a look at 0 there, over the next paragraph's.

**A copy is another matter**, and selectedText carries the look of a paragraph starting at the
range's start. Until 2026-11-02 the two were one function, and a Select All copied out of the
TextEngine showcase lost the title's centring - WhatsClip's preview showed it on the left while the
subtitle, the second paragraph, stayed centred. Oj, 18:35: "1 a" (selectedText for copies,
restorableText for undo) and "2 b": a paragraph an edit empties takes the pasted text's first look,
so the Ledger pasted into an empty box keeps its title centred. Undo puts the old look back through
the look TextHistory keeps beside the change (Change::startParagraphLook, handed to restoreText) -
recorded only where the edit changed it, which only an emptied paragraph does.

**Paragraph tags stream into the text.** Step 1 put paragraph properties in a wrapper,
ParagraphText, which owned whole lines and kept two flags beside the runs: the line end it still
owed, and whether a text began with one. Oj, 2026-11-02: "They should not be there at all. Also
ParagraphText is not needed. Also TextAlign and other paragraph tags should stream to the Text
directly like before - and be in effect till replaced", then "1 a; 2 a; 3 a": a tag written
mid-paragraph states the whole paragraph; a text written into another lays its paragraph looks over
the one in effect; a file writes P where the look changes. The look in effect is the last
paragraph's, and a line end gives the next paragraph the same - the Enter rule - so nothing sits
beside the runs. A caller writes its own line ends, and states a value again to end one.

**Words after a line end put in take the style after the edit**, as typed words at a paragraph's
start do, because TextHistory merges a typed run that holds a line end into one step and a redo
replays the step whole.

## TextFormat

The layout is handed it beside the text, and BakedText::rebuild reads it under every run - so a
control's own text stays named in the gather and recognised by its stamp, and nothing the text
says is copied to put the format under it.

The layout compares the format it is stated next against the copy it keeps. The engine's cache keys
on the format's hash beside the text's, and confirms a hit against the entry's copy.

## StyleSheet

Asked for 2026-11-02, for SeeDocs projects whose articles, footnotes and sidebars take their looks
from a sheet the project keeps. Oj settled the shape in five answers (the project doc
claude/style-sheets.md holds them): the reader states the sheet through a scope, a sheet may restate
a framework name, an edit is live, a removed style is kept for the texts naming it, and a paste
makes the styles the target lacks the text's own.

The same evening (19:40 on) Oj took the model further: "I would only keep TextStyle. And it cannot
be constexpr then", then "I would also only keep StyleSheet. Meaning no values in runs, and no
arrays or styles declared alone as consts. Everything in StyleSheets", and "1 b; 2 a; 3 a": each
Text has a small sheet of its own over the sheet it was built with, where cache-by-values lands;
the sheet is given at construction and defaults to the application's, so a wrapper carries a name
looked up as it is written; a name written in code that nothing holds sets the words in no style
and is logged. TextLook, the constexpr TextStyles, NamedIn, styleFor and sheet assignment went with
that step, and so did the TextBox styleSheet property - a paste lands in the box's text, which
knows its sheet.

**A style keeps its address.** A run's style is an entry in its text's sheet, and that entry's base
is a style somewhere up the cascade, held by address. So a sheet holds each style through a
unique_ptr and never frees one while it lives - a removed style is marked and kept. takeStyles
matches styles by name instead of replacing them, which is what lets a project read its sheet again
over the one its open pages are built over. A sheet's copy constructor maps a base among the
other's own styles to the copy at the same position, which is what a Text copy needs: its runs index
its sheet by position.

**A text's own sheet is the run table.** cache answers the position of an unnamed style stating
those values, so two runs drawn alike share one, and a run holds that position. Named styles there
are the ones the text carries for its words: a style put in from a text over another sheet, where
the receiving text reaches nothing of that name. A Text is assigned by copy and swap, since a
sheet is never assigned.

**A style goes by its name between texts.** A run taken into another text keeps its fields, and
its base is found again by name in the receiving text - its own styles first, then its sheet. Had
a pointer been kept, a text would hold styles of a sheet it is not over, which dangle once that
sheet goes and which a file could not name back. Two cases fold the base's fields into the run
instead of copying by name: a base removed from its sheet, and one standing behind a namesake.
Either way the source text's sheet no longer finds the base by its name, and copying by name would
bind the words to the other style of that name - a removed Quote and the Quote stated after it
would draw alike.

**Names in a sheet behave as they do in a file.** A file states a base by name, and a reader finds
the sheet's style of that name before its parents'. Had the sheet kept a base pointing at the
framework's Code while it also held a Code of its own, the next read would base it on the sheet's
instead, and a round trip would change what the style draws. So set normalises a base to the
sheet's namesake, and a style restating a parent's name takes over the bases pointing at the
parent's. The loop check follows from that: a restatement whose own chain reaches its namesake
would close a loop the moment it stood in for it.

**One revision for every sheet.** A layout knows only the text's stamp and the style addresses -
neither moves when a style's fields do. So any edit moves one number, TextEngine drops its cache
when it sees it move (select and layoutGeneration both look), a layout held by a control rebuilds
through the generation it already checks, and every form re-aligns on StylesEditedEvent. A sheet
that knew its readers was the other way: it needs a registry of every text and control, for an
edit that happens while a style editor is open.

**What is an edit.** A format names its style, and is resolved at every bake, so whatever changes
the answer to a name redraws: a change to a style held, a rename, a removal, and an add under a name
one up the parents holds where it draws otherwise. An add under a fresh name is not - no text is
set in it, and a format naming it was reported as naming nothing. A text's own sheet and a sheet
being read take styles through carry, which raises nothing: adoption, a text file's own styles, a
sheet file read into a fresh sheet before takeStyles states it over the one in use. Raising there
would re-shape every window on a paste or a read. The reader adds each style after its base, then
puts them in the file's order with moveTo, which no text draws from.

**Equality wants one sheet.** Two texts are equal when their words draw alike - each field
compared as the run's chain states it - and they are over the same sheet. The sheet decides what a
run leaves to Body and what a format's name means, so two texts over different sheets can draw
apart while every run states the same; the layout cache confirms a hit with this equality.

**The package's text is over the framework's sheet.** A package is written out when another
application asks, which can be after the sheet the text was copied from has gone. addText puts the
text into one over the framework's sheet, so every style beyond the framework's becomes one of the
package text's own and nothing points into the source sheet. The same styles go beside it as a
ClaFi.StyleSheet. takeText reads the text through the carried sheet and puts it into a text over
the stated one, where each base is found by name - the stated sheet's where it has the name, the
text's own copy where it lacks it. A sheet's restatement of a framework name is not carried, nor
is an application's Body: the target finds those names in its own sheet, as the paste did before
texts had sheets of their own.

**A run's look in a file is #n.** Step 2 named such entries after their base (Code 1). Once a text
had named styles of its own that went by name between texts, two files' Style 1 met by name in one
box after two pastes, and the second paste took the first one's look (the review on 2026-11-02 found
it). The file has to tell a look from a carried style, so a look is # and a number, which no style
name starts with, and reads back as the run's style, named none.

**Equality compares the named styles a text carries.** A text's own Body, or an own style a format
names, draws without any run stating it, so two texts differing there are not equal - the layout
cache confirms hits with this equality.

**Script levels.** A style's own levels stand inside its base's. A wrapper laid over words whose
base states levels would leave the base's levels outermost, so where the wrapper adds any, such a
base is folded into the run: its fields and levels become the run's own.

**The base font.** The platform layouts fell back to the framework's Body for an empty paragraph
past the last span and for the leading font. That Body now depends on the text's sheet and format,
so BakedText resolves it once per rebuild (BaseFont) and the layout passes it in BuildParams; an
empty text in a box formatted as Code takes Code's size for its caret.

## Step names

A run's name is written when the list asks, from the text the run holds by then, so a word typed
a letter at a time costs nothing per key.

## Links

The native layout answers where the link's glyphs put ink between the line's own top and bottom -
INativeTextLayout::appendInkAcross, and INativeMonoFont::inkAcross for a paragraph on cells - and
each of those stretches is widened by the line's thickness on both sides and cut out. A piece left
no longer than the line is thick would read as a dot between two descenders, and is not drawn. The
ink is read off the glyph coverage the CPU path draws from, a row at a time, counting a pixel
covered past a fifth; on Direct2D the glyphs of a hovered link are rasterized into that cache for
the purpose. It is asked on every paint while a link is hovered and kept nowhere. On Linux a raised
or lowered run inside a link is measured where its line places it, since a layout is told about
scripts only when it draws.

Two links that meet and name one target carry one name index, and BakedText spans neighbouring
runs of one target as one link. The ink is k_linkInk in TextEngine.Text, and goes into the theme with the other
text inks.

## Anchors

**A field of the run rather than a point**, so it rides every edit a look does: an edit inside it
moves its end, a deletion of the whole of it takes it out, and an undo puts it back where it
stood, as it does for a link. A point would need rules of its own for all three.

## MonoFont

ASCII is answered from a table and everything else from a map, so a document of source pays one
lookup per character.

The draw walks only the characters whose cells the clip shows, and a cell either side for a
glyph whose ink hangs past its own. The first of them is found by scanning the row for tabs,
the stretch between two of them being a column per character, so a long line costs a scan
for one character and what is on screen, not a lookup and a glyph per character of its
length. cellLeft and the bands under the hits and the selection are counted the same way.
Inside the shown span, glyphs are gathered into a run for as long as the colour holds and no
tab breaks the cells, and each run is handed to the platform with the colour's brush - a
gradient to transparent at the box's right edge where the line fades, over the span
lineFadeSpan answers for the row's overrun.

The fonts live in one table for the application, found by family, size, weight and style;
a request is resolved once, negative answers included.

## Mono paragraph

What it buys is the shaping: a document of source is one pass over its characters instead of a
native layout per paragraph, and its footprint is a paragraph record per line. The pixels are the
ones the native layout would draw - DirectWrite's natural measuring mode places a monospace run at
column times advance already, the tab stops are the same, and the glyphs reach the same draw
primitive.

## Paragraph placement

**One line, or a row of cells, is placed by the layout.** The native layout is never told the
alignment and holds the line at the leading edge; paragraphOffset moves it by the alignment's share
of the room the line leaves in the placement width, or not at all where the paragraph is wider than
that width. Most text is this: a label, a caption, a cell, every text that does not wrap.

**Several lines are aligned by the native layout**, once, in the width they were broken at
(alignedWidth), because only it can place each line. It is told after its metrics are read - see
shapeParagraph - and moved as a whole from there: half the difference between the two widths for
Center, all of it for Right. That holds because the lines fit both widths or neither: a wrapped
text is never placed wider than it was broken at nor narrower than its widest line - see
TextLayout::acceptsWidth and placementWidth.

**Justified lines are stretched to the width**, which no move reproduces, so a justified paragraph
of several lines is told every width it is placed in (alignParagraph) - the one setMaxWidth left.

Every reader of the native layout's coordinates - the draw, both hit tests, charRect and the ink a
link's underline is cut around - adds paragraphOffset. A collapse line a layout holds at the
leading edge is moved by the draw's origin, since the layout moves no line left of where it put it.

## Layout cache

TextEngine::select holds one TextLayout per text per width it was broken at, keyed on what the
text says - see LayoutKey - and confirmed against the copy the entry keeps. A measurement and
the paint after it ask for the same entry: the paint's box is the measured size, which
acceptsWidth takes for the width the lines were broken at.

**What the pass asked for is kept.** Every entry is stamped with the pass it was last asked for in,
and FormBase::reAlign starts a pass through beginPass. A sweep drops only the entries no pass
since the one before the current asked for: the measurement and the paint that draws it are
one pass, and a tooltip or a popup laying out between them is a pass of its own, which the
second generation covers. A cache swept by use alone lost the top of a page before its first
paint - a SeeDocs page measures the surface tree and every cell on the page before it paints,
many more than the sweep kept, and the rows a window shows are the first a pass measures. The
paint then built those layouts again, at the box's own width, and a text broken at a width equal
to its own measured width fell to a second line on the ULP the rect round trip loses.

**The sweep runs at a threshold**, k_sweepLayouts, and the threshold moves with what the sweep
left: the next one waits for the count to grow by half of k_sweepLayouts again. A page asking
for more entries than that keeps every one of them, and is not swept on every insert for it.

**Past** k_maxCachedLayouts a pass is holding more than the cache will carry for it, and its own
entries go too, down to the most recent half by use. A list of a million fixed-size items never
gets there, since a fixed size is not measured; a page of that many texts churns the entries
its paints need, as every page did before.

## HeldLayouts

Which paragraphs of a TextLayout hold their native layout while k_releaseNativeLayouts is on -
the switch at the top of TextEngine.Layout.cpp - and which of them are given back first.

**Every paragraph is still shaped once**, when the text is. Its bounds and its lines go into the
paragraph record and the pool, and those are what the fit to the box, the extent the bars range
over and a caret's line and column are read from. What is given back is the native layout behind
them: the object that draws the paragraph and answers a hit test or a character's rect.

**A paragraph is built again when it is drawn or asked about**, by shapeParagraph from what it was
first built from - its slice of the baked text, the span cursors found by bisection at its start,
the width the text was broken at and the event phase it was shaped in, a change of which
reshapes the text. The width ensurePlacement told the held layouts to place their lines in is
stated to the rebuilt one after the build, the order a held one was told in.

**What is kept** is the paragraphs nearest what the last draw showed, counted in paragraphs, up to
k_heldLayoutBudget. A shaping keeps the first ones: all a text shorter than the budget has, so a
label is never built twice, and what the top of a longer one shows. An edit keeps the last
paragraphs it shaped, where the caret is, and takes the last of them for the view until the next
draw shows where the view went. The drawn range is kept whatever the budget says, so a
view taller than the budget holds what it shows. A draw gives nothing back while it runs, since
a row it has not reached yet could be the one to go, and trims once it is over; a question
outside a draw makes room before it builds, so the paragraph it answers about is never the one
given back.

**What it costs** is a native build, with the allocations inside it, for every paragraph that comes
into view: a wheel notch builds a few, a thumb dragged across the document a screenful per frame.
A paint that shows what the last one showed builds nothing.
