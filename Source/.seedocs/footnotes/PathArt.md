# PathArt

The words that do not fit above a declaration.

## Scene

A scene is a moving picture, drawn at a time into any bounds. Most of its motions run through a
cycle and come back to where they began. The MillScene showcase holds a list of scenes, offers them
as tiles in the Scene group of its Scene page and in the scene's context menu - drawIcon() over
name() - and stores the one picked under that name. A click on a tile picks it; in the app menu a
double click on one closes the menu. A scene it takes up fades in over a still of the frame before
it.

**A tile whose scene offers options carries a dropdown**, opened under the tile: the scene's theme
picker, its weather, its look, its choices and its switches. Opening the dropdown picks the scene,
as pressing the tile's face does. The tiles of the app menu and of the scene's context menu carry
it alike.

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

**The figure fills bounds.** Its extent is framed to the shorter side and centered, so an icon of
any size shows the figure with no margin. A part the square cuts - a wing tip, a glow, the android
below its waist - is clipped at bounds.

**One picture in every theme and look.** The figure is one of the scene's sprites in the colors the
scene draws it in. MillScene's mill wears Clear Day's colors whatever theme is picked, and
ChaseScene's wolf and hare their own daylight coats.

## SceneReadoutColors

**What a host's readout over the scene is drawn in**: the field and the series of its chart, and the
ink its text is written in - light unless the scene states a darker one for a light sky. The host
draws the field faint and the series near full, whatever alpha the colors carry, and writes the
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

The MillScene showcase offers the looks as radio lines of the options its tile opens. The `Earth`
section of `SceneViews` keeps the choice under `GlobeLook`. The default is `Realistic`.

## Scene offersThemes

**A scene that offers themes is drawn in the themes of Themes::allThemes**, and crossfades to the
one setTheme() last named - see ThemeBlend. MillScene and ChaseScene offer them. A scene that offers
none ignores the call.

**One pick for every scene that offers themes.** The MillScene showcase shows the theme picker in
the options each such scene's tile opens, every picker showing the one pick, and a pick or a hover
crossfades them all. It keeps the pick under SceneTheme, by the theme's name. With no pick kept,
they open in the first theme of the list, Clear Day.

**The scene's context menu picks it too**, from the picker its Theme line opens - see Scene
offersView.

**Between the timer's ticks**, the showcase moves the shown scene's crossfade on at every move of
the pointer over it, through animationTick().

## WeatherEffect

**One particle effect is active at a time.** ThemeDefault uses the palette's effect. None turns
the particles off. The other choices are rain, snow, falling petals, blowing dust, fireflies and
falling leaves.

## Scene setWeather

**Weather overrides stay in effect across theme picks.** MillScene and ChaseScene use them.
An empty SceneWeather::lightning follows the palette. A boolean sets lightning independently
of the particle effect. wearTheme() applies both choices immediately for a still render.

## SceneTheme

**One table of colors for every scene that offers themes**, and the effects that come with them.
Its own fields hold the scenery both scenes share - the sky, the sun, the mountains, the meadow and
its grass, the trees, the reeds, the stones, the flowers - and what else the Mill draws: its river,
its cow, its wolf, its birds, its ducks and its fish. MillTheme holds what only the mill wears, and
ChaseTheme what only the Chase draws.

**effectColor1 and effectColor2 color the weather, and their alpha is a strength.** Rain and snow
wear effectColor1; leaves wear the two by turns, and petals petal1 and petal2; a streak of dust
takes a color at random between the two; a firefly is effectColor1 in a glow of effectColor2. Each
is drawn at its color's alpha times the opacity the scene gives it.

**flock is the birds flying across the mill's sky**, a color of their own beside the Chase's crow.

**motherDuckBody, motherDuckWing and motherDuckEye dress the duck leading the ducklings**, who wear
duckLight, duckShade and duckBeak, as the Chase's rubber duck does.

**fish is the fish leaping from the pond.**

**The sun and the moon take sunDisk and sunGlow.** See drawCelestial for the disc and its rim, and
drawCelestialGlow for the wide glow. The glow is twice as wide as it is tall and takes its strength
from celestialGlowStrength. The moon shows three craters as the night falls by hasStars.

**The mountains' clear colors are rock in the theme's light, before any haze.** mountainTop is the
rock where the most sunlight falls, at the top of the highest range, and mountainFoot the rock at
the ground line - see hazed. Every theme starts them from one neutral rock, 8E8E8E at the
top and 616161 at the foot, darkened and tinted as its sky lights it. It then lightens both alike by
day, or darkens them at night, to set the near range, hazed, apart from the runners' heads. Clear
Day's, High Noon's, Summer Sunset's, Vintage Noir's, Toxic Waste's and Magic Forest's are set by
hand: Clear Day's darker and bluer, High Noon's and Summer Sunset's with their tops kept over a dark
gray foot, Magic Forest's over a dark gray-violet one, Vintage Noir's near black under its dark air,
Toxic Waste's a dark yellow-green top over a near black foot. Where the two ranges meet, rock darker
than the air leaves the near range the darker one. The haze gives them the theme's air.

**Every trunk is the stumps' bark.** The branch run's tree is drawn in bark over barkShade, with its
branch in barkShade. The far hills' trees take bark, hazed with them, and the Mill's pines take it
too. Each is drawn as a stump without its slice - see drawTrunk. The Mill's
footbridge is bough, as the Chase's signpost is; the mill's own timber is in MillTheme.

**petal1 to petal3 are the flowers' petals**, shared by every flower the Chase draws - each wears
one, by turns - and by the falling petals, which wear the first two in place of the effect colors.

**Figures and props stand apart from what is behind them by color, not only by brightness.** In
OKLab their color difference from the background is at least 0.15 for the runners, the cow and the
mill, and 0.10 for every prop, where the theme leaves room for it - about as far apart as the
daylight scenes already draw them. Golden Hour's and Winter Frost's cows keep their white.
Summer Sunset's wolf and hare are drawn lighter and stand about 0.10 from its meadow.

**A crossfade blends every color field one by one** - see ThemeBlend. They stand in a row from
skyTop through lilyFlowerTint, and colorsNum() counts them.

## SceneTheme cowTips

**The tint shared by the horns, hooves and tail brush.** Its alpha sets the tint's strength over
the coat shading and the share of cowSpots set by cowLimbInkShare. The tint is transparent by
default.

## SceneTheme lilyFlowerTint

**The tint over the Mill's water lily flower.** Its alpha sets the strength of the tint over
petal3. It is transparent by default.

## SceneTheme reedStem

**The color of the reed stalks in Mill and Chase**, tuned for Mill's far bank. It also colors the
cattails' leaves. Mill shades it in the foreground and hazes it in the small reed beds.

## SceneTheme airHaze

**How thick the theme's air is, from 0 to 1.** It is the share of the air's color a layer takes at
the ground line at the scene's reference distance - in ChaseScene, the far hills'; see hazed for
the air's color. Nearer layers take less of it, farther ones more.

- 0 is no air: every layer keeps its own colors.
- 0.10, the default, is clear weather.
- Rain, snow, dust and fog take more.
- 1 turns every layer the scene hazes into the air's color.

A crossfade blends it with the effects.

## SceneTheme cowLimbInkShare

**How much of cowSpots goes into the cow's far legs, tail shaft and tips**, from 0 to 1. The default
of 0 keeps their shading derived from cowBody and cowDetail. A crossfade blends the share with the
effects.

## SceneTheme celestialGlowStrength

**The multiplier for the wide glow around the sun or moon**, from 0 to 1. The default of 1
draws the glow at full strength. The Mill uses the same multiplier for the glow that lights
the clouds. A crossfade blends the strength with the other effects.

## MillTheme

**The colors only the mill wears**: its walls, its roof, its timber, the lamp behind its windows,
its wings' cloth and the water wheel. Its plinth, its chimney and its door take SceneTheme's stone
and bark, which the Chase's boulder and trunks wear too. Every theme fills every field but
windowLight.

