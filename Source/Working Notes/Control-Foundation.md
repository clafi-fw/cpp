# Control Foundation

The author's side of the footnote of the same name.

## ClickEvent

ClickEvent reaches the target alone and NestedClickEvent walks up from it. One bubbling ClickEvent
served both before, and every handler on a container had to tell its own clicks from its
children's by `event.control`.

**The target skips ActiveContainer** because clickTarget walks on isMouseResponsive: a press on a
label inside a plain panel inside a stack is a click on the label, not on the stack.

**`clickedControl` is never rewritten.** RowContainer and ButtonBase used to set `event.control` to
themselves on the way up, so the stack above would pick the row or the button. The stack now
finds its item with itemAt from `clickedControl`. The one behaviour this moved: a mouse click on a
MouseOnly control inside a stack item picks that item, where it used to read as a click on
nothing.

**The indicator is the target of a click on the mark**, being MouseOnly, and
ButtonBase::nestedClick raises the button's own click from it and lets the walk go on with Ctrl
set. Until 2026-10-29 a virtual isClickTarget let the indicator answer false, which made the
button the target; it was the one override left, and went when respondsToPointer became
isMouseResponsive and clickTarget started walking on that.

**DoubleClickEvent and TripleClickEvent split the same way**, through Control::raiseClick, which
holds the one walk all three use. Two things moved with them. A double click on the mark is spent
in ButtonBase::nestedDoubleClick so a tile does not open from its mark. ExpanderHeader, None in
three of its looks then, claimed isClickTarget so that a double click anywhere on its strip
toggled it - without that, a target above it would take the double click and the walk would start
past the header. Since 2026-10-16 the header is Focusable in every look, which makes it a target
with no override.

## RichControl

Its metrics properties - `ControlMetrics`, `MinSize`, `PreferredSize`, `MaxSize`, `Padding`,
`Spacing`, `Border`, `Radius`, `FixedSize` - are forwarded into fields of `m_metrics` by the
`BIND_` lines in the constructor rather than declared. Their defaults belong to `ControlMetrics`
and not to RichControl, which is why a `DECLARE_PROPERTY` here would be the wrong place to state
one.

## FormControlBase

FormBase holds its root as one of these and asks it for `Control::focusDelegate`, which is
protected: a friend of this class reaches an inherited protected member through an object of this
class, and Control's own friendship with FormBase does not.

**Its destructor stops the root's animations** while the root can still name its form. Control's own
destructor stops them too, but by the time it runs getForm is Control's, which walks the parents
- and a root has none - so an animation the root left running would tick on into freed memory.
Here getForm still answers m_form, and the stop reaches the controller.

## Control animate

**A control going down stops what it was running**, and where it does so depends on whether it can
still find its form. One destroyed in place - deleteControl erases it attached - stops from its
own destructor. The children of a dying container go down detached, since releaseChildren
detaches them before they are destroyed, so releaseChildren stops the whole subtree's animations
first, while the container can still reach the controller; a container detached by then has
nothing left running, its own owner having done the same. A form's root has no parent at all,
and stops its own from FormControlBase's destructor, where getForm still names the form.

## LayoutPass

Two calls that look alike and differ by where they are made from is a distinction every caller has
to be told about; one channel that exists only where it works tells them itself.

## invalidatePass

FormBase::wnd_beforePaint **is what answers it**, and it answers a bounded number of times - see
`k_maxAlignPasses`. Past that the frame goes out on what the last pass produced and the next frame
carries on, so a request that cannot converge costs frames rather than the application.

**A deferred scroll waits for a layout that stands.** `scrollIntoViewOnAlign` requests are answered
only by a pass that lays the content into the window and that no control put back in question;
a measuring pass and an invalidated pass leave them for the pass that follows. Both lay a
wrapping row out on one line - the remembered widths are not read while measuring, and the
first real pass is the one that records them - so a scroll answered there is measured against
rows standing higher than they will. SeeDocs' tree opened on its remembered page a few
rows short of it for exactly this (2026-10-08).

## Action

Connecting only inserts into the dispatcher this control has already contributed, so the derived
part being unbuilt does not matter, and nothing asks the control what it is until its first state
query.

## Tooltip

