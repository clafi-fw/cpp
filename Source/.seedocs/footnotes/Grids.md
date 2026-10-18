# Grids

The words that do not fit above a declaration.

## Dt::Grid

**The design is the grid's argument list:**

    Dt::Grid& m_grid{ parent.add<Dt::Grid>(
        themeMetrics().page,
        themeColors().section,
        Columns{ ... },
        Header{},
        Rows{ ... }) };

The properties go to Rt::Grid and work as on a plain grid. The nodes are applied once it is
complete - columns first, then the header, then the rows, wherever each is written. Rows keep their
order.

**The node tree dies with the constructor.** Reach what it made through the grid - columnByTag,
forEachRow.

## Grid

**A grid follows the user as a container of its own**, keeping the selected cell and answering
the keys inside it: the arrows, Home, End, PageUp and PageDown. Built with Interactivity::None it
stands in the container above it instead - a page holding the one selected item: its rows are
that container's items, the selection goes where the container's item goes, the four keys that
address a container are the container's, and Up and Down travel on past the first and the last
row into the rest of it - see StackBase.

**The arrows read the cells as a table.** The edges the cells share are its lines - bands down,
columns across - and a merged cell, a group's span or an expander's header covers more than one
of either. A step goes to the first cell on the line the run keeps that lies past the whole of
the selected one, so a run of Down keeps its column across rows with nothing in it, and a step
back always returns where it came from. Where nothing ahead stands on the line, the step goes to
the nearest band ahead holding anything, at the column nearest the line.

- Left and Right rest at the ends of a line. Up and Down hand the key on past the first and the
  last row.
- Home and End go to the ends of the band the selected cell is on.
- A selection scrolled out of view is still where a key moves from.

## RowGroupSpan

Its cells stretch over every row of the group, so a control in one is the group's - a choice the
whole group shares. In a group that folds, the span opens and closes it - see Collapsible.

## RowContainer

**The row holds the focus for its controls**, which are all MouseOnly. The control in the
selected cell takes what is typed, and every key the grid leaves:

- The grid keeps the arrows, Home, End, PageUp, PageDown and Tab. With Alt held they go to the
  control - Alt+Down drops a combo box's list.
- Return and Space press the control where it leaves them. A check box toggles, and a combo box
  drops its list or opens its editor.

### Picking

**A mouse press on a control picks its cell first.** Then:

- A check box or a button acts on the same press.
- A popup the control drops implicitly - a combo box's list from its main area, an in-place
  editor - waits for a press on the cell already selected. In a `SelectionMode::Multi` grid the
  row must be held too.
- A part whose one job is its popup, such as a dropdown strip, drops it from the picking press.
- Ctrl or Shift in a `SelectionMode::Multi` grid make the press a selection change alone, and the
  cell refuses every popup.
- A key's click - Return or Space handed to the control - is always allowed its popup.

The click reaches the grid as a click on the row.

## Editing a cell

**A click on the cell the user is already on opens its editor**, as it does on a tile: the first
click picks the cell, the next edits it. The editor opens on the release, so a press that becomes
a drag - a carry of the row - opens nothing. F2 and Return open it from the keyboard. A cell
holding a control is that control's, and its popup waits for the same second click.

## RowContainer isChildWidthGiven

**A Fixed or Fill column gives its control its width.** A FitContent column reads the width off
the control, so the control keeps what it measured and has no outside width to wrap at - see
Control::isWidthGivenFromOutside.

## Selection from code

`selectCell` puts the selection where a pick would - in a `SelectionMode::Multi` grid, on that row
alone. A null column selects the row with no cell. The row is made the current item of whichever
container keeps one - the grid, or the container a grid built with Interactivity::None stands in -
see Control makeCurrentItem.

**The focus stays where it is.** To move it there, call `setFocus` on the row, then
`StackView::setSelection` for a wider held set - a focus arriving outside the set reads as a pick.

