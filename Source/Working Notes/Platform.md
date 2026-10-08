# Platform

The author's side of the footnote of the same name.

## ShadowWindow

The DIB the window is presented from and the canvas the strips are painted on are sized in steps and
never shrink, so a resize allocates nothing until the window outgrows them; a resize costs painting
four strips and copying them into the DIB, and the hole stays clear because only the strips are ever
written and the last fill's strips are cleared by the next.

## FadeTextRenderer

THE LINES ARE PLACED BY A TABLE STATED BEFORE THE DRAW. DirectWrite hands over every glyph
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

THE SPARES CAN GO BEFORE THE LAYOUTS. The text engine's cache is a static, and the layouts it
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

## PollSource

WaylandPlatform declares one per manager after the display and the manager it polls for, so the
descriptor is in the loop exactly while both are up, and the loop names none of what it polls: the
connection first, then the registered sources in the order they were added, then the appearance bus.

## CompositionPresenter

A SWAP CHAIN AND NOT A COMPOSITION SURFACE: a present waits its turn at the vertical blank
and every frame presented is a frame shown, and it is what Direct2D draws into.

THE BUFFERS ROTATE, so the back buffer handed out holds the frame before the last one, and
what the last frame changed is stale in it. That rect is kept and copied again with the new
frame's own, which is the whole of what two buffers need.

## TimerManager

The two hooks System.Timer declares - PlatformTimer::start and stop - are defined in its module and
reach it through TimerManager::standing, a pointer the manager sets as it is built and clears as it
goes, because a UiTimer sits below every context: the animation controller's, a directory watcher's,
a control's. The platform stands before anything that owns a UiTimer and after it - see Application -
so a hook finding none standing is a broken order, and unreachable.

## ShmBuffers

TWO OF THEM, because a buffer that has been attached belongs to the compositor until it
says otherwise. Painting into the one on screen tears the frame being read. The other is
painted into meanwhile, and they change places.

## Window opacity

While the window is off the screen the multiplier waits in the surface's pending state for the
commit that maps it, since a bare commit on an unmapped surface asks for it to be mapped.

Premultiplied colour scales with its own alpha, so all four channels take one factor and the result
is still premultiplied colour - no destination is read, and a rounded corner keeps the coverage it
was drawn with.

TWO CHANNELS SHARE EACH MULTIPLY. The pixel is read as one 32-bit word, blue with red in its
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

## DisplayManager::awaitSessionId

Waiting is a roundtrip, and a roundtrip dispatches every event that has arrived, at a moment - a
window being hidden - where a configure or a close would be reentered into the middle of the hide.
So the session's proxy is moved to an event queue of its own, waited for there and moved back, and
nothing else is dispatched. One pass is enough: the compositor answers get_session before the sync
that ends the wait.

## DisplayManager::activate

THE SURFACE HOLDING THE KEYBOARD ASKS. KWin grants a token only to the surface of the window it
holds active, and answers any other surface with a token it will not honour; the keyboard is on
that window. The surface the input landed on can be another one: a menu line is clicked in a
popup, which is never the active window and is unmapped before its command runs. The raise is
granted while the asker is still active, and the window only demands attention otherwise.

The token is waited for on an event queue of its own, as DisplayManager::awaitSessionId waits
for its id, and spent before the call returns, so nothing is left to cancel when a window
closes. One pass is enough: the compositor answers commit before the sync that ends the wait.
Without the global nothing is raised.

## WindowsManager::settingChanged

WM_SETTINGCHANGE IS A BROADCAST, sent to every top-level window - a form, a menu, a hint - and to
no message-only window at all, which is why it is read in the manager's window procedure rather
than by a listener window of its own. The mode is read again on `ImmersiveColorSet` and announced
only where it differs from the last one read, so one change of mode raises one
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
