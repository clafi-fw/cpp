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

**The gap wraps with the word after it.** A line may break before the gap and never right after
it, so the words it holds at a line's end stand at the end of the next line when they wrap. A
blank written between the gap and the word is a break of its own, and the word wraps alone.

**A measurement fills the gap where the width is stated.** Lines broken at a stated width - see
TextLayout::setBreakWidth - measure the gap as it is drawn, so a hint repeating a cell's lines puts
the words where the cell has them. Measured at no stated width, the gap is its minWidth.

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

## Text

**A text is its words in runs.** A run is a stretch of characters in one look - see TextLook -
with the link it goes to and the anchor it is, where it has them. The runs follow one another with
no gap, so every character has one look.

**A run names its look in the text's own table**, and a look names its style by address. The style
stands outside the text: in TextStyles, among an application's constants, or in a StyleSheet.

**A paragraph may state a look of its own**, which every line it holds reads - see ParagraphLook.

**Two texts are equal when their words, runs and paragraphs say the same**, whatever order they
were written in.

## TextStyle

**A named look, stated once and named by address.** TextStyles holds the framework's: Body, Title,
SubTitle, Heading, SubHeading, Section, SubSection, SubBody and Code. An application states its own
as constants, or keeps any number of them in a StyleSheet:

    constexpr TextStyle k_note{
        .name = L"Note",
        .base = &TextStyles::SubBody,
        .ink = InkWell::spotInk(),
    };

**A style states the fields it sets**, and takes the rest from the style it is based on. A style
based on none leaves them to whatever it is laid over.

**It outlives every text and format that names it** - a constant at namespace scope or a static
member does, and one declared inside a function does not. A style made for one call is refused at
compile time. A constant never changes; a sheet's style changes where the sheet is edited.

**A file names a style by its name.** Reading finds it in the reading sheet - see
StyleSheet ReadingScope - and then among the framework's; any other name reads back as words in no
style.

## StyleSheet

**Named styles kept at run time, any number of them.** A sheet owns its styles, and each stays at
one address for the sheet's life, so a text names a sheet's style by address as it names a constant:

    StyleSheet sheet;
    const TextStyle* note = sheet.set(TextStyle{
        .name = L"Note",
        .base = &TextStyles::SubBody,
        .ink = InkWell::spotInk(),
    });
    text << StyledText{ *note, L"Read this first" };

**The sheet outlives every text naming its styles.** A copy has styles of its own at addresses of
its own.

**A name the sheet holds is the sheet's style.** find answers the sheet's style of a name, else the
framework's. A style named as one of the framework's stands in for it: everything read through the
sheet takes it, and so does every base in the sheet - `Name=Code` with an ink is the framework's
Code in that ink, for every style based on Code. styleFor answers what the sheet sets in place of a
framework style, the way a builder of pages takes its looks:

    StyledText{ sheet.styleFor(TextStyles::Code), name }

**A base is one of the framework's or one the sheet lists.** A base leading back to the style is
refused, and so is one reaching, through another style, the framework style it stands in for.

**Assigning a sheet keeps every style the two name alike where it stands**, with the other's fields,
so a sheet read again over the one in use keeps its texts' styles. A style the other lacks is
removed, as remove says.

**An edit redraws.** A change to what a style the sheet already holds draws - its fields, its base,
a style standing in for one that others were based on - moves StyleSheet::revision on and raises
StylesEditedEvent, and every window lays out again. A style added is no edit: no text names it yet.
Nor is a rename, since a text draws by address - unless the new name makes the style stand in for
one of the framework's, which can change what it and the styles based on it draw.

**A field one style lends a run set in another is copied into the run** - see TextLook - and an
edit of the lending style does not reach that copy.

**NamedIn gathers the styles a text is set in beyond the framework's**, with those they are based
on: what a copied text carries beside it - see StdActions.

## StyleSheet ReadingScope

**The sheet a name read from a file is looked up in, while the scope lives:**

    {
        const StyleSheet::ReadingScope scope{ &projectStyles };
        caption = node.get();
    }

A name is looked up in the text's own styles first, then the reading sheet, then the framework's.
Where no scope is set, the reading sheet is the application's - AppParams::styleSheet. A scope of
none reads through the application's sheet as well. Scopes nest, and each puts back the one it
found. A scope is held per thread.

