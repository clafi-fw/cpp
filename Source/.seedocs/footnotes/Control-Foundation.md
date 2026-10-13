# Control Foundation

The words that no longer fit above a declaration in `Control.cppm`, `RichControl.cppm` and
`Form.cppm`. Referenced from those files.

## RichControl

Adds to the Control class the stored appearance state - metrics, colors and text.

The event connectors and the `On...` handler properties live on Control, because the dispatcher
they run over is contributed by Control. RichControl only feeds its own state into an event
before letting the base emit it.

## Control text

A `ControlText` rather than a `Text`: the control's own text is written once and then asked
about on every measurement and every paint, which is what a stamp is for. It is handed out as
the live type so that what a caller writes through it is counted.

## Text format

What a control's text is laid out over - see TextEngine-Types#textformat. Control answers none.
RichControl keeps one, stated through the TextFormat property - several compose kind by kind, a
later one replacing what an earlier one said of the same kind - or through setTextFormat, which
replaces the whole of it. It answers none while the format is empty. A class states its format as
a property ahead of the arguments it forwards, never through setTextFormat in its constructor body:
the body runs after the caller's properties were bound, and would wipe them.

It is read where editProps is read. Control::drawText and Control::measureText hand it to the
engine. WithTextLayout states it to its own layout and keeps it beside its last measurement. A
TextBox lays its placeholder out over it, an in-place editor lays its text out over the covered
control's, and an over-text hint repeats it: GetHintEvent::format carries it and HintLabel answers
with it.

Stating one invalidates nothing, the way setColorRules does not. The caller asks for the pass.

## FormAlignedEvent

The form's content has been laid out and nothing has asked for another pass. `sender()` is the
form. Raised at the end of the pass, so every rect in the form may be read from a handler; a
message loop may not be started from one.

**A form nobody can see is not placed.** A setter that moves what the placement reads -
`setPlacement`, `setPlacementRect`, `setMinWidth` - places a visible form again at once and
leaves a hidden one for `show()`, so the pass that raises this event runs with the form on
screen. A handler that ignores a hidden form, as the completion list's hint does, hears the
pass that lays out the form it is about to see.

## RootLoopEvent

A form has taken the loop the application stands on, and its first layout has not run yet.
`sender()` is the form. Raised on the application's dispatcher - `AppContext::events()` - each time
a form takes the root loop, which makes that form the main window - see FormBase::holdsRootLoop. It
is where whatever belongs to the main window is put on it before the window is first shown: the
application's notices come over the window this way.

## FormBase::topLayer

**A control drawn over the root, inside the window**, in the box the root is laid out in. A form
holds one at most. setTopLayer puts one in and takes the one standing away, and the form owns it
from then on. The layer is a root of its own - built with a CreateParams over the form, with no
parent - and every pass that lays the root out into the window lays the layer out after it, in the
same box. A pass measuring what to ask a window for leaves it out, so nothing over the content has
any say in the size of the window.

**Painted after the root and met by the pointer before it.** A point the layer meets nothing in but
itself - the open space around what stands in it - goes on to the root, so a layer that fills the
window takes the pointer only where it shows something. It takes no part in the focus walk, which
runs over the root, and a control in it that is to leave the focus where it is says MouseOnly. The
form forgets its controls as they go, as it forgets the root's.

## FormClosingEvent

The form is asked to close from outside - the window's own close button, the system, a click
on the form behind a popup. `sender()` is the form. A handler holding work the user has not
finished settles it here, or calls `refuse()` and the window stays up; the press or the request
that asked does nothing else either. Raised by `FormBase::readyToClose`, which answers with
whether anything refused, and a form with an answer of its own overrides that instead - an
in-place editor answers by whether its value was taken. `requestClose()` is the ask followed by
the close, and is what the title bar's close button and the platform's close request both call.
`close()` never asks: Escape has to work whatever state the form is in.

## FormPaintedEvent

The form has painted: the rect it was asked to paint, and when the paint started and finished.
`sender()` is the form.

