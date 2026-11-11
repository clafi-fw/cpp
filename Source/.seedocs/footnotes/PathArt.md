# PathArt

The words that do not fit above a declaration.

## Scene

A scene is a moving picture, drawn at a time into any bounds. Most of its motions run through a
cycle and come back to where they began. The MillScene showcase holds a list of scenes, offers them
as tiles in the Scene group of its Scene page - drawIcon() over name() - and stores the one picked
under that name. A double click on a tile, or on a theme, picks it and closes the menu. A scene it
takes up fades in over a still of the frame before it.

**Under the tiles, a section named after the picked scene shows its page of options** - Mill
Options, Earth Options: the themes, a scene's look and choices, or a line saying the scene has none.
The pages share one room as large as the largest of them, so a pick never asks the menu for more.

**A motion that comes round runs a whole number of times a cycle**, so the frame at the end of the
cycle is the frame at its start and the loop shows no seam. A scene that needs a slow motion beside
a fast one gives itself a longer cycle - cycleDuration(). A motion meant never to come round, such
as the orbit in EarthScene, runs on the cycles passed as well - see SceneTime.

**The showcase owns the clock.** It advances the cycle time by the time since the last frame over
cycleDuration(), wraps it back into 0 to 1, and counts each wrap in the cycles passed. A scene
keeps no time of its own.

**A scene draws its figures as sprites and its weather as effects** - free functions that take a
SceneFrame and a struct: a sprite's pose, an effect's extent. Each takes the time its motions run
on, and comes round when that time runs a whole number of units a cycle.

## Scene drawIcon

**The figure fills bounds.** Its extent is framed to the shorter side and centred, so an icon of
any size shows the whole figure with no margin. A part the square cuts - a string, a glow - is
clipped at bounds.

**One picture in every theme and look.** The figure is one of the scene's sprites in the colours the
scene draws it in. MillScene's mill wears Fresh Spring's colours whatever theme is picked, and
ChaseScene's wolf and hare their own daylight coats.

## SceneReadoutColors

**What a host's readout over the scene is drawn in**: the field and the series of its chart, and the
ink its text is written in - light unless the scene states a darker one for a light sky. The host
draws the field faint and the series near full, whatever alpha the colours carry, and writes the
units and the chart's readings in the ink at a lesser opacity.

## Scene readoutColors

**Each scene states its own**, taken from its palette: a dark tone for the field and a light accent
for the series. The MillScene showcase asks the shown scene whenever another scene is shown, and
whenever a theme is picked or tried on. A scene that offers themes answers with its picked theme's -
see readoutColorsOf.

## readoutColorsOf

**The theme's river**: riverBottom for the field and riverWave for the series. The ink is the light
one, or riverBottom at half toward black where that stands out more against skyTop.

**Taken from the picked theme, not the blend.** The host asks as the pick lands, before the
crossfade has moved, and keeps the answer until it asks again - see ThemeBlend.

## Scene draw

Paints the whole of bounds, so nothing needs to be drawn under it.

**A frame depends on its SceneTime, its bounds, and the look and the view the scene keeps.** The
showcase jumps between scenes and times - a hover preview, a pick, a resize - and a scene draws
the same picture for the same of these, whatever was drawn before. A scene that offers themes also
keeps its theme's crossfade between frames - see ThemeBlend.

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
when the two differ for it - today EarthScene, whose clouds, land and sea they change - and draws in
the look setLook() last named from its next frame on. A scene that offers no looks keeps the value
and draws as it always does.

The MillScene showcase offers the looks as radio buttons captioned Look on the scene's page of
options, and keeps the pick under SceneLook. With no pick kept, the scene is drawn Realistic.

## Scene offersThemes

**A scene that offers themes is drawn in the themes of Themes::allThemes**, and crossfades to the
one setTheme() last named - see ThemeBlend. MillScene and ChaseScene offer them. A scene that offers
none ignores the call.

**One pick for every scene that offers themes.** The MillScene showcase shows the theme picker on
the page of options of each of them, every picker showing the one pick, and a pick or a hover
crossfades them all. It keeps the pick under SceneTheme, by the theme's name. With no pick kept,
they open in the first theme of the list, Fresh Spring.

**Between the timer's ticks**, the showcase moves the shown scene's crossfade on at every move of
the pointer over it, through animationTick().

## SceneTheme

**One table of colours for every scene that offers themes**, and the effects that come with them.
The fields up to effectColor2 are the mill's, and the sky, the sun, the clouds, the meadow - hillTop
and hillBottom - and the trees serve ChaseScene as well; ChaseTheme holds what only the Chase draws.
Every theme fills every field.

