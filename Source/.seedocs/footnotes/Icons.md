# Icons

The words that do not fit above a declaration.

## ChannelForm

**One switch for the whole set.** Ring and pie show the same data, so the form is a look for all
three channel icons rather than for one. k_channelForm sets it, and drawChannelSweep reads it for
each.

## SweepEnds::Stepped

**The seam stays a hard edge**, so the two ends meet without blending. That step is what the icon
shows.

## GlyphSlot

paintDisc and paintTriangle hand one back once the fill is drawn.

**The box is the badge's square, not the shape's**, so one set of fractions places a glyph the same
on the disc and on the triangle.

## Halo

**For a magnifier laid over another icon.** The band keeps the ring apart from the picture's own
outlines, which would otherwise meet and leave neither legible.

- The band comes out of the rect, so the lens shrinks to make room.
- Halo::No is the default. On its own control the glass has nothing beneath it, and the band would
  show as a pale ring on the control's surface.

## WedgeJoint

**The pie only.** A ring is always drawn sealed - gaps would cut its band into dashes.

## WedgeJoint::Spoked

**The spokes follow the theme on their own** - they are the backdrop showing through. They also
give the disc a hub without one being drawn.

## SweepEnds::Cyclic

Segments run across the seam, so the join disappears. Hue is the only cyclic channel of the three.