**Raised on the application's dispatcher** - `AppContext::events()` - and not on the form's own.
Whatever watches paints follows them from window to window, and a connection to a form must not
outlive the form, while the application outlives every form. Raised only while something is
connected there: a form nobody watches reads no clock.

## FormControlBase

The root control of a form, and the other half of the FormBase pair.

A window looks the way its root's properties say: the metrics block, the colour set and the
WindowShadow the root states. The window's role is read for none of them, and a root that leaves
one out wears none of it.

## Control

Provides core control functionality with a minimal memory footprint.

Do not create controls by calling constructors directly. Use the factory `add-` or `create-`
methods provided by the parent control or the form.

Its event connectors add no per-instance storage - each one is a member function over the
EventDispatcher that EventComponent already contributes to every Control - so a control that
connects nothing pays nothing for them.

## Control::animate

**Runs this control's animation through the application's controller**, which a control reaches
through its form - `animator()` answers it, or null while the control stands in no form, there
being no application to reach then. A control in no form has nothing to run an animation on
and takes the end value outright: nobody sees it, and the end value is what it is drawn from
once it is seen. That is what `setExpanded` on a header built expanded, or a grid's cell
selected before the grid is shown, come to. `stopAnimation` and `stopAnimations` are the same
route for the stops, and stop nothing where there is no controller to have started anything.
`GridDescriptor::animateCell` gives the same answer for a cell, whose key is the cell rather
than a control.

## LayoutPass

The state one layout pass keeps. The form owns it, and every control reaches it through the
`AlignEvent` it is measured and laid out with.

It holds one question: whether everything laid out so far stands at a size this pass measured it
against. A control that finds it does not - a wrapping panel broken at a width the measure never
saw, a scrolled body handed a viewport the measure ran ahead of - says so through
`AlignEvent::invalidatePass`, and the form walks the tree again before the frame goes out.

**Only an align event can say it.** `invalidate` is private and `AlignEvent` is the friend, so the
request is unspellable outside a pass. `FormBase::invalidateAlign` is the route from outside one,
and it is dropped where it is raised inside a pass - `updateAlign` marks the form aligned before the
walk begins.

**Reading it is not skipping work.** `valid()` is there for a control with something to decide on
it. A pass that has been put in question still has to produce a layout the next one can be measured
against: the remembered wrap width, the viewport a box states for its body, the range a scroll bar
carries the view across are all written by the align. A control that returned early on the flag
would leave the pass after it reading numbers nobody wrote, and the walk order would decide which
ones.

## invalidatePass

Says the control being laid out has been given a box the measure did not answer for, so the tree
is walked again once this walk ends.

**It asks, it does not lay out.** The walk is in progress and the sizes around it are half of one
answer and half of another, so nothing is recomputed on the spot.

**What it is asked on has to converge.** The pass that answers it must hand back the same number, or
every pass asks for another and the program lays itself out until it is killed. A width carried
over in design units converges in one step, because the pass measured against it produces that
width again. A height does not: `Control::calculate` ceils the content it measured and the align
pass does not, so a fraction of a unit separates them for ever.

## Remembered text width

A wrapping text is measured at the width the last align broke it at. The measure runs against the
control's own maximum, which states no width unless a MaxSize does, so without that width a text
breaking into two lines in its box measures as one - and a control whose measured height is final,
a panel's bar, is laid out one line short with the rest collapsed onto it.

`Control::alignContent` remembers the content width it re-wraps the text at, and `calculateContent`
measures against it - `textWidthLimit`. A width that has changed asks for one more pass through
`invalidatePass`, and that pass hands back the same width. A control breaking its text somewhere
else states its own: `PanelBase` remembers the body slot its text stands in.

The guards are the ones every remembered width needs - see Item-Containers: design units,
`isWidthGivenFromOutside()`, and nothing while the form measures for a placement. An empty text
remembers nothing.

