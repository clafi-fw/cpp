# Platform

The words that no longer fit above a declaration.

## ShadowWindow

**The shadow of a composed window**, as a window of its own. Owned by the window it shadows, so it
stands above it and follows it through minimize and activation; transparent by style, so the pointer
goes through it to whatever is underneath, another application included - the one thing a shadow
painted in the window's own surface cannot say on Win32, where a pixel with any alpha at all owns
the pointer. Painted by the form's ShadowPainter into the four strips of margin around the hole
where the window itself stands - the same call the form paints its own ring with, clipped to a strip
each time - and presented as a layered window.

## IdWriteRunEffect

What one run is drawn with, over and above the format the layout was built from. A drawing
effect is also what DirectWrite breaks runs at, so two ranges that differ only in what they
carry here still arrive as two runs - which is what lets a superscript stand beside a
subscript of the same size.

## FadeTextRenderer

The IDWriteTextRenderer a DirectWrite layout is drawn through. It fades the lines the fit
named, and draws the collapse: the lines below the last one that fits, carried on along its
baseline.

## Direct2DBackend

Satisfies the GPU backend contract: it renders into a window the platform owns. **The device
is the window's.** Every paint arrives with the window's CompositionPresenter as its native
context, and everything here - the Direct2D device, its context, the target bitmap over the
presenter's frame texture, the brushes and the kept images - is built on that presenter's
D3D device and built again when the presenter says the device moved. What is drawn lands in
the frame texture; presenting it is the window's.

## Window

**One window, and what a compositor will and will not answer about it.**

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

## FaceMetrics

The vertical metrics of a face at one em size, in pixels. descent is positive and downwards.
A line stands ascent + descent + lineGap tall with its baseline ascent below its top.

## FontFace

One font file at one face index, as fontconfig matched it. FreeType opens it for the metrics and the
glyph coverage, HarfBuzz reads it for shaping. HarfBuzz is scaled to the face's design units, so a
shaped position is an integer in those units and is put into pixels in float by whoever asked.

## FontSet

The faces of this machine, as fontconfig names them. A family reaches here as the text states
it: a generic name - see GenericFamily - is a fontconfig alias and resolves through its
configuration, a concrete name matches an installed family or falls to the configured
default, which is fontconfig's answer for a family the machine does not have.

## DirWatchManager

One inotify descriptor serves every watch in the process, and the platform owns it. inotify
multiplexes any number of directories onto a single file descriptor, so the main loop polls one fd
no matter how many DirWatch objects exist - as the TimerManager exposes its eventfd. WaylandPlatform
holds it beside the display and puts notifyFd() in the display's loop through a PollSource, calling
dispatchPending() when it is readable; both happen on the UI thread, so no marshalling is required
on this platform.

## TimerManager (Linux)

**Where every UiTimer ticks on Linux**: a thread that waits out the delays and an eventfd it raises,
which the display's loop polls through a PollSource WaylandPlatform holds beside it. One per process
and the platform's member.

## PollSource

**A descriptor's place in the display's loop**, held for as long as the object stands: registered
with the display on construction and taken back on destruction, with the dispatcher to call when the
descriptor is readable.

## CompositionPresenter

**The pixels of a composed window.** A window made without a redirection surface has nothing DWM
could show of it; this hangs a DirectComposition visual on it whose content is a swap chain,
and fills the swap chain's back buffer with the form's frame. The frame arrives one of two
ways: as the premultiplied bitmap a CPU backend paints, uploaded from memory, or drawn by a
GPU backend into the frame texture this object lends it - renderSurface - and copied on the
device. Either way the frame is the whole surface, margins included, and the form paints
the part the window takes; the shadow window paints its own margins and reads nothing back.
A window-wide opacity is an effect on the visual rather than a second pass over the pixels.

**The device can go away** - a driver reset, a GPU switch, a remote session - and everything here
goes with it, the frame texture and whatever a backend built on the device included. The
generation counts the devices; a backend that finds it moved builds again. An upload that
finds the device gone answers false, drops what it held, and the next one builds it all
again; the caller answers a false by repainting the whole window.

## WaylandPlatform

**What an application is built against on Wayland**, and the owner of everything process-wide the
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

**What stands behind IPlatformServices on Wayland**: the display. WaylandPlatform derives from it,
and the clipboard's Wayland half casts the neutral name to it - sound, the one platform of a
Wayland build being the WaylandPlatform - for the data device, the selection and the change
handler. The Wayland twin of the Windows IMessageWindowFactory.

## Win32Platform