**timber is the mill's wood**: the window and door frames, the wings' stocks and hub, the wheel's
axle and the trough.

**windowLight is the lamp behind the mill's windows**, as strong as its opacity. A theme with no
lamp lit leaves it out, so it stays transparent and the windows show their dark glass; a crossfade
toward a theme with a lamp lights it by degrees.

**The water the trough pours is SceneTheme's foam**, at the opacity of the drops running down the
wheel as they start. The drops, the steam where the pour lands, the foam at the wheel's foot, the
rings and the ducks' wakes are foam too.

## ChaseTheme

The Chase's own colors in a theme: the tree a branch run ends at, the pebbles, the four runners'
coats, most of its props and the stars circling a dazed wolf's head. The scenery it shares with the
Mill is SceneTheme's.

**Figures and props are well lit, at night as by day.** They start from Clear Day's colors, tinted
a little toward the theme's sky, and keep their color - except in Paper Sketch, whose look is a
lack of it. Each object then moves lighter, or at most 0.10 darker, only as far as its contrast
needs. Midnight Fantasy, Toxic Waste and Steampunk keep their objects in the theme's own light, with
the ones that read too dark or dull lifted one by one. Vintage Noir's grays are set by hand, with
one deep red for its apple, its toadstool's cap and the wolves' tongues.

**The tree a branch run ends at has a crown of its own** - treeCrown, treeCrownLight and
treeCrownMiddle - lit as the props are. The far hills' crowns and the Mill's keep the theme's light;
they are drawn from treeLeavesBase, crownLight and crownMiddle.

- A wolf's head and ears stand against the mountains; a hare stays below the far hills' top. Both
  are measured as they are drawn, hazed.
- A mountain's clear colors keep the tint of the theme's light; only their lightness moves.
- theirWolf stays darker than ourWolf.
- A coat's tongue, mouth, teeth, tail, inner ears and nose rise with the fur where it is lifted and
  never darken with it, so a white tail stays white by day.
- Yellow things - the banana peel, the rubber duck, the ball's yellow, the flowers' hearts, the daze
  stars - keep their color at night and stay light, as a dark yellow reads as olive. The ball, the
  flowers' petals and the whole apple stay lighter at night too.
- Every theme's petals come in three colors. A white petal is joined by a blushed pink and a
  lilac, or by cream or pale blue where one of those is lost on the meadow; a colored petal by its
  hue turned a little either way. Some themes choose their own: Twilight's run from a light rose to
  lavender, Magic Forest's lighter still, Cherry Blossom's are its falling petals' pinks, Lunar
  Eclipse's dull purples, Cyberpunk Night's magenta, neon green and acid yellow, and Midnight's
  are darker. On snow - Winter Frost, Winter Night and Arctic Dawn - every flower is white, the
  butterfly white with an icy blue edge, and the trail an icy gray.
- The toxic and neon themes and Underwater Realm give the toadstool caps and the apples toxic reds:
  a lava red in Toxic Waste, a hot crimson in Cyberpunk Night and Underwater Realm, a neon red in
  Neon City Night. Vintage Noir's apples, its toadstool's cap and its wolves' tongues share the one
  deep red in its gray.
- One green, grassBlade's light, serves the flowers' stems, the big daisy's leaves, the tufts on
  the lawn and the grass blades; every third blade, and every third tuft in the Mill's meadow,
  wears its dark, a shade darker. Midnight's, Toxic Waste's and Neon
  City Night's are lighter, to show against their dark meadows; Toxic Waste's is a dim toxic green,
  and Neon City Night's the green of its trees, muted. Cherry Blossom's plants are a dark sage, on a
  gray-green meadow by a rose-gray trail. Vintage Noir's plants are gray, as the rest of it is.

**The themes not of this world dress the plain things in their own colors** instead of daylight's:
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
- Midnight Fantasy: moonlit blue-gray and indigo, with pale lilac, mint and pale gold.
- Steampunk Adventure: limewash and aged oak, with brick, brass and verdigris.

## ThemeBlend

**What a scene that offers themes is drawn in.** pick() names a theme by its index in
Themes::allThemes, and worn() crossfades to it over 555 milliseconds, eased in and out, colors and
effects alike. A pick made during a crossfade starts the next one from where the blend stands.

**Different particle effects take turns during the transition.** The first fades out before the
next fades in. Lightning blends independently of them.

**worn() is for drawing, picked() for the rest** - the theme's name, and colors that are read once
rather than every frame. worn() takes the picked theme's name at the pick.

**Always a fade in.** worn() starts as a null theme - every color zero and every effect off - and
the first theme fades in out of it. A pick made before the first frame, as a host restoring a config
makes, fades in that theme instead.

**wornVersion() moves on with every step of the crossfade**, so a scene that keeps something drawn
in the theme knows to draw it again - MillScene keeps its sky and its land in bitmaps.

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
click there is the scene's first - see Scene doubleClickView. A press anywhere else leaves the
scene moving as it was, and one on a control over the scene, such as the readings beside it, is
that control's. The pointer shows a hand wherever a drag moves the view.

**A double click the scene leaves is the window's.** Where a drag does not move the view, where
the scene answers that it took nothing, and on the readings over the scene, the showcase enters
fullscreen or leaves it.

**The scene's context menu opens at the pointer** on a right click in the scene or its readings.
Tab or a click focuses the scene. Its keyboard context menu opens at the padded top-left corner.
The Scene line wears the picked scene's icon and opens the scene picker beside it. On
a scene that offers themes, the Theme line under it wears the picked theme's swatch and opens a
theme picker. The pointer resting on a scene or a theme crossfades to it, and leaving the picker or
closing the menu brings back the pick. A click on a scene picks it and leaves the menu up; a click
on a theme picks it and closes the menu. Full screen follows, the F11 toggle, its line selected
while fullscreen, and the menu ends on the application's Exit.

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

**The scene answers whether it took the double click.** True keeps it the scene's. False, which
is what a scene that overrides nothing answers, hands it to the host - see Scene offersView.

## SceneChoice

**A scene states what else it lets the viewer pick, beside its look**: each choice has a name and
its options' names, and the host shows it as a caption over a radio line for every option, in the
options the scene's tile opens. The names are shown as they are written, and they are also what the
scene keeps its pick under.

## Scene choices

**The host asks for the choices and leaves the picks to the scene.** It reads chosenOption() to
show what is picked and calls chooseOption() when an option is picked. The scene keeps its picks
with its view - see Scene storeView - and an index past the choices it states is a mistake it may
refuse.

## Scene switches

**A scene states the on-off settings it offers by name**, and the host shows each as a check line
with that name in the options the scene's tile opens. The host reads isSwitchOn() to show the line
and calls setSwitch() on a click. The scene keeps its switches with its view - see Scene storeView
- and an index past the switches it states is a mistake it may refuse.

## SceneGesture

**A scene states the gestures it answers, for the viewer to read**: each has a name and its effect,
and a host that shows them writes a line each, the name in bold and the effect after it.

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

**The shapes may lie over one another.** A fill follows the path's fill rule, which is SVG's
nonzero unless the path asks otherwise: figures that run the same way round join where they
overlap, and a figure running the other way cuts a hole. The shapes a batch takes are all wound the
same way, so overlapping ones read as one. See PixelPath fillRule.

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

## SceneLayer draw

**`shift` moves the layer**, in the frame's units. The kept pixels move by the nearest whole number
of pixels, so they lay in as sharp as they were kept; drawn live, the part moves by `shift`
exactly.

**A frame fits the kept pixels** where it maps to pixels as the params do, or differs from that only
by a whole number of pixels across and down - on the same CPU canvas or another, as
WaterReflection's own. Where it does not, the part is drawn live.

**A shifted layer uncovers a strip** at each edge it moves away from. The caller fills the strip,
as Bot Talent fills it in the wall's and the floor's colors when the stage shakes.