**petal1 to petal3 are the flowers' petals**, shared by every flower the Chase draws - each wears
one, by turns - and by the falling petals, which wear the first two in place of the effect colours.

**Figures and props stand apart from what is behind them by colour, not only by brightness.** In
OKLab their colour difference from the background is at least 0.15 for the runners, the cow and the
mill, and 0.10 for every prop, where the theme leaves room for it - about as far apart as the
daylight scenes already draw them. Golden Hour's and Winter Frost's cows keep their white.

**A crossfade blends every colour field one by one** - see ThemeBlend. They stand in a row from
skyTop to the end of `chase`, and colorsNum() counts them.

## ChaseTheme

The Chase's own colours in a theme: the mountains' clear colours, which the scene hazes as it does
the far hills, their trees and fireflies - see ChaseScene hazed - the shades of the tree a branch
run ends at, the trail and the grass, the four runners' coats, and every prop.

**Each object's colours are its daylight ones in the theme's light.** They are dimmed and tinted as
the theme's sky is to daylight's, washed out as far as the theme has no colour, and then all moved
lighter or darker together - only as far as its contrast needs, keeping to its daylight character.

- A wolf's head and ears stand against the mountains; a hare stays below the far hills' top. Both
  are measured as they are drawn, hazed.
- A mountain's clear colour is a rock in the theme's light, in the hue its mountains had before the
  haze.
- The other couple's black wolf stays darker than ours.
- In Winter Frost the couples trade hares: ours is grey and theirs brown, so its brownHare holds
  the grey coat and its greyHare the brown.
- A coat's tongue, mouth, teeth, tail, inner ears and nose rise with the fur where it is lifted and
  never darken with it, so a white tail stays white by day.
- Yellow things - the banana peel, the rubber duck, the ball's yellow, the flowers' hearts - keep
  their colour at night and stay light, as a dark yellow reads as olive. The ball, the flowers'
  petals and the whole apple stay lighter at night too.
- Every theme's petals come in three colours. A white petal is joined by a blushed pink and a
  lilac, or by cream or pale blue where one of those is lost on the meadow; a coloured petal by its
  hue turned a little either way. Some themes choose their own: Twilight's run from a light rose to
  lavender, Magic Forest's lighter still, Cherry Blossom's are its falling petals' pinks, Lunar
  Eclipse's dull purples, Cyberpunk Night's magenta, neon green and acid yellow, and Midnight's
  are darker. On snow - Winter Frost, Winter Night and Arctic Dawn - every flower is white, the
  butterfly white with an icy blue edge, and the trail an icy grey.
- The toxic and neon themes and Underwater Realm give the toadstool caps and the apples toxic reds:
  a lava red in Toxic Waste, a hot crimson in Cyberpunk Night and Underwater Realm, a neon red in
  Neon City Night. Vintage Noir's apples are the one red in its grey, daylight's.
- One green serves the flowers' stems, the tufts on the lawn and the foreground grass, whose every
  third blade is a shade darker. Midnight's, Toxic Waste's and Neon City Night's are lighter, with
  their daisy leaves, to show against their dark meadows; Toxic Waste's is a dim toxic green, and
  Neon City Night's the green of its trees, muted. Cherry Blossom's plants are a dark sage, on a
  grey-green meadow by a rose-grey trail. Vintage Noir's plants are grey, as the rest of it is.

**The themes not of this world dress the plain things in their own colours** instead of daylight's:
the stump, the signpost, the crow, the butterfly, the bird, the frog, the snail, the boulder, the
reeds, the boot, the hedgehog, the rake and the leaves. Each keeps the steps of light and shade it
has by day.

- Cyberpunk Night: violet chrome, with magenta, cyan, neon green and acid yellow.
- Neon City Night: asphalt, with cyan, hot pink, magenta and yellow. Its signpost is a neon sign,
  dark with a bright outline and writing, and its rake's handle is light.
- Toxic Waste: rotten olive and slime, with lime, a hazard yellow sign and a hazmat yellow boot.
- Underwater Realm: driftwood and deep blue, with coral, gold and cyan; the hedgehog is a sea
  urchin.
- Magic Forest: plum wood and amethyst, with pink, lilac and fairy green.
- Midnight Fantasy: moonlit blue-grey and indigo, with pale lilac, mint and pale gold.
- Steampunk Adventure: aged oak, iron and leather, with brass, copper and verdigris.

