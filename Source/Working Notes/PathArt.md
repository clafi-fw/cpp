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

**Scenes, sprites and effects stand in three folders** (Oj, 2026-11-07, "Make folders for scenes,
effects, sprites"). A sprite is a figure with a pose of its own - a struct of where it stands and
how far into its motions it is - drawn by one free function; an effect is a field or an overlay -
stars, rain, fog, steam - drawn the same way from a struct of what it covers. What is drawn once,
in one scene, stays in its scene: the stage, the graveyard, the plate, the lake, the globe, the
landscapes. Module names keep the kind as a suffix and leave the folder out, as Icons does:
`ClaFi.PathArt.WolfSprite` in `Sprites/WolfSprite.cppm`.

**A sprite keeps its colours in a palette of its own**, except the mill's: the mill's figures and
weather take their colours from the theme, passed in as a struct beside the pose, since the theme
crossfades them. Three scenes whose palette would share a sprite's name took a `Scene` in theirs -
`GhostSceneColors`, `CometSceneColors`, `SatelliteSceneColors`.

**A sprite's idle motions run on a time the scene hands it** - blinks, twitches, flickers, each a
whole number of times per unit of that time - so it loops in any scene that hands it a whole
number of units a cycle. The satellite takes two: `orbitTime` turns its panels once a round, and
`blinkTime` is the cycle time times `k_lively`.

The split was checked against renders taken before it: the seven scenes at 800 x 600 and 500 x
700, eight times each, and the mill in every theme at four times. Nothing moved by more than two
levels of a colour channel, except the pendulum in ClockworkScene - seven levels along its edge,
where its swing's sum now comes out in another order.

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
stand apart go through ScatteredShapes: the stars and the city lights, binned into six opacities,
the asteroids, the tufts, pebbles and flowers, the blades of grass, the rivets, the fence, the
floor's planks and seams, the ripples. The rule is also used on purpose, for the hole in a gear's
rim and in the escape wheel.

**What a fill costs differs between the backends.** The CPU rasterizer builds a coverage mask
over the path's bounding box, so its cost follows the area the path spans, whatever the shapes in
it cover. Measured at 3840 x 2160: 170 dots of 1.5 units scattered across the frame took 21 ms in
one path and 0.22 ms filled one by one; a box over the whole frame took 17 ms through the path
rasterizer and 1.3 ms as a rectangle fill, and with a gradient 19 ms and 9.3 ms. Direct2D builds a
geometry for every fillPath, so there the count of fills is what adds up. Direct2D was not
measured. So ScatteredShapes fills one by one on the CPU backend and gathers into one path on
Direct2D, and fillBox, with drawGlow through it, fills a rectangle whenever the frame only moves
and scales.

**Scattered shapes first shared a path on every backend**, on the reasoning that a fill per path
was the cost. At 3840 x 2160 that put the Chase's front grass at 41 ms of an 88 ms frame, and
every scene at 66 to 117 ms. With ScatteredShapes and the rectangle fills, averaged over eight
times of the cycle (milliseconds, before and after):

- Puppeteer 66 and 37, Ghost 87 and 37, Clockwork 111 and 61, Orrery 117 and 26, Comet 94 and 50,
  Satellite 103 and 56, Chase 88 and 28.
- At 1920 x 1080: Puppeteer 14.6 and 9.3, Ghost 21.4 and 8.3, Clockwork 24.5 and 14.4, Orrery 27.1
  and 6.6, Comet 23.2 and 12.1, Satellite 23.1 and 12.6, Chase 20.9 and 7.0.

Filled one by one, two flowers in every Chase tile and a few asteroids overlap, and the CPU
backend now draws them whole where the shared path cut holes. Direct2D still shares the path and
still shows those holes.

**What is left is mostly what does not move.** At 3840 x 2160 the parts of a frame that are the
same at every time of the cycle - walls, skies, plates, ridges - take about two thirds of the
Puppeteer, Ghost, Clockwork and Comet frames and four fifths of the Orrery's: the Clockwork's
plate alone 40 ms, the Puppeteer's stage 25 ms. Keeping them in an image (the backend's
pushImage and drawImage) would trade those for a blit of the whole frame, which measured 6.5 to 9
ms at that size.

PixelPath::drawEllipse and drawRoundedRect start a path of their own, so addEllipse appends an
ellipse as four cubics.

**Draw times**, CPU backend, clang Release, one thread, 800 x 600 (the Satellite at 800 x 450),
averaged over three frames: Mill 1.5 to 3.4 ms, Puppeteer 5, Ghost 4.6 to 4.9, Clockwork 6.2 to
6.8, Orrery 6.5 to 6.7, Comet 4.3 to 5.7, Satellite 5.4 to 6.8, Chase 4 to 4.7. Not measured
under Direct2D.

