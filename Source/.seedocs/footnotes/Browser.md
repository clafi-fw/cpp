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
`IconOnly` with the title as the hint.

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

**Send a pick of one of the page's anchors through the browser** as a url, and show the anchor when
this event names it. Then the url a tab stores always matches what its page shows.

## HistoryEntry

A tab keeps a back stack of 50 urls at most, and a forward stack.

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

**Entries are urls**, so travelling to a page a fetch dropped builds it again. A rename rewrites
the entries at or under the page, in every tab. The stacks live in the tab's own file, read when the
tab is first selected.

## resetToHome

**For a browser whose whole tree is replaced** - another project opened in the same window. The
tabs go with their settings and history. Home is built again as initialize built it, so its list is
fetched afresh, and the new tab is selected.

## BrowserSettings

**The application config** holds the selected tab's id and an entry per open tab - id, title, url
and what the application adds. A tab not yet opened shows from that entry alone.

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
