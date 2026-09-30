# Controls

The words that no longer fit above a declaration.

## ReadOnly

Whether the text the box shows may be changed through the box. A read-only box is a full
text box in every other respect: it takes the focus, carries a caret, selects and copies.
What it is not is a disabled box, which answers nothing and cannot be read out of.

## DropdownEvent

Raised when the dropdown of a SplitButton is activated. The handler owns the popup: it
decides what the popup contains and when it closes.

## SplitButton

A button whose dropdown is a second action alongside the primary one. Everything about the
dropdown itself - the mark, the strip, the layout, the keys - belongs to the base; what is
left here is the primary action and handing the popup to the caller.

THE DROPDOWN IS EITHER AN ACTION OR A HANDLER. Given an action - see
dropdownAction - the strip shows that command and pressing it runs it, and OnDropdown is
not raised at all; the strip is then a command of its own and stands whether or not the
face's can be run. Given none, every press is handed to OnDropdown, which owns whatever
popup it puts up, and the strip is half of the face's command: it greys with the face, so a
list about a command that cannot run - the steps behind an Undo with nothing to undo - is
never offered.

## ComboBox

### The face is as wide as the widest item

The face measures the text it would show for every item, the text it shows now included, and
answers the widest. A pick then changes the words and never the size: nothing beside the
combo box moves, and nothing above it has to be laid out again.

That second half is what a window sized once needs. A form that is not `AutoFit::Yes` keeps
the window it was placed in - the application menu is one - so a face that grew inside it would
be cut to the width it opened at, and a theme picked from the Settings page's own list would
read "New Th".

The text shown now counts because a derived control may word it unlike any item: WhatsClip's
pickers write "(Auto)" after the name they found.

Every pass that measures the face measures every item, answered by the text cache after the
first. `Control::calculateText` answers an empty text without calling `measureText`, so a face
with nothing picked and no placeholder measures no item at all - a combo box is built standing
on an item.

### The list is sized by its items

The dropped list's window is `AutoFit::Yes`, as a `Menu` is: it is whatever the items came out
as. A list too wide for the screen is cut to the work area, and the horizontal bar that cut puts
up is room the window is asked for again, so a list of ten rows shows ten rows over the bar. A
list laid out into the window it was first placed in would give the bar its last row instead.

Where the side the list fell to has no room for the bar, the window is cut there as well and
the vertical bar comes up beside it - see ScrollBars::Auto.

## ComboBoxAcceptTextEvent

The text typed over an editable combo box's face, on its way to picking an item. The application
answers first, through the AcceptEditEvent it carries:

- a handler that TAKES the text stops the event - the Themes app reads `+0.02` and `=0.66` as
  rules this way;
- a handler that refuses it says why, and the editor stays up with the reason under the box;
- a text nobody takes is looked up among the items by each item's own name, case aside: the
  whole name first, then the first item whose name starts with the text. Nothing found is
  refused. An empty text names the item that stands for no value - see PlaceHolderText - and
  with no such item it changes nothing.

The editor opens on the picked item's text as AdjustItemTextEvent words it, in plain text -
formatting and inline objects dropped. A value left as it opened names nothing new and is never
offered.

## PlaceHolderText

What a combo box item shows while its text is empty, which makes it the item that stands for no
value. The face and the list draw the placeholder muted, the editor opens empty over it, and an
emptied value picks it. It still names the item for a lookup, so typing its first letters finds
it. The Themes app's No change is one: an operation cell that changes nothing has no value to
type.

## DialogAnswerEvent

An answer was given, and whether it settles the question.
A handler that CANNOT ACT ON THE ANSWER YET calls keepOpen(), and the dialog stays
standing with its answers where they were. A Save that has to ask where the work goes and
is told no there is that case: the question it was answering was never settled, so putting
it away would leave the user with nothing to answer.
Anything a handler raises is owned by `button` and so stacks ON TOP of this dialog,
which stays on screen behind it - the question and the question about the question, both
visible, both where they were pressed.

## Menu

A popup list of commands, owned by the control it was opened from.
Built on the stack where it is opened, filled, then run: execute() returns once the
menu has closed, so nothing outlives the call that raised it.
A list built out of data states a placeholder with setPlaceHolderText(), and a menu
holding nothing but that placeholder opens to show it. A menu with nothing to show at
all does not open.

## StepsMenu

A popup listing steps from the top, where the pointer picks how many to take - the list an
undo button drops. Built on the stack like a Menu: made over the control that owns it with the
verb the foot speaks ("Undo", "Redo"), filled with add(), the first step added standing at the
top, then run with executeUnder(), which answers how many steps were taken from the top, or
none where the menu was left by Escape or a click outside.