**A Text is read when it is taken out of its node**, by get or getTo, so the scope has to stand
around that call and not only around the file's load.

## StyleSheet set

**States a style under its name.** The sheet's style of that name takes the fields and the base,
and keeps its address; a name the sheet does not list adds a style. The strings are copied in.

**A style named as one of the framework's and based on none is based on that one**, which is what
a file reads such an entry as. Nothing is answered where the name is empty or the base is refused.

## StyleSheet remove

**A removed style leaves the list and stays where it stands**, so a text naming it draws as it did,
and the sheet holds it until it goes. The styles based on it are based on its base instead, and one
named as a framework style that is left based on none is based on that one. A file written
afterwards names it nowhere, and a text naming it reads back in whatever then holds its name: a
style added under it, the framework's, or none.

## TextEngine layoutGeneration

**What a layout held outside the engine's cache checks** to learn that a build made before it would
come out differently now. invalidateLayouts drops the cache and moves it on - for the font table
above all - and so does an edit to any style sheet. An owner reading a number other than the one it
built at invalidates its own layout.

## TextLook

**How a run of words looks**: the style it is set in, the fields it states over that style - a
family, a size, a weight, a slant, an ink - and the script levels it stands in, the outermost first.

**The inner wins.** A wrapper laid over words fills only the fields they leave unstated, by
themselves or through their style. So the family in
`FamilyText{ L"Georgia", StyledText{ TextStyles::Section, ... } }` is Section's own; the face goes
inside the style, or into a style of the application's own based on Section.

**Two named styles, one inside the other**: the inner is the run's style, and what the outer states
beyond it is copied in as the run's own fields.

**Scripts compound.** A level moves the baseline by its share of the size it is entered at - the
size the words take without it - and takes a share of that size for them.

**A field is drawn from the first that states it**: the run itself, its style, the TextFormat, the
format's style, and Body. A link's ink stands between the run's style and the format - see Links.

## ParagraphLook

**How a paragraph is laid out, as far as it states**: an alignment, an indent, a line spacing.
What it leaves out comes from the TextFormat, and then from left, no indent and single spacing.

**It belongs to the whole paragraph** - see ParagraphText and Edits.

## InlineObject

**A thing standing in a text as one character**: an icon, a space, a vertical space, a flex space
or a tab stop. Its character is k_objectChar, and it is a run of its own.

## TextLayer

**What a wrapper lays over its words**: a look, a link target, an anchor name. Each wrapper fills
one of them - see ScopedText.

## ScopedText

**Words a wrapper lays a look, a link or an anchor over.** StyledText, ColoredText, BoldText,
ItalicText, SizedText, FamilyText, SuperscriptText, SubscriptText, LinkText and AnchorText are each
one, and a text is written through them:

    caption << StyledText{ TextStyles::Title, name } << L" saved "
        << ColoredText{ InkGrade::Muted, when };

**What a wrapper holds is written first, and the wrapper is laid over it** - words, numbers,
objects, other wrappers, a Text built over several statements. Where two state one field, the inner
wins - see TextLook.

**A wrapper that holds no character writes nothing.**

**A named value is held by reference and a temporary by value**, so a wrapper kept in a variable
must not outlive the named values it was given.

**A number with a format goes through std::format**:
`ColoredText{ InkGrade::Muted, std::format(L"{:.2f}", value) }`.

## ParagraphText

**Words on lines of their own.** Where the text holds words and does not end in a line end, the
wrapper ends that line first. The line end after its words is written when something more is
written, so a text that ends in one ends with no line end. A line end written right after it is
an empty line.

**Every line of the words takes the property** - a TextAlign, a ParaIndent or a ParaLineSpacing.
Two properties are two wrappers, one inside the other, and where both state one the inner wins.

**A paragraph reads its own look**, and the control's TextFormat for what it leaves out, so a
property runs on into no paragraph after its own.

**A Text that starts or ends with one keeps those words on lines of their own** wherever it is
put.

**Lines that hold no character write nothing.** A blank line is a line end of its own.

## TextPiece

**A run as a document holds it**: its words, its whole look, its link and its anchor, or the object
it is. A piece holding no words and stating a paragraph look stands at that paragraph's start.

