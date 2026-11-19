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
lies too far from the line to change. Bot Talent's floor - 37 planks two units wide, fanning
toward a point above the stage - spanned 0.6 of the frame in plank boxes. A Visual Studio profile
of the MSVC build at 3436 x 2051 put 72% of an 81 ms frame in this function and in
calcStrokeEdgeDistSq, which MSVC keeps out of line; the same frame under clang spent 12% there.

The masks it writes are the same bit for bit - checked over every scene and the mill's themes -
and on Bot Talent the work counted by callgrind fell to a sixth.

**rasterizeGlowMask narrows its rows the same way** (2026-11-08): a pixel the radius or more from
the line takes no glow, and the mask keeps the greater of the two. The Satellite's soft clouds - a
Shadow layer round each lump, the reach 20 units - cost 6.8 ms a frame at 3436 x 2051 with whole
boxes; with narrowed rows and the companion lumps added, about 4. The masks are the same bit for
bit.

## RasterBuffers rasterizeFill

**Only the groups of eight pixels near an edge work out the distance field** (2026-11-08). Every
pixel of a fill's box used to measure its distance to each edge active on its row. Further than
half a pixel and the outset from every edge the alpha clamps to the winding's 0 or 1, so a row now
marks the groups any edge comes within reach of - the edge's run over the rows that far either
side, widened by the same, a pixel more against rounding - and the rest take the winding alone. A
large shape's inside costs a store. The Satellite's globe clip, a disc the frame's width, fell from
3.5 to 2 ms, its coastlines and clouds from 6 to 4; every frame came out the same bit for bit.

## RasterBuffers rasterizeStrokeMask

**A stroke clears and writes each row of the mask only over the columns it reaches** (2026-11-08).
The mask used to be cleared over the stroke's whole box, and the composite after it read all of it.
The Satellite's atmosphere is three rings the frame's width across and some 700 rows deep, the ring
itself a narrow band of that: three box-sized clears and three box-sized reads a frame. Each row
now starts with an empty run; a segment reaching the row clears only the columns it adds to the run,
the gap between two runs included, so the run stays one span. compositeSolid, compositeRamp and
compositePointGlow read a row of their own mask only over its run, and prepareForEffect drops the
runs, so any other mask is read whole as before. The atmosphere fell from 6.4 to 3 ms; every frame
came out the same bit for bit.

## RasterBuffers rasterizeGlowMask

**A glow on its own keeps runs, as a stroke does** (2026-11-08). rasterizeGlowMask opens an empty
run for each row and hands over to rasterizeGlowOver, which claims a row's run before writing it.
The Shadow layer, whose fill writes the whole mask first, calls rasterizeGlowOver directly and keeps
no runs. The Satellite's atmosphere as three OuterGlow rings, each the frame's width across, cost
7.8 ms a frame at 3436 x 2051 with the mask cleared and read whole, and 3.2 with runs - what the
three strokes before them cost. Every frame of the scenes and the mill came out the same bit for
bit.

## RasterBuffers compositeRamp

**Eight pixels the path and the clip leave uncovered are passed over before the gradient is
looked up** (2026-11-08). The lookup is a gather; the blend after it was skipped already, the
gather was not. In Oj's MSVC profile of the Satellite at 3436 x 2051, compositeRamp was a third of
the frame, most of it inside the boxes of the atmosphere's three rings - each the frame's width by
some 700 rows, the ring a fraction of that. There the atmosphere fell from 10.5 to 6.4 ms in the
harness, and every frame of the scenes and the mill came out the same bit for bit.

## RasterBuffers compositeRampRows

**A gradient whose axis runs straight down is resolved once a row** (2026-11-18). compositeRamp
computes a pixel's place along the axis, clamps it, scales it by 256 and gathers the ramp, for
every eight pixels. On a background - a sky, a field, a wall shaded from top to bottom - that
place is the row's alone, so the three products, the conversion and the gather were paid for an
answer that did not change along the row. compositeRampRows clamps and steps once a scanline and
carries the colour in a register. A row neither the mask nor the clip reaches, in a colour already
opaque, is written whole, with no destination read at all.

