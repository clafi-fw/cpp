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
`GhostSceneColors`, `CometSceneColors`, `EarthSceneColors`.

**A sprite's idle motions run on a time the scene hands it** - blinks, twitches, flickers, each a
whole number of times per unit of that time - so it loops in any scene that hands it a whole
number of units a cycle. The satellite takes two: `orbitTime` turns its panels once a round, and
`blinkTime` is the cycle time times `k_lively`.

The split was checked against renders taken before it: the seven scenes at 800 x 600 and 500 x
700, eight times each, and the mill in every theme at four times. Nothing moved by more than two
levels of a colour channel, except the pendulum in ClockworkScene - seven levels along its edge,
where its swing's sum now comes out in another order.

**Each scene states its own cycle.** All of them run at 40 seconds except OrreryScene and
EarthScene, which run at 240: the first mockups ran both at 40, and the planets and the turning
Earth were far too quick. Slowing the whole cycle slowed the twinkling and blinking with it, so
those two multiply their quick details by `k_lively`, 6, to keep the 40-second pace. Changing a
scene's speed is its `k_cycle` alone - every motion is counted per cycle, so a new length changes
the pace and never the loop.

**SceneTime carries the cycles passed** (Oj, 2026-11-08). Fitted into a cycle, the Satellite's orbit
showed every region only at 4 orbits and 3 turns of the Earth in 12 minutes, and Oj: "you have an
infinite surface - the orbit does not have to repeat itself". A motion that never comes round needs
time past the cycle, which a frame depending on cycleTime alone cannot hold. Of three ways -
SceneTime in draw, a setter on Scene beside the look, the scene counting the wraps itself as the
mockup did - Oj picked SceneTime in draw. A scene counting the wraps itself would make a frame
depend on the frames before it, so a still or a jump would start the orbit within the first cycle.
The showcase counts the wraps as it brings the cycle time back into 0 to 1. The other scenes' frames
came out bit for bit the same, in both looks.

**A scene moves its own view** (Oj, 2026-11-08, "Drag & Scroll (the globe)"). Asked how the drag and
the wheel should reach a scene - virtuals on Scene with the scene keeping its view, the view passed
into draw beside SceneTime, or the showcase talking to the Satellite alone - Oj picked the virtuals.
Passed into draw, the view would be summed pixels and notches the scene turns into angles each
frame, so a drag made at one altitude would mean another after a zoom; kept by the scene, each drag
is turned into angles at the altitude it is made at. The glide and the easing of the wheel stay with
the host, so the scene keeps no clock: ScenePanel runs both on the time between frames, which a
paused scene still has.

