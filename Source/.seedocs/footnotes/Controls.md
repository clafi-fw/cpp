# Controls

What a declaration's one line has no room for.

## ReadOnly

A read-only box still takes the focus, shows a caret, selects and copies. A disabled box answers
none of that, and its text cannot be copied out.

## DropdownEvent

**The handler owns the popup** - what it holds and when it closes. A SplitButton given a
dropdownAction runs that action instead, and raises no event.

## SplitButton

- **Given a dropdownAction**, the strip runs that command, and OnDropdown is not raised. The strip
  stays available whether or not the face's command can run.
- **Given none**, the strip raises OnDropdown and greys with the face, so a list about a command
  that cannot run - the steps behind an empty Undo - is never offered.

## ComboBox

**A pick never changes the size.** The face is as wide as the widest text it would show for any
item, so nothing beside the combo box moves. A window sized once - the application menu - would
otherwise cut a face that grew.

**The list is sized by its items**, as a Menu is. A list wider than the screen is cut to the work
area. Its window then grows by the horizontal bar, so ten rows still show ten rows. A side too short
for the list cuts the window as well, and the vertical bar comes up - see ScrollBars::Auto.

## ComboBox measureText

The face measures the text it would show for every item, and its current text, and answers the
widest. The current text counts because a derived control may word the face unlike any item -
WhatsClip's pickers write "(Auto)" after the name they found.

## ComboBoxAcceptTextEvent

Raised for a text typed over an editable face, unless it is the value the editor opened on.

- **A handler that takes the text** stops the event - the Themes app reads `+0.02` as a rule.
- **A handler that refuses it** calls `accept.refuse`, and the editor stays up with the reason.
- **Otherwise** the item named by the whole text is picked, ignoring case, or else the first name
  starting with it. A text that finds none is refused.
- **An empty text** picks the item that stands for no value, where there is one - see
  PlaceHolderText.

The editor opens on the item's text as AdjustItemTextEvent words it, in plain text.

## DropdownMaxSize

By default the list grows until its placement's room stops it, and a scroll bar comes up for the
rest.

## PlaceHolderText

**The item with an empty text stands for no value.** The face and the list show its placeholder,
muted, the editor opens empty over it, and an emptied value picks it - the Themes app's No change
is one. Typing the placeholder's first letters still finds the item.

## TextBox placeHolderText

The caret stands over the placeholder, and the box is measured to fit it while its text is empty.

## DialogAnswerEvent

**keepOpen leaves the question up**, with its answers where they were. A handler calls it when it
cannot act on the answer yet - a Save that asks where the work goes and is told no there.

A dialog raised on `button`, the answer pressed, stacks on top of this one, which stays on screen
behind it.

## Menu

**Build it on the stack**, fill it, then run it - execute returns once the menu has closed. An empty
menu stays closed, unless setPlaceHolderText gave it a placeholder to show.

## HistoryButton

The strip drops a StepsMenu of the IEditHistory the face's click would act on, and walks the run
picked in one go. It greys with the face, and drops nothing where no history answers.

## TextBox history

**A text box is its own IEditHistory**, with no steps while it is read-only. A walk of several
steps leaves the caret where the last one left it, and raises one edit. Steps are named as in
TextHistory; a CodeBox names its commands and folds its own indents into the step that caused them.

## StepsMenu

Build it on the stack like a Menu, fill it with add, and run it with executeUnder - 0 means Escape
or a mouse click outside.

**A step stands for its run** - itself and every step above it - and the pointer or the arrows
light the whole run. The foot says what a click takes, "Undo 3 steps", or reads Cancel off the
steps. The list scrolls past 400 design units.

## AcceptEditEvent

**A sink that will not have the value calls refuse.** The editor stays open with the caret where the
user left it.

**A sink knows its target from how it was built.** The control the editor covers may be gone by the
time the sink runs, so the event leaves it out. `askedBy` lives inside the editor, so a sink may
hang a window on it.

## PromptBox

The box of an in-place editor and of every MessageBoxBase dialog. It answers four things the window
around it cannot:

- where its first glyph is drawn - textOriginInForm;
- which characters belong - none before a key press reaches it, so the press that opened its window
  types nothing;
- which line breaks belong - Shift+Return's, while a plain Return is the window's;
- why a value was refused - setRefusal and showRefusal, cleared by the next key or mouse press.

