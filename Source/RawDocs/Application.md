# Application

The words that no longer fit above a declaration.

## watchDirectory

A watch cannot be set over a directory that does not exist, and on a fresh installation the
themes directory does not: the first write makes it - New theme through needDirectory, or a
Save as through the file format's own create_directories. Until the watch stands, the list
is what was read while the directory was missing, and no change is ever reported. So every
request for the list first asks whether the directory exists unwatched, puts the watch over
it, and reads the list again. That is one existence check per request while the directory
is missing, and none once it is watched.

## Application

THE GPU BACKEND IS THE APPLICATION'S TO NAME, AND NAMING NONE IS AN ANSWER. The CPU backend is
the core's own and every application has it, so `Application<Win32Platform>` draws on the CPU and
offers the user nothing, while `Application<Win32Platform, Direct2DBackend>` carries both and
starts on whichever the config holds - see AppContext::gpuAcceleration. The type travels no
further than this constructor: what reaches the context is `gpuBackendFactory<GpuBackend>()`, a
plain function pointer, null where no backend was named.

THE PLATFORM STANDS FIRST AND FALLS LAST. Application<Platform, GpuBackend> holds the platform
in PlatformHolder, a base ahead of ApplicationBase, rather than as a member behind it: bases
are built in declaration order and destroyed in reverse, so the platform is built before the
context and the themes manager and destroyed after them. Everything the application owns -
its config, its windows, the themes watcher and its thread - is built on a standing platform
and goes down on one, and a UiTimer stopped by a dying member finds the timer manager still
up. Nothing that owns a UiTimer outlives the platform - the animation controller is
AppContext's - so a timer hook finding no manager standing is a broken order and says so.

## ApplicationBase::finalize

WHAT THE USER ALLOWED TO BE KEPT IS KEPT, whether or not the application saves for itself.
Keep settings on this PC is the framework's offer, so honouring it is the framework's work:
finalize calls `saveConfig(false)` before the diagnostic window goes, and an application that
never saves for itself still writes its theme, its window placement and the diagnostic
window's state once the folder is there. `false` is the whole of the permission - nothing is
written where no folder was allowed - so an application saving here is not an application
deciding to store.

An application that wants its config on disk EARLIER still saves it itself; the second write
costs a file and changes nothing. `saveConfig(true)` is a different thing entirely - it makes
the folder, which is the permission - and belongs only where an application means to store
without being allowed to.

AN APPLICATION MUST NOT WRITE OVER A LOADED VALUE to state its own default. The config is
loaded before an application's own code runs, so `config() / L"Theme"` already holds
what the user last chose; an application whose default differs from the schema's states it
behind `configFolderExists()`, which is false exactly when nothing was loaded.

## configSharePath

WHAT THE PUBLISHER'S APPLICATIONS SHARE STANDS UNDER `Share`, one folder beside their own.
The themes directory is `<publisher>/Share/Themes`, and that is where every application of
the publisher reads its user themes from. The path ends with the separator, as
`configPublisherPath()` does, so a name is appended to it without one.

THE NESTING IS WHAT KEEPS A SHARED FOLDER OUT OF THE APPLICATIONS' REACH. An application's
own folder is named after the application, one level under the publisher, so a shared folder
at that same level answers to any application called after it. An application named Themes
would write its `Settings.ini` and its open tabs into the themes directory, and clearing Keep
settings on this PC would take every user theme on the machine with it - deleteConfigFolder
removes the folder and everything under it. Under `Share` no application name reaches one.

## AppMenu

The application menu, under the AppButton of every application: a popup of the menu role holding
a TabbedBox, its pages down the left column and its commands under them. Information and Settings
stand at the foot of the strip, Information above, and an application's own pages follow them -
see connectAppPage. Show diagnostic stands only where Diagnostic::Options::showForm is on; Exit
names the application, spelled as its name is spelled, and closes the form the menu stands on,
through its root form.

THE MENU KEEPS THE WINDOW IT WAS PLACED IN. It is `AutoFit::No`: a window placed again around
its pages would move under the pointer, a section opened or closed on the Settings page taking
the edge the pointer stands on somewhere else. Its floor is what holds the pages instead - the
height is the Settings page with every section open - so the placement never cuts the menu below
that, and a section opened again never meets the window's edge.

