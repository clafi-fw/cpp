# Control Foundation

The words that do not fit above a declaration in `Y-Core/Foundation`.

## RichControl

**Events and their `On...` handler properties belong to Control.** RichControl's overrides add its
stored state to an event, then let the base raise it.

## Control text

**A `ControlText`**, so every write through it moves on the stamp that each measurement and paint
reads. See ControlText.

## Text format

Control returns none - see TextFormat.

**Every path that lays out a control's own text reads it**, next to editProps:

- Control::drawText and Control::measureText hand it to the engine.
- WithTextLayout states it to the layout it holds.
- A TextBox lays its placeholder out over it.
- An in-place editor lays its text out over the format of the control it covers.
- An over-text tooltip repeats it - GetTooltipEvent::format carries it, and TooltipLabel answers
  with it.

## RichControl textFormat

**RichControl keeps a format**, and textFormat returns none while it is empty.

- Each TextFormat property composes kind by kind - a later one replaces what an earlier one said of
  the same kind.
- setTextFormat replaces the whole format.

**A derived class states its format as a property**, ahead of the arguments it forwards. The
caller's value then wins for each kind it states, then the most derived class's, down to the base.
setTextFormat in a constructor body would wipe the caller's format, since the body runs after the
caller's properties are bound.

**Stating one invalidates nothing**, as with setColorRules. The caller asks for the pass.

## FormAlignedEvent

`sender()` is the form. **Raised at the end of the pass**, so a handler may read every rect in the
form. A handler may not start a message loop.

**A hidden form is placed by show()**, so this event comes with the form on screen, and a handler
may ignore a hidden form.

## RootLoopEvent

`sender()` is the form. **Raised on `AppContext::events()`** each time a form takes the root loop
and becomes the main window - before its first layout, and before it shows.

**Put what belongs to the main window on it here.** The application's notices come this way.

## FormBase::topLayer

**A control over the root, inside the window**, laid out after the root in the same box. A form
owns at most one, and setTopLayer replaces it.

- It is a root of its own, built with a CreateParams over the form.
- It has no say in the window's size.
- It is painted after the root and meets the pointer first. A point where it meets only itself goes
  on to the root.
- The focus walk leaves it out. A control in it that should leave the focus alone says MouseOnly.

## FormClosingEvent

`sender()` is the form. Raised by the window's close button, the system, or a mouse click on the
form behind a popup.

**A handler settles unfinished work here**, or calls `refuse()` to keep the window up. The request
that asked then does nothing else.

- `requestClose()` asks, then closes. The close button and the platform's close request call it.
- `close()` never asks, so Escape always works.
- A form with an answer of its own overrides `readyToClose`, as an in-place editor does.

## FormPaintedEvent

`sender()` is the form. **Raised on `AppContext::events()`**, which outlives every form, so a
watcher follows paints from window to window.

**Raised only while something is connected** - a form nobody watches reads no clock.

## FormControlBase

**The root's metrics, colour set and WindowShadow decide how the window looks.** The window's role
decides none of them, and a root that leaves one out has none.

## Control

**Create controls through the `add-` and `create-` methods** of the parent control or the form,
rather than through a constructor.

**An event costs a control no storage until it is connected.**

## Control animate

**Runs on the application's controller**, reached through the form. `animator()` returns it, or
null while the control is in no form.

**A control in no form takes the end value at once** - nobody sees it. A header built expanded, or
a grid cell selected before the grid shows, takes this route.

stopAnimation and stopAnimations take the same route. GridDescriptor::animateCell does the same for
a cell.

## Control makeCurrentItem

**The nearest container above that keeps a current item answers**, whether or not it takes the
control - see Control takeCurrentItem. The focus stays where it is; a container that holds the
focus shows the new item through its focusDelegate.

## Control takeCurrentItem

Asked up the chain by makeCurrentItem. A container that keeps a current item answers yes, and
takes the control where it can be an item of that container - StackBase does. Every other
control answers no, and the question goes on up.

## Control nestedCurrentItemChange

**Told to what the item left.** When a container's current item moves, every control from the item
that was current up to the first one still holding the new item hears it, the item itself first. A
grid standing in a container drops its selected cell on it - see Grid.

## Control reveal