The width remembered is the content's. A control drawing its text in a narrower box takes the
difference off in `measureText`, so both passes break the text where the paint does - see
Controls-Base#buttonbase.

## Control::contentAroundText

**What the content comes to, given what the text came to.** The default is the text alone. A
control that sets something beside or over its text - an icon, a mark - adds it here.

Both passes go through it. `calculateContent` hands it the text measured against the remembered
width, and `alignContent` hands it the text measured again in the box the control was granted. A
control laid out at the width it measured therefore keeps that width, and its text keeps the box
it was measured for. An empty measurement is not handed in, and the granted box stands.

A control that overrides `calculateContent` without calling the base answers the measuring pass
itself, and adds what stands beside its text there; the align pass still comes here.

## TraversalOrder

Describes how a container orders its children, so the viewport range can be narrowed by a binary
search instead of a full scan. An axis flag means the children are sorted along that axis. At
most one flag may be set - two sorted axes at once do not define a single ordering.

`laneExtent` is the distance from a child's leading edge to the far edge of the lane holding it.
Zero means the container has a single lane, so a child's own trailing edge is the exact cull key.
A wrapping container reports its widest lane, which bounds every child's trailing edge from
above: a child's own trailing edge is not usable there, because items sharing a lane are aligned
individually within it and so end at different offsets.

## TripleClickEvent

The third press of one run of clicks. Windows reports the second press as a double click and
every one after it as an ordinary press, so the run is counted by the platform window - see
`FormWindow`. A press that no control reads as the third of a run is delivered as the press it
also is, which is why a control that does nothing with this one loses nothing by ignoring it.

## EditContextPopupEvent

Raised by a control that is about to show its own built-in edit menu, so that a handler can
adjust that menu instead of replacing the context menu outright.

A handler stops `ContextPopupEvent` to take the menu over entirely. A control raises this one
only after `ContextPopupEvent` went unhandled, which is why it is a separate type rather than a
second listener on the same one.

## DestroyEvent

The control is being destroyed. A handler gets identity and nothing else: the derived parts of
the object are gone by the time the base emits, so a virtual answers for Control and whatever a
derived class owned has been released. What this is for is dropping a pointer to the control - an
Action holds one per presenter and has no other way to hear that it has gone.

## AdjustChildInsetEvent

Where a control's children begin, as an inset from its own top left. Not the padding of a child -
the inset the children sit at. It starts as the control's content padding, which is what keeps
text, icons and children together for everything that does not separate them.

## InvalidateEvent

Why a run of states is being invalidated, which decides what each control on the way is told
besides the invalidation itself. `None` is the invalidation alone.

## DropPopupEvent

A control about to drop a popup from the click it was given asks first, and the question it asks
says what dropping the popup is to that click. A part whose one job is the popup - a dropdown
strip - asks `Control::mayDropPopup`: the click asks for the drop. A control whose click has a
popup assigned to it besides - a combo box dropping its list from its main area, a caption opening
its in-place editor - asks `Control::mayDropPopupImplicitly`: the click implies the drop. An
in-place editor is a popup, so it asks what a list does.

Both raise this event, with `implicit` set by the second, and walk `nestedControlDroppingPopup`
from the control up to the root, the control itself first, the way `nestedControlFocusing` walks.
Any control on the way may stop it. A stopped event refuses the popup: the control drops nothing,
and leaves the click to go on up.

This is how a container keeps a popup off a click that is a step of its own gesture. A grid row
refuses an implicit drop on the press that picks a cell, and every drop on a press that changes
the selection - see Grids#picking.

A click that drops no popup asks nothing. A check box toggling in place and a button running its
command act on every click they are given.

## Popup owner

**The control a popup is about**, answered by `Control::popupOwner`. A control answers itself; a
part of another control's face answers that control, which is how a dropdown strip says it is half
of the control it drops for. The form takes the answer as the popup is built, so
`FormBase::popupTarget` is the owner whichever part the popup was opened on - through `dropPopup`,
a `Menu` built on the pressed part, or a command whose presenter is the strip.