THE PAGE IT WAS LEFT ON IS THE PAGE IT NEXT OPENS, kept in the config under AppMenuPage BY
CAPTION: a page an application adds moves every index after it, and a config read by hand should
show a name. A caption the strip no longer has - an application that has stopped stating a page -
opens Settings, which is also where a first run opens. The answer is written once the menu has
closed, while the menu is still standing, since only the menu can be asked which tab is open.

THE MENU IS RUN TO ITS CLOSE, AND WHAT IT ANSWERED IS CARRIED OUT AFTER. A command pressed inside
the menu takes note of itself and closes the menu; openAppMenu reads the answer once execute has
returned and acts on it. Exit closes the window the menu stands on, and doing that from inside the
menu's own loop would take the loop down under the click that asked.

## connectAppMenu

Connects appMenuAction's click to openAppMenu, once per application - ApplicationBase::initialize
calls it. Until it has run the action has no handler, and a button presenting it reads as
disabled.

## OptionsPage

A page of option sections, and a StackPanel itself - THE PAGE IS THE STACK, a Panel with slots
having come up empty here. It is the shape every page of the backstage shares: the Settings page
below holds one, and so does each page an application asks for.

A SECTION IS A GROUP THAT CAN BE PUT AWAY. addSection puts one down the page - an expander whose
header is the caption and whose body is the column the caller fills, so the two are one control
and collapsing the caption takes the options with it. addGroup is that column alone, which is all
a caller filling a section in needs; addSection is for a page that shows or hides the whole of
one, since a section draws itself and one whose options are all hidden is a card with nothing in
it.

The look is `ExpanderViewMode::Section`, which is what tells a page's own groups from the groups
a control INSIDE one carries - ThemesList holds two of its own and they wear the divider look, so
the two levels read apart rather than stacking four rules of the same weight down the page.

Every section states `VerticalAlign::Top`, for the reason ThemesList states it: a wrapping panel
deeper in measures one lane and wraps into as many as it needs once the width arrives, and every
group between it and the page has to be free to follow.

## SettingsPage

The Settings page of the application menu: the options every application has, on a box that
scrolls, then a line and the storage answer at its foot.

A PANEL RATHER THAN A PAGE OF ITS OWN, because two things stand here and only one of them
scrolls. The body is a ScrollBox over an OptionsPage - every section on one column - and the
bottom bar is the foot. A column could not have done it: a stack hands its items the heights
they measured and keeps nothing back, only a filling item takes what a lane has over (see
Control::fillsLane), so a box standing in one is never given the room that is left nor cut to
it. PanelBase's slots are the arrangement for a flexible middle between bars that hold still.

THE BAR ALWAYS STANDS - `ScrollBars::Vertical`. Its strip is part of the page's width whether or
not there is anything to scroll, so a section opening or closing moves nothing across the page.
It has travel only where the options outgrow the box: past the box's stated maximum, which is how
tall the page grows, or on a screen too short for the menu's floor.

The common options are Appearance, which is the theme and the size the application is drawn at, a
row each - Theme, a ThemePick over the themes the application can wear and a button for each
colour mode setting, Auto first; and Scale, the percent and the slot that moves it. The two
captions share a stated width, so what each row names starts at one left edge whichever word is
the longer, and the slot carries no stepping buttons - a percent is stepped by an arrow key, and
an end button on a slot that resizes the form it sits in walks out from under the pointer holding
it. Then Always on top, which addresses the window the menu stands on rather than the menu; and
Keep settings on this PC. THE SECTION GOES, NOT THE CHECK, where
the platform has no keep-above a client may ask for - see IPlatformWindow::canSetAlwaysOnTop. A
check that could be turned on and would not hold says something untrue about the window, and hiding
only the check would leave a Main Window card holding nothing.

ENABLE GPU ACCELERATION IS WRITTEN, NOT ACTED ON, the way the colour mode is: the check writes
AppContext::gpuAcceleration, and the node's change is what states the backend and tells every
window. Its section goes whole where the application named no GPU backend, for the reason Always
on top's does - there is nothing to turn on.

THE POINTER TRIES A THEME ON, THE CURRENT TILE CHOOSES IT. The list is built
`PreviewMode::Hover`, so a tile under the pointer is worn through
`AppContext::takeThemeColors` and nothing is written; a click or an arrow key moves the current
item, and that is what states the path in the config. So looking costs nothing and choosing is
a separate gesture - and what the menu closing gives back is the stored theme, applied by
openAppMenu once the menu has gone.

