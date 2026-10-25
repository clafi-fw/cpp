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

## BrowserControl

A mouse close holds the sum of the tabs' widths in m_heldTabsWidth. alignContent takes what the
tabs stand past it by through fitTabs, the levelling the title's own shortfall uses, and the larger
of the two is taken. This is Chrome's tab closing mode in the strip's terms. Chrome freezes the
width its tabs may take, less the closed tab's for any tab but the last, and lays them out again
when the pointer moves out of the strip and 40px of slop below it
(TabContainerImpl::UpdateClosingModeOnRemovedTab, kTabStripAnimationVSlop). The title bar with the
crumb bar under the tabs stands for that region here. Oj asked for it on 2026-10-25, from the TODO
closeTab carried.

- **A sum rather than a width per tab** lets the last close widen the tabs before it into its
  place, as Chrome does, by the same rule that keeps the others still.
- **The sum is added up in strip order** at the close and at the align after it, skipping the
  closing tab rather than subtracting it. Over tabs at their own widths the two sums agree to the
  bit, and the hold takes nothing off. A subtraction can leave a stray ULP, which would shave the
  widest tab and align the title a second time on every pass.
- **Closing the open tab** keeps room for the tab opened in its place to grow to an open tab's
  minimum, the 48.8 of Tab::fitData - Chrome's next-active width. Without it every other tab of a
  strip that narrow shrinks a little on each such close.
- **The release listens to nestedControlHovered, never to hoverLeave.** The title's drag room
  answers HTCAPTION on Windows, so a pointer moving onto it ends the client area's TrackMouseEvent
  with WM_MOUSELEAVE - and the drag room is where the pointer lands after a last tab closes. A
  hover given up is never heard, so a pointer that leaves the window keeps the tabs held until it
  lands in the browser outside the title bar.
- Open: a scale change while the tabs are held leaves the sum in the old pixels until the release.
