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