The steps are MenuItems. A step under the pointer names a run - itself and every step above
it - and the whole run reads as hovered, so the eye sees what one click takes; the arrows
name the run the same way through the focus. A click on a step takes its run. The pointer
leaving the steps - for the foot, the bar, or the outside - names no run, and the foot reads
"Cancel" and closes on a click; while a step is under the pointer the foot says what a click
on it takes ("Undo 3 steps"). The list scrolls past 400 design units.

## AcceptEditEvent

The text the user is committing, and the sink's answer to it.

A sink that takes the value does nothing here. A sink that will not have it calls
refuse(), and the editor stays open with the caret where the user left it - a value
nothing would take is never lost by being committed to nowhere.

The control the editor covers is NOT carried here. A sink runs while the editor is
up, and by then that control may be gone - a list rebuilt under an open editor takes it
with it. A sink knows what it is writing to because it was built knowing; naming the
control again here would only be a pointer nobody is left to clear. `askedBy` is the
other thing entirely: it lives INSIDE the editor and is there for as long as the sink
runs, which is what makes it something a sink may hang a window on.

## EditBox

A text box that can refuse the value typed into it and say why.

It answers for four things a form around it cannot: where its text starts; which characters
belong in the text, which is none until a press has reached the box, since the press that opened
its window has a character queued behind it; which line breaks belong, a plain Return being the
form's and Shift+Return the box's; and why a value was refused. Every window that takes one line
of text and may not have it wants all four, which is why this is not the in-place editor's alone.

## InPlaceEditForm

A text box in a window of its own, placed over the text it edits.

The window is a MENU, not a dialog: it takes the pointer and holds the caret
without taking the activation, so the form being edited stays lit behind it and gets the
focus back when the editor closes. The caret works there because the focus is one per
application rather than one per window - see Input.

## HexPane

Which of the two byte columns the caret stands in. A press lands in the column it fell on,
and the ring is drawn there; the keys move the caret through the bytes either way.

## AppButton

The button an application puts its own mark on. It states the icon and nothing else, so an
app button is the same in every application and only the mark differs. It presents
appMenuAction, so a click on it opens the application menu - see AppMenu - and nothing is wired
by the application.

## CaretMoveEvent

The caret has come to rest somewhere new: an arrow key, a click, an edit, or the box
taking or losing the focus. WHERE it stands is asked of the box - see
TextBox::caretLineColumn - so a listener that only wants to know that it moved pays
nothing for the answer.

## TextEditEvent

The text has been changed through the box - typed, deleted, pasted, cut, undone or redone.
Every route an edit takes ends in the same place, so one listener covers all of them. It is
raised LAST, once the box has finished with the edit, so a listener is free to do anything
with the text it has just been told about, the box included. A caret move that changes no
text raises CaretMoveEvent alone.

## LinkClickEvent

A link in the box's text was followed: pressed and released on it with nothing selected in
between, in a read-only box, or with Ctrl held in one the user types into - a plain click there
places the caret. The same two cases decide when a link is underlined and shows the hand under
the pointer.

A target of # and a name is an anchor, and one the box's own text holds is followed by the box
without raising this event - see Anchors. A name the text does not hold is raised like any
other target, which is how a link reaches an anchor on another page.

A handler that answers the target stops the event. What no handler stops, the box opens
through the shell - a target starting with http:, https: or mailto:, and nothing else. A text
can come out of a file or off the clipboard and name any target, and the shell runs a program
it is handed. The click that followed a link goes no further up.

## Link hints

The hint over a link in a TextBox. The tooltip asks a control for its hint once, when the pointer
comes onto it, and a link is a part of the box the pointer enters and leaves - so the box tells
the tooltip when the link under the pointer changes, through Tooltip::hoveredZoneChanged, the way
a grid tells it its hovered cell did. A hint already up is asked again where it stands, and one
that is not starts the usual wait.

EVERY LINK HAS ONE, LIVE OR NOT. The underline says a click would follow the link, and the hint
says where it goes, which a reader wants in a box they type into as much as in one they read. It
stands over the line the pointer is on, from where the link starts on that line, so it covers
neither the link nor the line it reads in - FormPlacement::Top, which drops it under the line
where the screen has no room above. While the pointer is on a link, the link's hint stands in for
the box's own.

GetLinkTooltipEvent is raised first, with the target, the range the link covers, and the hint's
placement and anchor already stated. What a handler writes is the whole of the hint. Where nothing
is written the box writes its own:

- a target of # and a name the box's own text holds: the paragraph the anchor stands in, with its
  formatting - in the Ledger, the clue a reference names;
- any other target: the target as the text states it.

