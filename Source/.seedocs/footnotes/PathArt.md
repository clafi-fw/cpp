# PathArt

The words that do not fit above a declaration.

## Scene

A scene is a picture that moves through a cycle and comes back to where it began. The MillScene
showcase holds a list of them, offers them by name() in the Scene group of its Scene page, and
stores the one picked under that name.

**Every motion in a scene runs a whole number of times a cycle**, so the frame at the end of the
cycle is the frame at its start and the loop shows no seam. A scene that needs a slow motion beside
a fast one gives itself a longer cycle - cycleDuration() - rather than a motion that does not come
round.

**The showcase owns the clock.** It advances its cycle time by the time since the last frame over
cycleDuration(), and wraps it back into 0 to 1. A scene keeps no time of its own.

**A scene draws its figures as sprites and its weather as effects** - free functions that take a
SceneFrame and a struct: a sprite's pose, an effect's extent. Each takes the time its motions run
on, and comes round when that time runs a whole number of units a cycle.

## Scene draw

Paints the whole of bounds, so nothing needs to be drawn under it.

**A frame depends on cycleTime and bounds alone.** The showcase jumps between scenes and times - a
hover preview, a pick, a resize - and every scene but MillScene draws the same picture for the same
two values, whatever was drawn before. MillScene keeps its theme's crossfade between frames.

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

What a scene's frames depend on besides the cycle time: the bounds it is drawn into, the canvas's
transform from the bounds' coordinates to pixels, and the backend that draws them.

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

**A path clip does not reach the kept pixels.** They are laid in as drawBitmap lays a bitmap, which
on the CPU backend stops at the clip's rectangle only.

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

**The cycle is 240 seconds.** The Earth turns twice a cycle and the Sun goes round once, so each
continent passes once by day and once by night. The satellite goes into the Earth's shadow once a
cycle: it darkens and the stars brighten. Its panels turn to face the Sun, so in the shadow they
show their backs. The blinking and twinkling keep the pace they would have in a 40-second cycle.

The city lights come on past the terminator - see SatelliteScene Terminator - and the
atmosphere along the limb is blue by day, orange at dusk and dark at night, by the sunlight above
each point of it. The clouds are soft-edged lumps, some overlapping, each cluster drifting a few
degrees back and forth.

## SatelliteScene Terminator

The line between day and night across the part of the globe in view, taken as straight: over the
small cap the frame shows, the curve of the true terminator does not show.

distanceTo() is above 0 on the day side and below it on the night side, in pixels.

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
