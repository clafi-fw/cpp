# StdActions

The words that do not fit above a declaration.

## ClaFiText

**`k_name` is the format's identity.** Each platform derives its native clipboard spelling from it.

## ClaFiStyleSheet

**The styles a copied text is set in beyond the framework's**, and the one its format names,
written as a style sheet - see CompositeSerializer<StyleSheet>. addText puts it beside the text,
for an application that reads the styles a copy carries.

## ClaFiTextFormat

**The format of the control a text was copied from** - see TextFormat and
CompositeSerializer<TextFormat>. A preview draws the text in it, as it was drawn where it was
copied. A paste leaves it: the box pasted into keeps its own format.

## addText

**Puts a text on a source, and beside it its styles and its format.** The text is ClaFiText, put
over the framework's sheet: each style it is set in beyond the framework's comes as one of the
text's own - see Text styles - so the copy names nothing in the sheet it was copied from, which may
go first. Where it carries any, the same styles are ClaFiStyleSheet, with the style the format names
where that is beyond the framework's too.

**The format is ClaFiTextFormat**, where one is given and states anything. TextBox gives its own.
Plain text is advertised out of the conversion table.

## takeText

**The text an offer holds, over the sheet stated** - the application's where none is stated.

**A style goes by its name.** One the stated sheet holds by the name draws the words. One it lacks
comes as one of the text's own, so the words draw as they were copied; the sheet itself is left as
it is.

A text pasted into a TextBox is taken over the sheet the box's text is over.
