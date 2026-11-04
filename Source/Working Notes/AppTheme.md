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

## Blocks

**The article blocks came in on 2026-11-03**: Callout, DoDont and CodeBlock - a themed block
holding a CodeBox that shows the code (Oj, 2026-11-04: "a child CodeBox on a themed block to show
its content"; a CodeBox wearing the block's element and set was the first build, and a code()
accessor the second, before "should be WithBody::body()"). The rule they keep is that a block's
shape is a control property - a SeeDocs project picks it - and its colours and sizes are the
theme's. A theme made for an export states the frames and bands; the built-in one gives tints
alone.

- The colours start from pigments through seedPigmentOf, which PaintEvent applies to any control
  wearing a seeded element. A rule could not name a pigment: a hue operation names a palette slot
  or a value.
- The marks - the message badges, the check and the cross - are drawn in pigment inks rather than
  the element's stroke, the way MessageBar's badge is.
- **The band is the edge indicator the framework already drew** (Oj, 2026-11-04, showing an open
  vertical tab, an open tab, a text box and a slider in edit mode: "these are examples of a band
  use"). Each control draws it at its own edge, in its own colour, when its state says; the
  theme states the width per set, and TextBox and Slider gained sets for it. Drawing it for
  every control on the left in defaultPaintFrame was the first build, and could not hold once
  a tab's set stated one.
- **The band's colour is the theme's** (Oj, 2026-11-04: "Band is new item beside surface and
  stroke, and Edit is a new input - virtual in control, not in StateFactors. Tab already has all
  inputs needed"). Band is a PaintChannel seeded like the stroke, and Edit a RuleInput read from
  Control::editFactor, which WithEditMode answers with its animated factor. The tab's and the
  edit mode's bands were drawn in indicatorRgb - the accent over SelectedText's resting surface -
  and the blocks' in their stroke. The built-in rules keep those colours: the Tab element reads
  Selected and Any Non-Window reads Edit, both in the accent's values, and the callouts' and the
  code block's stroke rules became their band rules. The tab's band ran from half opacity to full
  as the tab opened; its opacity is now the rule's level, from none to full, and its length still
  follows the open state.
- **MessageBar is a callout** (Oj, 2026-11-04, showing a notice card: "That is what Themes call a
  Callout"). It wears the Callout set and the element of its icon, one for each of the five
  icons, and its stripe became the set's band. The built-in callouts are that card: a surface
  near the tooltip's, tinted by the kind's pigment, and a 4 point band in it. A surface rule can
  not keep the tooltip's own hue - the band's colour starts from the surface, so the kind's hue
  has to be there.
- Figure waits on a question about SVG: every figure in the manuals is SVG, and the framework
  decodes PNG alone.

## ElementRules

**The cascade came in on 2026-11-04**, after a round of decisions (Oj: "1 b; 2 shared base; 3;
4 a; 5 b", then "1 a; 2 a; 3 nothing; 4 separate list", then "1 a; 2 b; 3 a; 4 b; 5 a"):

- **Chains are additive.** A rule is a pipeline step, not a property, so a child's rules adjust
  what its base's left rather than replacing them; the same selector in a child and its base is
  two steps. Related sets got a shared base holding what they state alike - Any Button, Any
  Indicator, Any Callout, Do and Don't - rather than one deriving from the other, since a child
  cannot take a rule away by restating it. Dropping an inherited rule is its own list, so the
  editor can move a greyed inherited row there and restore it.
- **Resting rules first, then state rules, each base to element.** Pure base-first order would
  run Any Button's Hot before Button's resting surface, and Pressed is a Scale, so the two orders
  differ. The chosen order reproduces every colour the four keyed lists produced, except where an
  element's own state rule meets a base's on one channel - a tab's or a tool button's stroke now
  adjusts the ring instead of being wiped by it.
- **Any Window is a base, not a position.** A root wearing nothing takes no rules and paints
  nothing, by design: other window kinds are coming (full-screen overlays with no surface of
  their own), and the gap is visible at once. Any Non-Window is no default either, which is why
  TextBox wears a Text Box set and why the element-less interactive controls were listed for a
  decision.
- **The list is open.** A theme states as many sets as it likes; an application names its own
  in AppParams and ships their built-in sets as a theme file's text, until the built-in themes
  are refactored. The Themes app's Testee and Bestee are the first application elements.
- **A theme file carries every set whole**, because the Dom answers a list whole. A user theme
  no longer takes a list it omits from the built-in, and a migration script wrote every set into
  the existing themes.
- **The two indicators' Selected values were a bug**; the shared Any Indicator took the accent's,
  which Hover Indicator had.
- The generator writes the sets in one `rules.elements = { ... }` statement for the built-in
  file, and `rules.add(...)` / `rules.remove(...)` statements for a theme pasted into an
  application. Write to source leaves the application's own sets out.

**The Themes app edits a set's chain on its page** (the second step, the same day, from mockups
Oj picked: "1 b; 2 a; 3 a; 4 b"). A page shows the set's own rows first, then one group per base
of the chain, the nearest first, each headed by the base's name and how many of its rules reach
the set; Delete on a base's row puts the rule's selector into the set's dropped list, and the row
stays in its group struck through, with a Restore link. A rule a set between dropped is struck
too, named, with no link. The three facts stand as lists under the title; a base is offered only
where it would not close a circle, and so is a set to stand on, since the resting walks along
standsOn have no guard. The sets no row of the framework's shows stand under one Own sets
category, listed again from the theme after every change: a set the theme names is a row and a
page there, Add set on the category's page makes one over Any Non-Window and opens its row for
the name, and Delete set is the last tool on such a page.

The inherited rows are text rows written through the writers the editors use (Themes_App.RuleText,
which the editors' faces read as well), struck by the row's own paintCell over the measured lines.
A rule restored, a set added, renamed or deleted and the rows a move left out of order are
settled on a tick: the click or the in-place editor that asked stands on rows that go.
