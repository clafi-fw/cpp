# Syntax

The words that do not fit above a declaration.

## TokenKind

**A kind is what one line shows:**

- a keyword by its spelling
- a function by the parenthesis after it
- a property by its colon
- a type by a table or by its capital

A local and a member, or a parameter and a field, look alike on a line and get no kind of their own.

**Plain is the run between tokens**, so a line holds only the tokens that show.

## LineState

- `Mode::normal` (0) - ordinary source.
- `Mode::blockComment` (1) - `aux` is the index of the open comment rule.
- `Mode::firstLanguageMode` (8) and up - the language's own. A line starting in one goes to the
  hook first.

**A state is four bytes**, so a raw string's delimiter goes in the StateStrings, and `text` holds
its id.

## Inks

**An ink names a colour rather than stating one**, so one table holds in light and dark themes.

**A kind left at `Ink{}` is drawn as plain text** and states no colour span. A new table leaves
every kind there.

defaultInks, the framework's table, inks every kind but Plain. Operators and punctuation stand a
grade behind the text.

## Language

**A Language is a few views over constants**, so it is passed by value and stays valid while the
constants do. Syntax::Languages holds the framework's own.

- **The tables** list keywords, types and constants, the comment and string delimiters, and the
  operator and punctuation characters. A name no table lists may still be a type - see TypeRule.
- **The hook** reads what a table cannot - a raw string, a preprocessor line, an XML tag. See Hook.
- **The detector** says how sure the language is of a text. See Detector.
- **The readers and the indent rule** serve a CodeBox - see DeclarationReader, ParameterReader,
  IndentRule and BlockReader.
- **`routineWords`** lists the words that open a routine's header, so a header's own brackets are
  not read as a call.

**Every table is sorted by code unit**, since a name is looked up by bisection. With `ignoreCase`,
as in Pascal, the tables are written in lower case. A lower-case table sorted by code unit is
sorted case-folded too, so one bisection reads it either way.

## TypeRule

**A prefixed capital outranks a call**, so `TButton(Sender)` reads as a cast and TButton as a Type.
A leading capital says less, and a name before a parenthesis stays a Function.

## Claim

- **Possible** is what the plain-text language, `Languages::text`, answers for any text.
- **Likely** - a shape another language could share: an object in brackets, a tag, a key and its
  equals sign.
- **Certain** - a signature no other language spells: a header, a declaration, the standard
  namespace.

**A detector that sees no evidence answers None**, never Possible. Possible would tie with the
plain-text language, and a tie goes to the language listed first.

## Detector

**A detector gets the text alone.** A file's extension, a format's name or a MIME type is the
caller's to settle before asking.

**A detector reads only what it needs** - the first lines, the first and last character, one
search for a word. It is handed every text whole.

## detect

**Put `Languages::text` in the list** and it answers for whatever no other language claims, so the
caller needs no fallback of its own. A language with no detector is never detected, and is only
picked by hand.

## Hook

**To claim a position**, a hook emits its tokens, moves `index` past them and answers true. A true
answer that moved nothing makes the lexer throw.

**To pass**, a hook leaves `index` where it was and answers false. The tables are asked next.

**A mode carried across lines** goes in `state.mode`, from `Mode::firstLanguageMode` up. A line in
such a mode goes to the hook at every position, and the hook must claim each one.

## Scan

**A hook emits through `emit`.** A pass over a whole document leaves `tokens` null and asks only
for the state each line ends in, so a hook still moves `index` and sets `state`.

## StateStrings

**A hook interns a string with idOf** - a raw string's delimiter - and keeps the id in
`LineState::text`. stringOf reads it back on the next line. Ids start at one, so a `text` of zero
holds no string.

## LineStates

**An edit costs the lines it reaches.** applyEdit re-lexes until a line starts in the state it
already had - for a key press, the next line. A block comment opened above ten thousand lines
re-lexes those lines, once.

applyEdit takes the `replaced` and `insertedLength` of a TextEdit as they are.

**Tokens are lexed on each call.** tokensOf lexes the line from its stored state, and nothing keeps
the tokens.

## CompletionEntry

**The host fills the list** from its own language files, and the framework reads none. A file
states a kind by its word, which completionKindOf reads in any case.

**Members go one level deep.** A Class entry carries its parent's name, its methods and its
properties, and those carry no members of their own.

**The list is the host's.** A box refers to it and copies nothing, so the host keeps it alive.

## completionPlace

The word runs back from the caret over the language's name characters.