**One per form, built with it.** The window a tooltip shows in stands over the window whose control
the tooltip is about, and it is **owned** by that window - which is what keeps it above that window
without being told anything about the rest of the screen. A form is the one thing that knows
which window that is, so a form is what a tooltip is a member of.

The calls that are about whatever is on screen rather than about a form - the user pressed a key, a
control is being destroyed - are static and reach it through s_current.

Bounded by the screen alone, a long sentence ran out as one line from the pointer to the edge.

**Each controller is heard alone.** The hover moves under a still pointer - a window appears under
it, a hit-test follows a scroll - and a click lands the focus with the pointer's tooltip already
standing for it, so neither is taken for the other. A list going down hands the focus back to the
box it was dropped from, which held it all along: that is no landing, and a tooltip that went down
with the list stays down.

## FormBase::windowFocusedFactor

The platform reports the change through `wnd_focusChanged`: `WM_SETFOCUS` and `WM_KILLFOCUS` on
Win32, the **activated** state of a toplevel configure on Wayland. A Menu or Tooltip window never
has the focus to report - `WS_EX_NOACTIVATE` on Win32, and an `xdg_popup` has no **activated**
state - which is why `windowFocusedFactor` answers a popup's owner's factor instead.

**A paint states its own factor since 2026-11-04**, through
`AdjustPaintEvent::setWindowFocusedFactor`, and `shadowDesign` takes the factor rather than reading
the form's. Both came in for the Themes preview (Oj: "Show two dialogs mockups: focused and not"):
until then every mockup in the preview panel followed the Themes window's own focus, the shadows
under them included.

## FocusNavigator

2026-10-17. The arrows and the page keys were a spatial search: every focusable control within a
screenful of the source scored by the distance between centres, on rects clipped to the
viewport, with a source container shrunk to a point and held headers moved back. Oj called it
prone to bugs and overengineered, and the way back often differed from the way forward - centres
of rects of different sizes score differently each way. Tab used the same search to pick the next
sibling.

The framework has two container families and a grid, and each layout already says who its
neighbours are, so each container answers for its own children - Control navigationNeighbor and
navigationEntry - and the navigator only walks up the parents. Decided with Oj:

- A container with nothing current is entered by the line rule, not at its first or last item:
  the first rule lands on a toolbar's last button when the focus comes up into it from below.
- Tab reads the containers' order and nothing else.
- The grid keeps its own field walk for now. It is scored spatially too, but inside its own
  field; walking columns and rows by address is a separate step.

The line pair is the grid's desiredCellX/Y made general. Without it, Up from a full-width row
back into a toolbar picks the button under the row's centre instead of the one the run left.

What changed for a user: a page press counts from the focused item even after the wheel has
scrolled it away - it used to page from the screen in that case - and a diagonal jump into
another panel is gone.

2026-10-30. enterReading is the one public entry besides formKeyDown: PageControl enters the page it
turns to with it when the focus was inside the page it turns from - see Item-Containers. It is
readingEntry from the top, landed with no line, so a page's first arrow starts a run from the stop.
The alternative was a redirect in Control::setFocus for a control that cannot take the focus; Oj
kept setFocus as it is. Exporting :Navigation from Foundation_Facade is what made the class
reachable outside the module.

2026-10-31. enterReading became enterShown, which follows every container's current item ahead of
the reading order - see Item-Containers. FormBase::focusEntryPoint enters the root through it. It
set the focus on the root's focus delegate, and for a root with none emulated a Tab from the root,
which reached the same first stop. Tab and the arrows read readingEntry as before.
FormBase::currentItem became rootFocusDelegate: a Form<T> derives from both FormBase and its
root, and the root's own currentItem means the item a show enters. Action is its one caller.

2026-11-01. The top layer joined the Tab ring, after the root, so that a notice's answers are
reached from the keyboard - Oj's report was that they show unfocused and should be reachable by
Tab. Until then the walk left the layer out. Decided with Oj: the layer after the root rather than
before it, and the focus a hidden layer control held goes back to the root control it came from
rather than on to the next card. FormControlBase::nestedControlFocusing has the form note that
control while Input still names it (rememberContentFocus); forgetControl drops it, and
controlHidden hands the focus back.

Not handled, named as a risk and not yet seen: Return or Space on an answer hands the focus back
inside the key-down, and the character of that press, queued before the dispatch, then reaches
the control the focus went back to - a space typed into a text box. m_popupTookKey answers the
same shape for a popup.

