# Context

The author's side of the footnote of the same name.

## BackendSwitchEvent

A FORM DOES NOT SWAP HERE. It marks itself, invalidates whole and takes the new backend at the
top of its next frame - see FormBase::stateBackend, the one place a canvas may change what it
draws through. So a switch asked for from inside a click lands between frames however deep in
the tree that click was handled, and a window nobody can see keeps the mark until the paint its
next showing asks for.

## ScaleSwitchEvent

A FORM WITH NOBODY UNDER IT IS WHAT TAKES IT. It writes the percent into its own scaler, and the
scaler's own change re-frames the window and lays it out again. Every other form is drawn at the
scale of the form it stands on and follows through the scaler they share, so it takes nothing
here - see FormBase::scaleSwitched.

FOLLOWING AGAIN IS A PLACEMENT. A popup's window is as big as what was in it when the window was
placed, so a form handed a scale it has to be measured at again is placed rather than aligned -
an alignment lays the content out inside the window that is already there and cuts it to a size
taken at the scale being left. Nothing else would ask: a popup is placed again when the control
it stands on moves, and the form underneath is standing still by then.

## Platform::pickFolder

Windows answers with the common item dialog in its folder mode, on the OLE apartment the window
opened. Wayland answers nothing yet; the XDG portal's FileChooser is the dialog there, and its
asynchronous Response is still to be waited on.

## AppContext::animator

The destructor stops the crossing first: its callbacks reach back into this context.

## IPlatformWindow::showWindowMenu

Win32 tracks the window's own system menu at the point itself, the item states set for the window's
state - DefWindowProc would raise it only over a caption it measures, and a window that is all
client area has none by that measure; Wayland sends xdg_toplevel.show_window_menu with the point in
surface coordinates.

## AppContext::connectUpdateCheck

The call is made while the context is built, before the config is read; the stored answer arrives
with the load - see UpdateCheck::keepIn.

## UpdateCheck::keepIn

THE SECTION'S OWN CHANGE IS WHAT A STORED ANSWER ARRIVES ON. The config is read after the
context is built, and a document load is one transaction, so the section is told once, with both
values in. keepIn also reads the section at once, so a section that was loaded before it was
handed over is read as well.

An answer is written the same way: both values in one transaction, so the handler never reads a
new time beside the previous answer's version. The handler and the answer's own path reach the
same state, and the second one moves nothing - see takeAnswer.