## hazed

**Distance is haze.** A layer's clear colors are taken toward the air's color, as far as its
distance goes. The distance is read off the layer's parallax - its share of a passing camera's
speed: a layer at a third of it stands three times as far as one at 1. ChaseScene's trail moves at
1; MillScene gives each of its layers the parallax it would have, the mill's foot being 1.

**The air's color is the sky's top, a third of the way to its bottom.** Haze takes a layer's own
color away and puts the air's in its place: by day mostly the sky's blue with some of the horizon's
light, at night near black.

**A layer at k_hazeReferenceParallax, 0.2, takes the theme's airHaze.** Every stretch of air as long
as the one from parallax 1 to that layer takes airHaze of what is left, so a layer x such stretches
beyond parallax 1 takes 1 - (1 - a)^x of the air's color, where a is airHaze. In clear air, 0.10, a
layer twice as far beyond as the reference takes 0.19. ChaseScene's far hills stand at the
reference.

**The air thins with height.** A point above the ground line is seen along a line rising through
ever thinner air, so it takes the air along that line in place of its distance, and less haze than a
point at the ground line as far away. The air thins to 1/e every 24 frame heights, as seen at
parallax 1.

## shadedForeground

**The foreground shade reduces OKHSL lightness and saturation by a quarter.** The returned color
keeps its original opacity.

## WaterReflection

**What a scene shows mirrored in its water**, laid into the water's shape and broken by ripples.
The scene draws the parts itself, each upside down about the line where it meets the ground or the
water; the reflection does not turn them.

**The still parts are kept.** What `still` draws is kept until forget() is called - MillScene
calls it when it draws its land again - or until what holds it goes: on the CPU backend a bitmap
of the reflection's own, kept with the water's coverage for one frame size and one top row; on any
other backend a kept image of the surface's, which a resize or a lost device empties. `moving` is
drawn over them every frame.

**A surface keeps one reflection's still part at a time.** Off the CPU the still part lives in the
surface's kept image 6 and each frame's mirror in image 7, so a second reflection with a still part
on the same surface would draw over the first's. A reflection with nothing still passes an empty
`still` and leaves image 6 alone - the Comet's lake does, so the Mill's kept mirror lives through a
visit to it.

## WaterReflection draw

**Each row is shifted sideways by two waves**, by farReach at the water's top row growing to
nearReach at the frame's bottom, and shaded a little darker in bands, then faded to the ripples'
opacity and cut to the water. The waves run a whole number of times a cycle, so a cycle ends where
it began. Both ways below take each row's shift and shade from one formula.

**On the CPU backend the rows are shifted in memory.** The mirror is drawn on a canvas of the
reflection's own, each row moved and scaled by its shade and by the water's coverage, and the band
goes in with one drawBitmap. A row moved past the band's side repeats that side's pixel there, so
water that reaches the frame's edge stays mirrored to it.

**On every other backend the rows are laid from kept images** - see IBackend drawImagePart. Image 6
holds the still part. Each frame image 7 is cleared and given image 6 and the moving parts, and then
laid into the water under a clip to it, one drawImagePart for each run of the surface's rows that
shares a shift and an opacity. The rows are the surface's own, so the ripples are as fine as its
pixels at any scale, and the pixels stay on the backend's side.

**Where a backend cannot build the images**, the parts are drawn live, clipped to the water and
faded, and the water lies still under them.

## rippleShiftAt

**The shift WaterReflection draw gives row y**, from the same formula, for what a scene lays on the
water itself to move with the mirrored rows - the Mill's sun sparkles, say. Hand it the Ripples the
reflection is drawn with, and y in the frame's pixels.

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
  contours at 200, 1000, 2000, 4000, 6000 and 8000 meters, made coarser to the grain of the cover.
  Each point takes the deepest contour that holds it, counted evenly from 0 at the coast to 1 past
  8000 meters, and the levels are blurred into slopes about a degree across, so the depth rises
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
reads 0, so the depth is for coloring the sea, with the land drawn over it.

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

## LightTown

A town the lights gather about - see NightLights. **Its size sets how much gathers about it**: the
light at its middle, how many rays and spokes it sends out and how far they reach, the villages
about it, and how far it raises the chance of villages over the land around it.

## LightQuery

The four spans run in step, one value per point, and the longitudes set how many points are read.

**A point's footprint is the ground it spans**, in radians of arc - for a point drawn as a pixel,
the ground under that pixel. It sets the size of the cells the tiny lights stand in, and how wide
the glow gathers its light.

**The darkness switches the lights on**, from 0 by day to 1 deep in the night. A point at 0 reads 0,
and 8 points at 0 together are passed over, so a caller sets 0 wherever it wants no light: by day,
over the water, past the limb.

## NightLights

The towns' lights at night, as countless tiny lights gathered into sprawls: thick in the towns,
running along the coasts and out into the land, scattered in villages between, and never over the
water.

**The lights are built from the towns and the land**:

- Each town is a star - a blot of light at its middle, and rays wandering out over the land, fading
  along their length - with a few villages about it.
- The towns of size 0.1 or more send out spokes, the larger the more and the longer. A town near
  the coast sends its first along the coast, a little inland; the rest wander inland, turning aside
  from the water, and the largest towns' spokes fork.
- Each town sends a corridor to the two others nearest it within 1.6 degrees, where the land joins
  them.
- Villages stand over the land from 56 degrees south to 72 north, by the chance the land's map
  gives a town there, and more of them near the towns.
- Noise clumps the light, and thins it between the clumps.

Every point of the light that lies over the water holds none. `inhabited` holds the outlines whose
land may hold lights, as landOutlines() gives them, so an outline left out holds none. One set of
towns, outlines and map always makes the same lights.

**The light is kept as the light a pixel of its ground gathers**, the same at any altitude, at 8192
points round each circle of latitude from 85 degrees north to about 61 south, and at 8 coarser
levels of detail, each half as fine each way as the one before. The points are kept a byte each, in
tiles of 16 by 16, and a tile with no light is kept once: about 6 MB.

**Building the lights takes about a second of one core.** Build them off the paint.

**The reads work through eight points at a time with AVX2**, and several threads may call them at
once. They wrap round the longitudes any number of turns either way.

## NightLights readLights

The light the tiny lights lay on each point, from 0 up.

**Each light stands at a spot fixed to the ground**, in a cell of its own. A cell holds a light by
the chance its ground's light gives it, and the light's brightness is drawn so a few stand out far
brighter than the rest; where the ground's light would fill every cell more than once, the cell's
light is that much brighter. Each light lays a tent one pixel in radius, so a pixel gathers on
average the light kept for its ground at any altitude, and the lights hold still as the Earth turns.

**The cells grow with the ground a pixel spans**, doubling in size from level to level. Each point
reads the two levels whose cells stand nearest 2 pixels across, and the coarser takes over across
the middle of each doubling, so no light pops in or out at once as the view climbs or comes down.

**The lights switch on one by one as the darkness rises**, each at a darkness drawn for it, and
brighten over the next 0.12 of it. The densest lights switch on while the darkness rises to 0.35,
the sparsest over the whole of its rise.

## NightLights readGlow

The glow over the ground about each point: the light kept for the ground about 6 pixels across
around it, read between the two levels nearest that, at 0.12 of its strength and as strong as the
darkness.

**The glow changes slowly from point to point**, so a caller may read it at points a few pixels
apart and spread it between them, as EarthScene does at every 4th pixel and row.

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

**Swarms drawn together take different seeds.** With one seed, the k-th firefly of each stands at
the same share across its area and wanders and blinks in step with the others' k-th; seeds 10 apart
keep them apart.

## StarField

**Each star grows and brightens through its own twinkle.** The star disappears at the low point.
The field's seed selects the positions, relative brightness, and twinkle rates.

## StarField drift

**A star moved out of the band comes round into it at the far side**, so a drift of whole frame
widths and band heights draws the field as it stands at none. Keep the drift to its part past the
whole numbers: a float holds a large drift too coarsely for the stars to move smoothly.

