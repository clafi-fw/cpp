# Graphics Types

The author's side of the footnote of the same name.

## GlyphCompositor

Accumulating every glyph into one mask before compositing is not an optimisation: N
separate blends over one pixel do not equal one blend at the combined coverage - two at 50%
give 75%, not 100% - and glyphs do overlap under italics, tight kerning and diacritics, so
per glyph blending would leave seams.

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

DECODED BY THE PLATFORM. The declaration is the core's and the body is the platform layer's,
in an implementation unit of this module the way Transfer::Clipboard's is: WIC on Windows,
through the PNG decoder alone rather than whichever codec claims the bytes, and libpng on
Linux. A build with no platform has no body and nothing that calls one.