## InPlaceEditForm

**The window is a menu**: it takes the pointer, never the activation, so the form being edited stays
lit behind it and gets the focus back. The focus is one per application, so the caret works there -
see Input.

## HexPane

A mouse press selects the column it lands in, and the ring is drawn there. The keys move through
the bytes in either column.

## AppButton

**It needs no wiring** - it presents appMenuAction, so a click opens the AppMenu. Only the icon
differs between applications.

## CaretMoveEvent

Raised for an arrow key, a mouse click, an edit, and a focus change. **Ask the box where the caret
is** - TextBox::caretLineColumn.

## TextEditEvent

Covers every edit - typed, deleted, pasted, cut, undone or redone. **It is raised last**, so a
listener may do anything with the text or the box. A caret move alone raises CaretMoveEvent.

## LinkClickEvent

**A link is live** in a read-only box, and with Ctrl held in one the user types into. A live link is
underlined, shows the hand, and is followed by a press and release on it with nothing selected
between. Return follows the live link the caret stands in, a caret at either end of it included.

- `#name` the box's text holds - the box goes there itself and raises nothing. See
  TextBox::goToAnchor.
- Any other target raises the event, and a handler that answers it stops it.
- What no handler stops opens through the shell, for an `http:`, `https:` or `mailto:` target alone.

The click goes no further up.

## GetLinkTooltipEvent

**Every link has a tooltip, live or not**, and it replaces the box's own while the pointer is on the
link. It shows above the pointer's line, from where the link starts - FormPlacement::Top - and
drops under the line where the screen has no room above.

**A handler writes the tooltip's words**, its placement and anchor already set. Where it writes
nothing, the box writes its own:

- `#name` the box's text holds - the anchor's paragraph, formatted, cut after six lines by
  GetTooltipEvent::writePreview;
- any other target - the target as the text states it.

In a box the user types into, the tooltip ends with a muted line, whoever wrote the words:
Ctrl+click to follow.

## LinkContextPopupEvent

A mouse right click on a link raises this ahead of the box's own edit menu, as does a menu asked
for from the keyboard while the caret stands in a link. **A handler that shows a menu stops
`popup`**, and the box shows nothing over it. Otherwise the box answers as anywhere else in it: a
ContextPopupEvent handler first, then the edit menu.

A menu raised from the keyboard drops at the caret.

## TextBox goToAnchor

Lands on the anchor as a browser lands on a fragment, and returns false where the text holds no
anchor of that name.

- Two anchors of one name - the first in the text wins.
- A far jump glides like any scroll the box asks for.
- A link to `#name` in the box's own text calls it.

**Every jump is remembered** for goBack, whether a link or the application made it.

## TextBox goBack

A place is the selection and the line at the top of the view. With nothing to go back or forward
to, Alt+Left and Alt+Right go on to whatever is above the box.

**Edits move the places:**

- an edit made through the box carries every place along, so going back after typing lands where
  the user was;
- an undo or a redo drops the history, as the box is not told what it changed;
- a host writing the text keeps the history, and a place is clamped to the text when it is gone to.

## SearchBox

A one-line box a search is typed into, with a count of what was found at its right end.

**The owner does the searching.** The box raises OnSearch with what the user asked, and the owner
answers with setResult, which sets the count. showList drops lines under the box - suggestions or
history.

- Enter and Shift+Enter raise Next and Previous.
- Escape hides the list, or with none shown raises End - the owner hands the focus back to what
  was searched.
- Previous, Next and Clear appear after the count while the box holds text. They answer mouse
  clicks only, and Previous and Next are dimmed while the count is zero.

**Escape keeps the text**, so the search stays while the user reads what it found. Typing and
Clear empty the box as edits, and Undo brings the words back.

**Ctrl+F, F3 and Shift+F3** are StdActions::find, findNext and findPrevious. Claim them on the
page holding the box, so that F3 steps from the searched text as well as from the box.

## SearchBox listKeptBy

A line picked with these modifiers held raises Pick as any pick does, and the list stays up on
that line for the next one. Set to Alt, it lets Alt+Enter open line after line in tabs behind the
one shown.

## SearchBox setResult

What the count reads:

- `2 of 7` - a match is current.
- `? of 7` - none is current.
- `No results`, in red - none found.
- `7 on page` - none current, in a box given setCountPlace(L"on page"). It counts one place of a
  wider search, so none found there reads `0 on page`.

