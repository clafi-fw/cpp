---
name: "comments-and-docs"
description: "Apply the ClaFi rules for comments and documentation - code comments, footnotes, working notes, manuals and site copy - whenever writing, editing or reviewing any of them, alongside cpp-code-style when the comment sits in C++."
---

# Comments and Documentation

These rules cover every word written for a reader of the ClaFi tree: comments in code, the footnotes under `Source/.seedocs/footnotes/`, the working notes under `Source/Working Notes/`, the manuals under `docs/`, and copy for sites and install guides. Every rule is binding. Apply them silently in what you write, and do not explain them unless asked.

## 1. Where the Words Go

- **A comment** stands beside the declaration or the line it is about. Section 3 says when one is written at all.
- **A footnote** holds what does not fit on a declaration's one line: what a thing is and does, and the rules a caller keeps. There is one note per subsystem, `Source/.seedocs/footnotes/<Stem>.md`, and SeeDocs reads it into the generated reference.
- **A working note** holds the authors' side under the same stem in `Source/Working Notes/`: why it is so, how it is done inside, what happened. No tool reads it.
- **A manual** is a self-contained HTML page in `docs/`, one per subsystem, with a lower-case file name and no suffix (`dom.html`, `events.html`, `textengine.html`), listed in `index.html`. Footnotes and working notes are raw material for manuals, not manuals.
- **Site and guide copy** is what a visitor reads before using the framework or an application. Section 6 covers it.

The test for a sentence that could go either way: a framework user needs it to use the thing - footnote; only an author of the framework needs it - working note.

## 2. Language

These rules hold for every kind of text in section 1.

- **No all caps.** Writing in all caps is not allowed - not for a lead phrase, not for a single word of emphasis. Weight comes from bold: `**...**` in Markdown and in code comments alike, `<strong>` in HTML - `// **The icon sets an item's height**, being taller than the caption`. Names spelled in capitals are names, not emphasis, and keep their spelling: acronyms (`API`, `CRLF`, `UTF-8`), identifiers and macros (`INIT_PROPERTY`, `WM_DESTROY`), and the `TODO:` marker.
    - Text already in capitals is rewritten in the lines being touched. The rest of the file is offered as a separate pass rather than folded into the change, where it would bury the real edit.
- **State the current fact, not the change.** A comment, footnote or manual says what is true now, never what it replaced. No "used to", "was removed", "is gone", "no longer", "previously". This is not a delete pass:
    - A pure changelog line is deleted.
    - A property dressed as history is restated as the property: "this used to be handed boundsInForm(), so it was counted twice" becomes "it must not be handed boundsInForm(), which already carries that topLeft".
    - A rejected alternative is kept, phrased hypothetically.
    - A comparison against a state the reader never saw - "one dispatcher rather than two" - is the same defect in a shorter form. State the count, not the delta.
    - Working notes are the one place history belongs: what happened is part of what they hold.
- **Lead with what is.** A sentence, paragraph, section or bold lead-in opens on the fact. A negation goes in the clause that follows, where it names the consequence or the trap. A manual's first sentence states what the thing is, not what it lacks.
    - The flip is usually free: "A control does not build a layout" becomes "The engine builds the layout"; "Asking twice costs nothing" becomes "Asking twice is free".
    - A negation stays where it is the content: a Don't list, or a heading that parts two words a reader will conflate.
    - Before calling a manual done, count the sentences that open on a negation.
- **A direction word is a claim.** "Continues", "runs on", "before", "after", "above", "below" and "toward" describe something with an anchor and a head. Name the member that holds the direction and check that the code reads it, before writing the word and before striking it.
- **Clarity over expressionism.** No emotional, descriptive or promotional adjectives. Strip words that add no technical value, such as completely, heavily, perfectly, incredibly, trivially.
- **No exclamation marks.** None in comments, documentation or explanations. A run of `!!!!!` or `?????` used as an attention marker becomes a `TODO:` with the actual question in it.
- **Short sentences.** Keep sentences short and direct, and avoid nested clauses.
- **Spaced hyphens.** Use a spaced hyphen ( - ) where a long dash would go.
- **Click and press.** These are the framework's event names, and both fire from the keyboard as well as the mouse. When the pointer is meant, write "mouse click".

## 3. Comments in Code

- **No comment by default.** A comment earns its line only by carrying what the code cannot say for itself - an invariant, a trap, a constraint imposed from outside, a rejected alternative, a reason. Strike the comment and read what is left. If the code is as clear and nothing true became unrecoverable, leave it struck. A clearer name or a named local retires a comment for good.
    - The public surface is the exception: a control, its properties and events, and the enums they take each carry one comment line. SeeDocs reads it as the declaration's hint.