## ThemeBlend

**What a scene that offers themes is drawn in.** pick() names a theme by its index in
Themes::allThemes, and worn() crossfades to it over 555 milliseconds, eased in and out, colours and
effects alike. A pick made during a crossfade starts the next one from where the blend stands.

**worn() is for drawing, picked() for the rest** - the theme's name, and colours that are read once
rather than every frame. worn() takes the picked theme's name at the pick.

**Always a fade in.** worn() starts as a null theme - every colour zero and every effect off - and
the first theme fades in out of it. A pick made before the first frame, as a host restoring a config
makes, fades in that theme instead.

**wornVersion() moves on with every step of the crossfade**, so a scene that keeps something drawn
in the theme knows to draw it again - MillScene keeps its sky and hills in a bitmap.

## Scene offersView

**A scene that offers a view keeps it, as it keeps its look**, and moves it as the host hands it a
drag - dragView(), in the coordinates of the bounds it draws into - and the wheel's notches -
scrollView(), counted as MouseWheelEvent counts them, away from the user above 0. What the view is
and how it moves is the scene's own: the Earth scene turns its globe and climbs above it - see
EarthScene. A scene that offers none leaves both as they are.

**The MillScene showcase hands them on from the panel the scene covers.** A press on the scene where
a drag moves the view - see Scene dragMovesViewFrom - stops a glide and holds the view until it is
let go - see Scene holdView and Scene letGoView. Let go before the pointer leaves the slack a click
is allowed (DragEvent pastThreshold), the press is a click - see Scene clickView - and a double
click there is the scene's as well - see Scene doubleClickView. A press anywhere else leaves the
scene moving as it was, and one on a control over the scene, such as the readings beside it, is
that control's. The pointer shows a hand wherever a drag moves the view.

**A drag let go glides on** at the speed of its last 80 milliseconds and slows to about a third of
it every 0.35 seconds, and the wheel's notches come in over about a tenth of a second. Another
scene shown stops both, and lets go of the view it leaves. Each scene keeps its view while the
showcase runs.

## Scene dragMovesViewFrom

**A drag moves the view from the points the scene names**, each asked in the coordinates of the
bounds it draws into. A scene that offers a view names the whole of it unless it states otherwise.
The Earth scene names its globe, and leaves the space round it - see EarthScene.

## Scene holdView

**A press holds the view until it is let go.** The host calls holdView(true) as a press comes down
where a drag moves the view - see Scene dragMovesViewFrom - and letGoView() or clickView() as it is
let go, so a scene can keep its own motion still while the viewer has the view - the Earth scene
holds its globe, see EarthScene. A drag and the wheel reach the scene as dragView() and
scrollView(); a press held still reaches it only here. A scene that offers no view is never told.

## Scene letGoView

**The let-go carries the pointer's speed**, in pixels a second across the bounds the scene draws
into, and 0 where the pointer stood still or moved too slowly for a glide. The host then glides on
at that speed through dragView(), so whatever the scene does with the speed comes on top of the
glide. The Earth scene takes it to choose how the satellite flies on - see EarthScene.

## Scene clickView

**A click lets go of the view as a still let-go does, unless the scene answers it otherwise.** The
host calls clickView() in place of letGoView() for a press let go inside the slack a click is
allowed, so a tremble of the pointer in a click reaches the scene as a click and not as a speed. The
Earth scene takes a click to stop the satellite or set it flying again - see EarthScene.

## Scene doubleClickView

**A double click where a drag moves the view reaches the scene after its first click.** The host
calls doubleClickView() as the second press comes down, once the first has been let go as a click -
see Scene clickView. The second press then goes on as a press: it holds the view, and a still
let-go of it is a click again, so a scene that toggles something on a click sees it toggled and
back. The Earth scene takes a double click to move the day on - see EarthScene.

## SceneChoice

**A scene states what else it lets the viewer pick, beside its look**: each choice has a name and
its options' names, and the host shows each as radio buttons captioned with the choice's name, on
the scene's page of options. The names are shown as they are written, and they are also what the
scene keeps its pick under.

## Scene choices

**The host asks for the choices and leaves the picks to the scene.** It reads chosenOption() to
show what is picked and calls chooseOption() when an option is picked or hovered over, putting the
pick back when the hover leaves. The scene keeps its picks with its view - see Scene storeView -
and an index past the choices it states is a mistake it may refuse.

## Scene switches

