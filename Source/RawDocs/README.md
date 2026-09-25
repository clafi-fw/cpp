# Raw Docs

The prose that would not fit above a declaration, one note per subsystem, reached from the code
by `See <note-stem>` at the end of a one-line comment.

These are neither working notes nor manuals: a manual is written for whoever reads it, and these
were written where the code needed them. NOTHING HERE IS SHIPPED IN THIS FORM. They are the
bricks the end-user documentation will be built out of.

## How a reference reaches a section

A section heading IS the name of the declaration it explains, so a comment writes `See Grids` and
reaches `## RowCell` without spelling an anchor. A section about something the code does not name
takes a heading of its own, and the comment spells the anchor: `See Control-Foundation#control-text`.

`python "Tools/Surface Scanner/scan.py" --check` reports a reference to a note that does not
exist, and a reference to a section the note does not carry.

## One note per subsystem

`Controls`, `Controls-Base`, `Grids`, `Browser`, `Item-Containers` and `Selection-Model` for the
controls; `Control-Foundation`, `Context`, `Graphics-Types`, `TextEngine-Types`, `Syntax`,
`Transfer`, `AppTheme`, `Dom`, `UI-Types` for the core; `Platform`, `Icons`, `Application`,
`Diagnostic`, `StdActions` for the rest.
