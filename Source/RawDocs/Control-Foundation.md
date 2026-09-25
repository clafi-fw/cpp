# Control Foundation

The words that no longer fit above a declaration in `Control.cppm`, `RichControl.cppm` and
`Form.cppm`. Referenced from those files.

## RichControl

Adds to the Control class the stored appearance state - metrics, colors and text.

The event connectors and the `On...` handler properties live on Control, because the dispatcher
they run over is contributed by Control. RichControl only feeds its own state into an event
before letting the base emit it.

Its metrics properties - `ControlMetrics`, `MinSize`, `PreferredSize`, `MaxSize`, `Padding`,
`Spacing`, `Border`, `Radius`, `FixedSize` - are forwarded into fields of `m_metrics` by the
`BIND_` lines in the constructor rather than declared. Their defaults belong to `ControlMetrics`
and not to RichControl, which is why a `DECLARE_PROPERTY` here would be the wrong place to state
one.

## Control text

A `ControlText` rather than a `Text`: the control's own text is written once and then asked
about on every measurement and every paint, which is what a stamp is for. It is handed out as
the live type so that what a caller writes through it is counted.

## FormAlignedEvent

The form's content has been laid out and nothing has asked for another pass. `sender()` is the
form. Raised at the end of the pass, so every rect in the form may be read from a handler; a
message loop may not be started from one.

## FormPaintedEvent

The form has painted: the rect it was asked to paint, and when the paint started and finished.
`sender()` is the form.

RAISED ON THE APPLICATION'S DISPATCHER - `AppContext::events()` - and not on the form's own.
Whatever watches paints follows them from window to window, and a connection to a form must not
outlive the form, while the application outlives every form. Raised only while something is
connected there: a form nobody watches reads no clock.

## FormControlBase

The root control of a form, and the other half of the FormBase pair. FormBase holds its root as
one of these and asks it for `Control::focusDelegate`, which is protected: a friend of this class
reaches an inherited protected member through an object of this class, and Control's own
friendship with FormBase does not.

A window looks the way its root's properties say: the metrics block, the colour set and the
WindowShadow the root states. The window's role is read for none of them, and a root that leaves
one out wears none of it.

ITS DESTRUCTOR STOPS THE ROOT'S ANIMATIONS while the root can still name its form. Control's own
destructor stops them too, but by the time it runs getForm is Control's, which walks the parents
- and a root has none - so an animation the root left running would tick on into freed memory.
Here getForm still answers m_form, and the stop reaches the controller.

## Control

Provides core control functionality with a minimal memory footprint.

Do not create controls by calling constructors directly. Use the factory `add-` or `create-`
methods provided by the parent control or the form.

Its event connectors add no per-instance storage - each one is a member function over the
EventDispatcher that EventComponent already contributes to every Control - so a control that
connects nothing pays nothing for them.

## Control::animate

RUNS THIS CONTROL'S ANIMATION THROUGH THE APPLICATION'S CONTROLLER, which a control reaches
through its form - `animator()` answers it, or null while the control stands in no form, there
being no application to reach then. A control in no form has nothing to run an animation on
and takes the end value outright: nobody sees it, and the end value is what it is drawn from
once it is seen. That is what `setExpanded` on a header built expanded, or a grid's cell
selected before the grid is shown, come to. `stopAnimation` and `stopAnimations` are the same
route for the stops, and stop nothing where there is no controller to have started anything.
`GridDescriptor::animateCell` gives the same answer for a cell, whose key is the cell rather
than a control.

A CONTROL GOING DOWN STOPS WHAT IT WAS RUNNING, and where it does so depends on whether it can
still find its form. One destroyed in place - deleteControl erases it attached - stops from its
own destructor. The children of a dying container go down detached, since releaseChildren
detaches them before they are destroyed, so releaseChildren stops the whole subtree's animations
first, while the container can still reach the controller; a container detached by then has
nothing left running, its own owner having done the same. A form's root has no parent at all,
and stops its own from FormControlBase's destructor, where getForm still names the form.

## LayoutPass

The state one layout pass keeps. The form owns it, and every control reaches it through the
`AlignEvent` it is measured and laid out with.

It holds one question: whether everything laid out so far stands at a size this pass measured it
against. A control that finds it does not - a wrapping panel broken at a width the measure never
saw, a scrolled body handed a viewport the measure ran ahead of - says so through
`AlignEvent::invalidatePass`, and the form walks the tree again before the frame goes out.

ONLY AN ALIGN EVENT CAN SAY IT. `invalidate` is private and `AlignEvent` is the friend, so the
request is unspellable outside a pass. `FormBase::invalidateAlign` is the route from outside one,
and it is dropped where it is raised inside a pass - `updateAlign` marks the form aligned before
the walk begins. Two calls that look alike and differ by where they are made from is a
distinction every caller has to be told about; one channel that exists only where it works tells
them itself.

READING IT IS NOT SKIPPING WORK. `valid()` is there for a control with something to decide on it.
A pass that has been put in question still has to produce a layout the next one can be measured
against: the remembered wrap width, the viewport a box states for its body, the range a scroll bar
carries the view across are all written by the align. A control that returned early on the flag
would leave the pass after it reading numbers nobody wrote, and the walk order would decide which
ones.

