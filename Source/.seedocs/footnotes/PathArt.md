# PathArt

The words that do not fit above a declaration.

## Scene

A scene is a moving picture, drawn at a time into any bounds. Most of its motions run through a
cycle and come back to where they began. The MillScene showcase holds a list of scenes, offers them
by name() in the Scene group of its Scene page, and stores the one picked under that name.

**A motion that comes round runs a whole number of times a cycle**, so the frame at the end of the
cycle is the frame at its start and the loop shows no seam. A scene that needs a slow motion beside
a fast one gives itself a longer cycle - cycleDuration(). A motion meant never to come round, such
as the Satellite's orbit, runs on the cycles passed as well - see SceneTime.

**The showcase owns the clock.** It advances the cycle time by the time since the last frame over
cycleDuration(), wraps it back into 0 to 1, and counts each wrap in the cycles passed. A scene
keeps no time of its own.

**A scene draws its figures as sprites and its weather as effects** - free functions that take a
SceneFrame and a struct: a sprite's pose, an effect's extent. Each takes the time its motions run
on, and comes round when that time runs a whole number of units a cycle.

## Scene draw

Paints the whole of bounds, so nothing needs to be drawn under it.

**A frame depends on its SceneTime, its bounds, and the look and the view the scene keeps.** The
showcase jumps between scenes and times - a hover preview, a pick, a resize - and every scene but
MillScene draws the same picture for the same of these, whatever was drawn before. MillScene keeps
its theme's crossfade between frames.

**Any size and any shape of bounds is drawn.** A scene lays itself out in the units of a
SceneFrame, so it keeps its proportions at any size; a narrow frame draws the figures closer
together, or smaller, rather than cutting them.

## Scene prepare

Readies what the scene keeps between frames - its layers, see SceneLayer - for the params it will
be drawn with. A scene that keeps nothing leaves it as it is.

**Call it before draw, whenever there is time for it.** It costs nothing when the params have not
changed. A frame drawn with params the scene was not prepared for comes out the same, drawn live,
so a host may leave it out while a resize changes the size from one frame to the next. The
MillScene showcase prepares on the first frame drawn at an unchanged size.

## SceneParams

What a scene's frames depend on besides the time: the bounds it is drawn into, the canvas's
transform from the bounds' coordinates to pixels, and the backend that draws them.

## SceneLook

How a scene that offers a choice of looks draws itself. Stylized is the flat drawing every scene
has; Realistic shades what it can the way a photograph shows it. A scene answers offersLooks() true
when the two differ for it - today the Satellite, whose clouds and land they change - and draws in
the look setLook() last named from its next frame on. A scene that offers no looks keeps the value
and draws as it always does.

The MillScene showcase offers the looks in a section of its Scene page captioned with the scene's
name, shown only while that scene is the picked one, and keeps the pick under SceneLook.

## Scene offersView

**A scene that offers a view keeps it, as it keeps its look**, and moves it as the host hands it a
drag - dragView(), in the coordinates of the bounds it draws into - and the wheel's notches -
scrollView(), counted as MouseWheelEvent counts them, away from the user above 0. What the view is
and how it moves is the scene's own: the Satellite turns its globe and climbs above it - see
SatelliteScene. A scene that offers none leaves both as they are.

**The MillScene showcase hands them on from the panel the scene covers.** A press stops what moves
the view. A drag let go glides on at the speed of its last 80 milliseconds and slows to about a
third of it every 0.35 seconds, and the wheel's notches come in over about a tenth of a second.
Another scene shown stops both. Each scene keeps its view while the showcase runs.

## SceneTime

**cycleTime is how far a frame falls into its cycle**, 0 to 1, and cyclesPassed counts the whole
cycles before it since the showcase started. Run backward past the start, the count goes below 0.

**A motion that comes round reads cycleTime alone.** One that never comes round reads both, as
cyclesPassed + cycleTime cycles summed in double: a float holds the time of a long run too coarsely
for the motion to stay smooth.

## SceneFrame

The shorter side of the frame is 600 units, and `unit` is the pixels in one. width and height
are in pixels.

**within() puts a local transform ahead of the frame's own**: what is drawn through the frame it
returns is placed by `local` first. Stroke widths, glow spreads and gradient points given through
it are scaled with it, so a figure drawn about its own origin through `within(Translation *
Rotation * Scale)` comes out whole.

## ScatteredShapes

Draws shapes in one brush - stars, rocks, rivets, blades of grass - the way the backend draws them
fastest. Call next() for each shape and build the shape in the path it returns, then call draw()
once at the end. A shape may be several figures, such as the petals of one flower.

