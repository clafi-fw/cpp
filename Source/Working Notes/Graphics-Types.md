# Graphics Types

The author's side of the footnote of the same name.

## ScopedTextRaster

A run that stayed silent would leave the backend guessing what is in force. With every run
stating its own params, no default has to be agreed on anywhere above the backend.

## GlyphCoverage

The one pixel transparent border is what makes the compositor's inner loop branch free: a
bilinear tap at any position inside the glyph reads a 2x2 block that is always in bounds, so the
per pixel range checks disappear.

## ShadowPainter

A form's own design follows the focus its window holds - the colour is the window's shadow colour
at that focus - so each step of a focus change bakes its painter again, and so does each step for
the shadows its content casts in that colour.

A form holds its own painter around its window, which the platform reads. Its margins are
painted a strip at a time with the strip as the clip - by the form around its rectangle, or by a
shadow window around the window it shadows.

2026-10-19: a one-line tooltip (713x56, shorter than two corner reaches) was cast directly, and
each of the shadow window's four margin strips cast the whole of it: 6 ms per fill, 15 ms cold,
against 2 ms for a 477 px tooltip drawn from tiles. Measured with logWindowSteps while chasing the
tooltip flash - the fill stood on the chain between the window's move and its shadow. The strips
by short side came out of that: one bake per distinct height, then tile blits.

## GlyphCompositor

Accumulating every glyph into one mask before compositing is not an optimisation: N
separate blends over one pixel do not equal one blend at the combined coverage - two at 50%
give 75%, not 100% - and glyphs do overlap under italics, tight kerning and diacritics, so
per glyph blending would leave seams.

## TextRasterizationParams

snapOrigins, gridFit and subpixel antialiasing are one group rather than three independent
settings - each ties a glyph's raster to where the run sits - and they are the reason the struct
is passed whole.

## ArcEllipse

Both backends take the grown radii from the same conversion: the CPU backend cuts the arc into
cubics of at most a quarter turn each, within 0.03 percent of the radius of the ellipse, and
flattens those as any other curve; Direct2D is handed the arc itself, with the grown radii, and
draws it.

## RadialGradient

The CPU backend takes each pixel back through the brush's transform into the brush's own
coordinates and measures it from the origin in radii. There the ellipse is the unit circle, the
origin is the point f = offset / radii inside it, and a pixel at d from the origin lies on the
unit circle shrunk toward f by its position t:

    |f + d / t| = 1,  so  t = (f.d + sqrt((f.d)^2 + (1 - |f|^2) |d|^2)) / (1 - |f|^2)

With the origin at the centre, t is |d|, the distance in radii. The map back from a pixel is
affine, so each pixel costs a multiply and an add per axis, then the root. The whole gradient
goes back through the transform, so it turns, shears and scales with what it fills.

## decodePng

**Decoded by the platform.** The declaration is the core's and the body is the platform layer's,
in an implementation unit of this module the way Transfer::Clipboard's is: WIC on Windows,
through the PNG decoder alone rather than whichever codec claims the bytes, and libpng on
Linux. A build with no platform has no body and nothing that calls one.
