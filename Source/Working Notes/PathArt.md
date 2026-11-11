# PathArt

The author's side of the footnote of the same name.

## Scene

Scene was cut out of MillScene when the showcase gained more pictures than the mill: MillScene
derives from it, named Mill, and the seven others come from createScenes(). The showcase holds the
mill as a member, since its theme picker drives it, and the others in a list after it.

The theme picker belongs to the mill alone and stands on the mill's page of options; a
hover preview over the Scene group shows the scene under the pointer and puts the picked one back
when the pointer leaves. The config keeps the scene by name in `Scene` and the theme in
`SceneTheme`, so a scene added or moved in the list does not shift what was stored.

The list runs Puppeteer, Ghost, Clockwork, Chase, Comet, Earth, Orrery: the Chase and the Orrery
changed places (Oj, 2026-11-10, "Swap Orrery and Chase"), and createSceneViewsSchema follows the
list, the Earth's view before the Orrery's.

The theme list runs from Fresh Spring through the day themes to the night ones, Calm Night last
(Oj, 2026-11-10, "Reverse the scenes theme list, make the Fresh Spring default"). Fresh Spring is
the default by being first: ThemeBlend and the showcase both start at index 0. A stored pick is
found by name, so it still opens in the theme it names.

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
of option names; the host builds a picker per choice the way it builds the look's, previews an
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

