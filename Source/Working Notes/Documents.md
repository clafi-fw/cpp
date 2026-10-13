# Documents

The author's side of the footnote of the same name.

## DocumentsFolder readDocument

The node is a Dom node rather than a value: the layer compares and copies nodes without knowing
what a document is, and the derived folder casts to the type it stated in the tab schema.

## DocumentPage hasUnsavedEdits

The saved document is the other side of the comparison, not a flag kept as edits arrive. The
tab's view state is where an edit lands and where it stays until Save, so a tab restored from
settings comes back holding edits the file has never seen - and a flag set as they were made
would have been left behind with the session that made them. The saved side is read into a clone
of the document node, so the two sides are nodes of one type.

## DocumentsHomePageBase

Deleting asks once for the whole selection and settles all of it. Deleting the untouched files
first and asking about the rest would leave the user answering about a selection that is no
longer the one they made.

## Restored

A document page reads its document out of the tab's view state, so the file is read into the
tab first. The one tab that must not have it is one standing on a page it was restored onto: its
view state already holds this document, edits and all, and those are exactly what has not reached
the file yet. Every other tab wants the file - one arriving from another page carries the last
page's document there, and one the user has just opened carries nothing at all. The tab answers
that itself, isOnRestoredPage. The read goes through the page's loadDocument, which is
readSavedDocument on the document node - so a page whose saved side is not a file, a built-in
say, comes up on that side from the first read.
