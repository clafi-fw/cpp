# Graphics Types

The words that do not fit above a declaration.

## TextRasterizationMode

- **Cached** reuses one texture per glyph - what every still run wants.
- **Outline** fills the glyph's curves as geometry and bypasses the glyph cache. Its plain
  antialiasing varies smoothly with position.

Compiled in while CLAFI_TEXT_MOVING is 1 - see TextRenderMode.

## PathRenderMode::OuterGlow

The shape drawn over the glow covers its inward half.

**Direct2D draws the ramp as a blur** of the outline widened to where the falloff stands at half. It
falls along a bell rather than the falloff's curve: a little fainter at the outline, and fading out
a little past the reach.

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

**A glyph with no ink** - a space - is still worth caching, so it is analyzed once.

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
  rectangle of that length. One too small on both axes is cast directly, its color's alpha scaled
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

## Canvas drawPixelViews

**Where two views meet, the later one lies over the earlier**, exactly as the same views handed to
drawPixelView one at a time would lie. Each is read premultiplied, as drawBitmap reads a bitmap,
and `opacity` goes on each alike.

**Hand the views over in one call.** The CPU backend lays them in on all the cores at once, a strip
of rows to a thread, and on two cores that nearly halves the time. Direct2D lays them in one at a
time.

## Canvas fillEdgeIndicator

**The band follows the outline along one side and round its two corners.** It is thickness deep
along the side and only thins from there. Round each corner its inner edge is one arc. The arc
leaves the inner line along it and meets the neighboring side along it, where the band ends in a
point.

**The arc's radius is the corner's, or the thickness where that is larger.** With the corner's
radius the arc is the corner's own arc moved in by the thickness, and the band reaches one
thickness past the corner's arc. A larger arc would swell the band past its thickness round the
corner. A square corner takes the thickness, so the band still wraps it.

**Draw it after the outline.** The band starts at the shape's outer edge, so drawn last it covers
the outline along that side, as a tab's indicator covers the tab line.

**Round each corner the color turns to tipColor.** Each corner grades from where the band leaves
the side to its tip, and the tip stands in tipColor. Passed the outline's own color, the band runs
out into the outline; passed its own color at no opacity, it fades out. Both corners take their
gradient's length from the larger arc, so a corner tighter than the other stops a little short of
tipColor at its tip.

**As the thickness falls the arcs close in on the corners**, so a band whose thickness is
animated thins away whole.

- lengthFactor is the share of the side the band covers, centered on it, its ends cut square
  across the side.
- At 1 the band runs from one neighboring side to the other.
- A lengthFactor or a thickness of 0 draws nothing.

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
- Subpixel antialias colors an edge by the third of a pixel it covers.

**A run that can move turns all three off** for as long as it can move. Changing any of them
between two draws of an unmoved run changes how it looks.

Compiled in while CLAFI_TEXT_MOVING is 1 - see TextRenderMode.

## PathRenderMode::Shadow

What an opaque shape casts. The shape's own area is solid, so the shadow is whole wherever the
shape is then drawn.

**Direct2D draws it as a blur** of the shape grown to where the falloff stands at half. The shape's
edge then stands a little under full strength, and the ramp fades out a little past the reach.

## ArcEllipse

**A path holds the endpoint form alone.** arcEllipse works this center form out each time the arc
is drawn, by the conversion in SVG's implementation notes.

## FillRule

**What a fill counts to decide a pixel is inside.** SVG's names, and SVG's initial value.

- Nonzero sums the crossings' directions either side of the pixel: a figure inside another fills
  unless it runs the other way round, and one that does cuts a hole. Every path starts here, as
  SVG's initial fill-rule does.
- EvenOdd counts the crossings alone: a figure inside another is a hole whichever way either runs.

**The rule is the path's**, as SVG's fill-rule is a property of the path element: a fill and a clip
take it, and a stroke and a glow ignore it. Direct2D's geometry sink starts at alternate and is told
the rule; the CPU rasterizer sums each crossing's direction for Nonzero.

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

