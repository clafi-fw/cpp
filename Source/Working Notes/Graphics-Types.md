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

2026-11-05: the corner reach carried the blur as well, on the reading that the ramp straddles the
silhouette's edge. castShadow draws the silhouette solid with the ramp running outward, so a
pixel's shadow follows its distance to the silhouette and the blur takes no part in where an edge
turns straight. Measured on a Themes app screenshot at 250%: above a dialog's top edge the shadow
was the same along the edge from 5-7 units past the window's left edge, against a reach of 38.
Dropping it took a primary window's corner reach from 38 units to 14 and a secondary window's
from 21 to 9, and the depth the tiles paint under the content with it. The spread counts as a
distance: a negative one draws the silhouette in, and the arc ends that much further along.

## GlyphCompositor

Accumulating every glyph into one mask before compositing is not an optimisation: N
separate blends over one pixel do not equal one blend at the combined coverage - two at 50%
give 75%, not 100% - and glyphs do overlap under italics, tight kerning and diacritics, so
per glyph blending would leave seams.

## TextRasterizationParams

snapOrigins, gridFit and subpixel antialiasing are one group rather than three independent
settings - each ties a glyph's raster to where the run sits - and they are the reason the struct
is passed whole.

## RectPainter

A rect filled through RectPainter and the same rect filled as a path came out differently in
three ways, until PathArt's fillBox began filling rects to keep scene frames off the path
rasterizer and showed them:

- **A solid colour ignored a path clip.** Both solid routes write pixels straight, and the clip
  reaches a pixel only through a coverage mask, so Jupiter's bands spilled past the planet. Under
  a path clip a solid rect goes through fillCovered, which builds the mask.
- **A brush other than a solid colour filled its rim pixels whole**, so a gradient rect whose edge
  fell between pixels grew by up to a pixel on that side - a tree trunk in the Chase. The inner
  pixels still take the brush without a mask, and the rim, at most a pixel wide on each side, goes
  through fillCovered.
- **A point glow composited without a mask skipped the path clip**, which the gradients already
  honoured without one, so the city glows spilled past the Earth's limb.

After the three, the scenes match the path-filled frames to within an antialiasing step on a few
corner pixels: RectPainter's exact area coverage against the rasterizer's.

## RasterBuffers rasterizeStrokeSegment

**A row of a slanting segment covers only the run near its line.** The distance used to be
measured for every pixel of the segment's bounding box, and on a slanting segment most of that box
lies too far from the line to change. The Puppeteer's floor - 37 planks two units wide, fanning
toward a point above the stage - spanned 0.6 of the frame in plank boxes. A Visual Studio profile
of the MSVC build at 3436 x 2051 put 72% of an 81 ms frame in this function and in
calcStrokeEdgeDistSq, which MSVC keeps out of line; the same frame under clang spent 12% there.

The masks it writes are the same bit for bit - checked over every scene and the mill's themes -
and on the Puppeteer the work counted by callgrind fell to a sixth.

## RasterBuffers compositeRamp

**Eight pixels the path and the clip leave uncovered are passed over before the gradient is
looked up** (2026-11-08). The lookup is a gather; the blend after it was skipped already, the
gather was not. In Oj's MSVC profile of the Satellite at 3436 x 2051, compositeRamp was a third of
the frame, most of it inside the boxes of the atmosphere's three rings - each the frame's width by
some 700 rows, the ring a fraction of that. There the atmosphere fell from 10.5 to 6.4 ms in the
harness, and every frame of the scenes and the mill came out the same bit for bit.

## PixelView compositeSurface

**Eight pixels at a time** (2026-11-08). Taken one at a time, the pixels of a scene's kept layer
cost about 13 instructions each under callgrind - a third of a layered Puppeteer frame. Copying a
run of opaque pixels whole saved only the opaque ones: a translucent pixel, or a clear one inside
the layer's cut, still went one at a time, and a translucent or sparse part cost more kept than
drawn live. Eight clear pixels now cost a load and a test, eight opaque ones at full opacity a load,
a compare and a store, and a mixed eight is blended in 16-bit lanes. The pixels left at the end of
a row still take the one pixel route.

**The lanes are exact.** The division by 255 is (t * 0x8081) >> 23, equal to t / 255 for every t
up to 65535, and the rounded product reaches 65152. Four million random groups of eight, and every
source channel against every alpha at five opacities, came out equal to the one pixel route, and the
showcase scenes and the mill render bit for bit as before.

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