**A scene states the on-off settings it offers by name**, and the host shows each as a check box
with that name on the scene's page of options, under its pickers. The host reads isSwitchOn() to
show the box and calls setSwitch() on a click. The scene keeps its switches with its view - see
Scene storeView - and an index past the switches it states is a mistake it may refuse.

## SceneGesture

**A scene states the gestures it answers, for the viewer to read**: each has a name and its effect,
and the host writes them a line each, the name in bold and the effect after it. The MillScene
showcase shows them captioned Controls on the scene's page of options, under its pickers.

## Scene gestures

**What the scene states is what it answers.** The host only shows the hints; the gestures reach the
scene through the view calls - see Scene offersView. The Earth's read: **Click** to stop and
resume, **Double click** to advance time, **Drag** to choose position, **Drag and fling** to make
an orbit.

## Scene storeView

**A scene that keeps its view across runs writes it into a config section of its own.** The host
keeps a section per scene under the scene's name(), and hands it over as its window closes;
restoreView takes the view back from it as the scene is made. What the section holds is the
scene's to say: a static viewSchema() states it, and createSceneViewsSchema() gathers those for the
host's config schema - see EarthScene. A scene with no section keeps no view and is never asked.

**A section never written holds its schema's defaults**, so a schema states the view the scene
starts with, and restoring it changes nothing. The section holds the scene's choices as well, each
under its own name, and its switches, under names of the scene's own.

## createSceneViewsSchema

**A keyless section**, so each scene's own section stands straight in the section the host puts
this one under: the showcase keeps it as SceneViews, so the Earth's altitude is SceneViews, Earth,
Altitude. A scene that keeps its view is gathered here, as it is listed in createScenes().

## SceneTime

**cycleTime is how far a frame falls into its cycle**, 0 to 1, and cyclesPassed counts the whole
cycles before it since the showcase started. Run backward past the start, the count goes below 0.

**A motion that comes round reads cycleTime alone.** One that never comes round reads both, as
cyclesPassed + cycleTime cycles summed in double: a float holds the time of a long run too coarsely
for the motion to stay smooth.

## SceneFrame

The shorter side of the frame is 600 units - or the units given to Spanning() - and `unit` is the
pixels in one. width and height
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

The Earth as maps, from pole to pole: how high the land stands, what covers it and how deep the sea
is.

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
- **The sea's depth** comes from Natural Earth's bathymetry at 1:10m - see landOutlines: its
  contours at 200, 1000, 2000, 4000, 6000 and 8000 metres, made coarser to the grain of the cover.
  Each point takes the deepest contour that holds it, counted evenly from 0 at the coast to 1 past
  8000 metres, and the levels are blurred into slopes about a degree across, so the depth rises
  toward the coasts without steps.

The relief keeps 4096 points round each circle of latitude and 2048 from pole to pole, and the
cover, the sea's depth with it, half as many each way. Each keeps 7 coarser levels of detail as
well, each half as fine each way as the one before, its points the average of the 2 by 2 under them:
53 MB in all. One build always makes the same maps.

**Building the maps takes about 7 seconds of processor time**, spread over all the cores. Build them
off the paint.

**readLand() reads each point as fine as the ground it stands for** - see MapQuery. It takes the
two levels whose points lie about as far apart as that ground reaches, blends the four nearest
points in each, then blends the two levels by how near each is. Detail finer than the points can
hold fades out, so it does not shimmer as the view moves.

**readDepths() reads the sea's depth as readLand() reads the land**, as fine as the ground each
point stands for, from 0 at the coast to 1 in the deepest trenches. Away from the coasts the land
reads 0, so the depth is for colouring the sea, with the land drawn over it.

The reads wrap round the longitudes any number of turns either way and stop at the poles. readLand()
and readDepths() work through eight points at a time with AVX2, and several threads may call them at
once.

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

## Fireflies

Each firefly has a place in the area, wanders up to 50 units across and 30 up and down from it, and
glows 15 units round - all scaled by `size`.

**A firefly strays past the area** by up to 65 units times `size`. An area drawn in tiles along a
moving layer reaches past the frame by that much, or a firefly pops in at the edge.

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

**The orbits can be hidden** - the scene's one switch, Show orbits, see Scene switches. The scene
keeps it in its own config section as OrbitsShown, on as the scene starts.

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

## EarthScene

A large Earth turning under a satellite, through day, dusk and night.

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
the turned orbit with its track still running the way it ran across the screen; while the satellite
hangs, the turn moves the point it hangs over. The Sun stands where it stood, so the day and the
night come along with the ground. Each layer of cloud leans off the Earth's axis by a turn fixed to
the Earth. A drag turns the globe from a point on it, and the space round it is left to the scene -
see Scene dragMovesViewFrom.

