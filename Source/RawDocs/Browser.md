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

## BrowserSettings

Where the browser's settings are kept, and when each part reaches the disk.
The application config carries the selected tab's id and an entry per open tab - id, title
and path, plus what the application adds to an entry - which is what is shown of a tab that
has not been opened. The Themes app keeps what a tab's icon is drawn from there. Nothing of
the page is there.
What a page keeps is in the tab's own file, in the OpenTabs folder beside the config file,
named by the tab's id with the config file's extension and laid out by the application's tab
schema alone. The file is read the first time the page asks for it, which is when the page is
shown; a tab whose page is never shown in a run has its file neither read nor written.
The tab files are written when the application config is - BrowserSettings answers the config
document's SaveEvent - each one that was asked for, and every file in the folder that no entry
names is removed then. So closing a tab changes nothing on the disk until the next save, and a
run that ends without one leaves the files the last written entries name. BrowserApplication
finalizes from its own destructor so that the exit save still finds the settings standing.