A press asks the same question of the control it lands on. The second press on the owner, on any
part of it, closes the popup and does nothing else: the strip and the face close a list either of
them opened, and a press on a split button's face while its list is up does not run the face's
command.

Everything read off the target reads the owner: the whole control looks dropped down, a popup
placed by its target alone falls from the whole control, and the popup goes down when the owner is
hidden or deleted - hiding the strip alone leaves it up.

## GetActionStateEvent

An action asks its two questions with `GetActionStateEvent` and `ActionClickEvent`, of each
control from the focused one upwards. A control answers them to volunteer as that action's
subject.

## Action

An action given to a control as a property takes that control as a presenter, in the role
`PresenterRole` names.

## Shortcut

A key and the modifiers held with it. An empty shortcut is one an action does not have:
no key matches it, and it contributes nothing to a hint.

**An action holds any number of them, and every one runs it.** The first is the one it is known
by - a menu line and a hint show that one alone. A declaration states them in that order, one
`Shortcut` property each; `addShortcut` puts one more after them, and `setShortcut` makes the key
it is given the only one, so `setShortcut({})` leaves the action with none.

## ActionEventBase

What both action events carry. An action asks its two questions with these and no others,
and each is answered on either side - by the action itself, or by a control that
volunteers as the subject.

## GetActionStateEvent in Action

Whether the action can be run, and by whom. Asked of each control from the focused one
upwards, and of the action last.

## ActionClickEvent

Run the action. Reaches whichever control claimed the state, and the action itself when
none did.

## Actions

A set of actions a key can be looked up in. A form holds one for the commands that
belong to it, and there is one for the application - see AppActions below.

A shortcut belongs to a scope rather than to a control: bound to a control it would stop
working whenever the button showing it was scrolled away or its menu closed.

## ClipMode::None

Every control in the subtree, in the order its containers hold them. Geometry names
nothing here: neither the viewport nor a system clip rect narrows the walk, and a
control the view is scrolled away from is visited like any other. For a walk that is
about document order rather than about what is on screen.

## Hint

Hint is an internal class used by the framework - never use it directly

**What a hint is about follows the controller.** While the mouse drives it is the control under the
pointer, and the hover moving is what takes a hint down and starts the wait for the next. While the
keyboard alone drives it is the item the focus rests on - the control a key landed on, which a
container holds the focus for - and the focus landing does the same. A landing is the focus coming
to another item. A list going down hands the focus back to the box it was dropped from, which held
it all along, so a hint that went down with the list stays down. Each hears its own controller and
not the other: the hover moves under a still pointer, when a window appears under it or a hit-test
follows a scroll, and a click lands the focus with the pointer's hint already standing for it. When
the mouse takes over, the hover has its say at once, wherever the pointer was left - unless it rests
on the very item the focus is on, where nothing changed hands. A hint already up is left standing
while it is still the answer the control pointed at would be given - its own, or that of a control
it stands in: crossing children that have no hint of their own is not leaving the control that has
one.

**A hint up is asked again when its words move.** Its window is measured for the words it went up
with and keeps them on screen while it stands, so a control that changes the words of its hint
while the hint may be up - a hint spelling out what the text typed into it stands for - calls
invalidateHint. A hint up about that control is asked again the way the hover asks it, from
that control up, and new words are measured and placed afresh. A hint up about any other control
is left as it stands.

**At most one is in play.** Which form's hint that is follows the control pointed at.

**A hint stands where its control comes to rest.** A control a glide is still carrying - a list
scrolling to the row a hint was put up beside - is measured where the glide will leave it, so
the hint goes there at once and stands still while the content travels to it. After that the
window goes with the control the way a popup goes with the control it was opened on: a pass
that lays the form out again moves it by the control's step. A control scrolled out of its view
is nothing to follow, and the hint stands where it was put.

