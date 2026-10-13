# Footnotes

The prose that would not fit above a declaration, one note per subsystem, reached from the code
by `See <note-stem>` at the end of a one-line comment. SeeDocs reads these and nothing else: a
section stands under its declaration in the generated documentation, so it is written for whoever
builds on the framework - an application, or a control derived from one of its classes - and says
what a thing is, what it does, and the rules a caller or a derived class keeps. Why it is so, how
it is done inside and what happened on the way is in the working note of the same name, under
`Source/Working Notes`.

## How a reference reaches a section

A section heading **is** the name of the declaration it explains, so a comment writes `See Grids`
and reaches `## RowCell` without spelling an anchor. A section about something the code does not
name takes a heading of its own, and the comment spells the anchor: `See
Control-Foundation#control-text`. The `#` title at the top names the note and opens no section, so a
declaration named like the note takes the `##` section of its name, and one with no such section
takes nothing from it.

A declaration whose comment carries no reference takes the section headed by its name in its
folder's note. A member's heading is its owner and its name, one space apart -
`## SearchBox listKeptBy` - so one heading reaches one declaration. A constructor takes no section
by name, since its name heads its type's. Two headings whose slugs match collide, and a reference
reaches only the first.

`SeeDocsCli check` reports a reference to a note that does not exist, and a reference to a section
the note does not carry.

## How a section reads

A reader arrives from a declaration and takes the section in at a glance. It holds what the
declaration's one line leaves out - what the thing does and the rules a caller keeps - and nothing
the hint or the signature already says.

- One idea per paragraph, in short sentences, with a bold lead-in where it states a rule.
- Bullets for anything that enumerates - keys, values, cases.
- A cross reference names the declaration - see DropList - and SeeDocs links it. A note's anchor
  written in prose shows as plain text.

## One note per subsystem

`Controls`, `Controls-Base`, `Grids`, `Browser`, `Item-Containers` and `Selection-Model` for the
controls; `Control-Foundation`, `Context`, `Graphics-Types`, `TextEngine-Types`, `Syntax`,
`Transfer`, `AppTheme`, `Dom`, `Dt`, `Events`, `UI-Types` for the core; `Platform`, `Icons`,
`Application`, `Diagnostic`, `StdActions` for the rest.
