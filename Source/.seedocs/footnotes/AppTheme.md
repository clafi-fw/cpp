# AppTheme

The words that do not fit above a declaration.

## WindowShadow

The fields follow Graphics::ShadowParams:

- `blur` - how far the shadow reaches past the window
- `offset` - which way the light falls
- `spread` - how much wider than the window the shadow starts
- `opacity` - how strong the shadow's colour is

**Zero opacity or zero blur is no shadow**, and the window keeps no margins for one.

A window casts the shadow its root states in `FormControlBase::windowShadow`, in the colour
BakedColors::windowShadow answers.

**A theme leaves a window's frame alone.** These fields are metrics, and a theme states colours
only.

## BakedColors windowShadow

The colour comes from the root's Shadow rules, in this order:

- the rules of the element the root wears, if it wears one
- Any Window's
- the shared ones, if it wears an element

A window's only state is its focus, so a rule reading only other inputs never applies.

**The rules start from black in the hue of the window's stroke.** A rule raising saturation tints
the shadow toward that hue, and a rule naming a hue tints it toward its own.

**A shadow is read at the dark end, whatever the lightness.** Elevation here is luminosity: `{}` is
plain black, and one rule gives one shadow in both modes.

**A shadow has no floor.** The floor lifts the theme's own surfaces, and a shadow falls on what is
behind the window.

## themePathOf

**A theme is known by a virtual path** - a root and a bare name, such as `BuiltIn/Default` or
`User/Ocean`. The config holds it, and every route that changes the theme states one.

- `BuiltIn` names a theme compiled into the framework, whatever is on the disk.
- `User` names a file in the themes directory the publisher's applications share, by its stem. A
  change of extension keeps the path.

The path names the colours only, and the config states the mode beside it. The application layer
resolves a path - see ThemesManager::themeByPath and connectAppThemes.

## k_defaultThemePath

The config's default, and the theme an application wears when its stored path names a missing one.

## ThemeColors

**The mode belongs to the user, not to the theme.** The dark mode floor lets one design read right
at both ends. The config keeps the mode beside the theme's path - see applyColorMode.

`bake` and `rootSurface` are handed the mode they read a theme at.

## Lightness

`lightnessOf` and `colorModeOf` convert to and from a ColorMode. `elevationOf` and `luminosityOf`
take a lightness in place of a mode.

A control's lightness is PaintEvent::lightness, inherited from the control above. A control that
knows better states its own through AdjustPaintEvent::setLightness.

**Contrast closes as lightness nears 0.5.** At 0.5 every elevation lands on one luminosity, so a
crossing between the modes washes through a flat mid-tone.

## ThemeColors darkModeFloor

**At the dark end the elevation axis runs from the floor to white.** An elevation's luminosity
becomes `floor + (1 - floor) * luminosity`. `elevationOf` stays the inverse, and each step a theme
states shrinks by `1 - floor`.

**The floor is taken by `1 - lightness`.** Light mode keeps its axis, and a crossing between the
modes fades the floor.

**Every elevation a theme states is lifted**, text included. A check, a caret, a focus ring and
accented text rise with the surfaces around them.

- Ink on a dark page still reaches white.
- An ink set low, such as the one over a check mark, lands on the floor.
- A faint grade and the disabled fade land on the lifted surface they are drawn on.
- Over a light surface the floor stops the ink ladder short - see lightGradeOf.

**A window root sets its surface's elevation outright**, and a Set lands on the lifted axis. The
bare colour a form starts from is at the pole, so an Offset there would start from black.

**The floor lies below 1.** At 1 `elevationOf` divides by zero. The Themes app's slider
stops at 0.5.

## BakedValue

**What a theme becomes so that two themes have a half way.** A theme's rule states one operation
per channel. A baked channel holds all three at once:

- `offset` - a net move in elevations, 0 for none
- `multiplier` - a net scale, 1 for none
- `setTarget` and `setPull` - an absolute elevation and how far a value is pulled to it, a pull of
  0 for none

BakedHue is the same pair for a hue: `hue` with `setPull`. A pigment resolves to its hue as the
theme is baked.

**`applyTo` runs Scale, then Offset, then Set**, crossing to the elevation axis once and back.

## blend

**Two themes that agree on a rule leave it still** at every factor of the crossing.

- `offset`, `multiplier` and `setPull` cross in a straight line.
- `setTarget` and `hue` cross as a mean weighted by the two pulls. A hue takes the short way around
  the wheel.
- Two rule lists cross rule by rule when both read the same inputs and write the same channels, in
  one order. Lists that differ in any of that take the destination's.

## ThemeRules focusRing

**The ring is the control's own stroke, taken further by these rules.** They apply after the
stroke's own rules, on every control whose interactivity is other than None, whatever element it
wears. Only their Stroke rules count.

**How far the rules reach is how much ring there is.** That share thickens the border toward
ThemeMetrics::focusedBorder, in paint only. It also holds the frame out of the surface grow-in.

A line drawn through `strokeRgbOn`, such as a grid cell's or a divider's, takes no ring. The shadow
takes its hue from the stroke before the ring.

The default rules:

- focused and keyboard - the strongest ink
- current - the strongest ink, under the mouse too, since a current item marks where a Shift range
  grows from
- hovered, and current and mouse - the accent
- focused, and keyboard and window focused - the accent

The accent's values are written into these rules, so a theme that moves Accent edits them too.

## BakedColors pigmentHsl

**The hue is the theme's and the tone is the ink's.** The hue comes off the theme's harmony as the
theme is baked. InkWell states the tone once for each side of the theme.

A lightness between the sides is drawn between the two tones, so a yellow that stays yellow on both
sides stays yellow between them.

## seedPigmentOf

FoundText starts from Yellow. Every other element answers none and keeps the hue of the surface
under it. A rule naming a hue overrides the seed. The Themes app seeds its preview the same way.

## UiElement FoundText

**The band behind what a search has found**, seeded with Yellow - see seedPigmentOf.
ControlPaintContext::foundTextRgb resolves it, raised by the inputs of the control drawing it, as
the selection band is.

**Where a selection covers a match**, ControlPaintContext::selectedFoundRgb raises the selection
over the found band. It applies FoundText's rules, then SelectedText's, then the shared ones once.

- A SelectedText rule naming a hue replaces the seed there. The built-in one names Palette 2.
- The saturation and lift FoundText gave stay under the selection's offsets.
