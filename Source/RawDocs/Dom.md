# Dom

The words that no longer fit above a declaration.

## ScalarSerializer<Color>

A COLOUR AS CLAFI WRITES IT FOR CLAFI TO READ BACK: `#` and the red, green and blue bytes as six
hex digits for an opaque colour, with the alpha byte following as two more for any other. Upper
case is written and either case is read. The reader takes the two forms the writer produces and
nothing else - no bare digits, no short forms - and a string it cannot read leaves the colour as
it was, the way an unreadable number leaves an int.

It is the one spelling a colour has between ClaFi consumers: a `color #...` tag in Fmt and in a
document's markers, and a Color handed to Fmt's `{}`. How a user wants to see a colour, and what
a copy gives another application, is a presenter's choice - see Controls#colorspelling.

## WriteDefaults

Whether a saved document repeats what its layout already says. A document that states only
what it changes reads back the same, since a key the file leaves out keeps whatever the
layout built the tree with.

## Section

Section{ L"key", ...children..., OnEvent{ ... } }

A Section with no key is a grouping in the source only. Its children land
in the enclosing section and its handlers connect to it, so a helper can
return a bundle of nodes without adding a level to the document.