## PixelPath fillRule

**The path carries the rule every fill of it follows**, Nonzero until set with setFillRule. clear()
returns it to Nonzero, a cleared path being a new path.

- A hole made by a figure that runs the same way round as the shape it cuts asks for EvenOdd - the
  gear's rim and the escape wheel are the framework's two.
- Figures that only join are left at Nonzero: overlapping shapes in one ScatteredShapes batch, the
  crowns of a wood.

## RadialGradient

**Position 0 is the origin, `center + offset`, and 1 is the ellipse** of `radiusX` and `radiusY`
round `center`.

- Each line from the origin to the ellipse runs through the stops evenly.
- Past the ellipse the last stop holds.

This is Direct2D's definition, and the CPU backend computes the same.

**Keep the origin inside the ellipse and the radii above zero.** Outside that the color is
undefined on both backends, though the CPU keeps every pixel on the ramp.

## IBackend

**Every `draw*` lays its stroke inside the bounds it is given**, so a fill and a stroke over the
same bounds meet exactly.

**The backend insets, the caller does not.** Direct2D and any stroked path center the stroke on
the geometry, so a backend built on one insets by half the stroke width. A caller that insets as
well leaves half a pixel of surface outside the border.

## IBackend drawPixelViews

**An override gives what drawPixelView gives view by view**, in the order the views come. The
default lays them in that way, one at a time.

## IBackend drawImagePart

**The part is in the surface's own pixels**, whatever transform the caller carries, as drawImage
lays a whole image, and it lands texel for texel: `offset` moves it by whole pixels.

**It is laid over what is there**, read premultiplied and scaled by opacity, as drawPixelView lays a
view - where drawImage mixes a whole image in. A path clip cuts it on both backends.

**Draw from an image outside any push of it.** An image is not read while it is being drawn into.

## IBackend clearImage

**Clear an image outside any push of it, or while it is the one being drawn into.** Direct2D clears
a target only inside a draw of its own, so an image pushed beneath another cannot be cleared.

## CpuBackend drawPixelViews

**Each strip of 32 rows is laid in by one thread**, from every view in the order given, so the order
holds pixel by pixel and no two threads write the same pixel. Only the rows some view reaches inside
the clip are visited, and the strips are shared out by spreadOverCores.

**A path clip cuts the views at its rectangle**, as it cuts drawPixelView's.

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

**A rect is filled as a path of its outline would be.** Every brush goes through the path clip
when there is one, and an edge that falls between pixels leaves the pixels it cuts partly
covered.

## CirclePainter

The CPU backend folds the canvas transform into pivots and radii.

**brushTransform places the brush alone** - brush coordinates stay in the caller's space. Applied
to the geometry as well, it would move every circle twice.

## decodeDib

**What is read:**

- An info header of 40 bytes or more, V4 and V5 included, or the 12-byte core header.
- 1, 4 and 8 bits a pixel through the color table.
- 16 and 32 bits through masks - the block's own under BI_BITFIELDS or BI_ALPHABITFIELDS,
  5-5-5 and 8-8-8 under BI_RGB.
- 24 bits as the bytes they are.
- Rows bottom up, or top down where the height is negative.

**Alpha comes only from a stated alpha mask.** Under BI_RGB a 32-bit pixel is opaque whatever its
fourth byte holds - the byte is reserved, and a synthesised DIB leaves whatever was in it.

**A compressed block answers nothing** - RLE, or a JPEG or PNG inside a DIB - and so does a header
past the size a bitmap can be. A reader that wants to show something shows the bytes.

## decodePng

Reads any depth, color type and interlace. A picture larger than decodeDib accepts answers
nothing as well.

## encodePng

Writes a bitmap as a PNG of the same size, its pixels straight alpha as `Bitmap` holds them, and
answers the file's bytes: the picture is encoded over memory rather than to a path, so the caller
writes the file. A bitmap past the size decodePng accepts answers nothing as well.
