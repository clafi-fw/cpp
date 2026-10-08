# Icons

The author's side of the footnote of the same name.

## Magnifier

It came off Slider::paintButtonMark, where it was staged into a pixel buffer and drawn with
the func painters. That is why it never followed the canvas transform: the lens, its ring,
the tail and the mark inside were four independent centres, radii and line widths, and each
of them would have had to be mapped by hand. Built from canvas primitives instead, it scales
with its control like every other icon, and it can be drawn anywhere a rect can be named -
which is what lets it ride on another icon as a corner badge.

## GlyphSlot

The stroke is proportional to the badge rather than the Thin stroke the other icons draw with.
The fill is what carries a badge's shape, so a thin line laid over it thins away to a thread as
the badge grows. It never goes below that Thin stroke.

A dot is drawn wider than the strokes around it so that it reads as a mark of its own rather
than as the end of one.

The colour is an ink at elevation 0, which lies flush with the surface and carries the same
disabled fade the fill does.

## paintTriangle

A triangle is what says warning where a theme's hues are not the ones a reader expects, which
is why the shape carries the meaning rather than the colour alone.

Its corners are rounded: a fill carries its corners at full strength, and a sharp apex at
sixteen pixels is one aliased spike.
