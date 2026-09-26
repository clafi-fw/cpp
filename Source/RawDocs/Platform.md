# Platform

The words that no longer fit above a declaration.

## ShadowWindow

THE SHADOW OF A COMPOSED WINDOW, as a window of its own. Owned by the window it shadows, so
it stands above it and follows it through minimize and activation; transparent by style, so
the pointer goes through it to whatever is underneath, another application included - the
one thing a shadow painted in the window's own surface cannot say on Win32, where a pixel
with any alpha at all owns the pointer. Painted by the form's ShadowPainter into the four
strips of margin around the hole where the window itself stands - the same call the form
paints its own ring with, clipped to a strip each time - and presented as a layered window.
The DIB the window is presented from and the canvas the strips are painted on are sized in
steps and never shrink, so a resize allocates nothing until the window outgrows them; a
resize costs painting four strips and copying them into the DIB, and the hole stays clear
because only the strips are ever written and the last fill's strips are cleared by the next.

## IdWriteRunEffect

What one run is drawn with, over and above the format the layout was built from. A drawing
effect is also what DirectWrite breaks runs at, so two ranges that differ only in what they
carry here still arrive as two runs - which is what lets a superscript stand beside a
subscript of the same size.

## Direct2DBackend

Satisfies the GPU backend contract: it renders into a window the platform owns. THE DEVICE
IS THE WINDOW'S. Every paint arrives with the window's CompositionPresenter as its native
context, and everything here - the Direct2D device, its context, the target bitmap over the
presenter's frame texture, the brushes and the kept images - is built on that presenter's
D3D device and built again when the presenter says the device moved. What is drawn lands in
the frame texture; presenting it is the window's.

## Window

ONE WINDOW, AND WHAT A COMPOSITOR WILL AND WILL NOT ANSWER ABOUT IT.

Several members of IPlatformWindow have no answer here at all. They are honest no-ops with
the reason on each rather than omissions, because the contract is shared with a platform
that answers all of them.

## HarfBuzzLayout

One paragraph shaped by HarfBuzz and broken into lines here.

The paragraph is cut into items where the family, size, weight or style changes and at each
inline object; every text item is shaped as one run, with the whole paragraph as its context
so that a cluster is a position in the paragraph's own text. libunibreak names the positions
a line may break at, and the lines are then filled greedily, whole words only: a word wider
than the box stands on a line of its own and overflows it. Trailing whitespace belongs to the
line it ends and overhangs the box. A tab draws nothing and takes the pen to the next tab
stop, the stops standing four body ems apart from the line's start, as DirectWrite places
them by default.

Every position the layout answers is in its own coordinates - the top left of a box of its
bounds - and alignment moves finished lines inside that box without breaking them again.

A character the matched face has no glyph for is set in the first face fontconfig names
behind it that has one - see FaceChain - and drawn as the matched face's .notdef when none
has. Left to right only, one script per paragraph as HarfBuzz guesses it, and a weight or
slant the family has no face for is whatever fontconfig answers with.

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

## FaceMetrics

The vertical metrics of a face at one em size, in pixels. descent is positive and downwards.
A line stands ascent + descent + lineGap tall with its baseline ascent below its top.

## FontFace

One font file at one face index, as fontconfig matched it. FreeType opens it for the metrics
and the glyph coverage, HarfBuzz reads it for shaping. HarfBuzz is scaled to the face's design
units, so a shaped position is an integer in those units and is put into pixels in float by
whoever asked. FreeType's own scaled metrics are rounded to 1/64 pixel and to the grid, and
a layout working in floats does not want them.

## FontSet

The faces of this machine, as fontconfig names them. A family reaches here as the text states
it: a generic name - see GenericFamily - is a fontconfig alias and resolves through its
configuration, a concrete name matches an installed family or falls to the configured
default, which is fontconfig's answer for a family the machine does not have.

## DirWatchManager

One inotify descriptor serves every watch in the process, and the platform owns it. inotify
multiplexes any number of directories onto a single file descriptor, so the main loop polls
one fd no matter how many DirWatch objects exist - as the TimerManager exposes its eventfd.
WaylandPlatform holds it beside the display and puts notifyFd() in the display's loop through
a PollSource, calling dispatchPending() when it is readable; both happen on the UI thread, so
no marshalling is required on this platform. A DirWatch reaches it through
DirWatchManager::standing, a pointer the manager sets as it is built and clears as it goes,
because a DirWatch sits below every context; the platform stands before anything that owns
one and after it, so a watch finding none is a broken order, and unreachable.

## TimerManager (Linux)

