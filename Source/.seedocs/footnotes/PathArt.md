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

## SceneFrame

The shorter side of the frame is 600 units, and `unit` is the pixels in one. width and height
are in pixels.

**within() puts a local transform ahead of the frame's own**: what is drawn through the frame it
returns is placed by `local` first. Stroke widths, glow spreads and gradient points given through
it are scaled with it, so a figure drawn about its own origin through `within(Translation *
Rotation * Scale)` comes out whole.

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

The head gathers speed as it falls, and the dust tail grows with the speed. Sparks shed along the
way drift down and fade over 0.3 of a pass.

## SatelliteScene

A satellite in orbit over a large Earth, through day, dusk and night.

**The cycle is 240 seconds.** The Earth turns twice a cycle and the Sun goes round once, so each
continent passes once by day and once by night. The satellite goes into the Earth's shadow once a
cycle: it darkens and the stars brighten. Its panels turn to face the Sun, so in the shadow they
show their backs. The blinking and twinkling keep the pace they would have in a 40-second cycle.

The city lights come on past the terminator - see SatelliteScene Terminator - and the
atmosphere along the limb is blue by day, orange at dusk and dark at night, by the sunlight above
each point of it.

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