**On the CPU backend each shape is filled on its own.** A fill there costs in proportion to the
area its path spans, so shapes scattered across the frame cost far more in one path than one by
one. next() draws the shape before the one it begins.

**On Direct2D the shapes gather in one path and go down in one fill at draw().** Each fill there
builds a geometry of its own.

**The shapes must stand apart.** A path is filled even-odd, so two shapes that overlap in one path
cut a hole in each other. On Direct2D they share a path and show the hole; on the CPU backend they
are filled apart and do not.

**A kind of shape that lies over another goes down after it.** With two ScatteredShapes open at
once, the CPU backend draws their shapes interleaved, in the order next() is called. Build the
lower kind everywhere and call its draw() before the upper kind's first next().

## SceneLayer

A part of a scene that does not move, drawn once into pixels and copied into every frame.

- **prepare** draws the part on a canvas of its own and keeps the pixels it covers for the params,
  a band of 32 rows at a time, each band cut to the columns drawn in it. It does nothing when they
  are kept for those params already.
- **draw** copies the kept pixels in, at the part's own place in the paint order, so a layer over
  moving parts is drawn after them. Where the frame does not fit the params the pixels were drawn
  for, it draws the part live.

**The drawing is a function of the frame alone.** It reads no time and nothing else that changes
from frame to frame, so the same params give the same pixels.

**A layer pays where its part costs more to draw than to copy**, and copying is cheap:
translucent pixels are laid in eight at a time as opaque ones are, and only the bands the part
covers are read, each from its first drawn column to its last. A part spread thin - rings, an L
along two edges - keeps little more than its own pixels. A plain gradient costs about the same
either way.

**Only the CPU backend keeps pixels.** Direct2D copies a bitmap up to the GPU on every draw, so
there a layer draws its part live.

**A path clip does not reach the kept pixels.** They are laid in as drawPixelViews lays its views,
which on the CPU backend stops at the clip's rectangle only.

**The bands are laid in on all the cores at once** - see Canvas drawPixelViews.

## CloudKind

What a cloud map's clouds look like:

- **Puffy** - heaps of cumulus, bunched into fields with clear sky between and lit unevenly inside.
- **Patchy** - broad sheets of stratus with soft edges, curling where the noise is warped.
- **Streaky** - thin streaks of cirrus along the circles of latitude, gathered into broad bands.

## CloudMap

How much cloud lies over each point of a sphere, 0 to 1, kept at 2048 points round each circle of
latitude and 1024 from pole to pole, and at 7 coarser levels of detail, each half as fine each way
as the one before: 2.8 MB a map. A map is cut from noise sampled on the sphere itself, so it has no
seam where the longitudes meet and no pinch at the poles. The cut is set so that the share `cover`
of its points holds cloud. One kind, seed and cover always make the same map.

**A map takes about a second of processor time to build**, spread over all the cores. Build it
off the paint.

**readCover() reads each point as fine as the ground it stands for** - see MapQuery - between the
two levels nearest it, as readLand() does. The reads wrap round the longitudes any number of turns
either way and stop at the poles. readCover() works through eight points at a time with AVX2, and
several threads may call it at once.

## EarthMap

The land of the whole Earth as maps, from pole to pole: how high it stands and what covers it.

- **The relief** is laid in from 74 ranges - the Himalaya, the Andes, the Alps, the Transantarctic
  Mountains and the rest - each a line along its crest with a height and a reach, and from 12
  plateaus such as Tibet, the Altiplano and Greenland's ice, over low hills everywhere and a dome of
  ice over Antarctica. Noise cuts the ranges into ridges and makes their edges wander. A height of
  1 is the Himalaya's crest.
- **The broad relief** is the same relief read at no more than a seventh of its finest detail,
  which smooths it to the ranges' broad shapes.
- **The cover** is the dryness, from 33 dry regions such as the Sahara, the Atacama and the
  Australian deserts with noise along their edges, and the shares of northern forest and of tundra,
  which end along wandering lines of latitude.

The relief keeps 4096 points round each circle of latitude and 2048 from pole to pole, and the
cover half as many each way. Each keeps 7 coarser levels of detail as well, each half as fine each
way as the one before, its points the average of the 2 by 2 under them: 53 MB in all. One build
always makes the same maps.

**Building the maps takes about 7 seconds of processor time**, spread over all the cores. Build them
off the paint.