**The let-go decides how the satellite flies on** - see Scene letGoView. Let go still, it hangs over
the ground in view: the ground stands, and only the day and the night move on, with the clouds'
drift. Let go moving, it takes the orbit that carries the ground under the middle of the view on the
way the pointer went, from the picture as it stands, at the orbit's own pace; the track then runs
across the screen at that angle, with the horizon still at the top. The orbits and the sky's drift
stand still while it hangs, and the Earth's turn goes on.

**A click stops the satellite or sets it flying again** - see Scene clickView. A flying satellite
hangs as a still let-go hangs it; a hanging one flies on along the track it flew before it hung,
from the picture as it stands, at the orbit's own pace.

**A double click moves the day on by a quarter** - see Scene doubleClickView. The Sun goes back
round the Earth's axis as far as a quarter turn of the Earth carries the ground on, so noon turns to
dusk, dusk to midnight, midnight to dawn and dawn to noon wherever the view stands. The light glides
there in about a second of the scene's time. The ground, the orbit and the stars stay where they
are. The double click's own two clicks stop the satellite and set it flying again, or the other way
round.

**The wheel climbs above the globe**, a tenth of the way a notch, from the lowest view to the whole
globe in the middle of the frame. The globe's radius shrinks by the same share at each step, from
1.15 times the frame's longer side to 0.4 times its shorter, and the view leans back from 36 degrees
to 1, never quite down the Earth's axis. The satellite draws smaller toward the top right, out of
the globe's way, and what is drawn on the globe at a fixed size - the sea's glint, the city lights -
shrinks with it. The space is kept as a layer at the lowest altitude - see SceneLayer - and drawn
live above it; so is the ocean until the land's map is built. The view starts four notches up.

**The view is kept across runs** - see Scene storeView: the altitude, the orbit as the drags have
turned it and the angle its track runs at, the orbits flown, which say where the satellite is and
how far the sky has drifted, whether it hangs and where, the Earth's turns, and the Sun's turns
round the Earth's axis. The globe flies on, or hangs on, from where it was left; the clouds' drift
and the twinkling start over. The section holds Altitude, Orbits, EarthTurns, SunTurns,
TrackAngle in degrees, Hanging, and OrbitTurn and HangingCamera, each as an axis and an angle in
degrees: AxisX, AxisY, AxisZ and Degrees.

**The Earth turns eastward, as the real one does.** The scene states its gestures as hints - see
Scene gestures.

**The globe holds still while a press holds the view**, and a let-go moves it on at once. The
wheel leaves the satellite flying as it flew. The clouds go on drifting over the held globe, the
satellite goes on hovering and blinking, and the stars and the city lights twinkling. The time is
the scene's, so a paused scene holds the count as well.

**The Sun stands still among the stars until a double click moves it**, and so does the orbit's
plane, so each orbit passes a day and a night: a day of about two minutes as the scene starts, and
longer or shorter as the Sun moves round. In the Earth's shadow the satellite darkens and the stars
brighten. Its panels turn once an orbit to face the Sun, so in the shadow they show their backs.
The stars drift a fifth of the frame's width each orbit - see StarField drift.

**The cycle is 240 seconds.** The twinkling and the blinking keep the pace they would have in a
40-second cycle.

**The night falls by the sunlight on each pixel of the globe**: it starts at a sunlight of 0.03 and
is whole at -0.13, so the terminator curves as it does at any altitude, and the land, the sea and
the clouds darken by the same depth, toward a moonlit blue-grey. Deep in the night the land and the
sea keep a tenth of their colour, and the moonlight is an eighth. The city lights come on as the
sunlight falls past -0.01 and are full past -0.06. A town's light is a round dot at any size, laying
the light its area holds, and a glowing town's glow fades evenly to its edge. The atmosphere along
the limb is blue by day, orange at dusk and dark at night, by the sunlight on the limb, which rises
evenly across it toward the Sun: three rings, each fading out to both sides.

**The towns are placed by the land's map.** Until it is built, 1500 stand anywhere on the land
from 56 degrees south to 62 north. Then 1500 are tried at random points of the land and 5800 at
random points of the coastline, a little inland, from 56 degrees south to 72 north, each kept by a
weight: highest on low ground, and thinning with height, in the deep desert, in the tundra and a
little in the northern forest. The towns over the land thin away from the coast as well. The ice
of Greenland and Antarctica has none. No two towns stand within 0.2 degrees of each other, so their
lights stand apart as the view comes down; with the whole globe in view they still touch. About one
town in 38 glows, and no two glowing towns stand within 1.5 degrees: a town that would glow nearer
is a plain light. The towns are the same on every run.

