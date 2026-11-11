# Platform

The author's side of the footnote of the same name.

## ShadowWindow

The DIB the window is presented from and the canvas the strips are painted on are sized in steps and
never shrink, so a resize allocates nothing until the window outgrows them; a resize costs painting
four strips and copying them into the DIB, and the hole stays clear because only the strips are ever
written and the last fill's strips are cleared by the next.

## FadeTextRenderer

**The lines are placed by a table stated before the draw.** DirectWrite hands over every glyph
run of a layout first and its inline objects after them, so the order the runs arrive in
says nothing about where along a line they stand. DWriteLayout::draw reads the lines off the
layout and states a LineMove for each: the collapse line moves by the collapse offset and no
further left than the box, each line below it is lifted onto the collapse baseline and
starts where the one before it ends, trailing whitespace included, and a line that would
start past the right edge is left out. A run belongs to the line whose baseline lies nearest
its own, which holds while pixel snapping rounds the baseline it arrives on. HarfBuzzLayout
walks its lines in order and places them by the same rule.

## HarfBuzzLayout::SpareBuffers

Every buffer a layout grows - the glyphs, the items and runs, the lines, the character cells,
the inline objects and the compositor's placed glyphs - goes from a destroyed layout to the next
one createNativeTextLayout makes. A paragraph built again after another was given back, or a
text shaped again, grows nothing it has grown once. TextLayout knows nothing of it: a layout
takes spares in its constructor and leaves its own in its destructor, emptied.

At most k_maxSpareBuffers sets are kept, and a layout whose buffers grew past
k_maxSpareCharacters gives them to the allocator, so the spares never hold what one long
paragraph once needed.

**The spares can go before the layouts.** The text engine's cache is a static, and the layouts it
holds are destroyed at exit in no set order against this one. The spares' destructor closes
them, and a layout destroyed after that frees its buffers as it would with no spares at all.

Windows has no twin: an IDWriteTextLayout takes its text at creation, so a spare DWriteLayout
would still build a new one for every paragraph.

## FontFace

FreeType's own scaled metrics are rounded to 1/64 pixel and to the grid, and a layout working in
floats does not want them.

## DirWatchManager

A DirWatch reaches it through DirWatchManager::standing, a pointer the manager sets as it is built
and clears as it goes, because a DirWatch sits below every context; the platform stands before
anything that owns one and after it, so a watch finding none is a broken order, and unreachable.

## TimerManager (Linux)

The two hooks System.Timer declares are defined in Linux.Timer.cpp and reach it through
TimerManager::standing, the same pointer and the same rule as the Windows TimerManager: none
standing is a broken order, and unreachable.

**Fired timers are taken one at a time** (2026-11-11, found by the MillScene showcase's theme list
under sway). dispatchPending took every fired timer off the shared list before firing the first.
A menu's hover timer opens a list beside a line by running the list's loop inside its handler, so
the timers taken with it - MillScene's repaint timer among them - waited for that loop to end: the
loop slept in poll with the eventfd drained, and the window stood still until the list closed.
- **Measured.** Opened by a rest of the pointer, the list froze the window in 2 of 6 tries with
  the picker in it and 2 of 6 with three plain lines; opened by a press, 0 of 8. With the fix, 0 of
  12 by a rest of the pointer. A frozen process sat in poll_schedule_timeout at 0% CPU.
- **Now** takeFiredTimer takes the first and writes the eventfd while others are left, so the
  inner loop wakes for them. It also leaves cancel() able to reach a timer an earlier handler of
  the same batch destroyed; taken whole, that timer would have been fired after it was gone (read
  off the code, not seen).
- Win32 is not changed: each timer arrives there as its own WM_TIMER, which a loop run inside a
  handler dispatches like any other message. Not run.

## PollSource

WaylandPlatform declares one per manager after the display and the manager it polls for, so the
descriptor is in the loop exactly while both are up, and the loop names none of what it polls: the
connection first, then the registered sources in the order they were added, then the appearance bus.

## CompositionPresenter

**A swap chain and not a composition surface**: a present waits its turn at the vertical blank
and every frame presented is a frame shown, and it is what Direct2D draws into.

**The buffers rotate**, so the back buffer handed out holds the frame before the last one, and
what the last frame changed is stale in it. That rect is kept and copied again with the new
frame's own, which is the whole of what two buffers need.

## Direct2DBackend drawSoftEdge

**A soft edge is a blur on the GPU** (2026-11-08). OuterGlow and Shadow were built of strokes, one
ring per pixel of reach, each composited over the last at the opacity that left the falloff's share
standing; a Shadow kept its rings outside the silhouette with a layer whose mask was a rectangle
round it less the silhouette, and filled the silhouette solid after.