## SearchBox showList

The list is a DropList, and the caret stays in the box. While the list is up:

- Up, Down and the page keys walk it. Down enters at the first line; Up on the first line leaves
  no line current and hands the keys back to the text.
- Enter on the current line, or a mouse click on any line, raises Pick with the modifiers held.
- Enter with no line current hides the list and raises Next.
- Escape, Tab and the focus leaving the box hide it.

An empty set of lines hides the list. Once it is hidden, Down raises Search, so that the owner can
list again. listPlacement says where it drops.

## ScrollBars::Auto

**A bar comes up against a ceiling** - the box's MaxSize, or the window for a root box. A box
inside another takes its vertical bar from the slot it was laid out into.

**A bar takes its strip from the other axis.** Content that fits beside no bar is measured again
beside one that comes up, and may need the second bar too.

## PromptDialog

**A handler may refuse the answer** with AcceptEditEvent::refuse, and the dialog stays up with the
reason under the box. **It may ask a question of its own** first, owned by askedBy, so it stacks on
top. Told no there, it refuses with no reason, and the dialog stays as the user left it.

## MessageDialog

**The message is read-only**, and still takes the focus, selects and copies - an error message can
be copied into a search.

## MessageDialog OnLinkClick

The dialog raises the message box's LinkClickEvent again here - the same event object. **A handler
that stops it** keeps the box from opening the target through the shell. Otherwise the link opens
as in any TextBox - see LinkClickEvent.

## MessageBar

A band across the top of what it is about. **The page goes on while it shows** - an answer runs its
handler, and the page hides the band once the question is settled.

The icon sits in its own column, with a stripe of its colour down the left edge. The message wraps
beside it, and the answers sit at the right end, each as wide as the widest. Only the answers take
input.

**An answer names what the page does** - Read as text. It is a Button, and the arguments after the
handler in addAnswer override the band's own - an icon for the caption, or MouseOnly to leave the
focus where it is.

## EditTarget

**State `textRect` and `textAnchor` as the covered control lays its text out.** The editor takes
the rect's width as its minimum, and the same anchor. Its glyphs then land on the covered ones, and
nothing jumps as it opens. `textRect` is in the coordinates of the form the control is in.

- **The whole rect is a minimum**, height included. The editor hides the old text, so a box that
  shrank to what is typed would let a wrapped caption show from under it.
- **The window grows right and down** from its top left. Text that outgrows the rect moves a
  centred or right-anchored run away from the covered one.
- **`typed` replaces the value**, which opens selected whole. A combo box opened by typing a
  character starts its value with it, and the old value stays in the undo history.
- **`suggestions` points into the caller's list**, which is safe because the editor runs its loop
  inside the call that opened it - see SuggestionList.

## SuggestionList

What goes with what is typed, listed under an in-place editor - what a combo box shows over its
editor. A Suggester answers the lines for the typed text, and what a picked line puts in the box;
EditTarget::suggester names one, or EditTarget::suggestions holds items the editor lists itself. A
read-only editor lists none.

**The caret stays in the box.** The list is a DropList on the box, as tall as the screen allows.

**Items list what the sink would take.** A row matches by typedName against the trimmed text,
ignoring case, as a combo box looks up a typed value. A row shows the item's text, or its
placeholder muted - never what a face adds.

- The list appears on the first edit, never on open, and hides when nothing matches or the text
  is empty.
- F4 or Alt+Down list everything - the suggester is asked for an empty text. The next edit filters
  again.
- Down enters at the first row, Up from the first row goes back to the box, and Down on the last
  row stays.
- Return on the current row, or a mouse click on any row, puts what the suggester answers in the
  box and offers it. A refusal shows under the box, with the list already down.
- A suggester whose pickAccepts says no keeps the edit going from what it put in the box, as a
  folder picked from a path is walked into, and the list follows that edit as any other.
- Escape hides the list and keeps the edit. The next Escape cancels it.

**A row is current only after Down.** Return with no row current offers what was typed, so a sink
taking free text keeps "new" rather than "newfile". The box keeps what was typed while the rows are
walked.

## WithInPlaceEdit editorSuggestions

The default answers none, and the editor lists nothing - see SuggestionList.

## DropList

The list a SearchBox, a CodeBox and an in-place editor drop.

