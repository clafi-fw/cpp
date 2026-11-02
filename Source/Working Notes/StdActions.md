# StdActions

The author's side of the footnote of the same name.

## ClaFiText

Declared here rather than in the Transfer core, which names no type above itself: a format is owned
by whoever owns the content it carries.

## ClaFiTextFormat

Oj, 2026-11-03: "copy the control's text format to the clipboard too and use it in the WhatsClip's
preview". Two choices were put to him and he took both recommendations: a format of its own beside
ClaFi.Text and ClaFi.StyleSheet, rather than a Format section inside the text's package, which a
text's spelling would carry only on the clipboard; and an application style the format names
carried in ClaFi.StyleSheet, rather than flattened at copy into the framework style it is based on
plus its fields, which would lose the name.

The format's style is found through the copied text's sheet, which is the box's, and carried by
name the way Text::adopted carries a run's - a name the framework's sheet holds stands for itself.

WhatsClip lays its preview over one sheet kept for the process, which takes the carried styles of
each copy it shows. A sheet per preview would have to outlive every layout of the text shown over
it, and a TextLayout keeps a copy of the last text it laid out.
