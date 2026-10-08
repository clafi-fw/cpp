# Icons

The author's side of the footnote of the same name.

## Magnifier

It came off Slider::paintButtonMark, where it was staged into a pixel buffer and drawn with
the func painters. That is why it never followed the canvas transform: the lens, its ring,
the tail and the mark inside were four independent centres, radii and line widths, and each
of them would have had to be mapped by hand. Built from canvas primitives instead, it scales
with its control like every other icon, and it can be drawn anywhere a rect can be named -
which is what lets it ride on another icon as a corner badge.
