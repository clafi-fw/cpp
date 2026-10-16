# Control Foundation

The author's side of the footnote of the same name.

## ClickEvent

ClickEvent reaches the target alone and NestedClickEvent walks up from it. One bubbling ClickEvent
served both before, and every handler on a container had to tell its own clicks from its
children's by `event.control`.

**The target skips ActiveContainer** because isClickTarget defaults to respondsToPointer: a press
on a label inside a plain panel inside a stack is a click on the label, not on the stack.

**`clickedControl` is never rewritten.** RowContainer and ButtonBase used to set `event.control` to
themselves on the way up, so the stack above would pick the row or the button. The stack now
finds its item with itemAt from `clickedControl`, and the indicator is no click target at all.
The one behaviour this moved: a mouse click on a MouseOnly control inside a stack item picks that
item, where it used to read as a click on nothing.

**DoubleClickEvent and TripleClickEvent split the same way**, through Control::raiseClick, which
holds the one walk all three use. Two things moved with them. The indicator stopped its own double
click so a tile would not open from its mark; being no target, it never sees one, and
ButtonBase::doubleClick spends it instead. ExpanderHeader, None in three of its looks then, claimed
isClickTarget so that a double click anywhere on its strip toggled it - without that, a target
above it would take the double click and the walk would start past the header. Since 2026-10-16
the header is Focusable in every look, which makes it a target with no override.

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