**readLand() reads each point as fine as the ground it stands for** - see MapQuery. It takes the
two levels whose points lie about as far apart as that ground reaches, blends the four nearest
points in each, then blends the two levels by how near each is. Detail finer than the points can
hold fades out, so it does not shimmer as the view moves.

The reads wrap round the longitudes any number of turns either way and stop at the poles.
readLand() works through eight points at a time with AVX2, and several threads may call it at once.

## MapQuery

The four spans run in step, one value per point, and the longitudes set how many points are read.

**A point's spans are how far the ground it stands for reaches**, in longitude and in latitude - for
a point drawn as a pixel, the ground under that pixel. The greater of the two, counted in the map's
points, picks the level of detail the map is read at. A span of 0 reads the finest.

## LandPoints

Each span holds a value for every point read, and must be at least as long as the query's
longitudes.
Heights and slopes are in the map's own units: how high a height of 1 stands as drawn is the
caller's to choose.

## LandOutline

A coast as one closed loop, its last point joined back to the first: x is the longitude and y the
latitude, in degrees. It runs counterclockwise round land, seen from outside the Earth or on a map
with north up, and clockwise round a sea inside the land. Antarctica's closes along the south pole,
from 180 degrees west to 180 east. Where the grain pushes a coast past the 180th meridian, its
longitudes reach a little beyond 180 degrees.

## landOutlines

**The coasts are Natural Earth's land at 1:110m**: the 117 outlines that enclose at least 0.4
square degrees, Antarctica's among them, and the Caspian Sea inside them. Natural Earth is free
vector and raster map data at naturalearthdata.com, in the public domain.

**Each outline gets a point every 0.1 degree of arc**, about 34,500 in all, and each point moves by
noise fixed to where it stands on the sphere: a broad warp of up to about 0.35 degrees bends the
coast, and a fine grain of up to about 0.22 degrees roughens it. Where Natural Earth cuts the land
at the 180th meridian, a point two outlines share moves alike in both, so they stay joined. No coast
folds over itself.

**One call takes about 16 milliseconds of one core**, and gives the same outlines every time. Call
it off the paint.

## reachToward

Bends a two-part limb so that its end reaches `target`, by the law of cosines.

- **A target out of reach** pulls the limb nearly straight toward it, and the end stops short.
- **A target too close** holds the end at the nearest distance the two parts can fold to.
- **Two joints answer every reach**, mirror images across the line from root to end. BendToward
  picks the one further left, further right or lower down.

## calculateSyncRotation

The spin of a falling flake or leaf, in turns. An even index turns clockwise by one
`symmetryFactor`th of a turn a cycle, an odd one the other way and twice as fast, so a shape with
that symmetry looks the same at both ends of the cycle. The index adds whole turns, which change
nothing on screen.

## StarField drift

**A star moved out of the band comes round into it at the far side**, so a drift of whole frame
widths and band heights draws the field as it stands at none. Keep the drift to its part past the
whole numbers: a float holds a large drift too coarsely for the stars to move smoothly.

## PuppeteerScene

An android on a small stage works a marionette under a spotlight, between swaying curtains.

The dance is 16 sways a cycle, with a step on every half sway - see danceMarionette. The android
sways with the puppet and holds its control from the right.

## danceMarionette

Places the control for one moment of the dance and hangs the puppet from it.

- **The main bar tilts with the sway** and carries the head and hand strings, so each tilt lifts
  one hand.
- **The leg bar hooked under it rocks twice a sway** and lifts each knee in turn, as the feet step.
- **The arms and legs are reached from the strings** with reachToward; the control stands 284
  units above the feet, over the puppet's head.

## GhostScene

A ghost loops a figure eight twice a cycle over a graveyard by moonlight, leaning into its path.

Clouds cross the moon, bats fly through, a jack-o'-lantern flickers and two fog banks drift in
front. The ground under the ghost glows the more the nearer it flies.

## ClockworkScene

A pendulum escapement drives a train of brass, copper and steel gears on a riveted plate, with a
steam pipe, a valve and a pressure gauge.

**The pendulum swings 20 times a cycle**, and every crossing of the middle lets the escape wheel
turn one tooth. Each gear's teeth and spokes are counted so that it turns a whole number of teeth
and of spoke spacings in a cycle, and the train stands where it started when the cycle ends.

The valve lets off steam four times a cycle. The gauge's needle wanders three times a cycle and
trembles as it goes.

## CowColors

**The horns, the tail and the legs keep clear of what stands behind them.** They are drawn in
`detail`, or in `body` a shade darker where that has clearly more contrast against `skyBehind` or
`groundBehind` - a near-black detail against a night sky turns pale. The two blend near the turn,
so a theme crossfade carries the change smoothly.

