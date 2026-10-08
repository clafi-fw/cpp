# Footnotes

The prose that would not fit above a declaration, one note per subsystem, reached from the code
by `See <note-stem>` at the end of a one-line comment. SeeDocs reads these and nothing else: a
section stands under its declaration in the generated documentation, so it is written for whoever
builds on the framework - an application, or a control derived from one of its classes - and says
what a thing is, what it does, and the rules a caller or a derived class keeps. Why it is so, how
it is done inside and what happened on the way is in the working note of the same name, under
`Source/Working Notes`.

## How a reference reaches a section

A section heading IS the name of the declaration it explains, so a comment writes `See Grids` and
reaches `## RowCell` without spelling an anchor. A section about something the code does not name
takes a heading of its own, and the comment spells the anchor: `See Control-Foundation#control-text`.

`seedocs check` reports a reference to a note that does not exist, and a reference to a section
the note does not carry.

## One note per subsystem

`Controls`, `Controls-Base`, `Grids`, `Browser`, `Item-Containers` and `Selection-Model` for the
controls; `Control-Foundation`, `Context`, `Graphics-Types`, `TextEngine-Types`, `Syntax`,
`Transfer`, `AppTheme`, `Dom`, `Dt`, `Events`, `UI-Types` for the core; `Platform`, `Icons`,
`Application`, `Diagnostic`, `StdActions` for the rest.
