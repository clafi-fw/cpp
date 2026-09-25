# Graphics Types

The words that no longer fit above a declaration.

## TextRasterizationMode

How a glyph's coverage is computed. Cached rasterization reuses one texture per glyph and is
what every still run wants; Outline fills the glyph's beziers as geometry instead, with plain
antialiasing that varies smoothly with position, and bypasses the cache to do it.

## PathRenderMode::OuterGlow

The ramp both ways from the outline, as far in as out. What covers the inward half is
the shape drawn over it.

## GlowFalloff::Linear

Straight to nothing. What light wants: a lamp does not thin out, it runs out, and the
eye reads the even ramp as a lit region with an edge.

## GlowFalloff::Smooth

Steep at the shape and level by the time it ends. What a shadow wants: an occluder
takes most of the light close in and little of it further out, and the far end has to
be too faint to find or the shadow reads as a border drawn around the shape.

## ScopedTextRaster

Names the raster params one text run wants. Every run states its own, including a run that
wants what the target already carries - deciding that costs nothing is the backend's job,
since it is the only thing that knows what is installed. A run that stayed silent would leave
it guessing, and no default has to be agreed on anywhere above the backend.

## ScanLines

The lines of whole pixels a view may address, and how they sit in memory. This is the
allocation rather than the geometry: bounds are floats, and a buffer of whole pixels
placed at a fractional origin spans ceil - floor, one pixel more than it holds on that
axis. Every read and every write is bounded by these counts, never by bounds.

## ScopedCanvasOpacity

Holds an opacity layer open for as long as it is alive. A full opacity costs nothing and
opens no layer, so a caller with a factor that may reach 1 states it here rather than
branching around the whole passage it guards.

## GlyphCoverage

Coverage for one glyph, rasterized at a whole pixel origin. left and top are the bearings:
where the ink stands relative to the glyph origin, in whole pixels, y growing downwards. The
pen position is applied when the coverage is placed, which is what lets a single coverage
serve every occurrence of that glyph in every string.

Held as finished coverage in the range 0..1, whatever the rasterizer produced it from. A
platform that averages subpixel channels does so once here rather than once per pixel per
draw.

A one pixel transparent border on every side is what makes the compositor's inner loop
branch free: a bilinear tap at any position inside the glyph reads a 2x2 block that is
always in bounds, so the per pixel range checks disappear. row() addresses the ink inside
that border.

A glyph that rasterizes to nothing - a space - is a coverage with no ink. It is still worth
caching, so that it is analysed once rather than on every run that contains one.

## PlacedGlyph

One glyph's coverage and where its ink stands on the surface. The pointer is a view: the
coverage must not move between place() and composite(). A cache holding every glyph of a
face in storage sized once, at the face's glyph count, guarantees that; a cache that grows
to admit a higher index moves what earlier placements point at.

## ShadowPainter

A window's shadow as nine tiles: four corners, four edges, and nothing in the middle, since
what casts the shadow covers it. The tiles are baked once per Design - the ShadowParams and
the corner radius, in real pixels - by casting the shadow around a small rectangle on a CPU
canvas and cutting it up, so the look is castShadow's own to the pixel. An edge tile is a
fixed length and an edge is as many of it as fit, the last cut by the clip, so nothing in
the cache is ever as long as a window. A corner tile reaches into the rectangle past the arc
and past the ramp's inward half, which is where the shadow along an edge stops depending on
the corner. The window covers its rectangle, so no caller asks for the inward part but a
corner square.

paint draws around a rectangle inside the canvas's clip and nowhere else, and reads the clip
to skip whole tiles: a form painting one corner square costs one tile, and the margins are
painted a strip at a time with the strip as the clip - by a form around its rectangle, by a
shadow window around the window it shadows. A rectangle too small for its corner tiles to
stand apart is cast directly - two corner tiles overlapping would each deny the other's arc -
which is what tooltips and short menus get.

The cache holds no state of the window: the active and inactive looks differ by a constant
alpha, and both windows apply that when they present.

## GlyphCompositor

Draws one run of glyphs through a single coverage mask.

Accumulating every glyph into one mask before compositing is not an optimisation: N
separate blends over one pixel do not equal one blend at the combined coverage - two at 50%
give 75%, not 100% - and glyphs do overlap under italics, tight kerning and diacritics, so
per glyph blending would leave seams.