The rings took a plain stroke's miter joins until the Satellite's soft clouds - a Shadow layer round
each lump, the reach 20 units - showed on Direct2D as blocks with straight sides wherever a lump
thinned toward the limb. A ring's miter at a sharp corner stood past the reach, up to ten times it,
and the layer's rectangle cut it along its sides. Replayed in cairo from the outlines the scene
drew, miter joins made the same blocks and round joins the CPU backend's soft edge.

With round joins, the raised clouds and the atmosphere's three glows, Oj's video showed the
Satellite at 4 frames a second on Direct2D against 41 on the CPU backend, 125 ms of CPU a frame:
some 50 lumps of about 40 rings and 160 rings of a frame-wide circle, every one tessellated on the
CPU. The clouds near the terminator and the atmosphere also came out with bright rims and separate
rings, since the ring opacities were sized for an opaque brush and a translucent gradient compounded
past its share. Oj picked a blur on the GPU over the CPU's ramp drawn and uploaded, which would
match it exactly at what the CPU backend pays, and over fewer rings.

drawSoftEdge records the shape into a command list - the fill for a Shadow, and a stroke round it -
and draws a Gaussian blur of that list. The device context's target is switched to the list and back
with the frame's clips and layers pushed, as Firefox's DrawTargetD2D1 does. A kept image's target is
reached through ID2D1DeviceContext; one that does not answer for it draws no soft edge. Written and
compiled against MinGW's headers only, so the speed and the look on Direct2D are Oj's to measure.

## TimerManager

The two hooks System.Timer declares - PlatformTimer::start and stop - are defined in its module and
reach it through TimerManager::standing, a pointer the manager sets as it is built and clears as it
goes, because a UiTimer sits below every context: the animation controller's, a directory watcher's,
a control's. The platform stands before anything that owns a UiTimer and after it - see Application -
so a hook finding none standing is a broken order, and unreachable.

## ShmBuffers

**Two of them**, because a buffer that has been attached belongs to the compositor until it
says otherwise. Painting into the one on screen tears the frame being read. The other is
painted into meanwhile, and they change places.

## Wayland::Window alpha

While the window is off the screen the multiplier waits in the surface's pending state for the
commit that maps it, since a bare commit on an unmapped surface asks for it to be mapped.

Premultiplied colour scales with its own alpha, so all four channels take one factor and the result
is still premultiplied colour - no destination is read, and a rounded corner keeps the coverage it
was drawn with.

**Two channels share each multiply.** The pixel is read as one 32-bit word, blue with red in its
even bytes and green with alpha in its odd ones, and a byte times a byte fits the 16 bits
each channel has there. The division by 255 is (t + (t >> 8)) >> 8 over t = c * o + 128,
which is c * o / 255 rounded to nearest and exact for every pair of bytes. Written this way
the compiler vectorises the loop, and on a frame larger than the cache it runs as fast as a
plain copy of the same rows. A hand-written AVX2 kernel would be x86 only and measured no
faster there.

## INativeEventSink

One method per thing that can happen, rather than one method and a tagged struct. The
events a display server sends are already typed when they arrive - the code that knows a
button was pressed is the code that hears the button - and flattening that into a tag only
to switch on it again further up throws the knowledge away and then rebuilds it. It also
lets the compiler name every site when one of these changes, which a switch with a default
arm does not.

## DisplayManager

The Platform statics alone, having no object in hand, reach it through DisplayManager::standing, a
pointer the display sets as it is built and clears as it goes; a static asked while none stands is a
broken order, and unreachable.

## DWriteMonoFont

A module-private class, so SeeDocs shows no page for it. The face is the system collection's first
match on weight and style, which is the face a layout's own matching finds, and IsMonospacedFont is
the gate. The cell and the line come off the one-space probe layout the tab stops are measured from,
so they are the numbers a layout of the same font comes to; a glyph is compared against the space in
the design units the face states, so no scaled float is compared. A run is drawn by
ID2D1RenderTarget::DrawGlyphRun with natural measuring on the Direct2D backend - the same call
FadeTextRenderer makes for a layout's runs - and through the shared glyph cache and the compositor
on the CPU backend.

## FontKey

Keyed on one glyph rather than a whole run - keying on the run would make every distinct string a
separate entry holding its own texture, "Background" and "Backgrounds" sharing nothing, in a map
with no eviction that grows with everything the application ever drew. A glyph key is bounded by the
font's repertoire at the sizes in use, a few hundred entries for an entire interface, and every
string reuses them.

