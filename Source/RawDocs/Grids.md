# Grids

The words that no longer fit above a declaration.

## Grid

A grid that is described rather than assembled. The design is not a separate
object applied to a grid - it is the grid's own argument list:

    Dt::Grid& m_grid{ parent.add<Dt::Grid>(
        themeMetrics().page,
        themeColors().section,
        Columns{ ... },
        Header{},
        Rows{ ... }) };

Properties and nodes share one pack. Properties are forwarded to Rt::Grid and
handled there exactly as they would be on a plain grid; the nodes are picked out
by type and applied in the constructor body, once Rt::Grid and its descriptor
are complete.

Column nodes are always applied before row nodes, whatever order they are
written in, because cells resolve their columns by Tag. Row nodes keep their
relative order.

The node tree dies with the constructor and no bookkeeping outlives it. Columns
carry their Tags and rows own their cell connections, so everything the design
produced is reachable through the grid itself - columnByTag, forEachRow.

## RowGroupSpan

A RowContainer, so its cells hold controls the way any row's do. One control here
stands beside every row of the group, which is what a choice belonging to the group as
a whole looks like: the cell is stretched over them, so the control is theirs jointly.

In a group that folds, the span is what opens and closes it, and what stays while it is
closed - see Collapsible.

## RowContainer

A row whose cells hold controls. Every control a row holds is MouseOnly, so the row holds the
focus for them and hands the control in the selected cell every key the grid does not move by,
and what is typed. The grid keeps the arrows, Home, End, PageUp, PageDown and Tab - unless Alt is
held, which makes the key the control's: Alt+Down drops a combobox's list.

A key the control leaves goes on as it would have. Return and Space are the exception: they press
the control, the way FocusNavigator presses a control that has the focus, so Return on a
checkbox cell toggles it and on a combobox cell drops its list or opens its editor.

## RowStop

Where a key lands on a row that holds no cell to land on. The control is what takes the
focus - an expander's button, which opens and closes it. The rect is what the move is
scored against, and is the header's rather than the button's: a button sits in one corner
of its header, and a stop the eye reads as a whole row must be reachable from any column.

## MovingText

Whether a column's cells say something different from one paint to the next. A cell's
text is shaped through the TextEngine's cache, which is addressed by what a text says: a
value rewritten every paint misses on every paint, takes a fresh entry each time, and
evicts the static layouts every window draws from. The cells of a Yes column are shaped
into a layout of their own for the one paint or measurement and discarded with it, so
the cache never sees them. Per column, because a column is what says where a cell's text
comes from.

## GetCellTooltipEvent

The hint for one cell, asked while the pointer rests on it. It goes to the pair
GetCellTextEvent goes to - the row, then the grid - and a handler writes into text(),
which is the tooltip's own buffer. The tooltip event underneath is reachable for its
placement and anchor: the anchor arrives set to the cell's rect, and the placement to
the pointer. A cell nobody wrote for falls back to repeating the words its column cut.

## Column

Column{ Tag{ ColumnTag::Hue }, Text{ L"Hue" }, Column{ ... }, Column{ ... } }

Props are forwarded verbatim to ColumnCollection::add(), so everything that
works there today (Tag, Text, ShowInHeader, ColumnWidthMode, widths, colours,
TextAlign) works here unchanged. Nested Column nodes become sub-columns.

The Tag is read but not consumed: the design uses it to resolve cells and to
check uniqueness, and add() receives it like any other property.

## Columns

Columns{ ... } - an anonymous group. Its children are spliced into the
enclosing collection, so it adds nesting in the source without adding a
level to the grid.

## Cell

Cell{ tag }                          - row fills this column, text via the grid event
Cell{ tag, [](GetCellTextEvent&){} }  - row fills this column, text from the lambda
Cell{ tag, this, &Page::method }      - the same, bound member function

## CellSet

CellSet{ ... } - a reusable bundle of cells. Declare once as a const
object and reference it from every row that shares it.

## Row

Row{ ...props..., Cell{ ... }, CellWith<T>{ ... }, someCellSet }

Props are forwarded to GridBase::add<Rt::RowContainer>(), so Tag and any
other row property still applies. The row is an ordinary RowContainer: the
design contributes no row type of its own.

## Expander

Expander{ Header{ Text{ ... } }, ...rows... }

An expander's header is one text control, so its Header takes text and nothing
else. A cell there is a mistake with its own message.

Init<Rt::RowExpander>{ ... } and OnEvent{ ... } run against the expander once its header
is written, before its rows are applied.

## Group

Group{ Span{ Tag{ ... }, Cell{ ... }, CellWith<T>{ ... } }, ...rows... }

A group's span is a row, sectioned by the same columns as every other row, so its
Span takes a Tag, cells and controls, and they are applied exactly as a Row's are.
The span's cell is stretched over the rows beside it, so a control there is the one
the whole group answers to - a per-group choice goes here rather than in every row.

Group{ Collapsible::Expanded, Span{ ... }, ...rows... } folds its rows away under the
span - see Collapsible.

Init<Rt::RowGroup>{ ... } and OnEvent{ ... } run against the group once its span is
described, before its rows are applied. The folding is the span's to report, so an Init
that follows it connects to group.span().