WHERE EVERY UiTimer TICKS ON LINUX: a thread that waits out the delays and an eventfd it
raises, which the display's loop polls through a PollSource WaylandPlatform holds beside it.
One per process and the platform's member. The two hooks System.Timer declares are defined in
Linux.Timer.cpp and reach it through TimerManager::standing, the same pointer and the same
rule as the Windows TimerManager: none standing is a broken order, and unreachable.

## PollSource

A DESCRIPTOR'S PLACE IN THE DISPLAY'S LOOP, held for as long as the object stands: registered
with the display on construction and taken back on destruction, with the dispatcher to call
when the descriptor is readable. WaylandPlatform declares one per manager after the display
and the manager it polls for, so the descriptor is in the loop exactly while both are up,
and the loop names none of what it polls: the connection first, then the registered sources
in the order they were added, then the appearance bus.

## CompositionPresenter

THE PIXELS OF A COMPOSED WINDOW. A window made without a redirection surface has nothing DWM
could show of it; this hangs a DirectComposition visual on it whose content is a swap chain,
and fills the swap chain's back buffer with the form's frame. The frame arrives one of two
ways: as the premultiplied bitmap a CPU backend paints, uploaded from memory, or drawn by a
GPU backend into the frame texture this object lends it - renderSurface - and copied on the
device. Either way the frame is the whole surface, margins included, and the form paints
the part the window takes; the shadow window paints its own margins and reads nothing back.
A window-wide opacity is an effect on the visual rather than a second pass over the pixels.

A SWAP CHAIN AND NOT A COMPOSITION SURFACE: a present waits its turn at the vertical blank
and every frame presented is a frame shown, and it is what Direct2D draws into.

THE BUFFERS ROTATE, so the back buffer handed out holds the frame before the last one, and
what the last frame changed is stale in it. That rect is kept and copied again with the new
frame's own, which is the whole of what two buffers need.

THE DEVICE CAN GO AWAY - a driver reset, a GPU switch, a remote session - and everything here
goes with it, the frame texture and whatever a backend built on the device included. The
generation counts the devices; a backend that finds it moved builds again. An upload that
finds the device gone answers false, drops what it held, and the next one builds it all
again; the caller answers a false by repainting the whole window.

## WaylandPlatform

WHAT AN APPLICATION IS BUILT AGAINST ON WAYLAND, and the owner of everything process-wide the
Linux and Wayland layers have: the DisplayManager, which is the connection and the loop; the
TimerManager and the DirWatchManager, each with a PollSource that puts its descriptor in the
loop; the clipboard. In that order, which is the order they are built in and the reverse of
the order they go in, so nothing is polled for a manager that is not there and the clipboard
hands its selection back to a standing display. It derives from Platform because
ApplicationBase is handed one - the statics are answered in Wayland.Platform.cpp, through
DisplayManager::standing - and from IDisplayAccess, which is what the clipboard's Wayland
half casts the platform to. createWindow answers a FormWindow made on the display; the
constructor throws where there is no compositor, since the display's does.

## IDisplayAccess

WHAT STANDS BEHIND IPlatformServices ON WAYLAND: the display. WaylandPlatform derives from it,
and the clipboard's Wayland half casts the neutral name to it - sound, the one platform of a
Wayland build being the WaylandPlatform - for the data device, the selection and the change
handler. The Wayland twin of the Windows IMessageWindowFactory.

## Win32Platform

WHAT AN APPLICATION IS BUILT AGAINST ON WIN32, and the owner of everything process-wide the
Windows layer has: the WindowsManager, which registers the window classes and opens the OLE
apartment; the TimerManager every UiTimer ticks through; the clipboard. In that order, which
is the order they are built in and the reverse of the order they go in, so a window is never
made from a manager that is not there and the clipboard's flush finds the apartment open. The
two window factories are answered here, from the manager: Platform's createWindow with a
FormWindow, and the Windows IMessageWindowFactory's createMessageWindow with a MessageWindow.
Nothing in the Windows layer reaches the manager any other way - a window is handed it by
whoever makes the window.

## IMessageWindowFactory

WHAT STANDS BEHIND IPlatformServices ON WIN32. The neutral name the clipboard holds the
platform by says nothing; this is the Windows layer saying what a Windows platform offers a
service below the context - a message window - and Win32Platform derives from it. The
clipboard's Windows half casts the name to this on the first watch, which is sound because
the one platform of a Windows build is the Win32Platform.

## TimerManager

