# Documents

The words that do not fit above a declaration.

A documents application is a browser over one folder of files of one kind. The home page lists
the folder as tiles, a document page edits one file, and the tab keeps the document as it is on
screen. The application states the kind, the editor control and what its tool bar adds.

## DocumentKind

**Give the words as a sentence uses them** - "script", "scripts", "New Script". Titles and
messages are built from them: "New script", "Script saved".

## DocumentTemplate

**New lists the templates in the folder's order.** With more than one, New's face makes the first
and its strip lists them all. With none, New makes an empty file.

## DocumentsFolder

**The derived folder states the format** - readDocument, writeDocument, paintIcon and isEdited.

**The application owns the folder.** It is built after the platform and outlives every window.
Hand it on as a `DocumentsFolder*` prop - the exact type Props::get looks for.

**The list is read on demand.** A report from the directory watch tells every listener, and the
next files() reads the disk. A missing directory reads as an empty list, and filesRead is told so.

## DocumentsFolder templates

**Fill it before any page is built** - a home page shapes its New button from it.

## DocumentsFolder createFile

Returns the new file's name, or an empty string where nothing was written.

## DocumentsFolder matchesTemplate

**Call it from isEdited**, so deleting an untouched new document asks no question. It reads the
file through readDocument and compares it with each template's document.

## DocumentsFolder readDocument

**Replace the node's whole content**, and leave the empty document there when the read fails. A
page opened on a file that has gone then comes up empty, not on the last page's work.

The node is a Dom node. Cast it to the type the tab schema states.

## DocumentsFolder renameFile

**It takes the file's path**, since the tiles can be rebuilt while the editor is up. A change of
capitals alone renames the file. The name it returns carries the extension, which pages and tiles
go by.

## documentInQuestionText

**Every question marks its subject through this.** The subject is one document's name, or a
count - "2 scripts".

## DocumentTile

**Its caption is the file's stem**, so editing it in place renames the file. Every tile fills its
share of the lane, so the marks line up. A long name wraps under the mark, each line centred.

## IDocumentTiles

**Tiles are keyed by file name.** A rebuild builds every tile again and comes back to the file the
list was on. A list of another shape - groups, or tiles with no file behind them - uses the same
file names.

## DocumentsList

Rename is offered only where the list is given a DocumentEditHandler.

## DocumentsBasePage

**Before a tab leaves the page, the browser asks hasUnsavedEdits.** On yes it offers Save, Discard
and Cancel, and Save calls saveEdits with that question's Save button as the initiator. A question
of the page's own then stands over the browser's.

## DocumentsHomePageBase

**The derived page connects its tiles once**, from its constructor, through connectTiles. It builds
them with editHandler and menuHandler. Every command waits for that connection.

- New makes a file from the first template, or an empty one, and opens an editor over its tile.
- Delete serves the tool bar button, the Delete key and the menu item alike, over the selection. It
  asks once, and only where isEdited says a selected file holds work.
- Open acts on the current tile. A right mouse click makes the tile under the pointer current.
- Return, Space and a mouse double click open a tile. With Ctrl or Shift, the key selects instead.

## DocumentsHomePage

With the default DocumentsList it is the whole home page of a flat list of documents.

## DocumentPage

- Save writes the page's file and says so beside the button.
- Save as writes under a new name and takes the tab there, leaving the original as it was.
- A name already taken asks before replacing the file.
- A page with no file of its own answers no to canSaveEdits, and its Save works as Save as.

**A write makes the folder again first**, in case it was deleted. A failed write says so beside
what asked for it, and the leaving question stays up.

## DocumentPage hasUnsavedEdits

**It compares the document with the saved one, read afresh**, so a tab restored from settings
reports its unsaved edits too. A saved side that cannot be read counts as unsaved. A page whose
saved side is not its file overrides readSavedDocument.

## DocumentPage loadDocument

**The browser calls it when a tab arrives at a document page**, unless the tab was restored onto
it - its view state holds unsaved edits. It reads through readSavedDocument.

## DocumentPage setEditHistory

**Call it once, from the derived page's constructor.** The page then answers Undo, Redo and
GetEditHistoryEvent for that history. Its Undo and Redo buttons act on it wherever the focus is,
and a key inside a box still reaches the box.

- A script page names its CodeBox, so keys and buttons walk the same steps.
- The theme page names the theme's own history, and its code boxes keep theirs.

## DocumentsBrowserBase

**The home crumb's strip lists the folder's files**, fetched again on every drop.

**A page holds its page data by reference**, so a tab that moves to another gets a new page, even
of the same kind.

## DocumentsBrowserBase documentFileOf

**It asks the disk**, since the folder's list waits for the directory watch. The path is empty for
the home page and for a document whose file has gone. A page can be renamed only where this
returns a path.

## DocumentsBrowser

- `HomePageType` - DocumentsHomePage, or another page derived from DocumentsHomePageBase.
- `DocumentPageType` - the application's own DocumentPage.

## DocumentsApplication

**The application hands its folder to run**, and the browser and every page take it from their
props. The name and the three config schemas are the application's. In the tab schema, the
document's attribute has the document's own type.