**The caret stays in the target**, the control the focus is in - the field, not the box around it.
The list never takes the focus. The pointer reaches its rows, and the target hands on keys:

- the arrows - moveCurrent;
- the page keys - moveCurrentByPage;
- Enter - pickCurrent.

Each returns false while the list is down, so the target keeps the key.

**Hide the list before the focus moves.** A list going down hands the focus to its target, as every
popup does. So a key such as Tab hides the list first, or the list going down takes the step back.

## DropList show

BottomLeft drops the list from the anchor's left edge, BottomRight from its right edge. The list is
at least as wide as the anchor and DropListOptions::minWidth. An empty set of lines hides it.

**The list follows its anchor**, a control or a rect:

- **A control** is followed as it moves and grows. It must hold the target or be it - show throws
  std::logic_error otherwise - so it lives as long as the list does.
- **A rect** is in the target's form and moves with the target - the way to drop under a spot
  inside it, such as the caret a CodeBox's list drops under.

**Pass a box as a control, not as its rect.** A rect would move with the box's field, and a field
narrowed by a count coming up would pull the list's right edge in.

## DropList pick

**A pick holding every modifier in DropListOptions::keptBy leaves the list up**, on the same line.
Any other pick hides the list before the owner is told, so whatever the owner opens is clear of it.

## DropListLine

- **A flex space** in the text spans the list's width, so a word after it sits at the right edge.
- **The tooltip** shows beside the list while the line is current, at its row's height - to the
  right, or off the left edge where the right has no room, never over the rows.
- **A divider** parts the line from the lines after. The keys pass over it, and the shownLines cap
  makes room for it.

## DropListOptions

Stated once, when the list is built.

- `shownLines` counts rows at the height they measure, which their text and padding set.
- `UpFromFirst::Stays` suits a list the keys stay in, as a CodeBox's does. `Leaves` hands the keys
  back to the target's text, as a SearchBox's and an in-place editor's lists do.

## SelectionMoveEvent

Raised for a key, a mouse press, a drag, Select All and new bytes. **Ask the view what is
selected** - HexView::selection and caretOffset.

**Hang a reading of the selection off this**, in a status line the host owns. A tooltip is sized
from its first words, and user input hides it.

## HexView

Each line holds sixteen bytes.

**The bytes are borrowed.** Whoever calls setBytes owns them, and they must outlive the view, or be
taken back with setBytes of an empty span.

## CodeBox

**The text stays what was typed.** The colours are an overlay drawn paragraph by paragraph, so a key
press re-shapes one paragraph, and a copy carries the user's own formatting.

- `language` is a Syntax::Language by value - one of Syntax::Languages, or a caller's own.
- `inks` holds one ink per kind of token. A kind left in the text's own ink costs nothing.
- WordWrap::No and TextFormat{ TextStyleId::Code } come ahead of the caller's arguments, which may
  override them. A text needs no style of its own to be set in the monospace font - see TextFormat.

**Completion** draws on the Syntax::CompletionEntries set through `completion`, the keywords, and
the names the text declares - see CompletionList. The list is named, not copied, and the host keeps
it alive. A box with no list, or a read-only box, completes nothing. The list also feeds the
signature tooltip - see CodeBox::showParameterTooltip.

**A title block** - the comment lines a text opens with to state its settings - completes out of the
Syntax::TitleBlock set through `titleBlock`, named the same way.

The box also indents lines as they are typed, draws IndentGuides, and scrolls past its end - see
ScrollMetrics.

### Indents

The box sets each line's indent as it is typed, at the column the language places it - see
IndentRule. A read-only box does none of this.

**Enter** puts the new line where the language places it.

- The line left loses its trailing blanks. Where Enter finished its first word - an else, an
  end - that word places the line first.
- The text carried down loses its leading blanks and is placed as a line of its own.
- A closer carried down from under its opener - `begin|end`, `{|}` - keeps an empty line between
  the two, with the caret on it.
- An opener left at the end of the line - begin, try, case, a class with a body, a brace - is
  closed below it while nothing below closes it.

**A finished first word places its line** - end, until, else, except, finally, a visibility word,
a begin under a then, a section word, a closing bracket. The character after it, Enter, or a
completion row taken finishes the word. It happens once, so a line moved by hand stays.

