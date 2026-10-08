# Documents

The words that no longer fit above a declaration.

A documents application is a browser over one folder of files of one kind: the home page lists
the folder as tiles, a document page edits one file of it, and the tab keeps the document as it
stands on screen. What the application states is the kind - the extension, the words, how a file
is read and written and drawn - the editor control, and whatever its own tool bar adds. The rest
is here.

## DocumentKind

The words are used in sentences the user reads, so they are given in the case a sentence uses
them: "script", "scripts", and the stem a new file is made under, "New Script". Titles and
messages are built from them - "New script", "Delete scripts", "Script saved", "The script needs
a name". The attribute name is where a page keeps its document in the tab's config, so a tab
restored from settings comes back with what was not saved.

## DocumentsFolder

The files stand in the base; how one is read, written, drawn and told apart from an untouched
one is the derived folder's, stated in its overrides. The folder is the application's, built
after the platform and outliving every window: the browser and its pages hold it by reference,
handed over as a DocumentsFolder pointer prop, which is the exact type Props::get finds it by.
The files are read from the disk on the first ask after the directory watch reports, and every
listener is told; the list itself is read again by whoever asks for it. A folder that keeps
something per file - a parsed theme, say - rebuilds it in filesRead, which is told the list each
time it has been read afresh, a missing directory included. An empty directory is a folder the
platform could not name, and nothing is ever written there.

## Templates

A template is a document a new file is made from, offered under its name: the node the file is
written with, through writeDocument, and the stem the file is made under where the template has
one of its own - the kind's otherwise. A template with no document makes an empty file. The
folder holds them in the order New lists them, and the application fills the list before any
page is built, because a page reads it once, when its buttons are made. With one template or
none, New is a plain button making that template or an empty file; with more, the face makes the
first and the strip lists them all. A file that still holds what a template put there holds no
work, and matchesTemplate says so - by reading the file into a clone of each template's document
and comparing - so a folder's isEdited asks it before it calls a file edited, and deleting a
document that was only ever made asks no question.

## Read

readDocument puts a file's document into a node, whole - nothing of what the node held before
survives it. Where the file cannot be read, the node is left holding the empty document, so a
page opened on a file that has gone comes up empty rather than on the last page's work under the
new name. The node is a Dom node rather than a value: the layer compares and copies nodes
without knowing what a document is, and the derived folder casts to the type it stated in the
tab schema.

## Rename

A rename runs while the editor that asked for it is still up, so it is given the file's path
rather than whatever holds it - a list of tiles can be rebuilt under it, and a tile or a page of
that list with it. The stem comparison that says "nothing to do" is case sensitive and the file
system is not, so a change of capitalisation reaches the disk and finds its own file there;
equivalent() is what lets it through. What is answered is the file's new name with the extension,
which is what a page and a tile are known by.

## Questions

A question is put about ONE thing, and documentInQuestionText says which words in the sentence
are that thing, in the spot ink. Said in one place so that two questions never mark it
differently. It is not always a name: a question about several documents names none of them,
and the count stands in the sentence where a name would.

Deleting asks once for the whole selection and settles all of it. Deleting the untouched files
first and asking about the rest would leave the user answering about a selection that is no
longer the one they made. Whether a file holds anything worth asking about is the folder's
answer, isEdited.

## Tiles

A tile is a button wearing the tool button look, its mark over its name. The caption is the name
the file goes by, so an editor over it is a rename, and a list with no edit handler connected
offers no Rename at all. Every tile takes the share of the lane it is handed, so the marks line up
whatever the names are, and a long name wraps under the mark, every line of it centred - the tile's
format, so an editor over it centres the same way. The list is the view alone; what scrolls it is
the host's, because a window gives the tiles the whole of its room.

The list comes back to a file name after a rebuild, not to a tile: every tile is taken down and
built again, and a document is the same document under the same name. A file whose tile does not
exist yet - just written, not yet reported - is named to selectFileNameAfterRebuild, and the
rebuild that reads the name spends it.

