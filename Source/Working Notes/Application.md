# Application

The author's side of the footnote of the same name.

## watchDirectory

A watch cannot be set over a directory that does not exist, and on a fresh installation the
themes directory does not: the first write makes it - New theme through needDirectory, or a
Save as through the file format's own create_directories. Until the watch stands, the list
is what was read while the directory was missing, and no change is ever reported. So every
request for the list first asks whether the directory exists unwatched, puts the watch over
it, and reads the list again. That is one existence check per request while the directory
is missing, and none once it is watched.

## Application

The type travels no further than this constructor: what reaches the context is
`gpuBackendFactory<GpuBackend>()`, a plain function pointer, null where no backend was named.

**The platform stands first and falls last.** Application<Platform, GpuBackend> holds the platform
in PlatformHolder, a base ahead of ApplicationBase, rather than as a member behind it: bases
are built in declaration order and destroyed in reverse, so the platform is built before the
context and the themes manager and destroyed after them. Everything the application owns -
its config, its windows, the themes watcher and its thread - is built on a standing platform
and goes down on one, and a UiTimer stopped by a dying member finds the timer manager still
up. Nothing that owns a UiTimer outlives the platform - the animation controller is
AppContext's - so a timer hook finding no manager standing is a broken order and says so.

## configSharePath

**The nesting is what keeps a shared folder out of the applications' reach.** An application's
own folder is named after the application, one level under the publisher, so a shared folder
at that same level answers to any application called after it. An application named Themes
would write its `Settings.ini` and its open tabs into the themes directory, and clearing Keep
settings on this PC would take every user theme on the machine with it - deleteConfigFolder
removes the folder and everything under it. Under `Share` no application name reaches one.

## AppMenu

**A command runs in the click that chose it**, inside the menu's loop. The menu's Form is a local of
openAppMenu, so it outlives the click whatever the handler closes. Exit closes the menu before it
asks the root form to close, so no popup stands on a window that has gone.

**The menu keeps the window it was placed in** - AutoFit::No, the form's default. A window placed
again around its pages would move under the pointer, a section opened or closed on the Settings
page taking the edge the pointer stands on somewhere else. The floor - k_height, the Settings page
with every section open - holds the pages instead, so the placement never cuts the menu below that.

## OptionsPage

Every section states `VerticalAlign::Top`, for the reason ThemesList states it: a wrapping panel
deeper in measures one lane and wraps into as many as it needs once the width arrives, and every
group between it and the page has to be free to follow.

## SettingsPage

**A panel rather than a page of its own**, because two things stand here and only one of them
scrolls. The body is a ScrollBox over an OptionsPage - every section on one column - and the
bottom bar is the foot. A column could not have done it: a stack hands its items the heights
they measured and keeps nothing back, only a filling item takes what a lane has over (see
Control::fillsLane), so a box standing in one is never given the room that is left nor cut to
it. PanelBase's slots are the arrangement for a flexible middle between bars that hold still.

**The slot is stated, not filled.** A lane hands every item the width it measured and only a
FlexSpacer takes what is over - see Control::fillsLane - so the slider carries a width of its own,
stated to leave the row about as wide as the themes above it are capped at. The percent stands in
a box wide enough for the widest of them, so the slot's left edge holds still while the number
under the pointer changes.

**The slots carry no stepping buttons.** A percent is stepped by an arrow key, and an end button on
a slot that resizes the form it sits in walks out from under the pointer holding it.

## ScaleSlider

**A slot that answers to its own position is a loop.** The position is read off where the pointer
stands in the slot - see SliderBase::Thumb::nestedDrag - so a slot free to grow and move with the value
it is naming does not settle under a pointer. A slot half as long makes a pixel of travel worth
twice the percent, and the slot's own left edge travels further than the pointer does, which
reverses the sign. What it does instead of following the pointer is swing between the two ends.

**The hold is taken at the press, in two places.** A press on the thumb stops propagating before the
slot sees it - see SliderBase::Thumb::nestedPressDown - so thumbPressDown is the hook that covers that
one, and nestedPressDown covers a press on the slot. Both are taken before the base runs, since the
base sends the position to the point under the pointer and raises the change that writes the
config. The release needs one place: an up walks the whole chain, so a thumb, a slot and an end
button all reach nestedPressUp here.

A press on an end button stops propagating as well, so it never reaches the hold; its release does
reach followScale, which has nothing to let go of and does nothing.

## InformationPage

The page holds its connection to the check's event in a ScopedEventConnection, which drops with the
page.

**The page states how long its lines run.** The backstage is as wide as its widest page - see
PageSizing::WidestPage - and a text with no maximum is measured one line per paragraph, so an
unbounded description would widen the menu to fit it on one line. The text under the line is
Left-aligned beside that maximum, because a maximum on a Fill axis is a caller error - see
Control::align. The title and update rows Fill, so they span the page whatever its width. The name
and version share a paragraph and a baseline. The check button and automatic check box have the
same minimum width, which keeps their column in place when the button reads Try again.

## ThemesList

**The view alone, with no box around it.** A box here would be a second bar inside the page's
first, kept short by a stated height that cuts the tiles off.

`ItemSizing::Equal` is what keeps a lane's surplus from piling up at the right: the row is cut into
equal shares and each tile stands in the middle of its own, at the size it always had - a
DocumentTile takes the share of the lane it is handed, and the share is a box rather than a size.
## ThemeTile

ThemesList owns the parsed themes and clears its tiles before releasing them. The selected
storage reads the files, including defaults for fields omitted from legacy files.

## connectAppPage

An application's pages join the strip the menu already has rather than a second tab control inside
Settings: one click reaches any of them, the application is named where the eye goes first, and a
page is free to be a list where the Settings page is a column of small answers.

## UiElementDescriptor

Changing `token` changes the file format: an unrecognized key is discarded with a SchemaErrorEvent,
so a theme saved under the old spelling loses those rules rather than failing to load.

## ThemesManager

WindowApplicationBase builds the manager after AppContext and destroys it first. The manager owns
the connections to the theme path, color mode, and desktop mode. Their destruction disconnects
them while both the config and the platform event source still stand.

WindowApplicationBase::initialize applies the initial theme before loading config. The path and
mode changes during the load use the same handlers as changes from Settings. The first window
opens with the resolved theme in place. AppContext takes the baked colors and metrics and runs
transitions once windows are listening.
