# AppTheme

The words that no longer fit above a declaration.

## WindowShadow

THE SHADOW A WINDOW CASTS, in logical pixels, mirroring Graphics::ShadowParams: blur is
how far it reaches past the window, offset which way the light falls, spread how much
wider than the window it starts. Zero opacity or zero blur is no shadow, and a window with
none keeps no margins for one. All four are metrics, and a theme is colours only, so
putting a theme on never changes a window's frame. A window casts the shadow its root states as
a property, and a root stating none casts none.

THE COLOUR IS THE WINDOW ROOT'S SHADOW RULES, taken at this opacity: the Shadow rules of the
element the root wears, then Any Window's, then the shared ones. `BakedColors::windowShadow`
resolves them for the root - Any Window's alone for a root that wears no element - and
`FormBase::shadowDesign` hands the colour to the painter.

The rules are applied to black carrying the hue of the window's stroke, and read at the dark end
whatever the lightness, with no floor. A shadow is the absence of light on either side of the
theme, and a rule read in the window's own direction cannot say that: `Set 0` is black at the
dark end and white at the light one. So elevation here is luminosity, `{}` is plain black, and
one rule gives one shadow in both modes. Raising saturation alone tints it toward the stroke's
hue, and a stated hue toward that hue. `bakeShadow` gives the elevation `k_noFloor`: the floor
lifts the theme's own surfaces, and a shadow falls on whatever stands behind the window.

A theme crossing carries the shadow with the rest of the window. The form restates its frame
on every tick, and the painter bakes its tiles again on each tick the colour has moved. A
switch between the modes moves nothing here.

## ThemeRoots

A THEME IS NAMED BY A PATH, and the path is virtual: it stands for a theme, not for a file. Two
roots, `BuiltIn` and `User`, and a bare name under one of them - `BuiltIn/Default`, `User/Ocean`.
That is what an application's config holds, and what every route that changes the theme states.
The path names the colours and not the mode they are worn in, which the config states beside it -
see ThemeColors.

The root is what decides where the name is looked up: `BuiltIn` is answered from the colours
compiled into the framework whatever stands on the disk, `User` from the themes directory the
publisher's applications share. A name carries no extension - the file's is how the directory
spells it, not how the theme is known - so a theme keeps its path when the extension a file
carries changes.

The core states the spelling and nothing else. Reading a theme file is not the core's work, so
resolving a path is answered above it - see `ThemesManager::themeByPath` and `connectAppThemes`.

`k_defaultThemePath` is `BuiltIn/Default`, the one theme compiled into the framework, which is
what an application wears where nothing else is stated: the config's default, and what a path
naming a theme that is no longer there falls back to.

## ThemeColors

A THEME STATES NO MODE. Once the dark mode floor lifts the dark end, one design reads right at
both, so which end a theme is worn at is no longer the theme's to say: it is the user's, kept in
the application's config beside the theme's path - see `applyColorMode` in Application. Whatever
reads a theme at a mode is handed the mode: `bake` and `ThemeColors::rootSurface`.

THE BUILT-IN THEME IS WRITTEN, NOT EDITED. `ThemeColors()` states it, and its definition stands
alone in `AppTheme_BuiltInColors.cpp`, which the Themes app's Write to source rewrites whole from
the theme it has open. The declarations say what each member is and carry no values, so a theme
change rebuilds that one file rather than everything that imports the module.


## Lightness

WHERE A THEME IS WORN BETWEEN THE TWO POLES, as a float: 0 is Dark, 1 is Light, and anything
between is a crossing between the modes part way through. `lightnessOf(ColorMode)` is the one
place a mode becomes the float, and `colorModeOf` the way back - the pole a lightness stands
nearer to. `elevationOf` and `luminosityOf` take it in place of the mode - a control's own
lightness is `PaintEvent::lightness`, inherited from the control above it unless the control
states one.

Contrast closes as lightness approaches 0.5 and is gone there, every elevation landing on one
luminosity. That is the shape of the model rather than a fault in the mapping: a continuous path
from an elevation that rises with luminosity to one that falls with it has to leave the maps that
are one to one, and 0.5 is where this family does. A crossing between two modes is a wash through
a flat mid-tone, and it is chosen knowing that.

## DarkModeFloor

WHERE THE DARK END OF THE THEME PUTS ELEVATION 0. There the elevation axis runs from the floor to
white rather than from black: the luminosity `luminosityOf` would name is taken to
`floor + (1 - floor) * luminosity`, so every elevation stays reachable, `elevationOf` stays its
inverse, and each step a theme states is scaled by `1 - floor`. The floor is taken by
`1 - lightness`: light mode is left as it is, and a crossing between the modes fades it.