## OrreryScene

The Sun, six planets and an asteroid belt on an orbit plane tipped toward the viewer.

**The cycle is 240 seconds**, set by Saturn, which goes round once. Jupiter goes round twice, the
belt three times, Mars 4, the Earth 6, Venus 8 and Mercury 12; the Moon goes round the Earth 16
times. The stars twinkle and the Sun's rays flicker at the pace they would keep in a 40-second
cycle.

Each planet is lit from the Sun - see shadeBall - and is drawn larger on the near half of its
orbit. What stands beyond the Sun is drawn before it, what stands in front after it.

## shadeBall

Fills the part of a ball's disc that faces away from the light, inside a clip to that disc.

The dark part is the half disc away from the light, closed by the terminator: a half ellipse as
wide as the light comes toward or away from the viewer. Lit from behind, the lit side is a
crescent; lit from the front, a crescent of shade.

## CometScene

A comet falls across a night sky above a lake, twice a cycle - from the left onto a peak on the
right, then from the right onto a peak on the left.

**A pass is 60 percent fall and 40 percent glow.** The comet sets off across the sky and drops
behind the near ridge, its dust tail fanning out along the curve it came by and its ion tail
running straight back. Where it lands, the sky behind the ridge lights up and fades.

The lake shows everything above it upside down and dimmed, with ripples running across. Three
shooting stars cross the sky in the quiet between the passes.

## CometPose

**A pass is the comet's whole visit.** It falls along its course for `fallShare` of the pass,
lands at the course's end, and its tails fade over the next 0.08.

The head gathers speed as it falls, and the dust tail grows with the speed. The tail opens from
the nucleus with a rounded front over its first tenth. Sparks shed along the way drift down and
fade over 0.3 of a pass.

## SatelliteScene

A satellite in orbit over a large Earth, through day, dusk and night.

**The satellite goes round over both poles every 3 minutes, and its orbit never comes round.** It
runs on the cycles passed as well as the cycle - see SceneTime. The Earth turns once under it every
4.618 orbits, 3 and the golden ratio: about 14 minutes. That number lies far from every fraction, so
each track runs between the earlier ones, and the tracks spread evenly over the globe. Every
continent passes by day within the first quarter of an hour.

**The frame looks across the track.** The ground flows across the screen, the horizon at the top
lies beside the track, and north turns as the satellite passes over the poles. The view looks
straight down 36 degrees from the satellite, away from the horizon. It is turned from the Earth's
own axes by the spin about the axis, then the tip of the axis toward the viewer, then the tilt in
the screen, each worked out from the orbit.

**A drag turns the globe the way the pointer goes, as a hand turns a ball**: the ground under the
middle of the view moves where the drag takes it, about the axis square to both, wherever the poles
stand. The turn is the orbit's, about the Earth and among the stars, so the satellite flies on along
the turned orbit with its track still across the screen. The Sun stands where it stood, so the day
and the night come along with the ground. Each layer of cloud leans off the Earth's axis by a turn
fixed to the Earth.

**The wheel climbs above the globe**, a tenth of the way a notch, from the lowest view to the whole
globe in the middle of the frame. The globe's radius shrinks by the same share at each step, from
1.15 times the frame's longer side to 0.4 times its shorter, and the view leans back from 36 degrees
to 1, never quite down the Earth's axis. The satellite draws smaller toward the top right, out of
the globe's way, and what is drawn on the globe at a fixed size - the sea's glint, the city lights -
shrinks with it. The space and the ocean are kept as layers at the lowest altitude - see
SceneLayer - and drawn live above it.

**The Sun stands fixed among the stars**, and so does the orbit's plane, so each orbit passes a day
of about two minutes and a night. In the Earth's shadow the satellite darkens and the stars
brighten. Its panels turn once an orbit to face the Sun, so in the shadow they show their backs.
The stars drift a fifth of the frame's width each orbit - see StarField drift.

**The cycle is 240 seconds.** The twinkling and the blinking keep the pace they would have in a
40-second cycle.

**The night falls by the sunlight on each pixel of the globe**: it starts at a sunlight of 0.03 and
is whole at -0.13, so the terminator curves as it does at any altitude, and the clouds darken by the
same depth, toward a moonlit blue-grey. The city lights come on as the sunlight falls past -0.01 and
are full past -0.06. The atmosphere along the limb is blue by day, orange at dusk and dark at night,
by the sunlight on the limb, which rises evenly across it toward the Sun: three rings, each fading
out to both sides.