A PREVIEW STOPS AFTER SIX LINES. The box lays the paragraph out at Tooltip::k_lineWidth, the width
the hint breaks its lines at, and cuts it where the sixth line ends. An ellipsis ends what is left,
in the styles still open where it stands - muted inside a muted run, regular after a bold word
that ends at the cut. A word the ellipsis would push onto a seventh line goes with the rest, and
a line holding one word longer than itself gives up characters instead. The paragraph is shaped
once more for this than the hint shapes it.

In a box the user types into, a muted line follows saying Ctrl+click follows the link: a plain
click there places the caret, and nothing else on screen says the link can be followed at all.

## Anchors

TextBox::goToAnchor selects the text the anchor holds and brings its line to the top of the
view, the way a browser lands on a fragment. A plain scroll into view would stop as soon as the
line showed, which going forward is at the bottom. The request is the one the page keys make: a
rect exactly as tall as the client area ScrollBox measures against, which fits only with its top
against the top of that area. A far jump glides like any other scroll the box asks for. Where
two anchors share a name, the first in the text is the one gone to.

EVERY JUMP IS REMEMBERED. The box keeps where it stood - the selection, and the position the view
showed at its top - before each jump, and Alt+Left goes back to it, Alt+Right forward again, the
way a browser's history does. goToAnchor is what records a place, whether a link or the
application called it. With nothing to go back or forward to, the key is left unhandled and
carries on to whatever stands above the box.

A PLACE NAMES POSITIONS, AND AN EDIT MOVES THEM. An edit made through the box carries every
place it holds by what that edit did, so going back after typing lands where the user was. An
undo or a redo changes the text by an amount the box is not told, and drops the history. A host
writing the text leaves it standing, and a place is clamped to the text when it is gone to.

## ScrollBars::Auto

A bar on whichever axis the content overruns, and nothing on the axis it fits.
A ROOT ANSWERS AGAINST ITS MAXIMUM, in the measuring pass - see calculateContent. The
window reaches a root as that maximum, so a menu cut short by its placement is measured
against the room it got and puts the bar up in one pass.
A BOX INSIDE ANOTHER ANSWERS THE VERTICAL BAR FROM ITS SLOT. Its maximum is its own, and
a window cut short lowers no maximum below the root's. So the align pass records whether
the body stood past the slot it was laid out into, and asks for another pass where the
bar standing is the wrong one - see settleAutoVerticalBar. The measuring pass that
follows stands the bar that answer names. One pass settles it: a body laid out narrower
is never shorter, nor a wider one taller. Until the box has been laid out once, its
maximum answers for it.
A pass measuring what to ask a placement for records nothing - it lays the box out at the
content's own size, so its slot is no viewport - but it reads the answer, and the size it
asks for carries the bar.
THE HORIZONTAL BAR IS ANSWERED AGAINST THE MAXIMUM ALONE. A box whose width is its
host's answer has no maximum of its own there and states the horizontal bar it wants.
A BAR TAKES ITS STRIP FROM THE OTHER AXIS. Content that fits beside no bar may not fit
beside one, so once a bar comes up the axis still without one is read again against what
is left - see calculateContent. Only a bar coming up is read then: one taken down would
hand its strip back and ask the first question over. A bar the second reading raises finds
the other one up already, so a pass measures the box again twice at most, and only where a
bar changes.

## PromptDialog

A question with one line of text for its answer, dropped under the control that
raised it.
The text is the answer here, so the box is drawn as something to write in: its own
surface, its own border, over the middle the question is written on.
THE ANSWER IS REFUSABLE. A handler that calls AcceptEditEvent::refuse keeps the
dialog standing with the reason under the box, so nothing is written until a value nothing
refuses has been given. That is the in-place editor's contract, and this is its box.
A HANDLER MAY PUT A QUESTION OF ITS OWN before it answers, owned by
AcceptEditEvent::askedBy - so it stands on top of this dialog with the name still on
screen behind it. Told no there, it refuses with nothing to say and this dialog is left
exactly as the user left it.

## MessageDialog

A short question and a row of answers to it, dropped under the control it was
raised from.
The message is the dialog's own text and not an answer to it, so the box takes no
edits. It stays a full text box in every other respect - it takes the focus, selects and
copies - which is what lets an error message be got out of the dialog and into a search.

A LINK IN THE MESSAGE IS THE CALLER'S TO ANSWER. The box is the dialog's own, so the dialog raises
the box's LinkClickEvent again as OnLinkClick, the same event object. A handler that stops it
keeps the box from opening the target through the shell; what no handler stops is opened the way
any box opens it - see LinkClickEvent.

## EditTarget

What an in-place editor is placed over: the control it covers, where that
control's text is laid out - in SCREEN coordinates - how the text is anchored in that
rect, and how far the editor may grow.