KEEP SETTINGS ON THIS PC IS THE CONFIG FOLDER AND NOTHING ELSE. It reads
AppContext::configFolderExists rather than a value of its own, since a value of its own
would have to be stored somewhere the user has not yet allowed. Ticking it asks first,
naming the folder about to be made, and writes the config as soon as the folder is there -
an empty directory keeps nothing. Clearing it asks again, naming the folder about to go,
and takes it away with everything in it - see AppContext::deleteConfigFolder.

THE FOLDER IS NAMED IN TWO INKS. The platform's application data root is the system's part and is
drawn muted; the publisher's folder and the application's under it are drawn in spot ink. A part
is a link only while the folder it names stands: the question before the folder is made links the
system's part, the question before it goes links the whole. A click opens the folder in the
system's file manager - see MessageDialog. The hint over the check says what ticking it is for,
led by the application's name, and once it is ticked, where the settings are kept.

THE SLIDER WRITES THE CONFIG AND EVERY WINDOW TAKES IT. A step writes AppContext::scale, and the
node's own change is what reaches the windows - see ScaleSwitchEvent. The menu is one of them,
except while the pointer is moving the thumb: the slider holds the menu's size for the length of
that gesture and lets it follow again at the release - see ScaleSlider. One unit of the slot is
one percent, so an arrow key and a click on an end button are each a step of one, and the readout
states the percent that was taken rather than the one the thumb names.

THE SLOT IS STATED, NOT FILLED. A lane hands every item the width it measured and only a
FlexSpacer takes what is over - see Control::fillsLane - so the slider carries a width of its own,
stated to leave the row about as wide as the themes above it are capped at. The percent stands in
a box wide enough for the widest of them, so the slot's left edge holds still while the number
under the pointer changes.

THE MODE IS STATED, NOT TRIED ON. A mode button writes the config the way a pick writes the path,
and the node's change carries it to the theme - see `applyColorMode`. Auto hands the mode to the
desktop - see `wornColorMode`.

The page is built afresh every time the menu opens, so every option is read from where it
lives at the moment it is shown and there is no state here to keep in step with anything.

## ScaleSlider

The scale slider on the Settings page, and a Slider in every other respect. What it adds is a
hold: the form it stands in is drawn at the size this slider names, so for as long as the pointer
owns the slot that form is put on a scale of its own - FormBase::holdScale at the press,
FormBase::followScale at the release.

A SLOT THAT ANSWERS TO ITS OWN POSITION IS A LOOP. The position is read off where the pointer
stands in the slot - see SliderBase::Thumb::drag - so a slot free to grow and move with the value
it is naming does not settle under a pointer. A slot half as long makes a pixel of travel worth
twice the percent, and the slot's own left edge travels further than the pointer does, which
reverses the sign. What it does instead of following the pointer is swing between the two ends.

THE HOLD IS TAKEN AT THE PRESS, IN TWO PLACES. A press on the thumb stops propagating before the
slot sees it - see SliderBase::Thumb::pressDown - so thumbPressDown is the hook that covers that
one, and pressDown covers a press on the slot. Both are taken before the base runs, since the
base sends the position to the point under the pointer and raises the change that writes the
config. The release needs one place: an up walks the whole chain, so a thumb, a slot and an end
button all reach pressUp here.

THE END BUTTONS AND THE ARROW KEYS ARE NOT HELD, and want no hold. Each names a step rather than
a place, so nothing is read back off the slot and there is nothing to swing - the form grows a
percent at a time under the gesture asking for it. A press on an end button stops propagating as
well, so it never reaches the hold; its release does reach followScale, which has nothing to let
go of and does nothing.

## InformationPage

The Information page of the application menu: the application's name, the publisher under it,
what the application does, the framework the application is built with, and the framework's
address as a link.

WHAT THE APPLICATION DOES IS ITS OWN SENTENCE, stated in AppParams::description and kept by
AppContext::description. It is a Text like the name, so it may carry inks and styles, and may
spell the name inside it the way the name is spelled. An application that states none gets the
page without it: the publisher is followed by the framework line, with one blank line between.

