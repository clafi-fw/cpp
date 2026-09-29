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

A language stated as tables, one hook, one detector and one reader of declarations, and passed
by value: every member is a view over a constant, so a Language is a few pointers and stays
valid for as long as the constants do. The tables say what a table can - the keywords, the
comment and string delimiters, which characters are operators and which punctuation, how a name
no table lists is taken for a type. The hook says what a table cannot - a raw string, a
preprocessor line, an XML tag - and is asked before the tables at every token start, so it can
claim a position the tables would otherwise take. The detector says whether a text is the
language's at all, which is the language's own knowledge as much as its hook is: what a ClaFi
line looks like is written once, and the hook and the detector both read it. The reader of
declarations says which names a text in the language declares for itself and where each is in
force, which is what a box adds to its completion list - see Declarations. A language whose
texts declare nothing states none. The indent rule says where a line of the language stands -
see Indent; a language that states none is placed by its brackets.

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

## Completion

What a host offers a CodeBox to complete to, and the reading of a line that says what the
caret's place can take. The data is a list of CompletionEntry - a name, its kind, the
signature a hint shows, the hint's sentence - where an entry of kind Class carries two
lists of its own, its methods and its properties, one level deep: a member has no members.
The list is the host's, held by whoever read it, and a box names it rather than copying
it. Nothing here reads a file: the framework's language files are the application's for
now, and this is the shape they are read into.

A CompletionPlace is a caret's place on a line: where the name the caret ends starts, and
whether a dot stands right before that name. The name runs back from the caret over the
language's name characters and is a name only if it opens as one, so a caret after `10`
names no word. The dot counts only where what stands before it could carry a member - a
name that is not a number, or a call's or an index's closing bracket - so a range's second
dot and a number's decimal point name no member. What follows a dot is answered from every
class's members, whichever class the name before the dot would have been: telling them apart
needs the type of that name, which a Declaration records and nothing reads yet. Until it is
read the members are one list.

Matching is by prefix under the language's case rule - a language with `ignoreCase` matches
any case and one without matches exactly - and the rows are ordered the same way, so among
the names a prefix reaches the shortest, an exact match, stands first. A name is not
completed inside a comment or a string: the line's tokens say where those stand, and a
caret inside one or right at its end is left alone.

The kinds are spelled once, in completionKindName, and read back by completionKindOf in any
case - which is how an application's file states a kind by its word.

## Declarations

The names a text declares for itself, read off the whole text by the language's own reader
so that a box lists them beside the host's names - a script's variables, constants and
parameters ahead of the library it is written against. A Declaration is a CompletionEntry
of the name's own - the name, its kind, and the declaration as spelled for the hint, with
its blanks collapsed and cut past a hundred characters - and with it the type the name is
declared with where one name spells it (an `array of Integer` spells none), the scope the
name is in force in, as a range of the text, and the depth: how many routines stand around
the declaration, none for a global. A global's scope is the whole text. A routine's own
names - its parameters, its locals, the Result of a function with its return type - are in
force from the routine's header to its closing end, and a nested routine's stand one depth
deeper inside that. Where the words run out before the routine closes, the routine is being
written, and its names reach the text's end.

The Pascal reader walks the text as words - names, keywords, numbers and marks, with the
comments, strings and compiler directives the lexer found left out, so nothing inside them
declares - and reads the declaring part of the language: const, resourcestring, var and
threadvar sections at every level, a header's parameters with their modifiers, an inline
var or const inside a block and the variable of a `for var`, and the sections and blocks a
routine is made of. Everything between is stepped over: a statement declares nothing, and a
type section is read only so that the fields and methods inside a class, record, object or
interface body are not taken for declarations of their own - a type declares no name yet.
A header alone declares nothing either: a unit's interface part, a forward, an external.
An entry counts once its shape is there - a name with its colon, comma or equals sign after
it - so the name being typed at a section's end is no declaration until it is one.

The reader answers a fresh list each time and reads the whole text each time. It is asked
when a list opens, and the box compares the answer with the one it has before it rebuilds
any row - see Controls#completionlist.