- **Tab** with nothing selected, or a selection inside one line, puts in the blanks to the next
  stop. A selection past a line, or holding a whole line, moves its lines - see
  CodeBox::indentLines. **Shift+Tab** moves them back a stop.
- **With Ctrl or Alt held, or right after Escape**, Tab moves the focus on - the way out of a box
  that takes Tab. A read-only box never takes Tab.
- **Backspace** in a line's indent, with nothing selected, goes back to the previous stop.
- **Home** goes past the blanks the line opens with, then to the line's start.
- **A paste** of several lines moves as one block. Its first whole line goes where the language
  places it, and the rest keep their shape against it. Where the paste's first line came without
  its indent, the line after it is placed too, and the rest follow that one.
- **Reindent** - StdActions::reindent on the edit menu, or reindentLines - places the lines the
  selection reaches, or every line, top down. A line of blanks is emptied, and a line running on
  inside a comment or a string stays as written.

**Each is one undo step**, after the key's own. The first undo takes back what the box did, the
second what was typed. A step that moves lines lands the selection with them, and so does its
redo - see TextHistory.

**The unit** of one level is indentUnit, four spaces unless stated - or, with detectIndent on, what
the text says. See DetectIndent.

## CodeBox indentLines

Tab's move of lines, for a command to call; outdentLines is Shift+Tab's. A line of blanks stays,
and so does a line the selection reaches only at its start. The selection moves with the text.

## CompletionList

A DropList under the caret. **The caret stays in the box**, and every key reaches the box first.

**It opens** on the first character of a name typed and on a typed dot, and narrows with each
character after. Ctrl+Space lists everything the place can take, a name typed or not. Only a typed
character opens it, so a paste does not. It stays closed inside a comment or a string, a title
block's line aside.

**It closes** when nothing matches, when the name is backspaced away, on Escape, and when the caret
leaves - a mouse click, an arrow key, the focus going.

**What it lists:**

- After a dot, the members of the class the dot reads and of its parents - see
  completionMemberClasses. Where that class is unknown the list stays closed, Ctrl+Space included.
- Elsewhere, the text's own names first, nearest scope ahead - the caret's routine, the routines
  around it, the globals - then the host's names and the keywords. Each run is in name order, so
  an exact match leads it.
- A name shows only where it is in force. A local and a global of one name both show, the local
  first, and a host name of that spelling after both.

The text's declarations are read as the list opens, and again on Ctrl+Space - see
DeclarationReader. The current row's signature and hint show beside the list - for the text's own
names, the declaration as written.

**Taking a row.** The first row is current as the list opens. Return or Tab takes the current row,
a mouse click any row. Its spelling replaces the name typed in one edit, with the caret after it -
`str` becomes `StrToInt` in a language that ignores case. The character the taking press queued -
Return's line break, Ctrl+Space's space - stays out of the text. The keys walk the rows, Page Up and
Page Down a view at a time, and stop at the ends.

**On a title block's line** the rows are the block's alone:

- up to the sign, its keys, less those the block's other lines state;
- past the sign, the values of the line's key, less those the line holds.

There the prefix's mark, the equals sign and a comma open the list too, with nothing typed. A key
taken where nothing follows the caret is written with ` = ` and lists its values at once. A value
is written a blank off the sign or the comma. So `//#Doc` becomes `//#DocTypes = `, and a Return
on Smeta gives `//#DocTypes = Smeta`.

The list is at most ten rows tall and at least 180 design units wide, its left edge at the start of
the name.

## ParameterTooltip

The signature of the call the caret is in, from its opening bracket, with the entry's hint under it.
The argument being written is bold, and a routine with no parameters reads No parameters. **The
caret stays in the box**, and the pointer passes through the tooltip.

It shows only in a box that has a completion list and can be typed into, in a language with a
ParameterReader.

- **It opens** on a typed `(` or `[` after a name that resolves - a routine the host lists, a member
  of the subject before a dot, a routine the text declares. Ctrl+Shift+Space opens it anywhere
  inside a call. A bracket inside a comment or a string opens nothing.
- **The mark follows the caret** across the commas, and past the last parameter nothing is marked.
  A call may span lines.
- **It closes** when the caret leaves the call, when the focus leaves the box, and when the window
  moves or loses the focus. Escape closes it too, after the completion list where both are up.

A name that is no routine shows nothing - a type or a class cast, a property with no index, a
variable. A routine shadowed at the caret by a variable of its name shows nothing too.