THE ANCHOR AND THE RECT TRAVEL TOGETHER. The placement puts one point of the
editor on one point of this rect, so the two runs of glyphs only land on each other
while both are laid out the same way. The box is given this rect's width as its MINIMUM
and this anchor, which makes the two layouts identical AS THE EDITOR OPENS - nothing
jumps when it appears, whichever end the glyphs start from.

It holds at that size and not past it. The window is pinned by its top left and grows
right and down, so text that outgrows the covered rect moves a centred or right-anchored
run away from where the covered one sits. By then the editor is visibly its own window
showing more than the control could, which is what it is for.

THE WHOLE RECT IS A MINIMUM, HEIGHT INCLUDED. Only the new text is visible while an
edit is on, and the editor is what hides the old one - a box that shrank to what has been
typed would let the rest of a wrapped caption show out from under it.

WHAT WAS TYPED TO OPEN THE EDITOR REPLACES THE VALUE. The value opens selected whole, so the
keys that asked for the editor land where the next ones would - a combo box opened by typing a
character starts its value with that character, and the value it replaced stays in the undo
history.

`suggestions` names the values the editor completes from - see SuggestionList. It is a
pointer into the caller's list rather than a copy, and that is safe for the same reason
`source` is a reference: the editor runs its loop inside the call that opened it, so the
caller's list is still there for as long as the editor is.

## SuggestionList

The list under an in-place editor of the values whose names begin with what is typed. It is
what a combo box shows over its editor: `WithInPlaceEdit::editorSuggestions` answers a
TextItems, and the editor lists it.

THE CARET STAYS IN THE BOX. The list is a window of its own standing on the box, and nothing
in it can take the focus - its root, its stack and its rows are all MouseOnly, so the form's
entry search finds nothing to land on, and a press on a row walks up to nobody. The rows are
reached by the pointer, and by the keys the box hands on: Up and Down move the current row
while the list is up, and the stack paints that row selected itself, since a stack whose
current item is moved from outside is not one that follows the user. Keys reach the list
through the ordinary forward chain - the form under the editor forwards to the editor, the
editor to the list - and the list's walk starts from the box, which holds the focus. A form
whose popup is up but does not hold the focus keeps its characters; see wnd_char.

WHAT IS LISTED IS WHAT THE SINK WOULD TAKE. A row is matched by typedName, and case aside by
startsWithFolded - the two functions ComboBox::lookUpItem matches by - against the text as
the sink is offered it, trimmed. The list appears on the first edit and never on open, and
goes away with no match or an empty text. F4 or Alt+Down - the keys that drop a combo box's
list - list every item whatever is typed, the filter lifted until the next edit puts it back.
A row shows the item's own text, or its placeholder muted while it has none, and never what a
face adds.

NOTHING IS CURRENT UNTIL DOWN. Return with no row current offers what was typed, as it always
does: a sink taking free text with hints must not have "new" turned into "newfile", and a
combo box's own prefix rule already lands on the first match. Down from the box enters at the
first row, Up from the first row goes back to the box, and Down from the last row stays. The
box keeps what was typed while the rows are walked - nothing is written into it to restore on
Escape.

A PICK IS A PICK. Return on a current row, or a click on any row, puts its name in the box and
offers it in the same press; a refusal then stands under the box as any refusal does, with
the list already down. Escape with the list up takes the list down and leaves the edit
standing; the next Escape cancels it.

The list is asked for on every edit and answered on a 0 ms timer, once the input that made
the edit has been delivered - a held key writes characters faster than a window can be placed
for each. A request that finds the editor's layout unsettled waits on the pass that settles
it, so the list is placed on a box that has been laid out. It is hidden rather than
destroyed when nothing matches, and sized by what it lists: AutoFit, at least as wide as the
box, under it.

## SelectionMoveEvent

The selection has come to rest somewhere new: a key, a press, a drag, Select All, or a new
set of bytes. WHAT it covers now is asked of the view - see selection and caretOffset - so
a listener that only wants to know that it moved pays nothing for the answer.

THIS IS WHAT A READING OF THE SELECTION HANGS OFF. A hint cannot carry one: a form holds
one hint, it is sized from the first words it is given and user input takes it down, so a
reading that changes with every byte is not a thing a hint can be. A status line the host
owns is, and this is how the host learns to rewrite it.

## HexView

Bytes drawn as the three columns a hex dump has: the offset each line starts at, sixteen
bytes as their digits, and those same bytes read as characters.

ONLY THE LINES THE VIEWPORT REACHES ARE SHAPED. The view states its whole height through
calculateContent, so the scroll box it stands in takes the range from the layout and
nothing holds a second copy of it. The paint then draws the lines its viewport covers and
touches no others, which is what lets the view hold a buffer larger than a shaped document
could be.

