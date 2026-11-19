# Context

The author's side of the footnote of the same name.

## BackendSwitchEvent

**A form does not swap here.** It marks itself, invalidates whole and takes the new backend at the
top of its next frame - see FormBase::stateBackend, the one place a canvas may change what it
draws through. So a switch asked for from inside a click lands between frames however deep in
the tree that click was handled, and a window nobody can see keeps the mark until the paint its
next showing asks for.

## ScaleSwitchEvent

**A form with nobody under it is what takes it.** It writes the percent into its own scaler, and the
scaler's own change re-frames the window and lays it out again. Every other form is drawn at the
scale of the form it stands on and follows through the scaler they share, so it takes nothing
here - see FormBase::scaleSwitched.

**Following again is a placement.** A popup's window is as big as what was in it when the window was
placed, so a form handed a scale it has to be measured at again is placed rather than aligned -
an alignment lays the content out inside the window that is already there and cuts it to a size
taken at the scale being left. Nothing else would ask: a popup is placed again when the control
it stands on moves, and the form underneath is standing still by then.

## DrawIconEvent

Lives here rather than beside PaintEvent because an icon is painted from two places that
cannot see each other: a control's paint, in Foundation, and a text run, in TextEngine.
Foundation imports TextEngine, so TextEngine can never name anything Foundation owns - and
FormContext is the subsystem both of them already share.

## Platform::pickFolder

Windows answers with the common item dialog in its folder mode, on the OLE apartment the window
opened. Wayland asks the XDG portal's FileChooser - OpenFile with `directory` set - through
Linux::FolderRequest, parented by the handle Wayland::ExportedToplevel takes for the form's
toplevel, and runs the display's loop until the Response arrives, with the bus socket among what
the loop sleeps on. The loop is the application's own, so the form paints and takes input
meanwhile: what keeps a second click from asking again is the desktop's modality, where it has
any.

## AppContext animator

The destructor stops the crossing first: its callbacks reach back into this context.

## IPlatformWindow::showWindowMenu

Win32 tracks the window's own system menu at the point itself, the item states set for the window's
state - DefWindowProc would raise it only over a caption it measures, and a window that is all
client area has none by that measure; Wayland sends xdg_toplevel.show_window_menu with the point in
surface coordinates.

## IPlatformWindow setFullscreen

2026-11-11, asked for as "MillScene fullscreen mode". The framework had maximize, restore and
minimize; Wayland read the fullscreen state a compositor set and nothing asked for it.

Spelled as setAlwaysOnTop and isAlwaysOnTop are, and declared after them. `fullscreen()` beside
`maximize()` was not taken: `restore()` would then have two meanings, and unset_fullscreen and
unset_maximized are two requests.

**The form is told by a call of its own**, IForm::wnd_fullscreenChanged, and not through
wnd_resize. A window maximized on an output with no panel can go fullscreen at the size and the
frame it already has, and wnd_resize returns at once on a resize that moved nothing.

Oj's picks: the config keeps no fullscreen state ("4 a"), so storeFormPlacement stores the
placement fullscreen was entered from on both platforms.

## IPlatformWindow setScreenHeldOn

2026-11-11. **A switch and no getter**: FormBase keeps what was asked, and nothing else reads it.
Named for what is held, the screen on. The screensaver, the dimming and the display turning off
hang on one idle state on Wayland and cannot be held apart there.

What stands behind each platform's half is in the Platform note, under Window setScreenHeldOn.

## AppContext::connectUpdateCheck

The call is made while the context is built, before the config is read; the stored answer arrives
with the load - see UpdateCheck::keepIn.

## UpdateCheck

The question is one GET of api.github.com/repos/<repository>/git/matching-refs/tags/<tagPrefix>,
which lists every tag that starts with the prefix - one application's tags rather than the
repository's whole release list, about four hundred bytes each.

## UpdateCheck::keepIn

**The section's own change is what a stored answer arrives on.** The config is read after the
context is built, and a document load is one transaction, so the section is told once, with both
values in. keepIn also reads the section at once, so a section that was loaded before it was
handed over is read as well.

An answer is written the same way: both values in one transaction, so the handler never reads a
new time beside the previous answer's version. The handler and the answer's own path reach the
same state, and the second one moves nothing - see takeAnswer.

## IForm wnd_userFrameStarted

**A popup is moved and sized by the client on Wayland, 2026-11-18.** `xdg_popup` has no move or
size request, so `Wayland.FormWindow` takes a title drag or an edge press itself when the form lets
the user frame the popup - its size range is more than one size - and hands the gesture to the
window instead of the compositor. It calls `wnd_userFrameStarted` at the threshold or the edge
press and `wnd_userFrameEnded` on the release.

The popup's geometry rectangle is the one the last `xdg_popup.configure` named: its x and y are
relative to the parent's window geometry, and its size is the geometry's. A positioner is read once,
so a move sends `xdg_popup.reposition` with a one-pixel anchor at the parent's origin, gravity
`BOTTOM_RIGHT` and no constraint adjustment, and the popup's place as the offset. **The anchor is
inside the parent and the offset is not**: a positioner refuses an anchor rectangle that reaches
outside the parent's window geometry, and a moved popup has to be able to. A resize changes the same
rectangle, sets `m_geometrySize` through `surfaceSizeFor`, and calls `applySize`, which resizes the
buffer and tells the form; the positioner states the new size and place together. The popup's shadow
margins stand between the pointer, which arrives in the surface, and the geometry; the parent's
margins stand between the placement the form gave and the positioner's coordinates.

The release is the client's, unlike a toplevel's move or size, where the compositor holds the
pointer and the form is told the gesture is over as it is handed over. A compositor below
`xdg_wm_base` 3 has no reposition request, so `canUserFramePopup` refuses and the press reaches
nothing; the popup stays as placed.

**A press on a Wayland popup title took the popup down, 2026-11-18.** The hold that keeps the owner
from closing what stands on it was taken in `wnd_userFrameStarted`, which the client calls only once
the press has traveled. The focus change the press itself causes reached the owner first, so a
message box raised from the app menu's Settings page vanished on the press that meant to drag it.
`wnd_framePress` takes the hold on the press and `wnd_frameRelease` lets it go when the press ends
without a move; `wnd_userFrameStarted` still takes it for a real gesture. Tested on fedya: the Setup
settings storage box survives a title press.

**The resize ran away, 2026-11-18.** The first client-side resize measured the pointer against the
rectangle it had just asked for, while the compositor was still applying the previous request. Every
event added the lag back in: the left edge and the width doubled until a 16,000-pixel buffer was
asked for, and the app crashed. The pointer is now measured against the last configure - where the
popup actually is - and the held edge is set absolutely to it, the opposite edge staying where the
press put it. The released resize grew a box to 16k; the cap is a safety net, the absolute edge is
the fix.
