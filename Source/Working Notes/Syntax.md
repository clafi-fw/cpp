# Syntax

The author's side of the footnote of the same name.

## LineState

The state at the START of a line is what is kept, one per line, because it is all that has
to be: the line's tokens follow from it and the line's text in the time it takes to draw the
line, and the state the line ends in is the next line's to keep. A document is a table of
these and nothing else grows with it.
