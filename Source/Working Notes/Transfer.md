# Transfer

The author's side of the footnote of the same name.

## Clipboard

Nothing about the clipboard is a static: what it holds, the listeners and the protocol object behind
the package all live and die with the platform, in the order the platform states. On Win32 that
order is the OLE apartment first and the clipboard inside it, so the flush on the way out finds the
apartment open and OleUninitialize finds the clipboard gone.

It is handed the platform at construction as IPlatformServices - the one name it can hold it
by, Transfer standing below Context - and its Windows half casts that to the Windows layer's
IMessageWindowFactory for its listener window. set, offer, release and the two constructors
are the platform's and are defined in an implementation unit of this module under Platform/;
what the package itself is made of is not any platform's business. The platform's own state
stands behind NativeClipboard.

## NativeClipboard

THE PLATFORM'S HALF OF THE CLIPBOARD. Declared in the interface without a body and defined
in the platform's implementation unit, so the interface names no platform type: on Win32 it
is the IDataObject OLE holds a reference to, the clipboard sequence numbers the held reader
and the own copy were seen at, the retry flag and the listener window; on Wayland it is the
wl_data_source behind the selection and the offer object the held reader was built for. It
is a friend of Clipboard and the only thing that is: the helpers a platform builds - the
data object, the source listener - reach the package through it and never through the
clipboard itself.

THE LISTENER WINDOW IS THE PLATFORM'S TO MAKE. On the first watch the Windows half casts the
platform the clipboard was handed to the Windows IMessageWindowFactory - sound, the one
platform of a Windows build being the Win32Platform, which derives from it - asks it for a
MessageWindow and names itself as the window's sink, so it receives WM_CLIPBOARDUPDATE and
its retry timer as a window subclass would, without knowing the manager the window was made
from.

Its destructor is where a platform lets go. Win32 flushes, and only where OleIsCurrentClipboard
says the package is still what is on the clipboard: OLE refuses the flush from any other
owner, and a package another application's copy replaced has already been released with it.
Wayland destroys the standing source and clears the display's change handler, which named
this object. The display it does that on is the platform's, reached by casting the name the
clipboard holds the platform by to the Wayland layer's IDisplayAccess - see Platform.

## Format::Kind

The members stand in the same order as the identity alternatives, which is what lets kind() read off
the index.