**Opened from the host down**, so an outer body is opened ahead of one inside it. A holder says
through childSearch which of its children it hides and shows on request - an expander's body, a
grid section's rows - and opens one through revealChild. The control itself and the host are left
as they are. A search reveals a match this way; a page reveals a row a url names.

## LayoutPass

The form owns it, and a control reaches it through its AlignEvent.

**It holds one question**: whether everything laid out so far is at a size this pass measured it
against. A control laid out at a size the measure never saw says so through
`AlignEvent::invalidatePass`, and the form walks the tree again before the frame goes out.

**Only an align event can ask.** Outside a pass, call `FormBase::invalidateAlign`. Inside one, that
call is dropped.

**An invalid pass still lays everything out.** The next pass reads what this one wrote - remembered
wrap widths, body viewports, scroll ranges - so a control may not return early on `valid()`.

## invalidatePass

**It only asks.** The walk carries on, and the tree is walked again before the frame goes out.

**What it asks on must converge.** The next pass must hand back the same number, or the form lays
itself out again on every frame. A width in design units converges in one step.

## Remembered text width

**A wrapping text is measured at the width the last align broke it at**, rather than at the
control's maximum, which has no width unless a MaxSize gives one. Otherwise a two-line text would
measure as one line, and a panel's bar would lose a line.

- A changed width asks for one more pass, which hands back the same width.
- It is kept in design units, only while `isWidthGivenFromOutside()`. A form measuring for a
  placement ignores it, and an empty text keeps none - see Stack.
- It is the content's width. A control drawing its text in a narrower box takes the difference off
  in `measureText` - see ButtonBase.
- PanelBase remembers the body slot its text is in instead.

## Control::contentAroundText

**Adds what a control sets beside or over its text** - an icon, a mark. The default returns the
text alone.

**Both passes call it**: `calculateContent` with the text measured at the remembered width, and
`alignContent` with a wrapping text measured again in the granted box. So a control laid out at its
measured width keeps it. An empty measurement is not passed, and the granted box is kept.

An override of `calculateContent` that skips the base adds these itself. The align pass still calls
this.

## TraversalOrder

**Lets the viewport range be found by a binary search.** Set `x` or `y` for children sorted along
that axis, never both.

`laneExtent` is zero for a single lane. A wrapping container states its widest lane, since items in
one lane end at different offsets.

## ClickEvent

**Raised once, on the control the click is on.** A key's press is on the focused control. A mouse
click is on the control under the pointer, or on the nearest click target above it - see Control
isClickTarget. A mouse click on a label inside a tile is a click on the tile.

**NestedClickEvent follows it**, from the same control up to the root. A handler that stops the
ClickEvent spends the whole click, and no NestedClickEvent is raised.

**A handler that deletes its control stops the event**, or the walk that follows reaches the deleted
control.

## NestedClickEventBase

**What NestedClickEvent, NestedDoubleClickEvent and NestedTripleClickEvent carry** besides the
click itself:

- `control` is the control raising the event, whose handlers run.
- `clickedControl` is the control the click is on, the same at every step.

## NestedClickEvent

**Raised on the control the click is on, then on each parent in turn**, after ClickEvent.
OnNestedClick hears a click on its own control and on every control inside it.

**Any control on the way may stop it**, and the controls above hear nothing. A stack picks its
current item from it - the item `clickedControl` stands in - so stopping it below a stack keeps
that stack's current item where it was.

## Control isClickTarget

**True for a control the pointer addresses** - MouseOnly or Focusable - unless the control says
otherwise. A mouse click on a control that is not a target is a click on the nearest target above
it, or on the control itself when there is none.

**A part that stands for its host answers false.** A button's selection indicator does: a click on
the mark is a click on the button, read as a toggle, and a double click on it is spent by the
button.

**A control that answers the pointer without taking input answers true**, so a double click on it
is its own rather than that of a target above it.

## DoubleClickEvent

**Raised the way ClickEvent is**, on the control the click is on, and NestedDoubleClickEvent
follows it up from there.

**A double click is also a press.** The form reads it as one unless a handler stopped either event,
so a control that acts on a double click stops it.

## TripleClickEvent

**Raised the way DoubleClickEvent is**, and read as a press on the same terms.

**The platform window counts the run**, since Windows reports every press after a double click as
a plain press. A third press nobody handles arrives as that plain press, so ignoring this event
loses nothing.

