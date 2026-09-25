# Context

The words that no longer fit above a declaration.

## FrameMargins

THE ROOM A SURFACE KEEPS AROUND THE WINDOW IT SHOWS, in real pixels. The shadow is drawn
in it; the pointer falls through it.

## PlacedWindow

What the placement did. THE POSITION IS NOT IN IT and cannot be: a client is not always
told where its own window was put, and nothing above the platform layer reads it.

## ThemeSwitchEvent

The colours every window is painted from have moved. Carried once for a set taken outright,
and once per frame for the length of a crossing - `kind()` says which.

## ThemeSwitchKind

Which of the two a theme switch is, and so when a window paints it.

A TAKEN set is raised inside `AppContext::takeTheme`: an edit to the set worn, or a set with
nothing to cross. That is inside whatever took it - a page being built, a view state being
restored - so a window invalidates and waits for the scheduled paint. A frame made on the spot
would show that work half done.

A CROSSING FRAME is stated by the animation controller, and a window paints it on the spot.
Every frame of a crossing is one someone is meant to see, and a scheduled paint waits behind the
pointer's own messages - see FormBase::themeSwitched.

## BackendSwitchEvent

The application has changed which backend its windows draw through, raised on
`AppContext::events()`. It carries nothing: every window of an application is on the one kind,
and `AppContext::createBackend` is what answers with it.

A FORM DOES NOT SWAP HERE. It marks itself, invalidates whole and takes the new backend at the
top of its next frame - see FormBase::stateBackend, the one place a canvas may change what it
draws through. So a switch asked for from inside a click lands between frames however deep in
the tree that click was handled, and a window nobody can see keeps the mark until the paint its
next showing asks for.

## ScaleSwitchEvent

The size the application is drawn at has moved, raised on `AppContext::events()`. It carries
nothing: there is one percent for the process, and `AppContext::scalePercent` is what answers
with it.

A FORM WITH NOBODY UNDER IT IS WHAT TAKES IT. It writes the percent into its own scaler, and the
scaler's own change re-frames the window and lays it out again. Every other form is drawn at the
scale of the form it stands on and follows through the scaler they share, so it takes nothing
here - see FormBase::scaleSwitched.

A FORM MAY ASK TO HOLD STILL. FormBase::holdScale puts a form on a scaler of its own carrying the
scale it is drawn at, and nothing above it moves it until FormBase::followScale puts it back under
the form it stands on. A form with nobody under it has nothing to follow and keeps its own either
way.

The backstage holds itself for the length of a drag on the scale slider, because a control that
steers the size of the form it stands in is steering its own geometry - ScaleSlider is where the
whole of that is. The hold covers the gesture and no longer, so the menu takes the size it has
named as soon as the pointer lets go.

FOLLOWING AGAIN IS A PLACEMENT. A popup's window is as big as what was in it when the window was
placed, so a form handed a scale it has to be measured at again is placed rather than aligned -
an alignment lays the content out inside the window that is already there and cuts it to a size
taken at the scale being left. Nothing else would ask: a popup is placed again when the control
it stands on moves, and the form underneath is standing still by then.

## SystemColorModeEvent

The desktop has changed the mode it asks applications to be drawn in, raised on
`Platform::events()`. It carries nothing: it says ask again, and `Platform::systemColorMode` is
what answers. One change raises one event, however many of the application's windows the platform
heard it through.

## KeyDownEvent::isRepeat

The key was already down and the system is repeating it. The key acts on every
repeat - that is what makes a held arrow travel - but a key still down is not a new
reach for the keyboard.

## PaintIconEvent

Lives here rather than beside PaintEvent because an icon is painted from two places that
cannot see each other: a control's paint, in Foundation, and a text run, in TextEngine.
Foundation imports TextEngine, so TextEngine can never name anything Foundation owns - and
FormContext is the subsystem both of them already share.

## KeyDownEvent::stamp

What the display server called this press, for a request the key is about to justify.
Empty on a key nobody pressed - a shortcut the framework raised for itself.

## WindowFrame

THE FRAME A WINDOW WEARS, in real pixels: the margins its shadow needs and the radius of its
corners. A form states the DESIGN from its root's properties - see
IPlatformWindow::setFrame; the platform answers with what it APPLIES - see
IForm::wnd_resize. A maximized, fullscreen or docked window applies neither, a snapped or
tiled one keeps the margins and applies no corners, and a platform that cannot composite a
frame rounds no corners. The border is the root's own and is painted with its content.

## WindowPlacement