**What an application is built against on Win32**, and the owner of everything process-wide the
Windows layer has: the WindowsManager, which registers the window classes and opens the OLE
apartment; the TimerManager every UiTimer ticks through; the clipboard. In that order, which
is the order they are built in and the reverse of the order they go in, so a window is never
made from a manager that is not there and the clipboard's flush finds the apartment open. The
two window factories are answered here, from the manager: Platform's createWindow with a
FormWindow, and the Windows IMessageWindowFactory's createMessageWindow with a MessageWindow.
Nothing in the Windows layer reaches the manager any other way - a window is handed it by
whoever makes the window.

## IMessageWindowFactory

**What stands behind IPlatformServices on Win32.** The neutral name the clipboard holds the
platform by says nothing; this is the Windows layer saying what a Windows platform offers a
service below the context - a message window - and Win32Platform derives from it. The
clipboard's Windows half casts the name to this on the first watch, which is sound because
the one platform of a Windows build is the Win32Platform.

## TimerManager

**Where every UiTimer ticks**: a MessageWindow and its WM_TIMER, one per process and Win32Platform's
member, the manager being the window's sink.

## MessageWindow

**A message-only window**: an HWND of the message class under HWND_MESSAGE, never shown, with
none of what a platform window has - no frame, no placement, no alpha - so it stands on
WindowBase alone and is not a Window. The timers tick through one, built by the
TimerManager from the manager, and the clipboard listens on one, handed out by the
platform through IMessageWindowFactory to a clipboard that never sees the manager. Whoever
holds one names itself as the sink and receives every message the window does; what the
sink leaves unhandled goes to the system's default.

## IMessageSink

**What the manager's window procedure delivers to**: every window the manager made, by the
pointer in the window's user data - a Window and a MessageWindow alike - and, one step on,
what a MessageWindow hands its messages to. One method, the wndProc shape, so that a
service holding a message window handles its messages exactly as a window subclass would,
without being one.

## ShmBuffers

**Pixels the compositor can read without being sent them.** One anonymous file, mapped into
this process and handed to the compositor as a file descriptor, holds both frames; from
there a frame costs a commit rather than a copy down a socket.

## KeyPress

**One press as the framework hears it**: the key, the modifiers held with it and the character
it types, if any. Win32 delivers these as two messages, WM_KEYDOWN and the WM_CHAR that
TranslateMessage queues behind it; here they are one thing because they come out of one
translation, and the display hands them on in that order.

## Keyboard

**The keyboard as xkbcommon describes it**, and nothing of the protocol that carries it. The
display hands in what wl_keyboard says - a keymap, the modifier state, a key going down or
up - and this answers with presses in the framework's terms: a Win32 virtual-key code, the
modifiers held, the character typed. It also holds the key repeat, which on Wayland is the
client's to do: the compositor states a rate and a delay and sends each key exactly once.

**The key code is the Latin letter on the key**, whichever layout is active. A shortcut is
written as Shortcut{ **l'c'**, ctrl } and has to fire under a Cyrillic layout too, so a key
whose own letter is not Latin is named by the Latin letter another layout puts on it, and
failing that by its position on a US keyboard - which is how Win32 names every key.

## FormWindow

**Where a compositor event becomes something the framework understands.** The window above
knows about surfaces and buffers; this knows about a form, and the two meet only here.

## Window opacity

**A window's opacity is the compositor's to apply** where it offers wp_alpha_modifier_v1. The value
is stated on the surface as a multiplier, so a change is one commit - no paint, no pixel pass, no
new buffer. The compositor applies it to a buffer without an alpha channel too, so an opaque window
fades the same way.

**Where the compositor offers none, the frame carries it.** FormWindow::paint writes every pixel at
the window's opacity, and a change repaints the window.

**Zero unmaps** on either path. A frame nobody can see is not worth drawing, and an unmapped
window is how a hint says it is not there.

## INativeEventSink

**What the display delivers to.** A separate interface so the connection does not have to know
what a window is, and so a window can be registered before its form exists.

## SelectionOffer

**A selection, and which channel it arrived on.** Two protocols carry the one clipboard: the
seat's data device, which speaks only to the client holding the keyboard focus, and the data
control device, which speaks whatever the focus. An offer is one or the other and never
both, each interface having its own receive request, so the two pointers are not
interchangeable and this is what keeps them apart. Empty while there is nothing to read.

## DisplayManager

**The connection, the globals and the loop.** One per process and WaylandPlatform's first member: a
single-threaded UI has no use for a second connection, and every window in the process shares this
one's file descriptor, which is what makes one poll() serve all of them. The constructor connects,
binds, and **throws** where there is no compositor, naming what WAYLAND_DISPLAY held; under
CLAFI_WAYLAND_TRACE it reports what it bound as it finishes. Every window is handed the display by
whoever makes it - the platform to a FormWindow, and a window to its buffers - and the clipboard
reaches it through IDisplayAccess.

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