**Three layers of cloud float above the ground**: low heaps, a middle sheet and high streaks - see
CloudKind. Each is a map on a sphere of its own, 1.0, 1.4 and 1.8 hundredths of the Earth's radius
up, turning about an axis that leans up to 6 degrees off the Earth's. Each drifts steadily east
against the ground, 0.4, 0.6 and 0.9 degrees a second from the lowest up. The clouds fade into the
haze as they near the limb, turn a moonlit blue-grey on the night side, and pass over the city
lights.

**The land is worked out pixel by pixel inside the coastlines**, from the land's map - see EarthMap.
The coastlines are landOutlines(), built off the paint from the moment the scene is made; the first
frame waits for them if it comes sooner.
A height of 1 stands 0.014 of the Earth's radius up as drawn, about ten times the Himalaya's true
height, and each slope is lit by the Sun where it stands: lighter facing it, darker facing away. A
Sun lower than about 14 degrees lights the slopes as one at 14 degrees would, so the shading deepens
toward dusk without turning black, and it fades out across the terminator. Snow lies above a height
that falls from the tropics toward the poles, and past 66 degrees south it covers all the land.

**The sea is coloured by its depth**, from the land's map - see EarthMap: greenish over the shelves,
deepening to blue down the slopes and over the ridges to the abyss, darkest in the trenches.
Realistic shades evenly between the depths' colours, its shelves about as light as the land beside
them; stylized cuts five flat tones, its shelves light as on a map. Until the map is built, the sea
is one gradient from the haze at the limb to the deep blue.

**The air's haze veils the ground by the air the view looks through to it**, past the air straight
down: none looking straight down, more as the ground turns away toward the horizon and the limb. The
air thins with height - over the Himalaya's crest a third of it is left - so the ranges stand out of
the haze, and the sea, at height 0, hazes as the low land beside it does. Where the view looks
through 8 times the air straight down, half the ground's colour is veiled.

**The looks part at the clouds' edges and in the land.** Stylized cuts each layer's cover into two
flat tones, and the land into flat zones of green, steppe, sand, taiga and tundra with flat bands of
height and one tone of slope each way, from the broad relief. Realistic shows the cover as it is,
and the land in the detailed relief, its colours blended - see SceneLook.

**The maps are built off the paint** the first time the scene is drawn: a fraction of a second on a
processor with many cores. The clouds show from the first frame after, and the land is drawn flat,
green with the ice sheets white, until its map is ready.

**The globe is one picture a frame**, from its sea up to its air: the sea, the land, the night, the
Sun's glint, the towns' lights, the clouds and the atmosphere. The picture is worked out in bands
of rows spread over the cores - see spreadOverCores - and each band lays its layers one over the
other. A backend is handed the globe's pixels once a frame: Direct2D uploads them once, and the CPU
backend lays them in on all the cores - see Canvas drawPixelViews. Both backends show the same
pixels. Until the land's map is built the picture holds neither the sea nor the land, and lies over
the flat ocean and the flat land.

**The points stand apart by the globe's size.** On a globe over 3200 pixels in radius - a frame
about 2800 pixels wide or more - the land is worked out at every 4th pixel and row, over 1300 - a
frame about 1130 pixels wide - at every 2nd, and below that at every pixel. The cloud maps' points
stand twice as far apart as the land's, and so do the clouds' points and the sea's. What lies
between is blended before the look cuts the cover, the zones, the bands of height or the sea's
tones.

**Each point reads the land, the sea and the clouds as fine as the ground it stands for**, taken 1.4
times as wide as the points stand apart - see MapQuery. The ridges and the clouds hold still as the
Earth turns, and soften toward the limb, where each point stands for more ground.

**The realistic coast is blurred as much as the land is**, by a Gaussian whose spread is half the
distance between the land's points: a pixel where they stand every 2nd pixel. A coast is then as
sharp as the land's sharpest edges. The stylized look cuts its coast at the pixel, as it cuts its
zones and bands.

**Each edge the stylized look cuts is softened by what its field changes over the pixel**: the
dryness and the cold at a zone's edge, the height at a band's, the light at a tone's, the cover at a
cloud's, the depth at the sea's. An edge stays sharp where its field changes slowly, and moves
smoothly across the pixels where it changes fast, so it does not step from pixel to pixel as the
Earth turns.

