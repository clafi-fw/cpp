# Graphics Types

The words that do not fit above a declaration.

## TextRasterizationMode

- **Cached** reuses one texture per glyph - what every still run wants.
- **Outline** fills the glyph's curves as geometry and bypasses the glyph cache. Its plain
  antialiasing varies smoothly with position.

Compiled in while CLAFI_TEXT_MOVING is 1 - see TextRenderMode.

## PathRenderMode::OuterGlow

The shape drawn over the glow covers its inward half.

## GlowFalloff::Linear

**What light wants.** A lamp runs out rather than thinning out, and the eye reads the even ramp
as a lit region with an edge.

## GlowFalloff::Smooth

**What a shadow wants.** An occluder takes most of the light close in and little further out.
The far end must be too faint to find, or the shadow reads as a border drawn round the shape.

## ScopedTextRaster

**Every run states its own params**, even those the target already carries. The backend alone
knows what is installed, so it decides when a change costs nothing.

Compiled in while CLAFI_TEXT_MOVING is 1 - see TextRenderMode.

## ScanLines

**The allocation, not the geometry.** Bounds are floats: at a fractional origin they span
ceil - floor pixels, one more than the buffer holds on that axis.

**Bound every read and write by these counts**, never by the bounds.

## ScopedCanvasOpacity

**A full opacity opens no layer** and costs nothing. A caller whose factor may reach 1 passes it
here rather than branching round the passage it guards.

## GlyphCoverage

**left and top are the bearings** - where the ink sits relative to the glyph origin, in whole
pixels, y down. The pen position is added at placement, so one coverage serves every occurrence
of the glyph.

**The values are finished coverage, 0 to 1.** A platform that averages subpixel channels does it
once, here.

**A clear border one pixel wide surrounds the ink.** allocate sizes and clears it, and row()
addresses the ink inside it.

**A glyph with no ink** - a space - is still worth caching, so it is analysed once.

## PlacedGlyph

**The coverage must stay put between place and composite** - the pointer is a view. A glyph
cache sized once, at the face's glyph count, guarantees it. A cache that grows to admit a higher
index moves what earlier placements point at.

## ShadowPainter

**The middle tile is empty** - what casts the shadow covers it.

**setDesign bakes the tiles from castShadow's own output**, so the look matches it to the pixel.
Each corner may take its own radius.

**paint draws inside the canvas clip only**, and skips whole tiles the clip misses. A form
painting one corner square pays for one tile.

- The tiles go on whole pixels: the rectangle's edges round to the nearest.
- One bake serves every opacity, so a shadow fading in costs one bake.
- A rectangle too short or too narrow for two corner tiles to stand apart takes a strip of three
  tiles baked at that length - the ends and the piece between - and the strip is kept for the next
  rectangle of that length. One too small on both axes is cast directly, its colour's alpha scaled
  by the opacity.
- A bake leaves the CPU rasterizer's clip as it found it, so setDesign is safe in the middle of a
  paint.

**Take a painter from FormBase::shadowPainter**, which keeps one per design: the cards over a
window share one bake, its held headers another.

## Canvas::drawBitmap

**The pixels are read premultiplied** - as a CPU canvas over a clear base leaves them, and as
Direct2D reads a bitmap. ShadowPainter's tiles are baked that way.

- An opaque pixel replaces what is there, and a clear one leaves it.
- Any other is laid over what is there, so on a clear surface - a window's margins - it lands as
  it is.
- Opacity scales all four channels alike.

## GlyphCompositor

**A fractional pen position is sampled, not dropped.** Coverage sits at a whole pixel origin,
and the compositor samples it at the fraction. Dropping the fraction would make a moving run
stand still, then jump a whole pixel.

A glyph placed on whole pixels takes an exact path that adds its coverage as it is.

## TextRasterizationParams

A control states what its text needs as a TextRenderMode. These params are the answer a backend
is given.

**Three of them tie the raster to where the run sits:**

- snapOrigins puts the origin on a whole pixel.
- gridFit pulls the glyph's stems and baseline onto pixel edges.
- Subpixel antialias colours an edge by the third of a pixel it covers.

**A run that can move turns all three off** for as long as it can move. Changing any of them
between two draws of an unmoved run changes how it looks.

Compiled in while CLAFI_TEXT_MOVING is 1 - see TextRenderMode.

## PathRenderMode::Shadow

What an opaque shape casts. The shape's own area is solid, so the shadow is whole wherever the
shape is then drawn.

## ArcEllipse

**A path holds the endpoint form alone.** arcEllipse works this centre form out each time the arc
is drawn, by the conversion in SVG's implementation notes.

## PixelPath arcTo

**A path keeps the arc as stated** - the endpoint form of SVG's A, which an SVG writer would write
back.

- Radii too small to span the ends grow together until they span them exactly. The arc is then
  half the ellipse.
- A radius of zero makes the arc a straight line to its end.
- An arc ending where it starts draws nothing.

**Under a matrix an arc stays an arc.** transform moves its end with the matrix and the ellipse's
axes with the matrix's linear part. A mirroring matrix reverses the turn. The choice of the larger
or smaller arc stays as stated.

## RadialGradient

**Position 0 is the origin, `center + offset`, and 1 is the ellipse** of `radiusX` and `radiusY`
round `center`.

- Each line from the origin to the ellipse runs through the stops evenly.
- Past the ellipse the last stop holds.

This is Direct2D's definition, and the CPU backend computes the same.

**Keep the origin inside the ellipse and the radii above zero.** Outside that the colour is
undefined on both backends, though the CPU keeps every pixel on the ramp.

## IBackend

**Every `draw*` lays its stroke inside the bounds it is given**, so a fill and a stroke over the
same bounds meet exactly.

**The backend insets, the caller does not.** Direct2D and any stroked path centre the stroke on
the geometry, so a backend built on one insets by half the stroke width. A caller that insets as
well leaves half a pixel of surface outside the border.

## ScopedCanvasTransform

**An identity matrix touches no state** and costs nothing, so a caller passes its matrix as it is.

## RoundedRectPainter

The CPU backend folds the canvas transform into bounds and radius, so the joints between arcs and
edges snap to whole device pixels.

**brushTransform places the brush alone** - brush coordinates stay in the caller's space. It goes
on to the rect and circle painters for that only.

## RectPainter

The CPU backend folds the canvas transform into the rects, since the span rasterizer draws
axis-aligned edges only.

**brushTransform places the brush alone** - brush coordinates stay in the caller's space. Applied
to the rect as well, it would move every shape twice.

## CirclePainter

The CPU backend folds the canvas transform into pivots and radii.

**brushTransform places the brush alone** - brush coordinates stay in the caller's space. Applied
to the geometry as well, it would move every circle twice.

## decodeDib

**What is read:**

- An info header of 40 bytes or more, V4 and V5 included, or the 12-byte core header.
- 1, 4 and 8 bits a pixel through the colour table.
- 16 and 32 bits through masks - the block's own under BI_BITFIELDS or BI_ALPHABITFIELDS,
  5-5-5 and 8-8-8 under BI_RGB.
- 24 bits as the bytes they are.
- Rows bottom up, or top down where the height is negative.

**Alpha comes only from a stated alpha mask.** Under BI_RGB a 32-bit pixel is opaque whatever its
fourth byte holds - the byte is reserved, and a synthesised DIB leaves whatever was in it.

**A compressed block answers nothing** - RLE, or a JPEG or PNG inside a DIB - and so does a header
past the size a bitmap can be. A reader that wants to show something shows the bytes.

## decodePng

Reads any depth, colour type and interlace. A picture larger than decodeDib accepts answers
nothing as well.