EVERY COLUMN IS A COUNT OF CHARACTERS. One measurement of the monospace style per layout
pass gives the cell, and a line is drawn as ONE text laid out on that same grid - so where
a byte is drawn and where the view says it is cannot part. This holds for as long as
TextStyleId::Code resolves to a font of one advance; a proportional fallback would slide the
drawn line out from under the rects.

THE BYTES ARE BORROWED. Whoever calls setBytes owns them, and they must outlive the view
or be taken back with a setBytes of an empty span.

## CodeBox

A TextBox drawn as source in a language. THE TEXT STAYS WHAT WAS TYPED: the colours are an
overlay the box states to its layout, asked for paragraph by paragraph as the paragraphs are
drawn - so the text carries no marker per token, a key press re-shapes one paragraph the way
it does in any text box, and what is copied out is the text and the user's own formatting.

Beside the layout's shaping the box keeps a Syntax::LineStates - the state every line starts
in, which is all that has to be remembered, since a line's tokens are read off it and that
state in the time it takes to draw the line. The two are brought into step together, in
textTaken: an ordinary edit re-reads the lines from the one it reached until a line starts in
the state it already had, and a whole text stated anew is read from its start.

The language is a Syntax::Language passed by value - one of Syntax::Languages, or a caller's
own. The inks are a Syntax::Inks, one per kind of token; a kind left in the text's own ink
states no span at all, which is what keeps operators and punctuation from costing anything.
WordWrap::No is stated before the caller's arguments, as HexView states it: source is lines.

A box completes names out of a Syntax::CompletionEntries the host states through
`completion` - named, not copied, so the host keeps it alive for as long as the box stands.
The language's keywords join the list, and so do the names the text declares for itself,
where the language has a reader of declarations - see Syntax#declarations. A box with no
list stated completes nothing, and a read-only box completes nothing either. What the list
looks like and how the keys drive it is under CompletionList.

A box places each line as it is typed, in the language's reading of where lines stand - Enter,
Tab, Backspace, Home, a paste and Reindent. See Indents. It draws a line down each block its
text opens - see IndentGuides.

## CompletionList

The list under the caret of the names the word being typed can complete to. It is what the
in-place editor's SuggestionList is, made for source: a window of its own standing on the
box, nothing in it able to take the focus, the rows reached by the pointer and by the keys
the box hands on. THE CARET STAYS IN THE BOX, and every key reaches the box first - see
SuggestionList for the forward chain and wnd_char.

WHEN IT OPENS. By itself on the first character of a name typed and on a typed dot, and
narrowing with every character after; on demand on Ctrl+Space, which lists everything the
place can take, a name typed or not. It closes when nothing matches, when the name is
backspaced away, on Escape, and on the caret leaving - a click, an arrow key, the focus
going. It never opens inside a comment or a string, and never over a paste: only a typed
character asks for it, an edit merely brings a list already up up to date. The request is
answered on a 0 ms timer once the input that made it has been delivered, and one that finds
the form's layout unsettled waits on the pass that settles it, as the SuggestionList's does.

WHAT IS LISTED. The rows are built once per language, list and reading of the text - the
text's own declarations, the entries and the language's keywords in one scope, every class's
methods and properties in the other, each row keeping its class - and shown by scope, by
prefix and by where the caret stands; see Syntax#completion for the place, the prefix rule
and how the class a dot reads is found. After a dot only the members of that class and of
its parents are shown, a member both state from the class itself, and where the class is not
known the list does not open, Ctrl+Space included. The text's own names come first, the
nearest scope's ahead: what the routine the caret stands in declares, then what the routines
around it do, then the text's globals, and only then the host's names and the keywords -
each run in name order under the language's case rule, so within a run an exact match stands
first. A name is shown only where it is in force, so the locals of another routine are not
listed, and a local and a global of one name both are, the local first; the host's name of
the same spelling stands after both. The declarations are read by the language when the list
opens, and again on Ctrl+Space with the list up; a list up keeps its reading while the keys
narrow it. The reading is compared with the last before any row is rebuilt, so typing in a
routine's body rebuilds nothing. See Syntax#declarations for what is read. The first row
shown is current when the list opens, and an exact match is that row. A row prints the name
with the part typed so far in the accent ink, and the kind muted at the end of its line -
function, procedure, class, property, keyword, variable, constant - the way a menu prints a
key.

THE HINT. The current row's signature, in the code style, and under it the hint's sentence,
stand beside the list at the row's height - the form's tooltip, placed FormPlacement::Right
on a rect at the list's edge, raised right away from the pass that lays the row out and
after a keyboard move, and on hover as any hint is. A keyword, or an entry with neither
signature nor hint, shows none; one of the text's own names shows its declaration as
spelled. The hint goes with the list.

