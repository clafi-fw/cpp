# Browser

The words that no longer fit above a declaration.

## FetchState

How much is known about what lies under a page.
Unfetched is the state a page is built in: whatever sub-items it has are the ones
walked into on the way somewhere, and the browser has never been asked for the rest.
HasChildren says the browser knows there is something under this page without having named
it. Fetched says the sub-items are the whole set.

## SelectBrowserDataEvent

A crumb was chosen; the browser is being asked to go to that page.
The crumb comes with it. A browser that has to put a question before it moves - work
that is not saved - drops that question under the crumb, which is where the user pressed.

## GetPageIconSizeEvent

What a page's icon is. A zero size says the page has none, and closes the slot.
Asked of the root crumb when it is given its page, and of nothing else: the crumbs
after it are text. A line of a crumb's list holds a slot of its own size and asks only for
the paint.

## CanRenamePageEvent

Whether this page's name is the user's to change.
Asked of the crumb naming the page the browser is showing, as that crumb is given its
page, and of no other crumb. A page nobody says can be renamed carries no editor, so the
click that would have opened one goes on meaning what it means everywhere else on the bar.

## RenamePageEvent

The name typed over a crumb, on its way to whatever the page stands for.
The name is carried by the accept event, and so is the answer: a handler that will
not have the name calls AcceptEditEvent::refuse and says why, which keeps the editor
standing with the reason over it and the value still there to be corrected.

## FetchSubItemsEvent

A crumb is about to drop its list; the browser is being asked to name the pages
under this one.
Raised every time a crumb's list is opened. The bar keeps no list of its own, so
what a handler leaves in PageData::items is what the crumb shows, and whether an answer is
worth working out again is the handler's question - PageData::fetchState is where it
records that.

## ShowAnchorEvent

A tab has arrived at a url on this page, which shows the anchor it names.
A tab stands on a url - a page's path and an anchor inside the page, see UI-Types. goTo moves the
current tab to one, and the browser raises this on the page once the page is on screen and the
crumbs are laid out. It is raised when the url changed - another page, or another anchor on the
same page - and when this show built the page, whatever the url did: a tab restored from
settings, or a new tab opened on a url. A page just built has not been told its anchor.
Selecting a tab whose page already stands raises nothing, so the page keeps the place it was left
at. A goTo naming the url the tab already stands on raises nothing either.
An empty anchor names the page itself, and what the page shows for it is the page's own choice.
A pick inside a page that names one of its anchors goes to the browser as a url rather than being
shown on the spot. The browser records where the tab is, and the page shows what this event
names - so the url a tab stores is always the place its page shows.

## HistoryEntry

How a move keeps the url it leaves.
A tab keeps two stacks of urls: back, nearest last, and forward, nearest first. Push puts the url
left on the back stack and drops the forward stack - what a move the user asked for does. Replace
forgets the url left and keeps the forward stack. A page stepping through its own anchors a key at a
time replaces, so a walk leaves one entry behind rather than one per anchor passed over. An entry
that a replace leaves naming the url on screen is dropped, since a step to it would go nowhere.
Only a move that changes the url is kept. Selecting a tab, a goTo naming the url the tab stands on
and a move canLeavePage refused all record nothing.
Back and Forward travel the stacks through the same move, so they ask canLeavePage and raise
ShowAnchorEvent like any other. A right click on either button drops the stack as a list, nearest
first; a travel of several steps carries the urls passed over to the other stack in the order they
were visited. The back stack holds 50 urls, and the oldest go first.
Browser::Actions::back and forward run on Alt+Left and Alt+Right, deferred as Open is. A side
button of the mouse travels wherever the pointer stands inside the browser. A text box with a jump
history of its own answers Alt+Left first.
Entries are urls, so a page a fetch drops stays named by its path, and travelling back to it builds
it again as goTo builds any path. A rename rewrites the entries at or under the renamed page, in
every tab - reading the file of a tab not shown in this run, whose history names paths too.
The stacks are kept in the tab's own file, and read from it the first time the tab is selected.

## BrowserSettings

Where the browser's settings are kept, and when each part reaches the disk.
The application config carries the selected tab's id and an entry per open tab - id, title
and url, plus what the application adds to an entry - which is what is shown of a tab that
has not been opened. The Themes app keeps what a tab's icon is drawn from there. Nothing of
the page is there.
What a tab keeps is in its own file, in the OpenTabs folder beside the config file, named by
the tab's id with the config file's extension: the browser's history, k_tabFileSchema, and the
application's tab schema beside it. The file is read the first time the browser or the page
asks for it, which is when the tab is shown; a tab never shown in a run has its file neither
read nor written. A tab the user opens starts with an empty file, and one a closed tab left
under the same id is never read - the next save writes the new one over it.
The tab files are written when the application config is - BrowserSettings answers the config
document's SaveEvent - each one that was asked for, and every file in the folder that no entry
names is removed then. So closing a tab changes nothing on the disk until the next save, and a
run that ends without one leaves the files the last written entries name. BrowserApplication
finalizes from its own destructor so that the exit save still finds the settings standing.
