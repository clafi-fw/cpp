# Platform

The words that do not fit above a declaration in the Platform folders.

## ShadowWindow

**The pointer passes through it**, so a mouse click on the shadow reaches whatever lies below,
another application included. On Win32 a shadow drawn in the window's own surface would catch the
pointer.

The window it shadows owns it, so it stays above that window through minimize and activation.

## IdWriteRunEffect

**DirectWrite breaks runs where the effect changes.** Two ranges that differ only in their effect
arrive as two runs, so a superscript can sit beside a subscript of the same size.

## FadeTextRenderer

The IDWriteTextRenderer a DirectWrite layout is drawn through. It fades the lines that run past the
edge and draws the collapse - see LineMove.

## LineMove

**A collapse lifts the lines below the last one that fits** onto that line's baseline. Each starts
where the one before it ends, and a line that would start past the box's right edge is hidden.

## Direct2DBackend

**The device is the window's.** Each paint brings the window's CompositionPresenter as its native
context. The backend builds everything on that presenter's D3D device and draws into its frame
texture, and the window presents it. When generation() changes, the backend builds again.

## Wayland::Window

**Several members of IPlatformWindow are no-ops here**, each with the reason on its comment. The
contract is shared with Win32, which answers them all.

## HarfBuzzLayout

**Lines break greedily, whole words only**, where libunibreak allows a break.

- A word wider than the box takes a line of its own and overflows it.
- Trailing whitespace stays on its line and may overhang the box.
- Tab stops are four spaces apart, in the font the paragraph starts in.

setAlignment and setMaxWidth move the finished lines inside the box without breaking them again.

A character the matched face lacks comes from the next face in the FaceChain that has it, else it
is the .notdef. Text runs left to right only, with one script per run as HarfBuzz guesses it.

## FaceMetrics

`descent` is positive, downwards. A line is `ascent + descent + lineGap` tall, with its baseline
`ascent` below its top.

## FontFace

FreeType reads the metrics and the coverage, HarfBuzz the shaping. **hbFont() works in design
units**, so scale a shaped position by emSize / unitsPerEm().

## FontSet

A generic name - see GenericFamily - resolves through fontconfig's aliases. A concrete name matches
an installed family, or falls to fontconfig's default.

## DirWatchManager

WaylandPlatform owns it and polls notifyFd() in the display's loop through a PollSource. A watch's
callback runs on the UI thread.

## TimerManager (Linux)

A thread waits out the delays and raises an eventfd, which the display's loop polls through a
PollSource. The timers fire on the UI thread.

## PollSource

The constructor adds the descriptor to the display's loop, with the dispatcher to call when it is
readable. The destructor removes it.

## CompositionPresenter

**DWM shows a composed window through it.** It hangs a DirectComposition visual with a swap chain on
the window and fills the back buffer with the form's frame:

- present - from the bitmap a CPU backend paints.
- presentSurface - from the frame texture a GPU backend draws into, taken from renderSurface.

**The device can be lost** - a driver reset, a GPU switch, a remote session - and everything built
on it goes too. generation() changes with each new device, and a backend that sees it change builds
again. A present that finds the device gone returns false: repaint the whole window.

## WaylandPlatform

**It owns everything process-wide** on Wayland, built in this order and destroyed in reverse:

- DisplayManager - the connection and the loop.
- TimerManager and DirWatchManager, each with a PollSource in the loop.
- The clipboard.

The constructor throws where there is no compositor - see DisplayManager.

## IDisplayAccess

The clipboard's Wayland half casts IPlatformServices to it. The cast is sound because
WaylandPlatform is the only platform of a Wayland build. Its Windows twin is IMessageWindowFactory.

## Win32Platform

**It owns everything process-wide** on Windows, built in this order and destroyed in reverse:

- WindowsManager - registers the window classes and opens the OLE apartment.
- TimerManager.
- The clipboard, whose flush needs the apartment open.

createWindow and createMessageWindow build their windows from the manager. A window gets the
manager from whoever makes it, and from nowhere else.

## IMessageWindowFactory

The clipboard's Windows half casts IPlatformServices to it. The cast is sound because Win32Platform
is the only platform of a Windows build. Its Wayland twin is IDisplayAccess.

## TimerManager

A member of Win32Platform and the sink of its own MessageWindow. Each WM_TIMER fires the UiTimer it
carries.

## MessageWindow

An HWND under HWND_MESSAGE. It has no frame, placement or alpha, so it derives from WindowBase
alone.

**The holder sets itself as the sink** and gets every message. What the sink leaves unhandled goes
to DefWindowProc. TimerManager and the clipboard each hold one.

## IMessageSink

The manager's window procedure delivers to the sink in the window's user data - a Window and a
MessageWindow alike. With the wndProc shape, a service holding a message window handles messages as
a window subclass would.

## ShmBuffers

One anonymous file, shared with the compositor as a file descriptor, holds both frames. A frame
costs a commit, with no copy down the socket.

## KeyPress

**One translation yields the key and its character.** Win32 sends them as WM_KEYDOWN and then
WM_CHAR, and the display hands them on in that order.

## Keyboard

Turns what wl_keyboard sends into KeyPress values. **It runs the key repeat**, which on Wayland is
the client's job.