**A hint breaks its lines at** k_lineWidth, 480 design units, and its window is sized to the widest
line it came to - a short hint stays as short as its words. The one exception is a hint standing
over text: it repeats the lines the control broke, at the width the control broke them. Either way a
hint shows k_maxLines lines at most, 20, and fades the last of them when the text runs on.

## GetHintEvent::writePreview

A hint written as a preview of a longer text - the paragraph an anchor link leads to, the footnote
a mark leads to. The preview goes after what the hint holds already.

**A preview stops after six lines.** The text is laid out at Hint::k_lineWidth, the width the hint
breaks its lines at, and cut where the sixth line ends. An ellipsis ends what is left, in the
styles still open where it stands - muted inside a muted run, regular after a bold word that ends
at the cut. A word the ellipsis would push onto a seventh line goes with the rest, and a line
holding one word longer than itself gives up characters instead. The cut costs one shaping of the
text on top of the hint's own.

An indent or an alignment the preview states holds for what is written after it, so a line added
under a preview states its own.

## Score

How one candidate is ranked against another when a key has to choose between them. A
candidate on the lane the move keeps is taken over one merely ahead of it, however much
nearer the second one is.

## Input

The input state the whole framework reads: which control holds the focus, which one the
pointer is over and in which of its zones, whether a mouse button is down, and which of the
two input controllers are on. There is one of each per application rather than per form - the
focus leaves a control when it arrives somewhere else, and where that is may be another
window - which is why they are static here instead of being members of FormBase.

A control asks about **itself** through Control::isFocused(), isHovered() and isPressed()
rather than comparing pointers against these. A container answers for the item it holds,
and only those helpers know it.

## InputController

The mouse or the keyboard, as the rest of the framework sees it: whether it is on, and a factor
that follows that through its own animation slot. The two are switched by what the user does:

- Anything the pointer does - a move, a press, a release - turns the mouse on and the keyboard
  off. The first move a form hears since the pointer came in is not one of them: Win32 sends a
  window that appears under a still pointer a move, and a Wayland enter arrives as one, so that
  move only hit-tests - see FormBase::wnd_mouseMove.
- A key turns the keyboard on and the mouse off.
- A bare modifier - Shift, Ctrl, Alt - turns the keyboard on and leaves the mouse as it is. It
  qualifies whatever comes next, from either device, and the pointer is still where the user
  left it.

So both are on only after a modifier, until the next pointer event or key, and the one state
that cannot occur is both off. A held key's repeats switch nothing.

`!Input::mouse().active()` is how code asks whether a key did this. A click, or a menu the
pointer raised, always finds the mouse on. A key other than a modifier finds it off, unless the
pointer moved while that key was held down and repeating.

**The hover is where the pointer is**, and only the form's hit-test moves it. A control reached by a
key reads through its own focus and the keyboard's factor instead: the default rules gate each
look the pointer lights on the mouse and repeat it "when focused, and keyboard", so with one
controller on, one item is lit. `FormBase::mouseTick` is the hit-test, and it is no act of the
mouse's - the platform handlers turn the mouse on, so a hit-test after a scroll or an edit leaves
the keyboard as it is.

The factors are what paint reads, through the colour rules: `RuleInput::Keyboard` and
`RuleInput::Mouse`. The focus ring's default rules are built on them - see Focus ring in AppTheme.
The two slots mirror each other, so a switch from one device to the other is one crossfade, and
each step of either fade repaints every window - see InputSwitchEvent in Context.

## ContextMessage

Something said about a control without stopping the user: why the value they typed
was refused, that the file they asked for was written. It is shown in the hint window,
under the control it is about.

**A message is raised by something the user did.** That is what tells it from a hint,
which is about whatever the pointer happens to be on. It is placed under its control for
the same reason: it answers an action, and the hand that took that action left the pointer
wherever it happened to be.

**It lives exactly as long as the window showing it.** The pointer moving to another
control takes it down, and so does anything the user does - the same two things that take
a hint down - and it is forgotten as it goes, so there is nothing left to point at it
for again. A message raised while the pointer rests on the control it is about therefore
stands for as long as the pointer rests there, and needs no time of its own.