## AdjustContextPopupEvent

**A handler adjusts the built-in menu here**, through `actions`. To replace the menu outright, stop
ContextPopupEvent, which comes first. Its control is what the menu stands on - a text box, a tree's
row.

## DestroyEvent

**A handler gets the control's identity and nothing else** - the derived parts are already gone.
Use it to drop a pointer to the control, as an Action does for each presenter.

## AdjustChildInsetEvent

**The inset the children sit at**, not a child's padding. It starts as the content padding, which
keeps text, icons and children together.

## InvalidateEvent

`None` and `InputController` are the invalidation alone. `HoverEnter` and `HoverLeave` also tell
each control the hover entered or left it. `PressDown` and `PressUp` invalidate only a control that
responds to the pointer.

## DropPopupEvent

**Raised before a control drops a popup from its click**, through mayDropPopup or
mayDropPopupImplicitly. It travels `nestedControlDroppingPopup` from that control up to the root,
the control itself first.

**Any control on the way may refuse the popup** by stopping the event. The control then drops
nothing, and the click goes on up.

**A container refuses to keep a popup off a step of its own gesture.** A grid row refuses an
implicit drop on the press that picks a cell, and every drop on a press that changes the
selection. See RowContainer.

**A click that drops no popup asks nothing.** A check box toggling in place and a button running
its command act on every click.

## Control mayDropPopup

**Asked by a part whose one job is the popup**, such as a dropdown strip: the click asks for the
drop.

## Control mayDropPopupImplicitly

**Asked by a control whose click has a popup assigned besides.** The click implies the drop, and
the event carries `implicit`:

- a combo box dropping its list from its main area;
- a caption opening its in-place editor, which is a popup like a list.

## Popup owner

**A part of another control's face answers that control** - a dropdown strip answers the control it
drops for. Any other control answers itself.

The popup's target, `FormBase::popupTarget`, is the owner whichever part opened it. So:

- a second press on any part of the owner closes the popup and does nothing else - a split
  button's face runs no command while its list is up;
- the whole control looks dropped down, and a popup placed by its target falls from the whole
  control;
- the popup closes when the owner is hidden or deleted, and stays up when only the strip is hidden.

## GetActionStateEvent

**The first control to claim the action is its subject**, and ActionClickEvent reaches it. Asked
upwards from each of these, then of the action itself:

- the presenter asking;
- the focused control;
- the control a popup form was opened on;
- the form's current item.

With no claim, the action is enabled while it has an OnClick handler.

## Action

**An action serves controls and owns none.** It serves several at once, and runs with no control at
all - a shortcut still runs it while the button sharing it is scrolled away and destroyed.

**Its state is asked for, never kept.** Enabled and selected belong to whatever the action acts on.
Call invalidateState() when the answer changes.

**A control given an action as a property presents it**, in the role its `PresenterRole` property
gives - `Button` unless it states another.

## Shortcut

**An empty shortcut matches no key** and adds nothing to a tooltip.

**An action holds any number, and each one runs it.** A menu line and a tooltip show the first.

- Each `Shortcut` property adds one, in the order stated.
- `addShortcut` adds one after those.
- `setShortcut` makes its key the only one, so `setShortcut({})` leaves none.

## ActionEventBase

**Either side answers**: a control claiming the action as its subject, or the action itself, with
`control` null.

## ActionClickEvent

**Reaches the control that claimed the action**, or the action itself when none did.

## Actions

**Each form holds one, and AppActions is the application's.** A key goes to the focused control,
then the form's actions, then AppActions.

**A shortcut belongs to a scope, not a control**, so it works while its button is scrolled away or
its menu is closed.

## ClipMode::None

**Visits every control in the subtree**, in view or not, in the order its containers hold them. For
a walk in document order rather than over what is on screen.

## Tooltip

**The framework drives it**, asking a control for its tooltip through nestedGetTooltip. A control
that must show its tooltip at once calls `form().tooltip().showRightNow(control)`.

**What a tooltip is about follows the controller:**

- While the mouse drives, the control under the pointer. A hover move hides the tooltip and starts
  the wait for the next.
- While the keyboard alone drives, the item the focus is on - a container holds the focus for its
  current item. The focus moving to another item does the same.
- When the mouse takes over, the hover counts at once, unless the pointer is on the focused item.