## SceneLayer

**Oj's shape** (2026-11-07): a scene prepares its still parts without a canvas, from the params,
and keeps them itself; draw lays each one in with a line at its place in the paint order. The first
proposal - one still image under everything that moves - could not have kept the Puppeteer's
valance, which hangs over the swaying curtains.

**Measured** at 3436 x 2051 on the CPU backend, clang, frame times averaged over eight times of
the cycle, live and with the layers kept:

- Puppeteer 24 and 13 ms - the stage and the valance.
- Ghost 24 and 15 ms - the sky, the moon, the land with the tree and the fence, the tombstones.
- Clockwork 41 and 17 ms - the plate with its rivets and the lamp's glow; the pipes, the valve and
  the gauge without its needle.
- Orrery 19 and 9 ms - space and the Milky Way, and the far and the near halves of the orbits.
- Comet 28 and 20 ms - the two ridges and the lake's water, the ridges again upside down, the
  lake's shade and the shore.
- Satellite 29 and 22 ms - space above the limb, and the ocean.

In Oj's MSVC profile the Puppeteer's stage alone was half of its frame. Layered frames match live
ones within two levels of a channel - the rounding of premultiplied pixels.

**Translucent and sparse parts pay since 2026-11-08.** Until then compositeSurface copied an
opaque run whole and took every other pixel one at a time, the clear ones inside the cut included,
and a layer was one rect round everything it drew. The Clockwork's pipes - an L along two sides,
5.6 million pixels cut and nearly all clear - were 6.6 ms slower kept, the lake's translucent shade
5 ms; the moon's halo, the tombstones, the shore's pines and the orbits were slower kept as well.
compositeSurface now takes eight pixels at a time (see Graphics-Types) and a layer keeps a band of
32 rows at a time, each cut to its own drawn columns - the pipes keep 0.6 million pixels. Each of
these was then measured in the frame, drawn live while the rest stayed kept, against all kept: the
orbits gain about 0.9 ms a half, the pipes 0.5 to 1.5, the lake's shade about 0.5, the moon 0.3 to
0.45, the shore and the tombstones about 0.2 - within a single run's noise, though each drawn
alone over and over agrees.

**The Chase's sky and grass, and the Comet's sky, are drawn live**: a gradient is as quick to fill
as to copy.

**Preparing costs** 11 to 47 ms a scene - a canvas the size of the scene for each layer, cleared,
drawn on and cut into bands; the Comet's seven layers cost the most. That is why the showcase
prepares only once two frames running were drawn at one size: a resize draws live until it rests.

**The bands** come from the first pixel that is not clear from each end of a row, so a covered row
costs two pixels. Every band is copied out, a whole frame's too; at 32 rows a band, a frame 2051
rows tall is 65 blits.

**The Comet's reflection keeps its ridges as layers of their own,** drawn upside down through the
upright frame: a layer fits the frame the scene is drawn in, so one prepared for the upright frame
could not be laid into the frame turned over inside the lake. They are laid in inside the lake's
path clip, which a layer does not see - the row along the lake's edge that the clip covers in part
is covered by the shore after.

**Direct2D draws layers live.** Its drawPixelView copies the bitmap up to the GPU on every call,
which a kept layer would pay on every frame. A layer kept on the GPU would need a backend call of
its own.

**Under a path clip** a layered frame differs from a live one: the kept pixels are laid in by
compositeSurface, which knows the clip's rectangle but not its mask - the same as the mill's
background bitmap. The showcase draws no scene under a path clip; Oj's profiles show none.

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

## shadeBall

The shade is built in a frame turned so the light comes from +x, from two arcs: the half disc on
-x, slightly larger than the ball so its antialiased rim is covered under the clip, and the half
ellipse of the terminator, swept one way or the other by the sign of `toViewer`.

## CometScene

The reflection draws everything above the lake a second time through a mirrored frame -
`Translation(0, 2 * lakeY) * Scale(1, -1)` - under a clip to the lake and a 0.42 canvas opacity,
which roughly doubles the sky's cost. The afterglow is drawn between the far ridge and the near
one, so the near ridge hides the landing itself.

**The dust tail opens from the nucleus** as a quarter ellipse over its first tenth (2026-11-08).
The mockup started it at full width with a straight edge across the head, and at 3436 x 2051 Oj
saw that edge through the coma's glow as a square butt. Rendered against it: a half circle round
the head read as a blunt capsule, and a nose a fifth of the tail long grew a belly on the fanned
side. Oj picked the tenth.

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