## RowStop

`control` takes the focus - an expander's header. `rect` is what a key move is scored against. An
expander gives its whole header, so the stop is met from any column.

## MovingText

**Set `Yes` where a cell's value changes on every paint.** The TextEngine caches layouts by what a
text says, so such values miss every time and evict the layouts every window draws from. The cells
of a `Yes` column bypass the cache.

## MaxLines

A cut cell is as tall as its first lines and fades the last of them. Its tooltip repeats the lines
past the cut, up to Tooltip::k_maxLines, and the in-place reader shows the whole value.

## GetCellTooltipEvent

Asked of the row, then the grid. Write the tooltip into text(). The anchor arrives as the cell's
rect and the placement as the pointer - change them through tooltip(). A cell nobody writes for
repeats the words its column cut.

**A link has a tooltip of its own.** link() names the link under the pointer, as the cell's text
states it, and is empty off a link. The tooltip is asked again where it stands as the pointer moves
onto a link, off one, or from one to another.

## CellLinkClickEvent

A cell's text carries links as any Text does - PushLink around the words. A mouse click on one
follows it. Return follows the selected cell's first link, which stands for the cell, in a column
that opens no editor; a column that edits keeps Return for the editor and follows on Ctrl+Return.
The click goes no further up, so the grid's own click handling never sees it.

## CellLinkContextPopupEvent

Raised from the keyboard, it is about the first link in the selected cell, and the menu drops under
that cell.

**A handler that shows a menu stops `popup`**, and the request goes no further up. Left unstopped,
it goes to the grid, then on up as from anywhere else on the row.

## Dt::Column

    Column{ Tag{ ColumnTag::Hue }, Text{ L"Hue" }, Column{ ... }, Column{ ... } }

Properties go to ColumnCollection::add() as written. A nested Column is a sub-column.

**Cells find their column by Tag**, so two columns may never share one - debug builds check. A
column with no Tag takes no cells from the design.

## Column cellTextFormat

**The column's TextAlign comes first, then its TextFormat**, so an alignment the format states
wins. The format reaches the cell's paint, tooltip and in-place editor. The cell's text holds the
value alone - see TextFormat.

## Dt::Cell

Where the text comes from:

- `Cell{ tag }` - the grid's OnGetCellText.
- `Cell{ tag, [](GetCellTextEvent&){} }` - the lambda, then the grid's OnGetCellText unless the
  lambda stops the event.
- `Cell{ tag, this, &Page::method }` - the member function, as the lambda.

## CellSet

**Declare it once as a const object**, and list it among the cells of every Row that shares it.

## Dt::Row

    Row{ ...props..., Cell{ ... }, CellWith<T>{ ... }, someCellSet }

Properties go to GridBase::add<Rt::RowContainer>().

## Expander

    Expander{ Header{ Text{ ... } }, ...rows... }

The Header takes a Text alone - a cell there fails to compile. Init<Rt::RowExpander> and OnEvent
run once the header is written, before the rows.

## Group

    Group{ Collapsible::Expanded, Span{ Tag{ ... }, Cell{ ... }, CellWith<T>{ ... } }, ...rows... }

**The Span is a row**, and takes a Tag, cells and controls as a Row does - see RowGroupSpan.
Collapsible::Expanded or Collapsed folds the rows under it.

Init<Rt::RowGroup> and OnEvent run once the span is described, before the rows. The span reports
the folding, so an Init that follows it connects to group.span().

## NewItem

    NewItem{ PlaceHolderText{ ... } }

It adds a RowNewItem, whose one cell says the text.

## Collapsible

- `No`, the default - the group stays open.
- `Expanded` - the rows fold away, and the group starts open.
- `Collapsed` - the rows fold away, and the group starts closed.

**The span holds the open state** - expanded(), setExpanded(), toggleExpanded() - and emits
ToggleExpandedEvent as an expander's header does. Its mark, an ExpanderButton, leads the first
cell it fills.