**One at a time.** There is one hint window, so a second message takes the place of
the first rather than queueing behind it.

**What a control has to say every time it is asked** belongs in its nestedGetHint, not
here. A message is what is said once, at the moment it becomes true. A control that must
go on saying it answers for itself as well - see PromptBox and the value it has refused.

## GetShortcutEvent

What key runs a control's action, asked by a control that shows the key beside the
command - a menu item does. Whatever action is attached answers, and an empty shortcut
comes back when none is: a control holds no pointer to its action, and asking is what a
channel is for.

## Action in Action

A named command: what it says, what it draws, what it does, and whether it is
available now.
An action is not a control and does not own one. It serves several controls, and it
must be able to act with no control at all - a shortcut pressed while the button that
shares it is scrolled out of existence still runs it. Every member is therefore stated in
terms the action can answer on its own.
It stores no state. Enabled and selected are properties of whatever the action acts
on, so they are asked for and never kept; invalidateState() is the one way to say the
answer has changed.

## AppActions

The application's scope: the commands that belong to the application rather than to one
of its forms, and the scope a key is looked up in last.

The framework's own commands fill it: StdActions::registerAll puts them here, and they
are declared above Core, in ClaFi.StdActions, because an action carries an icon and
Icons sits above Core.

## appMenuAction

**The one command every application presents the same way**: an AppButton is a presenter of this
action from its constructor, and ApplicationBase::initialize connects its click to the application
menu through connectAppMenu.

## TraversalMode

TraversalMode::Manual :
The visitor must call context.traverseChildren() manually from within the visit event.
Use this mode if the visitor creates data on the stack that needs to be passed down to the children's contexts.
TraversalMode::Auto :
traverseChildren() is called automatically by ControlTreeWalker.
In this mode, child OnVisitControl events each execute within their own stack frame.

## OrientedRect

A rectangle turned so that the direction of travel runs up the primary axis, whichever key
the move came from. One comparison then answers for all four keys: a candidate is ahead
when its primary span starts past the source's, and on the same lane when the secondary
spans meet. Left and Up mirror their axis, so the values are negated and only differences
between two oriented rectangles are meaningful.

## FormBase::setConfigName

**The name a form's placement is kept under** in the application's config - the name of a section
the application declared in its Forms schema, `MainForm` for the one every application has.
Set before show(): the first placement is what hands the stored placement to the window, and
on Wayland that is also the last moment a toplevel can be named to its session. A form with no
name stores nothing and restores nothing. The placement is written on every hide - a close hides
first - so a system close, Alt+F4 or the compositor's, stores it too: the platform routes those
through IForm::wnd_closeRequested, which closes the form the way its own close button does.

## FormBase::windowFocusedFactor

**How far a form's window holds the focus**, from 0 to 1. The form animates it itself, on
`AnimationSlots::windowFocused`, whenever the platform reports a change through
`wnd_focusChanged`: `WM_SETFOCUS` and `WM_KILLFOCUS` on Win32, the **activated** state of a toplevel
configure on Wayland. A form not painted yet takes the value outright.

**Nothing subscribes.** A control reads the factor while it paints, through
`PaintEvent::windowFocusedFactor`, which is read from the form once at the root of the paint and
handed down the chain. The form cannot know which controls read it, so every step of the
animation invalidates the whole window.

**A popup reads the window it stands on.** A Menu or Hint window never takes the focus -
`WS_EX_NOACTIVATE` on Win32, and an `xdg_popup` has no **activated** state - so a popup answers its
owner's factor, up to the first Dialog. A Dialog answers its own, owned or not. A Wayland
layer-shell Dialog, placed `ScreenRight`, is sent no **activated** state and reads 0.

What reads it:

- The colour chain, through `AdjustPaintEvent::setWindowSelectedAmount` - how much of the
  selected factor is the window's focus. `DialogTitle` states 1, so its `Selected` rules follow
  its window.