- **A word must open as a name**, so a caret after `10` has an empty word.
- **A dot means a member** after a name that is not a number, or after a call's or an index's
  closing bracket. A range's second dot and a decimal point mean none.

## completionMatches

**The case rule is the language's** - any case with `ignoreCase`, the exact case without it.
completionNameLess sorts by the same rule, so an exact match lists ahead of the longer names it
begins.

## completionBlocked

A caret right at the end of a comment or a string counts as inside it.

## completionSubject

The subject is read back from the dot, on the caret's line only.

- A link is a name, with a call's or an index's brackets after it stepped over. A bracket inside a
  comment or a string counts for nothing.
- A dot before the name links it to the name before. A range's two dots end the chain.

**Anything else leaves the subject unknown**, and the answer empty: a bracketed expression, a name
apart from its dot, a chain whose root is on the line above.

## completionMemberClasses

- **The root** is a name the text declares, in force at the caret - the innermost of several -
  read by its declared type. Failing that, it is a name the host offers. A class answers itself,
  so a class name and a cast to it both answer the class. Any other entry answers its type.
- **Each later link** is the member of that name its class states - the class's own ahead of a
  parent's - read by its type.
- **Brackets after a link**: a routine and a property answer their type through them - a call's
  result, an indexed property's element. Any other name with brackets answers no type.

**A type naming no class of the list leaves the subject unknown**, and no member is listed. The
answer holds each parent once, so a parent named in a loop ends the walk.

## Declaration

A box lists these ahead of the host's names - a script's variables, constants and parameters
before the library it is written against.

- `entry.type` is set where one name spells the type, and empty for `array of Integer`.
- `entry.signature` is the declaration as spelled for the hint, its blanks collapsed and cut past
  a hundred characters.
- A global's `scope` is the whole text, and its `depth` is zero.

## DeclarationReader

**A reader takes the whole text on every call**, and answers a fresh list. A box adds the names to
its completion list.

**The Pascal reader** reads the declaring part of the language. A comment, a string or a compiler
directive declares nothing.

- Sections: const, resourcestring, var and threadvar, at every level.
- A header's parameters with their modifiers, an inline var or const inside a block, and the
  variable of a `for var`.
- A routine's own name, as a Function or a Procedure. Its signature is the whole header, and a
  function's type is its result type. It is in force everywhere at the top level, and inside the
  enclosing routine when nested.
- A routine's parameters, its locals and a function's Result, one depth deeper. They are in force
  from the header to the routine's closing end. A routine still being written reaches the text's
  end.

The Pascal reader passes over:

- A method's implementation, its name qualified by its class.
- A name's second header. A forward and its body, or an interface's header and the
  implementation's, declare the name once, with the first header.
- The parameters of a header alone - in a unit's interface part, a forward, an external.
- A type section. It is read so that the fields and methods inside a class, record, object or
  interface body are not taken for declarations.
- An entry before it has its shape - a name with its colon, comma or equals sign. A name being
  typed at a section's end is no declaration yet.

## ParameterReader

A box marks the parameter being written in the hint over a call.

**The Pascal reader** takes the first bracket outside a comment or a string, round or square - an
indexed property's list is square - and reads the list up to its closer.

- A group is its names with commas between, an optional modifier ahead - const, var, out,
  constref - and its colon.
- After the colon, the type and a default are stepped over to the semicolon that ends the group or
  the bracket that ends the list. An attribute's or an array type's brackets and a string's quotes
  are stepped over with them.
- A list left open is being written, and is read as far as it goes.

## completionCall

The walk goes back from the caret, over the lines above, to the bracket left open. On the way it
counts the brackets closed and the commas at the caret's depth. A bracket, a comma or a semicolon
inside a comment or a string counts for nothing.

What is before the open bracket decides:

- **A name** - a chain of names and their brackets, read as a member's subject is - is the routine
  called. The commas counted give the argument.
- **A name after one of the language's routineWords** - a header's own list - ends the walk with
  no call.
- **Anything else** - an operator, a keyword such as if or not - groups an expression, with commas
  of its own. The walk goes on outward and counts again.

**A semicolon or a brace at the caret's depth** is the statement's start, and the walk ends there
with no call. Until then it reads on above the caret's line, so a call may span lines. A caret
inside a string still has the call around it.

## completionCallee

- **A single name** is one the text declares, in force at the caret - the innermost of several, of
  any kind, so a variable of the routine's name shadows it. Failing that, it is one the host
  offers, of any kind.
- **A chain** is a member. The links before the last are its subject, resolved as
  completionMemberClasses resolves one. The last is the member of that name its classes state.