**The ocean is a layer of its own, and space stops at the limb** (2026-11-08). In Oj's MSVC profile
the Satellite was the slowest scene; the ocean's gradient, laid through the globe's clip over the
whole frame, took 4.5 ms of a 34 ms frame in the harness, and below the limb the kept space was
copied only to be covered. Kept, the ocean costs 2 ms, and space reaches only the lowest point of
the limb in the frame, a little past it for the edge's antialiasing. The night is a box over the
frame rather than a quad far past it: its gradient holds its first stop, which is clear, before its
start, and the quad's rasterizing cost 1.5 ms. Live frames came out the same bit for bit, layered
ones within a level.

After the fill and stroke changes in Graphics-Types the parts at 3436 x 2051 are the night 3.3 ms,
the coastlines and clouds 3.7, the atmosphere 3, the kept space and ocean 4.6, the globe's clip 1.6
- rasterized anew every frame.

**A cloud is a lump with a soft edge, and some clusters carry one or two smaller lumps overlapping
it** (2026-11-08). Each cloud was three translucent ellipses in a row, a larger one in the middle,
and every one of the 90 read the same - Oj: "repetitive like water molecules", the overlaps showing
as lenses. Rendered against it: one lumpy outline each, streaks with smaller lumps, a lump with a
brighter core; then each blurred. Oj: "they have to be blurred", the softest of the blurs "the most
natural", and "let them be lumpy and some randomly overlapping". The edge is four waves of 2, 3, 5
and 9 round an ellipse, their starts drawn for each lump; the soft edge is the Shadow layer, the
outline 0.8 of the size so the fade keeps the cloud about as large. Lumps are filled one by one, so
an overlap shows brighter rather than cutting a hole under the even-odd fill. 45 percent of the
clusters carry a companion, 15 percent two. Each lump takes 0.35 to 1.45 of the cloud colour's
opacity, on Oj's "randomize their opacity too": some read as wisps, some as dense. At 3436 x 2051 on
the CPU backend the clouds cost about 4 ms a frame more than the three ellipses did, after
rasterizeGlowMask's rows were narrowed - see Graphics-Types.

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

To watch: the peeking wolf, the gator and the cow move with `sin(t * 12)`, `sin(t * 15)`,
`sin(t * 30)` and `sin(t * 50)`, which are not whole turns of a 0 to 1 cycle, so they jump when the
cycle wraps. The water lily bobs one and a half times a cycle, and jumps the same way.

**The mill's figures and weather are sprites and effects** like the other scenes' (Oj,
2026-11-07, "Pull them out too"): the cow, the trees, the mill, its wheel, the reeds, the lily, the
wolf and the gator, the rain, the lightning, the snow, the leaves, the fireflies and the dust.
What stayed is the scenery drawn once - the sky and hills cached in a bitmap, the sun's corona, the
clouds, the river, its foam, its reflection and its fog - still drawn through the canvas with the
offset, as before. `faded` and `calculateSyncRotation` went to the kit, and `pseudoRand` gave way to
pseudoRandom, which is the same sum.

## CowColors

Oj, 2026-11-07: "at night its horns illegible", then, picking light horns at night from renders
of three ways, "limbs are suffer too". By the theme colours, the horns and the tail stood as low
as 1.0:1 against the night skies and the legs as low as 1.1:1 against the hills. In the 13 themes
where they turn light, they now stand at 3.5 to 13:1.

**The rule compares two contrasts rather than reading how dark the backdrop is.** The renders Oj
picked from turned the limbs light below a sky luminance; carried to every limb, that made
Twilight's legs and Vintage Noir's horns worse, 2.4 to 1.2 and 2.5 to 1.1, where the backdrop is
middling. Comparing the contrast of either colour never makes a limb worse, and the night themes
come out as in those renders. Where neither stands out the limbs stay as they were: Lunar Eclipse,
whose body and detail are one grey, and the legs in Underwater Realm and Cyberpunk Night.

The backdrops are read off the theme in MillScene::draw: the sky's gradient at 0.58 of the height,
and the front hill's at 0.38 of its own, darkened by a tenth as drawHills does.

## StarStyle

The mill drew its own stars, all sparkles, until Oj asked whether it needed stars of its own
(2026-11-07) and picked moving them into the shared field. Sparkles keeps their look: the same
positions from the same random numbers, a size that follows the twinkle, nothing below a fifth of
it.

**Sparkles are filled one by one**, where dots are binned by opacity: they all share one opacity,
so binning would put every star in one path, and two that overlap would cut holes in each other
under the even-odd rule.