## invalidatePass

Says the control being laid out has been given a box the measure did not answer for, so the tree
is walked again once this walk ends.

IT ASKS, IT DOES NOT LAY OUT. The walk is in progress and the sizes around it are half of one
answer and half of another, so nothing is recomputed on the spot.

WHAT IT IS ASKED ON HAS TO CONVERGE. The pass that answers it must hand back the same number, or
every pass asks for another and the program lays itself out until it is killed. A width carried
over in design units converges in one step, because the pass measured against it produces that
width again. A height does not: `Control::calculate` ceils the content it measured and the align
pass does not, so a fraction of a unit separates them for ever.

FormBase::wnd_beforePaint IS WHAT ANSWERS IT, and it answers a bounded number of times - see
`k_maxAlignPasses`. Past that the frame goes out on what the last pass produced and the next frame
carries on, so a request that cannot converge costs frames rather than the application.

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

## GetActionStateEvent

An action asks its two questions with `GetActionStateEvent` and `ActionClickEvent`, of each
control from the focused one upwards. A control answers them to volunteer as that action's
subject.

## Action

An action given to a control as a property takes that control as a presenter, in the role
`PresenterRole` names. Connecting only inserts into the dispatcher this control has already
contributed, so the derived part being unbuilt does not matter, and nothing asks the control
what it is until its first state query.

## Shortcut

A key and the modifiers held with it. An empty shortcut is one an action does not have:
no key matches it, and it contributes nothing to a tooltip.

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

## Tooltip

Tooltip is an internal class used by the framework - never use it directly

ONE PER FORM, BUILT WITH IT. The window a hint shows in stands over the window whose control
the hint is about, and it is OWNED by that window - which is what keeps it above that window
without being told anything about the rest of the screen. A form is the one thing that knows
which window that is, so a form is what a tooltip is a member of.

AT MOST ONE IS IN PLAY. Which form's tooltip that is follows the pointer, and Input keeps a
single hovered control. The calls that are about whatever is on screen rather than about a
form - the user pressed a key, a control is being destroyed - are static and reach it
through s_current.

A HINT BREAKS ITS LINES AT k_lineWidth, 480 design units, and its window is sized to the widest
line it came to - a short hint stays as short as its words. Bounded by the screen alone, a long
sentence ran out as one line from the pointer to the edge. The one exception is a hint standing
over text: it repeats the lines the control broke, at the width the control broke them.

## Score

How one candidate is ranked against another when a key has to choose between them. A
candidate on the lane the move keeps is taken over one merely ahead of it, however much
nearer the second one is.

## Input

The input state the whole framework reads: which control holds the focus, which one the
pointer is over and in which of its zones, whether a mouse button is down, and which device
the user last acted with. There is one of each per application rather than per form - the
focus leaves a control when it arrives somewhere else, and where that is may be another
window - which is why they are static here instead of being members of FormBase.

A control asks about ITSELF through Control::isFocused(), isHovered() and isPressed()
rather than comparing pointers against these. A container answers for the item it holds,
and only those helpers know it.

## ContextMessage

Something said about a control without stopping the user: why the value they typed
was refused, that the file they asked for was written. It is shown in the tooltip window,
under the control it is about.

A MESSAGE IS RAISED BY SOMETHING THE USER DID. That is what tells it from a tooltip,
which is about whatever the pointer happens to be on. It is placed under its control for
the same reason: it answers an action, and the hand that took that action left the pointer
wherever it happened to be.

IT LIVES EXACTLY AS LONG AS THE WINDOW SHOWING IT. The pointer moving to another
control takes it down, and so does anything the user does - the same two things that take
a tooltip down - and it is forgotten as it goes, so there is nothing left to point at it
for again. A message raised while the pointer rests on the control it is about therefore
stands for as long as the pointer rests there, and needs no time of its own.

ONE AT A TIME. There is one tooltip window, so a second message takes the place of
the first rather than queueing behind it.

WHAT A CONTROL HAS TO SAY EVERY TIME IT IS ASKED belongs in its getTooltip, not
here. A message is what is said once, at the moment it becomes true. A control that must
go on saying it answers for itself as well - see EditBox and the value it has refused.

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

THE ONE COMMAND EVERY APPLICATION PRESENTS THE SAME WAY: an AppButton is a presenter of this
action from its constructor, and ApplicationBase::initialize connects its click to the
application menu through connectAppMenu. It is declared here rather than in StdActions because
it carries no icon - the button paints the application's own mark - and because a control in
Controls has to reach it, which is below StdActions. It is built on first use, so nothing depends
on the order the statics of a translation unit are initialized in.

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

THE NAME A FORM'S PLACEMENT IS KEPT UNDER in the application's config - the name of a section
the application declared in its Forms schema, `MainForm` for the one every application has.
Set before show(): the first placement is what hands the stored placement to the window, and
on Wayland that is also the last moment a toplevel can be named to its session. A form with no
name stores nothing and restores nothing. The placement is written on every hide - a close hides
first - so a system close, Alt+F4 or the compositor's, stores it too: the platform routes those
through IForm::wnd_closeRequested, which closes the form the way its own close button does.