WHERE EVERY UiTimer TICKS: a MessageWindow and its WM_TIMER, one per process and
Win32Platform's member, the manager being the window's sink. The two hooks System.Timer
declares - PlatformTimer::start and stop - are defined in its module and reach it through
TimerManager::standing, a pointer the manager sets as it is built and clears as it goes,
because a UiTimer sits below every context: the animation controller's, a directory
watcher's, a control's. The platform stands before anything that owns a UiTimer and after it
- see Application - so a hook finding none standing is a broken order, and unreachable.

## MessageWindow

A MESSAGE-ONLY WINDOW: an HWND of the message class under HWND_MESSAGE, never shown, with
none of what a platform window has - no frame, no placement, no alpha - so it stands on
WindowBase alone and is not a Window. The timers tick through one, built by the
TimerManager from the manager, and the clipboard listens on one, handed out by the
platform through IMessageWindowFactory to a clipboard that never sees the manager. Whoever
holds one names itself as the sink and receives every message the window does; what the
sink leaves unhandled goes to the system's default.

## IMessageSink

WHAT THE MANAGER'S WINDOW PROCEDURE DELIVERS TO: every window the manager made, by the
pointer in the window's user data - a Window and a MessageWindow alike - and, one step on,
what a MessageWindow hands its messages to. One method, the wndProc shape, so that a
service holding a message window handles its messages exactly as a window subclass would,
without being one.

## ShmBuffers

PIXELS THE COMPOSITOR CAN READ WITHOUT BEING SENT THEM. One anonymous file, mapped into
this process and handed to the compositor as a file descriptor, holds both frames; from
there a frame costs a commit rather than a copy down a socket.

TWO OF THEM, because a buffer that has been attached belongs to the compositor until it
says otherwise. Painting into the one on screen tears the frame being read. The other is
painted into meanwhile, and they change places.

## KeyPress

ONE PRESS AS THE FRAMEWORK HEARS IT: the key, the modifiers held with it and the character
it types, if any. Win32 delivers these as two messages, WM_KEYDOWN and the WM_CHAR that
TranslateMessage queues behind it; here they are one thing because they come out of one
translation, and the display hands them on in that order.

## Keyboard

THE KEYBOARD AS xkbcommon DESCRIBES IT, and nothing of the protocol that carries it. The
display hands in what wl_keyboard says - a keymap, the modifier state, a key going down or
up - and this answers with presses in the framework's terms: a Win32 virtual-key code, the
modifiers held, the character typed. It also holds the key repeat, which on Wayland is the
client's to do: the compositor states a rate and a delay and sends each key exactly once.

THE KEY CODE IS THE LATIN LETTER ON THE KEY, whichever layout is active. A shortcut is
written as Shortcut{ L'C', ctrl } and has to fire under a Cyrillic layout too, so a key
whose own letter is not Latin is named by the Latin letter another layout puts on it, and
failing that by its position on a US keyboard - which is how Win32 names every key.

## FormWindow

WHERE A COMPOSITOR EVENT BECOMES SOMETHING THE FRAMEWORK UNDERSTANDS. The window above
knows about surfaces and buffers; this knows about a form, and the two meet only here.

## Window opacity

A WINDOW'S OPACITY IS THE COMPOSITOR'S TO APPLY where it offers wp_alpha_modifier_v1. The
value is stated on the surface as a multiplier, so a change is one commit - no paint, no
pixel pass, no new buffer. The compositor applies it to a buffer without an alpha channel
too, so an opaque window fades the same way. While the window is off the screen the
multiplier waits in the surface's pending state for the commit that maps it, since a bare
commit on an unmapped surface asks for it to be mapped.

WHERE THE COMPOSITOR OFFERS NONE, THE FRAME CARRIES IT. FormWindow::paint writes every pixel
at the window's opacity, and a change repaints the window. Premultiplied colour scales with
its own alpha, so all four channels take one factor and the result is still premultiplied
colour - no destination is read, and a rounded corner keeps the coverage it was drawn with.

TWO CHANNELS SHARE EACH MULTIPLY. The pixel is read as one 32-bit word, blue with red in its
even bytes and green with alpha in its odd ones, and a byte times a byte fits the 16 bits
each channel has there. The division by 255 is (t + (t >> 8)) >> 8 over t = c * o + 128,
which is c * o / 255 rounded to nearest and exact for every pair of bytes. Written this way
the compiler vectorises the loop, and on a frame larger than the cache it runs as fast as a
plain copy of the same rows. A hand-written AVX2 kernel would be x86 only and measured no
faster there.

ZERO UNMAPS on either path. A frame nobody can see is not worth drawing, and an unmapped
window is how a hint says it is not there.

## INativeEventSink

WHAT THE DISPLAY DELIVERS TO. A separate interface so the connection does not have to know
what a window is, and so a window can be registered before its form exists.

