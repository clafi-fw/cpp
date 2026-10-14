# Browser

The author's side of the footnote of the same name.

## BreadCrumbBar

The bar fits its crumbs itself, in alignContent, through IFittableList::shrinkBy - the levelling
the tab strip and the grid's columns use. A single lane of Stack hands every item its measured
width whatever room it was given, so a bar left to Stack ran its last crumbs under the search box.
Taking the shortfall in Stack would reach every Horizontal and Vertical stack in the tree, and rows
that clip today would shrink instead. Oj chose the bar alone on 2026-10-14, and widest first over
keeping the current page whole or folding the leading crumbs into an overflow crumb.

BreadCrumbBarItem::takeOffWidth charges the crumb alone. The IFittable contract asks for the
containers above to be charged as well, which matters where their width is read after the fit. The
bar's width is the one its slot hands it, and alignSingleRow reports the content from where the
crumbs end.

## BrowserSettings

BrowserApplication finalizes from its own destructor so that the exit save still finds the settings
standing.