## FontGlyphs

Sized once, to the face's glyph count, when the face is first seen, and never grown after - the
compositor holds pointers into it for the length of a run, and a vector that grows moves what those
point at. The face is held so the address the key carries stays this face's: a face nothing holds is
released, and DirectWrite gives its address to the next face it creates - after a reshape, the
regular of the same family at the italic's address, wearing the italic's glyphs.

Indexed by glyph index rather than searched: hashing per glyph would put a scattered lookup in the
middle of every run, and a font resolved once makes each glyph after that an array subscript.

## Window::addToSession

add_toplevel carries no timing rule, and a name the session does not know turns restore_toplevel
into exactly that request.

## Window setFullscreen

2026-11-11. The Win32 half is written without a Windows build here; Oj builds and runs it.
Its calls are compiled against the mingw-w64 headers in a stand-in file, which checks the
spelling of the calls and nothing of what they do. The Wayland half compiles under clang 20 and
was run under sway.

**First version, 04:30: shown normal at the monitor's rect.** A maximized window was taken out
of that state on the way in, by SetWindowPlacement with SW_SHOWNORMAL, and the kept placement
handed back on the way out. Two readings stood behind it, neither a measurement: ReactOS's
IntSetWindowPlacement moves a window still maximized with SWP_NOSIZE, and Blender's task T86859
reports a window made fullscreen from maximized stopping short of the taskbar until WS_MAXIMIZE
was cleared.

**What Oj's two recordings showed, 04:45** (3840 x 2160, taskbar on top, read frame by frame at
30 a second):
- Back to maximized went through the restored rect: two frames of the fullscreen picture's corner
  in the restored window, a frame of the restored window laid out, then the system's own
  maximize animation.
- From a normal window, one frame of the windowed picture, title bar and all, in the top left of
  the screen: the window stood at the monitor's rect before its new frame was shown.
- Back to a normal window, one frame of the fullscreen picture's corner in the window, in one of
  two tries.

**Second version: the state is kept and only the rect moves.** It is what Chromium's
fullscreen_handler.cc does - the styles off, one SetWindowPos to the monitor's rect, the styles
and the saved window rect back - and the system has no state change to animate. The Blender
report is about maximizing a window already maximized through SetWindowPlacement, which this
does not do.

**The frame is made before the move**, through FormWindow::stateBounds, the way a placement
makes it: the form is told, laid out and painted for the next rect, the presenter shows that
frame, and SetWindowPos follows. For a maximized rect not yet taken, appliedFrame measures the
overhang on statedBounds, which is the same rect as the window's own at every other time.

**Entering goes to HWND_TOP**, as the recipe of The Old New Thing (2010-04-12) does; Chromium
keeps the z-order and tells the shell through ITaskbarList2::MarkFullscreenWindow instead. If a
taskbar is ever seen over the fullscreen window, that call is the next thing to try.

Left as it is, none of it seen:
- WM_DPICHANGED on a fullscreen window still takes the rect the system suggests.
- A display or work area change does not refit the window, and the rect kept for the way out
  is the one from before it.
- A minimized fullscreen window that was maximized is restored by the system's own rule.

## Window setScreenHeldOn

2026-11-11, asked for as "Preventing the platform's screensaver while MillScene in focus". The
Win32 half is written without a Windows build here, as setFullscreen was: its calls compile
against the mingw-w64 headers, and what they do to a screensaver is not measured. The Wayland
half was run under sway 1.9.

**Win32 holds by a display-required request and answers no SC_SCREENSAVE.** Two readings stand
behind that, and no measurement on Windows:
- Microsoft's page for SetThreadExecutionState says of it: "This function does not stop the
  screen saver from executing."
- SDL's history says the state does stop one on a current Windows. A revision of
  SDL_windowsevents.c copyrighted 1997-2010 already answers SC_SCREENSAVE and SC_MONITORPOWER
  with 0. The later commit "Implement SuspendScreenSaver for Win32" reports the screensaver still
  starting over windowed applications, and adds SetThreadExecutionState with ES_CONTINUOUS and
  ES_DISPLAY_REQUIRED as the fix. Mozilla's bug 1193610 found that state not enough on XP and
  Vista only.

If a screensaver is seen starting over a focused MillScene, answering SC_SCREENSAVE in
FormWindow's WM_SYSCOMMAND is the next thing to try. It is said not to hold where a policy
protects the screensaver with a password - quoted from Microsoft's WM_SYSCOMMAND page on the
nesdev forum and on VLC's list; the page itself was not read here.

