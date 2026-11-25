# Icons

The author's side of the footnote of the same name.

## Magnifier

It came off Slider::paintButtonMark, where it was staged into a pixel buffer and drawn with
the func painters. That is why it never followed the canvas transform: the lens, its ring,
the tail and the mark inside were four independent centers, radii and line widths, and each
of them would have had to be mapped by hand. Built from canvas primitives instead, it scales
with its control like every other icon, and it can be drawn anywhere a rect can be named -
which is what lets it ride on another icon as a corner badge.

## GlyphSlot

The stroke is proportional to the badge rather than the Thin stroke the other icons draw with.
The fill is what carries a badge's shape, so a thin line laid over it thins away to a thread as
the badge grows. It never goes below that Thin stroke.

A dot is drawn wider than the strokes around it so that it reads as a mark of its own rather
than as the end of one.

The color is an ink at elevation 0, which lies flush with the surface and carries the same
disabled fade the fill does.

## drawTriangle

A triangle is what says warning where a theme's hues are not the ones a reader expects, which
is why the shape carries the meaning rather than the color alone.

Its corners are rounded: a fill carries its corners at full strength, and a sharp apex at
sixteen pixels is one aliased spike.

## PropertiesIcon

The Documents home page's Properties button wore `GearIcon` while no icon stood for the command.
The gear is the Settings mark, so the app menu's Settings page and a storage's Properties popup
shared one face inside one application.

The first answer was a sheet with two fields inside it. Oj dropped the frame on 2026-11-25: the
outline spent the room a row could have stood in, and the page it made read as a document rather
than as the fields. The icon is now three rows alone, each key and value a filled rounded
rectangle, and they take the whole square the sheet was using.

Four shapes were tried first, each at 16, 18, 24, 32 and 64 design units on light and dark: a page
with a heading rule (read as a document), a page with a filled title bar (read as a window, which
is `DialogIcon`'s meaning in this framework), a page of three rows (the frame closed them into a
block of text at 18), and sliders (read as Settings, the mark the change exists to leave alone).
The mocks are in `Agent/sessions/Properties Icon/`.

What the small size decided: rows 0.14 of the square tall with 0.30 between their centers, so the
gap between two of them stays wider than a row is tall at 18; and a key about half the length of
its value, the two separated by two pixels at that size - below that they merge and the row reads
as one thick line.

## SlidersIcon

The mark began as the Options row icon of SeeDocs, drawn on the RowIcons grid: three tracks, their
knobs in the accent, each knob's halo clearing its track. Oj asked for it on Themes' Design tab on
2026-11-25, and an application cannot import another application's icon, so the drawing moved into
the framework and both now draw it from there.

The shares are that grid's: tracks from 0.14 to 0.86 of the square, knobs at 0.66, 0.34 and 0.56,
a knob of 0.11 and a halo of 0.16. **The square is not given a stroke back** - no part of the mark
reaches its edge, so shrinking it the way the stroked icons do would have moved every part off the
grid it was drawn on.