A PICK IS A PICK. Return or Tab takes the current row, a click takes any row: the name as
typed is replaced by the row's own spelling in one edit, undone as one, with the caret
after it and the list down - so `str` becomes `StrToInt` in a language that ignores case.
The character a taking press queued - Return's line break, Ctrl+Space's space - does not
reach the text: the press settles it and the character reads it, as EditBox does for a
Return the form takes. Up and Down walk the rows and stay at either end; Page Up and Page
Down walk them a view at a time, less the row left current, the step any items view pages
by, and stay at either end too.

The list is at most ten rows tall, the tool button's height each, and scrolls past that;
at least 180 units wide; hidden rather than destroyed when it closes, and opened again at
its first row with no glide, since where it was left is not on screen to travel from; and
placed under the caret's line with its left edge at the start of the name, so the rows stand
under the letters they complete.

## Indents

What a CodeBox does with a line's indent as it is typed, by the language's reading of where a
line stands - see Syntax#indent. Nothing here happens in a read-only box.

ENTER. The new line stands where the language places a line after the one left. The line left
loses its trailing blanks, all of them where it holds nothing else; where its first word has
just been finished, it is placed by that word first - an else, an end. The text the break
carried down loses its leading blanks and is placed as a line of its own, and a closer carried
down from under its opener - begin|end, {|} - keeps an empty line between the two, with the
caret on it. An opener left at the end of the line - begin, try, case, a class with a body, a
brace - is closed below it when nothing below closes it yet: the next line holding anything
stands inside the block, or at the block's column placing itself there.

A FINISHED FIRST WORD PLACES ITS LINE. A line whose first word decides its column - end, until,
else, except, finally, a visibility word, a begin under a then, a section word, a closing
bracket - is realigned when that word is finished: by the character after it, by Enter, or by
a completion row taken. Once only, so a line moved by hand afterwards stays where it was put.

TAB AND SHIFT+TAB. With nothing selected, or a selection inside one line, Tab puts in the
blanks that reach the next stop. A selection reaching past a line, or holding a whole line,
moves every line it reaches that holds more than blanks to the next stop, and a line the
selection reaches only at its start is left alone. Shift+Tab moves the lines back to the
previous stop - the caret's line with nothing selected. The selection moves with the text.
With Ctrl or Alt held, or right after Escape, Tab is the form's and moves the focus on, which
is how the focus leaves a box that takes Tab; a read-only box never takes it.

BACKSPACE in a line's indent, with nothing selected, takes the blanks back to the previous
stop. HOME goes past the blanks the line opens with, and to the line's start from there.

A PASTE of several lines moves as one block. The first line it writes whole goes where the
language places it, and the rest keep the shape they had against it - against the column the
paste gave that line, not the blanks standing before the paste. A paste whose own first line
came without its indent says nothing of where that line stood, so the line after it is placed
too, and the rest keep their shape against that one.

REINDENT - StdActions::reindent, on the edit menu of a box that can be typed into, and
reindentLines - places the lines the selection reaches, or every line with nothing selected,
top down. A line of blanks alone is emptied, and a line running on inside a comment or a string
is left as written. indentLines and outdentLines are Tab's and Shift+Tab's moves of lines, for
a command to call.

ONE STEP EACH, AND THE EDITOR'S OWN. Each of these is a single replacement of the text, undone
as one. What the box does after a key - the indent after Enter, a realign after a word, the
move after a paste - is a step of its own after the key's, so the first undo takes back what
the box did and the second what was typed. A step that moves lines lands the selection where
it put it, and a redo puts it back there - see TextEngine-Types#texthistory.

THE UNIT is what one level is written as: the indentUnit property, four spaces unless stated,
or with detectIndent on - the default - what the text itself says. See DetectIndent.

## IndentGuides

Whether a CodeBox draws a line down each block its text opens - the language's reading of its
blocks, see Syntax#blocks. Yes by default, in every CodeBox.

A guide stands at the x of the opener line's first character, taken from the layout, so it
holds whatever the font. It runs from the line under the opener to the line over the closer,
through blank lines, and it is left out beside a line whose text starts at or left of it - a
private, an except, a case's else standing at the block's own column. Only the blocks crossing
the viewport are drawn, under the text and on the layout the text is drawn from, so a
selection band covers a guide.

A guide is a solid line one device pixel wide, at any scale, standing at the middle of its pixel
column, in the text's Faint ink. The block the caret stands in - the innermost whose opener and
closer lie either side of the caret's line - is drawn a grade over it, in the Subtle ink.

