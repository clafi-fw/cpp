# StdActions

The words that do not fit above a declaration.

## ClaFiText

**`k_name` is the format's identity.** Each platform derives its native clipboard spelling from it.

## ClaFiStyleSheet

**The styles a copied text is set in beyond the framework's**, written as a style sheet - see
CompositeSerializer<StyleSheet>. addText puts it beside the text, so an application that does not
hold those styles can still draw the words as they looked.

## addText

**Puts a text on a source, and beside it the styles it names.** The text is ClaFiText; where it is
set in any style beyond the framework's, StyleSheet::NamedIn of it is ClaFiStyleSheet. Plain text is
advertised out of the conversion table.

## takeText

**The text an offer holds, given to the sheet stated.** A name the text uses is looked up in that
sheet - the application's where none is stated - and then among the framework's, as any reading
does.

**A style the sheet does not know arrives with the text.** Where the offer carries
ClaFiStyleSheet, a style the stated sheet lacks becomes fields of the run's own over the nearest
style in its chain the sheet knows, the inner fields winning. The sheet itself is left as it is, and
no run names a carried style.

A text pasted into a TextBox is taken through the box's styleSheet.
