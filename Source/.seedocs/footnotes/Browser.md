# Browser

The words that do not fit above a declaration.

## FetchState

- `Unfetched` - the state a page is built in. Its sub-items are only those walked into on the way
  somewhere.
- `HasChildren` - the page is a list, even while empty. A fetch naming nothing keeps the mark, and
  the crumb drops a list reading "No items".
- `Fetched` - the sub-items are the whole set.

## SelectBrowserDataEvent

**The initiator is what was pressed** - a crumb, or a line of its list. A question the browser puts
before moving, such as one about unsaved work, drops under it.

## GetPageIconSizeEvent

**A zero size means no icon**, and closes the slot.

Only the root crumb asks, as it is given its page. A line of a crumb's list sizes its own slot
and asks only for the paint.

The view mode comes from BrowserControl::rootCrumbViewMode - `LeftIcon` beside the title, or
`IconOnly` with the title as the tooltip.

## CanRenamePageEvent

Asked only of the crumb for the page on screen, each time a gesture would open its editor. The
answer is never kept. A page without a yes has no editor.

## RenamePageEvent

**The name and the answer travel in the accept event.** To refuse, call AcceptEditEvent::refuse
with the reason. The editor stays open with the reason over it and the value still in it.

## FetchSubItemsEvent

**Raised each time a crumb's list opens.** The crumb shows what the handler leaves in
PageData::items. Whether to fetch again is the handler's call, recorded in PageData::fetchState.

## BreadCrumbBar

**A path longer than the bar gives up room from its widest crumbs.** They come down to one common
width, the way a tab strip's tabs do, and the next widest join them as the bar narrows further. A
crumb's floor is no width at all, so a bar narrow enough cuts every crumb.

## ShowAnchorEvent

Raised on the page when its tab moves to a Url, once the page is shown and the crumbs are
rebuilt:

- when the url changed - another page, or another anchor;
- when this show built the page - a restored tab, or a new tab opened on a url.

Selecting a tab whose page exists raises nothing, so the page keeps its place. A goTo to the url
the tab is already on raises nothing either. An empty anchor means the page itself.

**The position says whether the tab has stood here before.** Back, Forward, a tab restored from
the settings and a page built again (see BrowserControl rebuildPage) carry the position the page
stood at, and the page is put back there once it is laid out. A first arrival - a link followed, a
tab opened on a url - carries none, and the page shows the anchor.

**Send a pick of one of the page's anchors through the browser** as a url, and show the anchor when
this event names it. Then the url a tab stores always matches what its page shows. A pick that is a
selection on the page rather than a move goes through BrowserPage setAnchor.

## Visit

What a tab's history holds per step: the url, and where the page stood in view as the tab left
it, read through BrowserPage viewPosition. The tab's entry in the settings holds the same of where
the tab stands.

## BrowserPage setAnchor

**A selection is not a move.** The tab's url takes the anchor, the entry is stored, and nothing
travels: the history is left as it stands, but for a nearest entry naming the url now on screen,
which goes as a replace drops it. No ShowAnchorEvent is raised - the page knows where it is. So
the url a tab leaves by a link carries the item the user was on, and Back brings it back.

A page going down with its tab already off it is not heard: what a dying page says of its
selection is not the tab's.

## BrowserPage viewPosition

**The measure is the page's own.** The browser keeps the number and hands it back; what it
measures is for the page to decide, and the page reads it back as it was given. How far the page
is scrolled is one measure. SeeDocs measures from the item the url's anchor names - its top below
the viewport's top - so a page laid out at another width or height puts that item back where it
stood, and falls back to the scroll where the url names nothing.

Answered unscaled, so a value written by one run reads the same at another scale. Asked as the tab
leaves the page, as the tab's entry is stored, and in the browser's destructor for every page still
standing - a move in view after the last move between pages reaches the settings that way. A page
that scrolls nothing leaves the default.

## BrowserControl rebuildPage

**For a page whose source changed under it.** The page's position is read, the page goes, and the
selected tab's page is built again at the url and the position - an ordinary ShowAnchorEvent with
the position carried. Nothing is recorded. A tab not selected only loses its page, and gets it
back, at that position, as it is selected.

## HistoryEntry

A tab keeps a back stack of 50 visits at most, and a forward stack - see Visit.

`Replace` suits a page stepping through its anchors a key at a time, so the walk leaves one entry.
An entry a replace leaves naming the url on screen is dropped. `Push`, the default, suits every
other move.

**Only a move that changes the url is recorded.** Selecting a tab, a goTo to the current url and a
refused move record nothing.

**Back and Forward are ordinary moves** - they ask canLeavePage and raise ShowAnchorEvent. A travel
of several steps moves the urls passed over to the other stack, in visiting order.

- Alt+Left and Alt+Right run Browser::Actions::back and forward. A text box with a jump history
  answers Alt+Left first.
- A mouse side button travels from anywhere inside the browser.
- A right mouse click on Back or Forward lists its stack, nearest first.

**Entries are visits**, a url each, so travelling to a page a fetch dropped builds it again. A
rename rewrites the entries at or under the page, in every tab. The stacks live in the tab's own
file, read when the tab is first selected.

## resetToHome

**For a browser whose whole tree is replaced** - another project opened in the same window. The
tabs go with their settings and history. Home is built again as initialize built it, so its list is
fetched afresh, and the new tab is selected.

## BrowserSettings

**The application config** holds the selected tab's id and an entry per open tab - id, title, url,
where its page stands in view and what the application adds. A tab not yet opened shows from that
entry alone.

**Each tab's own file** holds the rest - the browser's history (k_tabFileSchema) and the
application's tab schema. It sits in the OpenTabs folder beside the config file, named by the tab's
id with the config's extension.

- It is read when the tab is first shown, so a tab never shown is neither read nor written.
- A new tab starts empty. The next save writes over any file a closed tab left under its id.

**Tab files are saved with the application config**, and files no entry names are removed then.
Closing a tab changes nothing on disk until that save.

## BrowserControl

**Closing the window asks about unsaved work when it would be lost.** A page's work waits in its
tab's view state for the next run, and that reaches the disk only where the config folder exists.
Where the folder is missing, the browser asks canLeavePage of every tab with a page, under that tab.
The first no keeps the window up. DocumentsBrowserBase asks Save, Discard or Cancel.