**doubleClickView answers whether the scene took the double click** (Oj, 2026-11-11, "Double click
which a scene has not handled", for the showcase's fullscreen). The default answers false and the
Earth answers true. ScenePanel asks only where a drag moves the view, as before, and enters or
leaves fullscreen on a double click nothing took.

**The panel hears the double click as it bubbles** (Oj, 2026-11-11, "It fails to catch double
click under(and on) the readout"). ScenePanel::doubleClick was raised only for a double click on
the panel itself. One on the readout, the chart or the bar holding them has that control for
its receiver, there being no ClickTarget::Self above it. nestedDoubleClick passes through the
panel from all of them, and the scene is still asked only where the panel itself was clicked.

**Every scene draws its own icon** (Oj, 2026-11-09, "Make the scene buttons big tiles with top
icons, in rows"). Of a still of each scene, the scene's own sprite and eight new line pictograms,
mocked up from the scene code, Oj picked the sprite. drawIcon is a pure virtual on Scene rather
than a painter table in the showcase, because the colours a figure is drawn in - the Chase coats,
the clockwork's metals - belong to the scene.

- **Framing.** Each figure was posed in the mockup harness and its extent measured where alpha
  passes 10 of 255. The pose is written about the centre of that extent, and SceneFrame::Spanning
  is given its longer side, so the figure fills the shorter side of any bounds. At 48 pixels most
  figures read as specks, so the showcase draws them in a 64-high slot.
- **Clips.** Where the mockup's square cut a part, the code clips at bounds: the mill's reflection
  and wing tips, the marionette's strings, the Earth's glow. The ghost's and the comet's glows
  spill a few levels of alpha past bounds (3 to 9 of 255), unclipped.
- **The mill wears Fresh Spring.** In the theme it is picked in - Calm Night with no pick stored -
  the house is a dark grey that reads as nothing on a tile, and a tile is not repainted while a
  theme is previewed. millColorsOf builds MillColors for draw and drawIcon alike.
- **Shared colours.** The clockwork's four metals moved out of draw into the file's anonymous
  namespace, and Saturn's colour into OrreryColors, so the icons draw with the scenes' own.
- **Checked.** The icons match the mockup's; every scene's frames are bit-identical to before (the
  eight scenes at eight times each, 256 x 160).

**A scene taken up fades in over a still of the one before** (Oj, 2026-11-09, "Scenes crossfade?",
then "A"). Of three ways - the scene left kept as a still and faded out over the new one drawn
live, both scenes drawn live through the fade, and a fade out and back in - Oj picked the first.
Both live would draw two scenes a frame for the length of the fade, about 40 to 60 ms at 4K on the
CPU backend for the Earth and the clockwork, and each scene would need a clock of its own: the cycle
time is a share of the shown scene's cycle.

- **The still is the frame on the screen.** On the first full paint after the picker asks for
  another scene, MillSceneForm::keepShownScene draws the panel's frame again into kept image 0: the
  form's surface, then the shown scene at the time that frame was drawn at. The surface goes in
  because the mill leaves parts of its frame open and draws on it. In the harness the still matches
  the frame on the screen bit for bit on the CPU backend, for every scene in createScenes - the
  Earth with its maps built.
- **Mid-fade the still is the blend.** A switch while a fade runs lays the scene fading in over the
  still kept before, through an opacity layer at the share it has reached, so one image is enough
  and the screen does not jump: within 1 level in the harness, Puppeteer to Chase broken off 0.43 of
  the way for Ghost. A resize empties the kept images, and the scene is then drawn whole.
- **Only on a full paint**, by the 80% test the readings use. A still drawn in a partial paint would
  leave the rest of the panel to change at once; until a full paint comes, the scene being left
  goes on being drawn.
- **The switch frame is the still alone.** The scene taken up is not drawn under it, and its layers
  are prepared in that frame (9 to 47 ms at 4K). The fade's clock starts once the frame is drawn, so
  that time is a frame held on the still rather than a jump into the fade.
- **The share is worked out from the clock at each paint**: smoothStep over 555 ms, the length of
  the mill's theme crossfade. An AnimationController moves its value on its own timer between
  paints too, so a still drawn mid-fade came out a frame ahead of the screen - 14 levels in the
  harness.
- **Cost.** One kept image the size of the window for the window's life (28 MB at 3436 x 2051 on
  the CPU backend); one more draw of the scene left on the switch frame; a full-frame blend a frame
  through the fade.
- **Unchecked on Direct2D.** The still and the fade run through the backend's kept image and
  opacity layer, untried with the scenes. drawSoftEdge needs its target to be an
  ID2D1DeviceContext, so a still of the Earth may come out without its atmosphere's glow if the
  image's target is not one.

**A scene's options stand on a page of their own** (Oj, 2026-11-09, "if MillScene started with
another theme and then select Mill, its the theme list below is trimmed"). The Scene page held a
section for the mill's themes, one for the Earth's look and one per choice, each hidden while its
scene was not the picked one. A hidden control is not measured, and the menu is placed around what
its pages measure as it opens and keeps that window (AutoFit::No), so a menu opened on another
scene had no room for the themes and cut them when the mill was picked. Of three ways - the
options as pages of a PageControl sized for the largest, the menu placed again around its content
on every change, and the application's pages in a scroll box as Settings is - Oj picked the first.
Then the shape: one section, Scene Options, holding the page control; a page per scene with no
sections inside; a placeholder where a scene has nothing to set.

- **The room is the largest page's** - the mill's themes - whichever scene is picked, so a pick
  never changes what the menu needs. Under the Earth and the scenes with no options it stands
  partly empty.
- **Each picker is a column**: its caption in strong ink over its options - Theme, Look, Turn. The
  section captions carried the scene's name; a page shows only while its scene is picked, so the
  captions leave it out.
- **The section names the picked scene** (Oj, "Make Scene Options header name the scene"): Mill
  Options, Earth Options, rewritten on every pick in the heading style AppMenuPage::addSection
  gives a caption. The placeholder then says only "This scene has no options".
- **Compiled under clang only**, not run as the app. The arrow keys from the tiles into a picker
  pass through the page control there.

**The Earth's Turn gave way to hints of its gestures** (Oj, 2026-11-10, "Remove the Earth Turn
option. Add controls hints label instead"). Asked where the hints are written - stated by the
scene, as its choices are, or kept as the showcase's own text - Oj picked the scene: SceneGesture
is a name and an effect, and gestures() lists them. Asked whether SceneChoice goes with the Turn,
Oj kept it, with no scene stating a choice. The Earth turns eastward only, and a Turn value an
older config holds is no longer read. The showcase writes the hints a line each under a Controls
caption, the name in bold.

**A scene states its switches** (Oj, 2026-11-10, "orrery Show orbits option"). Asked whether the
option is a check box, which needed an on-off kind on Scene, or an Orbits choice of Shown and
Hidden through SceneChoice, Oj picked the check box. switches() names them, isSwitchOn() and
setSwitch() read and turn one by index, as chosenOption() and chooseOption() do a choice's. A check
box has no hover preview: a click turns the switch, and the readings start again. The Orrery keeps
its one switch in a view section of its own under OrbitsShown - a name without the space, unchecked
whether the config's names may hold one - and draws its orbit layers only while it is on.

**Each scene states its readout colours** (Oj, 2026-11-10, "Each scene states its own redout
colors"). Mockups first: each scene with its readout over the top right, against Calm Night's river
pair that every scene wore, and two pairs from its own palette - a dark field and a light series,
after the mill's pattern - with a second sheet asking whether the text belongs with them, the app's
light ink being unreadable over the Chase sky and the mill's day themes. Oj: "A with the text", then
B for the Comet ("less sticking out"). SceneReadoutColors carries the ink as well, light by default;
the Chase states CrowBody. The mill keeps its theme's river and takes the light ink or riverBottom
at half by the contrast against skyTop - contrastBetween moved from the cow into SceneHelpers for
it. The showcase writes the units and the chart's readings at two thirds of the ink, which needed
ChartAxisColor on FpsChart.

**A double click picks and closes the menu** (Oj, 2026-11-10, "double click on a theme or a scene
closes the menu"), on the scene tiles and the mill's themes. The first click of the double click
makes the pick, so the handler only closes: DoubleClickEvent::closeForm closes the menu and stops
the event, which keeps the form from taking the second press as another click.

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

**The towns stand at least 0.2 degrees apart** (Oj, 2026-11-09, "Many still overlap", with a
screenshot of the Black Sea at night, once pseudoRandom kept its whole fraction). Placed at random,
a third of the 7300 towns had another within 0.1 degree, and about 4000 touched another four
notches up at 3436 x 2051. Of a least spacing of 0.15, 0.2 and 0.25 degrees, in mockups of the
scene's own towns, Oj picked 0.2, and glowing towns kept 1.5 degrees apart: 44 glows stood within a
degree of another, and the 221 glows became 194.

- A site too near a town already placed is tried again, so the counts stand. At 0.3 degrees the
  7300 never all found room and the build never ended; 0.25 placed them. To watch: a larger
  spacing, more towns or lighter weights can run past that, and nothing ends the loop.
- The towns placed are kept by their cell of the towns' grid, and a site is held against the cells
  townCellsNear gives: a row either side, and up to 3 columns either side at 72 degrees. It places
  the same towns as holding each site against every town placed.
- At 0.2 degrees about 700 towns still touch another four notches up, and none at the scale of
  Oj's screenshot. With the whole globe in view about 4100 touch, as the lights shrink by the
  square root of the globe's size; left as it is for now (Oj).
- The towns spread evenly before the land's map is built are not spaced.

**The globe is one picture a frame** (Oj, 2026-11-11, "Earth performance is not great", with a
CPU profile of the Direct2D backend at bc12a5c). The profile put the paint thread at 93 % of a core
and about 43 frames a second, 22 ms a frame: 8.2 ms uploading the sea's, the land's, the night's and
the clouds' bands (CopyFromMemory, four fifths of it in the display driver), about 7 in the
atmosphere's three soft edges at 2.2 each, 2 in the city lights and 4.1 with the night side in
view - 1.4 of it a radial brush made for each glowing town - and 2.6 in its own share of the bands.
The 24 helper threads stood at 7 % each. Of one picture, the two largest costs alone and the
backend alone, Oj picked one picture, delivered with a sheet.

- **Direct2D puts a soft edge's blur off until the next flush**, so the profile shows two of the
  three under the next ring's SetTarget and the third under EndDraw, on one call path inside
  d2d1.dll; drawAtmosphere alone reads 4.9 of the 7 ms. What a blur's 2.2 ms go on inside Direct2D
  was not seen, as its symbols could not be loaded.
- **The night's bands were uploaded whole on the day side too**, as each spans the sphere whatever
  it holds.
- **Each band takes its layers in turn**: a layer's rows are worked out into the band's spare
  bitmap by the rasterizers as they stood, and laid over the band's by compositeSurface. Clearing
  the band and laying the four layers over it costs 8 of 75 ms of processor time a frame at
  3436 x 2051 on the container, the lights 0.13 and the atmosphere 1.7.
- **The bands start a whole number of 8 pixels down the canvas**, the most any layer's points stand
  apart, so a layer's rows in a band start on its own points' rows. A band keeps the rows from the
  frame's first on and the columns its outline covers within the frame, so nothing is drawn past
  the frame. Drawn apart, a layer's first row and column reached up to a step past it.
- **The disc's clip is not needed.** The land ends at the limb on its own: the stylized coast's
  loops are closed along the rim, inside the circle, and the realistic coast is cut by the sphere's
  coverage. The glows and the lights are cut by heldByDisc, the limb's own coverage.
- **A town's light is a true disc.** Through the path rasterizer a small dot was a polygon, a
  diamond at the smallest, as a quarter circle flattens to a chord or two at half a pixel's
  tolerance, where Direct2D filled circles. A disc softened over a pixel at its edge lays its area
  and a twelfth of a pixel over, so areaShareOf scales its opacity to the light its area holds.
- **The atmosphere's shades are the gradient the CPU backend made of the 41 stops**: 257 shades,
  each between its two stops, at each ring's strength. A pixel's nearness to a ring is measured
  from the circle itself, where the path rasterizer measured from the circle's chords, up to half
  a pixel inside it. Direct2D's blur gave each ring a bell where this gives it the even ramp.
- **Until the land's map is built** the ocean and the flat land are drawn as paths, the land under
  a clip of its own, and the picture - the night, the lights, the clouds, the air - lies over them.
- Measured on the container (2 cores, CPU backend, median of 20 frames): at 3436 x 2051, four
  notches up 60 to 67 ms became 49 to 57, at the lowest 40.5 became 32.4 to 34.7, with the whole
  globe 65 to 67 became 52 to 53; at 1600 x 900, 21.1 to 22.3 became 17.7 to 20.1. Direct2D was
  not built or timed here. By the profile's parts a frame there is estimated at about 8 ms of the
  paint thread: one upload at 2.7, the bands at about 3, the rest 2.
- Against bc12a5c on the CPU backend: space is bit for bit the same, and the ground within 2 levels
  by day; the towns' dots differ along their edges, as discs differ from polygons; the limb is
  within 41 levels below the whole globe and 58 with it in view, a level on average, where the
  chords stood off the circle. Frames under an inset and a scale of 1.37 differ the same way
  inside the frame. Two runs give the same frames bit for bit.
- Sheet "Earth one picture - before and after.png" in Claude Output\MillScene Scenes\Earth Speed.
- To watch: the ocean's layer is still prepared once the land's map is built, and on the CPU
  backend keeps its pixels unused. A paint of a part of the scene works the whole picture out, as
  the picture is cut to the canvas and not to its clip box.

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
0.02 dying away over 0.0375, about 0.75 s. A first try let it go at once and it flicked back while
he was still on it. It died away over 0.015, about 0.3 s, until Oj asked that it rock "more for a
while" (2026-11-10) and picked 0.75 s from 0.3, 0.75 and 1.5 s mocked up.

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

**The near verge** (Oj, 2026-11-09: "Put on the closer side of the Chase path casual objects too, in
different order, with mismatched period, so the combinations repeat never, or for a looong time").
The first mockup stood the near one 100 units ahead of the far one in every run, taken from a list
of its own by the golden ratio's fraction a run, with lists of 13 and 11 offered beside it. Oj: "Set
mismatching distance and offset between the neighbors in far and near rows so the rows are
independent and not aligned, use the same objects number". The near row stands at its own spacing
along the trail: a cycle's travel (14,388 units) over 2.618, the golden ratio's square - 5,496
units, about 1.15 runs - the first half a spacing in. Its places drift through every moment of the
runs; in the first two cycles one stands by the boot as the wolf trips and one beside the hare where
she sits under the tree.

A near place can come by while the camera stands, while the hare runs ahead past it or while the
wolf lies down, so the far row's clock - worked out from where the hare runs at full speed - does
not serve it. The second mockup started the hare's part of the stirring when she came close and the
wolf's part when he did, both found by going back over the scene's time. Oj: "there should be a
trigger moment starting a casual object animation after which it runs only forward without pauses
and don't track a subject". The hare's first coming close became the one trigger, and the stirring
played on from it as in a pass at full speed. The gust is the far row's wake worked out for the two
running by at 330 units a second, the wolf k_wolfAfterHare behind her - 0.89 s, the far row's gap in
a run over the trail at closeness 1. The daisy's petals and the reeds' fluff still leave from
k_nodBlown and k_leanTorn; at 1.12 s the scripted wake is 0.03 short of its height, about a unit at
the daisy's head.

The trigger is found by stepping back from now 0.05 s at a time until the place stood 1,800 units
ahead of the middle of the frame, keeping the earliest step at which the hare went from short of it
to close, and bisecting that step 16 times. It is the first coming close and not the last, since
under the tree she hops back past a place to her apple and comes by it again. hareXAt is the hare's
place taken out of draw, so the search reads the motion draw shows. It works the apple hop out in
units where draw worked in pixels, which moves at most four pixels by one level in the branch run
from 0.705 to 0.94; with the near row left out, every other frame is as it was (640 x 360, 1600 x
500 and 400 x 600, four cycles). Frame time at 1920 x 1080 is the same within noise, 5.4 to 5.8 ms a
frame either way.

The near row stands 38 units below the ground line, on the grass at the trail's near edge, at 1.15
times the far row's size. It is drawn after the wolf, the daze stars and the dust, before the front
grass. Its frog hops toward the viewer, growing, where the far one hops away. The place along the
trail is a double, cyclesPassed times the cycle's travel plus the cycle's own, which holds at
100,000 cycles.

Then (Oj) "wolf passing and hare passing animations should have their own trigger moments". The
wolf's first coming close is a second trigger, found in the same pass back over time with wolfXAt,
the wolf's place taken out of draw as hareXAt was; the far row and the runners are unchanged by it
to the bit. RoadsideMotion carries wolfTime, the clock the wolf's part of the stirring runs on: the
far row hands it sinceHareClose, the near row the seconds since he came close plus k_wolfAfterHare,
so the timings in the draw functions stand as they were. The wolf's part is the boulder's, the
peel's and the apple's second and third hops, the signpost's second turn, the toadstool's cap
leaving the stalk, the daisy's petals, the reeds' fur and fluff and the snail coming out again. The
near gust is two scripted wakes, each from its own trigger. Under the tree a daisy the hare passes
as she runs on keeps its petals while the wolf lies dazed and loses them as he comes by, about six
seconds later.

**The leap's ends and the apples go round in turns** (Oj, 2026-11-09: "Suggest ideas to make the
tree collisions vary from run to run", then "let's do your pick and my option is that sometimes he
meets a flying bird instead of a tree"). My picks were a different tree each time, flying into the
trunk, leaping onto the branch, and how many apples fall with one dropping off his head; his was the
crow. LeapEnd goes round in a turn of four cycles, and treeRunOf picks it from cyclesPassed. Cycle 0
is the low branch and four apples, the run as it was. The camera's timing is the same in every
variant: a cycle's travel has to stay whole for periodNear's tiling, so each way down fits the
camera's stop rather than moving it.

Oj then: "Fruits stay always apples" - pear, plum, cherry and apricot trees went. On "A couple of
apples survive always" the tree took eight apples, two of them new, and the two at the top of the
crown never fall; of six falling out of eight, two out of six, or dropping the variant, he took six
out of eight. Cycle 0 differs from before only by the two added apples on the crown, 117 pixels at
640 x 360; the first two runs are the same to the bit.

**Into the trunk** flattens him against it by 0.584, slides him down by 0.618 and topples him back
onto his back by 0.645. At the trunk's foot his middle stands 66 units over his feet's line; at 48
his hind legs went below the ground.

**Over the branch** he leaps 150 units high, tops out at 0.572 and lands with his middle 30 units
past the branch's end, where the low branch ends in the other cycles. WolfPose took `dangle`, legs
and tail hanging straight down, for while he lies across the branch, which gives under him and
springs back as he slips off at 0.70. Oj on the first version: "in 'over the branch' the branch is
facing the other way" - asked which way, he picked the branch mirrored to the trunk's far side, tip
away from him. The first mirrored version kept the tree where it stood and carried his leap 266
units further in the same half second, past the trunk. Oj: "in over the branch the wolf jumps the
same way, only the tree position is offset to the left so he lands on the branch". The trunk now
stands 75 units short of where he lands, 45 before the branch's end, and trunkFromBranchEnd(LeapEnd)
answers the trunk's place for everything that works from it - the apples, the hare's hop to hers,
his head under the falling apple.

Under that tree the hare sits about 300 units past the trunk and hops back to her apple, which lies
78 units past it. He slips back 85 units down the branch and lies at the trunk's foot, so his tail
stops short of her; at 40 he lay where she eats, and his tail reached into her. Taking an apple on
the trunk's near side instead put the trunk between them, but she eats facing left, away from him.

**The apple off his head** was the third AppleFall, in a turn of three cycles: the nearest apple at
least 40 units over his head at 0.66 dropped onto it and bounced 40 units on, and the hare hopped to
it. Taking the hare's apple there regardless had left his head above it in one leap's end, where the
drop came out as the square root of a negative and the apple never fell. Oj (19:58): "remove the
variant when the hare gets close to the wolf to get the apple" - after the low branch and the trunk
the apple lay by his head and she hopped onto him to take it. With Four and Six left, a turn of two
by cycle would tie each to a leap's end, the low branch and over the branch always four and the
trunk always six. The fall changes instead from one turn of the leap's ends to the next, as the
crow's start does, so eight cycles show every pairing.

**The crow** stands in for the tree, flying at him 160 units a second at the height of his head. Oj:
"The crow have two initial positions - one as you has shown, and the other one is sitting on a stump
and getting scared by their arriving". The crow comes in every fourth cycle, so a turn of two by
cycle would land on the same start every time; CrowStart goes by how many crows have passed instead.
The stump is the roadside one at 1.8 times, 44 units past the branch's end on the far verge, drawn
before the runners while the crow is drawn after the wolf. The crow sits pecking, looks up about two
seconds before and springs up 0.4 s before his face meets it. Then the hare is about one of her
lengths from the stump at 1920 x 1080, 1600 x 500 and 400 x 600; at 0.55 s she was still well short,
and it went up unprovoked.

**The other couple** (Oj, 20:11: "another idea - they are meeting another couple going the other
direction and the two wolfs collide"). Asked where and how the hares pass, he took a fifth way for
the leap to end and one hare bounding over the other. The other couple is ours turned about the
point on the ground where the wolves meet, which holds still on the ground while the camera moves -
the mirror of our wolf's middle is `2 * meeting - his` on screen, his pose drawn in a frame turned
about that x. So the other pair runs at our speed and does all ours does, and nothing of theirs is
worked out on its own. The wolves leap as at the low branch and meet where its end would be, and
both fall as from it. The hares cross where the mirror crosses itself: haresCrossAt halves the span
from 0.4 to the contact 20 times for when our hare reaches the meeting point, about 0.572 at 1920 x
1080. Then both wolves are in the air, so the hares pass under them. The bounding hare rises 70
units at the top over 0.012 shares either side, the other crouches with her ears laid back; ours
bounds in one turn and theirs in the next.

LeapEnd's turn is five cycles long, and the apple fall, the crow's start and which hare bounds
change from one turn to the next, so the whole repeats in ten cycles; cycles 1 to 4 are the same to
the bit as before. The two couples close at twice the running speed, so at 1920 x 1080 the other
hare is in view 0.8 s before the wolves meet and her wolf 0.6 s; offered to slow them, Oj kept it.
What stands by the path stirs only for our two.

Then (Oj) "Give them different coats": of a black wolf with a grey hare, a brown wolf with a white
hare and a rust wolf with a dark brown hare, he took the black and the grey. WolfSprite and
HareSprite take their fur as a WolfCoat and a HareCoat, as ReedsSprite takes ReedColors, and
ChaseCoats holds the grey wolf and brown hare that were the sprites' own colours beside the two new
coats.

Then (Oj, 20:29) "slightly shift other couple timings after the collide". Their poses are worked out
again at a time of their own instead of being ours turned: draw holds the wolf's and the hare's
moment as wolfAt(t, travelThen) and hareAt(t, travelThen, over, ...), ours called at the episode's
time with the same arithmetic as before, so our two are the same to the bit. Their hare's time runs
k_theirHareAhead (0.008, 0.16 s) ahead of ours, eased in from 0.59 to 0.61 while she runs on; their
wolf's runs k_theirWolfBehind (0.012, 0.24 s) behind, eased in from 0.64 to 0.70 while he lies
still, so no shift shows as it comes in. Their hare turns to taunt and runs off sooner, their wolf
gets up later, and his leg twitches and daze stars go out of step with ours. Up to 0.59 every frame
is as it was.

hareXAt, wolfXAt and cameraShare left ChaseScene for the file's anonymous namespace, since they take
the run's LeapEnd and TreeRun, which live there. With all of it, frame time at 1920 x 1080 is the
same within noise.

**Twice as much by the path** (Oj, 20:36: "place the side objects denser. Add a cow on the stump
that success to fly away", then "sorry its a crow, my typo"). Asked how dense, he took twice on both
edges. The near row's spacing is halved, twice the golden ratio's square of places in a cycle. The
far edge keeps its place in each run, since the warning banana peel and apple rest on it, and gains
a row between: three in a cycle's travel at even spacing, the first 0.81 of a spacing in. The runs'
places sit 1,848, 6,633 and 10,362 units into a cycle of 14,388, and that share sets each
between-place about halfway between two of them, the nearest 1,681 units off. The between row is the
near row's machinery on the far edge - drawNearVerge became drawVergeRow with a `nearEdge` switch -
so its stirring starts by the same search for when each of the two came close.

The crow on a stump is the thirteenth object, in the near row and the between row only, so the runs'
row stays in step with the trail obstacles. It is the leap run's crow at half the size on the
roadside stump, pecking at it, springing up as the hare comes close and flying off ahead at 300
units a second, climbing like the sparrow. Frame time at 1920 x 1080 is the same within noise.

Oj (20:49): "twice as often was a bad idea, now there's a clutter around the tree action". The far
between-row's third place, 13,477 units into the cycle, stood 13 units from where the camera stands
for the tree, 13,464, so every cycle had something by the tree, and the near row often added one.
Going back to the old density was shown first; Oj: "consider other options to clear them off of the
trees". Of leaving the tree's stretch empty, pushing what falls in it out to its edges, doubling
only over the trail, and the old density with the stretch kept clear, he took the first. Neither row
has a place within k_treeStretchReach, 700 units, of the tree's standing point - wider than the half
of a 16:9 frame, 533 units - and the check comes before the search for the two's close moments, so a
left-out place costs nothing. Even spacing could not keep the far row's third place clear of both
the stretch and the runs' places by more than 660 units, so it is left out rather than moved, and
the far edge has five a cycle.

Then (Oj, 20:59) "offset the closer row more"; shown the near row moved toward the viewer, he
answered "more to the right, further from the opposite-side siblings". The rows are spaced so they
never fall into step, so moving the whole near row along would only change which pairs stand
together: in the first ten cycles 14 of 60 near places stood within 350 units of a far one, the
closest 55. Instead a near place within k_nearClearOfFar, 500 units, of a far place - the runs'
three and the between row's two, worked out per call from cameraShare - moves on to 500 units past
it, and the tree's stretch is checked after the move. With the far places at least 1,680 units
apart, a moved place lands clear of the next far one and of the next near one, 2,748 units on. The
check is cheap; frame time at 1920 x 1080 is the same within noise.

**The snap in the first run** (Oj, 21:16: "Make the wolf clang his teeth on the first run too one
time"). Over the trail he is 185 units behind the hare at 0.5 and 150 at the trip, too far for his
jaws to reach her tail, where the tree run's snap comes at 122. So in episode 0 he lunges
k_firstRunLunge, 60 units, at k_firstRunSnap, 0.5 - a bump 0.012 wide, carried in wolfXAt so the
stirring by the path sees it - and his jaws open and shut as at the tree run's snap, through
chompAt, which branchRunAt now calls too with the same arithmetic. The hare's next hop rises 0.8
higher, a bump 0.008 wide just after the snap. The second run and the tree run are the same to the
bit.

Then (Oj, 21:21) "the snail stick out its head a little earlier". It came out from 1.8 to 2.6 s of
the wolf's part of the stirring, and at 1920 x 1080 the camera carries the near snail out of the
left edge at about 2.3 s, so its head was barely seen. It comes out from 1.3 s, as the wolf's tail
clears it, to 1.9 s.

**The trips kept clear** (Oj, 22:03: "Clear the side objects off the collision stretch", then 22:13:
"by the 'collision stretch' i mean the wolf's tripping site"). The between row's places, 3,885 and
8,681 units into a cycle, stood 24 and 35 units from where the camera stands at the two trips, 3,861
and 8,646, so in every cycle the wolf came down beside one of them in both trail runs. Its third
place stood by the tree and was already left out. The clear stretch now reaches round the camera's
stand in each of the three runs: k_treeStretchReach became k_collisionStretchReach, still 700 units,
and drawVergeRow takes each run's stand from cameraShare at k_cameraWaiting.

Left at 0.81, the between row would have lost all three places, so it moved. Two places were shown:
A, about where each run starts, and B, just before the trip's stretch - an offset of 0.63, 830 to
870 units short of a stand, going by about 1.5 s before the trip, while its stirring could still be
playing as he falls. Oj took A. k_farBetweenOffset is 0, so the places stand 0, 4,796 and 9,592
units into a cycle, 924 to 946 units past a stand. The nearest of the runs' places is the tree run's
own, at 10,362, 770 units off. The far edge has six a cycle again. The near row loses 1.24 places a
cycle to the stretches, against 0.51 before, and keeps 4.0.

The in-between place and the tree run's own stand 770 units apart, under twice k_nearClearOfFar, so
a near place moved past the one could land within 500 units of the next. The far places are sorted
along the trail, and clearOfFar goes over them in that order, moving the place on past each far
place it is still within reach of. Over the first 1,000 cycles no near place stands within 500 units
of a far one or within 700 of a stand. Frame time at 1920 x 1080 is the same within noise.

Then (Oj, 22:22) "the side objects shadows are black". The shadow helper in drawRoadside and the
toadstool cap's shadow called withOpacity on ChaseColors::Shadow, which sets a colour's alpha rather
than scaling it, so the shadow's own 0x47 became 0xFF and the shadows of the apple, the banana peel,
the signpost, the toadstool, the tree stump, the bird, the snail and the crow's stump were the
shadow colour at full strength. They take faded, which scales the colour's own alpha, and match the
shadows the hare and the wolf cast. ChaseColors::Puff, 0xB3, goes through withOpacity the same way
for the duck's air streaks.

**The mill's themes** (Oj, 2026-11-10 01:18: "I can easily see how Chase and Mill can share themes",
01:26: "1. shared" - one pick for both). Whether the figures should take the theme's light was left
to mockups: Chase in five themes, its world from the theme, the figures once in daylight colours and
once darkened as far as the meadow is darker than in daylight. Oj: "They both seem OK to me".
Daylight was recommended and taken: in the theme's light Cyberpunk Night turned the hare red and the
wolf purple, and Calm Night made both dark silhouettes.

ChaseColors' world colours became ChaseWorldColors, rebuilt every frame from the worn theme by
worldColorsOf, with k_daylightWorld the old values. The trail by a per-channel ratio came out purple
at night and pink on snow, so it is scaled by brightness instead: the square root of the meadow's
brightness over daylight's grassTop, held between 0.25 and 1.15, then 0.35 toward the meadow. The
mockup measured against Fresh Spring's hillTop. Several themes' shadow colour is opaque black, so
the Chase keeps its own 0x47.

Bark, leaves and stems are the world's on the tree a run ends at, on the meadow and on the daisy,
and daylight on the tree stump, its sprout, the signpost and the apple, which are objects. The stars
twinkle and the weather comes round twice a cycle: a whole number keeps the loop seamless, and 30
seconds is nearer the mill's 40 than the Chase's 60. The readout's CrowBody and SunGlow, picked that
night, gave way to the theme's river, as the mill's.

**Fireflies** (Oj, 14:20: "Fireflies don't suit, unless they are in background and moving with
camera"). They flew over the whole frame, in front of the runners. A mockup showed two places: A,
over the far hills at 0.6 size, moving with the hills at 0.35 of the trail's speed, and B, over the
meadow behind the trail at full size, moving with the trail. Oj, 14:24: "A is better - less
distracting". Fireflies gained an area and a size for it; drawWeather leaves the fireflies to the
scene, which places them with firefliesOf. The Chase draws them in tiles of the hills' period: a
swarm moved across a frame-wide area by a drift would not have come round at the end of the cycle,
as the frame's width does not divide the hills' travel.

Then (Oj, 14:35) "fade the fireflies": their light and glow stand back toward the mountains' foot as
the hills do, keeping their own alpha.

**Theme tables** (Oj, 2026-11-10 14:48: "themes should be merged into a simple color tables, with
all lacking colors added, and primary and secondary objects color carefully chosen to match the
theme and have enough contrast"). Picked: figures and props, each with its own colours per theme;
named fields. Contrast was picked as 3:1 and 2:1 luminance ratios, then (15:03) "use color
difference": the daylight Chase separates by hue - the brown hare stood 1.07:1 against the far
hills, the grey wolf 1.48:1 - and the ratios turned the daylight wolf charcoal and the daisies grey.
The thresholds, 0.15 and 0.10 in OKLab, sit just above the daylight wolf's 0.117 and hare's 0.123.

The tables were written by a generator (Claude Output\MillScene Scenes\Chase Themes\generator): the
world as worldColorsOf derived it, with the trail and pebbles washed out as far as the theme lacks
colour (Vintage Noir's trail was brown); each object lit by the theme (linear light times the root
of the theme sky's luminance against daylight's, at least 0.3), its chroma scaled by the theme's own
colourfulness and by the light, tinted 0.3 of the sky's shift from daylight, capped at 0.08; then
one lightness shift for the whole object, the smallest that reaches its threshold against its
backgrounds, each key colour kept between 0.55 of its daylight lightness and 0.3 above it. Without
the lower bound the grey wolf went black in Winter Night and Twilight. Backgrounds: the meadow and
the far hills for the runners, the mountains for the wolves too (Oj, 15:18: "in Cyberpunk Night the
wolf's ears are lost against the mountains"); the trail for what lies on it; the meadow, grass and
far hills for the verge; hills, mountains and sky for what flies. The trail behind the runners
forced near-white figures at night and was left out.

Short where the theme leaves no room: Winter Night's black wolf at 0.07, between dark mountains and
lighter hills; Underwater Realm's grey wolf at 0.149; Desert Dusk's hares at 0.11.

The mill's cow, mill and peeking wolf were measured on renders: each drawn once in magenta for its
outline, every pixel just outside it paired with the nearest one inside, and the 15th percentile of
their colour difference taken. 37 of them moved, among them a grey cow in Winter Frost, Golden Hour
and Paper Sketch where the white one stood on light ground, a lighter mill in the night themes, and
Lunar Eclipse's peeking wolf, which was the tree's own colour. Vintage Noir's mill stays at 0.055,
grey on grey.

Then (Oj, 15:57) "Yellow objects, like banana and rubber duck should keep read as yellow, the apple
core and beach ball be lighter at night". At night the peel and the duck had gone olive and brown,
the ball and the apple's flesh dull grey. The peel, the duck, the ball's yellow and the flowers'
hearts take at least 0.8 of the light, keep their chroma whatever the light and take half the sky's
tint; the ball takes at least 0.65 and the apple's flesh 0.7. Vintage Noir has no colour to keep, so
its yellows are light greys.

Then (Oj, 16:04) "move other effects onto the hills layer". The rain, snow, leaves or petals and
dust fell over the whole frame, in front of the runners. They are drawn now with the fireflies, in
each tile of the far hills, at 0.6 size, falling from the top of the frame to the ground line, which
the meadow then covers. Faded toward the mountains like the fireflies, the snow and rain vanished
against the sky, so they keep the theme's effect colours. drawWeather takes a Weather - the five
strengths and two colours - which weatherOf reads from a theme; the mill draws weatherOf(theme()) as
before, the same to the byte.

Then (Oj, 16:13) "fade too". Weather gained an opacity over its five strengths, 1 unless set; the
Chase sets 0.45, the share of their colour the hills keep, so the weather fades against the sky and
the hills alike. Its colour difference from what is behind it now runs from 0.05 (Arctic Dawn's
snow) to 0.28 (Vintage Noir's dust); 0.6 was tried as well. The mill is the same to the byte.
Then (Oj, 16:23) "go" on 0.6, which puts the difference at 0.067 (Arctic Dawn's snow) to 0.37
(Vintage Noir's dust).

Then (Oj, 16:28) "the big flower petals and the apples should stay brighter at night". At night the
daisy's petals had gone mid grey and the apples maroon. The petals take at least 0.8 of the light,
as their hearts do, and the whole apple - skin, seeds and leaf - 0.7, as its flesh does. The petal
colour is shared with the meadow's little flowers and the butterfly's wing spots, and the leaf with
the stump's sprout, so those rise too. In the night themes the petals go from 0.67 to 0.93
lightness and the apple's skin from 0.41 to 0.56; themes in full daylight are unchanged. Vintage
Noir's apple lifts to 0.63 to stand off its grey trail.

Then (Oj, 16:39) "On toxic waste make petals green, and on Cyberpunk and neon make them some toxic
colors". From mockups of three or four options each (Oj, 16:58: "C C A"): Toxic Waste's petals are
a glowing mint, 9CFFC4; Cyberpunk Night's an acid yellow, E6FF1A, with the hearts darkened to an
amber, D4972D, that the petals would otherwise swallow; Neon City Night's an electric cyan, 00FFF0.
The generator sets them outright after the lighting. The meadow's little flowers and the
butterfly's wing spots wear them too. Each stands at least 0.34 from its meadow.

Then (Oj, 17:07) "On Cyberpunk Night make them magenta, and on Underwater Realm cyan". Cyberpunk
Night's petals are FF00FF, its sun's magenta, and its hearts the lit yellow again; Underwater
Realm's are 00FFFF, the colour of its bubbles, 0.22 from its teal meadow.

Then (Oj, 17:10) "Are the two flowers on the lawn supposed to grow together?" They were not: of the
three flowers in each meadow tile, the first two fell 6 units apart, under a flower's 9.5 across,
and every tile repeated the pair. Each flower now keeps to the middle three fifths of its own third
of the tile, so any two stand at least 53 units apart, across tiles too. The pebbles and tufts
never met (61 and 23 units at the closest).

Then (Oj, 17:17) "the same treatment on the same themes to the red objects - toadstools and apples -
red but more toxic". From mockups of a neon red, a hot crimson and a lava red (Oj, 17:21: "C B A
B"): Toxic Waste's caps and apples are lava red, FF3D14 shaded B4290C; Cyberpunk Night's and
Underwater Realm's hot crimson, FF1460 shaded B30B41; Neon City Night's neon red, FF1F3D shaded
B31228. Each shade is its red 0.15 darker with 0.85 of the chroma, as daylight's apple is. The
spots, stalks, flesh, seeds and leaf keep their lit colours. The apples stand at least 0.31 from
the trail, the toadstools 0.36 from the verge.

Then (Oj, 17:28) "In the magic forest, twilight, lunar eclipse and cherry blossom rose to purple
petals", and from mockups (Oj, 17:38) "Lunar Eclipce C, others lighter tones", then (17:47)
"Twilight D, then make the other themes vary them too in the next step - and they are can be merged
with effect petals; Magic forest D, lighter; Cherry Blossom - take the effect petals". A theme now
has three petal colours, petal1 to petal3, beside effectColor2 rather than in ChaseTheme, shared
by the Chase's flowers and the falling petals; ChaseTheme lost its petal. The flowers wear them by
turns - the meadow's three in a tile one each, the daisies and the butterfly's spots by their
place along the verge or their run (petalOf). The falling petals blend from the effect colours to
petal1 and petal2 as far as the theme has petals, so Cherry Blossom's are its old effect colours
and the mill draws the same to the byte. The meadow's petals now all go down before the hearts:
with three shapes a flower's petals waited for the next tile and covered its heart's edge.

Twilight: FCABC4, E69EE8, BC9AFA. Magic Forest: FDC4D5, EDB6EE, C8B0F9 (0.05 lighter, 0.75 of
the chroma). Cherry Blossom: its effect colours FFC8DC and FF96B4 and FFAFC8 between them. Lunar
Eclipse: A15CFF in all three. Every other theme wears one colour three times until the next step.

Then the next step, from a proposal for the other 21 themes (Oj, 18:06: "lunar eclipse - duller;
cyberpank - different vivid colors; winter frost - lighter; others are OK"; 18:09: "Midnight -
darker petals, lighter stems"). A theme left with one petal colour now wears it and two neighbours:
a coloured petal (chroma at least 0.06) its hue turned 25 degrees either way; a white one a blushed
pink and a lilac, 0.05 of chroma as far as the theme has colour, no lighter than 0.91, and where
either is lost on the meadow (under 0.10) the first of cream and pale blue that stands off it -
Winter Night takes cream for the lilac, Arctic Dawn pale blue for the pink; Vintage Noir two greys,
0.10 and 0.05 darker. Lunar Eclipse's are the proposal's at 0.6 of the chroma, 0.03 lighter: A07EDC,
7E8DD8, BC73C3. Cyberpunk Night: FF00FF, 39FF14, E6FF1A. Winter Frost: FED1AD, E9DBA6, B0ECBC -
lighter only with warmth, as a light grey, pink or lilac sinks into its snow (0.03 to 0.07 from it).
Midnight: its proposal 0.18 darker with 1.3 of the chroma, B4ABAF, C993A1, AB96C6; its stem and
daisy leaf 0.18 lighter, 2C422B and 2E462F, 0.15 from its meadow, where they stood at 0.04 and 0.02.

Then (Oj, 18:16) "share the stems with the bushes and foreground grass, in midnight even darker
petals", and (18:17) "by bushes i mean the ones on the lawn" - the tufts. ChaseTheme's tuft,
flowerStem and frontGrass are now one field, grass. The stems and tufts already wore the same colour
in every theme but Midnight, whose lighter stems had left its tufts and front grass near black; the
front grass was 0.05 darker, and now takes the stems' green, its dark third blade lifted by as much
to keep its step. Midnight's grass is the stems' 2C422B, and its petals 0.15 darker again with 1.15
of the chroma: 877D81, 9D6374, 7F679C.

Then (Oj, 18:26) "On the vintage noire make the apples red". Its apple skin is daylight's, E24F41
shaded 9E2924, the one colour in the theme - on the tree, on the verges and on the trail, and the
crumbs the hare bites off. The flesh, seeds and leaf stay grey. 0.21 and 0.17 from its grey trail.

Then (Oj, 18:30) "Make the small flowers blend into the lawn". The lawn's little flowers, petals and
hearts, are mixed half way to the lawn's colour at the middle of the band they grow in (63 units
below the ground line, on the hillTop to hillBottom gradient the lawn is filled with), and stay
opaque, so a heart over its petals shows no ring; the batching by colour stays. 0.7 was tried as
well: at night it all but hid them.

Then (Oj, 18:43) "Let's experiment - a theme states clear colors - hills, trees, mountains. The haze
color is SkyBottom - SkyBottom is a 100 haze. The scene applies the haze to a layer based on the
distance to that layer - less to the hills, more to the mountains." Rendered in scratch first: an
exponential haze by distance (1 - e^(-kd), d = 1/parallax - 1) at two strengths; Oj took the
stronger for the hills (0.43) and less for the mountains; d / (d + 2.46) gives 0.43 and 0.75 on one
curve, and Oj took 0.75. Taking 0.75 of haze back out of today's mountains asks for black or clipped
blues - their feet are darker than that much haze allows - so each theme's clear mountain colour is
a rock (8C8CA0 over 5C5F78) in the theme's light, in the hue of its old mountains at 1.5 times their
chroma up to 0.12; Oj picked that ("B") over the haze taken out in OKLab ("A").

ChaseTheme lost farHillTop, farHillFoot, farTreeTrunk, farTreeCrown, farTreeCrownLight,
farTreeCrownMiddle, farFireflyLight and farFireflyGlow; mountainTop and mountainFoot are the clear
colours. ChaseScene hazed(theme, clear, parallax) draws the mountains (0.12), the far hills from the
meadow's hillTop and hillBottom, their trees from treeTrunk, treeLeavesBase, crownLight and
crownMiddle, and the fireflies from the effect colours (0.35). The generator measures the runners
and props against the hills and mountains as they are now drawn: the hazed hills are lighter than
the old ones, which stood toward the dark mountain foot, so where a runner had been lifted to stand
off dark hills it now goes the smaller way, darker - Twilight's, Underwater Realm's, Steampunk's and
Cherry Blossom's grey wolf and hare, as they are by day; keeping them light instead would have paled
Fresh Spring's brown hare. Winter Night's runners fall short of 0.15 (0.12, 0.13, 0.15) against its
pale hazed snow; Steampunk's grey wolf stands at 0.10.

Then (Oj, 19:27) "blend the foreground grass into the bottom lawn". Three ways rendered: every blade
at half the lawn's colour (A), the roots in the lawn (B), and the roots in the lawn with the tips at
half (C). Oj took B (19:33). Each blade is filled with one vertical gradient, from its own colour at
the height of the tallest tip (58 units above the frame's bottom) to the lawn's hillBottom at the
bottom, so a shorter blade starts part way down it. drawFrontGrass takes the SceneTheme for
hillBottom. A 3840 x 2160 frame on the CPU path costs the same as with solid blades (1.72 against
1.73 seconds for 18 frames).

In the same message (Oj, 19:33): "on toxic waste the stem color needs tuning". Its grass, 0B240D,
stood 0.02 to 0.05 from its lawn, so the stems, the tufts and the foreground grass all but vanished.
Three greens rendered near the hue of its lime fireflies, each with its daisy leaf 0.08 lighter as
daylight's is: a dim toxic green 26541C, a toxic green 367C14 and an acid lime 5AA925. Oj took the
dim one ("A", 19:39): 26541C, 0.12 to 0.20 from the lawn, with leaves 386D35. The dark third blade
is 214F17, the same green at the theme's own step, 0.017 darker. The generator's lift would have
kept the old blade's low chroma and turned it grey at that lightness, so it leaves a dark blade
alone where the theme sets it.

Then (Oj, 19:46) "On unrealistic themes, the objects like stump, signpost, crow, butterfly should
gain the theme's vibe". Seven themes were taken as unrealistic: Cyberpunk Night, Neon City Night,
Toxic Waste, Underwater Realm, Magic Forest, Midnight Fantasy and Steampunk Adventure. Lunar
Eclipse, Vintage Noir and Paper Sketch were offered and left as they are (20:08); the four themes
out of allThemes were not asked about. The generator's vibes.py gives each of the seven a palette
of slots - wood, woodTop, board, life, stone, crack, crow, beak, wing, bird, shell, reed, spines,
boot, metal and the rest - and maps each plain prop's field to one, with the field whose daylight
colour stands for the slot. A field takes its slot's hue and chroma, its own daylight step of
lightness from that key, and chroma in proportion to its daylight chroma against the key's (0.6 to
1.3). The contrast solve then runs on these colours instead of the lit daylight ones, bounded 0.25
below and 0.3 above them. The yellows, the ball, the toadstools, the apples' skins and the petals
keep their own rules.

Two palettes were tuned after the first render. Underwater Realm's driftwood (3F7480) and bird
needed lifting, which paled the coral sign, so both went darker (1F4048, 1A5A90). Steampunk's
walnut was pushed near black against its rust meadow, so its wood is an aged oak (A07A50) and its
reeds copper. Oj took all seven (20:08), with two changes. Cyberpunk Night's rake tines were lost
in cyan, so they are magenta. Neon City Night's sign is "dark fill and bright outline and letters":
its board is 101018 and its outline and writing (bough) 00FFF0, through two slots a palette may
add, ink and handle, which otherwise fall back to the wood. Its rake's handle is a light B0ACD8,
0.34 from the trail where it stood near 0.10. A board a step lighter (16141F) fell short against
the asphalt meadow and lifted the whole group by 0.22.

Then (Oj, 20:25) "On neon city make stems brighter than surface". Its grass, 1B2421 at L 0.25, was
darker than its asphalt lawn, which runs from 333333 to 1C1C1C (L 0.32 to 0.23). It is now 357339,
the hue of its trees (32CD32) at L 0.50 and 0.11 chroma, 0.21 to 0.29 from the lawn. Its daisy leaf
is 478D53, 0.08 lighter as daylight's is, and its dark third blade 306E34, the theme's own step of
0.017 darker. L 0.46 and 0.55 were the steps either side.

Then (Oj, 20:36) "replace the Cherry Blossom acid green with light gray, make the path light rose
grayish pink, calm the vivid blue sky". The sky and the meadow are the theme's shared fields, so the
Mill's sky and hills change with the Chase's. skyTop 1E90FF and skyBottom 6495ED became a calm
88AED9 and C8DEF1 (chroma 0.19 down to 0.075); hillTop 7CFC00 and hillBottom 556B2F a light grey
D8D9DE and B3B4BB. The trail is E6D3B5BB, a light rose grey, and the pebbles A99196. Grey plants
(A) and sage plants (B) were rendered; Oj took B (20:43): the stems, tufts and foreground grass
718F76, the dark third blade 638068, the grass patch 5D7A62, 718F76 and 89A88E, the daisy leaf
88A88E, the reed stems 718F76, and the Mill's lily pad 89A88E. The calmer, lighter sky raises the
theme's light from 0.65 to 0.90, so the runners and props stand nearer their daylight colours.
Two fall short against the grey: the palest petal, FFC8DC, stands 0.07 from the meadow, and the
Mill's white cow 0.11 from the hills, against 0.10 and 0.15; the cow had stood at 0.29.

Then (Oj, 20:47) "Make the lawn-hills darker, add just a bit of green". hillTop is A3AFA0 and
hillBottom 7E8A7C: L 0.74 and 0.62 where they stood at 0.885 and 0.77, with 0.025 of chroma at a
green hue (140). The plants went down by the same 0.145 so they keep their step from the meadow:
the grass 48644D, the dark third blade 3B5640, the grass patch 35513A, 48644D and 5E7C63, the daisy
leaf 5D7C63, the reed stems 48644D; the Mill's lily pad stays 89A88E. The trail stays E6D3B5BB,
0.08 from the meadow's top. The palest petal now stands 0.17 from it and the Mill's cow 0.26, so
neither falls short.

Then (Oj, 21:02) "remove color from vintage noire stems". The world's greens come from the world
dump and were never washed out by the theme's saturation as the trail is, so Vintage Noir's plants
kept a faint green (chroma about 0.02 to 0.03). They are now the grey of the same lightness: the
grass 353535, the dark third blade 2E2E2E, the daisy leaf 404040 and the grass patch 2D2D2D,
373737 and 444444. Its mountains (0.026) and crown (0.012) keep their faint tint, as nothing asked
about them; the props were already grey.

Then (Oj, 21:07) "make trees foliage darker on vintage noire". Its foliage stood near its meadow
(5A5A5A, L 0.47, against hillTop 505050, L 0.43). treeLeavesBase, the shared field the Mill's trees
and the Chase's crowns are drawn from, is 323232, and the Chase's crownLight and crownMiddle 404040
and 373737: each 0.15 darker, and the crowns lose their faint tint. The far hills' trees are these
hazed, so they darken too. The Mill's peeking wolf, 1E1E1E, now stands 0.08 from the foliage it
hides behind, where it stood 0.23 from the old; mill_fix was not run again.

Then (Oj, 21:16) "That can be mitigated by making the hills lighter and the wolf darker - with a
darker wolf it will be pure vintage black and white vibe". hillTop is 9A9A9A and hillBottom
6A6A6A, where they stood at 505050 and 282828; the Mill's wolf is 0A0A0A with ears of 000000, where
it was 1E1E1E and 141414. The wolf now stands 0.17 from the foliage, the trees 0.37 from the hills'
top and the cow 0.27. The hills' top is lighter than the sky's foot (787878), so the ridge reads
against it; 8A8A8A would have stood 0.06 from it. The mill's roof, lifted for the dark hills,
stands 0.09 from the light ones. The same two fields are the Chase's meadow, so its lawn is light
too, and the generator's smallest shift turns its grey wolf dark (3E3E3E) and its hares light
(E1E1E1, E7E7E7) to stand off it; the other couple's black wolf stays darker than ours.

Then (Oj, 21:37) "flowers are invisible on snow - make tham all white. Also white-icy butterfly".
The snow themes are the three with snow falling: Winter Frost, Winter Night and Arctic Dawn. Their
petals are all FFFFFF - Winter Frost's peach, cream and mint stood 0.03 to 0.07 from its snow and
read no better than white - and ONE_PETAL keeps the generator from varying a lone white into a
blush and a lilac. The butterfly's wing is EAF6FF and its edge, body and wing tips 5E8FBF: the edge
is what carries it, as the wing stands 0.03 to 0.05 from Winter Frost's and Arctic Dawn's light
sky. Winter Frost's flowers measure 0.03 from its meadow; the daisy shows against the sky and by
its heart and stem.

Then (Oj, 21:41) "also grey icy path". The three snow themes' trails are a cool grey (chroma 0.02,
hue 245) in WORLD_EDITS, set before the props are measured against them, about 0.10 from the snow
where the trail is drawn: Winter Frost E6C6D2DD, Arctic Dawn E6A1ACB7 and Winter Night E695A0AA,
with pebbles 0.15 darker (97A2AD, 747F89, 68737D). The beige trails they replace stood 0.03 to
0.09 from the snow. The rubber duck and the banana peel moved a little to keep their contrast with
the grey.

Then (Oj, 21:45) "swap the primary hare with the secondary one", and (21:48) "I mean on the winter
theme". Swapping them in every theme was built first - ChaseScene drawing greyHare for our couple,
the icon in daylight's grey coat, the coats reordered couple by couple - and dropped at the second
message. In Winter Frost alone the generator trades the two solved hare coats (SWAP_HARES): our
hare runs grey (A8A39E) and theirs brown (BE936E), each with the contrast it was solved to, as both
are measured against the same backgrounds. The fields keep their names, so in that theme brownHare
holds the grey coat; the alternative, naming the coats by couple, would have changed ChaseTheme
for one theme's sake.

Then (Oj, 21:58) "Make the wind blow the opposite way, and check the dust visibility". The rain
leans down to the left and the blown dust ran to the right; the dust now runs to the left, so the
two agree, its streaks mirrored to trail behind it. Its streaks were stroked 1 pixel wide whatever
the frame's size, so on a 3840-pixel screen they were hairlines; they are a unit wide now, as the
rain's drops are 1.5. Against its sky's middle Sahara Heat's white dust stands 0.24 away, Vintage
Noir's 0.57, Steampunk's rusty orange CC9966 only 0.09 - 0.06 in the Chase, where the weather is
drawn at 0.6. Steampunk's effectColor1 is now FFF2DC, a warm steam white, 0.24 from the middle of
its sky, 0.10 from its pale foot and 0.52 from its rust hills; it is used only for the dust.

Then (Oj, 22:06) "Fade the dust effect on Mill". MillScene draws the theme's dust at half its
strength; the rest of its weather stays whole, and the Chase's weather keeps its 0.6.

Then (Oj, 22:12) "To make Chase more replayable, either start from a new position each time, or
from the previous one". Everything that varies in the Chase - the leap's end, what the tree drops,
the crow's start, what lies on the trail and stands by it - goes by cyclesPassed, and the showcase's
clock starts at cycle 0 each run, so every run opened on the same cycle. The scene now keeps the
cycle after the last one it drew in its view section (NextCycle), as the Earth keeps its orbits,
and adds it to the host's cycles. A random start was the other way; it needs nothing kept, but it
can open on a cycle just seen, where carrying on walks through the whole sequence before any cycle
comes back. A run starts at the beginning of its cycle, not partway into the one it was left in.

Then (Oj, 22:25) "the rainy themes lost lightning?" - only in the Chase, which never had it: the
Mill draws its lightning itself, apart from drawWeather, so the Chase took the rain and not the
storm when it took the themes. Oj (22:27) "You shall": drawBackdrop draws drawLightning over the
sky down to the ground line, after the clouds and before the mountains, so the bolt comes down
behind them and the flash lights the sky alone. It runs on the effects' time, as the weather does,
so a rainy theme strikes six times a minute, a strike flickering for 1.5 seconds; the Mill's strike
three times in its 40 seconds. The bolt is in the theme's sunDisk, as in the Mill: Stormy Night's
pale blue, Toxic Waste's acid green.

**The crow's eyes** (Oj, 2026-11-10: "The crow after bumping to the wolf gets big eyes"). The crow
wore an X in place of its eye for the half second of its tumble and the second it flaps off shaken.
Three readings were shown beside it: both eyes wide for the whole of that, wide with the X drawn
inside them as it flies off, and the X through the tumble with the eyes going wide as it gets away.
Oj took the first. crowEyesWideAt pops the eyes past full size in 0.08 s and settles them by 0.2 s,
each a white disc on a rim of ink with a pinpoint pupil, the far one coming out from behind the head
as they widen; they shrink back to size over the 0.3 s around the end of the shaking. The frames
before the hit, after the shaking and the stump start are unchanged; the X is drawn nowhere.

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

**The theme moved out to ThemeBlend** (Oj, 2026-11-10: "I can easily see how Chase and Mill can
share themes", then "1. shared" and "go"). The pick, the crossfade and animationTick went into
ThemeBlend for ChaseScene to use as well, readoutColors into readoutColorsOf, and the weather block
into drawWeather. The background bitmap is redrawn when wornVersion() moves. Renders of Fresh
Spring, Calm Night, Winter Frost, Golden Hour and Cyberpunk Night came out the same to the bit
before and after.

The crossfade's step reads its target from a member, m_crossFadeTarget, as MillScene's did through
m_nextTheme. AnimationController::start leaves an animation already heading to the same end value
alone, with the step it had, so a step that captured its target would go on toward the first of two
quick picks. A first cut captured it, and the renders differed.

The mill draws its fireflies just before drawWeather, where they came between the snow and the
leaves. No theme has fireflies beside other weather, so only a crossfade between a theme with
fireflies and one with rain or snow draws them in another order.

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

Then (Oj, 2026-11-10, 21:23, on a screenshot of Winter Frost) "Make the left hill lighter and the
winter cow white". The front hill was the back hill with its Hsl luminosity scaled by 0.9. Hsl is
Okhsl, whose saturation is relative to the gamut at that lightness, so scaling the lightness of a
near white keeps a high saturation and the chroma grows as the gamut widens: Winter Frost's F0F5FF
hill came out a light blue, L 0.80 and chroma 0.054 against the back hill's 0.90 and 0.027.
Three rules were rendered in six themes: the luminosity scaled by 0.95, a tenth of the way to
black, and a twentieth of the way to black. Oj took the twentieth (21:30), so frontHillColor is
colorBetween(color, black, 0.05) in drawHills and in the cow's groundBehind alike; Winter Frost's
front hill is now C3D0E1, L 0.85 and chroma 0.028. In the same message, "and the white cow on
golden hour": mill_fix had darkened both cows to stand 0.15 from the sky and hills (Winter Frost's
by 0.28 to A4A4A4, Golden Hour's by 0.235 to B2A9A0), and the generator's theme_edits.py now
restores their own colours after it: FFFFFF with C8C8DC spots, and FFF5EB with 5A3C28 spots.

## StarStyle

The mill drew its own stars, all sparkles, until Oj asked whether it needed stars of its own
(2026-11-07) and picked moving them into the shared field. Sparkles keeps their look: the same
positions from the same random numbers, a size that follows the twinkle, nothing below a fifth of
it.

**Sparkles are filled one by one**, where dots are binned by opacity: they all share one opacity,
so binning would put every star in one path, and two that overlap would cut holes in each other
under the even-odd rule.