The two paths agree to the bit. The row's position is the general path's own instructions: with
the axis's x zero, `(vRx * vDx + vRy * vDy) * vInvLenSq` is `((py - startY) * axis.y) *
invLenSq`, and the clamp, the 256 step and the round are its own intrinsics. The colour a row
mixes toward is opaque whatever the ramp states - the colour compositeRamp blends toward - and the
fast path that stores a colour at 255 whole is not asked for, because compositeRamp blends that
one by its weight.

Chase at 3840 x 1984 on the Linux CPU backend, a Release clang build: draw fell from 41.5 to 25.2
ms a frame, the sky alone from 13.7 to 3.9, and the meadow from 3.5 to 0.9. Eleven frames drawn at
that size - the mill, Chase in Clear Day, Twilight, Stormy Night and Sahara Heat, Bot Talent,
Ghost, Clockwork, Comet, Earth and Orrery - came out the same bit for bit.

## streamFillPixels

**A fill larger than the cache would hold is written past the cache** (2026-11-18). A store into a
line the cache does not hold reads the line in first, so a full-surface fill passed over a
frame's worth of memory twice: once to bring the lines in and once to write them. streamFillPixels
writes 32 bytes at a time with `_mm256_stream_si256`, so no line is read, and the caller fences
before anything reads the pixels back. RectPainter's snapped opaque route takes it for a fill of 2
MB or more; compositeRampRows' covered rows take it over a row of 2048 pixels.

At 3840 x 1984 the sky fell from 3.9 to 1.4 ms and the meadow from 0.9 to 0.7. The form's own
background, the fill under a scene that covers it, fell from 4.0 to 2.2 at 3840 x 2068 - which is a
cost worth knowing about: the scene redraws every pixel of it, and nothing reads what the fill
leaves. The frames of the same matrix came out the same bit for bit.

## PixelView compositeSurface

**Eight pixels at a time** (2026-11-08). Taken one at a time, the pixels of a scene's kept layer
cost about 13 instructions each under callgrind - a third of a layered Bot Talent frame. Copying a
run of opaque pixels whole saved only the opaque ones: a translucent pixel, or a clear one inside
the layer's cut, still went one at a time, and a translucent or sparse part cost more kept than
drawn live. Eight clear pixels now cost a load and a test, eight opaque ones at full opacity a load,
a compare and a store, and a mixed eight is blended in 16-bit lanes. The pixels left at the end of
a row still take the one pixel route.

**The lanes are exact.** The division by 255 is (t * 0x8081) >> 23, equal to t / 255 for every t
up to 65535, and the rounded product reaches 65152. Four million random groups of eight, and every
source channel against every alpha at five opacities, came out equal to the one pixel route, and the
showcase scenes and the mill render bit for bit as before.

## CpuBackend drawPixelViews

**The bitmaps a scene keeps were laid in by the paint thread alone** (2026-11-08). In Oj's Visual
Studio profile of the Satellite at 3436 x 2051 that was the largest share of the paint thread's
frame: 3.0 ms for the space-and-ocean layer and 2.1 for the cloud bands, beside 2.8 for gradients,
2.7 for the present and 2.4 each for the atmosphere and the coastlines.

**A thread takes a strip of rows, not a view.** Threads on views would race where two views overlap,
and lose the order a later view needs. A strip takes every view over it in turn, and
compositeSurface reads its source and writes the target's rows and nothing else, so the result is
the one-at-a-time one bit for bit: the Satellite at two times of its cycle came out the same as
with the views laid in one by one.

**32 rows a strip**, the height of SceneLayer's and the clouds' bands: a 2051-row frame gives 65
strips to share out, and each is far more work than handing it to a thread. Two layers of 32-row
bands over the whole frame, one opaque and one translucent, took 3.4 ms against 6.0 one by one at
3436 x 2051 on the container's 2 cores, and 0.7 against 1.2 at 1600 x 900.

**spreadOverCores moved to ClaFi.Core.System.Utils** for it, out of PathArt's SceneHelpers.

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

## The fill rule follows SVG

**A fill was even-odd in both backends** (until 2026-11-17): the CPU rasterizer XORed each row's
crossings, and Direct2D's geometry sink starts at alternate. SVG's initial fill-rule is nonzero, and
this painter's paths follow SVG's path data throughout, so the two disagreed wherever figures
overlap. Oj: "Don't you think that rather should be fixed in the path painter? That is not how svg
draws overlapping figures, and this framework path painter claims to be svg-compliant".

**The rule is the path's**, as SVG's fill-rule is a property of the path element: PixelPath carries
a FillRule, Nonzero from construction, and clear() returns it to Nonzero, a cleared path being a new
one. A fill and a clip read it; a stroke and a glow ignore it.

- The CPU rasterizer adds each crossing's direction - +1 one way and -1 the other - and fills
  wherever the sum is not zero. EvenOdd keeps the XOR route. Both are the same AVX2 row loop: a
  crossing's direction is broadcast beside its x in the pre-pass.
- Direct2D's sink is told D2D1_FILL_MODE_WINDING, its own default being alternate.
- ScatteredShapes needed no change. On Direct2D it gathers its shapes into one path, which now joins
  them where they overlap instead of cutting holes; the footnote that told callers the shapes must
  stand apart is rewritten to say they may lie over one another.
- Two holes are meant and their inner figure runs the same way round as the shape: the gear's rim
  and the escape wheel. Both set FillRule::EvenOdd on their path. Every other multi-figure fill in
  the tree either joins - the crowns of a wood, the land outlines with the Caspian wound the other
  way - or was one figure already.

Rendered with the CPU backend at 1280 x 720: Clockwork's gears and escape wheel keep their rims,
Earth keeps the Caspian, and the other scenes read as before. The Direct2D route is built and read
but not seen - no headless Direct2D frame exists here - so its fill mode is a line of code, not a
rendered check.