One method per thing that can happen, rather than one method and a tagged struct. The
events a display server sends are already typed when they arrive - the code that knows a
button was pressed is the code that hears the button - and flattening that into a tag only
to switch on it again further up throws the knowledge away and then rebuilds it. It also
lets the compiler name every site when one of these changes, which a switch with a default
arm does not.

## SelectionOffer

A SELECTION, AND WHICH CHANNEL IT ARRIVED ON. Two protocols carry the one clipboard: the
seat's data device, which speaks only to the client holding the keyboard focus, and the data
control device, which speaks whatever the focus. An offer is one or the other and never
both, each interface having its own receive request, so the two pointers are not
interchangeable and this is what keeps them apart. Empty while there is nothing to read.

## DisplayManager

THE CONNECTION, THE GLOBALS AND THE LOOP. One per process and WaylandPlatform's first member:
a single-threaded UI has no use for a second connection, and every window in the process
shares this one's file descriptor, which is what makes one poll() serve all of them. The
constructor connects, binds, and THROWS where there is no compositor, naming what
WAYLAND_DISPLAY held; under CLAFI_WAYLAND_TRACE it reports what it bound as it finishes.
Every window is handed the display by whoever makes it - the platform to a FormWindow, and a
window to its buffers - and the clipboard reaches it through IDisplayAccess. The Platform
statics alone, having no object in hand, reach it through DisplayManager::standing, a pointer
the display sets as it is built and clears as it goes; a static asked while none stands is a
broken order, and unreachable.

## FaceChain

The faces fontconfig answers for one family at one weight and slant: the face it matched,
and behind it, in its order of preference, the faces it would turn to for a character the
match has no glyph for. fontconfig trims the list to faces that add coverage, so a walk over
it is short, and a face is opened only once a character asks for it.

## DWriteMonoFont

A monospace face DirectWrite resolved, drawn through the run primitive the layouts reach.
The face is the system collection's first match on weight and style, which is the face a
layout's own matching finds, and IsMonospacedFont is the gate. The cell and the line come
off the one-space probe layout the tab stops are measured from, so they are the numbers a
layout of the same font comes to; a glyph is compared against the space in the design units
the face states, so no scaled float is compared. A run is drawn by
ID2D1RenderTarget::DrawGlyphRun with natural measuring on the Direct2D backend - the same
call FadeTextRenderer makes for a layout's runs - and through the shared glyph cache and the
compositor on the CPU backend.

## FreeTypeMonoFont

A monospace face fontconfig matched, drawn a glyph at a time through the compositor. The
face is the primary of the chain the request names and FT_IS_FIXED_WIDTH is the gate; the
cell is the space's advance at the em size and a glyph's advance is compared as the same
scaled float, so equal design advances compare equal. The line is the face's metrics at that
size, the same height and baseline a line of one run comes to in HarfBuzzLayout.

## FontKey

One face at one size in one measuring mode: what a cached glyph is keyed on. Keyed on one
glyph rather than a whole run - keying on the run would make every distinct string a
separate entry holding its own texture, "Background" and "Backgrounds" sharing nothing, in a
map with no eviction that grows with everything the application ever drew. A glyph key is
bounded by the font's repertoire at the sizes in use, a few hundred entries for an entire
interface, and every string reuses them.

## FontGlyphs

The glyphs of one face at one size, indexed by glyph index rather than searched: hashing per
glyph would put a scattered lookup in the middle of every run, and a font resolved once
makes each glyph after that an array subscript. Sized once, to the face's glyph count, when
the face is first seen, and never grown after - the compositor holds pointers into it for the
length of a run, and a vector that grows moves what those point at. The face is held so the
address the key carries stays this face's: a face nothing holds is released, and DirectWrite
gives its address to the next face it creates - after a reshape, the regular of the same
family at the italic's address, wearing the italic's glyphs.

## DisplayManager::sessionManager

xdg-session-management-v1, staging in wayland-protocols since 1.48 and answered by KWin from
Plasma 6.7. The client never learns where its windows are; it asks the compositor for a
session - a new one, or the one whose id its config holds - names each toplevel in it before
the toplevel's first commit, and the compositor puts the window back where it was on the next
launch, size and state included. The id comes back in `created` when the compositor made a new
session, or is confirmed by `restored`; `replaced` means another instance of the application
took the session over, and everything made from it is inert. A compositor without the global
places every toplevel by its own rule, and the size and state the config kept are then the
client's own answer to the first configure - see Window::createToplevel.

## Window::addToSession

