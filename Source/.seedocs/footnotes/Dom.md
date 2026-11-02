# Dom

The words that do not fit above a declaration.

## ScalarSerializer<Color>

- `#RRGGBB` - an opaque colour.
- `#RRGGBBAA` - any other colour, alpha last.

Upper case is written and either case is read. Any other string leaves the colour as it was.

**This is the spelling between ClaFi consumers** - an `Ink=#...` among a text's styles.
How a user sees or copies a colour is a presenter's choice - see ColorSpelling.

## WriteDefaults

**Set to No, a saved document holds only what differs from its layout.** It reads back the same,
since a key the file leaves out keeps the layout's value. A list's items hold only what differs
from an item nothing is stated about - a multi-type sequence's, from their name's blueprint - and
every item stays, since a list is read by position.

## MultiTypeSequence

**A list kept in the order written, its items of several kinds, each under a name.** The name
picks the item's blueprint, which holds what the item is before anything is stated: a value, or a
section's schema. A name may repeat, and one blueprint may be the unnamed one:

    MultiTypeSequence& pieces = section.addMultiTypeSequence(L"Pieces");
    pieces.addBlueprint(L"", std::wstring{});
    pieces.addBlueprint(L"Run", runSchema);

**Each format keeps the order:**

- ClaFi: a `name=value` or `name=[` line per item, and `=value` for an unnamed one.
- JSON: an array of one-member objects - `[ { "": "Use " }, { "Run": { ... } } ]`.
- XML: an element per item, named as the item is. Unnamed items are the text between the
  elements, written with nothing added around it, and a reader takes every blank there as words.

**An item whose name no blueprint has is skipped**, with a schema error, and so is one whose value
is of another kind than its blueprint's.

## CompositeSerializer<Text>

**A text is a section of its own styles and its pieces**, the pieces a MultiTypeSequence in text
order:

    Caption=[
        Styles=[
            =[
                Name=#1
                Base=Code
                Ink=spot
            ]
        ]
        Pieces=[
            P=[
                Align=Center
            ]
            ="Use "
            Run=[
                Style=#1
                =replaceText
            ]
            Space=4
        ]
    ]

**A piece in no style and linking nowhere is one line**: its words unnamed, or an object under its
name - Icon, Space, VSpace, FlexSpace or TabTo - whose character goes without saying.

**A Run holds anything more**: its words as its unnamed value, or an object under its name, and
Style, Link and Anchor. Style names one of the text's own styles, or else one in the reading sheet
or up its parents - see StyleSheet ReadingScope; any other name reads as words in no style.

**A P states the paragraph's Align, Indent or LineSpacing** and stands where the paragraph starts.

**In XML a run's words are its element's text**, and an unnamed piece is text between the elements:

    <Pieces><P Align="Center"/>Use <Run Style="#1">replaceText</Run></Pieces>

**The named styles the text carries come first** - see Text styles - each after its base. An entry
states its fields: FontFamily, FontSize, FontWeight, FontStyle, Ink and Scripts. An entry stating no
Base is based on the style of its own name up the reading sheet, so `Name=Code` with `Ink=spot` is
this text's code in the spot ink.

**A look follows them**: an entry for each run style stating more than its base, written once and
named # and a number - #1, #2. A look is no style, and reads back as the style of the runs naming
it, so texts read from two files never share a look by its name.

**An ink is an ink colour and a grade**: the colour is left out where it is text, and the grade
where it is strongest and the colour is not text - `spot`, `accent muted`, `muted`, `strongest`. A
grade between the steps is written as its share, and a colour stated outright as `#FF8800`. An icon
is `w,h,baseline` and reads back with no painter, drawing nothing.

## CompositeSerializer<StyleSheet>

**A style sheet is a section of its styles**, each an entry the way a text's own style is written:

    Styles=[
        =[
            Name=Key
            Base=Code
            FontWeight=Bold
        ]
        =[
            Name=Code
            FontFamily=Consolas
        ]
    ]

**A Base names one of the sheet's styles, or else one up its parents**, wherever in the list it
stands - Key above is based on this sheet's Code. An entry stating no Base, or its own name, is
based on the style of that name up the parents where there is one. A Base leading back to its entry
is dropped, and so is one naming no style.

**An entry with no name, or with a name an entry above it took, is skipped.** The styles read are
listed as the file lists them.

**Read over a sheet in use**, by getTo, the sheet keeps every style the two name alike where it
stands, and the parent it is over - see StyleSheet.

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
