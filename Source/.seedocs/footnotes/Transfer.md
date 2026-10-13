# Transfer

The words that do not fit above a declaration.

## StandardFormat

**A paste asks for Text or Picture**, and the platform picks which of its own formats serves it -
see Offer::platformFormat.

**A clipboard viewer lists the platform formats instead** - CF_UNICODETEXT beside CF_TEXT - and
reads each as it is.

## Format

Made with `of` for a standard format, `custom` for a named one. **The tables say whether the
framework knows a named format** - ByteSpellingTable and ConversionTable. The name alone says
nothing about it.

- A registered format keeps the name it was declared with, and each platform spells it from that.
- Any other format goes by its platform name, CF_UNICODETEXT and the like, so a viewer can list
  and read it.

## ConversionTable

**One step only.** A path of two steps is a conversion of its own, registered by whoever chose
what it loses.

Read outward, it says what a Source advertises beyond what it holds. Read inward, it says what an
Offer can satisfy.

**Fill it by calling registerConversion**, never at static initialisation. A static library drops
a translation unit nothing references, so the table would be full in some builds and empty in
others.

## ClipboardChangeEvent

It carries nothing, because neither display server says more. Ask the clipboard what it holds when
the event arrives.

- **Windows** raises it whichever window is in front, once the change has been read. A change that
  cannot be read after a few quick attempts is read a second later and announced then. Until then
  Clipboard::offer answers the previous reading, not an empty clipboard.
- **Wayland** tells only the client holding the keyboard focus, and tells a client again as it
  takes the focus. An unfocused window may not read the selection either.

**A clipboard watcher on Wayland** sets `watchesClipboard` in WaylandPlatform::Params. Where the
compositor offers a clipboard manager's channel, the focus then does not matter. Mutter offers
none.

## ByteSpellingTable

**Fill it by calling registerByteSpelling**, for the reason ConversionTable gives.

**It is also the set a platform spells natively.** A named format crosses to another process only
with a byte spelling. Anything else keeps its platform name and is read as the bytes it arrived as.

## Source

Handed to Clipboard::set by a copy or a cut. It holds content by framework format, and nothing of
how a platform spells it.

**Capture is eager**: the content goes in at the copy or cut, since a cut leaves nothing to read
back later. Conversion waits for the first paste that asks for it.

**Moved, never copied**: a package can hold a whole document selection.

## Offer

**A read blocks** while the content is fetched from whoever owns the selection. To learn what is
available, ask match or Clipboard::accepts, which read the advertised names alone.

**platformFormat picks the format that serves Text or Picture**, best first:

- Windows - Text from CF_UNICODETEXT, which Windows synthesises from CF_TEXT, so it is there
  whenever text is. A Picture from PNG or image/png, else CF_DIBV5, else CF_DIB, else CF_BITMAP.
- Wayland - Text from text/plain;charset=utf-8, text/plain or UTF8_STRING. A Picture from
  image/png, else image/bmp.

**CF_BITMAP is a GDI handle.** read decodes it into a picture, readBytes answers no bytes, and
readHandle answers the handle.

This process's own copy is listed under the names the platform places it under - CF_UNICODETEXT
and CF_DIB, or text/plain;charset=utf-8 and image/bmp.

## PlainText

One of the two formats the core ships, with Picture. Every other format - the framework's own rich
text included - is declared and registered by whoever owns the type it carries.

## Picture

A Graphics::Bitmap, decoded from the format that serves it - a PNG by Graphics::decodePng, a DIB or
BMP by Graphics::decodeDib. A reader takes pixels, never a header.

**Reading only.** A Picture placed in a Source goes out with no bytes, under CF_DIB or image/bmp.

## Clipboard

One object per process, owned by the platform. A control reaches it through
FormContext::clipboard, the core through Platform::clipboard.

**A copy is a standing offer this process serves.** The package is kept here and read when
somebody pastes, so it lives until another application takes the selection.

- On Win32 the package is written onto the clipboard when the clipboard object is destroyed, if
  this process still owns the selection. A copy outlives the process.
- On Wayland the offer ends with the process. A cut loses its content there unless a clipboard
  manager took a copy.

## Format::Kind

- `Standard` - one of StandardFormat, read with standard(). Each platform spells it in formats of
  its own.
- `Custom` - a format known by its name, read with name(): a registered format or a platform's
  own.