## StarField bottomBrightness

**The bottom brightness is a fraction from 0 to 1.** Set it below 1 to dim stars as they approach
the bottom of the band. The fade begins one fifth of the way down the band.

## BotTalentScene

An android puts on a show on a small stage, under a spotlight between red drapes. A curtain of
the drapes' cloth closes between its acts.

**Fifteen acts of ten seconds each, four to a cycle**, follow one another across the cycles, so the
show comes round every 150 seconds: PuppetShowAct, RobotDanceAct, JugglingAct, AccordionAct,
AcrobaticsAct, TopHatAct, OneManBandAct, BallBalanceAct, GatorAct, BalletAct, KettlebellAct,
WolfHoopAct, StringsCutAct, SwanLakeAct and CurtainCallAct. The accordion and the one-man band come
three acts apart.

**The curtain is shut for 0.6 seconds across each change of act**, and runs for 0.75 seconds
either side of that, so the next act is in place before it parts - see BotTalentScene
drawCurtain.

**The marionette hangs on a stand** inside the left drapes through every act it takes no part in
while the stand is there, swinging a little, 12 times a cycle - see hangMarionette. Each act says
how the stand stands - see StandPose.

**An act's jolt shakes the stage.** From the moment the act's joltAge passes 0, the stage and all
on it shake by up to 7 units, dying away in about half a second. The drapes and the hanging
marionette swing on for about two seconds. The curtain and the valance hang from above and keep
still. The strips the shaken stage uncovers at the top and the bottom are filled in the wall's and
the floor's colors.

## BotTalentScene drawCurtain

Each half of the curtain is 14 folds wide. The two halves run out from behind the side drapes and
meet 5 units past the middle of the frame.

- **Moving, a half's hem trails its top**, by how fast it runs, so the cloth swings as it is
  drawn.
- **Shut, a warm glow rests on it.**
- **Its folds stir** twice a cycle.

## ActStage

The stage as BotTalentScene lays it out in the frame, handed to every act it poses.

**`closeness` is 1 in a frame at least 640 units wide**, and the width over 640 in a narrower one.
An act multiplies its distances across the stage by it, so its figures stay between the drapes.

## StandPose

How the marionette's stand stands in an act, as BotTalentAct standPose answers it. BotTalentScene
draws the stand by it on every frame, over the stage and under the spotlight.

- **`shown`** fades the stand as one image, so where its parts overlap they do not show through one
  another.
- **`bow`** bends it toward the middle of the stage - see drawMarionetteStand.

## BotTalentAct

BotTalentScene poses an act, reads where the spotlight falls and whether something has jolted the
stage, and then draws it into the frame the jolt shakes.

- **pose keeps all that draw needs**, from `actTime` - 0 to k_actSeconds - and the scene's time;
  draw reads nothing else.
- **usesMarionette** is true in the acts that play with the marionette. In the rest the scene
  hangs it on its stand.
- **standPose** is whole and upright in most acts. StringsCutAct takes the stand away in a puff of
  smoke, SwanLakeAct dances on without it, and CurtainCallAct has it back, bowing - see StandPose.
- **joltAge** counts the seconds since something landed hard enough to shake the stage, and is
  below 0 before that - see BotTalentScene.
- **marionetteStringDeflection** gives the sideways motion of the hanging puppet's head string
  in sprite units. The scene passes it to MarionettePose.

## danceSway

**The marionette dances 16 sways a cycle by the scene's time**, with a step on every half sway -
see danceMarionette. Read from the scene's time rather than the act's, the dance keeps one beat
through every act it plays in.

## drawMarionetteStand

A pole on a round foot, an arm out to the right at its top, and a gold hook under the arm's end,
where hangMarionette hangs the control.

**A bow bends the pole above about a third of its height.** The bend is an arc of the pole's own
length, turning up to 0.9 radians at a bow of 1, so the top comes forward and down. The arm and the
hook turn with the top, and the arm dips like a head.

## PuppetShowAct

The android stands to the right of the dancing marionette and works its control from above, its
arm reaching up and over. It bobs on every step and watches the puppet.

## RobotDanceAct

The android wears a fedora, a black sequin jacket, and an oversized white glove on the audience's
left. It opens with two arm poses, then moonwalks across the stage and back from beat 2 to beat 10,
turning at beat 6. Each beat is 0.625 seconds. A shuffle leads into a wide stance, the fedora
dipped, the gloved hand at the hip, and the other arm outstretched. The android sways with its feet
planted as the stage darkens.

## JugglingAct

**A cascade of three balls**: a throw every 0.42 seconds from each hand in turn, 235 units high,
and every ball rests 0.3 seconds in a hand. A hand catches out wide and throws from near the
middle, and each ball changes hands with every throw. The android's eyes follow the highest ball.

**One throw goes too far**, and the android stops the ball with its leg. In seconds:

- **At 5.04** the cyan ball leaves the left hand, as high as any throw, toward a point 146 units
  right of the android.
- **From 5.16** each hand keeps the next ball it catches and holds it out for balance.
- **From 5.58** the android lunges right: the left knee bends, and the right leg goes out nearly
  straight, its heel on the floor and its toes up.
- **At 6.09** the ball lands on the shin and rolls down to rest against the toes. The android
  balances it there, the body teetering and the arms answering, its eyes on the ball.
- **At 7.56** the leg kicks the ball up to the right hand, and the android rises back to its place.
- **From 7.98** the hands throw their balls again in turn, and the cyan ball lands in the right hand
  at 8.52, so the cascade goes on by its usual order.

## AcrobaticsAct

In seconds:

- **To 0.9** it stands in a star, feet apart and arms in a V.
- **To 2.4** a side somersault: a crouch, a tucked turn high in the air, a landing in a crouch.
- **To 3.9** the star again, then over onto its head, traveling 70 units to the left.
- **To 7.0** a headspin on the antenna, three turns with the hands off the floor, sparks flying.
- **To 7.9** down onto its feet, 70 units further left, then the ta-da.

restAndroidOnFloor keeps it on the boards while it is upside down.

## OneManBandAct

The android strums a guitar and keeps the beat on a drum set with its foot. The set stands to its
left: a bass drum lying on its side with a pedal at its batter head, a tom on top, and a crash
cymbal on a stand behind. In seconds:

- **To 7.8** four chords, a bar of 1.25 seconds each. A kick every 0.625 seconds to 7.5: the foot
  presses the footboard, the beater swings onto the head, and the head flashes.
- **From 6.6** the guitar's neck goes up.
- **From 7.5** a fill on the bass drum, a kick every 0.3125 seconds.
- **From 7.8** the strumming hand reaches for the drumstick behind the left ear, where only a short
  end shows. It draws the stick out at 8 and raises it over the cymbal.
- **At 8.44** the stick strikes the cymbal with the last kick.
- **From 8.6** the ta-da, the stick held up.

Each bass drum kick makes the cymbal wobble and the hanging marionette's strings vibrate.

**The stick stays behind the head until it points away from it**, so it never crosses the face as it
comes out.

## BallBalanceAct

The android uses a tutu as a board atop a circus ball 54 units in radius. The ball rolls 150 units
either way, times `closeness`, once in 6.6 seconds, and comes to rest in the middle by 8.4. The
android leans as the ball rolls and finishes on one leg. The tutu's center rests on the ball's
top, and the android plants its feet on the tulle just in front of that center.

**The tutu clears the hanging marionette.** The ball's reach shrinks in a narrow frame to leave
room for the marionette at the left end of the roll.

## GatorAct

In seconds: the gator gapes from 1.3. The android kneels and leans in from 2.3 until its head is in
the mouth, one hand on the snout. From 3.9 it lets go with both hands while the jaw trembles. Its
head is out at 5.6, the jaws snap at 6.02, and it wipes its brow and takes the ta-da.