It stands above the caret's line, its left edge at the opening bracket. It goes below where there is
no room above, or where the completion list is above the line. It is no wider than a tooltip's line,
and a long signature wraps.

## IndentGuides

Yes by default, over the blocks the language reads - see BlockReader.

A guide runs at the x of the opener line's first character, from the line under the opener to the
line over the closer. It breaks beside a line whose text starts at or left of it - a private, an
except. It is one device pixel wide, in the Faint ink, under the text and its selection band. The
innermost block around the caret's line is drawn in the Subtle ink.

## ScrollMetrics

A body states it through Control::scrollMetrics, and its ScrollBox works out the rest.

**The step** is how far a button press or a wheel notch moves the view, in design units. It also
sets how deep an edge fade is, so a text fades by a line. A TextBox steps down by its last line's
height.

**Past the end** lets a text's last line scroll up to the top of the view, where the first line
rests. A line written at the end is then at eye height. A CodeBox asks for it while it can be typed
into; a read-only one stops at its last line.

- The wheel, the bar, a drag and Page Down reach the room below the text. Page Down moves the view
  as far as the caret travels.
- Bringing the caret into view - Ctrl+End, a line typed below the view - stops at the bottom
  margin and never enters the room.
- A resize keeps the last line where it is. Lines deleted at the end shrink the range, and the view
  glides back to the new end.
- The bottom fade ends with the text, so a last line near the bottom edge is not faded.
- Under ScrollBars::Auto the room counts as content, so a box inside another shows its bar for any
  text of two lines or more. A root box leaves the room out, so a text that fits it does not
  scroll.

## Control scrollMetrics

**The ScrollBox asks once a pass**, right after the body is measured. Answer from what that pass
left - a TextBox reads its last line's height off that layout. The default returns ScrollMetrics{}:
a step of 20 on both axes, and no scrolling past the end.

## ScrollBox bodyExtent

**The room past the end is part of the body.** The body is laid out into its slot plus the travel
that brings the last step to the top.

- The body's surface paints the room, and a mouse press there reaches the body - a text box puts
  the caret on its last line.
- A drag selection autoscrolls into the room, and the bar's range takes it in.

**Only a box whose height is dictated** - Fill, or hosted as a body - lays the room out. A box
growing to hold its body would grow by the room instead. A body that wraps takes no room, nor does
a body aligned other than Fill.

## DetectIndent

Yes by default. A text handed over whole is read for its unit, and one that says nothing keeps
indentUnit. **setIndentUnit ends detection**, so a unit picked by hand stays. Switching detection
back on reads the current text at once.

## DetectLanguage

**The box raises DetectLanguageEvent** for each text handed to it whole, never for an edit.
Switching to Yes asks at once, for the text the box holds.

**setLanguage ends detection**, so the next text does not override a language picked by hand.

The answer decides what the box draws in, and the language property keeps the stated language. A
handler that answers nothing leaves the text plain.

## DetectLanguageEvent

**The box asks lazily**, when its layout takes the text - at the next measurement or paint, not
when the text was set. A handler reads what it needs from the event and from state it holds itself.

The pass that asked draws the answer. A handler that changes anything else invalidates it all the
same. A box switched to detection by hand asks from outside a pass, and there only the invalidate
brings the change to the window.

## PixelSelectEvent

A new picture opens on its middle pixel. **Ask the view where the pixel is** -
PictureView::selection and selectedColor.

## PixelActivateEvent

Raised on the second press of a double click on a pixel - a double click on the squares beside the
picture raises nothing. The pixel is the one that press selected, and PictureView::selection and
selectedColor answer for it.

**The application decides what acting on a pixel means.** WhatsClip opens the colour editor on it.

## ZoomChangeEvent

**A new picture's fit lands at the first pass after setPicture**, once the view knows its window. A
zoom readout hears of it then.

## PictureView

**The view borrows the picture** - whoever gave it keeps it alive while it is shown.

**Zoom is in device pixels**, so 1 draws the picture one for one at any form scale. A new picture
opens in the middle, at the zoom that shows all of it, never above one for one.

**The picture stays where it is put.** The zooms and the drags move it, and a margin of it always
stays in the window. A picture larger than the window is not pulled in to cover it, nor a small one
held to the middle. The bare part of the view shows grey squares at half brightness, lined up with
those under the picture.

