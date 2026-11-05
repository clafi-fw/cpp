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

**A theme states the shadow of each window set.** A root sized by MetricsSet::PrimaryWindow or
MetricsSet::SecondaryWindow casts the theme's shadow for that set, and takes the new one when the
theme's sizes switch. A root given a WindowShadow of its own keeps it.

## MetricsSet

**A set is the sizes of one kind of control, and a theme states every set.** A control given a set
as a property - `MetricsSet::Button` - is sized by the theme's set of that name, and a theme switch
hands it the new sizes.

**The sizes given beside the set are the caller's.** A Padding, a MinSize or a Border stands over
the set's value, and no switch writes it.

**A ControlMetrics given whole is a block of the control's own**, not a set: every size is the
caller's, and a switch leaves the control as it is.

## MetricsField

One size of ControlMetrics, named so that a set of them can say which sizes a control was given
outright. `Depth` is `zDepthFactor` and `GrowIn` is `surfaceGrowInScale`.

## MetricsFields

**Every size a control's properties and setters stated.** A theme switch hands the control the
set's new value for every size not in it - see ControlMetrics copyFields.

## ControlMetrics band

**The width of the band a control draws along one of its edges**, over the frame and round the
corners there - see PaintEvent paintBand. It is apart from the border, which the focus ring
thickens. The edge and when the band shows are the control's, the colour its Band rules' - see
PaintChannel:

- an open tab - the edge away from its page, as the tab opens
- a text box or a slider - the bottom edge, while it is in edit mode
- a callout, a message bar, a code block - the left edge, always

**The built-in theme draws a tab's band and the edit mode's in the accent's values**, read as the
tab is selected and as the control is in edit mode, and a block's in its element's own colour.

**The Tab, Text Box and Slider sets state twice the border's width**, the Callout set 4, and
every other set 0.

## ThemeMetrics drawsBand

The sets whose controls draw a band: Tab, TextBox, Slider, Callout and CodeBlock. The Themes app
offers a Band size on their pages alone.

## ThemeMetrics

**A theme file states its sizes in a Metrics section**, each set under its token in k_metricsSets
and the two window shadows beside them. A file that states none, or leaves a size out, keeps the
built-in value.

**The maximum sizes stay the built-in ones.** Every set leaves its maximum unbounded, apart from
the check mark's, which follows its minimum - see ThemeMetrics settle.

## ThemeMetrics settle

**These sizes follow from others**, and a theme read from a file has them worked out again:

- a primary window's padding is its border's width - the ring its content stands inside
- a check mark's height and a scroll button's are their widths, so both are square
- a check mark's maximum is its minimum, so its size is fixed and a list item can skip measuring it

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

## k_luminosityShareAtHalfElevation

**The elevation axis bends toward white between its ends.** Elevation 0 and 1 land where they would
on a straight axis, and elevation 0.5 lands this share of the way up the luminosity range - from
the floor in dark mode, from black in light mode. The middle of the axis is lighter in both modes.

With `m` this share and `e` the elevation turned toward the mode - 0 at the darker end of the
range - the share of the range an elevation lands on is

    m e / (m e + (1 - m) (1 - e))

`elevationOf` is the same weighting with `m` and `1 - m` swapped, so the two stay exact inverses.

**A step near the darker end grows and one near white shrinks.** An offset moves the luminosity
`m / (1 - m)` times its size at the darker end and `(1 - m) / m` times at white. On a dark page the
first steps above it spread apart; on a light page they draw together.

**Everything a theme states as an elevation lands on the bent axis** - Offsets, Scales and Sets
alike, so an accent set to an elevation is lighter in both modes. Saturations keep a straight axis.

## ThemeColors darkModeFloor

**At the dark end the elevation axis runs from the floor to white.** An elevation's luminosity
becomes `floor + (1 - floor) * share`, the share being the one the bent axis gives - see
k_luminosityShareAtHalfElevation. `elevationOf` stays the inverse, and each step a theme states
shrinks by `1 - floor`.

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

**`movedOnAxis` runs Scale, then Offset, then Set.** `applyToLuminosity` crosses to the elevation
axis once and back around it, and `applyToSaturation` runs it on the saturation itself.

## blend

**Two themes that agree on a rule leave it still** at every factor of the crossing.

- `offset`, `multiplier` and `setPull` cross in a straight line.
- `setTarget` and `hue` cross as a mean weighted by the two pulls. A hue takes the short way around
  the wheel.
- Two rule lists cross rule by rule when both read the same inputs and write the same channels, in
  one order. Lists that differ in any of that take the destination's.

## PaintChannel

**Each channel starts from a colour of its own**, which the rules writing it then move:

- Surface - the surface the control stands on, in its element's seed hue where its set states
  one - see ElementRules
- Stroke and Band - the control's own surface, as its Surface rules left it
- Text - the ink the control inherits, in its surface's hue while no rule names one
- Shadow - the shadow it inherits, in the stroke's hue while no rule names one

**A stroke or a band shows as far as its rules reach.** A control whose rules write no Band draws
none, whatever width its size set states - see ControlMetrics band.