WHAT A WINDOW ASKS TO BE PLACED BY. Everything in it is the FORM's own knowledge: what it
stands on, stated in the coordinates of the window it stands in, and what its content
measured. NOTHING IN IT IS A SCREEN COORDINATE - where a window ends up is the platform's
answer, and there are platforms that never tell a client what that answer was.

## Platform

WHAT AN APPLICATION IS BUILT AGAINST. The statics every platform answers - the cursor, the
key names, the data path - are defined per platform in an implementation unit of this module
under Platform/, one platform per build. createWindow is virtual and answered by the
platform built for the display server, Win32Platform or WaylandPlatform, which is what holds
what a window is made from. The clipboard is that platform's member, named to this base at
construction, and reached from a form's context as FormContext::clipboard; what the
clipboard itself holds the platform by is IPlatformServices, a name declared below this
module - see UI-Types.

## Platform::createFormsConfigSchema

THE SHAPE IS THE PLATFORM'S. A config's Forms section holds one section per form named here,
and what stands in it is whatever this platform can put a window back from - which is not the
same thing on every platform, and is nobody else's business. Win32 keeps the window's ordinary
geometry in screen pixels and whether it stood maximized: the client owns the rectangle there.
Wayland keeps the ordinary size in surface pixels and the maximized state per form, and one
value for all of them - the id of the session the compositor remembers the windows under, since
where a window was is the compositor's to keep and the client is never told.

## Platform::restoreFormPlacement

Called once per form, before its first placement, with the whole Forms section: the platform
finds the form's own section by name and reads whatever else it keeps there. It hands the
platform window what it needs and places nothing itself - the first placement does, reading
what was handed over. ON WAYLAND THE ORDER IS THE PROTOCOL'S: a toplevel is named to its session
before the first commit of its surface, and that commit is made by the placement that creates
the toplevel, so the name has to be on the window before the form is placed for the first time.

## Platform::storeFormPlacement

Called while the window is still up - on hide, and a close hides first - which is the only time
its placement can be read. Written into the config and nowhere else: the file is the
application's to save, and it saves only into a folder the user has allowed.

## Platform::addFormToSession

The other half of the rule above, for the run in which the user allows storing. A window made
before that was never named to a session - none was asked for - and it is too late to restore
it, the surface having been committed long since. It is added to one instead: the compositor
keeps the state of every toplevel a session holds, from the moment it is given it, so a window
added while it is still up leaves its place behind for the next launch. Where the platform
tells the window manager nothing - Win32 - there is nothing to do.

## Platform::systemColorMode

The mode the desktop asks applications to be drawn in. Windows answers from the app mode of its
personalisation settings; Wayland answers from the XDG desktop portal's colour scheme, read over
the session bus - see Platform. A desktop stating no preference is answered Light: GNOME's
default look states none, and so does a session without a portal, and both show a light desktop.

THE FIRST ASK WAITS FOR THE ANSWER. On Wayland it connects to the bus and gives the portal up to
a second, so the first window opens in the desktop's mode rather than crossing to it; an answer
later than that arrives as a SystemColorModeEvent.

## Platform::events

Where the platform announces a change of what the desktop asks for - SystemColorModeEvent. One
dispatcher for the process, standing before the first window is made and after the last is gone.

## AppContext::animator

THE APPLICATION'S ANIMATIONS - every control's state fades, every glide, a tooltip's alpha and
the theme crossing - stepped by one UiTimer, owned by the context that owns everything they
run in. A control reaches it through its form - see Control::animate - and the crossing code
here uses the member. The destructor stops the crossing first: its callbacks reach back into
this context.

## AppContext::createBackend

THE ONE PLACE A FORM ASKS FOR A BACKEND, and the answer is the kind the application is on at
that moment - so a window opened after a switch matches the windows that swapped, and no two
windows can disagree.

The CPU backend is the core's own type and is always there. The GPU backend is the application's
to name - see Application - and arrives as a factory; `gpuAvailable` is that factory being there
at all, which on a platform with no GPU backend it never is.

## AppContext::gpuAcceleration

WHERE THE USER'S ANSWER IS KEPT: a `Dom::Value<bool>` under GpuAcceleration, true by default.
Writing it is the whole of a switch - the node's own change reaches the context, which states the
kind and raises BackendSwitchEvent - which is the path a colour mode takes to the theme.

THE NODE IS IN THE SCHEMA ONLY WHERE A GPU BACKEND IS. An application that named none keeps no
answer to a question it cannot ask, so the node is null there and `gpuAvailable` is that same
answer. `usingGpu` is what is worn now, taken from the node while the context is built, so a
stored answer is what the first window stands on rather than something switched to once it is up.