**A request of the window's own, not the thread's state.** SetThreadExecutionState is one state
for the whole thread, so a second window letting go would take the first one's hold with it. A
power request is the system's to count, one per handle, and PowerRequestDisplayRequired is the
same ask as ES_DISPLAY_REQUIRED.

**On Wayland the inhibitor is no surface state**, so making and destroying it waits for no
commit. Measured under sway 1.9 with swayidle set to 3 seconds:
- MillScene focused for 8 seconds: no idle, and sway's tree shows inhibit_idle on the view.
- Another client focused beside it, MillScene still shown: the inhibitor is destroyed on the
  configure that takes the focus away, and idle came 3 seconds later.
- MillScene focused again: the inhibitor is made, sway resumes from idle at once, and 7 seconds
  pass with no idle.
- The app menu opened and closed, fullscreen entered and left: the same inhibitor throughout.

KWin, Mutter and sway offer the global and Weston does not, by wayland.app's table on this date.
The protocol lets a compositor drop an inhibitor whose surface is not seen. The form's rule is
the narrower one, so that never shows.

## DisplayManager::awaitSessionId

Waiting is a roundtrip, and a roundtrip dispatches every event that has arrived, at a moment - a
window being hidden - where a configure or a close would be reentered into the middle of the hide.
So the session's proxy is moved to an event queue of its own, waited for there and moved back, and
nothing else is dispatched. One pass is enough: the compositor answers get_session before the sync
that ends the wait.

## DisplayManager::activate

**The surface holding the keyboard asks.** KWin grants a token only to the surface of the window it
holds active, and answers any other surface with a token it will not honour; the keyboard is on
that window. The surface the input landed on can be another one: a menu line is clicked in a
popup, which is never the active window and is unmapped before its command runs. The raise is
granted while the asker is still active, and the window only demands attention otherwise.

The token is waited for on an event queue of its own, as DisplayManager::awaitSessionId waits
for its id, and spent before the call returns, so nothing is left to cancel when a window
closes. One pass is enough: the compositor answers commit before the sync that ends the wait.
Without the global nothing is raised.

## ExportedToplevel

The handle is waited for on an event queue of its own, as DisplayManager::activate waits for its
token: the export is asked from inside a click, and a plain roundtrip would dispatch whatever
else has arrived into the middle of it. The proxy goes back to the default queue afterwards,
since it outlives the wait. Exporting a surface that is no xdg toplevel is a protocol error,
which is why Platform::pickFolder asks Window::isToplevelRole first.

## FolderRequest

**The answer is watched for before it is asked for.** The request path is the caller's unique name
and the `handle_token` it chose, so the match rule for its Response is sent ahead of OpenFile on
the same connection and the bus has it before the portal can answer. A portal older than the
token answers OpenFile with a path of its own, and that one is watched as the reply arrives.

The socket is read after every pass of the loop, not only when poll reports it readable: a bus
that hangs up is reported whatever was asked for, and a source dispatched on POLLIN alone would
wake every wait after it without ever reading the hang-up. The poll source therefore only wakes
the loop.

Measured 2026-10-10 against a stand-in portal on a private dbus-daemon: the options arrive as
sent (`current_folder` as the path's bytes with its null), an escaped URI comes back decoded -
spaces and Cyrillic included - and a cancelled dialog, no portal, a portal answering on a path
of its own and a bus killed mid-wait each settle in one or two passes.

## WindowsManager::settingChanged

WM_SETTINGCHANGE **is a broadcast**, sent to every top-level window - a form, a menu, a tooltip -
and to no message-only window at all, which is why it is read in the manager's window procedure
rather than by a listener window of its own. The mode is read again on `ImmersiveColorSet` and
announced only where it differs from the last one read, so one change of mode raises one
SystemColorModeEvent however many windows were told, and a change of the accent colour, which
names the same area, raises none.

## WebFetch on Windows

Cancelling closes the request handle from the UI thread, which fails the call blocked on it. The
thread registers the request under a mutex, and whichever side closes it first clears it there,
so it is never closed twice. The thread reads the cancel flag after every call and makes no
further call on a closed handle.

## WebFetch on Linux

The few option and info numbers a fetch passes are libcurl's ABI and are written out in the source.

The library is never closed. A transfer aborted while a name resolves leaves libcurl's resolver
thread detached and running libcurl's code until the lookup returns, so unloading the library
would take that code from under the thread.

CURLOPT_NOSIGNAL is set because the fetch is not on the main thread. libcurl initialises itself on
the first curl_easy_init; before 7.84 that is unsafe when two fetches start at the same moment.