**A closed group is its span alone.** Each leaf column no span fills - this group's or an outer
one's, at that leaf or above it - shows a blank. A blank stands for the span's nearest cell:

- Up and Down land on that cell whatever column they follow.
- A mouse press on a blank selects that cell, and a double click there opens the group.

**The keys are a tree node's.** On the mark's cell Right opens and Left closes. Every other key
moves the selection as anywhere in the grid. A double click on the mark toggles twice.

## RowBase cellLead

The measure, the paint, the tooltip, the in-place editor and a hosted control all leave this room at
the cell's start. A group's span keeps its mark there.

## RowBase hasBlank

**A blank is a cell's surface and lines alone.** The cell walk reports it so the lattice closes,
and everything that addresses a cell steps over it - text, editors, keys and mouse clicks.

## RowBase traverseLanes

A cell's lane is its rect, widened over the blanks it stands for - each blank goes to the cell
nearest it across the row. The blanks themselves are left out.

## SubGrid

A grid only in that it holds rows. It shares the descriptor of the grid it is in.

## RowGroupBase

It holds a head, and a body that is a SubGrid of rows. Closed, it shows its head alone. An
expander's header stacks above the body. A group's span lies over it, on different columns, so the
span's cells run down the whole group.

**It paints nothing.** Its colour shows through the cells of the rows inside it, so a group is
selected whenever one of them holds the selected cell - see UiElement::GridRow.

## RowExpander

Its header rests under the grid's header and any outer expander's - see WithHeldHeader. It is
never selected, so the selected rule stops at the rows inside it.

**Shut over the selected cell, its header stands for the cell** - the stop the next key moves on
from, and the current item of the container the grid stands in.

**Entered from above, it lands the focus on its header; entered from below, in its rows** - see
Control navigationEntry. Shut, it has only the header to land on. The steps of a page press walk
it the same way: Down from the header into the rows, Up from the rows back to the header.

## RowNewItem

Add it with GridBase::addNewItemRow, or NewItem in a design. A grid holds one, and rows added later
go ahead of it.

**A click on its cell raises NewItemEvent** - a mouse click, or Return or Space. The cell spans
every column and behaves as any cell, showing the text muted after a plus.

**It is a command, not an item**, so the selection never holds it - Delete and the like pass it by.

## NewItemEvent

The handler adds the item's row. The grid then selects that row's leading cell, focuses the row
and scrolls it into view. A handler that adds nothing leaves the selection where it is.

## GridRow

**Every row of a grid wears it**, groups and sections included. Each part draws its lines in its
own element's stroke:

- GridRow - the lines between a row's cells, raised off each cell's surface
- GridHeader - the column header and its lines
- SectionHeader - a section's header and the line under it
- Divider - a divider and the line it closes with
- Grid - the grid itself, and the frame around it

**A row paints through its cells**, with no rect of its own. The rows inside a group or a section
paint its colour for it, so rules apply once per level of nesting. Selected stops short of a
section, and Hovered and Pressed reach the rows alone.

A stroke that reaches nothing is transparent, and the line's gap stays open.

## RowCell

- `rect` - what the cell fills, measured from the origin passed to the walk.
- `sectionIndex` - the vertical section it sits in.
- `isFirstColumn` - whether it is at the row's leading edge.

## GridLines

The inner lines are those cells and dividers draw between one another. The frame around the grid
and the header row's lines are drawn whatever this says.

## AcceptCellTextEvent

It goes to the row, then the grid, as GetCellTextEvent does - so whatever supplies the text takes
the new one.

**To turn the text down, call refuse(why).** The editor stays open, so the user reads the reason
and corrects the edit.

## CellWith

    CellWith<ComboBox>{ tag, ...ctor args..., OnEvent{ ... }, Init<ComboBox>{ ... } }

The arguments are held by value - wrap anything expensive or shared in std::ref.