## DetectIndent

Whether the box reads the indent unit off each text it is handed whole, in textTaken beside the
language's reading; a text that says nothing leaves the indentUnit property's. Yes by default.
A unit stated through setIndentUnit ends detection, so a unit picked by hand is not overridden
by the next text, and switching detection on reads the text the box holds at once.

## DetectLanguage

Whether the box asks which language each text it is handed is in, rather than reading the
language property. The question is DetectLanguageEvent, asked in textTaken for a text
handed whole and never for an edit: an edit is a key press in a text whose language is
settled, and re-reading the lines it reached in that language is all it needs. Switching to
Yes asks at once, for the text the box holds. A language stated through setLanguage ends
detection, so a language picked by hand is not overridden by the next text.

The answer goes into the line states and nowhere else: the language property keeps the
stated language, and what the box draws in is what the last answer said. A handler that
answers nothing leaves a language with no name, and a text read in that stands plain.

## DetectLanguageEvent

Asked with the whole text, plain, and a language to fill in. The box asks lazily, when its
layout takes the text - the next measurement or paint, not the moment the text was set - so
a handler reads what it needs from the event and from state it holds itself. The pass that
asked is the pass that draws the answer, and the paint that follows draws whatever else the
handler changed: an invalidate made inside the align pass is dropped by the form, and one made
inside a paint asks for one more paint of what it names. A handler that can also be asked from
outside a pass - the box switched to detection by hand - invalidates what it changed all the
same, since that is the one route by which the change reaches the window.

## PixelSelectEvent

The selected pixel moved. Raised for every route: a click, a notch of the wheel, a setter,
and a new picture, which opens on its middle pixel. It carries no position, the way
CaretMoveEvent does not: the view answers selection() and selectedColor(), and a listener
reads what it needs from there.

## PixelActivateEvent

The selected pixel was acted on: the second press of a double click on it. The view raises
it only where a pixel lies under that press - a double click on the surface beside the
picture asks for nothing. It carries no position either: the pixel it is about is the one
the press just selected, and the view answers selection() and selectedColor() for it.

WHAT ACTING ON A PIXEL MEANS IS NOT THE VIEW'S. A picture read at a zoom has no command of
its own, so the gesture is reported rather than answered: WhatsClip opens the colour
editor on it, the same editor its corner readout opens.

## ZoomChangeEvent

The zoom moved, by whichever route: a notch of the wheel, a setter, or the fit a new
picture opens at - which lands once the view knows the window it is seen through, so a
listener writing a zoom readout hears of it then and not at setPicture.

## PictureView

A picture at a zoom, standing where the zooms and the pans put it, one pixel of it
selected.

THE PICTURE STANDS WHERE IT IS PUT. It is the size it is at its zoom, in device pixels,
and the view is measured around it: the picture, and any strip of the window it leaves
bare while hanging out the other side. The box the view stands in ranges its bars over
that, so a picture larger than the window is not pulled in to cover it, and a small one is
not held to the middle. A new picture opens in the middle; the zooms and the drags leave it
where they put it, and a margin of it always stays in the window. The view borrows the
picture: whoever gave it keeps it alive for as long as it is shown.

ZOOM IS DEVICE PIXELS PER PICTURE PIXEL, and 1 draws the picture one for one whatever the
form's scale. A pixel viewer answers to the pixels, not to design units.

ONLY WHAT IS ON SCREEN IS DRAWN, AND SAMPLED ONCE. The part of the picture on screen is
sampled at the zoom into a cache, and a paint copies the cache's rows into a staging view,
so a paint costs the size of the view and never the size of the picture or the zoom, a
zoom costs one sampling of the view, and a pan samples only the strip it uncovers. Below
400% a device pixel blends the four picture pixels around its centre; from 400% it takes
the one under it, so a pixel is drawn as the block it is. A translucent pixel is laid over
grey squares, and what leaves for the surface is opaque.

THE SELECTION IS A PRESS OR A NOTCH. A press selects the pixel under it as the button goes
down, and the crosshair moves there; so does a notch of the wheel, which selects the pixel
under the pointer before it zooms, so the readout lands where the zoom is going. A press
that goes on to travel pans, and leaves selected the pixel it was taken hold of at. The
pointer merely passing over the picture moves nothing: a value read off the corner is the
value of a pixel that was chosen. A new picture opens on its middle pixel.

THE MENU IS ABOUT THE PIXEL IT WAS RAISED ON. The press that raises a context menu selects
that pixel first, exactly as a left one does, so the commands and the crosshair name the
same pixel; one raised from the keyboard moves nothing and drops at the crosshair, which is
what contextMenuAnchor answers and where a picture larger than the screen would otherwise
have dropped it nowhere useful. The view fills no menu: what a picture can be asked to do
belongs to the application, which builds its own from the ContextPopupEvent.