A subject of a class the list lacks, or a name nothing states, answers null.

## TitleBlock

A GrandSmeta macro opens with lines such as `//#DocTypes = Smeta, ObjSmeta`, its prefix `//#`.

**The block ends at the first line of source.** It is the run of lines the text opens with while
each holds only comments and blanks. A key's line past it states nothing.

**The block is the host's**, like the completion list. A box refers to it and copies nothing.

## TitleKey

**A key with no values takes any text**, and nothing is listed for it. Listed values are the known
ones, and the key may take others too - a category of the user's own.

## completionTitlePlace

**A line has a place** when it opens with the block's prefix, and the caret is at or past the
prefix's end.

- **Up to the end of the name after the prefix**, the place is the key. The keys the block's other
  lines state are left out - a key is stated once.
- **Past the key, its blanks and an equals sign**, the place is a value of that key - a key the
  block knows, and one that lists its values. Any other sign, `:=` among them, gives no place.
- **In a list**, the value is the text between the commas around the caret. The values of the
  line's other items are left out.
- **For a key taking one value**, the value is the whole text after the sign, so a value holding
  blanks - a category of several words - matches whole.

The blanks are the space, the tab and the no-break space. The case rule is the language's. The
names left out are views into the text, valid while the text is.

## IndentRule

**A rule works from the columns of the lines above**, as a writer places a line by hand, so a text
indented in any style keeps its own.

**A line starting inside a comment or a string skips the rule.** lineIndent gives it the column of
the nearest line above holding text, since its words are not the language's.

**The Pascal rule is Borland's style**, read back over the words of the lines above, with the
comments, strings and directives left out.

- A line is one level in after a begin, a try, a case's of, a repeat, an asm, a record, and a
  class, interface or object with a body.
- end and until align with the line of the opener they close, found by matching every block and
  bracket between.
- except, finally and the visibility words align with their block's opener, and indent what
  follows them.
- else aligns with the if its then pairs with, or with its case after a branch's semicolon.
- A then or a do at a line's end indents the next line alone. The line after that statement
  returns to where the statement started, however many thens it ran through.
- A begin under a then or an else aligns with the statement's start.
- A var, const, type or uses section indents its entries. The next section word, a begin or a
  routine's header closes it and aligns with the section's word. A uses clause closes at its own
  semicolon.
- A routine's header aligns with the routine closed before it. Inside a routine still open, whose
  body is to come, it is one level in - a nested routine.
- A unit's parts are at the file's column. A unit's interface headers and a class's members align
  with those before them.
- A statement running past its line is one level in from where it started. The items of a bracket
  opened on an earlier line align with the first item.

## bracketIndent

- A line takes the column of the nearest line above holding text, one level in where that line
  leaves a bracket open. Where it closes an earlier line's bracket instead, the line goes back to
  that earlier line's column.
- A line opening with a closing bracket takes the column of the line holding its opener.
- A line whose last bracket is an open brace or square bracket states its closer in `closer`.

**Only punctuation counts**, so a bracket inside a comment or a string is ignored. C++, JSON and
ClaFi are placed this way. A text in no language keeps the column of the line above.

## detectIndentUnit

- **Tabs**, where more lines open with a tab than with a space.
- **Otherwise the step taken most often** between the indents of two lines in a row. A step of one
  is an alignment, and no step at all answers nothing.

## IndentUnit

**A tab is four columns** - the layout's own tab stop - whatever `width` is.

## IndentEdit

**Each edit is one undo step.** breakEdit, realignEdit, shiftEdit, reindentEdit and pasteEdit
each answer a single replacement. What each key does in a box is under CodeBox.

## BlockReader

A box draws its indent guides down the blocks a reader answers - see IndentGuides. **A reader
covers the whole text**, and answers only blocks that span lines.

**The Pascal reader** walks the text forward over the words the indent rule reads.

- A block opens at begin, try, case, repeat, asm, record, initialization, and a class, interface
  or object with a body.
- It closes at the end or until that matches it.
- A record's variant part belongs to the record: its case is closed by the record's own end.

## sourceBlocks

Two blocks opening on one line come outer first, so the last block found holding a line is the
innermost.

## bracketBlocks

**Parentheses are matched too**, so a brace inside a call's arguments pairs with its own closer. A
parenthesis spanning lines is a statement running on, and opens no block.

**Only punctuation counts**, so a bracket in a comment or a string is ignored. C++, JSON and ClaFi
are read this way. XML and a text in no language have no blocks.