**A tooltip stays up while it is still the answer.** Moving onto a child with no tooltip of its own
keeps the tooltip of the control the child is in. One tooltip is in play at a time, in the form of
the control pointed at.

**A tooltip is placed where its control comes to rest.** For a control still gliding, the tooltip
goes where the glide ends and waits there. Later it moves with the control when the form is laid out
again, as a popup does. A control scrolled out of view leaves the tooltip where it is.

**Its size:**

- Lines break at k_lineWidth, 480 design units, and the window is as wide as the widest line.
- A tooltip over text repeats the lines the control broke, at the width the control broke them.
- At most k_maxLines lines show, 20, and the last fades when the text runs on.

## Control invalidateTooltip

**A tooltip keeps the words it was shown with.** A control that changes its tooltip's words while
the tooltip may be up calls this - a tooltip spelling out what the typed text means does.

A tooltip up about this control is then asked again and placed afresh. A tooltip about another
control stays as it is.

## GetTooltipEvent::writePreview

**For a tooltip previewing a longer text**, such as the paragraph an anchor link leads to. The
preview goes after what the tooltip holds.

**A preview stops after six lines** at Tooltip::k_lineWidth, ended by an ellipsis in the styles
still open there. A word the ellipsis would push onto a seventh line goes too.

An indent or alignment the preview states holds after it, so a line added after a preview states
its own.

## FocusNavigator

**It takes the keys no control on the focus path took**, after the form's shortcuts and the
application's: Return and Space, Tab, the arrows, Home, End and the page keys.

**The nearest ActiveContainer is the scope.** Tab steps out of it as a whole, the arrows and the
page keys move inside it, and Home and End go to its first and last stop.

- **An arrow** asks the focused item's parent for the child the step lands in - see
  Control navigationNeighbour. A parent with none hands the step to its own parent, up to the
  scope. The child named is entered down to a stop - see Control navigationEntry.
- **Tab** reads the order the containers keep their children in: the collection, or
  Control::navigationSlots where a container keeps one. Shift+Tab walks the same order back, and
  past the last stop the walk comes round to the first.
- **A page press** is as many arrow steps along the axis the scope scrolls on as fit in its
  viewport, the item it starts from counted. Where nothing overflows, it goes to the end.

**Entering an ActiveContainer lands on its current item**, whichever way the focus comes in.

## NavigationStep

**The line is what makes the way back the way forward.** A run of steps one way keeps one
coordinate: a vertical run keeps x, a horizontal run keeps y. Each step writes the other
coordinate into the stop it landed on, at the edge it crossed. Down from a button and Up again
come back to that button, however wide the item between them.

**Any other move of the focus starts a new run**, from the middle of the item it lands on.

**A container reads the line among its own children** - FormBase::contentOriginOfControl gives
their origin - against where they were laid out, never where a hold draws them.

## Control holdsNavigationStop

**Shown, enabled, and able to take the focus or holding a child that holds a stop**, read in the
order navigation reads the children in. Every child a container's navigation answers with holds
one.

## Control navigationNeighbour

**`child` is the child the focus is in, and the answer is another child**, holding a stop. Null
hands the step to the parent. The default answers null: a control has to say how its children
are placed before a step moves among them.

- StackPanelBase - along the lanes, the next item in the order. Across them, the item of the next
  lane nearest the line - see Stack.
- PanelBase - up and down from row to row, sideways along the middle row - see PanelBase.
- GridBase - null for an arrow, which the grid has walked over its field already. The steps of a
  page press walk the rows.
- TreeNode - Right from the header of an open node goes to its rows.
- RowExpander - Down from the header into its rows, Up from the rows back to the header.

## Control navigationEntry

**The answer is the child a step from outside lands in**, holding a stop. The default is the first
such child in reading order, whichever way the step comes.

- StackPanelBase - entered along the lanes, the near end of the lane nearest the line. Entered
  across them, the item of the near lane nearest the line.
- PanelBase - from above or below, the first row from that edge holding a stop. From a side, the
  row nearest the line, from that side.
- RowExpander - from above, its header; from below, its rows. From a side, the header while the
  line runs above the header's foot, the rows under it.


## Input

**One per application, not per form**, so every member is static. The focus leaves a control when it
arrives elsewhere, and that may be another window.

