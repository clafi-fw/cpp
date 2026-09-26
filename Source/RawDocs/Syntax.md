# Syntax

The words that no longer fit above a declaration.

## TokenKind

What a run of source is, as far as colouring it goes - and no further. A lexer here reads
one line at a time and knows nothing of the lines around it beyond the state the line
before handed over, so a kind is what a line can tell on its own: a keyword by its
spelling, a function by the parenthesis after it, a property by the colon, a type by a
table or by its capital. What it cannot tell - a local from a member, a parameter from a
field - has no kind here, and a name nothing claims is Plain.

Plain is the kind that states no token at all. Most of a source file is Plain - the names,
the blanks, the punctuation in a language that does not colour it - and leaving it without a
token is what keeps a line's tokens to the handful that show.

## LineState

What a line starts in, carried over from the line before it. Four bytes: a mode, which of
the mode's several rules is open, and an id for a string the mode has to carry - a raw
string's delimiter - that the StateStrings hold. Mode zero is ordinary source, one is a
block comment, and everything from Mode::firstLanguageMode up belongs to the language's
hook, which is handed a line standing in one of those before anything else looks at it.

The state at the START of a line is what is kept, one per line, because it is all that has
to be: the line's tokens follow from it and the line's text in the time it takes to draw the
line, and the state the line ends in is the next line's to keep. A document is a table of
these and nothing else grows with it.

## Inks

What each kind of token is drawn in, one Ink per kind. An ink is a colour named rather than
stated - a pigment, a rule of the theme, a grade of the text - so a table holds
on either side of the theme without saying so twice. A kind left in the text's own ink is
plain to whoever draws it: it states no span, and costs what a plain run costs. The
framework's table keeps only Plain there; operators and punctuation are drawn a grade
behind the text, so the structure of a line recedes from what the line carries.

## Language

A language stated as tables, one hook and one detector, and passed by value: every member is
a view over a constant, so a Language is a few pointers and stays valid for as long as the
constants do. The tables say what a table can - the keywords, the comment and string
delimiters, which characters are operators and which punctuation, how a name no table lists
is taken for a type. The hook says what a table cannot - a raw string, a preprocessor line,
an XML tag - and is asked before the tables at every token start, so it can claim a position
the tables would otherwise take. The detector says whether a text is the language's at all,
which is the language's own knowledge as much as its hook is: what a ClaFi line looks like
is written once, and the hook and the detector both read it.

The keyword tables are looked up by bisection, so each is written in code unit order and
asserted so where it is written. A language that reads a word in any case - Pascal - says so
with ignoreCase and writes its tables in lower case: a lower-case table sorted by code unit is
sorted under the case-folded comparison as well, so the same bisection reads it with either.

A name no table lists is taken for a type by the language's TypeRule: a leading capital, which
is the framework's own convention and most C++ code bases', or Delphi's prefixed capital - a T,
I, E or P with a capital after it, as in TForm, IInterface, EAbort and PChar. The prefixed
shape is specific enough to outrank a parenthesis after the name, so TButton(Sender) reads as
a cast; a leading capital alone says less, and a call is a call.

## Claim

How sure a language is that a text is in it, in four grades. None: nothing of the language
was seen. Possible: nothing rules the language out, and nothing speaks for it either, which
is all a plain text can say of anything and is what its detector says of everything. Likely:
the language's shape - an object in brackets, a tag, a key and its equals sign - which
another language could share. Certain: a signature no other language spells - a header, a
declaration, the standard namespace.

The grades are ordered so the surest compares greatest, and they are what lets a list of
languages be asked in any order. A detector that saw no evidence answers None rather than
Possible, or it would tie with the plain text and the tie would go to whichever stands first.

## Detector

A language's reading of a whole text, answering a Claim. It is handed the text and nothing
else: a format's name, a file's extension, a MIME type are context the caller owns, and a
caller that has any settles what it can from it before asking. A detector reads what it
needs and no more - the first lines, the first and last character, one search for a word -
since it runs once per text, on the whole of it.

The free detect asks every language in a span and answers the one surest of the text, the
first among equals, and a Language{} where none claims it at all; it stops at the first
Certain. A list that holds the plain text answers Text for whatever nothing else claims,
which keeps the caller from special-casing the fallback. A language with no detector is
never found this way and is only ever picked by hand.

## Hook

A language's own rule, asked at every token start before the tables, and asked first of all
while a line stands in one of the language's own modes. A hook that claims a position moves
the scan past what it took, and answers true; one that does not leaves the scan where it
was and answers false, and the tables are asked instead. A hook carrying a mode across lines
sets the mode on the scan's state, and is handed the next line standing in it - so a hook
answers for its own modes without being asked, or the walk over the line cannot end.

## Scan

One line as the lexer walks it: the language, the line, where the walk stands, the state it
stands in, and where the tokens go - or nowhere, while only the state the line ends in is
asked for, which is what a pass over a whole document asks. What a hook is handed, and what
it takes tokens with.

## StateStrings

The strings a line state cannot spell, held by id. A raw string's delimiter has to reach
the line after the one that opened the string, and a state four bytes wide cannot carry
sixteen characters - so the delimiter is interned when the string opens, and its id is what
the state carries. The table is cleared with the states it serves and grows only where a
new string appears, which for a document is a handful of times.

## LineStates

The state every line of a text starts in, kept in step with the text through its edits.
Reset reads the whole text once. An edit re-reads from the first line it reached and on,
until a line starts in the state it already had - which for an ordinary key press is the
line after the edit, and for a comment opened above ten thousand lines is those ten thousand
lines, once. The edit is stated the way TextEdit states it, in the coordinates of the text
before it, and the line starts are moved by what the edit changed the length by rather than
counted again, so a key press in a long document costs the lines it reached and one pass
over the starts after them.

The tokens of a line are read on demand, from its text and its stored state, by whoever is
about to draw it. Nothing keeps them: a table of every token of a document would have to be
spliced on every key press, and the lines on screen are the only ones anyone asks about.