## Indent

Where a line of source stands, worked out from the lines above it the way a writer places it
by hand - from where those lines actually stand, so a text indented in any style keeps its
own. The answer is a LineIndent: the column; whether the line's own first word decides it - a
closer, an else, a section word - which is what realigns a line once that word is finished;
and what closes the block the line leaves open at its end, which is what a block completion
writes below it.

A language states its reading through Language::indent. One that states none is placed by its
brackets: the column of the line above, a level in for each bracket that line leaves open, and
back to the column of the line that opened a bracket the line above closes; a line opening
with a closing bracket stands where the line that opened it stands, and a line ending with an
open brace or square bracket is closed below by its closer. Only punctuation counts, so a
bracket inside a comment or a string is none. C++, JSON and ClaFi are placed this way, and a
text in no language keeps the column of the line above. Whatever the language, a line starting
inside a comment or a string the line before it left open keeps that column too: its words are
not the language's.

The Pascal reading is Borland's style, read back from the line over the words of the lines
above - the comments, strings and directives the lexer found left out. A line stands one
level in from a begin, a try, a case's of, a repeat, an asm, a record, and a class, interface
or object that has a body; end and until stand at the line of the opener they close, found by
matching every block and bracket between. except, finally and the visibility words stand at
their block's opener and indent what follows them; else stands at the if its then pairs with,
or at its case after a branch's semicolon. A then or a do at a line's end indents the next line
alone: the line after that statement returns to where the statement started, however many
thens it ran through, and a begin under a then or an else stands where the statement starts.
A var, const, type or uses section indents its entries, and the next section word, a begin or
a routine's header closes it and stands where it stood; a uses clause is closed by its own
semicolon. A routine's header stands beside the routine closed before it, or one level inside
a routine still open, whose body is still to come - a nested routine. A unit's parts stand at
the file's column, as a unit's interface part's headers and a class's members stand at the
column of those before them. A statement running on past its line is one level in from where
it started, and the items of a bracket that opened a line before stand where the first did.

The rule reads a SourceLines - the lines, their tokens, and whether each starts inside a
comment - which IndentLines implements over a box's LineStates. IndentLines also restates
lines ahead of an edit, so an edit that places several lines places each against the lines
above as they will stand.

IndentUnit is how one level is written: a width in columns, and whether a tab is written
wherever a whole one fits - a tab reaching the layout's own stop, four columns. detectIndentUnit
reads a text's own: tabs where more lines open with a tab than with a space, and otherwise the
step taken most often between the indents of two lines in a row, a step of one being an
alignment and no step at all saying nothing.

The edits - breakEdit, realignEdit, shiftEdit, reindentEdit, pasteEdit - each answer one
replacement of the text and the selection it leaves, which is what lets a box make each of
them one step of its own; what each is for is under Controls#indents. The rule reads back
only as far as the block or the statement it needs, so placing a line costs what that block
holds rather than what the text does; a unit's closing end., which matches nothing, reads back
to the text's start.

## Blocks

The blocks a text opens and closes over more than one line - a line number for the opener and
one for the closer - in the order they open, the outer first where two open on one line. What
a box draws its indent guides down; see Controls#indentguides. A block opened and closed on one
line is none.

A language states its reading through Language::blocks. One that states none is read by its
brackets: a brace or a square bracket closed on a later line is a block, and a parenthesis is
matched so that a brace inside a call's arguments pairs with its own closer, but opens none -
a parenthesis over several lines is a statement running on. Only punctuation counts, so a
bracket in a comment or a string is none. C++, JSON and ClaFi are read this way; XML and a
text in no language have no blocks.

The Pascal reading walks the text forward over the same words the indent rule reads: a block
opens at begin, try, case, repeat, asm, record, initialization, and a class, interface or
object with a body, and closes at the end or until that matches it. A record's variant part
opens with a case the record's own end closes, so it is no block of its own.

The reading covers the whole text. A box reads it again on the first paint after the text
changes, which costs what lexing the text costs - a few milliseconds for a script of several
thousand lines, and little for one of a few hundred.
