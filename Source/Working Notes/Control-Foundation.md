# Control Foundation

The author's side of the footnote of the same name.

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

ITS DESTRUCTOR STOPS THE ROOT'S ANIMATIONS while the root can still name its form. Control's own
destructor stops them too, but by the time it runs getForm is Control's, which walks the parents
- and a root has none - so an animation the root left running would tick on into freed memory.
Here getForm still answers m_form, and the stop reaches the controller.

## Control::animate

A CONTROL GOING DOWN STOPS WHAT IT WAS RUNNING, and where it does so depends on whether it can
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

FormBase::wnd_beforePaint IS WHAT ANSWERS IT, and it answers a bounded number of times - see
`k_maxAlignPasses`. Past that the frame goes out on what the last pass produced and the next frame
carries on, so a request that cannot converge costs frames rather than the application.

## Action

Connecting only inserts into the dispatcher this control has already contributed, so the derived
part being unbuilt does not matter, and nothing asks the control what it is until its first state
query.

## Hint

ONE PER FORM, BUILT WITH IT. The window a hint shows in stands over the window whose control
the hint is about, and it is OWNED by that window - which is what keeps it above that window
without being told anything about the rest of the screen. A form is the one thing that knows
which window that is, so a form is what a hint is a member of.

The calls that are about whatever is on screen rather than about a form - the user pressed a key, a
control is being destroyed - are static and reach it through s_current.

Bounded by the screen alone, a long sentence ran out as one line from the pointer to the edge.

## InputController

The fades run on the application's animator, reached through the AppContext the form reporting
the input hands in, so Input owns nothing that has to be released. Each fade is keyed on no
control, so its slot is what tells it from the other one.

## appMenuAction

It is declared here rather than in StdActions because it carries no icon - the button paints the
application's own mark - and because a control in Controls has to reach it, which is below
StdActions. It is built on first use, so nothing depends on the order the statics of a translation
unit are initialized in.

## Form

Bases initialize in declaration order, so FormBase is complete - window, canvas, context - before
CreateParams is formed from it. Nothing about the window moves into Control, so the same class stays
usable as a nested child.