**The key code is the Latin letter on the key**, so `Shortcut{ L'C', { .ctrl = true } }` fires under
a Cyrillic layout too. A key with no Latin letter takes the one another layout puts on it, else its
position on a US keyboard, as Win32 names keys.

## Wayland::FormWindow

Its base, Window, handles surfaces and buffers. FormWindow handles the form, and the two meet only
here.

## Wayland::Window alpha

**The compositor applies the opacity** where it offers wp_alpha_modifier_v1. A change is one
commit, with no paint and no new buffer, and an opaque window fades too.

**Elsewhere the frame carries it.** FormWindow::paint writes every pixel at the opacity, and a
change repaints the window.

**Zero unmaps the window** on either path - that is how a tooltip says it is not there.

## INativeEventSink

**A window can register before its form exists**, since the display sees only this interface.

## SelectionOffer

The clipboard arrives on one of two channels:

- `seat` - the data device, heard only by the client with the keyboard focus.
- `control` - the data control device, heard whatever the focus.

At most one is set, since each has its own receive request.

## DisplayManager

**Every window shares its file descriptor**, so one poll() serves them all.

**The constructor throws where there is no compositor**, naming what WAYLAND_DISPLAY held. Under
CLAFI_WAYLAND_TRACE it reports what it bound.

## FaceChain

The primary is fontconfig's match. Behind it, in fontconfig's order, are the faces it would use for
a character the match lacks. Each adds coverage, and each opens when a character first asks for it.

## FreeTypeMonoFont

The face is the one the layouts use, the primary of fontconfig's chain. createNativeMonoFont returns
null unless FT_IS_FIXED_WIDTH holds and the face has a space.

The cell is the space's advance, and glyphOf takes a glyph whose scaled advance equals it. The line
height and baseline match a one-run line in HarfBuzzLayout.

## FontGlyphs

**`face` holds the face**, so the address in its FontKey always belongs to this face. The slots are
sized once, so a reference coverageOf returns stays valid.

## DisplayManager sessionManager

xdg-session-management-v1, in wayland-protocols staging since 1.48 and offered by KWin from Plasma
6.7.

**The compositor remembers where windows were.** The client opens a session - new, or by the id its
config holds - and names each toplevel before its first commit. The next launch restores each
window, size and state included.

- `created` - a new session, with its id.
- `restored` - the stored id is confirmed.
- `replaced` - another instance took the session over, and everything made from it is inert.

A compositor without the global places toplevels by its own rule. The config's size and state then
answer the first configure - see RememberedPlacement.

## Window::addToSession

**For a session opened after the window's first commit**, when the user allows storing mid-run. A
toplevel can be restored only before that commit, so the window is added instead, and the
compositor keeps its state from then on. A toplevel is added once, under a name unique in the
session.

## DisplayManager::awaitSessionId

Call it before storing the config. A session asked for with no id gets its id in `created`, and the
store writes that id.

## DisplayManager::activate

**The compositor does the raise**, with a token it issues for the input the stamp carries. The call
does nothing where the compositor offers no xdg-activation.

## ExportedToplevel

Another client parents its own window by the handle - the portal's file chooser takes `wayland:`
and the handle.

**Pass a toplevel's surface.** Any other is a protocol error, so check Window::isToplevelRole first.
handle() is empty where the compositor offers no exporter, and a dialog given an empty handle opens
unparented.

## NormalPlacement

**What a config keeps of a form on Win32.** `geometry` is WINDOWPLACEMENT's normal rect in screen
coordinates, without the ring the window reaches past its geometry.

**Set it before the first placement.** Until the first show, a Default placement uses the geometry,
grown to the content's minimum, and SetWindowPlacement pulls a rect wholly off screen onto a
monitor. The first show applies `maximized` and `alwaysOnTop`, and the window forgets the placement.

## RememberedPlacement

**What a config keeps of a form on Wayland.** The size is in surface pixels, so screens at 175% and
200% agree. The position is the compositor's - see DisplayManager::sessionManager. A compositor
restoring the window from its own record overrules both fields.

**Un-maximizing returns to the remembered size.** KWin restores a window closed maximized into the
maximized rect, and un-maximizes into that same rect. A configure that leaves the maximized state at
the size the window already fills gets the remembered size instead.

## WindowsManager::appsColorMode

Reads `AppsUseLightTheme` under `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize`,
the half of the setting that speaks to applications. A value that cannot be read answers Light.

## AppearanceHandler

The appearance reader sits below the core's events, so it calls a plain function. The Wayland
platform sets one that raises SystemColorModeEvent.

## desktopColorMode

The portal's `org.freedesktop.appearance` `color-scheme`, over a private session bus connection. The
first call connects and waits up to a second for the value. After that the Wayland loop polls the
bus, and a change arrives as a signal.

- 1 is Dark, and any other value is Light.
- A session without a bus answers Light and is not asked again.
- A bus that hangs up leaves the last answer in place.

## FolderRequest

Opens the portal's FileChooser for a folder, over a private session bus connection held until the
request settles.

**Poll fd() and call dispatch() after every wake** until settled(). answer() then holds the
Response's first URI as a local path. It holds nothing where the dialog was cancelled, the session
has no bus or no file chooser, or the bus hung up.