Coverage is stored at a whole pixel origin, so the fraction of a pen position is resolved by
sampling rather than dropped - dropping it is what makes a moving run stand still and then
jump a whole pixel. A glyph placed on whole pixels takes an exact branch that adds the
coverage as it is.

## TextRasterizationParams

Everything about rasterizing a run of text that a backend has to be told, and nothing about
why it was asked for. Callers above decide what a control's text needs; this says only what
the result is.

The three flags below are one group rather than three independent settings, and they are the
reason this struct is passed whole. Each of them makes a glyph's raster depend on where the
run happens to sit: snapping moves the origin to a whole pixel, grid fitting pulls the glyph's
own stems and baseline onto pixel edges, and subpixel antialiasing gives an edge a colour that
depends on which third of a pixel it covers. Any of them changing between two draws of the
same unmoved run changes how it looks. A caller that wants a run to survive being moved has to
turn off all three, and has to leave them off for as long as that run can move.

## PathRenderMode::Shadow

The shape's own coverage, solid, and the ramp outward from it: what an opaque shape
casts, wherever the shape itself then stands.

## IBackend

Stroke alignment: every draw* here lays its stroke INSIDE the bounds it is given. The
outer edge of the stroke is the edge of the shape, so a fill and a stroke over the same
bounds meet exactly and no fill shows past the border.

That is not what the underlying APIs do - Direct2D strokes centred on the geometry, and so
does any stroked path - so a backend built on one insets by half the stroke width itself.
A caller must not pre-inset; doing both is what left a half-pixel of surface outside
every control's border on the CPU backend.

## ScopedCanvasTransform

Composes a transform onto whatever the canvas already has, for as long as it is alive, then
puts the previous one back. An identity matrix costs nothing and touches no state.

## ScopedCanvasImage

Draws into the kept image at this index for as long as it is alive, and puts the surface
back when it dies.

## RoundedRectPainter

The parts reaching this painter are already in buffer coordinates, bounds and radius alike -
the CPU backend folds the canvas transform into them before calling in, so that the joint
snapping below lands on whole device pixels. The matrix travels on to the rect and circle
painters only to place brush coordinates, which are still stated in the caller's space.

## RectPainter

The rects reaching these methods are already in buffer coordinates - the CPU backend folds
the canvas transform into the geometry before it calls in, because the span rasterizer below
cannot express anything but axis-aligned edges. The matrix is passed along only so that brush
coordinates, which are still stated in the caller's space, can be placed against the mapped
geometry. Applying it to the geometry here as well would move every shape twice.

## CirclePainter

Pivots and radii reaching this painter are already in buffer coordinates - the CPU backend
folds the canvas transform into them before calling in. The matrix is passed along only to
place brush coordinates, which are still stated in the caller's space. Applying it to the
geometry here as well would move every circle twice.

## decodeDib

A device-independent bitmap read into pixels: the block a Windows clipboard carries as
CF_DIB or CF_DIBV5, and the same block behind the file header a .bmp starts with, which is
what image/bmp carries and what decodeBmpFile takes.

WHAT IS READ. An info header of 40 bytes or more, the V4 and V5 headers included, or the
12-byte core header. 1, 4 and 8 bits a pixel through the colour table, 16 and 32 through
masks - the header's own with BI_BITFIELDS, 5-5-5 and 8-8-8 without - and 24 as the bytes
they are. Rows bottom up unless the height is negative.

WHAT ALPHA MEANS. A 32-bit pixel under BI_RGB is opaque whatever its fourth byte holds:
the byte is reserved there, and a synthesised DIB leaves whatever was in it. Only an alpha
mask stated with BI_BITFIELDS makes a pixel translucent.

NOTHING FOR WHAT IS NOT PIXELS. A compressed block - RLE, or a JPEG or PNG carried inside a
DIB - and a header past the size a bitmap can be answer nothing rather than a guess: a
reader that wants to show something shows the bytes.

## decodePng

A PNG file read into pixels, straight alpha, whatever its depth, colour type or interlace;
nothing for bytes that are not a PNG or for a picture past the size decodeDib holds to.

DECODED BY THE PLATFORM. The declaration is the core's and the body is the platform layer's,
in an implementation unit of this module the way Transfer::Clipboard's is: WIC on Windows,
through the PNG decoder alone rather than whichever codec claims the bytes, and libpng on
Linux. A build with no platform has no body and nothing that calls one.