**The head goes between two calls**: drawGator draws the gator but its upper jaw, the android
comes next, and drawGatorUpperJaw puts the jaw and its teeth over the head.

## BalletAct

The android dances in a classical tutu and pointe shoes, its pink chest light alone lit. In
seconds:

- **To 2.1** it stands in fifth position, then rises onto pointe as its arms go up through first
  to fifth, rounded over its head.
- **To 3.1** a bourree to the left, its feet trembling on their points, its arms opening to
  second.
- **To 3.5** down off pointe into a plie in fourth, ready to turn.
- **To 5.4** three pirouettes on the right pointe, the left foot drawn up to the right knee.
- **To 7.1** a penche: upright on the left pointe, it tips to the left as the right leg rises
  straight out to the right.
- **To 8.45** a grand jete to the right, the legs flung apart a little below level, so they show
  beneath the tutu.
- **Then** the reverence: the knees bent, one foot behind, the arms opened low and the head
  bowed, while the chest lights flash.

## KettlebellAct

In seconds:

- **From 1** a squat to the handle; from 1.6 a strain.
- **From 3** the bell flipped up to the chest; from 4 a dip and a press overhead.
- **From 4.7** the bell held overhead, trembling, steam puffing from the android's ears.
- **At 6.45** the android lets go and hops back; the bell falls forward, turns back onto its base
  and lands.

**The landing is the act's jolt** - see BotTalentAct joltAge. Cracks run across the boards, dust
flies out either side, and the android is thrown off its feet, looks at the bell and flexes.

## WolfHoopAct

The wolf is the Chase's, in WolfCoats Gray. In seconds: he waits by the stand panting to 1.4,
then runs, leaps clean through the hoop and runs off past the android. At 4.2 he comes back in from
the right, hops too low, catches the rim and goes over onto his back. He lies dazed under circling
stars and is up again by 8.6. The android holds the hoop out at its side, sends him, cheers,
holds its head, and takes the ta-da anyway.

**He runs in front of the android**, and through the hoop between its two halves.

## StringsCutAct

The puppet show's finale, in a top hat. In seconds:

- **From 1.2** the wand is raised, and from 1.6 waved round.
- **At 2.15** sparkles fly to the control, and from 2.4 they run down the strings.
- **By 3** the strings have faded, and the control goes up in sparkles. The marionette dances on
  alone in a glow, while the wand beats time.
- **From 3.6** the wand is raised again and waved round from 4, the android's head and left hand
  turned to the stand. At 4.55 sparkles fly over to it.
- **From 5** the stand goes in a puff of smoke, and is gone by 5.25.
- **Then** the wand beats time again, while the marionette rises and twirls.
- **From 6.8** the android stands still, facing the audience, its left hand on its chest. From 7 the
  wand is raised a third time and waved round.
- **From 7.8** sparkles spiral up round the android.
- **At 8.25** it goes in a puff of smoke, fading out inside it in a tenth of a second. Its top hat
  hangs in the air a moment, drops to the floor, hops and rocks still. The smoke is still there as
  the curtain closes.

## SwanLakeAct

Four androids of the one model dance the little swans from Swan Lake, in a line, their arms crossed
in front and each hand holding a neighbor's. Their tutus differ a little - see TutuLook. Each dances
a few hundredths of a second ahead of or behind the music, so they keep almost together, and one
turns its head the wrong way once. In seconds:

- **From 1** on pointe, a step a beat, the feet lifting in turn to the other shin, as the line
  travels right. The heads turn sharply every two beats.
- **From 4.75** two echappes, each a beat out to second in a plie and a beat back to fifth on
  pointe.
- **From 6** a bourree back to the left, the heads turning again.
- **From 8.1** the steps again, on into the curtain.

**The marionette and its stand are gone**: StringsCutAct, just before, took the stand away in a puff
of smoke.

## CurtainCallAct

The whole cast takes its bows in a row across the stage, while bouquets fly in from the audience.

- **The stand** stands empty inside the left drapes, back from the smoke StringsCutAct took it in.
  It nods at 2.4 seconds and bows on its own at 5.4 - see drawMarionetteStand.
- **The gator** pokes in from the left wings, its tail offstage, grinning and nodding.
- **The marionette** stands on its own feet, its strings gone - see standingMarionette. It
  curtsies, waves and curtsies again.
- **The android** stands in the middle in its top hat. From 1.15 seconds it lifts the hat off and
  sweeps it down to its side with a bow, the hat turning over, and at 2.45 the rabbit pops out of
  it. It waves from 3.2, catches a bouquet at 7.15 and holds it up from 7.75.
- **The wolf** stands on the right facing the rest, wagging his tail like a happy puppy, panting
  and nodding, an eye on the rabbit.

**From 8.3 the whole cast bows together**, as the android's chest lights flash.

**Nine bouquets land in front of the cast** through the act. Each is thrown from below the front
of the stage, turns over in the air along its arc, bounces once and lies where it fell.

## AndroidBody

The android's measures in units, x to the audience's right and y down, from the middle of its
pelvis as it stands upright. A place on its left mirrors the right one's by its x. An AndroidPose
gives its hands and feet in the same units.

## AndroidPose

A pose places the pelvis in the frame and gives everything else relative to it.

- **`tilt`** turns the whole figure about the pelvis; pi stands it on its head.
- **The feet** are ankles in units about the pelvis, or knees while it kneels, and the legs bend
  to reach them. plantAndroidFeet sets them from points of the frame.
- **The hands** are in units about the pelvis in the frame of the upper body, which leans by
  `bend` at AndroidBody Waist, so a hand on the chest goes with the chest as it leans.
- **A limb stretches to a hand or foot beyond its reach.** Within reach, an elbow or a knee bends
  out to its own side, and an elbow set to ElbowBend Down bends down.
- **`elbowPosition`** places an elbow in the hand's coordinate frame. Use `elbowPositionBlend`
  from 0 for the solved elbow to 1 for the specified position. The hand stays at its solved point.
- **`kneeOffsetX`** shifts a standing knee sideways in pelvis units while its ankle stays fixed.
- **`facing`** narrows the figure as it turns and shows its back below 0: vents on the chest and a
  plate over the face. At its narrowest it keeps 0.03 of its width, so a turn passes through a
  sliver.
- **`forearmOnTop`** leaves a forearm and hand to drawAndroidForearm, drawn after what the hand
  holds, so the hand closes over it.
- **`wearsHat`** puts a top hat on at a slant, the antenna through its crown.
- **`wearsTutu`** puts a classical tutu round the hips, drawn between the pelvis and the torso - see
  drawTutu. It keeps its width as the figure turns, and `tutuTurn` turns its pleats, so a spin shows
  on it. `tutuLook` sets one tutu a little apart from another of the same cut - its shade, its size,
  how it sits on the hips.
- **`wearsPointeShoes`** dresses the feet in pink satin, the ribbons crossed over the shins.
- **`bow`** bows the android to the audience: the upper body shortens as it tips toward them,
  and the face goes down, the eyes on the floor. The head keeps its shape.
- **`headOffsetX` and `headOffsetY`** move the head sideways and down in the upper body's
  coordinates. The neck follows the head, and `headTurn` controls the angle independently.
- **`cycleTime`** drives the blink and the chest lights' chase; `eyeOpening`, `lights` and
  `antenna` set them by hand.

restAndroidOnFloor moves the pelvis until the lowest of its soles, palms and antenna tip touches the
floor line.

## androidWornHat

The top hat the android wears, as a TopHatPose in the frame: the middle of its brim, its turn and
its size, as AndroidPose wearsHat draws it. An act that takes the hat off the head starts it from
here and draws it itself, with wearsHat false.

**The pose follows the head as it turns and bends.** A figure turning away squeezes the hat it wears
across, which a pose cannot carry, so the pose is exact for a figure facing the audience.

## drawAndroidSilhouette

The silhouette follows the android's body and limb pose in the supplied frame. It draws the bare
figure in a solid color. Use an opaque color for an even fill where body parts overlap.