**A control asks about itself through Control::isFocused(), isHovered() and isPressed()**, rather
than comparing pointers with these. A container answers for the item it holds, and only those
helpers know it.

## InputController

**What the user does switches the two:**

- A pointer move, press or release turns the mouse on and the keyboard off.
- A key turns the keyboard on and the mouse off. A held key's repeats switch nothing.
- A bare modifier turns the keyboard on and leaves the mouse as it is.

The first move after the pointer comes into a form only hit-tests - see FormBase::wnd_mouseMove.

**`!Input::mouse().active()` tells whether a key did this.** A mouse click always finds the mouse
on. A key other than a modifier finds it off, unless the pointer moved during the key's repeats.

**The hover is where the pointer is**, moved only by the form's hit-test. A control reached by a key
reads its focus and the keyboard's factor instead.

**Paint reads the factors** through `RuleInput::Keyboard` and `RuleInput::Mouse`. A switch is one
crossfade, and each step repaints every window - see InputSwitchEvent.

## ContextMessage

**Says something once, about what the user did** - why a typed value was refused, that a file
was written. It shows in the tooltip window under its control, since the pointer may be anywhere.

**It lasts as long as that window.** Moving the pointer to another control, or any user action,
hides and forgets it, with no timer. A second message replaces the first.

**What a control says every time it is asked belongs in its nestedGetTooltip** - see PromptBox.

## GetShortcutEvent

**The attached action answers**, or an empty shortcut comes back. A control holds no pointer to its
action, so a menu item asks.

## AppActions

**The commands that belong to the application** rather than to one form. A key is looked up here
last. StdActions::registerAll puts the framework's own commands here.

## appMenuAction

**Every AppButton presents it from its constructor.** ApplicationBase::initialize connects its click
to the application menu, through connectAppMenu.

## TraversalMode

- `Manual` - the visitor calls `context.traverseChildren()` itself, from inside the visit. Use it
  when the visitor keeps data on the stack for the children's contexts.
- `Auto` - ControlTreeWalker calls `traverseChildren()`, and each child's visit runs in its own
  stack frame.


## FormBase::configName

**The name of a section in the application's Forms schema** - `MainForm` for the main form. A form
with no name stores and restores nothing.

**Set it before show().** The first placement hands the stored placement to the window, and on
Wayland it is the last moment the toplevel can be named to its session.

**The placement is stored on every hide**, and a close hides first - Alt+F4 and the compositor's
close included.

## FormBase::windowFocusedFactor

**The form animates it** on `AnimationSlots::WindowFocused` whenever its window takes or loses the
focus. A form not painted yet takes the value at once.

**A popup reads the window it opens over.** A Menu or Tooltip window never takes the focus, so a
popup returns its owner's factor, up to the first Dialog. A Dialog returns its own, owned or not. A
Wayland layer-shell Dialog, placed `ScreenRight`, gets no activated state and reads 0.

What reads it:

- The colour chain, through `AdjustPaintEvent::setWindowSelectedAmount` - how much of the selected
  factor is the window's focus. `DialogTitle` states 1, so its `Selected` rules follow its window.
- `RuleInput::WindowFocused`, alone or joined to another input in a rule's `andInputs`. `Focused`
  is the control's own focus alone. The band's focus rule reads "when focused and window focused",
  so a selection in an unfocused window shows only the band's resting rules.
- The focus ring's default rules: a keyboard focus ring is in the accent only while its window
  has the focus, and in the strongest ink otherwise.
- `TextBox`'s caret, drawn only while the factor is 1.

`FormFocusChangeEvent` marks the moment of each change. Paint reads the factor.

## PaintEvent windowFocusedFactor

**A control reads the factor here while it paints**, with no subscription. Each step of the
animation repaints the whole window, since the form cannot know which controls read it.

## FormBase::shadowDesign

**Scaled to the form, and coloured as the rule set colours a window's shadow** at the focus this
window holds.

The window's own shadow uses its root's rule set. A control over the content passes its own - the
notices layer passes Tooltip, so a card casts a tooltip window's shadow.

## FormBase::shadowPainter

**The form keeps the last four designs baked**, so the controls of one window share bakes. A painter
holds until four other designs are asked for, so ask for it where you draw rather than keeping it.

The window's own shadow has a separate painter.

## EditPhase