**Text::pieces and the Text it constructs from them are each other's inverse**, and a document
reads and writes a Text through them - see Dom.

## TextRunView

**One run as Text::forEachRun hands it over**: where it stands, its look, its link, its anchor and
its object. The views are into the text and hold while it stands unchanged.

## Edits

**Words put in take on the look where they go.** replaceText lays what goes in under that look the
way a wrapper is laid over its words: what the words state stands, and the surroundings fill the
rest.

- Replacing a range: the look, link and anchor of the first character replaced.
- At a caret: the look and anchor of the character before it.
- At a paragraph's start: the look and anchor of the character after it.
- A link at a caret only from inside it, where the characters on both sides carry it.
- After a line end put in: the look and anchor of the character after the edit, as at a
  paragraph's start - so words put in at once come out as they would typed one by one.

**A paragraph keeps its look through every edit made inside it.** One joined to it takes that look:
every paragraph starting inside what an edit takes out goes, and its words read the look of the
paragraph the edit starts in. The first line of a text put in joins that paragraph the same way.

**A paragraph an edit empties takes the first look of the text put in**, over its own: a text pasted
into an empty paragraph, or over every word of one, starts in the look it was copied in. An undo
puts the paragraph's own look back.

**A line end put in splits a paragraph, and both halves keep its look.** A text put in with
paragraph looks of its own lays each over that look.

**selectedText takes the runs of the range and the looks of the paragraphs starting in it**, its
start and its end included, so a copy of whole paragraphs reads as they look.

**restorableText takes the same, less a look for a paragraph starting at its start**, and
restoreText puts such a text back as it was, every run and paragraph in the look it had. An undo
goes this way, so what an edit took out comes back exactly.

## TextFormat

**The look and the paragraph look a text is laid out over**: a style, an ink, an alignment, an
indent, a line spacing.

**The text's own looks stand over it.** A field a run states, by itself or through its style,
wins, and the format fills the rest - see TextLook. A paragraph's own look stands over the
format's the same way.

**It is presentation, kept apart from the text.** selectedText, the clipboard, the history and a
document never see it. A control with a format and no text is blank.

**A layout keeps a copy**, so the caller's format may change or go. An empty format and none are
the same answer.

## TextFormat operator<<

**Each value replaces what the format states of its kind**, and another format replaces every
field it states. Two formats saying the same thing are equal and share a cache entry.

**A size or a family comes in a style.** An application states one as a constant and the format
names it:

    constexpr TextStyle k_readout{ .name = L"Readout", .fontSize = 11.0f };
    TextFormat{ k_readout };

**Values only.** A character, a link or an anchor is refused at compile time.

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

A link is the words of a LinkText. The target is whatever it names - a URL, a path, or a word
the application answers. A document and the clipboard carry any target whole.

**Typing at a link's end goes outside it**, where any other run takes it in. Typing over the last
word of a link replaces part of it, and the new word belongs to the link.

**Two links that meet and name one target are one** - pointed at and underlined whole, and typing
where they meet goes inside.

**A link is drawn in the accent ink** where no colour is written inside it, by itself or through
its style. A colour around the link gives way to that ink, and one inside it wins.

**A link inside another is the inner's.**

**The underline shows on hover.** The control holding the layout names the hovered link with
TextLayout::setHoveredLink, and the layout underlines it:

- one segment per line, under the glyphs that line shows, in the colour of its first glyph
- over the glyphs and any selection band
- with a gap wherever it would cross ink
- unfaded on a line the box cuts

**TextLayout::linkAt** names the link under a point. Past the end of a line, between lines and
below the text there is none.

## Anchors

An anchor is the words of an AnchorText, named by it. It draws nothing. A link reaches it with #
and the name:

    text << AnchorText{ L"clue-17", L"Clue 17" };
    text << LinkText{ L"#clue-17", L"Clue 17" };

**A TextBox following the link selects the anchor's text** - see TextBox::goToAnchor. Of two
anchors with one name, the first in text order is reached.

**Edits treat it as a run**: an edit inside moves its end, deleting all of it removes it, an undo
puts it back, and text typed at its end joins it.

**An anchor inside another is the inner's**, so the outer is reached at the first stretch of its
words the inner does not hold.

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
