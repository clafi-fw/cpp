# Transfer

The words that no longer fit above a declaration.

## StandardFormat

THE FRAMEWORK'S OWN FORMATS, AND THE PLATFORM'S ARE ANOTHER THING. Text and Picture are
what a paste asks for; CF_UNICODETEXT, CF_TEXT, CF_DIB, CF_DIBV5, PNG, image/png are what a
clipboard carries, each under its platform name, each with bytes of its own. A paste that
wants a text or a picture asks for the framework format and the platform decides which of
its formats fits - see Offer::platformFormat. A viewer of the clipboard never asks for one:
it lists every platform format under its platform name, CF_UNICODETEXT beside CF_TEXT, and
reads each as it is.

## Format

WHICH FORMAT, at runtime. A name is a name: what separates one this framework knows how to
deal with from one it does not is whether anything is registered against it, which is a
question for the tables rather than for the identity.

A REGISTERED FORMAT'S NAME IS THE ONE IT WAS DECLARED WITH, and each platform derives its
native spelling from that. A PLATFORM FORMAT'S NAME IS ITS PLATFORM NAME, that being the
only name it has - the predefined ones spelled by the platform layer, CF_UNICODETEXT and its
kind - so a viewer can list what is on the clipboard and read it, whether or not anything
here understands it.

## ConversionTable

WHICH FORMATS REACH WHICH, and how. Single hop: a path of two steps is a conversion of its
own, registered by whoever chose what it loses. Read in both directions - outward it says
what a package can advertise beyond what it holds, inward what an offer can satisfy.

## ClipboardChangeEvent

THE SELECTION MOVED ON, and that is the whole of what it says. It carries nothing because
neither display server offers more: the answer to what is on the clipboard now is read by
asking, and this only says that asking again is worth it.

WAYLAND SAYS LESS THAN WINDOWS HERE, and the difference is in the contract rather than in
the implementation. A selection is announced only to the client holding the keyboard focus,
and announced again as it takes the focus, so an unfocused window is told nothing - which
costs nothing, because an unfocused window is not allowed to read the selection either.
Windows raises it whichever window is in front.

AN APPLICATION THAT WATCHES THE CLIPBOARD RATHER THAN PASTING FROM IT can say so and be
given the clipboard manager's channel, where the focus does not come into it - see
WaylandPlatform::Params. Not every compositor offers one, so the paragraph above is still
what this event promises rather than what it always delivers.

ON WINDOWS IT IS RAISED ONCE THE CHANGE HAS BEEN READ. Every listener in the session is told
at the same moment and one holder at a time is all the clipboard allows, so a reader can find
it shut. A change that could not be read within a few quick attempts is read again a second
later and announced then; until then the last reading stands, and Clipboard::offer answers
what it answered before rather than an empty clipboard.

## ByteSpellingTable

WHICH FORMATS THIS APPLICATION KNOWS HOW TO DEAL WITH, and how. Nothing is in it until
something puts it there - see ConversionTable, which says why that is a call rather than
static initialisation.

A named format that states no bytes cannot cross to another process, so this is also the
set a platform derives its native spellings for. Everything else on a clipboard is named by
whatever the platform calls it and read as the bytes it arrived as.

## Source

WHAT LEAVES THE APPLICATION: the content captured when the user copied, cut or started a
drag, and nothing about how any platform spells it. Built on its own and then handed to a
context, so one package serves a copy and a drag alike.

CAPTURE IS EAGER. A cut has taken the text out of the control before anything pastes, so
there is nothing left to read back, and a copy goes the same way. What waits is conversion,
not capture.

Moved, never copied: a package can hold the whole of a document selection.

## Offer

WHAT ARRIVES FROM OUTSIDE: the formats somebody else advertises, and a way to ask for one.
A read blocks - the content is fetched from whoever owns the selection, and no platform
says whether it is held or about to be made - so nothing reads to answer a question about
availability.

THE LIST IS THE PLATFORM'S FORMATS, every one under its platform name, and the framework's
own are reached through platformFormat: asked for Text or Picture, an offer answers which of
the formats it lists would serve it, by reference into that list, or null. Windows serves
Text from CF_UNICODETEXT, which it synthesises from CF_TEXT so it is there whenever text is,
and a Picture from PNG where the source placed one - lossless, its alpha intact - else
CF_DIBV5, else CF_DIB, else CF_BITMAP, a GDI handle read as the DIB its pixels make (which
is also what its bytes are); Wayland from the text/plain names, and from image/png else
image/bmp. A read of the framework format decodes what that platform format carries; a
read of the platform format itself answers its bytes. This process's own package is listed
the same way, under the names the platform would place it as - see platformFormatOf.

## PlainText

ONE OF THE TWO FORMATS THE CORE SHIPS. Plain text is content whose type this layer can name
without knowing anything about the framework above it. Every other format - the framework's
own rich text included - is declared and registered by whoever owns the type it carries.

## Picture

A PICTURE AS PIXELS, a Graphics::Bitmap, which is the one picture type the layers below the
platform agree on. The platform serves it from whichever of its formats carries one best -
see Offer - and decodes that on the way in, a PNG through Graphics::decodePng and a DIB or
BMP through Graphics::decodeDib, so what a reader takes is pixels and never a header.

READING ONLY. A package holding a picture is not spelled on the way out yet: it would go
out as CF_DIB and as a BMP file, the platform names this framework places it under.

## Clipboard

THE ONE WAY ON AND OFF THE SYSTEM CLIPBOARD, and one object per process. The platform built for the
display server - Win32Platform, WaylandPlatform - owns it as a member and names it to the Platform
base it derives from, so the core reaches it through any Platform without knowing which one; a
form's context hands it out as FormContext::clipboard, which is how a control asks for it.

A COPY IS A STANDING OFFER THIS PROCESS SERVES. Neither platform takes the bytes at copy
time: the package is kept here and read when somebody pastes, so it lives until another
application takes the selection. On Win32 the package is rendered onto the clipboard as the
clipboard object goes, while this process still owns it, so a copy outlives the process; on
Wayland leaving the process ends the offer, and a cut therefore loses its content there
unless a clipboard manager has taken a copy of it.

## Format::Kind

Which vocabulary names a format: the framework's own closed set, or anything else. Both kinds have
names; only the standard set has one this layer can state, every platform spelling the others for
itself.
