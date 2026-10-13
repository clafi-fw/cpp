# Dom

The words that do not fit above a declaration.

## ScalarSerializer<Color>

- `#RRGGBB` - an opaque colour.
- `#RRGGBBAA` - any other colour, alpha last.

Upper case is written and either case is read. Any other string leaves the colour as it was.

**This is the spelling between ClaFi consumers** - a `color #...` tag, in Fmt or in a document's
markers, and a Color handed to Fmt's `{}`. How a user sees or copies a colour is a presenter's
choice - see ColorSpelling.

## WriteDefaults

**Set to No, a saved document holds only what differs from its layout.** It reads back the same,
since a key the file leaves out keeps the layout's value.

## Dt::Section

    Section{ L"key", ...children..., OnEvent{ ... } }

**A Section with no key is a grouping in the source only.** Its children and handlers go to the
enclosing section, so a helper can return a bundle of nodes without adding a level.

## Value

    Value{ L"key", defaultValue, OnEvent{ ... }, OnEvent{ ... } }

A handler can also be a bare lambda. Its parameter names the event.

## Iterators

**A range-for visits the live child nodes** - over a Sequence<Section>, the child Sections
themselves. Dereferencing goes through operator[], so no item value is built or cloned.
