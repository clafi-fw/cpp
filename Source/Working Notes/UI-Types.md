# UI Types

The author's side of the footnote of the same name.

## IPlatformServices

The clipboard is handed one at construction, because Transfer cannot import Context - Context
imports Transfer for the clipboard, and Transfer names nothing above itself - so the name the
clipboard holds the platform by has to be declared below both. What stands behind the name is each
platform layer's own statement: on Win32 the Windows IMessageWindowFactory, which Win32Platform
derives from and the clipboard's Windows half casts to for its listener window; on Wayland the
IDisplayAccess of the Wayland layer, which WaylandPlatform derives from and the clipboard's Wayland
half casts to for the display. The cast is what keeps the neutral name empty: no platform answers
for a thing it has no notion of.

## Pigment

Purple and Orange joined the four on 2026-11-05. Oj first asked for six references evenly spread,
60 degrees apart with red at 0, under the widest-gap fill. Okhsl crowds red, orange, yellow and
green into about 120 degrees, so that set painted yellow a lime, green a teal and red a pink, and
no turn of the six brought every name within 25 degrees of its middle. A fixed grid of twelve
from 0 put every reference within 5 degrees of its name, but the widest-gap fill still pushed
unclaimed pigments to their window edges - the built-in theme's red to 0 and green to 172.

Oj's rule replaced the claim and the fill: the grid starts at the theme's anchor, and a pigment
takes the grid hue nearest its reference. Every harmony colour is a multiple of 30 degrees from
the anchor, so it stands on the grid, and a pigment takes one wherever it is the nearest point.
The references are the middles of the names - 25, 57, 90, 148, 245 and 305 - and stand more
than a grid step apart, so no two pigments share a hue. The three fills it retired (reference,
widest gap, turned set), the claim windows and the tie margin went with it. A pigment now moves
with the anchor and jumps back a step each time the anchor passes the midpoint between two grid
hues.

The built-in theme's anchor and palette hues, 240 and 60, are HSL's pure blue and pure yellow, set
before the hue channel was Okhsl's. In Okhsl they paint a sky blue and an orange; HSL's blue and
yellow stand at about 264 and 110 there.

## lightGradeOf

Why the light side mirrors the steps rather than reading the dark side's shares as they are: equal
steps in lightness do not read as equal at the two ends of the axis, and a step off a dark surface
reads larger than the same step off a light one.

## UiTimer

Named for WM_TIMER, which the Windows backend ticks on: Windows generates no WM_TIMER while input
messages are still pending, which is what keeps a tick from landing in the middle of user input.

## Interactivity

KeyFocusable came on 2026-11-01 with the notices joining the Tab ring. Focusable answers would have
moved the focus onto the card for the length of a mouse press and back when the card went, and a
press dragged off the button would have left it on the card. Oj chose a value of its own over a
rule in Input::setMouseDown that a press inside the top layer moves no focus: the rule would have
been one place in the code and invisible on the buttons it governs.

The right-click walk in FormBase takes takesFocusOnPress too, so a right click on a KeyFocusable
control does not focus it. That walk goes on to the parent of a control that cannot take the focus
from a press, MouseOnly included, where the left-click walk stops at a MouseOnly control - a
difference older than KeyFocusable.

## NavigationReach, MouseFocus, ClickTarget, FocusScope

2026-11-01. Oj, on the selection manual's Parts section: "What does "isPart" differently from
having Interactivity::MouseOnly?", then, of "A mouse press on a part focuses its owner": "that's an
active container behavior". Then: "I think Interactivity should split into separate members that
answer they own answers: How that control is reachable by keys, what happens with focus on mouse
presses. And there is also HitTest that somehow intersects with that".

Interactivity answered four questions with five values, so each new combination needed a value of
its own - KeyFocusable was one - and Control::isChildPart patched a further answer onto the parent.
Decided with Oj:

- Four enums, one question each: NavigationReach { Yes, No }, MouseFocus { Self, Parent, Keep },
  ClickTarget { Self, Parent }, FocusScope { Yes, No }. The names are Oj's. Keep over None was
  mine: None read as no focus anywhere.
- No presets. Every class states its answers, so a stock focusable control reads
  `NavigationReach::Yes, MouseFocus::Self, ClickTarget::Self`. What was ActiveContainer is
  `FocusScope::Yes, NavigationReach::Yes, MouseFocus::Self` on a plain container, and
  FocusScope::Yes alone where the class answers the other two already.
- FocusScope is the container holding the focus for its current item: focusHolder and
  focusDelegate differ exactly while a scope holds it. StackBase gates focusDelegate, the
  retargeting in nestedControlFocusing and the focusDelegateChanged call on it, so a PageControl
  holds no focus for its page.
- isChildPart went: a part answers MouseFocus::Parent. RowContainer::addControl appends
  `ClickTarget::Self, MouseFocus::Parent, NavigationReach::No, FocusScope::No` after the caller's
  arguments, as it appended MouseOnly. A row group's mark says MouseFocus::Parent where it is made,
  since an ExpanderButton in an expander header answers Keep - the two answers isChildPart gave the
  one class by its parent.
- The press walk moved into Control::setFocusByPress, and the right-click walk in FormBase calls
  it, so a right press stops at Keep as a left one does.

HitTest stays apart: it is read per point, before the pointer is over any control, and the four
answers are per control.

ApplicationBase::createDialog's comment said the root was offered ActiveContainer. The code never
passed it - true since the first release - and the comment now says the root answers for itself.
Open: whether createDialog should offer FocusScope::Yes after all.