## Collapsible

Whether a group's rows fold away under its span, and whether the group starts open.
Collapsible::No, the default, is a group that stays open. The span holds the open state -
expanded(), setExpanded(), toggleExpanded() - and emits ToggleExpandedEvent, the event an
expander's header emits, so a host answers both the same way. RowGroup answers it by
showing and hiding its body.

THE MARK LEADS THE SPAN'S FIRST CELL, the first column the span fills, left to right. It
is an ExpanderButton the size of a check mark, standing where the column puts a line of
text, and the cell's content starts after it. RowBase::cellLead is where a row keeps that
room, and everything that finds a cell's content box reads it: calculateCell, paintCell,
the tooltip, the in-place editor and the box a hosted control is given.

A CLOSED GROUP IS ITS SPAN ALONE, with a blank in every leaf column no span fills - this
span, in the leaf or in a column above it, or the span of any group this one stands in,
whose cells stretch over it. A column nothing fills has no width and gets no blank. Rows
that fill different columns fold into the same closed row, so a column none of them fill
shows a blank while the group is closed and a gap while it is open.

A BLANK IS THE CELL'S SURFACE AND LINES AND NOTHING ELSE - see RowBase::hasBlank. The
walk reports it so that the lattice closes, and everything that addresses a cell steps
over it: no text is asked for it, no editor opens on it, and no key or click lands there.

A CLOSED GROUP IS MET IN EVERY COLUMN. Its row reads whole, so each blank stands for the
cell of the row nearest to it, and that cell's lane is its rect widened over the blanks it
stands for - see RowBase::traverseLanes. Up and Down stop on the group whichever column the
run follows and land on that cell, keeping the column for the next press. A press on a blank
selects the same cell, and a double click there opens the group.

THE KEYS ARE A TREE NODE'S. On the mark's cell Right opens a closed group and Left closes
an open one. Every other press moves the selection as it moves anywhere in the grid, so
Right from an open group's mark steps into its rows, where a tree node's Right goes too.
The mark is MouseOnly like every control a row holds, so the row keeps the focus, and a
double click on it toggles twice, as a double click on an expander's button does.

## SubGrid

THE BODY OF A GROUP OR A SECTION. A grid only in that it holds rows: it stands inside the
grid whose columns its rows fill, and shares that grid's descriptor.

## RowGroupBase

A ROW HOLDING A GRID OF ROWS, under an expander's header or beside a group's span. A span
and the rows fill disjoint sets of columns, so the two are laid over each other rather than
stacked and the span's cells merge down the whole group - see RowGroup::alignContent. A
closed group is left stacked, its body hidden - see Collapsible.

It holds no cells of its own, so it PAINTS NOTHING - see RowBase::paintSurface. Its surface
is still the group's colour: the span and the rows of the body inherit it, add only what
they have of their own, and each paints it through its own cells, so a row that also holds
the selected cell applies the rule over it a second time. This is why the group is in
effect whenever any row inside it is - see RowBase::getControlState, which answers that for
every row alike.

## RowExpander

Its header is held against the top of the view while the rows of its body are scrolled
under it, resting under the grid's own header and under the header of any expander this
one stands in - see WithHeldHeader.

## GridRow

THE ELEMENT EVERY ROW OF A GRID WEARS, a group and a section among them -
RowBase::adjustPaint puts it on each. A row paints no rect of its own: its cells are filled
from its surface, so every rule in the set reaches the screen through them.

The rows inside a group or a section inherit its colour and paint it for it. Surface, Text,
Active and Active text are therefore applied once per level of nesting: a row inside a group
applies its own over the group's. Active stops short of a section, which declines the
selection - see RowExpander. Hovered and Pressed reach only the rows: a group and a section
are not interactive, and a control that is not reads as neither - see Control::visualState.

The column header keeps Active and Active text alone. Its surface, ink and flip are
Header's, and a column name answers no pointer of its own - see GridHeader::adjustPaint. A
divider wears Divider.

The lines between a row's cells are GridLine, drawn over each cell's own surface, so
GridLine stands on GridRow. A row draws no border of its own, and the set lists no Stroke.

## RowCell

One cell of one row, as the cell walk reports it: the column it belongs to, the rect it
fills measured from the origin the walk was given, the vertical section it sits in,
whether it stands at the row's leading edge, and whether it is a blank - see Collapsible.
Everything the walk knows and no more - a property of the row as a whole, such as whether
the row is the first of the grid, is the consumer's to carry.

## GridLines

Which of a grid's inner lines are drawn - the lines its cells and its dividers put between
one another. The frame around the grid is not one of them: it is the grid's own border, and
it stands whatever this says.

## AcceptCellTextEvent

The text an in-place edit was left with, on its way back to whatever supplies the cell.
It goes to the same pair GetCellTextEvent does - the row, then the grid - so a handler
that answers for a cell's text has one place to take the new one.

The text is const: the value in it is what the user typed, and this asks whether the
cell's source will have it. A handler that will not calls refuse() and says why - the
editor is still up, so the user is told and gets to correct it rather than losing the
edit to a write that went nowhere.