**A press tells the scene it holds the view** (Oj, 2026-11-09, "you press and hold, you zooming in
and out, you dragging - means that you are in control now and the globe should not move for a
while"). A press held still reached the scene as nothing, so the scene could not tell it was held.
Of a virtual on Scene - holdView(bool), the scene holding what it chooses - and the host freezing
the scene's time, which would have stopped the satellite's lights and the stars' twinkle with the
globe, Oj picked the virtual. Its parameter is unnamed in Scene, as dragView's and scrollView's are,
for the empty body's sake; EarthScene names it `held`. ScenePanel calls it on a press and its
let-go, and lets go of a scene shown away from while held.

**A scene keeps its view in a config section of its own** (Oj, 2026-11-09, "Remember zoom, position
and orbit"). Of one line of named values passed as text, a config section the scene reads and
writes, and a struct of numbers on Scene, Oj picked the section. The config is read with its schema
as the application is built, before the form makes any scene, so a virtual viewSchema() had no
scene to ask; of a static per scene gathered by createSceneViewsSchema(), the scenes made before
the application and handed to it, and a throwaway scene list (start-up waiting about 15 ms for the
Earth's coastlines as it went), Oj picked the static. storeView and restoreView are virtuals; the
showcase restores in its constructor and stores on the form's close, as BrowserControl does.

**A scene states its own choices** (Oj, 2026-11-09, "Add a direction selector to the options"; the
Earth's turn, labelled Eastward and Westward). Of choices on Scene kept by the scene in its view's
section, the same choices kept by the showcase under a section of its own, and a turn property on
EarthScene alone that the showcase would name, Oj picked the first. SceneChoice is a name and a span
of option names; the host builds a section per choice the way it builds the look's, previews an
option on hover and puts the pick back, and never stores it: the scene writes it in storeView.

**The let-go is a call of its own** (Oj, 2026-11-09, "The decision is made when the user releases
the mouse button"). The scene needs the pointer's speed at the release, which only the host knows.
Of letGoView(speed, bounds) in place of holdView(false), and holdView taking the speed beside the
flag, Oj picked letGoView; holdView now only ever says true. ScenePanel hands over the speed its
glide starts at, 0 below the glide's 2 pixels a second, and lets go of a scene it shows away from
at 0.

**A drag takes hold of the view, and only from the globe** (Oj, 2026-11-09, "you can press
anywhere and the rotation stops. And you may want press anywhere just to close a popup menu"). The
press held the view and a still let-go hung the satellite, so every click did: one on the readings,
which the panel's nested handlers caught, and the one that closes the backstage, which
FormBase::wnd_mouseDown hands on to the control under the pointer once the popup is down. Read off
the code and not seen: a single pixel of jitter in a click's last 80 ms made at least 12 pixels a
second, a fling that re-aimed the orbit. Of the view taken once a drag leaves the click's slack,
the same with a long press to hang, a press holding at once and giving the flight back on a click,
and the framework keeping the press that closes a popup, Oj picked the first, adding "only presses
on the ball itself should count, and the cursor should be set to handpoint while over the ball".

- Scene::dragMovesViewFrom, a virtual answering offersView() by default; EarthScene answers for
  the disc globeDiscAt places, the same one heldPointAt reads.
- ScenePanel keeps a press only when it lands on the panel itself, not a control over it, and
  where the scene says a drag moves the view. The slack is DragEvent::pastThreshold, 16 unscaled
  pixels; the drag then starts from the press point, so the first step carries the whole slack and
  is dated at the press - dated at the move, a slow drag past the slack let go at once would read
  as a fling of up to 1920 pixels a second at 100%.
- The hand comes from mouseMove asking the scene, so a wheel zoom under a still pointer leaves the
  shape as it was until the pointer moves.

**The wheel leaves the flight alone** (Oj, 2026-11-09, "also the wheel should not anymore stop the
fly - and that is the last code that uses the 5-seconds resume timer"). The wheel was the one touch
left that held the globe for k_heldStill and eased it back over k_easeBack, a let-go having reset
both. They went with it, as did stillTimeBy, m_viewTouched and m_lastTouchAt: stillSecondsAt counts
only the frames drawn while a drag holds the view, so every frame away from a drag is bit for bit
what it was.

**A press on the globe holds it at once** (Oj, 2026-11-09, "with a handpoint indication and limited
click area, click may stop without a threshhold"). The slack went, and ViewPress with it: a press
where the scene says a drag moves the view holds the view as it comes down, and a still let-go hangs
the satellite, a click included. The jitter fling read off the code - one pixel in a click's last
80 ms reading as at least 12 pixels a second - applies again to a click on the globe; named to Oj,
not guarded.

**A still click toggles the flight** (Oj, 2026-11-09, "make a still click to toggle the movement").
Of clickView() on Scene, a clicked flag on letGoView, and the host toggling through a query on the
scene and a resumeView(), Oj picked clickView. Its default lets go still, so a scene that offers a
view and takes no clicks goes on as it was. Still is the framework's click slack,
DragEvent::pastThreshold: a press let go before any drag event past it is a click whatever its last
moves read, which also answers the jitter fling above. EarthScene's let-go is split into
hangSatellite and flySatelliteOn(trackAngle); a click flies a hanging satellite on at the track
angle kept from before it hung, laid on the picture as it stands exactly as a fling lays its own.

**A double click is the scene's too** (Oj, 2026-11-09, "double click to advance day/night cycle").
Of doubleClickView() beside clickView and an advanceDay() named for what it does, which the host
would map the double click to, Oj picked doubleClickView. A double click's first click toggles the
flight, and its second press, let go still, toggles it back; of keeping that, a single click
waiting out the double-click time, and the first click's toggle standing, Oj kept it. ScenePanel
answers DoubleClickEvent on the panel itself and does not stop it, so FormBase::wnd_doubleClick
hands the second press on to wnd_mouseDown as it does for every control that leaves it.

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

## pseudoRandom

**The sine and its multiple are taken in double** (Oj, 2026-11-09, "The distribution algorithm has
a flaw - it may put many lights into one spot", with a screenshot of a cluster of city lights).
Taken in float, `|sin| x 43758.5453` keeps 8 bits of the fraction above 32768 and 9 above 16384:
65 percent of the draws fell on a multiple of 1/256, and 20,000 draws gave 2290 distinct values.
Where one draw picks one of many, the picks bunched:

- A coastline town picks one of the 30,689 edges of the outlines it may stand on. 5800 towns stood
  on 1192 edges, up to 28 on one edge 0.1 degree long - the clusters of lights. In double they
  stand on 5215 edges, at most 4 to one.
- 65 percent of the land towns stood on 256 meridians 1.4 degrees apart.

The angle is still summed in float, so each draw moves by about the float's step: 0.0012 on
average and 0.0048 at most, over indexes to 100,000 and shifts to 99. About 1 draw in 430 lay
within that step of 1 and now falls just above 0, or the other way round, so in every scene a star
or a tuft here and there stands elsewhere. The town numbers come from a replica of the coastline
sites over the outlines without their grain and before the land map's weights.

To watch: the cast to float rounds a fraction within 2^-25 of 1 up to 1.0, about 3 draws in 10^8.
Nothing guards it.

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

## EarthScene

Called SatelliteScene, and Satellite in the showcase's picker, until 2026-11-09; the notes below
from before then say Satellite.

The globe is an orthographic projection, looked at from latitude -20 with its north turned 0.22
radians off the vertical; its radius is 1.15 times the larger side of the frame, so only a cap
shows.

**The terminator is a straight line** through its points on two rows of the frame. The first
version drew the night as five fills of different reach to soften the edge, and they banded
visibly. One gradient across a straight line has no bands, and over the small cap in view the
line stands in for the curve.

The coastlines are Natural Earth's land at 1:110m with a grain added, a point every 0.1 degree of
arc - see the entry of 2026-11-08 below. A shape is drawn only when some of it is on the near side
and what is left of it reaches the frame.

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
swapped. The grained outlines were checked: no edge crosses another within 120 points of it.

**The ocean is a layer of its own, and space stops at the limb** (2026-11-08). In Oj's MSVC profile
the Satellite was the slowest scene; the ocean's gradient, laid through the globe's clip over the
whole frame, took 4.5 ms of a 34 ms frame in the harness, and below the limb the kept space was
copied only to be covered. Kept, the ocean costs 2 ms, and space reaches only the lowest point of
the limb in the frame, a little past it for the edge's antialiasing. The night is a box over the
frame rather than a quad far past it: its gradient holds its first stop, which is clear, before its
start, and the quad's rasterizing cost 1.5 ms. Live frames came out the same bit for bit, layered
ones within a level.

After the fill and stroke changes in Graphics-Types the parts at 3436 x 2051 are the night 3.3 ms,
the coastlines and clouds 3.7, the atmosphere 3, the kept space and ocean 4.6, the globe's clip 1.6,
rasterized anew every frame.

**The clouds were lumps with soft edges, some clusters carrying one or two smaller lumps overlapping
them** (2026-11-08), until the maps below took their place. Each cloud was three translucent
ellipses in a row, a larger one in the middle, and every one of the 90 read the same - Oj:
"repetitive like water molecules", the overlaps showing as lenses. Rendered against it: one lumpy
outline each, streaks with smaller lumps, a lump with a brighter core; then each blurred. Oj: "they
have to be blurred", the softest of the blurs "the most natural", and "let them be lumpy and some
randomly overlapping". The edge is four waves of 2, 3, 5 and 9 round an ellipse, their starts drawn
for each lump; the soft edge is the Shadow layer, the outline 0.8 of the size so the fade keeps the
cloud about as large. Lumps are filled one by one, so an overlap shows brighter rather than cutting
a hole under the even-odd fill. 45 percent of the clusters carry a companion, 15 percent two. Each
lump takes 0.35 to 1.45 of the cloud colour's opacity, on Oj's "randomize their opacity too": some
read as wisps, some as dense. At 3436 x 2051 on the CPU backend the clouds cost about 4 ms a frame
more than the three ellipses did, after rasterizeGlowMask's rows were narrowed - see Graphics-Types.

**The atmosphere is three glows** (2026-11-08). Its rings were three flat strokes, and Oj saw their
edges: "There are visible sharp atmosphere edges". Rendered against them were the three rings soft,
one wide haze with a soft line, and soft hazes with the line kept sharp; Oj picked the three soft.
Each ring is an OuterGlow with the limb's gradient as its brush, reaching 0.02, 0.005 and 0.016 of
the radius at 0.3, 1 and 0.34 of the colour's opacity. The line is centred at 1.001 of the radius;
at the strokes' 1.003 its glow left a dark gap between it and the ocean. With the glow's mask kept
in runs - see Graphics-Types - the rings cost what the strokes did, about 3 ms at 3436 x 2051.

**The clouds float a hundredth of the radius above the ground** (2026-11-08). Drawn on it, under the
globe's clip, they read to Oj as glued to the surface, and more realistic on a larger radius.
Rendered at 1, 2 and 3.5 percent, Oj picked 1. The lumps lie on a sphere that much larger, cut at
its own rim, and drawn over the night, the sea's glint and the city lights.

**A lump faded into the haze as it neared the limb**: its opacity rose from nothing where its center
is on the rim to whole where the center faces the viewer by 0.12. Drawn whole up to the rim, a lump
near the limb stood in the sky with space under it, and its far side showed in the ring between the
Earth's edge and the sphere's rim, down to sqrt(1 - 1/1.01^2) behind it - drawn as the near side of
its outline mirrored through the plane of the rim, before the ocean, whose disc hid what fell inside
the Earth's edge. Seen that slantwise a lump is a thin streak sliding along the rim: the far part
grew along the whole of it in half a second as the rim swept through it lengthwise, and the near
part narrowed into the rim. Oj, from a video: "They like stretching out and back to 0". Rendered
against it, as 20 seconds of the limb: no far side, where a lump narrows into the rim up in the sky;
and a fade toward the limb, light and strong. Oj picked the light fade, and the far side went with
it, being behind the rim and so faded out whole.

Each lump's colour was mixed with the night's stop by that stop's opacity, at the lump's own
opacity, which leaves the pixels the night laid over the lump would; a lump wholly on one side of
the night's gradient takes that side's colour solid, the rest a gradient. With the clouds raised and
the atmosphere soft, a frame at 3436 x 2051 on the CPU backend went from 22.8 to 23.6 ms, measured
in one run against the frame before either.

**The clouds are three layers of map** (2026-11-08). Oj asked for layers "with proper clouds bigger
and more widespread and translucent, with layers rotation slightly differ by angles and speed, draw
them all on top of each other", each worked "in a separate thread", and for mockups first. The
mockups drew maps of value noise sampled on the sphere, in Python; Oj picked variant A: puffy heaps
at 0.6 percent of the radius, a patchy sheet at 1.1 on an axis leaning 5 degrees, streaks at 1.8
leaning -6, at 0.9, 0.55 and 0.45 of the cloud colour's opacity, with that opacity raised 2.2 times
(0x5C to 0xCA). The recipes in CloudMap are the mockups' own, seeds 11, 21 and 31, covers 0.32,
0.45 and 0.30, each cut at the quantile of its noise.

**The layers drifted back and forth until 2026-11-09.** The mockups turned them at 0.985, 1.02 and
1.05 of the ground's speed, which is 1.97, 2.04 and 2.1 turns a cycle, and would have jumped where
the cycle starts over. Each drifted as a sine instead, once a cycle, its amplitude 2 (speed - 1)
radians so that it moved as fast as the mockup's at its fastest: 1.7, 2.3 and 5.7 degrees, the three
starting at different points of the sine. A steady drift replaced it - see the entry of 2026-11-09
below, "The clouds drift steadily east".

**Two looks, one switch** (2026-11-08). Oj wanted the realistic look of the mockups and "more
stylized clouds", both, switched by a setting, and picked "Two-tone" of the stylized mockups (flat,
two-tone, soft flat): each layer shows 0.55 of its opacity where its cover passes about 0.31, and
all of it past about 0.65. The setting was to be app-wide on the Scene page, "It has nothing to do
with performance, it's a look"; then "the UI should clearly tell which scene have this setting,
after a theme selection" - so the showcase shows it captioned with the scene's name, only while that
scene is picked, where the mill's themes stand while the mill is. Scene::offersLooks lets the
showcase find the scene without naming it; the look is a plain value on Scene, set before a frame
rather than handed in SceneParams, so a change shows on the next frame instead of the one after a
prepare.

**The maps are built on the first draw, off the paint.** In the container (2 cores) the three take
1.1 s; std::lerp in the value noise took 2.2, its checks for exact ends costing as much as the noise
around them. Built in the constructor they would cost every MillScene launch that much processor
time, the Satellite shown or not. The scene draws no clouds until they are ready. MSVC's
std::future default-constructs the value it carries, and CloudMap has no default, so the future
carries a unique_ptr.

**Every pixel is projected back onto each layer's sphere**: the screen's tilt undone, the depth
from the radius, the layer's axis tip undone, then longitude and latitude by two arctangents. The
plain C++ cost 78 ns a pixel for the three layers - 34 ms of a 1600 x 900 frame on the container's
2 cores, against 5.6 for the rest of the scene. Written for AVX2, eight pixels at a time, with the
map's texel and its right neighbour read in one 4-byte gather (each row keeps its first texel again
at its end), it came out within one level of the plain version. At 3436 x 2051 under a scale of 2.5
it still took 18.7 ms on 2 cores, so from a scale of 2 the layers are worked out at every other
pixel and row. Blending the finished alpha between those points cost 6.7 ms but left the two-tone
edges in 2-pixel steps; blending each layer's cover and haze and cutting after keeps the edges as
sharp as at every pixel, for 10.8 ms. Laying the 39 bands in costs 2.2 ms more, on the paint's
thread. On the container's 2 cores the whole frame there went from 25 to 33 ms with the lumps to 36
to 44 with the maps; the projection's part shrinks with every core a machine adds, and the lumps'
4 ms are gone. Under Direct2D the bands are uploaded as bitmaps every frame.

**Eight pixels at a time skip the layers that show nothing there** (2026-11-08). Oj saw two cores
busy while the Satellite ran: the clouds' 20 ms of processor time a frame at 3436 x 2051, spread
over the cores, which at 50 frames a second is a core's worth. Timed apart, the points took 8.5 ms
of it and the pixels 12. Merging the per-pixel passes into one - each layer's cover and haze spread,
cut and combined in registers, then shaded and written - saved nothing, the pixels' arithmetic and
not their memory being the cost. What saved it: a pixel's cover lies between its points' covers, so
where all the points 8 pixels blend from fall at or under the cover a look starts showing at (0.28
stylized, 0 realistic), that layer is skipped, and 8 pixels no layer shows are written clear. Frames
came out bit for bit as before; the pixels' part fell from 12 to about 3.5 ms stylized and 5
realistic, the clouds from 20 to 12 ms of processor time.

**The bands were worked out while the ground was drawn, for one build** (2026-11-08). Oj, after the
skip: "the frame rate is what I after". His frame at 3436 x 2051 was 16.5 ms on the paint thread and
48 a second, and Task Manager's two busy cores stayed busy in the Orrery while MillScene read 92
percent of one core there - one paint thread moved between two cores, the cloud helpers being the
thin spikes on the other 22. startClouds handed the bands to a spreadOverOtherCores before space was
laid in, one std::async starting the helpers and waiting for them, and drawClouds waited after the
city lights. On the container's 2 cores the frame went from 33 to 44 ms to 25 to 32. On Oj's machine
it read 17.18 ms and 46 a second, as before, at 263 percent of a core against 142 to 170, and
Windows counted about 800 threads more - most likely MSVC's thread pool growing under a std::async
task that waits on others it started, not measured. The bands finish in about a millisecond on 24
cores, so there was little to hide; the split went back to the paint thread working on the bands
beside the helpers.

**On a large globe the points stand every 4th pixel** (2026-11-08), on Oj's "If B gives less Co2
that's welcome too". With the skip, the points were 8.5 ms and the pixels 3.5 of the 12 at every
other pixel; at every 4th the points take about 2.5 and the clouds about 6.5 ms of processor time,
7.5 realistic. Inland the frames match every other pixel to the eye; at the limb, where the maps are
squeezed most, the faint high streaks in the haze over the Earth's edge break into 4-pixel steps
("Satellite clouds - every 4th pixel.png", shown to Oj before he asked for it). Working the strip
out at every pixel was offered, for about 3 ms more.

**The step looked at the wrong scale, and the app never left 1** (2026-11-08). Oj's CPU profile of
that build (35 s at 3436 x 2051, 250 percent): the paint thread 95 percent of a core, 24 helper
threads together 1.15 cores, 95 percent of their samples inside rasterizeCloudBand and none
spinning: about 25 ms of cloud work a frame, the full-resolution figure. MillScene draws in device
pixels and states its scale through metrics, so the canvas transform the step was chosen by is 1
there; the harness drew under a scale of 2.5, which is where every measurement of the steps came
from. The step now follows the globe's radius in pixels, 3200 and up taking every 4th: the harness
frames under a scale stay as they were, and the same frame drawn without one (as the app draws) went
from 33 to 37 ms of cloud work to 4 to 7 on the container.

**The bands are laid in on all the cores** (2026-11-08). The same profile put 5.2 ms of the paint
thread's frame on laying in bitmaps - 3.0 the space-and-ocean layer, 2.1 the cloud bands - against
1.8 left for the clouds' own share, about 0.5 after the step. SceneLayer and drawClouds hand their
bands to Canvas::drawPixelViews in one call, which the CPU backend spreads over the cores in strips
of rows; see the Graphics-Types working note. Frames come out bit for bit as with the bands laid in
one by one.

**The form's fill under the scene stays.** The profile put 1.4 ms on MillSceneForm filling the whole
window under the scene panel, and filling only the strips round the panel would save it - but the
Mill draws on that fill. Drawn on a clear canvas it leaves 393,000 of a 900 x 600 frame's pixels
short of opaque, and the Puppeteer a row across the frame about 1 percent short; the other six
cover their frames whole. Leaving the fill out needs the scene to say it covers its frame, and no
Scene says so yet.

**The land is worked out per pixel** (2026-11-08). Oj: "Make land more detailed. Like mountain
ranges". Four looks were rendered from the scene ("Satellite land - four looks.png" and two more
sheets in Claude Output\MillScene Scenes): relief over the flat colours, relief with land cover,
stylized terraces, and terraces with flat zones. Oj picked "B+D": the second for Realistic, the
fourth for Stylized.

- EarthMap packs four bytes a texel: height, the two slopes and the variation of the detailed
  relief at 4096 by 1024; height and slopes of the broad relief, and dryness, forest and tundra, at
  2048 by 512. A slope is kept as the square root of its share of the steepest, 200 per radian -
  the steepest built is 149 - so a gentle slope keeps its grain. On the container's 2 cores the
  maps build in 1.9 s, and readLand takes 7.3 ns a point on one core.
- The colour is worked out at the points and spread between them. The stylized bands and tones of
  slope are cut at each pixel from the spread height and light, so their edges stay sharp; 8 pixels
  no band or tone reaches pass them over, and points no covered pixel reads are not worked out.
  Working the colour out at every pixel had cost 12 ms of processor time a frame at 1600 x 900.
- The coasts went first through a pushClip of the coastlines, the CPU clip mask read back into the
  pixels. That took 4.5 to 5 ms of the paint thread at 3436 x 2051 - about what the flat fill it
  replaced took - and left Direct2D to cut the bands itself. Each band now works out the
  coastlines' coverage from the area each edge sweeps in each cell, summed along the row, as font-rs
  does, on all the cores, and both backends lay the same pixels in. The coasts match the clip's to
  within an antialiasing step.
- Inside the disc's path clip, Canvas::clipBox() stands in the clip's own coordinates, not the
  canvas's pixels: under a canvas scale the land came out cut to the scaled width. The land is cut
  to the canvas, and the backend's clip does the rest.
- Measured on the container, a frame: at 1600 x 900, 2.9 to 4.2 ms of processor time and 1.7 to
  2.4 of the paint thread, against 0.7 to 1.0 for the flat fill; at 3436 x 2051, 9 to 13 ms of
  processor time, of which the paint thread keeps about 1 to lay the bands in, against 5 for the
  flat fill. The clouds came out bit for bit as before once the sphere's points were shared with
  the land.

**The land is read as fine as the ground a point stands for** (2026-11-08). Oj sent a recording at
dusk, 2296 x 1198: "it shimmers. I closed my eyes on that on clouds because on atmosphere that can
be mistaken with air turbulences but on the terrain that's no so great". The maps were read at their
finest everywhere. The whole visible cap faces away from the viewer - its depth stays under 0.7 -
and toward the limb the ground between two points grows as one over the depth, so the ridges and
the slope light, which carries their finest detail, were sampled too sparsely and flickered from
frame to frame.

- EarthMap keeps both maps as pyramids of 8 levels, each averaged 2 by 2 from the values of the one
  before and packed only then, and reads each point between its two nearest levels, the way a GPU
  reads a mip-mapped texture. The broad relief is gone as a map of its own: it is the relief read
  no finer than level 2.75. Against the stylized renders Oj approved, level 2.5 left Tibet spotty
  and level 3 lost the Atlas's bands; 2.75 matched.
- The scene works out each point's spans from the screen's two directions, turned as the points
  are and laid along east and north, adding what the two reach. The span picks the level by the
  greater of longitude and latitude, so north of about 60 degrees the land reads coarser along the
  meridians than it needs to. Near the limb that costs nothing: there the latitude span is the
  greater anyway.
- The flicker was measured as the mean second difference over time of the dusk frames, around a
  ridge near the limb. Read at the level the points' spacing gives, it fell from 5.6 to 2.7 with the
  clouds hidden; half a level coarser, to 1.7; a whole level, to 1.1, but the ridges went soft. Half
  a level was kept: the spans are taken 1.4 times as wide as the points stand apart. With the clouds
  shown, 4.9 fell to 1.6.
- Two reads along the longer side of each point's ground, each at the level of half that side,
  kept a little more detail across the ridges for a fifth more of the land's time. The difference
  did not show in the renders, and was left out.
- Measured on the container, a frame of the land: at 1600 x 900, 3.9 ms of processor time became
  4.8 to 5.5; at 3436 x 2051, 9.3 became 10.6. The maps take 28 MB, against 25. Building them takes
  3.1 s on the 2 cores, of which the heights, unchanged, take 2.2; the levels add 0.1.

**The stylized land's edges are cut at the pixel** (2026-11-08). Oj, after the levels of detail:
"Great in realistic, shimmers in stylized". On the dusk frames with the clouds hidden, the same
measure around the ridge gave 0.92 in all, 0.27 without the tones of slope, 0.87 without the bands,
and 0.16 with the zones alone. At 3436 x 2051 the zones' edges stepped: they were cut at the points
and spread between them, so an edge sat midway between the two points it fell between and jumped a
whole step - 4 pixels there - as the Earth turned.

- Every stylized edge is softened by k_edgeSoftening of what its field changes over the pixel - the
  four points' change across and down, added - and no less than its fade where the field changes
  slowly. The bands took their softness from the slope over the depth, held at 0.2, which ran short
  toward the limb.
- The zones are cut at the pixel only where the points about 8 pixels do not all lie whole in the
  same zones. Each point keeps a key of the zones it lies in, -1 on an edge; comparing the three
  colour channels instead cost 2 ms at 3436 x 2051.
- A pixel reads the point after its own across and the row of points below, so a point is marked
  needed from a whole step before it. At a step of 1 the row below gives the change down, and a
  band works out one more row of points there.
- With the edges softened the ridge measured 0.83. The measure counts every sharp edge that moves,
  so it falls little; the frames show the zones' steps gone at 3436 x 2051.
- Measured on the container, a frame of the stylized land: at 1600 x 900, 5.0 to 5.8 ms of
  processor time became 6.0 to 6.6; at 3436 x 2051, 14 to 15 became 16.5 to 17.3. Realistic came
  out bit for bit as before, at steps 1, 2 and 4.
- The stylized clouds' two tones still step toward the limb: their cover is cut at the pixel with
  a fixed fade, and the cloud maps are read at their finest.

**The clouds are read as fine as the ground a point stands for** (2026-11-08). Oj: "Do both, I
already know that draw rate dropped a bit (at least on GPU - both backends now show identical times,
and GPU was faster before), and there no much of it to spare". The two were the cloud maps' levels
of detail and the stylized tones softened by the cover's change over the pixel.

- CloudMap keeps 8 levels of bytes, each averaged 2 by 2 from the one before, and readCover takes
  the spans as readLand does. LandQuery became MapQuery in SceneHelpers, which both maps read.
- At the steps the clouds had - every pixel below a radius of 3200, every 4th above - the cloud pass
  cost twice as much at 800 x 600 and half again as much at 3436 x 2051. The spans cost little; the
  second level's reads and each read's level arithmetic took the rest. The spans' six divisions
  became two 12-bit reciprocals, which took 1.5 ms off at 3436 x 2051 and serve the land too.
- A cloud texel is twice as wide as the land's, 8 pixels at 2296 x 1198 and 12 at 3436 x 2051, so
  points every pixel or every 4th read the map far finer than it holds. The clouds are worked out at
  twice the land's step: every 2nd, 4th or 8th pixel. Cloud pass, processor time a frame, before and
  after: 800 x 600, 3.3 and 2.0 ms; 1600 x 900, 8.4 and 2.7; 2296 x 1198, 16 to 18 and 5.5; 3436 x
  2051, 7 to 9 and 8.5 to 9.5. The clouds soften toward the limb, and at 3436 x 2051 the finest
  wisps a little.
- At 3436 x 2051 writing the pixels takes 6 to 7 ms of the 8.5 and working out the points 1.8.
  Testing only the lanes of points the 8 pixels read, instead of all 8 lanes, changed little: the
  clouds cover most of those pixels.
- A stylized layer's 8 pixels take a whole tone without working out the change where the points
  about them, widened by the softest edge they allow, lie inside it. Edges that soften alike share
  one division, SoftEdge.
- Whole Satellite frames on the container, before and after: 1600 x 900, 13 to 16 ms and 10 to 12;
  2296 x 1198, 24 to 26 and 18 to 22; 3436 x 2051, 40 to 47 and 42 to 46; 800 x 600 about the same.

**The coasts are Natural Earth's, with a grain, and the towns follow the land** (2026-11-08). Oj:
"make the coastlines more jagged and curvy" and "place the towns like described", with "the cloud
beside Italy looks more like Italy that Italy itself". The coasts were 12 hand-drawn polygons, a
corner every 5 to 10 degrees. Of the mockups Oj picked Natural Earth's 1:110m outlines with a light
warp and grain.

- The data is Natural Earth's ne_110m_land, public domain. Kept: the outlines that reach north of 2
  degrees south and enclose at least 0.4 square degrees - 83 of land, 3,678 points, and the Caspian
  Sea, Natural Earth's one hole, 51. Land runs counterclockwise and the sea clockwise. The points
  are hundredths of a degree in int16, about 15 KB, in PathArt.EarthMap.NaturalEarth.cpp, made by a
  script outside the tree.
- The grain is the land map's own fractal noise on the unit sphere: east and north, 0.35 times 3
  octaves from frequency 5, seeds 11 and 12, plus 0.22 times 3 octaves from frequency 45, seeds 13
  and 14, east divided by the cosine of the latitude. The mockup cut the loops a fold makes, where
  an edge crosses one up to 120 points on, and found none at this strength; the code does not cut
  them. A stronger grain would need it.
- The Caspian is cut out wherever the land is: the coverage sums its opposite winding to 0, the flat
  fallback puts all the land's outlines in one even-odd path, and the towns' grid counts crossings
  even-odd across all outlines.
- Building the coasts takes about 98 million instructions, 12 ms on one core: 84 million in
  landOutlines, most of them in the noise, and the rest in the first towns. In the constructor that
  held the app's start, as MillScene makes every scene at once, so the constructor starts it off the
  paint and the first frame takes it.
- The towns stand on a grid of quarter-degree cells from 85 degrees north to 10 south: land where a
  row's crossings put the cell's middle inside, and each cell's distance from the coast by two
  chamfer sweeps from the coasts' points. A site is drawn at random, as likely anywhere on the
  sphere from 10 south to 72 north, from seed 5001 on. On land, its weight is (1 - smoothstep(0.06,
  0.3, height)) x (1 - 0.92 smoothstep(0.4, 0.8, dryness)) x (1 - smoothstep(0.2, 0.8, tundra)) x
  (1 - 0.55 forest) x (0.22 + 0.78 exp(-distance / 2.5 degrees)), read from the broad relief for
  half a degree of ground, and a draw under the weight keeps it. 1200 towns take some 10,000 sites,
  read 8 at a time, inside the land map's build. Greenland's outline is the ice sheet, known by the
  middle of its points, and has no towns. By the weights alone it got 4 in its south, where the
  land map's ice plateau stops at 63 north and the ground lies low below the snowline. Until the
  map is built the towns stand anywhere on the land below 62 north, the ice sheet left out, as
  the old polygons' did.
- Per frame the coast went from 279 edges to about 10,500 in view. Each point keeps its direction
  fixed to the Earth, turned by one spin and one tip worked out once a frame, and the near side's
  points come out as directions' x and y, which one matrix takes to pixels. Turning each point from
  its longitude and latitude, with sines and cosines per point and per projection, took 0.75 ms on
  the paint thread; now 0.20, against 0.03 with the old polygons. Splitting it: turning 0.09 ms,
  the near side 0.08, the edges 0.04. Skipping outlines wholly behind would spare little, as
  Afro-Eurasia and the Americas never are. Walking the outline without a modulo per point took
  0.23 to 0.20.
- Each band of the land gets only the edges that reach into it, counted into one vector. Sweeping
  every edge in every band, the coverage cost 0.92 ms of processor time a frame at 1600 x 900 and
  2.65 at 3436 x 2051; banded 0.48 and 1.94, against 0.47 and 1.61 with the old polygons. Whole
  frames come out about even with the old polygons on the container's 2 cores.

**Most towns stand on the coastline** (2026-11-08). Oj, of the towns placed by the land map:
"Doesn't look like there more towns density on the coastlines". Measured: land within half a degree
of a coast held 2.2 times the average density of towns, and 2 to 3 degrees inland still the
average. The coast weight fell off over 2.5 degrees, and nearly all of Europe lies within 3 degrees
of a coast; with 1200 towns a view of Europe holds about 60, too few to trace a coast.

- A sharper weight alone - falling off over 0.35 degrees - put half of all towns within half a
  degree of a coast, 4.9 times the average, and still drew no coastline. 2400 or 3600 towns at that
  weight began to. Oj: "put more towns on the exact coastline".
- A coastline town is tried at a random point of an outline, picked by point, as the points stand
  about 0.1 degree apart, and moved 0.03 to 0.12 degrees of arc inland: to the left of the way the
  outline runs, which is the land for a coast and for the Caspian alike. It takes the land map's
  weight without the coast's, from seed 90001 on; sites outside 10 south to 72 north are tried
  again. Of the mockups - 1200 land towns plus 1200, 2400 or 4800 on the coastline, and 800 plus
  4800 - Oj picked 1200 plus 4800.
- With the old share of glows, 18 percent above a brightness of 0.82, the extra glows ran together
  into blobs that hid the coast. A town glows above 0.964, about 220 of 6000, as many as before.
- Each town keeps its direction fixed to the Earth and is turned and projected as the coasts are.
  City lights a frame on the container, 1200 towns before against 6000 now: 2296 x 1198, 0.29 and
  0.60 to 0.65 ms; 3436 x 2051, 0.41 and 0.94 to 1.02. Turning each town from its longitude and
  latitude, as the mockup did, cost 0.3 ms more; the rest is drawing the lights in view.

**In the shadow the craft keeps 35 percent of its daylight colour**, and the panels' backs carry
the ribs between the cells and a spar along the boom. The panels turn about once a cycle with the
Sun, so through the shadow they show their backs, which were one flat gray (Oj, 2026-11-07, "one
solid gray"). The stars keep 65 percent of their night brightness by day; at 15, as a camera
exposed for daylight would show them, they read as dull. Both were picked from renders of the
options.

**The orbit crosses the whole globe and never comes round** (2026-11-08). Oj: "change the orbit so
it crosses various areas", then "I want antarctica and Australia and New Zeeland and India,
everything", and picked a polar orbit with north turning, the ground flowing across the screen as
before.

- The land map, the outlines and the towns reach the south. The map runs from pole to pole, 2048
  rows of relief, with 24 more ranges, 4 more plateaus, 17 more dry regions and a dome of ice over
  Antarctica. The outlines are all of Natural Earth's of at least 0.4 square degrees, Antarctica's
  with its edge along the pole, and the grain's east step is divided by the cosine of the latitude
  held at no less than 0.1, as that outline reaches the pole. The towns are tried from 56 south, on
  a grid to 58 south. Map: 53.4 MB against 26.7, 3.8 s against 1.9 on the container's 2 cores, about
  7 s of processor time. Outlines: 16.5 ms against 13.
- The orbit is modelled among the stars: the satellite at (0, sin u, cos u), the orbit's axis along
  x, the frame's across, up and toward worked out from u and the 36 degrees the view looks back,
  then turned into the spin, tip and tilt the per-pixel code already took. The Sun stands at 35
  degrees round from the equator crossing and 10 north. Its direction is handed over in the untilted
  frame; the first try handed it over in the screen's and lit the wrong side.
- A coverage simulation over twenty regions first fitted the orbit into the cycle: 4 orbits and 3
  turns of the Earth in 720 seconds, which slid the ground 37 degrees off the track at the equator.
  Free of the cycle, the Earth turns once in 3 + phi orbits. Of the ratios tried, from 1/phi^2 to
  1/(7 + 1/phi) turns of the Earth an orbit, all with small partial quotients, 1/(4 + 1/phi) showed
  every region by day soonest - within 14 minutes, the longest wait about 27 - and slides the ground
  12 degrees.
- The stars drift 0.2 frame widths an orbit, against 2 in the first mockup (Oj, "The sky much
  slower"). The cycle is back at 240 seconds and k_lively at 6.
- A frame on the container at 1280 x 720 averages 6.2 to 7.0 ms against 6.4 to 6.9 before, at 2560 x
  1440 25.1 to 28.2 against 27.2 to 28.7. At 160 x 90, with the map built, 1.11 to 1.22 ms against
  0.98 to 1.05 with the old map under the same scene code - the land's stage, from the outlines' 25
  percent more points. Timed while the map was still building, the frames ran 0.5 ms slower: the
  first runs waited 5 s, and the map takes 3.8.

**The globe drags and zooms** (2026-11-08). Oj picked grab the ground for the drag, zoom by altitude
for the wheel, a glide then the orbit on after a let-go, and for the satellite at the top: "shrink
and move".

- The first drag solved the two moves the polar orbit allows - along the orbit, and the Earth's turn
  under it - for the ground under a point held in the view, damped by (0.05 R) squared. Over a pole
  the Earth's turn only spins the ground about it, and Oj: "Vertical drag from a pole refuses to
  move that pole and only rotates the globe around. The poles shouldn't affect the drag behavior at
  all". The drag is now a turn of the orbit as a whole, among the stars: the ground under the held
  point - the frame's middle at 72 percent of its height low down, the globe's middle with the whole
  globe in view - turns onto where the drag takes it, about the axis square to both, as a trackball
  turns. The view keeps following its orbit, so the ground still flows across the screen afterwards,
  along the turned orbit, and the Sun keeps its place among the stars. A turn of the Earth itself
  was the other way: it would have tipped the Earth's axis toward or away from the Sun. With time
  held still the held ground moved 20 pixels a frame for a drag of 20 and 10 for 10 at the lowest
  view, and 4 for 4 a frame with the whole globe in view while a pole crossed the middle.
- The view's angles were worked out from the polar orbit's own formulas; they are now read from
  where the Earth's axes stand in the view (ViewAngles::Of), which gives the same rotation for any
  orbit - checked in double precision against the old formulas over 2000 random views - and holds
  looking straight down the Earth's axis, where any tilt does and the spin makes up for it. The
  clouds leaned by an offset to that tip, which would jump there as the tilt does; each layer now
  leans by a turn fixed to the Earth. Frames while a pole crossed the middle changed by at most 2.67
  against 2.11 on average, and after drags, with time running on, by at most 0.21 to 0.65 in three
  runs, under twice their means.
- The night was one linear gradient through a straight line fitted to the terminator at two rows of
  the frame, true enough on the small cap of the lowest view and wrong for a whole globe, where the
  terminator is half an ellipse. It is now worked out per pixel from the sunlight on the sphere, 8
  at a time, the bands laid in like the land's; a pixel by the limb fades over a pixel as the bands
  pass any clip. The clouds read the same depth per pixel in place of their position along the old
  gradient, and the city lights the sunlight on each town. At the lowest view a dusk with the Sun
  behind the Earth turned darker toward the frame's sides, where the straight line had lit the land:
  shown to Oj side by side, "It's alright".
- The atmosphere was coloured by the limb point above each x across the frame. On the limb the
  sunlight is linear in the screen's position - the limb faces square to the view - so one gradient
  along the Sun's way across the screen colours the whole ring exactly, 41 stops from one side to
  the other.
- The stylized snow edge was held at no less than 0.12 and the bands passed over below 0.08. The
  floor never mattered in the north, where the snowline stays above 0.19, but it left Antarctica's
  low coastal ice green in the stylized look while the realistic look showed it white. The floor is
  gone, and 8 pixels below the lowest band still get the snow where the snow edge lies under them.
- The view leans back 36 degrees at the lowest and 1 at the highest, not 0: at 0 the view looks down
  the Earth's axis over a pole, where the spin and the tilt of OnOrbit are undefined. Crossing a
  pole with the whole globe in view, the largest change from one frame to the next was 0.36 against
  0.22 on average.
- A frame on the container at the lowest altitude: 1280 x 720 6.2 to 7.0 ms against 6.6 to 7.1
  before, 2560 x 1440 21.5 against 21.9 to 23.1. Halfway up, 9.9 and 33 ms: more of the
  globe in view, the land at finer steps on the smaller globe, and the space and the ocean drawn
  live. With the whole globe in view, 7.4 and 26.

**The clouds at night are moonlit** (Oj, 2026-11-08, "Make the clouds at night brighter"). Deep in
the night they kept a tenth of their colour, the rest the night's near black, so they all but
vanished. Of the options rendered at five views - a third of their colour (A), half (B), and A's
brightness drawn toward a blue-grey (C) - Oj picked C: each night stop's colour is drawn toward
MoonlitCloud by the night's own opacity before it is mixed into the clouds', so deep in the night a
cloud stands at about (88, 100, 124). The picked renders and the code's match pixel for pixel.

**The sea shows its depth** (2026-11-08). Oj asked for a "Depth map" and picked the ocean's depth,
from real bathymetry. The sea had been one gradient from the haze at the limb to the deep blue,
which stays until the map is built.

- The data is Natural Earth's 1:10m bathymetry: of its twelve layers, the contours at 200, 1000,
  2000, 4000, 6000 and 8000 metres. Each layer is unioned, simplified by 0.12 to 0.3 degrees, rid of
  rings under 0.05 to 0.5 square degrees and rounded to tenths of a degree as int16: 1003 rings and
  25,668 points. The data file grows from 76 KB of source to 413 KB, and the binary by about 140 KB
  with the code. Oj kept that detail over a coarser half.
- The levels are rasterized into the cover's 2048 by 1024 by where the rings' edges cross each row
  of texel centres, each texel taking the deepest level that holds it, counted evenly from 0 to 1,
  and blurred by three box passes of 5 texels. The depth rides in the cover's fourth byte, which was
  unused, so the map's memory stays at 53.4 MB; the build takes 3.77 to 3.85 s against 3.73 to 3.83
  on the container's 2 cores.
- The first blur ran 2 texels each way across every row, which near a pole covers almost no ground:
  the depth read round the pole an eighth of a degree out still varied by 15 percent of its range,
  and an orthographic view over the Arctic showed spokes from the pole. Across, the box now reaches
  as far over the ground as up - 2 texels over the cosine of the latitude, up to half the row -
  sliding along the row; there it varies by 3 percent, and the spokes are gone.
- The sea pass reads the depths at points twice as far apart as the land's and spreads them per
  pixel. Realistic looks the colour up in 256 shades between six colours; the haze toward the limb
  repeats the old gradient's, from 0.55 of the radius out. The stylized look cut its five tones in a
  table as well, each edge 0.01 wide; after the mockup, the edges took the land's rule, softened by
  k_edgeSoftening of what the depth changes over the pixel, so they do not step from pixel to pixel
  at the whole globe. The approved realistic frames came out the same to within 1 of 255.
- A frame at 2560 by 1440 on the container, before against now, realistic: 19.9 to 24.0 ms against
  21.5 to 23.9 at the lowest view, 26.6 to 29.8 against 26.6 to 29.6 halfway up, 25.4 to 31.8
  against 25.8 to 28.8 with the whole globe; stylized, 20.5 to 21.2 against 21.0 to 21.8, 24.7 to
  27.6 against 25.9 to 28.3, 24.5 to 24.9 against 26.1 to 27.3. The other scenes render byte for
  byte as before in both looks.

**One haze for the land and the sea** (2026-11-08). Oj, with a recording of a zoom: "Why is color
getting washed out as you zoom in?" The sea alone paled toward the limb's blue from 0.55 of the
globe's radius out, the old gradient ocean's haze carried over into the sea's depth. Low down the
frame shows only the globe's outer ring - the bottom of a 16:9 frame at the lowest view lies at 0.73
of the radius, of his 2354 x 1898 frame at 0.61 - so the whole sea was about half veiled, beside
land with no haze. At the lowest view the sea's mean colour measured (62, 126, 195) at a saturation
of 0.68, and (34, 84, 147) at 0.78 with the haze off. Shown a haze by the air looked through on the
sea alone, and no haze on the sea, Oj: "What I don't like is the "sea alone" part. Any applied hazes
should base on Relief number not on a name".

- The haze is one function of the air looked through past the air straight down, 1 over the facing
  less 1, times the air left above the height, e to the minus 1.1 times it, taken as 1 / (1 + x +
  x^2/2 + x^3/6). It veils a share x / (x + 7) toward the haze's colour, the old limb blue. The land
  takes it per point, from the facing and the height its look draws, and spreads it with the colour;
  the sea per pixel, at height 0. Air thinning as at a scale height of 8 km puts a third of it over
  the Himalaya's crest at 8.8 km.
- The clouds keep their fade at the limb by how far they face the viewer, which hides the shells'
  rim; they take no haze colour. The gradient drawn before the map is built keeps its own haze from
  the limb.
- Frames at 2560 x 1440 on the container, before against now: 27.8 and 28.4 ms against 29.5 and 28.3
  at the lowest view, 30.5 and 34.0 against 34.0 and 31.3 halfway, 35.5 and 32.5 against 31.2 and
  31.9 with the whole globe - within the noise.
- Greener shelf colours were rendered the same evening, from a touch of green to teal; no pick, and
  the shelf keeps its blue.

**The realistic shelves are as light as the land** (2026-11-08). With the haze no longer paling the
sea low down, Oj, with a screenshot of a shallow bay: "Sea beside coastline is glowing like a sorrow
eye". Measured on it, the shelf sea stood at a luma of about 130, flat, against 91 to 110 for the
land about it and 98 for the deep sea. Of three ramps rendered over four shelf seas - a darker blue,
a darker greenish, and one near the deep sea - Oj picked the greenish: SeaShelf 0x1E6880, SeaSlope
0x185C8C, SeaRise 0x13508F, SeaRidge 0x0F4689, with OceanDeep and SeaTrench as they were. The
shelf's luma is about 90. The stylized look keeps its light, map-like shelves.

**The realistic grassland is a step darker** (2026-11-08). Right after the shelves, Oj, with a
screenshot of a coast where forest gives way to grassland: "Here's the opposite, at the right half
of the coastline the land is glowing". Measured on it, the grass by the coast stood at a luma of
about 135 against about 85 for the sea beside it and 88 for the forest; there was no light fringe at
the coast, only the colour. Grass was luma 128 and GrassLight 141, about as light as the old shelf
blue. Of a step darker and darker still, near the forest, Oj picked the step: Grass 0x507B38 (luma
109), GrassLight 0x61853F (121), Steppe 0x929355. The forest, the tundra, the sand, the rock and the
snow are as they were.

**The realistic desert is a step darker** (2026-11-09). Oj, with a screenshot of Baja California and
its gulf: "Calm down that land". Measured on it, the desert stood at a luma of 157 to 170 against 93
for the gulf's shallow water and about 107 for the green land; Sand was luma 169 and SandLight 194.
Of a step darker, two steps, and two steps greyer, Oj picked the step: Sand 0xAB8D5C (luma 144),
SandLight 0xBBA377 (165). The steppe keeps the colour it took with the grassland.

**The land and the sea are moonlit at night** (2026-11-09). Oj: "brighter water and land colors at
night". Deep in the night the ground kept a tenth of its colour under the night's near black. Shown
a quarter of it, two fifths, and a quarter drawn toward a moonlit blue, Oj: "mock moonlight with and
without a shares of a quarter - a quarter", read as a quarter of the ground's colour without
moonlight and with a quarter of moonlight; then "with moonlight but darker that with a quarter", a
sixth and an eighth of moonlight; "Darker still", the ground and the moonlight lowered together;
"less ground", the moonlight held at an eighth and the ground at a sixth, an eighth or a tenth. Oj
picked a tenth: the ground's share is as it was, and each stop of the night over the ground is drawn
toward the moonlight by 0.125 / 0.9, so deep in the night, at 0.9 of opacity, the moonlight is an
eighth of the view. The moonlight is the colour the night clouds already take, renamed from
MoonlitCloud to Moonlight; the clouds are unchanged, and the stylized look takes the same night.

**The globe holds still while the viewer has it** (2026-11-09). Oj picked 5 seconds after the last
touch, with the motion easing back over 2. The scene keeps how long the globe has stood still, in
its own time, and its motion - the orbit, the Earth's turn, the sky's drift, the panels' turn, the
clouds' drift - runs that far behind the scene's time; the hovering, the blinking and the twinkling
run on the scene's time as before. A frame after a touch counts the time since the frame before as
still whole. After the last touch the stillness grows by the integral of 1 less the smooth step
between the frames, so it comes out the same however far apart the frames fall, and the motion eases
in with no jump in place or speed. Untouched, the stillness is 0 and every frame is bit for bit the
one before the change, which the harness confirmed at three times and for all the other scenes.

Measured in the harness at 320 x 180, 30 frames a second, as the mean change of the ground's pixels
from one frame to the next, second by second - before against now: a wheel at 0 s, 0.72 to 0.79
after the zoom against 0.00 from 1 to 5 s, 0.02 and 0.42 easing, 0.84 to 0.88 after; a press held
from 0 to 3 s, 0.00 to 8 s, 0.18 and 0.75 easing, 0.93 after; a drag for 1 s and its glide, 0.00
from 3 to 7 s after the glide stopped, 0.26 and 0.66 easing. The harness's view script gained "t0 t1
hold" and calls holdView on its drags as the panel does.

**The scene is the Earth** (Oj, 2026-11-09, "Rename Satellite to Earth, make realistic by default,
swap settings buttons to radiobuttons"). Oj picked the rename everywhere - the shown name, the
class, the files, the module, the project's items, the colours' namespace and the footnote's section
- with the craft still the satellite: SatelliteSprite, drawSatellite and the satellite in the scene
keep their names. The showcase picks Realistic when no pick is kept. Of the pickers on the Scene
page, only the look became radio buttons; Oj: "Only Look - Scenes will turn into tiles later".

**The lower two layers of cloud stand higher** (Oj, 2026-11-09, "there are 3 layers of clouds -
elevate the lowest two"). They stood 0.6 and 1.1 hundredths of the radius up, under the drawn
Himalaya's 1.4. The globe is drawn as from far away, so a layer's height only moves it out from the
centre by its height times the distance: raised to 1.3 and 1.55, the clouds moved about 5 pixels at
the lowest view in 960 x 540, and fewer than a tenth of the pixels changed. Of evenly spaced (1.0,
1.4, 1.8) and both above the ranges (1.3, 1.55), Oj picked the evenly spaced, worried that the
artifacts the rim fade answers might come back. They do not: each layer fades by how far its own
sphere faces the viewer, so its fade moves out with it. With the whole globe turning at full speed
at 1280 x 720, the pixels at the rim that jumped by more than 40 from one frame to the next were
0.055 percent on average and at most 0.119 before, and 0.055 and 0.121 raised; at 2560 x 1440 the
rim looked the same. Shadows cast by the clouds, and letting the layers reach the rim, were offered
as what would show the height; not taken.

**The clouds drift steadily east** (Oj, 2026-11-09, "Legible Earth clouds drifting"). The sine's
drift was too slow to see: at most 0.04, 0.06 and 0.15 degrees a second, turning back every two
minutes, while the ground slides under the view at about 1.7 to 2 degrees a second - 3 to 10 pixels
a second against about 115 in a 3436-pixel-wide window. The orbit has run on through the cycles
since SceneTime, so the drift no longer has to come round. Mockups, as 2 x 2 videos at real speed
against the sine (low realistic, low stylized, whole globe): A steady east at 0.2, 0.3 and 0.45
degrees a second, B twice as fast, C wind belts - west near the equator, east in the middle
latitudes, west near the poles - each layer read twice from its map at two ages 80 seconds apart and
faded between them, clouds shrinking out of one and growing into the other by the share of the
cloudy area each keeps, so the belts never shear the clouds into streaks. Oj picked A, then B: 0.4,
0.6 and 0.9 degrees a second, about a quarter to a half of the ground's own pace.

Each layer turns about its own axis by `driftPerSecond` times the scene's seconds. A positive turn
carries the map toward greater longitudes, the way the Earth turns, so the clouds go east. The angle
is the fraction of a turn, taken in double from the seconds, as the orbit's is. Frames match the B
mockup within 1 level at 20, 675 and 200,000 seconds in both looks; across the cycle's seam and each
layer's whole turn the frame-to-frame change rises only where the satellite's lights blink. The cost
is unchanged: the same turn, steady instead of a sine.

**The clouds drift on while the globe is held** (Oj, 2026-11-09, "Does the drifts stops when the
user in control - it shouldn't?"). The drift first ran on the globe's seconds - the scene's time
less the time the globe had stood still - and stopped with the globe: in the harness, with the
globe held after a wheel notch, fewer than 300 pixels below the satellite changed between 1 and 4
seconds, all of them twinkling lights. It runs on the scene's time now, as the hovering and the
blinking do, so the clouds cross the held ground. Untouched, the globe's time and the scene's are
the same, and the frames are bit for bit B's.

**The view starts four notches up, and is kept across runs** (Oj, 2026-11-09, with a screenshot at
3836 x 1971: "Make this zoom default. Remember zoom, position and orbit"). The screenshot's limb,
fitted as a circle through 29 points, stands at centre 1700, 2850 with radius 2261, against 1688,
2814 and 2215 at altitude 0.4 - the atmosphere's glow widens the fitted one - and the satellite's
place, 0.73 and 0.21 of the frame, is 0.4's within its hover; View::altitude starts there.

What is kept: the altitude, View::orbitTurn as an axis and an angle in degrees, read through the
rotation's quaternion, and the orbits of the last frame. Restoring moves View::orbitsAhead by the
stored orbits less the last frame's - none drawn yet, the start's 0 - and the orbit, the Earth's
turn, the sky's drift and the panels' turn all run from the orbits. The clouds' drift and the
twinkling run on the scene's time, which starts over. Dom keeps no double, so Orbits is a float,
written to 6 decimals: about a quarter of a thousandth of an orbit at 2000 orbits, some 100 hours
flown.

Measured in the harness through a config file in the showcase's format: a scene dragged and zoomed
at the start and drawn to 3600 seconds - the clouds' three layers and the 240-second cycle all come
round there - stored, read back into a fresh scene drawn at 0, at 960 x 540: 4,102 pixels differ,
by at most 2. At 3436 x 2051 on 2 cores the default view draws in 49-50 ms against 31-33 at the
lowest: below a globe radius of 3200 pixels the land and the clouds are worked out at every 2nd
pixel rather than every 4th, and the space is drawn live.

**The Earth turns either way** (Oj, 2026-11-09, the Turn choice, Eastward or Westward). The Earth's
turns were the orbits over 3 + phi; they are now m_earthTurnsAhead plus or minus that, by the way it
turns. A change of way moves m_earthTurnsAhead so the turns at the last frame's orbits are the same
either way, so the Earth turns on from where it stood, with no jump. The turns no longer follow from
the orbits alone, so the view keeps them as EarthTurns beside Turn, and restoring moves
m_earthTurnsAhead so the stored orbits give the stored turns. Eastward with nothing stored, the
turns are the orbits' quotient exactly, and the frames are bit for bit those before (three times,
both looks). Switched westward at 10 seconds into a run at the whole globe, the step from one frame
to the next stays at 0.17 through the switch and the frames then part from the eastward run's
steadily; through a config file, a run switched at 1800 seconds and stored at 3600 restores within
2 levels, 3,597 pixels.

**The let-go decides how the satellite flies on** (Oj, 2026-11-09, "Remember which way the
satellite is flying. The decision is made when the user releases the mouse button. if the globe is
still at that moment hang the satellite still over the selected point - only day and night will
change. if the globe was moving - change the orbit and continue move in that direction"). Of the
picture rolling so the new track runs across it, and the track running the way the fling went, Oj
picked the second; of the fling's pace and the orbit's, the orbit's; of the 5-second hold after the
release and deciding at once, at once - the wheel keeps its hold.

- **The track angle** turns the view about the vertical under it, in the leaning view's own axes
  (0, sin b, cos b), b the lean back: orbitAxesAt takes it after the lean, so the horizon stays at
  the top. At 0 the rotation is exactly the identity, and the frames are bit for bit those before.
- **A fling re-anchors the orbit on the picture as it stands**: orbitTurn becomes the view's axes
  times the inverse of orbitAxesAt at the shown orbits and the new angle, so the next frame is the
  one before, and the satellite flies on from it. The angle is solved, not searched: the held
  point's motion across the view is the orbit's part, which turns with the angle (a cos + b sin of
  two motions worked out at 0 and a quarter turn), plus the Earth's turn's part, which does not;
  the angle at which the sum lies along the pointer's way is the root of a cos + b sin + c = 0 with
  the larger motion along it.
- **Hanging, the view turns with the Earth**: the view's axes are the Earth's turned axes times
  hangingCamera times the lean, so where the Earth's axes stand in the view does not change, and the
  Sun moves across it. A drag while hanging turns hangingCamera by the drag's turn carried through
  the lean.
- **The orbits stand still while it hangs**, with the sky's drift and the panels' turn, so the
  Earth's turns could no longer be read off the orbits: they are now the globe's time, counted in
  orbits, over 3 + phi, which is the same number as before while nothing has hung (the frames are
  bit for bit those before).
- **Drags no longer touch the view**, as the host's glide reaches the scene as drags after the
  let-go and would have held the globe 5 seconds; a press holds it through holdView, and the wheel
  still touches.

Measured in the harness at 960 x 540, from the default view: let go still at 30 seconds, an ice
crop by the held point moved at most 0.11 pixels over the next 57 seconds while the night crossed
the frame. Flung at 300 pixels a second in eight directions, the ground at the held point moved at
0.1, 45.3, 90.8, 134.7, -179.9, -135.6, -90.2 and -42.9 degrees against 0, 45, 90, 135, 180, -135,
-90 and -45, measured half a second after the release. Dragged while hanging by (60, -30) and
(-50, 40) pixels, the ground there moved (60.5, -31.1) and (-49.4, 39.5). Through a config file a
run flung at 900 seconds, switched westward at 1800, and then hung or flung again at 2700 and stored
at 3600 restores within 1 and 2 levels.

**The realistic coast is blurred as much as the land is** (Oj, 2026-11-09, "I got what's wrong with
the coast lines. That's not the colors at all. They are too sharp. The terrain itself has lower
resolution and is blurred, but the coastlines are razor-blade sharp"). Each edge was measured as
its contrast over its steepest slope along its normal, on frames of 2296 x 1198 without clouds; on
the same probe an unblurred edge of 1 pixel reads 1.4, and a Gaussian blur of spread 1 pixel reads
2.7, of 2 pixels 5.2. The coast read 1.5 to 1.7 at every altitude. The land's own steps read 2.5 to
3.8 at their sharpest tenth and 3.7 to 4.7 at their middle four notches up, 2.2 to 3.5 and 3.4 to
5.2 at the lowest, and 1.6 to 1.8 and 1.9 to 2.4 with the whole globe in view. The land is worked
out at points a step apart and read 1.4 steps wide, which blurs it by about a step over the square
root of 2; the coast was cut at the pixel. Mockups blurred the coast by half a step, 0.71 and a
whole step; Oj picked half a step, for the realistic look alone. The stylized look cuts its zones at
the pixel, and a soft coast stood alone among them.

- **The coverage is worked out for the band and the blur's reach beyond it** - 3 spreads, rounded
  up - on every side, each band taking the edges of that many rows above and below, and is then
  blurred across and down. Against one Gaussian over the whole frame's coverage it is within 0.84
  of a level away from the limb and the frame's edges, so the bands leave no seams.
- **Only the groups of 8 pixels the blur changes are worked out.** coverBand reports each group as
  clear, whole or partly covered, and a group whose blur reads only groups of one clear or whole
  cover takes that cover; unevenGroups finds the rest 32 groups at a time from the covers' bytes.
  Blurring every group cost 8 ms of processor time a frame at 3436 x 2051 on the container, against
  3 for the coverage unblurred; this costs 3.5.
- **The loops along the rim are closed past the sphere**, the blur's reach and a pixel out, and the
  limb is cut by the sphere's own coverage, as the sea's is. Closed at the rim, the blur would fade
  the land into the sea along the limb. The first mockup closed them past it without the cut, and
  the land showed haze-blue past the night side's limb.
- Measured on the container (2 cores), the land bands' processor time a frame: at 3436 x 2051, 22 to
  26 ms became 29 to 30 four notches up and 17.7 became 22.1 at the lowest; at 2296 x 1198, 10.9
  became 12.9. Besides the blur itself, the land's points are worked out past the coast as far as
  the blur reaches.
- The frames match the picked mockup within 11 levels, on pixels of the limb where a coast crosses
  it; the stylized look is bit for bit as before. The measured coast now reads 2.9 to 3.2 below the
  whole globe and 2.2 to 2.5 with it in view.

**A double click moves the Sun a quarter of the way round** (Oj, 2026-11-09, "double click to
advance day/night cycle"). Of moving the Sun round the Earth's axis and moving the time on, Oj
picked the Sun, as the time would carry a flying satellite over other ground; of half a day swept,
a quarter swept and half a day at once, a quarter swept. The Sun stands at k_sunAround plus its
turns, and the Earth's turn moves a point fixed to it the same way round, so the Sun goes back a
quarter for an eastward Earth and on a quarter for a westward one. The glide is the wheel's: two
thirds of the way in 0.25 seconds of the scene's time, and drawn at the end once within 0.0005
turns of it. stillSecondsAt was split into secondsSinceLastFrame and stillSecondsAfter, so the hold
and the glide read one frame time.

Checked in the container harness, at 960 x 540: frames without a double click are bit for bit those
of b800cba, flying and hanging, in both looks. Hanging at 300 seconds, a double click lights the
ground after a second as 207.8 more seconds of the Earth's turn light it, a quarter of its 831,
for either way of turning; the clouds stand apart, as they drift on the scene's time.

## ChaseScene

**The camera stops differently in the two episodes**: over 1.4 seconds after the log, within 0.4
seconds at the branch. With the log's stop in both, the wolf came to rest beyond the tree in the
second episode. Where he lies is measured from the obstacle for the same reason. The camera
stands until 0.82 of the episode and is back at full speed at 0.9.

The mountains and hills are single paths across the frame, their heights sums of waves that each
repeat a whole number of times per period. Drawn tile by tile, the antialiased edges of two tiles
overlapped and showed a seam where they met.

On his back, the wolf's head turned into the ground until `headTurn` took `0.7 * onBack`.

**The second episode's run** (Oj, 2026-11-09: "the wolf accelerates, gets dangerously close to
the hare, then a bit lags behind and with a great effort makes the final jump to the Hare - and in
the middle of the air meets that nasty branch"). branchRunAt states it as eased stretches, in
shares of the episode: closing in from 0.14 to 0.29, a snap at her tail, dropping back by 0.41,
coming on again from 0.43, a crouch, the takeoff at 0.555 and the branch at 0.58, where the camera
starts to stop as before. The leap is a parabola whose top would come at 0.585, so the branch meets
him just short of it, 62 units up. Everything from the camera's stop on keeps its timing.

The branch was raised to the height of his head in the leap - its end 143 to 150 units over his
feet, where it was 68 to 74 - and lengthened, so its end stands clear of the crown. The hare passes
under it without ducking, and the running wolf would have too. At the contact its end sits at his
nose. Placed nearer, it showed behind his head, as if run through it.

Positions are chase units, which a narrow frame scales down, while the sprites keep their size. At
500 x 700 the wolf's closest approach overlapped the hare, so his gap to her and the branch's
reach from him are divided by `closeness`.

The hare bolts 50 units ahead as he closes in and gives 30 back as he drops back (hareBoltAt).
In this episode her run ahead after the stop starts at 0.57 rather than 0.5, so she does not draw
away while he is in the air. WolfPose gained `leap` for the pose in the air: the forepaws reach
down ahead and the hind legs trail, which sprawl's legs stretched flat did not show.

**The jaws in the leap** (Oj, 2026-11-09: "when he leaps he's opening his snout showing the teeth
so big that you wonder how they were fit with the snout closed"). WolfPose gained `jawOpen`: the
snout lifts 0.45 rad and the lower jaw drops 0.9 about the back of the mouth, the mouth between
them is filled dark, and the teeth grow out of the gums with the opening, to a fang on each jaw
longer than the snout is deep. Rendered three ways - fangs, a row of saw teeth, block chompers -
and Oj picked the fangs. A first try opened the lower jaw alone, 0.95 rad, with teeth up to 16
units: they were longer than the gap and crossed into a jumble, so the snout lifts too and the
fangs stand at different places along the two jaws. The jaw opens from 0.558 to 0.569 of the
episode, and the branch shuts it within 0.004 of the contact. With the jaws shut the wolf draws
as before, pixel for pixel.

The snap at his closest opens the same jaws (Oj, "The snap too"): from 0.286 of the episode they
part on her tail, and shut from 0.297 to 0.2995, as the lunge ends.

**The hills stand back** (Oj, 2026-11-09, "Make the layer before mountains duller", then C of
three renders - greyed, hazed a third of the way, hazed past halfway): the hills and their trees
are their greens taken 0.55 of the way to MountainFoot. Their trees have colours of their own, so
the big tree of the second episode keeps its greens.

**The first episode's obstacle** (Oj, 2026-11-09: "Cycle between a few first run obstacles - a
log, an apple, a boot, a hedgehog", then "a hedgehog turns into a ball after the hare passes, after
the wolf trips swiftly walks away"). k_firstRunObstacles is taken in turn by cyclesPassed, and the
turn changes while the obstacle is off screen. Each is drawn in units about where it meets the
trail; the log, moved into that frame, draws pixel for pixel as before. The wolf trips and the
hare jumps the same way over all four. The hedgehog's face was first the trail's tan and vanished
into it, and the hedgehog read small, so the face is a darker brown and the whole a third larger.
Its outline runs over the top from left to right and, as it curls, on round under the body - a
whole turn for the ball - while the face shrinks into the spines about the neck. It curls from
0.002 to 0.012 after the hare passes over it, unrolls from 0.598 to 0.61, and from 0.612 walks off
to the left at 160 units a second (k_hedgehogWalk), its feet pattering every 9 units.

Then (Oj, "a rubber duck which deflates on the wolf's impact, a ball that only rolls off a little
after the collision") two more turns. The duck deflates from just before the trip to 0.02 after
it: it is drawn scaled 1.2 wide and 0.38 high at the end, about where it meets the trail, its head
drooping 0.5 rad, and three streaks of air behind it at 4 d (1 - d) of their colour. A first try
took the streaks' strength from a bell, which never reaches 0 and left them showing at rest. The
ball rolls k_ballRoll (40 units) in 0.05 of the episode, easing out, turned through the distance
over its radius; the roll is folded into its place on the trail as the hedgehog's walk is.

Then (Oj, "a patch of grass") a seventh: eleven blades, the back ones darker, each bending from its
root by its own tilt plus bladeLean, more for a taller blade. Unasked, the grass reacts: it is held
flat, 1.1 rad, for 0.012 of the episode while his legs are in it, then springs back - a cosine of
0.02 dying away over 0.015. A first try let it go at once and it flicked back while he was still on
it.

**Two obstacle runs, then the tree** (Oj, 2026-11-09, asked for one more obstacle and picked the
rake from four offered, then "swap the log with banana peel, then loop 2 obstacles one tree",
confirmed as three runs a cycle). The cycle is 60 seconds (k_cycle, cycleDuration) and three
episodes; k_branchEpisode is the last. The camera stops are k_tripStop and k_branchStop, chosen by
the episode; travelAt adds the earlier episodes' shares and periodNear the whole cycle's, so every
layer's period moved a little and the landscape is no longer the 40-second one. The runs before the
tree take k_trailObstacles in turn by cyclesPassed x 2 + episode. The log, its colours and drawLog
are gone; its dark brown stays as Bough, for the branch.

The banana peel flies 70 units ahead over 0.045 of the episode, easing out, up a parabola of 60
units and through two turns, and lands flat. The first peel had thin flaps that read as sticks; it
is half again larger with fuller flaps. The rake turns about the back of its head: its handle rises
1.75 rad over 0.006, drops back from 0.012 to 0.024 and bounces 0.15 rad. Drawn under the wolf, the
handle passed behind him, so while it swings it is drawn over him. His face is ahead of the tines
when they are trodden on, so a fixed swing crossed it for a frame; from 0.001 to 0.01 after the trip
the swing is drawn toward the angle that lays the handle across his snout (rakeOnFace), and his head
is knocked back 0.35 rad. At 80 units the handle stopped short of his face; it is 110.

**The boot falls over, the apple trades places with it, and the tree bears apples** (Oj, 2026-11-09:
"The boot falls on its side", then "swap the boot and apple. Make the tree an apple tree and apples
fall when the wolf hits it"). Caught at the toe, the boot turns about the back of its heel, a
quarter turn in 0.012 of the episode with the angle growing as the square of the time, and rocks
back 0.12 rad; in a side view, lying on its back is the nearest to on its side. The tree carries six
apples, the trail's apple at 0.55 of its size (k_treeApples). Four let go 0 to 0.006 after the wolf
hits the branch (sinceHit), fall at k_appleGravity, 1200 units a second a second, with
k_episodeSeconds turning episode shares into seconds, bounce once to a third of their landing speed
and lie on the trail; two stay up.

**The apple splits** (Oj, "The apple reacts at the wolf impact too" - the split was my pick, with a
squash or a kick offered). From 0.001 before the trip to 0.008 after it, each half leans 0.55 rad
out about its outer foot and slides 3 units apart; its cut, seen turning toward the viewer, is an
ellipse that widens from a line to 5.3 units, cream, with two seeds. The stem and leaf stay on the
right half. drawApple takes the split; the tree's apples pass 0 and draw whole, as before.

**The hare takes an apple** (Oj, "The hare grabs an apple that has been dropped from the tree", then
"showing her teeth too"). The run's next episode starts with her empty-pawed, so she eats it before
she runs on. k_hareApple is the fallen apple nearest where she sits; restingPlace gives where it
comes to rest, and from that the scene works out where her feet go for her forepaws to be over it.
HareApple: she hops back from 0.688 to 0.708 (a hop of her own, the sitting let go for it), lifts it
to her chest by 0.716 - the tree stops drawing it from 0.708 - and takes five bites 0.02 apart from
0.73. At each bite the apple rises 11 units to her teeth and comes back smaller, its size the square
root of what is left, and four crumbs, skin and flesh, fly from its top under k_appleGravity. A
first try bobbed it 6 units, which stopped well short of her mouth. HarePose gained holding (the
forepaws turned to 2.4 rad, up at the chest) and teeth (two front teeth under the nose, up to 6.5
units long, over a dark mouth): 0.4 while she holds the apple, full at each bite. Both 0 draw her as
before.

Her place was first worked out from where the apple lay at that moment. Once the camera moved on,
the ground carried her back toward the wolf, and her hop, which follows the ground she covers, stood
still in mid-air (Oj: "she turns around, jumps and freezes in the air while the wolf getting really
close, that looks strange"). The apple's place is taken while the camera stands (treeWhileStanding),
and from 0.86 she runs on from there as before.

**The wolf closes in over the trail** (Oj, "On the first runs, the distance between them keeps
shortening until he hits an obstacle"; of two ways he picked the one where both of them show it). He
came up 60 units by 0.5 and held there, 290 units behind her. He now runs from k_wolfChasing to
k_wolfBehindHareAtTrip (150) behind her feet along the square of the run's share to the trip, so he
gains ever faster, and his nose is half a hare short of her tail as his paw finds the obstacle. That
gap is held in pixels, divided by closeness, as the branch run's closest one is.

The obstacle still sits 30 units ahead of him at the trip, so she clears it later (0.532 rather than
0.511), and he lands 70 units on from where he tripped, nearer where she sits. She starts to run on
ahead at the trip rather than at 0.5, where her pulling away undid his last gains. His strain
follows what he has gained - head down, eyes on her - and lets go over the first fifth of his fall.
Her alarm rises from 0.3 to the trip and goes over the next 0.03 - ears flat, longer hops.

Landing 70 units on from the trip put him 180 units short of where she sits, against 320 before (Oj:
"the distance should restore when he tripped - she keeps moving and stops at the safe distance"). He
now tumbles to k_wolfDownAfterTrip (-70), where he lay before: the camera keeps slowing over his
fall, so he still goes 160 units on over the ground, and she runs on from the trip to sit where she
did. The obstacle lies just behind his tail, and the rake's handle reaches under the tip of his
tail.

**What stands by the path** (Oj, "in the middle of the first run put a casual object on the path
side, which does NOT slow them down. After the first tree encounter that should be an apple"). Of a
toadstool, a signpost and a bucket he named his own - a boulder, a grass tuft ("reeds from the mill
scene can be reused"), a larger flower, a bird that flies away - then "All four to take turns, the
apple fits between the first tree encounter and the obstacle apple, other gaps will take other
objects". A round of four cycles has twelve encounters, so twelve gaps; he filled the seven left
with the toadstool, the signpost, a frog that hops off the path, a snail, a butterfly that flutters
up, a banana peel and a tree stump, the peel as a warning ahead of the one on the trail. The order
in k_roadside is mine: the creatures each open a run to the tree, the reeds come before the duck and
the boulder before the ball.

Each passes the middle of the frame at 0.28 of its run, or 0.12 before the branch (the wolf surges
from 0.14). Its clock, sinceHareClose, starts as the hare comes within k_hareClose (70) and is
worked out from where the hare runs before the trip, so it needs no state. The draw loop also takes
the next run's thing, placed by the camera's travel to the run's end, so in a frame wide enough to
show it early it comes in from the edge rather than appearing at the run's start; at 32:9 the tree
run's thing comes into view at 0.95 of the run before. Reusing the mill's reeds took two fields on
ReedsPose, lean and inWater, whose defaults draw the mill's as before. The gust follows hareX and
wolfX, each passing leaving a wake that bends the reeds and the daisy after them.

Then "the road sign may revolve around": the board turns about the post, its x scaled by the cosine
of the turn with a floor so it stays a sliver edge-on, shaded as it turns away and plain on the
back. Of ending as it was (three turns) or pointing back the way they came (two and a half) he
picked the second. "the boulder may bounce": three hops, small after the hare, high after the wolf,
small again; first timed as each passed, they were hidden behind the runners, so each comes just
after one has cleared it.

Then (Oj) "banana peel hopes like a boulder, toadstool umbrella hopes when the hare passes, when the
wolf passes lands beside the leg, the flower sway when the wolf passes loses petals". hopAt and
hopping are the boulder's bounce, shared with the roadside peel, its heights in units of the verge
(the boulder's 5, 16 and 5 at its 1.35 scale); the peel gained a shadow for it. The cap hops on the
stalk 0.34 s after the hare comes close and leaves it at 1.32 s, flying 0.45 s to lie 19 units on
with its rim on the ground, turned 0.3 rad back. Five petals leave the daisy from 1.12 s, 0.035 s
apart, while the wolf hides the head. They start from where they would be at a nod of 0.48, the
height of his wake, so the step from the real nod is hidden behind him; they fall at 18 units a
second, gathering 28 a second each second, drift 55 a second after him and land in about 1.2 s.

Then "The warning apple jumps, reeds lose their fur": the apple takes hopAt like the peel. ReedsPose
gained fur, which narrows each head to 0.35 of its width at 0, and the scene takes the reeds from 1
to 0.35 over 1.0 to 1.3 s, while the wolf hides them. drawReedFluff tears four tufts from each head
between 1.02 and 1.27 s, each at its own pseudoRandom time, speed (45 to 75 units a second), rise
and size, from where the head would be at a lean of 0.5; they thin out over 1.6 s. Released at one
time and speed, the first tufts flew in rows like strings of beads.

Then "while the wolf rests the beach ball slowly rolls off". It first rolled back down the trail
from 0.64, gathering speed until the camera carried it away; Oj: "not completely rolls off just a
tad and stops naturally", so it rolls back 30 units over 0.64 to 0.74, eased in and out.

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
