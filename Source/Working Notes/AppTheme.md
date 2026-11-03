# AppTheme

The author's side of the footnote of the same name.

## WindowShadow

The form restates its frame on every tick, and the painter bakes its tiles again on each tick the
colour has moved.

## ThemeColors

**The built-in theme is written, not edited.** `ThemeColors()` states it, and its definition stands
alone in `AppTheme_BuiltInColors.cpp`, which the Themes app's Write to source rewrites whole from
the theme it has open. The declarations say what each member is and carry no values, so a theme
change rebuilds that one file rather than everything that imports the module.

## Lightness

Contrast closing at 0.5 is the shape of the model rather than a fault in the mapping: a continuous
path from an elevation that rises with luminosity to one that falls with it has to leave the maps
that are one to one, and 0.5 is where this family does. A crossing between two modes is a wash
through a flat mid-tone, and it is chosen knowing that.

## ThemeColors darkModeFloor

`bake` puts the floor into each elevation channel - `BakedValue::luminosityFloor` - so nothing on
the paint path is told which axis it stands on, and `k_noFloor` is what every saturation states.

## BakedValue

Each operation standing at its identity is skipped, so a set at rest - where one operation holds the
whole weight - costs the one crossing a rule always did, and only a set being crossed pays for more
than one.

A Theme and a baked set are separate types because the file format and the editor stay as they
are: a Theme is what a person writes, a baked set is what a window is painted from.

## blend

The weighted mean of `setTarget` and `hue` is what holds the invariant that a set crossed with
itself answers that set. Applying the outgoing rule at one weight and the incoming rule at the
other does not hold it - two `Set 0.5` taken at 0.7 and 0.3 pull `1 - 0.3 * 0.7` of the way rather
than the whole of it.

## BakedColors pigmentHsl

The two tones an ink states are one colour written once for each side of the theme rather than one
elevation the mode would orient. That pair has a real colour half way, which the elevation axis
does not: the tones are chosen for each side rather than mirrored.

## MetricsSet

**Sizes joined the theme file on 2026-11-03**, for the SeeDocs article blocks: a block's look is a
theme fact, and the looks that differ by size come out of a set's sizes. Three choices were made
with it:

- Sizes are a list of their own beside the colour elements. A control names its sizes and its
  colours apart, as it did, so the places that read a size stayed as they were.
- A switch reaches open windows through the set a control names and the sizes it stated. A
  control copying its set when built would have kept the old sizes until it was built again.
- A band width stands beside the border rather than a border weight per side, so the focus ring
  and everything reading the border stayed as they were.

**The maximum sizes stay out of the file.** Unbounded is the float's largest value, which reads
as a forty-digit number, and the check mark's is the only maximum a set states.

**A size worked out from a set and passed as a property stays as built** - tree rows, the
Settings page's captions and SeeDocs' fields, about 35 places. Each one follows a switch only by
stating its size again in themeMetricsChanged.
