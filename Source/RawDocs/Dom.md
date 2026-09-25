# Dom

The words that no longer fit above a declaration.

## WriteDefaults

Whether a saved document repeats what its layout already says. A document that states only
what it changes reads back the same, since a key the file leaves out keeps whatever the
layout built the tree with.

## Section

Section{ L"key", ...children..., OnEvent{ ... } }

A Section with no key is a grouping in the source only. Its children land
in the enclosing section and its handlers connect to it, so a helper can
return a bundle of nodes without adding a level to the document.