Every elevation a theme states is lifted - the surfaces, the stroke, the effects and the text
rules alike - so a check, a caret, a focus ring and accented text rise with the surfaces around
them. Text stands near the top of the axis, where the lift barely moves it: the ink on a dark page
still reaches white, and only an ink set low, such as the one over a check mark, lands on the
floor rather than on black.

`bake` puts the floor into each elevation channel - `BakedValue::luminosityFloor` - so nothing on
the paint path is told which axis it stands on, and `k_noFloor` is what every saturation states. The
ink ladder runs from the lifted surface to the ink, so a faint grade and the disabled fade land on
the surface they are drawn on.

The floor stops the light side's ink ladder short in turn. `BakedColors::darkModeFloor` carries it
to the paint path, where every grade below Strongest over a light surface ends as far short of the
ink as the dark side's surface stands above black, so the two sides draw in one set of greys - see
UI-Types#inkgrade. Strongest is the ink and still reaches black.

The bare colour a form starts from stays at the pole. Every window root sets its surface's
elevation outright, and a Set lands on the lifted axis from wherever it starts; a root that moved
its elevation instead would move it from black.

A floor lies below 1: at 1 the axis has no room left, and `elevationOf` divides by zero. The
Themes app's slider stops at 0.5.

## BakedValue, BakedEffect, BakedColorRule

WHAT A THEME BECOMES SO THAT TWO OF THEM HAVE A HALF WAY. A Theme names one operation per channel
and the ThemesManager edits it; a baked channel carries every operation at a weight, and the paint
path reads that. The two are separate types because the file format and the editor stay as they
are: a Theme is what a person writes, a baked set is what a window is painted from.

Per channel, in place of the operation and its value:

- `offset` - a net move in elevations, identity 0, offsets accumulating
- `multiplier` - a net scale, identity 1
- `setTarget` with `setPull` - an absolute elevation and how far it is taken, identity 0

A hue is the same pair, `hue` with `setPull`, a pull of nothing being the hue the colour arrived
with. A pigment resolves to its hue as the set is baked, so nothing downstream has a pigment
left to look up.

`applyTo` crosses to the elevation axis once, runs Scale, then Offset, then Set, and crosses back.
Each operation standing at its identity is skipped, so a set at rest - where one operation holds
the whole weight - costs the one crossing a rule always did, and only a set being crossed pays for
more than one.

Blending two sets lerps `offset`, `multiplier` and `setPull`, and takes `setTarget` and
`hue` as a mean weighted by the two pulls, the hue the short way around the wheel. The weighting is
what holds the one invariant the representation exists for: **a set crossed with itself answers
that set at every factor**, so two themes that agree on a rule leave it still. Applying the
outgoing rule at one weight and the incoming rule at the other does not hold it - two `Set 0.5`
taken at 0.7 and 0.3 pull `1 - 0.3 * 0.7` of the way rather than the whole of it.

Two lists of `BakedColorRule` blend rule by rule where both read the same inputs and write the same
channels in one order. Lists that differ in any of that have no half way, and the crossing takes the
destination's.

## Focus ring

THE RING IS THE CONTROL'S OWN STROKE, taken further by the theme's Focus ring list. That list
reaches every control whose interactivity is not None, whatever element it wears, and it is read
after the stroke's own chain - the control's rules, its element's, the window's, Any element's.
Only its stroke rules are read, and only for the control's own stroke: a line a control draws
through `strokeRgbOn`, a grid cell's or a divider's, does not take the ring.

HOW FAR THE LIST REACHES THE STROKE IS HOW MUCH RING THERE IS. That share thickens the border
toward `ThemeMetrics::focusedBorder`, and it holds the control's frame out of the surface
grow-in, since a ring marks the frame and not the surface. The weight is a paint effect, not
layout.

The shadow seeds its hue from the stroke before the ring, so a focused control does not tint the
shadow it casts.

The default list is four rules:

- when focused and keyboard - the strongest ink
- when current - the strongest ink. A container's current item wears the ring under the mouse as
  well, since it marks where a Shift range grows from.
- when hovered, and mouse and current and window focused - the accent
- when focused, and keyboard and window focused - the accent

The accent's values are stated in the rules themselves, so a theme that moves Accent moves the
ring by editing these rules too.

## A pigment as a baked theme holds it

A PIGMENT'S HUE IS THE THEME'S AND ITS TONE IS THE INK'S. The harmony answers while a theme is
baked, so what a baked set carries per pigment is the hue itself - a value two themes have a half
way between - and `ColorHarmonyKind` reaches no further than the bake.

The two tones an ink states are one colour written once for each side of the theme rather than one
elevation the mode would orient, so a theme standing between the sides is drawn between the tones.
That pair has a real colour half way, which the elevation axis does not: the tones are chosen for
each side rather than mirrored, and a yellow that stays yellow on both sides stays yellow between
them.