IDocumentTiles is what the home page works over, and DocumentsList is one answer to it: a flat
wrapping list of the folder's files. A list of another shape - groups, tiles with no file behind
them - answers it too, keyed by the same file names, and the home page is none the wiser.

## Pages

A page carries a tool bar over what it shows and answers the browser's questions before a tab
leaves it: whether it holds work the file has never seen, whether Save can write without asking
where, and how to put the work on the disk. The initiator a question of the page's own goes
under is what asked for the leaving - the crumb, the Up button, the tab being closed - so the
second question stands where the first did.

## DocumentsHomePageBase

The commands, worked over whatever IDocumentTiles the page that built them connects - once,
from its own constructor, after the tiles exist - through connectTiles; the handlers the tiles
are built with come from editHandler and menuHandler. Nothing here runs before that connection:
every command answers a press or a report. A derived page that owes the rebuilt tiles something
of its own adds it after documentsRebuilt.

New makes a file from the folder's first template, or an empty one where it has none - see
Templates - numbered past the names taken, and puts an editor over its tile once the rebuild has
made one - on a tick, because the rebuild that made the tile is still on the stack. Delete is
claimed by the page, so the tool bar button, the Delete key and the menu item run one
implementation against the page's selection. Open is the browser's command and the page names what
to open: the tile the user is on, which a right click has just moved the current item to. A press
on a tile opens it, and the keyboard's press is a press.

## DocumentsHomePage

The base with a list of the named type built as its scrolling body and connected - the whole of
what a flat list of documents needs, and the shape a list of another kind derives from.

## DocumentPage

Save writes the page's own file and says so beside the button that ran it. Save as asks for a
name, writes the work under it and takes the tab there; the original is left as it stands. A name
already taken is a question rather than a refusal. A page with no file of its own - one that
answers no to canSaveEdits - is saved as on Save, and on the leaving question's Save alike.

The folder is made again before either writes, since it can be deleted while a page stands open
on one of its files. A write that does not get there says so beside whatever asked for it - the
leaving question's Save included, and that question stays up.

## Unsaved

The saved document is the other side of the comparison, not a flag kept as edits arrive. The
tab's view state is where an edit lands and where it stays until Save, so a tab restored from
settings comes back holding edits the file has never seen - and a flag set as they were made
would have been left behind with the session that made them. The saved side is read into a clone
of the document node, so the two sides are nodes of one type; readSavedDocument is where a
derived page with a saved side that is not its file says so. A saved side that cannot be read is
work the disk does not hold.

## Browser

The home page lists the folder and a document page edits one file of it; the base answers the
browser's questions for both, and the template names the two page types - the home page as it
comes or derived from it, and the application's own document page - and builds them on the tab.
The home page is marked HasChildren as it is built, so its crumb carries a strip from the start
and a document page's carries none; the strip's list is fetched on every drop, because the
folder is written to while the application is up. A page is built for one page data and holds it
by reference, so a tab that has moved to another needs a new page even where the kind of page is
the same. Whether a page can be renamed is asked of the disk rather than of the folder's list:
that list is re-read when the watch reports, and the watch waits out a quiet period first.

## Application

The application owns or names the folder and hands it to run, which hands it to the main form,
and the browser and every page take it from their props. The name and the three config schemas
are the application's: the document's attribute in the tab schema is typed as the document is,
and the layer never names that type.

## History

A DocumentPage puts Undo and Redo after Save as HistoryButtons, and a derived page names the
history they walk with setEditHistory - once, from its own constructor. The page then answers
both actions and GetEditHistoryEvent for that history, and its buttons reach the page first, so
they walk the document wherever the focus stands. A key inside a box still reaches the box: a
script page names its CodeBox, so both come to the same steps, while the theme page names the
theme's own history and its code boxes keep theirs.