One face at one size in one measuring mode: what a cached glyph is keyed on.

## FontGlyphs

The glyphs of one face at one size, indexed by glyph index rather than searched: hashing per glyph
would put a scattered lookup in the middle of every run, and a font resolved once makes each glyph
after that an array subscript.

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

A session asked for with no id is answered by `created`, and the id is what the config keeps - so
the store cannot run ahead of it.

## DisplayManager::activate

Only the compositor restacks toplevels. The client asks it for a token, naming the input event
that wanted the raise - its serial and seat - and the surface asking, then hands the token back
with the surface to raise.

## ExportedToplevel

A toplevel named to other clients by a handle, through xdg-foreign's exporter, for as long as the
object stands. Another client parents a window of its own by the handle - the portal's file
chooser is parented this way, as `wayland:` and the handle. The handle is empty where the
compositor offers no exporter or the surface is no toplevel, and a dialog asked for with it then
stands alone.

## NormalPlacement

**What a config keeps of a form on Win32**, and the client keeps the rectangle: the geometry the
window comes back to when ordinary - WINDOWPLACEMENT's normal rect put into screen coordinates
and stripped of the ring the window reaches past its geometry by - whether it stands maximized
over it, and whether it is held above the others. Handed to the window before its first
placement, it is what that placement places instead of the default, on the monitor the geometry
stands on, held up to the floor the content states now; SetWindowPlacement moves a rect that
would be wholly off screen onto a visible monitor. The window forgets it at its first show,
which is also where a maximized one is shown maximized and one held above the others is put
there.

## RememberedPlacement

**What a config keeps of a form on Wayland**, which is what a client can put back by itself: the
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

## FolderRequest

One folder asked of the desktop's file chooser: the XDG portal's FileChooser, OpenFile with
`directory` and `modal` set and the start folder as `current_folder`, on a private connection to
the session bus that stands until the request is settled. The answer is the first of the URIs
the Response names, read as a path on this machine. A cancelled dialog, a session without a bus,
a portal without a file chooser and a bus that hangs up all settle the request with no answer.
Whoever waits polls `fd` and calls `dispatch` after every wake, until `settled`.

## WebFetch on Windows

WinHTTP in its blocking form, on the fetch's thread. The session takes the system's proxy
settings (WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY) and TLS comes from the operating system, so
nothing ships with an application for it. Each step - resolving, connecting, sending, receiving -
may take ten seconds.

## WebFetch on Linux

libcurl, opened with dlopen when a fetch runs: libcurl.so.4, then libcurl-gnutls.so.4, the only one
a fresh Ubuntu desktop carries. Nothing links against it - the build needs no libcurl headers, the
executable names no libcurl, and a system without one answers every fetch with nothing.

Cancelling is the progress callback answering nonzero, which libcurl asks at least once a second, so
a cancel waits that long at most. The connection may take ten seconds and the whole transfer thirty.

## CommandRun on Windows

cmd.exe from the system directory, given `/s /c` and the line in quotes, so the line is read as it
would be typed into a console: a program is found on the PATH whatever its extension - an editor's
launcher is often a .cmd - and the line's quotes are read as cmd reads them, `/s` taking off the
pair put round the whole. The process is made with CREATE_NO_WINDOW, and a thread waits for it
beside an event the destructor sets. cmd exits with 9009 where no program answers to the name the
line starts with, which is NotFound. Zero is Ended, and every other code is Failed, whether cmd or
the program gave it.

## CommandRun on Linux

/bin/sh -c over the line, in a process of its own. The application forks a child that forks the
waiting process and ends at once, so the waiting process belongs to init and the application has no
child to reap. The waiting process forks the shell, waits for it, and writes what it exited with
into a pipe that a thread of the run polls beside a stop pipe the destructor writes into. sh exits
with 127 where no program answers to the name and with 126 where the program found would not run -
NotFound and NotRunnable. Zero is Ended and every other code Failed; a program killed by a signal
is written as 128 and the signal's number. A waiting process that could not fork the shell writes
NotStarted's answer instead.

**The waiting process keeps standard input and output and the status pipe, and nothing else.** It
runs on as a copy of the application, running no program of its own, so it would otherwise keep
every descriptor the application has open for as long as the program runs - and a display server
keeps a client's windows up while its connection is open. It moves the status pipe to descriptor 3
and closes everything above it.