## AppContext::scale

WHERE THE USER'S ANSWER IS KEPT: a `Dom::Value<int>` under Scale, the percent of the design
everything is drawn at, 100 by default. Writing it is the whole of a change - the node's own
change reaches the context, which states the percent and raises ScaleSwitchEvent - which is the
path the backend answer takes to the windows.

THE PERCENT IS BROUGHT INSIDE THE BAND, between `k_minScalePercent` and `k_maxScalePercent`. The
config is text the user is free to edit, and a window drawn at a factor outside the band is one
nothing on it can be reached in. The node keeps whatever it was written with; `scalePercent` is
the answer that was taken, and it is what every window reads.

`scalePercent` is taken from the node while the context is built, so a stored size is what the
first window opens at rather than one it is moved to once it is up - FormBase::initialize is
where a form reads it, so a window opened later matches the windows already up.

## AppContext::configFolderExists

NOTHING IS STORED UNTIL THE USER ALLOWS IT, and the config folder is that answer: an
application creates it when the user agrees, and reads its config back on every later start
because the folder is there. The flag and the folder cannot disagree - the flag is read off the
folder at start and set when the folder is made. A window's placement follows the same rule
twice over. The config is not loaded without the folder, so there is nothing to restore; and on
Wayland no session is asked of the compositor without it, because a session id that may not be
written down would leave the compositor keeping records for nobody.

## AppContext::ensureConfigFolder

THE ONE PLACE A CONFIG FOLDER IS MADE. Making it is the user allowing storing at all - see
configFolderExists - so the question that leads here names the folder, and what appears on
the disk is what was read in the question. An application with nowhere to put one - no
appDataPath - has no folder to offer and makes none.

## AppContext::deleteConfigFolder

The folder and everything under it, because the folder is what the user was asked about: a
permission withdrawn is not withdrawn from one file. The publisher folder above it goes with
it once it is empty - an application that has stopped storing leaves nothing behind - while
one still holding another application's settings, or the `Share` folder the publisher's
applications keep their themes in, is not empty and stays.

THE APPLICATION DATA ROOT IS THE FLOOR. Where a publisher is unnamed the folder above the
config folder IS that root, shared with every other publisher, and its being empty says
nothing about who may still want it. Every step is taken through the error_code overloads and
stops at the first refusal, so a folder held open by something else leaves the rest standing
rather than throwing out of a click.

The config document is untouched and goes on answering in memory; it simply has nowhere to be
written, which is what configFolderExists already says everywhere it is read.

## AppContext::k_mainFormName

THE NAME AN APPLICATION'S MAIN WINDOW IS KEPT UNDER, stated once because two places have to
agree on it: ApplicationBase names it in the Forms schema, and every application hands the
same name to FormBase::setConfigName before its first show. A window whose config name is not
in the schema has nowhere to be written, and the two spellings drifting apart is exactly the
failure that costs nothing to make impossible.

## AppContext::restoreFormPlacement

Given to the window before its first placement, and only while the config folder exists - see
configFolderExists. A form asks for it through its config name - see FormBase::setConfigName -
so the platform's own half is reached with the Forms section and the name, and knows nothing
about how the application decided.

## AppContext::storeFormPlacement

Written into the config whether or not the folder exists: the config reaches the disk only
through an application that saves it, and the folder decides there. So an application that
saves at its first exit - the Themes app does - has the placement in the file from its first
run, and restores it from its second.

The session is the one thing here that is not written but asked for, and it is asked for while
the folder exists and the window is still up - see Platform::addFormToSession. Ticking Keep
settings on this PC therefore needs no restart: the run it was ticked in leaves both halves
behind, the config's own and the compositor's, and the next launch puts the window back.

## IPlatformWindow::showWindowMenu

THE SYSTEM'S OWN MENU FOR THIS WINDOW - Maximize, Minimize, Move, Close and whatever else the
desktop puts there - which a window drawing its own title bar has to ask for, since the system
never sees a title to raise it from. The form asks on a right click over the title zone that no
control claimed; the point is where the menu opens, and the stamp is the press that asked,
which is what a display server weighs the request against. Win32 tracks the window's own system
menu at the point itself, the item states set for the window's state - DefWindowProc would
raise it only over a caption it measures, and a window that is all client area has none by
that measure; Wayland sends xdg_toplevel.show_window_menu with the point in surface
coordinates. What the menu does is the system's: a command chosen from it arrives as a
WM_SYSCOMMAND on Win32 and as configures on Wayland, so a window closed from it is closed
through wnd_closeRequested like any other.