## drawSatellite

**In the Earth's shadow the craft keeps 35 percent of its colour**, by `sunlit`. Its panels turn
about their booms once a round of `orbitTime`, so past edge-on they show their ribbed backs, and
the craft rocks once a round. The navigation lights blink 16 times a unit of `blinkTime`, the
strobe 20 times, and the panels' glints sweep twice.

## ChaseScene

A clumsy wolf chases a hare across a landscape that scrolls past and comes round again.

**Three 20-second episodes a cycle**, so a cycle is a minute long. In the first two the wolf gains
on the hare ever faster, her ears going flat, until he trips over what lies on the trail and lands
on his belly, while she runs on to a safe distance. In the first he lunges at her tail once on the
way and snaps his jaws shut on nothing, and she jumps. In the third he closes in and snaps at the
hare's tail, drops back spent, then gathers himself and leaps at her with his jaws wide open, and an
apple tree, a crow or another wolf ends the leap. Each time the camera slows to a stop while he lies
dazed, the hare turns round to taunt him, and the chase picks up again; under the apple tree she
hops back to a fallen apple and eats it at him, teeth bared.

**How the leap ends changes every cycle**, in a turn five cycles long:

- the tree's low branch meets him in the middle of the air, shuts his jaws and drops him onto his
  back
- he flies into the trunk, slides down it and topples back onto his back
- the tree stands further back than in the other cycles, its branch reaching on from the far side of
  the trunk; he leaps too high, comes down across the branch, slips back down it and drops onto his
  back at the trunk's foot
- a crow meets his face where the tree would stand, tumbles back with its eyes popped wide and flies
  off shaken, its eyes settling as the shaking ends and its feathers drifting down round him; one
  time it flies at him, the next it sits on a stump by the trail until the hare's arrival frightens
  it up into his leap
- another couple, a black wolf and a grey hare, runs the other way, doing as the two do; the hares
  cross just before the wolves' leaps end, one bounding over the other - ours one time, theirs the
  next - and the wolves meet face to face in the air and drop side by side onto their backs, each
  hare taunting from her side. From then on the other couple keeps a time of its own: their hare a
  little ahead of ours, their wolf a little behind.

**What the tree lets fall changes from one turn of five cycles to the next** - the four apples
nearest the trail, or all but the two at the top of the crown, which stay every time. The crow's
start and which hare bounds over the other change with it. All of it goes by SceneTime cyclesPassed,
so a host that does not count the cycles shows the low branch and four apples every time.

**What lies on the trail changes every run** - a banana peel, a boot, an apple, a hedgehog, a rubber
duck, a ball, a patch of grass, a rake, two a cycle and round again - going by SceneTime
cyclesPassed, so a host that does not count the cycles shows the banana peel and the boot every
time. The peel shoots out from under his paw and spins, the boot falls over and the apple splits in
two. The hedgehog rolls into a ball once the hare has passed over it, and walks off once the wolf is
down. The duck goes flat under his paws, the ball rolls on a little and, while he lies dazed, a
little way back, the grass flattens under him and springs back, rocking for most of a second, and
the rake's handle swings up into his face.

**Something stands by the path in every run** and slows neither of them. It passes partway into the
run, and early in the third, before the wolf closes in. There are twelve, one for each run of the
four cycles the trail obstacles take: a banana peel, a signpost, a bird, an apple, a toadstool, a
butterfly, the mill's reeds, a boulder, a frog, a daisy, a tree stump and a snail. The banana peel
and the apple each come by earlier in the run that ends on one on the trail, and a host that does
not count the cycles shows the first three every time.

**More stands on the far edge between the runs' own**, one about where each run starts, three in a
cycle's travel. They are the twelve and a crow on a tree stump, in an order of their own that comes
round every thirteen cycles - a daisy, the crow on its stump, a boulder, a toadstool, a snail,
a signpost, the reeds, a frog, a butterfly, a tree stump, an apple, a bird and a banana peel - see
ChaseScene drawVergeRow.

**They stir as the two go by.** The reeds and the daisy bend in the wind of their passing, and the
wolf's wake strips the reeds' heads of their fluff and the daisy of petals. The signpost's board
spins round and is left pointing back the way they came, the boulder, the banana peel and the apple
bounce after each of them, and the toadstool's cap hops on its stalk, then off it to land beside it.
The bird, the butterfly and the crow on the stump fly off, the frog hops back into the grass, and
the snail draws into its shell until the wolf is by.