## UiElement

**The handle a control wears its set by.** The framework's elements are the enumerators, and an
application's own come after them - see appElement. A control wearing none takes no rules at
all: no surface, no stroke, no ring. The order is free: what is indexed by an element - the
baked effects, the baked rules - follows it.

## appElement

**An application's own elements stand past the framework's**, one per name the application
states in AppParams, in that order. `appElement(i)` is the handle for the i-th name, held as a
constexpr constant by the application that stated it, and a theme reaches it through a set of that
name. See AppElementNames.

## AppElementNames

The names an application states for its own elements, as AppParams carries them. They are what
the theme is laid out by, and what a theme is asked for before it is offered: one lacking a name
is not listed for that application.

## ElementRules

**One set of rules, named, standing over its base's.** A theme holds one for every element its
controls wear, and as many more as it likes: a base the sets of a kind stand over, or an
application's own.

- `name` - what a control's handle is matched against, and what a file states the set under.
- `base` - the set whose rules this one takes first, by name. Empty for a set standing over none.
  The framework's sets stand over AnyElement, AnyWindow and AnyNonWindow, and over AnyButton,
  AnyIndicator, AnyCallout and DoDont - see BaseElements.
- `standsOn` - what the element is painted on, for the editor's preview. Empty for a window root,
  which stands on the bare colour of the mode.
- `seed` - the pigment the surface starts its hue from, where the hue of what it stands on is not
  wanted: the callouts and the found-text band. A rule naming a hue overrides the seed.
- `rules` - its own, applied over the base's as ThemeRules resolvedRules says.
- `dropped` - the selectors of inherited rules it leaves out: a rule of any set up its chain whose
  inputs, and-inputs and channel match one of them is not applied to this set.

## RuleSelector

What names a rule apart from its effect - its inputs, its and-inputs and its channel. Two rules
with one selector are two steps on the same channel under the same conditions.

## ThemeRules

**The sets a theme states, found by name.** `of` answers a set or nullptr; `add` appends one,
replacing a set of the same name; `chainOf` walks a set's bases up, stopping at a name the theme
lacks or has walked already; `resolvedRules` is what the paint path applies.

## ThemeRules resolvedRules

**A base's rules apply first, and a set's own adjust what they left** - within each channel, in
two groups: the chain's resting rules, root first, then its state rules, root first. A surface is
established before a state modifies it, so a set's own resting rule lands before a base's Hot,
and a set's own Hot lands after a base's. A rule a set further down the chain has dropped is
left out.

Baked, the list stands flat: PaintEvent applies a control's own rules and then the one list its
element resolved to.

## BaseElements

**The sets the framework's own sets stand over**, by name:

- AnyElement - every set stands over it, through one of the next two.
- AnyWindow - what the three window roots take: the frame, the shadow. Anything wearing Dialog,
  Menu or Tooltip takes it wherever it stands, a mockup inside a page included.
- AnyNonWindow - what every other set takes. The focus ring lives here: its rules read Current
  and write the Stroke. So does the edit mode's band: its rule reads Edit and writes the Band.
- AnyButton - what Button and ToolButton share: the Selected, Hot and Pressed surfaces.
- AnyIndicator - what the selection and hover indicators share, their on colour included.
- AnyCallout - the five callouts' surface and band; each callout states its seed alone.
- DoDont - the two sides of a Do and Don't block.

**The ring's weight is the control's current**, whatever colour the rules gave the stroke. That
factor thickens the border toward ThemeMetrics::focusedBorder, in paint only, and holds the frame
out of the surface grow-in. A control drawing an indicator in the ring's place takes that share
off - see AdjustPaintEvent replaceRing.

A line drawn through `strokeRgbOn`, such as a grid cell's or a divider's, reads the list and
nothing of the weight.

The accent's values are written into the ring's rules, so a theme that moves Accent edits them
too.

## BakedColors pigmentHsl

**The hue is the theme's and the tone is the ink's.** The hue comes off the theme's harmony as the
theme is baked. InkWell states the tone once for each side of the theme.

A lightness between the sides is drawn between the two tones, so a yellow that stays yellow on both
sides stays yellow between them.

## BakedElementRules

**An element as the paint path reads it**: its chain resolved into one list, the rules of its own
set on their own, and its seed. The band resolvers raise a selection over a found band by
applying the found band's list and then the selection's own rules, so what the two chains share
is applied once. An element the theme states no set for holds nothing and paints nothing.

## UiElement FoundText

**The band behind what a search has found**, seeded with Yellow - see ElementRules.
ControlPaintContext::foundTextRgb resolves it, raised by the inputs of the control drawing it, as
the selection band is.

**Where a selection covers a match**, ControlPaintContext::selectedFoundRgb raises the selection
over the found band. It applies FoundText's rules, then SelectedText's, then the shared ones once.

- A SelectedText rule naming a hue replaces the seed there. The built-in one names Palette 2.
- The saturation and lift FoundText gave stay under the selection's offsets.
