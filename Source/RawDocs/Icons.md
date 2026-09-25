# Icons

The words that no longer fit above a declaration.

## Lens

A magnifier - the lens and the tail hanging off its lower right.

It came off Slider::paintButtonMark, where it was staged into a pixel buffer and drawn with
the func painters. That is why it never followed the canvas transform: the lens, its ring,
the tail and the mark inside were four independent centres, radii and line widths, and each
of them would have had to be mapped by hand. Built from canvas primitives instead, it scales
with its control like every other icon, and it can be drawn anywhere a rect can be named -
which is what lets it ride on another icon as a corner badge.

## ChannelForm

Which shape the sweep is drawn as. Ring and pie are two readings of the same data, and the
choice is a look for the whole set rather than a property of any one channel - so it is one
switch here, not three call sites that could drift apart.

## SweepEnds::Stepped

The channel runs from one end to the other. The seam is left as a hard edge so the two
ends meet without blending - that step is what the icon is showing.

## GlyphSlot

Where a glyph goes and what it is drawn with, handed back by whichever container was
painted. The box is the badge's own square, so a glyph is written as fractions of the whole
badge and one set of numbers reads the same under the disc and under the triangle.

## Gap

A band of theme surface carried all the way round the outside, so that the glass reads as
lying on top of whatever is already drawn there rather than tangled in it. The same trick the
arrow in OpenInExplorerIcon uses - the shape stroked once wide in the surface colour, then
again properly - and the reason an overlay needs it: without the band the ring and the
picture's own outlines meet and neither is legible. It costs the lens some size, because the
band is taken out of the rect rather than added outside it.

Off by default: an icon drawn on its own control has no picture underneath to be separated
from, and the band would show there as a pale halo on the control's surface.

## WedgeJoint

Whether the wedges of a pie meet, or stop short and let the backdrop through between them.
Applies to the pie only: gapping a ring would cut its band into dashes.

## WedgeJoint::Spoked

Wedges stop short of their boundaries, leaving the surface behind the icon showing as
spokes. They cost nothing to theme - being the backdrop, they follow it - and they give
the disc a hub without one having to be drawn.

## SweepEnds::Cyclic

The channel wraps: the colour at the seam is the same from both sides, so segments run
across it and the join disappears. Hue is the only one of the three.
