# TextEngine Types

The author's side of the footnote of the same name.

## FlexSpace

THE ROOM IS TAKEN LESS THE FIT TOLERANCE. The width is stated by building the line a second
time, and a line filled to the box exactly can come back from that build a hair wider than the
box - which a wrapping text breaks at the gap, and a one-line box then collapses.

## TextRenderMode

THE WHOLE PATH STANDS ON CLAFI_TEXT_MOVING, in Source/Y-Core/System/Switches.h, and it is 0.
At 0 this type, Control::textRenderMode, the mode parameter of TextEngine::drawText and
TextLayout::draw, and everything under them down to the backends' raster state are left out
of the build, and every run is drawn the way Static asks. Nothing moves text at 0 either: the
press animation scales a control's surface and children, and PaintEvent draws every control's
text without it, where the text was laid out.

## TextFormat

The layout is handed it beside the text, and BakedText::rebuild reads it ahead of the text's first
marker - so a control's own text stays named in the gather and recognised by its stamp, and nothing
the text says is copied to put the format in front of it.

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
gradient to transparent over the box's right edge where the line fades.

The fonts live in one table for the application, found by family, size, weight and style;
a request is resolved once, negative answers included.

## Paragraph placement

ONE LINE, OR A ROW OF CELLS, IS PLACED BY THE LAYOUT. The native layout is never told the alignment
and holds the line at the leading edge; paragraphOffset moves it by the alignment's share of the
room the line leaves in the placement width, or not at all where the paragraph is wider than that
width. Most text is this: a label, a caption, a cell, every text that does not wrap.

SEVERAL LINES ARE ALIGNED BY THE NATIVE LAYOUT, once, in the width they were broken at
(alignedWidth), because only it can place each line. It is told after its metrics are read - see
shapeParagraph - and moved as a whole from there: half the difference between the two widths for
Center, all of it for Right. That holds because the lines fit both widths or neither: a wrapped
text is never placed wider than it was broken at nor narrower than its widest line - see
TextLayout::acceptsWidth and placementWidth.

JUSTIFIED LINES ARE STRETCHED TO THE WIDTH, which no move reproduces, so a justified paragraph of
several lines is told every width it is placed in (alignParagraph) - the one setMaxWidth left.

Every reader of the native layout's coordinates - the draw, both hit tests, charRect and the ink a
link's underline is cut around - adds paragraphOffset. A collapse line a layout holds at the
leading edge is moved by the draw's origin, since the layout moves no line left of where it put it.

## Layout cache

WHAT THE PASS ASKED FOR IS KEPT. Every entry is stamped with the pass it was last asked for in,
and FormBase::reAlign starts a pass through beginPass. A sweep drops only the entries no pass
since the one before the current asked for: the measurement and the paint that draws it are
one pass, and a hint or a popup laying out between them is a pass of its own, which the
second generation covers. A cache swept by use alone lost the top of a page before its first
paint - a SeeDocs page measures the surface tree and every cell on the page before it paints,
many more than the sweep kept, and the rows a window shows are the first a pass measures. The
paint then built those layouts again, at the box's own width, and a text broken at a width equal
to its own measured width fell to a second line on the ULP the rect round trip loses.

THE SWEEP RUNS AT A THRESHOLD, k_sweepLayouts, and the threshold moves with what the sweep
left: the next one waits for the count to grow by half of k_sweepLayouts again. A page asking
for more entries than that keeps every one of them, and is not swept on every insert for it.

PAST k_maxCachedLayouts a pass is holding more than the cache will carry for it, and its own
entries go too, down to the most recent half by use. A list of a million fixed-size items never
gets there, since a fixed size is not measured; a page of that many texts churns the entries
its paints need, as every page did before.

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
reshapes the text. The width ensurePlacement told the held layouts to place their lines in is
stated to the rebuilt one after the build, the order a held one was told in.

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