**A drag is one step.** Each `Held` change joins the step before it, and the `Settled` change raised
as the press comes up closes it. `SliderBase::editPhase` answers it for a slider.

## IEditHistory

**Fits any history** - deltas of a text, whole copies of a theme. A walk past the history's depth
does nothing.

**The owner applies a walk** and reports one change for it. It puts the caret or the selection where
the **oldest** step undone was made, or where the **newest** step redone was.

## GetEditHistoryEvent

**Asked by whatever lists the steps**, of the control that claims Undo or Redo. That control sets
`history` to the history its click acts on. A HistoryButton asks from its strip, so the list is
what its face would walk.

## StepHistory

**One list, split by a cursor** into steps in force and steps undone. A new step drops the undone
ones and becomes the newest.

- The newest step can be open to a run. The owner decides what joins it, and a walk either way
  closes it.
- The store grows from `firstSteps` to `maxSteps`, then each new step replaces the oldest. push
  returns the step it let go.

## StateHistory

**A StepHistory of whole copies**, for a subject small enough to copy per step and put back by one
sync. It also holds the state before the oldest step, where undo stops.

**A held run keeps one step** - the first step's name and place, and the latest state - until a
settled step closes it.

## WithTextLayout

**Derive from it for a text that changes often** - a slider's value, a frame rate, the size of a
rectangle being dragged. Deriving is the whole declaration.

**TextEngine's cache is keyed by what a text says**, so a text rewritten every frame misses on every
paint and evicts the layouts the rest of the form draws from. A host here shapes its own text in
place.

## Form

**Call the root's members on the form:** `form.createTopBar<DialogTitle>()` rather than
`form.content().createTopBar<DialogTitle>()`.

## SearchResult

The application keeps one per page, and a searcher may serve several pages.

**The container keeps a pointer to it** and nothing else of the search. Either may go first: a
container gone is forgotten, and a result gone takes itself back.

**The paint shows it.** Every text inside the container is drawn with the searcher's places on the
found band, and the current match as a text box draws a selected match.

What the search reads:

- a control's own texts, then its children in the order it holds them - see Control::searchTexts;
- a hidden child only where its parent reads it shut or open (`ChildSearch::Revealed`) - an
  expander's body, a row expander's and a folding group's rows, whose matches count while shut;
- a container with a search of its own, through that search.

It skips what a control is made of, such as a scroll box's bars.

**What was found is read again when next asked**, after anything moved:

- Whatever asks for a new layout - a text written, a control added, shown or hidden, an edit -
  calls invalidateFound, and so does a caret moving.
- A text that changes without a new layout, such as a cell whose data moved, is reported the same
  way by whoever changed it.
- A control deleted under the container is reported by the container, through controlDeleted.
- New words are noticed by the searcher's revision.

currentHolder is the control the current match is in - the one an application focuses to hand the
focus back to what was searched.

## SearchResult step

**A step is measured from the reader's place**: the match a step made current, or the selection a
text box keeps there, which follows the caret. With neither, the reader is on the first selection
the walk meets.

**A caret in a match, or against either end of it, is on that match.** A caret where two matches
meet is on the second.

FindTarget says which match a step takes. Previous goes back from the match the reader is on, or
from the first match after the reader.

- Where the reader's text holds no match, the next text holding one is taken - going back, the last
  one before it.
- Past either end is the other end.
- With nowhere to stand, a step takes the first match, or the last going back.

**A step shows the match.** A hidden body on the way is opened first, from the outside in. Then the
holder's showFound shows it:

- a text box selects the match;
- a row selects the cell, as a press would;
- anything else is scrolled into view.

Where a body was opened, or the controls were built since the last layout, the view goes to the
match once they are laid out.

**A step takes back the match the last one showed**, where it shows one in another text. The
holder's hideFound does it: a text box collapses its selection onto the caret, unless the
selection has moved off the match since.

## SearchResult stepFrom

**The control is read from** the selection of the first text the walk meets in it, or else that
text's start. So a page opened at a member continues the search from there.

## Control searchTexts

**Hands the search each text the control shows**, by part, with the selection the control keeps in
that text. A step is measured from that selection.

- The control's own text is part 0.
- A row numbers its cells by the cell walk, and reads a cell holding a control with that control.
- A button showing its icon alone shows no text.