## danceMarionette

Places the control for one moment of the dance and hangs the puppet from it.

- **The main bar tilts with the sway** and carries the head and hand strings, so each tilt lifts
  one hand.
- **The leg bar hooked under it rocks twice a sway** and lifts each knee in turn, as the feet step.
- **The arms and legs are reached from the strings** with reachToward; the control stands 284
  units above the feet, over the puppet's head.

## hangMarionette

The marionette on its stand between its acts. Its control hangs by the middle from `hook`, tilted,
and the puppet hangs limp 232 units below, its head drooping. `swing` turns the puppet about the
control and throws its limbs the same way.

## standingMarionette

The marionette stands about `feet` with no strings or control, swaying a little by `time`.

- **`curtsy`**, 0 to 1, bends its knees with the left foot behind, spreads its arms low and drops
  its head.
- **`wave`**, 0 to 1, lifts its right arm and waves it.

Draw it with drawMarionetteFigure.

## drawTopHat

**What the hat holds goes between two calls.** drawTopHat draws the crown, the band and the brim,
the inside showing when the hat is upside down. Then come the head that wears it or the rabbit
coming out of it, and then drawTopHatBrimLip, which draws the near half of the brim's rim over
them.

## gatorUpperJawToFrame

The upper jaw turns up by `gape` about the back of the mouth, on a head that turns up by
`headLift` about the neck. A point of the jaw, given in the gator's units as the jaw lies shut, is
carried through both turns and placed in the frame at the gator's size. A hand resting on the snout
is placed this way, so it rides up with the jaw.

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

## MillScene

A watermill on a pond, in a valley under two ranges of mountains.

**Depth is drawn through hazed.** Each layer takes the parallax it would have: the mountains the
Chase's 0.02 and 0.05, so a theme's mountain colors read alike in both scenes, the far hills 0.07,
and the valley floor row by row, from the far hills' parallax at their foot to 1 at the mill's foot
and more toward the viewer. A knoll keeps 0.4 at its top. What stands on the land is drawn at the
size its row's parallax gives. The near bank lies in shade, and everything on it but the fireflies'
light keeps three quarters of its Okhsl lightness and saturation, its snow included.

**The sky and the land are kept as two bitmaps**, drawn again when the bounds' size changes or the
worn theme moves on - see ThemeBlend. The stars and the clouds pass between the two, so the land
hides their lower part. The reflection, the bridge, the light on the water, the cow and the tuft she
grazes, the predator and the fir he peeks from behind, the cattails across the pond, the mill, the
lily, the near bank's grass, cattails and daisies, the fireflies and the weather are drawn every
frame. The sun's glow lies over the land as over the sky, so the mountains it reaches are lit, and
each cloud takes as much of it as falls on its middle; the sun's disc keeps its own color.

**The pond mirrors what stands by it** - see WaterReflection. The mountains, the far hills, the
bridge, the firs by the shore and the mill's house are kept; the wheel, the wings, the cow, the
cattails across the pond and the ducks are drawn every frame, each mirrored about the line where it
meets the ground or the water. The bridge stands over the water, so it is drawn after the reflection
rather than kept with the land. The smoke, the water the trough pours and the steam where it lands
are left out of the mirror.

**The mill is drawn in five parts** - the smoke, the wheel, the house, the water, the wings - in
that order, so the house stands in front of its smoke and its wheel, and the wings in front of
everything. The smoke, the falling water, its steam and the foam come round a whole number of times
a cycle, so nothing jumps when the cycle wraps.

**Its story runs over two cycles.** The cow walks along the pond's far shore to drink at the river's
mouth and back, the ducks cross the pond and the birds the sky; a frame whose SceneTime has an odd
cyclesPassed shows the second half. A host that counts no cycles shows the first half only.

**The weather and the fireflies are spread over four depths** - the far hills, the field, the mill
and the near bank. Each depth's share is drawn over its scenery and stops at its bottom row; the
farther it lies, the smaller and fainter its pieces, and the slower they fall on the screen. The
lightning comes down behind the land, and its flash lights the sky alone.

**The wind blows toward the right**, the way the mill's smoke drifts. Each layer's weather is drawn
in a mirrored frame, so the rain leans and the dust blows that way.

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

**The lake mirrors everything above it** - see WaterReflection - dimmed, its rows rippling, with
streaks of light running across. The whole mirror is drawn every frame: the stars twinkle in front
of the far ridge, so nothing in it can be kept as a still part.

Three shooting stars cross the sky in the quiet between the passes.

**Draw foreground** shows the pines and rocks along the near edge of the lake. The switch starts
on. The scene keeps it as `DrawForeground` in its own config section. See Scene switches.

**The shore and foreground pines share a greenery color.** The near ridge and foreground boulders
share a rock color. Pines and foreground rocks darken diagonally from upper left to lower right.
The angular rocks use the same gradient heights, with the gradient centered on each rock.
The round boulder has its own gradient. The shore and ridge take haze, and the foreground uses
shadedForeground.

## CometPose

**A pass is the comet's whole visit.** It falls along its course for `fallShare` of the pass,
lands at the course's end, and its tails fade over the next 0.08.

The head gathers speed as it falls, and the dust tail grows with the speed. The tail opens from
the nucleus with a rounded front over its first tenth. Sparks shed along the way drift down and
fade over 0.3 of a pass.

## CometPose outerGlowShown

Set `outerGlowShown` to `false` for icons where the wide halo would reach the tile's edges.
The smaller glow around the head is drawn during the fall.

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
the globe's way, and the sea's glint, drawn on the globe at a fixed size, shrinks with it. The space
is kept as a layer at the lowest altitude - see SceneLayer - and drawn live above it; so is the
ocean until the land's map is built. The view starts four notches up.

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
satellite goes on hovering and blinking, and the stars twinkling. The time is
the scene's, so a paused scene holds the count as well.

**The Sun stands still among the stars until a double click moves it**, and so does the orbit's
plane, so each orbit passes a day and a night: a day of about two minutes as the scene starts, and
longer or shorter as the Sun moves round. In the Earth's shadow the satellite darkens.
Its panels turn once an orbit to face the Sun, so in the shadow they show their backs.
The stars twinkle at the same brightness range throughout the orbit and drift a fifth of the
frame's width each orbit - see StarField drift.

**The cycle is 240 seconds.** The twinkling and the blinking keep the pace they would have in a
40-second cycle.

**The night falls by the sunlight on each pixel of the globe**: it starts at a sunlight of 0.03 and
is whole at -0.13, so the terminator curves as it does at any altitude, and the land, the sea and
the clouds darken by the same depth, toward a dim moonlit blue. The night lies thicker over darker
ground and thinner over lighter, by the ground's luma: deep in it, black ground is hidden whole and
white ground keeps a fifth of its color, so ice shows through as the moonlit clouds do. The
moonlight is 0.13 of the view. The towns' lights switch on one by one as the sunlight falls from
-0.01 to -0.08, the densest first. The atmosphere along the limb is blue by day, orange at dusk and
dark at night, by the sunlight on the limb, which rises evenly across it toward the Sun: three
rings, each fading out to both sides.

**The towns' lights are countless tiny lights gathered into sprawls** - see NightLights. Each
pixel reads the lights on the ground under it where the land covers it, and they whiten where they
crowd. Their glow is read at every 4th pixel and row and spread between, and spills a little past
the coast. Both fade toward the limb.

**The towns are placed by the land's map.** 1500 are tried at random points of the land and 5800 at
random points of the coastline, a little inland, from 56 degrees south to 72 north, each kept by a
weight: highest on low ground, and thinning with height, in the deep desert, in the tundra and a
little in the northern forest. The towns over the land thin away from the coast as well. The ice
of Greenland and Antarctica has none. No two towns stand within 0.2 degrees of each other, so their
sprawls stand apart. A town's size is a draw squared, so few are large - see LightTown. The towns
are the same on every run.