- `RuleInput::WindowFocused`, which a rule reads on its own or joins to another input in its
  `andInputs`. `Focused` is the control's own focus alone. The band's focus rule reads "when
  focused and window focused", so a selection in a window without the focus shows only the
  band's resting rules.
- The focus ring's default rules: the ring is drawn in the accent only while its window has the
  focus, and stays in the strongest ink otherwise.
- `TextBox`'s caret, drawn only while the factor is 1.

`FormFocusChangeEvent` marks the moment of each change; the factor is what a paint reads.

## FormBase::shadowDesign

The Design of a shadow cast inside this window: the WindowShadow's blur, offset and spread and the
corner radius, scaled to the form, in the window's own shadow colour - the root's rule set and the
focus the window holds. A shadow built this way reads as one more of the window's. NoticesLayer
builds its cards' shadow from the theme's secondary window shadow, the one menus cast.

## FormBase::shadowPainter

A ShadowPainter baked for the design. The form keeps the last few designs asked for and bakes the
oldest over when a new one comes, so the controls of one window share their bakes: the cards over
it one, its held headers another. **A painter handed out holds until four other designs have been
asked for**, which is longer than the paint that asked for it, so it is asked for where it is drawn
rather than kept. The window's own shadow has a painter of its own, which the platform reads.

## Edge reach

**A pointer pushed against a maximized window's edge meets what stands k_edgeReach in** - eight
design pixels. FormBase::searchPoint moves a point that close to the edge to that depth before
the search, so a press against the top lands on a tab on the title and one in the corner on the
close button, rather than on the root's border ring or the band above the tabs.

The geometry is what the screen shows on both platforms - Win32 takes the overhang off it, see
WindowFrame in Context - so the pointer reaches the edge row on both, and the search is what
puts it inside the controls. What a maximized window gives up is the band itself: the strip
above a title's tabs answers as the tab, so it no longer drags the window.

A point outside the window is left alone. A drag carried past the edge keeps the coordinates it
has, and the positions handed to a drag are the pointer's own - only what the pointer meets is
moved.

## EditPhase

Where an edit stands while a history takes it. Held is the pointer still on the value - a slider's
thumb or slot under a press - and every held step joins the one before it, so a drag is one step
however many changes it raised. Settled stands on its own, or closes the run the pointer made:
the press coming up raises one more change, settled, and that is the step's last state.
`SliderBase::editPhase` answers it for a slider's change.

## IEditHistory

What an undo list and the Undo and Redo actions read of a history, whatever it keeps - deltas of a
text, whole copies of a theme. Two depths, the name of each step the way the list shows it, and a
walk of any number of steps each way; the newest step is 0 going back, the nearest undone one is 0
going forward. A walk the history does not reach does nothing.

The owner applies what it keeps - puts the text or the controls back, and the caret or the
selection where the step left the user: where the **oldest** step undone was made, where the
**newest** step redone was. A walk of several steps is one walk, so the owner reports one change for
it.

## GetEditHistoryEvent

Asked of the subject of Undo or Redo - the control that claims the action, found the way the
action finds it - by whatever lists the steps. The subject writes the history its click acts on.
A HistoryButton asks it from its strip, so the list is always the history the face would walk.

## StepHistory

The bookkeeping every undo history shares, over steps of whatever the owner keeps: one list in the
order the steps were made, and a cursor splitting it into the steps in force and the steps undone.
Undo and redo move the cursor; no step is ever carried between two stacks. A new step drops the
undone ones first - they branch off a past the new step has left - and becomes the newest.

The newest step can be open to a run: the owner decides that a step joins it, and the history only
says which step that is. Walking either way closes it.

The store grows from firstSteps to maxSteps and stays full there, the next step taking the oldest
one's place; push answers the step it let go, for an owner whose first state rides on it.

## StateHistory