- **A mouse press** selects the pixel under it. A press that goes on to travel pans, and that pixel
  stays selected. The pointer passing over the picture selects nothing.
- **A wheel notch** selects the pixel under the pointer, then zooms keeping it still.
- **A context menu** raised by a mouse press is about the pixel under it, which the press selects
  first. One raised from the keyboard drops at the crosshair. The view fills no menu - the
  application builds its own from ContextPopupEvent.

Below 400% a device pixel blends the four picture pixels around it; from 400% each picture pixel is
drawn as a block. A translucent pixel is laid over grey squares.

**The crosshair** is one row and one column of device pixels through the selected pixel, reaching
24 design units each way. Each of its pixels is the contrast of what it crosses, so it reads on any
picture.

## SliderButtonMark

- `Magnifier` - a glass with a plus or a minus, for a size being changed.
- `PlusMinus` - a sign in a ring, for a value being nudged.

## FineAdjust

The finer slider spans a tenth of the range around the value, kept inside the ends. It is as long
as the slider - at least 200 design units - so the same travel moves the value a tenth as far.

- The slider reports each position through its own OnChange, and answers positionHeldByPointer
  while the pointer holds the finer one.
- Releasing the pointer raises the slider's OnSettle, closes the popup and keeps the value.
- Escape puts back the value the popup opened on. Return, or a mouse click outside, keeps it.

A handler that stops OnContextPopup answers the click instead, and a disabled slider opens nothing.
The finer slot is drawn through the slider's paintSlot, so a ramp shows its part.

## SliderSettleEvent

**A drag is over.** OnChange reports every step of a drag, and this event closes it. It serves a
listener that treats a drag as one thing, such as an undo step or a save.

- Raised on the release that ends a hold on the thumb or the slot, with `pressPosition` where the
  hold began.
- A hold that ends where it began raises nothing.
- A key, the wheel or a setter raises OnChange alone.
- A finer slider's hold counts as the slider's - see FineAdjust.

## ColorSpelling

**The user's choice**, made in a presenter. A readout offers every spelling and writes the one
picked, and a copy takes the one in use. spell, nameOf and appendSpelling write it.

**It is for people.** A colour passed from one ClaFi consumer to another is written by its
serializer - see ScalarSerializer<Color>.

## ColorEditEvent

**Ask the dialog** - ColorEditDialog::color and edited. The event carries nothing of its own.

## ColorEditDialog

- **The sliders edit one colour.** The three ColorSliders share one Hsl through a
  ColorSlidersLinker, so a move on any of them redraws the others' ramps. A notch above each slot
  marks the value it was given.
- **The preview** shows the colour as a ring and as a bar over a run from black to white, so it
  reads against both ends.
- **Original** and a swatch of the given colour appear on the bar once the colour has moved, and
  go when it is back.
- **Copy** puts the colour on the clipboard as plain text, in the ColorSpelling the dialog was
  handed.

## FlexSpacer

It takes what its lane has over. Two either side of an item centre it, and two at one end share
what is left.

**A lane is as long as its stack was granted.** In a stack that wraps, or one nothing stretched,
the spacer keeps the size it was given. See Control::fillsLane.

## WithInPlaceEdit

    class ThemeButton : public WithInPlaceEdit<Button> { ... };
    tile.onAcceptEdit([](AcceptEditEvent& event) { ... });

**The host supplies only a sink.** The editor covers the rect the host lays its text out in, so it
lands on the text and wraps the same way.

- **F2**, or **a mouse click on the text of a control the user is already on**, opens it. The click
  that picks a control never does, nor one a control above refuses - see
  Control::mayDropPopupImplicitly. The second click of a double click opens it only where the first
  was refused.
- **StdActions::rename** is answered, so any menu item or toolbar button presenting it renames the
  host.
- **editorMode ReadOnly** opens a reader instead - the text selected whole, for copying. A reader
  needs no sink and claims no rename.
- **editorSuggestions** lists items under the box as the user types - see SuggestionList.

## Tree

**Every row is one line of body text.** Where the tree's width is given from outside - a pane the
window sizes, a bar of stated width - the tree measures that line once a pass, and a row is shaped
only when it is painted.

- A row needing more than a line - a larger style in its text - states its TextFormat, and is
  measured whole.
- A tree whose width is its content's - a bar fitted to it, a scroll box carrying it across -
  measures every row.
