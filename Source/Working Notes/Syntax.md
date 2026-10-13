# Syntax

The author's side of the footnote of the same name.

## LineState

The state at the **start** of a line is what is kept, one per line, because it is all that has
to be: the line's tokens follow from it and the line's text in the time it takes to draw the
line, and the state the line ends in is the next line's to keep. A document is a table of
these and nothing else grows with it.

## LineStates

applyEdit moves the line starts past the edit by what the edit changed the length by, rather than
counting them again, so a key press in a long document costs the lines it re-lexes and one pass
over the starts after them. The tokens are not kept because a table of every token of a document
would have to be spliced on every key press.

## IndentRule

The Pascal rule reads back only as far as the block or the statement it needs, so placing a
line costs what that block holds rather than what the text does; a unit's closing end., which
matches nothing, reads back to the text's start.

## BlockReader

The reading covers the whole text. A box reads it again on the first paint after the text
changes, which costs what lexing the text costs - a few milliseconds for a script of several
thousand lines, and little for one of a few hundred.