A StepHistory of whole copies: each step holds the state it left, its name and where it was made,
and the history holds the state before the oldest step, where undo bottoms out. Right for a subject
small enough to copy per step and put on by one sync. A held run keeps one step - the first
step's name and place, and the latest state - until a settled step closes it.

## WithTextLayout

A control that holds its own text layout instead of taking one from TextEngine's cache, for a
text that changes.

The cache is addressed by what a text says, which is what lets a hundred buttons saying one word
share a single shaping. A text rewritten every frame - the value under a slider's thumb, a frame
rate, the size of a rectangle being dragged - therefore misses on every paint, takes a fresh
entry, and evicts the static layouts the rest of the form draws from: a drag of a few seconds
empties the cache and leaves every label on the form to be shaped again. A host here shapes its
own text in place instead - no hash, no comparison against another control's words, nothing
evicted, and nothing of its own in there to be evicted from.

Using it is the declaration. A control whose text changes often is one that derives from this,
and a control whose text is written once and then stands does not. There is no property to state
and nothing to keep in step.

## Form

The root control is a base rather than a member, so a form is called like the control it hosts -
`form.createTopBar<DialogTitle>()` instead of `form.content().createTopBar<DialogTitle>()`.

## SearchResult

**A search result is what one searcher finds in the texts one container shows, and the match
standing current.** The application keeps one per page, beside a searcher it may share between
pages. Made over a container, it hands itself to that container, which keeps the one pointer and
nothing else of the search. The paint hands it down to every control inside: each text is drawn with
the searcher's places on the found band, and the current match in the colour a text box gives a
match it has selected - a text box shows its own selection, which a step has put on the match.
Either may go first: a container gone is forgotten, and a result gone takes itself back from its
container.

**A match is one place in one text.** A control names its texts by part: its own text is part 0, and
a row numbers its cells by the cell walk, a cell holding a control being read with that control. A
button showing its icon alone shows no text. The texts are read in the order the container holds its
children, a control ahead of what it holds. A hidden child is passed by, and so is what a control is
made of - a scroll box's bars and the corner between them - and a container keeping a search of its
own is read by that one. An expander's body, a row expander's and a folding group's rows are read
shut or open, so their matches count while they are shut.

**What was found is read again when next asked, after anything moved.** Whatever asks for a new
layout - a text written, a control added, shown or hidden, an edit - tells the search over it
through invalidateFound, and so does a caret moved. A control deleted under the container is told by
the container, through controlDeleted, and the reader standing in it is forgotten: an address it
leaves may be taken by a control built after it. A text that changes without a new layout, a cell
whose data moved, is told the same way by whoever changed it. New words are noticed by the
searcher's revision. One walk over the container answers the count and the current match: it keeps
the texts holding matches and how many each holds, and a step reads the places of one text again.
currentIndex counts the matches in reading order, and currentHolder names the control the current
match is in - the one an application focuses to hand the focus back to what was searched.

**A step is measured from where the reader stands**: the match a step made current, or the selection
a text box keeps there, which follows the caret; with neither, the first selection the walk meets.
The current match is the place the reader stands on, ends included, so a caret in a match or against
either end of it stands on that match, and a caret where two meet stands on the second. AtSelection
takes the first match at or after the reader, so typing a search keeps a match that still holds;
Next takes the first after it; Previous takes the one before the match it stands on, or before the
first match after it. Where the reader's text holds no match, the next text holding one is taken, or
going back the last one before it. Past either end is the other end, and with nowhere to stand a
step takes the first match, or the last going back. stepFrom puts the reader where a control under
the container is read from - the selection of the first text the walk meets in it, else that text's
start - and takes the first match at or after it, so a page opened at a member goes on with the
search from there.

**A step shows the match.** A hidden body on the way is opened first, from the outside in, and the
view goes to the match once the opened body is laid out - as it does for a match in controls built
since the last layout, a page just made. A text box selects the match, a row selects the cell as a
press would, and anything else is scrolled into view. The holder the current match left and the one
it reached are repainted.