**Something stands on the near edge of the trail too**, in front of the two and a little larger: the
same thirteen in an order of their own - a snail, a boulder, a daisy, a frog, the crow on its stump,
a signpost, an apple, a butterfly, a tree stump, a banana peel, the reeds, a bird and a toadstool.
They stand at a spacing of their own along the trail rather than one to a run, so they never fall
into step with the far edge or with the runs, and one can come by at any moment, while the camera
stands too - see ChaseScene drawVergeRow. The near frog hops toward the viewer.

**The story runs on from one run of the scene to the next** - see Scene storeView. The scene keeps
the cycle after the last one it drew, and the next run starts on it, with the host's cycles counted
on from there, so each run opens where the last one stopped instead of on the first cycle. The
section holds NextCycle.

**The landscape is layers at their own speeds** - clouds, mountains, hills, the ground and the
grass in front, each a share of the camera's speed. Each layer is tiled with a period that the
cycle's travel divides - see ChaseScene periodNear - so it ends the cycle where it began.

**Everything is drawn in the theme the scene is given** - see Scene offersThemes. The sky, the sun
or the moon, the clouds, the meadow and the tree a branch run ends at are the theme's shared
colours; the rest is the theme's ChaseTheme.

- The theme's fireflies and its rain, snow, falling leaves or petals and dust keep to the far hills'
  layer: behind the trail and the runners, smaller, moving with the hills. The fireflies are hazed
  as the hills are. The weather falls against the sky as well, so it is drawn at 0.6 of its strength
  instead.
- A rainy theme's lightning strikes behind the mountains, three times each time the weather comes
  round, and its flash lights the sky above the ground line.
- The theme's stars come out over the sky. Stars, fireflies and weather come round twice a cycle.
- The little flowers in the lawn take half the lawn's colour, so they lie in it rather than on it.
- The foreground grass grows out of the lawn: a blade's root wears the lawn's colour at the bottom
  of the frame, and its tip the theme's grass.
- The icon keeps the wolf and the hare in their daylight coats.

## ChaseScene drawVergeRow

**A cycle's travel holds twice the golden ratio's square of near places**, against the far edge's
six, the first of them half a spacing from the start of the trail. No whole number of cycles holds a
whole number of places, so the near row never comes round to the moments of the runs it stood at
before. The far edge's row between the runs' places holds three to a cycle's travel, so it keeps to
the same places among the runs every cycle. Both go by SceneTime cyclesPassed: a host that does not
count the cycles shows the same places every cycle, and they jump where the cycle comes round.

**The stretch where the wolf comes down stays clear.** Neither row has a place within 700 units of
where the camera stands in a run, so nothing by the path stands by what he trips over, the tree, the
crow or the other couple while he is down; a frame wider than 16:9 can show one at its edges then.

**The near row keeps clear of the far one.** A near place that would stand within 500 units of a
place on the far edge moves on to stand 500 units past it, and on past each next far place it would
stand as close to, so the two edges never show a pair side by side. The near places still never fall
into step with the far ones or the runs; only the ones that come close move.

**The hare and the wolf each start their part of the stirring** - the first time each comes within
reach of it, whatever the camera is doing. Each part then plays straight through, as it would with
that one running by at full speed: the bend in the wind of the hare's passing and the hops and turns
after her, then the wind of the wolf's passing and the hops, turns, petals and fluff that follow
him. A place the wolf reaches long after the hare - while he lies dazed - keeps his part until he
comes.

## ChaseScene hazed

**Distance is haze.** A layer's clear colours are taken toward skyBottom, the sky at the horizon,
as far as its distance goes. The distance is read off the layer's share of the camera's speed: a
layer moving at a third of it stands three times as far as the trail. The haze is the distance
beyond the trail over that distance plus k_halfHaze, so a layer 2.46 times the trail's distance
beyond it is half skyBottom: the far hills, at 0.35 of the camera's speed, take 0.43 of it, the
mountains, at 0.12, take 0.75.

- The far hills are the meadow's own hillTop and hillBottom, their trees the tree's own colours and
  the fireflies over them the theme's effect colours, all hazed alike. The mountains are
  ChaseTheme's mountainTop and mountainFoot.
- The weather keeps to the hills' layer but is not hazed - see the ChaseScene section.

## ChaseScene periodNear

The layer moving at `parallax` covers `parallax` times the cycle's travel in a cycle. The period
returned divides that distance a whole number of times, as near `wanted` as a whole number allows,
so every tile of the layer has moved on by whole tiles when the cycle comes round.
