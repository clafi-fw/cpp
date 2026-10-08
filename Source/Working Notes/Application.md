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

THE PLATFORM STANDS FIRST AND FALLS LAST. Application<Platform, GpuBackend> holds the platform
in PlatformHolder, a base ahead of ApplicationBase, rather than as a member behind it: bases
are built in declaration order and destroyed in reverse, so the platform is built before the
context and the themes manager and destroyed after them. Everything the application owns -
its config, its windows, the themes watcher and its thread - is built on a standing platform
and goes down on one, and a UiTimer stopped by a dying member finds the timer manager still
up. Nothing that owns a UiTimer outlives the platform - the animation controller is
AppContext's - so a timer hook finding no manager standing is a broken order and says so.

## configSharePath

THE NESTING IS WHAT KEEPS A SHARED FOLDER OUT OF THE APPLICATIONS' REACH. An application's
own folder is named after the application, one level under the publisher, so a shared folder
at that same level answers to any application called after it. An application named Themes
would write its `Settings.ini` and its open tabs into the themes directory, and clearing Keep
settings on this PC would take every user theme on the machine with it - deleteConfigFolder
removes the folder and everything under it. Under `Share` no application name reaches one.

## AppMenu

THE MENU IS RUN TO ITS CLOSE, AND WHAT IT ANSWERED IS CARRIED OUT AFTER. A command pressed inside
the menu takes note of itself and closes the menu; openAppMenu reads the answer once execute has
returned and acts on it. Exit closes the window the menu stands on, and doing that from inside the
menu's own loop would take the loop down under the click that asked.

## OptionsPage

Every section states `VerticalAlign::Top`, for the reason ThemesList states it: a wrapping panel
deeper in measures one lane and wraps into as many as it needs once the width arrives, and every
group between it and the page has to be free to follow.

## SettingsPage

A PANEL RATHER THAN A PAGE OF ITS OWN, because two things stand here and only one of them
scrolls. The body is a ScrollBox over an OptionsPage - every section on one column - and the
bottom bar is the foot. A column could not have done it: a stack hands its items the heights
they measured and keeps nothing back, only a filling item takes what a lane has over (see
Control::fillsLane), so a box standing in one is never given the room that is left nor cut to
it. PanelBase's slots are the arrangement for a flexible middle between bars that hold still.

THE SLOT IS STATED, NOT FILLED. A lane hands every item the width it measured and only a
FlexSpacer takes what is over - see Control::fillsLane - so the slider carries a width of its own,
stated to leave the row about as wide as the themes above it are capped at. The percent stands in
a box wide enough for the widest of them, so the slot's left edge holds still while the number
under the pointer changes.

## ScaleSlider

A SLOT THAT ANSWERS TO ITS OWN POSITION IS A LOOP. The position is read off where the pointer
stands in the slot - see SliderBase::Thumb::nestedDrag - so a slot free to grow and move with the value
it is naming does not settle under a pointer. A slot half as long makes a pixel of travel worth
twice the percent, and the slot's own left edge travels further than the pointer does, which
reverses the sign. What it does instead of following the pointer is swing between the two ends.

THE HOLD IS TAKEN AT THE PRESS, IN TWO PLACES. A press on the thumb stops propagating before the
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

## connectAppPage

An application's pages join the strip the menu already has rather than a second tab control inside
Settings: one click reaches any of them, the application is named where the eye goes first, and a
page is free to be a list where the Settings page is a column of small answers.

## UiElementDescriptor

Changing `token` changes the file format: an unrecognised key is discarded with a SchemaErrorEvent,
so a theme saved under the old spelling loses those rules rather than failing to load.

## connectAppThemes

WHAT MAKES THE CONFIG'S Theme AND ColorMode NODES MEAN ANYTHING. `AppContext` holds the path and
the mode and states no handler - resolving a path reads the themes directory, which is not the
core's work - so this connects each node's change to `applyStoredTheme` and applies what the
nodes already hold. `ApplicationBase::initialize` calls it BEFORE the config is loaded, so the
theme and the mode a file names are carried in by the load itself rather than applied a second
time after it, and no window exists yet - which is what makes the first window open in that
theme rather than cross to it. See AppContext::takeTheme

THE DESKTOP IS READ HERE TOO. Under Auto the desktop's mode is part of the setting, so this also
connects `Platform::events()`: a SystemColorModeEvent puts the stored theme on again where the
config states Auto, which crosses the lightness the way a click on a mode button does, and
changes nothing where a mode is stated outright.

Once per application, and never taken down: the node outlives every window.

## ThemesManager

The four overrides read and write a theme the way the tab config holds it, as a Dom node.
readDocument seeds the node with the defaults and loads the file over them, because a file states
only what it changes; writeDocument writes what differs from the defaults, statedTheme. isEdited
is that same difference asked of a file: a theme that states nothing is one nobody has touched.
paintIcon looks the theme up by file name, and the built-in's page is named with its own name -
Default.theme - so a crumb, a tab and a tile draw the built-in through the same call as a user's
theme. checkNameShape adds the built-in names to what the base refuses: a user theme under one of
them would never be read.