THE PAGE STATES HOW LONG ITS LINES RUN. The backstage is as wide as its widest page - see
PageSizing::WidestPage - and a text with no maximum is measured one line per paragraph, so an
unbounded description would widen the menu to fit it on one line. A maximum width breaks it into
lines there instead. The page is Left-aligned beside that maximum, because a maximum on a Fill
axis is a caller error - see Control::align.

THE PAGE IS A READ-ONLY TEXT BOX, the way a message dialog's text is, because a text box is what
follows a link: the address underlines under the pointer and opens on a click. The rest comes
with it - the page takes the focus, shows a caret, and its text can be selected and copied.

THE NAME IS A TEXT, AND ITS SPELLING IS SHOWN WHEREVER THE NAME IS. The page, the title bar an
application builds from ApplicationBase::name, and the questions the Settings page asks all draw
the same inks and styles. The plain text is the identity: the config folder, the platform's
application id and the native window title are made from it, so a spelling that only changes ink
changes none of them.

## AppPageBuilder

What an application puts on a page of its own, run against that page as the menu is built. It is
handed an OptionsPage and calls addGroup for each caption it wants, filling the column that comes
back:

    connectAppPage(L"Scene", [](OptionsPage& page){
        StackPanel& picked = page.addGroup(L"Picked Window");
        picked.add<Checkbox>(L"Highlight corners");
    });

## connectAppPage

Adds a page of the application's own beside Settings, by caption, once per application - and more
than once for more than one page, in the order asked. An application's pages join the strip the
menu already has rather than a second tab control inside Settings: one click reaches any of them,
the application is named where the eye goes first, and a page is free to be a list where the
Settings page is a column of small answers.

Nothing calls it for an application, and one that never does gets Settings alone.

## UiElementStateDescriptor

THE THREE SPELLINGS A RULE GOES BY, and they are not the same word. `name` is the label an
editor writes, `codeName` is the designator generated C++ has to compile, and `token` is
what stands in a theme file. Changing `token` changes the file format: an unrecognised key
is discarded with a SchemaErrorEvent, so a theme saved under the old spelling loses that
rule rather than failing to load.

## UiElementStates

The states one element paints with, as a set. Anything listing an element's rules walks
UiElementState from first to Count and takes the bits that are set, so the order is the
enum's and the membership is the element's.

## UiElementState

In the order they are applied. Surface, active, hovered and pressed build the element's
fill, then text and activeText build the ink over it - the order PaintEvent takes. Stroke
follows surface, being the border of the element at rest. Shadow comes last and PaintEvent
never reads it: the form casts it around a window root, and only the three roots list it -
see AppTheme's WindowShadow. Anything walking a set of states walks this order, so no
caller states an order of its own.

## UiElement

IN THEMECOLORS DECLARATION ORDER, which generated C++ depends on: a designated initializer
list has to name members in the order they are declared. Nothing else here needs an order,
so this is the one it takes. An editor is free to present them in any order it likes by
naming elements one at a time.

## UiElementDescriptor

What one element is: its three spellings, the ThemeColors member it names, what it is
painted on, and the rules it paints with.

## appThemes

THE ONE MANAGER AN APPLICATION MAKES, over the themes directory the publisher's applications
share - `ApplicationBase` holds it and `connectAppThemes` names it here, so a page reaches the
themes without being handed them. It answers before any window exists and for as long as the
application runs; asking for it before `connectAppThemes` has run is a precondition failure, not
an empty list.

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

## storedTheme

A PATH NAMING NOTHING IS NOT AN APPLICATION WITHOUT A THEME. A user theme can be renamed or
deleted between two sessions and the config still holds the path it had, so the built-in Default
stands in its place. The node is left as it is - the theme may be back the next time the
application runs, and rewriting the path would lose the user's choice to a file that was only
moved.

## applyTheme

Puts the application in a theme without stating it in the config - the route a preview takes,
and the one `applyStoredTheme` takes with what the config states. Without a mode the theme is
worn in the one `wornColorMode` answers; with one it is worn in that one, which is how the Themes application shows
a theme in the mode it previews without writing the user's choice.

## applyStoredTheme

Puts the application in the theme the config names, which crosses to it. Called wherever
something was worn that the config does not state: the menu closing over a previewed theme, and
the Themes application turning its preview off.

## applyThemePath