**Three layers of cloud float above the ground**: low heaps, a middle sheet and high streaks - see
CloudKind. Each is a map on a sphere of its own, 1.0, 1.4 and 1.8 hundredths of the Earth's radius
up, turning about an axis that leans up to 6 degrees off the Earth's. Each drifts steadily east
against the ground, 0.4, 0.6 and 0.9 degrees a second from the lowest up. The clouds fade into the
haze as they near the limb, turn a moonlit blue-gray on the night side, and pass over the towns'
lights.

**The land is worked out pixel by pixel inside the coastlines**, from the land's map - see EarthMap.
The coastlines are landOutlines(), built off the paint from the moment the scene is made; the first
frame waits for them if it comes sooner.
A height of 1 stands 0.014 of the Earth's radius up as drawn, about ten times the Himalaya's true
height, and each slope is lit by the Sun where it stands: lighter facing it, darker facing away. A
Sun lower than about 14 degrees lights the slopes as one at 14 degrees would, so the shading deepens
toward dusk without turning black, and it fades out across the terminator. Snow lies above a height
that falls from the tropics toward the poles, and past 66 degrees south it covers all the land.

**The sea is colored by its depth**, from the land's map - see EarthMap: greenish over the shelves,
deepening to blue down the slopes and over the ridges to the abyss, darkest in the trenches.
Realistic shades evenly between the depths' colors, its shelves about as light as the land beside
them; stylized cuts five flat tones, its shelves light as on a map. Until the map is built, the sea
is one gradient from the haze at the limb to the deep blue.

**The air's haze veils the ground by the air the view looks through to it**, past the air straight
down: none looking straight down, more as the ground turns away toward the horizon and the limb. The
air thins with height - over the Himalaya's crest a third of it is left - so the ranges stand out of
the haze, and the sea, at height 0, hazes as the low land beside it does. Where the view looks
through 8 times the air straight down, half the ground's color is veiled.

**The looks part at the clouds' edges and in the land.** Stylized cuts each layer's cover into two
flat tones, and the land into flat zones of green, steppe, sand, taiga and tundra with flat bands of
height and one tone of slope each way, from the broad relief. Realistic shows the cover as it is,
and the land in the detailed relief, its colors blended - see SceneLook.

**The maps are built off the paint** the first time the scene is drawn: a fraction of a second on a
processor with many cores. The clouds show from the first frame after, and the land is drawn flat,
green with the ice sheets white, until its map is ready. The towns' lights are built from the land's
map, and show from the first frame after both are ready.

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

**In the Earth's shadow the craft keeps 35 percent of its color**, by `sunlit`. Its panels turn
about their booms once a round of `orbitTime`, so past edge-on they show their ribbed backs, and
the craft rocks once a round. The navigation lights blink 16 times a unit of `blinkTime`, the
strobe 20 times, and the panels' glints sweep twice.

## drawTrunk

**A trunk is a stump without its slice.** It is drawn to the stump's measures, 11 units either side
of its middle, and scaled from them by `halfWidth`: its roots flare out over the ground, and its
bark shades from `bark` on the left to `barkShade` on the right. Faint grooves run up it to `height`
in three lanes, each lane a run of short wavy segments nudged off its line. A trunk's grooves are
the same every time it is drawn, so they hold still as it scrolls. The Chase's stumps, its trees
near and far, and the Mill's pines are all drawn with it.

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
- another couple, a black wolf and a gray hare, runs the other way, doing as the two do; the hares
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
down. The duck goes flat under his paws and, a little after he is down, fills up again, grows legs
and walks off away from him. The ball rolls on a little and, while he lies dazed, a little way back.
The grass flattens under him and springs back, rocking for most of a second, and the rake's handle
swings up into his face.

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
spins round and is left pointing back the way they came, the boulder, the tree stump, the banana
peel and the apple bounce after each of them, and the toadstool's cap hops on its stalk, then off it
to land beside it. The bird, the butterfly and the crow on the stump fly off, the frog hops back
into the grass, and the snail draws into its shell until the wolf is by.

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

**The landscape is layers at their own speeds** - clouds, two ranges of mountains, the pale hills
beyond the meadow's, the meadow's own hills and what stands on them, the ground and the grass in
front, each a share of the camera's speed. A layer scrolls by the journey, the camera's whole
travel, so it never jumps when a cycle comes round. A period that the cycle's travel divides still
brings a layer round with the cycle - see ChaseScene periodNear - and a strip longer than any frame
carries the sky and the far cover without showing one of its copies beside itself.

**Everything is drawn in the theme the scene is given** - see Scene offersThemes. The sky, the sun
or the moon, the clouds, the meadow and the tree a branch run ends at are the theme's shared
colors; the rest is the theme's ChaseTheme.

- The theme's fireflies keep to the far hills' layer: behind the trail and the runners, smaller,
  moving with the hills, and hazed as the hills are.
- The theme's rain, snow, falling leaves or petals and dust are shared out over four layers, from
  between the far hills and the trail to the front grass - see ChaseScene drawWeatherLayer.
- Lightning strikes behind the mountains, three times each time the weather comes
  round, and its flash lights the sky above the ground line.
- The theme's stars come out over the sky. Stars, fireflies and weather come round twice a cycle.
- The little flowers in the lawn take a fifth of the lawn's color, so they lie in it rather than
  on it.
- The foreground grass grows out of the lawn: a blade's root takes 0.4 of the lawn's color at the
  bottom of the frame, and its tip is the theme's grass.
- The far hills and the lawn are one meadow - see ChaseScene meadowGradient.
- The sky's clouds, the flocks crossing it and the woods on the pale hills each keep a strip longer
  than any frame, so none of them repeats within a cycle - see ChaseScene drawClouds,
  ChaseScene drawSkyFlocks and ChaseScene drawFoothills.
- The trees, coppices and bushes along the meadow's hills, and the hedges on its slope, are placed
  along a strip of their own - see ChaseScene drawHillside.
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

## ChaseScene drawWeatherLayer

**The theme's weather is shared out over four layers**, at 0.4, 0.7, the whole and 1.35 of the
camera's speed: two between the far hills and the trail, the trail's, and the front grass's. They
hold 0.4, 0.27, 0.19 and 0.14 of its drops, flakes, leaves and streaks, each share spread evenly
over the area its layer covers.

**Falling particles follow the Mill's pace.** Larger particles fall faster. Each layer's fall
pace accounts for its height and the Chase's longer cycle. The same effects draw snow, petals, and
leaves swaying in both scenes. Rain falls straight down within each layer. Dust blows at a rate
proportional to its layer's parallax. The effects round their pace to whole trips so they loop
smoothly.

**A nearer layer is drawn larger and stronger.** Size and strength grow from 0.6 of each at the far
hills' depth to the whole size and 0.85 of the strength on the trail, and on at the same rate: the
front grass's layer is drawn at 1.18 of the size and 0.96 of the strength. The weather falls against
the sky as well as the scenery, so it is drawn fainter with the distance rather than hazed.

**The shares keep the weather equally dense at every depth.** A layer's share times the square of
the size it draws at is the same for all four, so a farther layer's smaller pieces stand as thick as
a nearer one's. A layer added or moved takes its share by that rule.

**Each layer is drawn over its own scenery.**
- The two layers between are drawn over the far hills. They stand behind the ground and reach down
  to the ground line.
- The trail's layer is drawn over the trail, the runners and their daze stars, and under the near
  verge and the front grass. It reaches the bottom of the frame.
- The front grass's layer is drawn over the front grass, nearest the camera, down to the bottom of
  the frame.

The theme's fireflies keep to the far hills, and its lightning strikes behind the mountains.

## ChaseScene drawBackdrop

**The backdrop is drawn from the sky down to the ground line**, in the order its layers stand:
the sky with its stars, the clouds and the flocks, a rainy theme's lightning, the two ranges of
mountains, the pale hills beyond the meadow's, the meadow's hills with the trees and hedges on
them, and the theme's fireflies.