2026-11-01, later. The form root was described as a focus scope from the first public tree - so
that Home, End and the page keys had something to move within - and createDialog's comment said
the root was offered one, but no code made a root one. Decided with Oj: a root answers for itself,
as the code and selection.html already said, and a menu's frame says FocusScope::Yes so that the
four keys reach its lines. Weighed and not taken: every root as the outermost scope for those keys
(Home on a dialog's OK would jump to its first field), and the menu's list as the scope (Up from
the first line would no longer reach the command strip, and the focused line would read selected,
as a group line does while its list is open). isFormRoot stays: a root that is a scope is still
never entered, left or landed on, and every rule about crossing a container's edge has to say so.

A menu's frame is the ScrollBox and its list the body, so a page measured on the frame found no
overflow. pagedContent measures on the content a box scrolls the item in, where the box is the
scope or stands inside it; a scope scrolled from outside - a page's stack - is measured as before.
In the tree, the two menus are the only scopes with a box scrolling inside them.

## Control setFocus

2026-11-01. Oj, on the manual's setFocus paragraph: "Either API is bad either the manual. I can't
imagine anyone wanting to use setFocus with that vague description. And it's public and easy to
call." It was the API. setFocus was the mouse press's own path: FocusEvent took its modifiers from
Platform::keyModifiers() at the call, and StackBase, RowBase and ButtonBase read
Input::mouse().active(). So the same row->setFocus() collapsed a multi-selection after a key,
left it after a mouse click on a selected row, and selected a range with Shift held.

Decided with Oj: split the call code makes from the path input takes, and two calls for code -
setFocus throws (noReach) where the control cannot take the focus, trySetFocus answers false.
FocusEvent::byMouse replaces every read of the device in a focus walk; code builds its event with
no modifiers and byMouse false, so it lands as a key with no modifier would. The input paths are
protected statics, setFocusByPress and setFocusByKey, so the grid's and the tree's key handling
outside Foundation reach them.

I had proposed that code leave the selection alone. StackBase::setCurrentItem was already a pick
with no modifiers from code - "all from code ever meant" - and setFocus follows it, so
makeCurrentItem and setFocus agree.

The popup's return of the focus to its target stays unchecked (setFocusUnchecked): a MouseOnly
button that opened a menu is handed the focus back. Open: whether it should go where the focus was
before the popup took it.

2026-11-03, answered. Oj, once ToolButton answered MouseFocus::Keep: "the app button still grabs
focus" - the backstage closing handed the focus to the AppButton, its target. The owner form keeps
m_focusBeforePopup, the focus holder taken in updateVisibility after the previous popup is closed
and before the new one moves anything, and the way down hands the focus to it with trySetFocus;
the target, unchecked as before, stands in where it cannot take it or where no control of the owner
held it. forgetControl clears it, and dropActivePopup clears it with the target, so a popup going
down with its target still hands the focus to nobody.

Then Oj: "if the button is MouseFocus::Keep It shouldn't take focus away even temporarily -
currently focused item should remain in focus chain even under the active menu dropped by another
button". Input::focusPathParent goes on at the opener's focusBeforePopup's focusDelegate, and at
the popup target's only where none was held. Taken for every opener, not only a Keep one: a
control that took the focus on the press is the holder anyway, and a control hosted in a grid row
(MouseFocus::Parent) now leaves the path to the row, its list up, where it used to read focused
for as long as the list was. updateVisibility clears focusBeforePopup together with the
registration, after the hand-back, so the control it names stays on the path through the
hand-back and is told nothing.

2026-11-01, later. Oj, on the manual's "unless the focus is already inside it - an owner may focus a
dialog's default button before running it": "I don't understand, how focus can be in a window that
is yet invisible". standsShown left the window out on purpose, so setFocus on a control of a dialog
built and not yet run moved Input::focusHolder into it, and FormBase::updateVisibility skipped
entering a window whose form the holder already stood in. MessageBoxBase::addAnswer, PromptDialog,
the Themes App's write-to-source question, SeeDocs' New Project dialog, both crumb lists and the
palette's drop-down leaned on it.

Decided with Oj (his pick of the two I offered): the focus lands only in a window that is up -
standsShown asks form().visible() - and every window but a tooltip is entered as it comes up. Where
a window opens is its root's current item, named with makeCurrentItem. For that call to reach a
root with no focus scope above the control, ContainerBase::takeCurrentItem answers yes on a root and
records the item directly; StackBase hands a stack that is not a scope on to it. I had said
makeCurrentItem without checking that only scopes answered it.

Not taken: keeping the pre-show focus and documenting it. A dialog built and never shown left the
focus in it until it died, and then nowhere.

## Control isFocused

2026-11-01. Oj, on the manual's "A popup takes the focus and leaves the look ... -
Control::focusAnchor": "focusAnchor is a redundant entity, isFocused should answer that "reads like"
and popup stack be on a focus path". focusAnchor answered, per form, the opener's delegate while
the form held an active popup; visualState read it, and isFocused answered the delegate's own chain
only - so the opener read focused and was told it had lost the focus, and WithEditMode and
visualState each added isDroppedDown to make up for it.

Decided with Oj: the focus path runs from the delegate up, and at the root of a popup its opener
holds as active popup, on at the popup target's focusDelegate. isFocused answers whether a control
is on it; focusAnchor went. Input::focusMoved diffs the two paths, so opening and closing a popup
tell the popup's own controls only; FormBase::updateVisibility hands the focus back before it
unregisters the popup for that reason. A focus scope's fallback ring is read off the step the path
takes into it. A dropped-down control no longer reads focused by being dropped down - only selected.

Asked and decided: keys stay where they were. A key the popup leaves stops at its root - Oj picked
that over letting keys walk the path, which would hand a menu's unused arrow to the text box under
it.

Left as it was: a page control switching pages from a menu command now reads itself focused and
enters the new page while the menu is up, and the menu's unchecked hand-back then aims at the
control it was opened on - hidden with the old page. The same end as before, by another route.

## InputController

The fades run on the application's animator, reached through the AppContext the form reporting
the input hands in, so Input owns nothing that has to be released. Each fade is keyed on no
control, so its slot is what tells it from the other one.

The first move a form hears since the pointer came in only hit-tests and turns nothing on: Win32
sends a window that appears under a still pointer a move, and a Wayland enter arrives as one.

## AppActions

The framework's own commands are declared above Core, in ClaFi.StdActions, because an action
carries an icon and Icons sits above Core.

## appMenuAction

It is declared here rather than in StdActions because it carries no icon - the button paints the
application's own mark - and because a control in Controls has to reach it, which is below
StdActions. It is built on first use, so nothing depends on the order the statics of a translation
unit are initialized in.

## Form

Bases initialize in declaration order, so FormBase is complete - window, canvas, context - before
CreateParams is formed from it. Nothing about the window moves into Control, so the same class stays
usable as a nested child.

## FormBase paintPass

**An ordinary pass clears only outside the root's padded rectangle** (2026-11-08). A window that
carries alpha - every window on Windows - used to start each paint from nothing over its whole dirty
rect. On the CPU backend that was a clear of the MillScene's scene panel every frame, under a panel
and a scene that covered it again: 5.3% of Oj's frame in a Visual Studio profile at 3436 x 2051.
The children cover the root's rectangle inside its padding, the rule a frame without alpha already
rests on, so FormBase states that rectangle through IBackend::setCoveredBase and the CPU backend
leaves it as it stands. The padding ring, the margins under the shadow and each corner square still
start from nothing. Direct2D clears its whole rect as before, on the GPU.

The rule now carries more: a translucent surface inside an alpha window, with nothing opaque under
it, would build up over repaints where it used to be laid over a clear base each time.

## Control::contentAroundText

2026-10-12. The align pass took a wrapping text's second measurement as the whole content. A
ButtonBase draws its caption in the content box less the mark and its gap, so a radio button laid
out at its measured width - `HorizontalAlign::Left` on SeeDocs' project options page - had its
caption cut by exactly that, and it faded as trimmed. `Fill` hid it everywhere else: `align` takes
the larger of the granted and the measured width on a fill axis. The screenshots put the cut at
53-55 px at 2.5x on four captions of different lengths, against a 38 px mark and a 12 px gap.

`ButtonBase::calculateContent` was the one place the mark and the icon were added.
`SplitButtonBase::alignContent` guarded its part against the same loss with the larger of the
answer and the offered box; the guard stands, since `contentAroundText` leaves the part out.

## Edge reach

**A pointer pushed against a maximized window's edge meets what stands k_edgeReach in** - eight
design pixels, and a fullscreen window's edge is met the same way since 2026-11-11.
FormBase::searchPoint moves a point that close to the edge to that depth before
the search, so a press against the top lands on a tab on the title and one in the corner on the
close button, rather than on the root's border ring or the band above the tabs.

The geometry is what the screen shows on both platforms - Win32 takes the overhang off it, see
WindowFrame in Context - so the pointer reaches the edge row on both, and the search is what
puts it inside the controls. What a maximized window gives up is the band itself: the strip
above a title's tabs answers as the tab, so it no longer drags the window.

A point outside the window is left alone. A drag carried past the edge keeps the coordinates it
has, and the positions handed to a drag are the pointer's own - only what the pointer meets is
moved.

## WithTextLayout

**One text per host, since 2026-10-27.** The layout points at the control's own ControlText for
life, and the stamp is what says it moved. Before that the mixin kept a copy of whatever the gather
answered, with a gather buffer beside it, and every text change was a whole copy - for a TextBox,
the whole document per key press.

**A built answer is absorbed, not copied.** The gather still runs over Control's scratch buffer. An
answer that is not the control's text by address is assigned into it in drawText and measureText,
after a comparison, so an answer saying what the text already says moves nothing. Handing the
gather the control's own text as its buffer was the first idea and does not work: the buffer is a
`Text&`, and a write through it never reaches the ControlText stamp - the layout would keep its old
build on the paint that changed the words. Fixing that means the event tracking writes, exposure
through `operator Text&` included, and a non-const seam at Control's four own-text gathers.

**A composed answer is kept beside the words, since 2026-10-30.** Absorbing it into the text was
the mistake: RichControl::adjustText names the text as the first contribution of the next gather, so
whatever a gather had added - a handler's words, an end icon - was named back in and added again,
one copy per gather, shaped whole each time. The FPS page's footer grew that way for as long as the
page was up, and the frame rate it reported fell as 1/t with the pointer still. The mixin now keeps
`m_composedAnswer`, a ControlText of its own, and points the layout at it while the gather composed
something; a stamp carries its object's id, so the two sources are told apart with no flag read at
sync. The words stay the owner's, which is also what let the Action write a presenter's words into
its text instead of answering every gather with a handler, and what readmits an EndIcon on a host.

**No host composes its answer now.** TooltipLabel re-asked its control per paint and answered
differently by phase - the Calculate answer sized the window and the Paint answer was shown, so
a hue slider's tooltip kept the width of 360 degrees through a drag - until 2026-10-29, when
Tooltip began writing the words into the label's own text, as any host's caller does. The window
is sized to what it shows. ParameterTooltip had carried a second Text and a adjustText override to
replace the answer; setCall writes text() now.

## EditPhase

2026-11-03. Oj: "Every hue adjustment in the slider edit mode with arrows writes a ton Undo
records. Must write only one when accepted". The arrows' steps reported Settled, as every move
outside a pointer hold did, so each was a step of its own. A first answer added EditPhase::Pending
for them beside Held, and accepting the mode raised one OnSettle.

Then Oj asked for the scroll bar's snap-back - a drag that strays far from the bar is called off.
A called-off drag ends where it began, and the settle raises nothing there, yet each of its Held
steps had gone into StateHistory as an open run, which nothing then closed: the next edit would
have joined it under the drag's name. A drag brought back to its start pixel did the same before.
Oj chose to record on release: Held records nothing, the settle writes the one step, and Pending
folded into Held. StateHistory keeps no run of its own; StepHistory keeps runs for the text
history. Taking a run back out instead would have dropped the redo steps at the first move of
every drag, called off or not.

The reach is the one Chromium gives Windows scroll bars - eight thicknesses to the side, three
past the ends - with Windows' 17-pixel bar as the least thickness. Chromium leaves snap-back off on
Linux, as GTK and Qt do; Oj took it on every platform, since ClaFi draws its own controls, and for
sliders as well as scroll bars.

Open: Undo while the arrows hold the position walks back the step before them, and the restore
that lands there ends the hold with the arrows' steps unrecorded.