A ZOOM KEEPS A POINT STILL. The wheel keeps the picture point under the pointer where it
is, which is the pixel it just selected; setZoom and zoomBySteps keep the selected pixel,
or the middle of the view while none is selected. It stays across the size of the window
too, which is what the bare strip is for: a picture just wider than the window may have to
hang out one side by more than it is wider, and the strip that leaves at the other side is
scrolled off or grown over by the next notch. The new size needs a pass before the bars
range over it, so the view asks for the pass and sets the bars when the form reports
itself aligned - see FormAlignedEvent. Set, not left to glide: a bar whose range shrank
under it is on its way home, and setting it ends that where the placement says. The point is
kept to the fraction the placement floors off the origin: the picture is drawn on whole device
pixels and what the floor took is carried, so a slider dragged across its track, or an edge
dragged for a resize, does not walk the picture off its anchor a fraction of a pixel at a time.

THE FIT WAITS FOR THE WINDOW. A new picture opens at the zoom that shows all of it, never
above one for one, and that needs the window the view is seen through, which the first
pass after setPicture lays out. The fit lands then, with its ZoomChangeEvent.

THE CROSSHAIR IS A HAIRLINE OF CONTRAST. One row and one column of device pixels through
the middle of the selected pixel, reaching 24 design units each way, every pixel of them
the contrast of what it crosses: channel by channel, near white where the channel is dark
and near black where it is light, so the mark reads on any picture and covers nothing but
the line it stands in. Past the picture's edge the surface is what it crosses, and the
surface is contrasted the same way. What lies under the mark is read from the cache, never
from the surface: a backend that draws through a staging buffer of its own hands back no
pixels, and the cache holds every picture pixel on screen.

## SliderButtonMark

What the buttons at the two ends of a slider show. A glass with a plus or a minus is a
size being changed; a sign in a ring is a value being nudged. Stated at construction like
any property, and changed after.

## FineAdjust

Whether a context click on a slider opens a finer slider under it. The finer one spans a
tenth of the range, centred on the value and moved inward where it would run past an end.
It is as long as the slider it came from, so the same travel moves the value a tenth as far,
and never shorter than 200 design units, so a short slider still gets a usable one.

The value moves as the finer slider moves: the slider it came from takes each position and
reports it through its own OnChange, and while the pointer holds the finer one the owner
answers positionHeldByPointer too. Releasing the pointer after a press on the finer slider
raises the owner's OnSettle, closes the popup and keeps the value. Escape puts back the value
the popup opened on; Return, or a click outside, keeps where it is.

An OnContextPopup handler that stops the event answers the click instead. A disabled slider
opens nothing.

A slider draws the finer one's slot through paintSlot, with the span of the range that slot
covers, so a slot showing a ramp shows that part of it.

## SliderSettleEvent

The pointer has let go of a position it moved. OnChange reports every step of a drag as it
happens, so a listener that treats a drag as one thing - an undo history recording one step
for it, a document saving once - needs to know when the drag is over, and that is this event.
It is raised from the press-up that ends a hold on the thumb or the slot, and carries where
the position stood when the hold began; a hold that ends where it began raises nothing, since
no step was reported during it and there is nothing to close. A step made by a key or the
wheel raises no settle either: each of those stands on its own, and its OnChange is the whole
of it. A position set from code raises OnChange as any step does, and no settle.

A finer slider's hold counts as its owner's - see FineAdjust - so the owner's OnSettle comes
when the pointer lets go of the finer one.

## ColorEditEvent

The colour moved under one of the sliders. The dialog answers color() and edited() from
there; the event carries nothing of its own.

## ColorEditDialog

A colour under three sliders, with a preview, Copy and Close.

THE SLIDERS EDIT ONE COLOUR. The three ColorSliders share one Hsl through a
ColorSlidersLinker, so a move on any of them redraws the others' ramps at the colour it
left. Each slider marks the value it was given with a notch above its slot, which is the way
back to it.

THE PREVIEW IS THE COLOUR AGAINST BOTH ENDS. A run from black to white behind it, the
colour as a ring and as a bar across the run, so a light colour is read against the black
end and a dark one against the white.

THE ORIGINAL STANDS ON THE BAR. Once the colour has moved, the word Original and a swatch
are written on the bar in the colour the dialog was given, so the edit is read against it.
They take no click, and a colour back where it started hides them.

COPY TAKES THE CLIPBOARD. The dialog is handed how a colour is written - a ColorSpeller -
and puts that text on as plain text, which is what an application that has never heard of
this dialog can take. A window showing what the clipboard holds shows that text next.
