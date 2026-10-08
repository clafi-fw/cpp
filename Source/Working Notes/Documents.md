# Documents

The author's side of the footnote of the same name.

## Restored

A document page reads its document out of the tab's view state, so the file is read into the
tab first. The one tab that must not have it is one standing on a page it was restored onto: its
view state already holds this document, edits and all, and those are exactly what has not reached
the file yet. Every other tab wants the file - one arriving from another page carries the last
page's document there, and one the user has just opened carries nothing at all. The tab answers
that itself, isOnRestoredPage. The read goes through the page's loadDocument, which is
readSavedDocument on the document node - so a page whose saved side is not a file, a built-in
say, comes up on that side from the first read.
