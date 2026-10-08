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
