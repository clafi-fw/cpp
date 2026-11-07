# PathArt

The author's side of the footnote of the same name.

## Scene

Scene was cut out of MillScene when the showcase gained more pictures than the mill: MillScene
derives from it, named Mill, and the seven others come from createScenes(). The showcase holds the
mill as a member, since its theme picker drives it, and the others in a list after it.

The theme picker belongs to the mill alone. The Scene Theme section of the Scene page is shown
only while the mill is picked; a hover preview over the Scene group shows the scene under the
pointer and puts the picked one back when the pointer leaves. The config keeps the scene by name
in `Scene` and the theme in `SceneTheme`, so a scene added or moved in the list does not shift
what was stored.

**Each scene states its own cycle.** All of them run at 40 seconds except OrreryScene and
SatelliteScene, which run at 240: the first mockups ran both at 40, and the planets and the turning
Earth were far too quick. Slowing the whole cycle slowed the twinkling and blinking with it, so
those two multiply their quick details by `k_lively`, 6, to keep the 40-second pace. Changing a
scene's speed is its `k_cycle` alone - every motion is counted per cycle, so a new length changes
the pace and never the loop.

## Scene draw

Every scene was first built as an animated HTML page on a 2D canvas, reviewed, and then ported.
The C++ follows the mockups shape for shape, and the CPU backend's frames were compared against
them by eye.

**The fill rule is even-odd**, on the CPU rasterizer and in Direct2D's default alike, so shapes
that overlap inside one path cut holes in each other. Shapes that can overlap are filled one by
one - the cloud puffs over the Earth, the pines on the lake shore, a gear's spokes. Shapes that
stand apart share a path, since a fill per path is what costs: the stars and the city lights,
binned into six opacities with a path each, the asteroids, the petals, the blades of grass. The
rule is also used on purpose, for the hole in a gear's rim and in the escape wheel.

PixelPath::drawEllipse and drawRoundedRect start a path of their own, so addEllipse appends an
ellipse as four cubics.

**Draw times**, CPU backend, clang Release, one thread, 800 x 600 (the Satellite at 800 x 450),
averaged over three frames: Mill 1.5 to 3.4 ms, Puppeteer 5, Ghost 4.6 to 4.9, Clockwork 6.2 to
6.8, Orrery 6.5 to 6.7, Comet 4.3 to 5.7, Satellite 5.4 to 6.8, Chase 4 to 4.7. Not measured
under Direct2D.

## ClockworkScene

meshWith places the driven gear's phase so that the teeth past the line of centers, counted on
each gear, add up to half a tooth: the driver's tooth stands in the driven gear's gap.

The counts close the cycle. The pendulum's 20 swings are 40 crossings, so the 20-tooth escape
wheel turns twice, and the 30-tooth gear on its arbor passes 60 teeth. From there 60 teeth pass
the 12 and the 36, and the 12-tooth pinion on the 36's arbor passes 20 to the 40, the 20 and the
15. Every gear moves a whole number of teeth, and each spoked one a whole number of spoke
spacings: the 36 turns 600 degrees past 6 spokes, the 40 half a turn past 8, the 15 480 degrees
past 3.

## OrreryScene

The belt lies between Mars and Jupiter on screen and in the drawing order: the outer planets on
the far side, the far half of the belt, the inner planets on the far side, the Sun, the inner
planets on the near side, the near half of the belt, the outer planets on the near side.

## OrreryScene shadeBall

The shade is built in a frame turned so the light comes from +x, from two arcs: the half disc on
-x, slightly larger than the ball so its antialiased rim is covered under the clip, and the half
ellipse of the terminator, swept one way or the other by the sign of `toViewer`.

## CometScene

The reflection draws everything above the lake a second time through a mirrored frame -
`Translation(0, 2 * lakeY) * Scale(1, -1)` - under a clip to the lake and a 0.42 canvas opacity,
which roughly doubles the sky's cost. The afterglow is drawn between the far ridge and the near
one, so the near ridge hides the landing itself.

## SatelliteScene

The globe is an orthographic projection, looked at from latitude -20 with its north turned 0.22
radians off the vertical; its radius is 1.15 times the larger side of the frame, so only a cap
shows.

**The terminator is a straight line** through its points on two rows of the frame. The first
version drew the night as five fills of different reach to soften the edge, and they banded
visibly. One gradient across a straight line has no bands, and over the small cap in view the
line stands in for the curve.

The coastlines are coarse polygons, given a point every 4 degrees so a long edge bends with the
globe. The towns are scattered inside each inhabited polygon once, in the constructor, about one to
26 square degrees of the polygon's box and none above latitude 62. A shape is drawn only when some
of it is on the near side and what is left of it reaches the frame.

**A shape across the rim is clipped there**, by Globe::nearSideOf: the outline is cut into the
stretches in view, and each is joined along the rim to the next stretch that comes round, the way
the outline turns. A coastline knows its turn from the shoelace sum of its corners, in longitude
and latitude, which turn as the Earth seen from outside does. A shape can come out as several
loops, one subpath each. The first version moved each hidden point onto the rim at its own angle
and joined them straight. Oj's video of 2026-11-07 showed a vast straight-edged wedge sweeping over
the globe: while Eurasia's north coast passed behind the point opposite the viewer, its points
jumped round the rim, and the straight joins cut across the face. Joining each stretch to the
next one along its own outline, instead of along the rim, filled the whole disc whenever a coast
dipped behind twice.

**The joins rely on every coastline being a simple polygon.** A coast that crosses itself turns
the other way inside the crossing, and when that part dips behind, the join runs the long way round
and fills the disc outside the shape. Japan did, at Hokkaido, until its two northern corners were
swapped.

**In the shadow the craft keeps 35 percent of its daylight colour**, and the panels' backs carry
the ribs between the cells and a spar along the boom. The panels turn about once a cycle with the
Sun, so through the shadow they show their backs, which were one flat gray (Oj, 2026-11-07, "one
solid gray"). The stars keep 65 percent of their night brightness by day; at 15, as a camera
exposed for daylight would show them, they read as dull. Both were picked from renders of the
options.

## ChaseScene

**The camera stops differently in the two episodes**: over 1.4 seconds after the log, within 0.4
seconds at the branch. With the log's stop in both, the wolf came to rest beyond the tree in the
second episode. Where he lies is measured from the obstacle for the same reason. The camera
stands until 0.82 of the episode and is back at full speed at 0.9.

The mountains and hills are single paths across the frame, their heights sums of waves that each
repeat a whole number of times per period. Drawn tile by tile, the antialiased edges of two tiles
overlapped and showed a seam where they met.

On his back, the wolf's head turned into the ground until `headTurn` took `0.7 * onBack`.

## MillScene

To watch: the wolf, the gator and the cow move with `sin(t * 12)`, `sin(t * 15)`, `sin(t * 30)` and
`sin(t * 50)`, which are not whole turns of a 0 to 1 cycle, so they jump when the cycle wraps.