**The mountains are hazed by height**, in k_mountainHazeSteps steps from the top of their gradient
to the ground line, so their feet take their distance's share and their peaks less. In clear air the
near range, at 0.05 of the camera's speed, runs from 0.39 at its foot to about 0.36 at its peaks,
and the far range, at 0.02, from 0.72 to about 0.55. Both are k_mountainWaves, the far range scaled
taller and the near one lower, each shifted in phase.

**Both ranges share one gradient.** mountainTop sits at k_mountainTopShare of the frame's height,
above every range's peaks, and mountainFoot at the ground line. Sunlight grows with a slope's height
above the ground, wherever its range's top is, so each range takes the colors at its own heights:
the far range, taller, reaches nearer to mountainTop, and the near range only its own share.

- The far hills are part of the meadow, whose top color alone is hazed - see ChaseScene
  meadowGradient. Their trees are the tree's own colors and the fireflies over them the theme's
  effect colors, both hazed as at the ground line. Both mountain ranges are ChaseTheme's
  mountainTop and mountainFoot.
- The weather is drawn fainter with the distance instead of hazed - see ChaseScene drawWeatherLayer.

## ChaseScene drawClouds

**The sky's clouds keep three distances.** A low bank of small cloud moves at 0.03 of the camera's
speed over a 3600-unit strip, six to it, and the mountain peaks stand in front of it. The high wisps
move at 0.018 over a 4300-unit strip, three to it, and the heaps of cloud at 0.045 over a 3400-unit
strip, ten to it. Each strip is longer than the widest frame, so one copy of it fills the frame and
the sky comes round once every few cycles.

**No two clouds are the same shape.** A cloud's silhouette is a run of three to five lobes over a
flat base; each lobe takes a share of the width and a peak of its own, and the valley after it stays
above the base, so the lobes read as one mass. A wisp is one long hump and a smaller one beside it.
A cloud takes 50 to 100 per cent of the theme's cloud color, a wisp 30 and a low cloud 35 to 65.

**Each bank also drifts on its own clock**, at k_wispDrift, k_lowCloudDrift and k_cloudDrift units a
second - the farther, the slower - and bobs a unit or two as it goes, so the sky is never a painted
plate while the camera stands. A bank's drift wraps at its own strip, and the strip repeats, so no
cloud jumps when it comes round. The clock is the scene's seconds counted across cycles, so the sky
carries through the loop's end instead of snapping back with the cycle's fraction.

## ChaseScene drawSkyFlocks

**The flocks cross the sky at 0.08 of the camera's speed**, three to a 5600-unit strip, in the
theme's flock color hazed to that distance and drawn at 80 per cent of its opacity. A flock is
three to five birds at places of their own, each a swept pair of curves for its wings rather than
two straight strokes. Each bird beats 2.1 to 3.2 times a second on a phase and a rate of its own,
and drifts a little about its place, so no two in a flock beat together.

**A flock flies on the scene's own clock, not the camera's.** Its place on the strip advances by
k_flockSpeed and wraps at the strip's end, so the flock crosses the sky even where the camera
stands; the camera stops at the story's beats, and a flock held still by it would show at once.

**The clock counts across cycles.** The scene's time is the cycle's 0 to 1, so a per-second motion
takes it multiplied by the cycle's length, k_cycle - and the cycles passed are added, so the beat
and the flight carry on through the loop's end rather than snapping back with the fraction. Passing
the fraction straight would run every beat one cycle too slow - a bird beating once a minute.

## ChaseScene drawFoothills

**A pale ridge stands behind the meadow's hills**, at 0.06 of the camera's speed, and shows through
the meadow's dips. Its color is the theme's hillTop hazed at that distance, and nothing else: the
layer stands far enough back that the theme's own air parts it from the meadow's hills, which stand
at 0.2, so it reads as land farther off than the meadow and not as sky. Its crest is k_foothillWaves
over a 1400-unit period, and it is filled down to the ground line behind the meadow's hills.

**Its crest carries hedges and trees.** Sixteen trees stand along a 5200-unit strip, each a trunk in
the theme's bark under one to three crowns of the layer's foliage; the crowns of a tree overlap and
run the same way round, so one path fills them as one - see ScatteredShapes. Five more trees to a
4900-unit strip stand taller, the same bark and crowns. Fourteen short hedges to a 5800-unit strip
lie on the slope below the crest, each 42 to 78 units long, each wearing the theme's hedge color,
hazed at this layer's distance - the air alone parts a piece from the band, which is why a far hedge
is a veiled thing and never a dark band laid on it. A piece lies level, or at a slight tilt of its
own, so it does not follow the crest's line: its height below the crest changes from piece to piece
and along each one.

**The layer's foliage is mixed in clear air and then hazed.** The crowns take the theme's
treeLeavesBase shaded 0.3 of the way to hillBottom, the hedges the theme's hedge color, already
stood against the clear hill - see ChaseScene drawHillside - and the air at 0.06 is applied to both
afterwards, as to the ridge itself. Nothing here is mixed toward the ridge's color: that would fade
it into the land it stands on. That length is the meadow's 140 to 260 units brought to this layer's
distance: it stands at 0.06 where the meadow stands at 0.2, so a thing of the same real length shows
at three tenths of it, and the hedges are the same size in the world on both layers. Each keeps a
strip of its own, so no two of the layer's parts come round together.

## ChaseScene drawHillside

**The meadow's hills carry five kinds of tree and a bush.** A stand is a round tree, a broadleaf, a
poplar, a fir, a coppice of three trees or a bush, picked by the stand's own number; the round tree
and the broadleaf are the common ones. Forty-four stands lie along a 6400-unit strip, each at its
own place along it. A stand lower down the slope stands nearer and draws larger.

**The hedges run along the slope below the trees**, two runs to the same strip, each 600 to 1300
units long, and stand lower on the slope than the trees and are drawn before them, so a crown or a
trunk hides a run where the two meet. A run comes apart in stretches of 140 to 260 units, with a gap
of 70 to 150 units between them where a gate or a break in the hedge would be. A run is its own line
down the slope, not the hill's contour: it is laid from its start height with a slow sway and a tilt
of its own, and pushed down only where the crest falls into it, keeping 6 units below the crest and
12 above the trail.

**A hedge wears the theme's own hedge color**, not one worked out from its hills: a color that has
to read on a high-contrast theme and vanish into a washed-out one cannot be derived, so each theme
states it, as it states every other color. The layer's air is then applied to it as to everything
else at that distance, so the haze, not the hedge, decides what the distance makes of it. Its humps
rise and fall by about half its 16 to 24 units, with a taller clump now and then, and a stretch
closes on a rounded shoulder at each end, so no cut edge shows and the length reads as a hedge rather
than a band.

## ChaseScene meadowGradient

**The far hills and the lawn are one meadow**, filled with one gradient, so no seam shows where
the hills meet the lawn. It runs from the theme's hillTop, hazed as the far hills are, at
k_meadowTopShare of the frame's height - above the hills' highest tops - to its clear hillBottom at
the frame's bottom. Only the top color is hazed.

## ChaseScene periodNear

The layer moving at `parallax` covers `parallax` times the cycle's travel in a cycle. The period
returned divides that distance a whole number of times, as near `wanted` as a whole number allows,
so every tile of the layer has moved on by whole tiles when the cycle comes round.

**A layer is placed by the journey**, the camera's whole travel, which carries every layer on across
the cycle boundary without a jump. A period that the cycle's travel divides makes the layer come
round with the cycle too, so the ground, the grass and the weather keep their periods. The sky and
the far cover take a strip longer than any frame instead - see ChaseScene drawClouds, ChaseScene
drawFoothills and ChaseScene drawHillside - and come round once every few cycles.

**The mountain ranges scroll by the camera's whole journey** and repeat over their own period. A
range moves less than one period in a cycle, so a period fitted to the cycle would squeeze its
ridges.