- **For the reader of the code.** A comment serves whoever reads the code later. Nothing that only makes sense in the history of a prompt or a session, such as `// Added`, `// new member` or `// <--- changed`.
- **Comment the invariant, not the habit.** When a comment justifies why something is safe, state the property that makes it safe and what would break it. A justification that is merely true today misleads the next reader.
- **One line, one place.** A class, struct, enum, member, property or event carries one comment line at most. Two lines is a violation wherever they sit, and so is a comment both above a declaration and on it.
- **Placement.** The declaration decides where its comment goes.
    - A declaration that opens a scope - a class, a struct, an enum - takes its comment on the line above. That comment covers everything inside.
    - Every other declaration takes its comment on the declaration line while the whole line fits 100 columns, and on the line above when it does not.
    - An above-line comment touches its declaration, with no blank line between them.
    - A comment on the declaration line is a label: lower case, no full stop. A comment above a declaration is a sentence.
- **Longer explanations go to the footnotes.** Where the mechanics need more than a line, the words go into the subsystem's footnote, and the comment ends with a reference: the word `See`, the note's file stem, an optional `#` anchor, and nothing after it. A reference costs columns, so a comment carrying one usually sits above. What only an author needs goes into the working note of the same stem; no comment points there.

  Example:
  ```cpp
  // What a drag that starts on an item does. See Selection-Model
  export enum class DragMode
  {
      EasySelect,   // an item is one more place to start a selection rectangle from
      EasyDrag      // an item is the selection's handle, and the drag carries what is selected
  };

  // Whether the view holds a selection on top of the current item.
  DECLARE_PROPERTY(SelectionMode, selectionMode, SelectionMode::None)
  ```

- **Run the checker.** `SeeDocsCli check Source`, built from `Tools/SeeDocs` (its README has the build lines), reports every comment that breaks the length and placement rules on the declarations it reads, and every `See` that names a note or a heading the footnotes do not have.

## 4. Footnotes and Working Notes

- **One stem, two folders.** A subsystem's footnote and its working note share a file stem. A stem is a name: the hyphen joins words (`Item-Containers`, `UI-Types`) and never stands for a folder path.
- **A section per declaration.** A footnote section is headed `## <Name>`, the declaration as the code spells it, and a member's `## <Owner> <member>`, one space apart - `## SearchBox listKeptBy`. SeeDocs finds a declaration's section by that heading, in its folder's note, when its comment carries no reference. A section several declarations would share is split, so each reaches words about itself.
- **Slugs are unique in a note.** A reference reaches a section by its heading's slug - the part after the last `::`, lower case, other characters joined into one `-`. Two headings with one slug collide, and only the first is reached.
- **Read at a glance.** A reader arrives from a declaration in a hurry. A section holds what the one-line hint leaves out - what the thing does and the rules a caller keeps - and nothing the hint or the signature already says. One idea per paragraph, in short sentences.
- **Bold lead-ins.** A paragraph that states a rule opens on a bold sentence: `**The menu keeps the window it was placed in.** It is ...`.
- **Bullets for what enumerates** - keys, values, cases, steps. A bullet line starts `- `, and its continuation lines start with two spaces.
- **Cross references name the declaration** - `see DropList` - which SeeDocs links. A `Stem#anchor` written in prose shows as plain text.
- **Lines wrap at 100 columns.** SeeDocs rewraps a paragraph edited through it at 100 on save, and leaves the paragraphs it did not touch as they are.

## 5. Manuals

- **Update the manual when the public API changed.** When a finished piece of work has changed the public API, update the subsystem's manual before reporting the job done. The manual is part of the change, not a follow-up task.
    - **When:** only once the coding job is over and the API is settled. Whoever asked for the work says so ("the coding job's well done, the API is established"), or the work reaches an equivalent close. Development is iterative, and the API moves back and forth across many turns. Those intermediate shapes are not documented, and that is correct - a manual written mid-iteration describes an API that stops existing on the next turn. Hold the manual until the shape stops moving, then write the settled result once.
    - **What counts:** anything a caller writes or has to know - a name, a spelling, an argument order, a rule about ordering or lifetime, a constraint that stopped holding. Measure it across the whole job: the API as it stood before the work began against the API as it stands now. Churn that cancels out is not a change, and neither is an internal refactor no call site can observe.
    - **Where:** the subsystem's page in `docs/`, as section 1 describes. A new manual gets a line in `index.html` too.
    - **Remove what stopped being true:** if the change deleted a caveat, delete the warning about it. A manual that still warns about a fixed problem teaches a rule that does not exist, and is worse than one that never mentioned it.
    - **Describe, do not narrate:** the manual describes the API as it is now, by the current-fact rule in section 2. The reader has never seen the previous version.
- **Match the existing manuals.** Follow an existing manual's structure and copy its inline stylesheet rather than restyling.

## 6. Site and Guide Copy

- **Stay general.** Copy for sites and install guides carries no precise enumeration that goes stale soon, such as a list of which settings are kept, and no machinery detail the reader does not need, such as where themes are stored. Those belong where that machinery is explained.