States the path in the config, which is the whole of choosing a theme: the node's change carries
it to `applyStoredTheme`, and the value is kept between sessions where the user allowed storing.
A path equal to the one already stored writes nothing and changes nothing - `Dom::Value::set`
short-circuits - so a preview is not something this can give back; that is what applyStoredTheme
is for.

## applyColorMode

States the colour mode setting in the config, the way `applyThemePath` states a path: the node's
change carries it to `applyStoredTheme`, and the value is kept between sessions where the user
allowed storing. The mode is the application's and not the theme's, so every theme is worn in it -
see AppTheme's ThemeColors. Auto hands the choice to the desktop - see `wornColorMode`.

## wornColorMode

THE MODE THE APPLICATION IS DRAWN IN, which a ColorModeSetting is not: Dark and Light answer for
themselves, and Auto answers with the desktop's mode - see Platform::systemColorMode in Context.
Asked each time a theme is put on rather than kept, so it cannot fall behind the desktop.

## ThemesList

THE THEMES AN APPLICATION CAN WEAR, in two groups: the built-in theme and the themes in the
directory. A StackView, each group an expander whose body holds the tiles, so collapsing a header
takes its tiles with it.

THE VIEW ALONE, WITH NO BOX AROUND IT. What scrolls the themes belongs to whatever holds them: a
page carries these sections on the box its own column stands on, and a window that gives the list
the room puts it in one of its own - `createBody<ScrollBoxWith<ThemesList>>`. A box here would be
a second bar inside the first, kept short by a stated height that cuts the tiles off. `view()`
answers the list itself, for a host saying which of the two it means.

The list listens to the manager and builds itself afresh whenever the directory reports. THE
PATH IS WHAT IT COMES BACK TO, not the tile: every tile is taken down by a rebuild, and a theme
is the same theme under the same path. `selectPathAfterRebuild` is how a caller names a theme
whose tile does not exist yet - one just written to the disk, or the one that will stand where a
selection has been taken away.

What the list does NOT do is act on the themes. Renaming a file, deleting one, opening one: a
tile raises it and the host answers, through `ThemeEditHandler` and `ThemeMenuHandler` and
through the click and double-click events that reach the list from its tiles. So the Settings
page states one prop and gets a picker, and the Themes application states four and gets its home
page, out of one list.

FOUR TILES TO A LANE, IN A WINDOW OF ANY WIDTH, SPREAD ACROSS IT. `k_tilesPerLane` is stated as a
`LaneSize` on both groups, and a stated lane breaks the row in the align pass as well as in the
measure - so a wider host never gets a fifth tile in the lane, and the rows laid out are the rows
reported. `ItemSizing::Equal` is what keeps the surplus from piling up at the right: the row is
cut into four shares and each tile stands in the middle of its own, at the size it always had -
`ThemeTile` states `HorizontalAlign::Center`, and the share is a box rather than a size. The
built-in theme takes the first share of four, so Default stands in the column the first user
tile does. See Item-Containers

`ItemsIconSize` and `ItemsViewMode` are the tile's picture and how it stands against the name -
stated for the list, since every tile in one list shares them. A tile whose name stands UNDER
its picture is fixed to the width of the picture and the check mark beside it, so the pictures
line up down the lane and a long name wraps instead of widening its row; one whose picture
stands beside its name is as wide as it measures.

`SelectionMode::Multi` is what makes the list more than a picker - a selection is asked for
where a command acts on several themes at once. Only a user tile can be held selected: a
selection here is a command over files, and a built-in answers no such command. That is also
what decides the check mark - a tile that can never join a selection carries none.

## ThemeTile

A theme in a list: the palette inset in the theme's own surface, its name under it. THE CAPTION
IS THE NAME THE THEME GOES BY, so an editor over it is a rename - `WithInPlaceEdit` carries the
gestures and the geometry, and the host carries what the name is kept in. Only a user theme
offers one: a built-in is answered from the compiled-in colours whatever stands on the disk, so
there is no file behind its caption.

The theme a tile holds is the manager's own, by reference. That is load bearing: what an
application is wearing is an address, so a tile's theme is what a preview names, and it stands
still for as long as the list does. See AppContext::takeThemeColors

## ThemesRebuiltEvent

Raised once the tiles have been built and the list stands where it was asked to. What a host
owes a rebuild goes here: a new theme's editor, and a preview of whatever the list now stands
on - the rebuild took the previewed tile with the rest, and no gesture is going to ask for
another.