A toplevel joins its session before its first commit, which is the only moment it can be
restored in. A session opened later - the user allowed storing while the application ran - has
no such moment left, and needs none: add_toplevel carries no timing rule, and a name the
session does not know turns restore_toplevel into exactly that request. What it buys is the
compositor keeping this window's state from here on, which is the whole of what the next launch
reads. A toplevel is added once; a name is unique within the session, which the Forms schema
already is.

## DisplayManager::awaitSessionId

A session asked for with no id is answered by `created`, and the id is what the config keeps -
so the store cannot run ahead of it. Waiting is a roundtrip, and a roundtrip dispatches every
event that has arrived, at a moment - a window being hidden - where a configure or a close
would be reentered into the middle of the hide. So the session's proxy is moved to an event
queue of its own, waited for there and moved back, and nothing else is dispatched. One pass is
enough: the compositor answers get_session before the sync that ends the wait.

## DisplayManager::activate

Only the compositor restacks toplevels. The client asks it for a token, naming the input event
that wanted the raise - its serial and seat - and the surface asking, then hands the token back
with the surface to raise.

THE SURFACE HOLDING THE KEYBOARD ASKS. KWin grants a token only to the surface of the window it
holds active, and answers any other surface with a token it will not honour; the keyboard is on
that window. The surface the input landed on can be another one: a menu line is clicked in a
popup, which is never the active window and is unmapped before its command runs. The raise is
granted while the asker is still active, and the window only demands attention otherwise.

The token is waited for on an event queue of its own, as DisplayManager::awaitSessionId waits
for its id, and spent before the call returns, so nothing is left to cancel when a window
closes. One pass is enough: the compositor answers commit before the sync that ends the wait.
Without the global nothing is raised.

## NormalPlacement

WHAT A CONFIG KEEPS OF A FORM ON WIN32, and the client keeps the rectangle: the geometry the
window comes back to when ordinary - WINDOWPLACEMENT's normal rect put into screen coordinates
and stripped of the ring the window reaches past its geometry by - and whether it stands
maximized over it. Handed to the window before its first placement, it is what that placement
places instead of the default, on the monitor the geometry stands on, held up to the floor the
content states now; SetWindowPlacement moves a rect that would be wholly off screen onto a
visible monitor. The window forgets it at its first show, which is also where a maximized one
is shown maximized.

## RememberedPlacement

WHAT A CONFIG KEEPS OF A FORM ON WAYLAND, which is what a client can put back by itself: the
ordinary size in surface pixels - scale-independent, so a screen at 175% and one at 200% agree
- and whether it stood maximized. Where the window was is not in it and cannot be; that is the
compositor's, through the session - see DisplayManager::sessionManager. The size is what the
first placement asks for and what a configure naming no size is answered with; the state is a
request made before the first commit. A compositor restoring the window from its own record
names its own size and state in the first configure, and those win. What such a record lacks
is the size to come back to: KWin keeps the rect a window was closed at, which for one closed
maximized is the maximized rect, and un-maximizes the restored window into that same rect. A
configure that leaves the maximized state naming the size the window already fills is answered
with the remembered size instead - see Window::onToplevelConfigure.

## WindowsManager::appsColorMode

The app mode of Windows' personalisation settings, read afresh from `AppsUseLightTheme` under
`HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize`. That is the half of the
setting that speaks to applications; the system mode beside it colours the taskbar and the Start
menu. A value that cannot be read answers Light, the mode Windows starts in.

## WindowsManager::settingChanged

WM_SETTINGCHANGE IS A BROADCAST, sent to every top-level window - a form, a menu, a hint - and to
no message-only window at all, which is why it is read in the manager's window procedure rather
than by a listener window of its own. The mode is read again on `ImmersiveColorSet` and announced
only where it differs from the last one read, so one change of mode raises one
SystemColorModeEvent however many windows were told, and a change of the accent colour, which
names the same area, raises none.

## AppearanceHandler

Who the appearance reader tells when the mode it answers has moved. The reader stands in the
operating system layer, below the events the core declares, so the Wayland platform names a
plain function that raises SystemColorModeEvent - set the first time the mode is asked for.

## desktopColorMode

The XDG desktop portal's `org.freedesktop.appearance` `color-scheme`, read over a private
connection to the session bus. The first ask connects, asks the bus for the portal's
SettingChanged signal on that one key, and reads the value with ReadOne - Read, where the portal
is older than version 2 - waiting up to a second for the answer. After that the bus socket stands
in the Wayland loop's poll beside the display, the timers and the directory watches, and a change
arrives as a signal. 1 is Dark; 0, 2 and any value the portal adds later are Light. A session
without a bus answers Light and is not asked again; a bus that hangs up leaves the last answer
standing.