**The towns are placed by the land's map.** Until it is built, 1500 stand anywhere on the land
from 56 degrees south to 62 north. Then 1500 are tried at random points of the land and 5800 at
random points of the coastline, a little inland, from 56 degrees south to 72 north, each kept by a
weight: highest on low ground, and thinning with height, in the deep desert, in the tundra and a
little in the northern forest. The towns over the land thin away from the coast as well. The ice
of Greenland and Antarctica has none. About one town in 33 glows. The towns are the same on every
run.

**Three layers of cloud float above the ground**: low heaps, a middle sheet and high streaks - see
CloudKind. Each is a map on a sphere of its own, 0.6, 1.1 and 1.8 hundredths of the Earth's radius
up, turning about an axis that leans up to 6 degrees off the Earth's. Each drifts east and back
against the ground once a cycle, the higher layers further. The clouds fade into the haze as they
near the limb, turn a moonlit blue-grey on the night side, and pass over the city lights.

**The land is worked out pixel by pixel inside the coastlines**, from the land's map - see EarthMap.
The coastlines are landOutlines(), built off the paint from the moment the scene is made; the first
frame waits for them if it comes sooner.
A height of 1 stands 0.014 of the Earth's radius up as drawn, about ten times the Himalaya's true
height, and each slope is lit by the Sun where it stands: lighter facing it, darker facing away. A
Sun lower than about 14 degrees lights the slopes as one at 14 degrees would, so the shading deepens
toward dusk without turning black, and it fades out across the terminator. Snow lies above a height
that falls from the tropics toward the poles, and past 66 degrees south it covers all the land.

**The looks part at the clouds' edges and in the land.** Stylized cuts each layer's cover into two
flat tones, and the land into flat zones of green, steppe, sand, taiga and tundra with flat bands of
height and one tone of slope each way, from the broad relief. Realistic shows the cover as it is,
and the land in the detailed relief, its colours blended - see SceneLook.

**The maps are built off the paint** the first time the scene is drawn: a fraction of a second on a
processor with many cores. The clouds show from the first frame after, and the land is drawn flat,
green with the ice sheets white, until its map is ready. Each frame works the clouds, the land and
the night out in bands of rows spread over the cores - see spreadOverCores. On a globe over 3200
pixels in radius - a frame about 2800 pixels wide or more - the land is worked out at every 4th
pixel and row, over 1300 - a frame about 1130 pixels wide - at every 2nd, and below that at every
pixel. The cloud maps' points stand twice as far apart as the land's, and so do the clouds' points.
What lies between is blended before the look cuts the cover, the zones or the bands of height. The
bands are laid in on all the cores as well - see Canvas drawPixelViews.

**Each point reads the land and the clouds as fine as the ground it stands for**, taken 1.4 times as
wide as the points stand apart - see MapQuery. The ridges and the clouds hold still as the Earth
turns, and soften toward the limb, where each point stands for more ground.

**Each edge the stylized look cuts is softened by what its field changes over the pixel**: the
dryness and the cold at a zone's edge, the height at a band's, the light at a tone's, the cover at a
cloud's. An edge stays sharp where its field changes slowly, and moves smoothly across the pixels
where it changes fast, so it does not step from pixel to pixel as the Earth turns.

## drawSatellite

**In the Earth's shadow the craft keeps 35 percent of its colour**, by `sunlit`. Its panels turn
about their booms once a round of `orbitTime`, so past edge-on they show their ribbed backs, and
the craft rocks once a round. The navigation lights blink 16 times a unit of `blinkTime`, the
strobe 20 times, and the panels' glints sweep twice.

## ChaseScene

A clumsy wolf chases a hare across a landscape that scrolls past and comes round again.

**Two 20-second episodes a cycle.** In the first the wolf trips over a log and lands on his belly;
in the second he runs into a low branch and goes over onto his back. Each time the camera slows
to a stop while he lies dazed, the hare turns round to taunt him, and the chase picks up again.

**The landscape is layers at their own speeds** - clouds, mountains, hills, the ground and the
grass in front, each a share of the camera's speed. Each layer is tiled with a period that the
cycle's travel divides - see ChaseScene periodNear - so it ends the cycle where it began.

## ChaseScene periodNear

The layer moving at `parallax` covers `parallax` times the cycle's travel in a cycle. The period
returned divides that distance a whole number of times, as near `wanted` as a whole number allows,
so every tile of the layer has moved on by whole tiles when the cycle comes round.
