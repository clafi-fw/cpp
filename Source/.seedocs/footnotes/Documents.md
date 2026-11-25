# Documents

The words that do not fit above a declaration.

A documents application is a browser over folders of files of one kind. A storage page lists its
folder as tiles, a document page edits one file, and the tab holds the document as it is on screen.
The application states the kind, the editor control and what its tool bar adds.

## StorageDefault

Give each default storage an absolute folder location, an optional display name and its read-only
setting. A read-only default also protects a registration the user already has for that folder.

## DocumentKind

**Give the words as a sentence uses them** - "script", "scripts", "New Script". Titles and
messages are built from them: "New script", "Script saved".

## DocumentTemplate

**New opens a menu when the folder offers several templates.** The menu lists them in folder order.
With one template, New uses it directly. With none, New creates an empty file.

## DocumentsFolder

**The derived folder states the format** - readDocument, writeDocument, drawIcon and isEdited.

**The application owns the format and primary folder.** Its derived `DocumentsFolder` is built
after the platform and outlives the browser. The browser uses its format for each storage.

**The list is read on demand.** A report from the directory watch tells every listener, and the
next files() reads the disk. A missing directory reads as an empty list, and filesRead is told so.

## DocumentsFolder defaultStorages

Return the application's default folders. The entries must outlive the browser. The browser checks
the list at startup and remembers disconnected defaults and the IDs used by restored tabs.

## DocumentsFolder saveDocument

**Save into a storage through this function.** It checks that the storage permits writes and that
the path is a direct file in its directory, then calls the format's `writeDocument`.

## DocumentsFolder detach

**A detached folder remains available to pages that hold it as their original source.** Its list
is empty and writes are disabled. The folder stops reporting directory changes.

## DocumentStorageView

**The folder prop identifies the storage a browser list displays.** The `primary` flag identifies
the main home view, which can also show items supplied by the application.

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
share of the lane, so the marks line up. A long name wraps under the mark, each line centered.

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

- New creates the file and opens its tile for renaming. When several templates are available,
  choose one from the New menu first.
- Add storage opens the properties dialog from the browser home. Properties opens it for the
  current storage tile. Name defaults to the folder name. Location can be typed or browsed, and
  Read-only states the storage's access.
- Delete on a storage asks whether to remove its registration or also delete its folder and contents.
  The disk option is available for writable folders that contain no other registered storage.
- Delete serves the tool bar button, the Delete key and the menu item alike, over the selection. It
  asks once, and only where isEdited says a selected file holds work.
- Open acts on the current tile. A right mouse click makes the tile under the pointer current.
- Return, Space and a mouse double click open a tile. With Ctrl or Shift, the key selects instead.
- Middle-click a document or folder tile to open it in a background tab.

## DocumentsHomePage

With the default DocumentsList it shows a flat list of storage tiles on home and document tiles
inside a storage. `StorageTilesView` supplies the current entries on home.

## DocumentPage

- Save writes the page's file and says so beside the button.
- Save as writes under a new name and takes the tab there, leaving the original as it was.
- Save uses Save As when the page's file is read-only. The dialog lists writable storages and asks
  for the destination and name.
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

**The home crumb's strip lists the configured storages.** Each storage has a path segment and a
crumb whose strip lists its own files. Both strips fetch on every drop. A primary document's
unqualified path resolves to that document inside the primary storage.

**A page holds its page data by reference**, so a tab that moves to another gets a new page, even
of the same kind.

## DocumentsBrowserBase documentFileOf

**It asks the disk**, since the folder's list waits for the directory watch. The path is empty for
storage pages, detached folders and documents whose files have gone. Rename also checks whether
the storage allows writes.

## DocumentsBrowser

- `HomePageType` - DocumentsHomePage, or another page derived from DocumentsHomePageBase.
- `DocumentPageType` - the application's own DocumentPage.

## DocumentsApplication

**The application hands its folder to run**, and the browser and every page take it from their
props. The name, the three config schemas and the main form's minimum and preferred sizes are
the application's. In the tab schema, the document's attribute has the document's own type.
