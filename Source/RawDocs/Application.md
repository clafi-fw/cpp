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
loaded before an application's own code runs, so `config() / L"Appearance" / L"Theme"` already holds
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
a TabbedBox, its pages and commands down the left column. The strip reads top down: an
application's own pages first - see connectAppPage - then the commands, and at the foot the
framework's two pages, Information above Settings. Show diagnostic stands only where
Diagnostic::Options::showForm is on; Exit names the application, spelled as its name is
spelled, and closes the form the menu stands on, through its root form.

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

A page of option sections, and a Stack itself - THE PAGE IS THE STACK, a Panel with slots
having come up empty here. It is the shape every page of the backstage shares: the Settings page
below holds one, and so does each page an application asks for.

A SECTION IS A GROUP THAT CAN BE PUT AWAY. addSection puts one down the page - an expander whose
header is the caption and whose body is the column the caller fills, so the two are one control
and collapsing the caption takes the options with it. addGroup is that column alone, which is all
a caller filling a section in needs; addSection is for a page that shows or hides the whole of
one, since a section draws itself and one whose options are all hidden is a card with nothing in
it.

The look is `ExpanderViewMode::Divider` with the caption in the heading style: a section reads
as a labeled line with its chevron at the end, and draws no surface of its own under the options,
so a page of sections is headings and lines down one column rather than a stack of cards.

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

The common options are Appearance, which is the theme, the size the application is drawn at and
how far its controls move in depth, a row each - Theme, a ThemePick over the themes the
application can wear and a button for each colour mode setting, Auto first; Scale, the percent and
the slot that moves it; and Z-Hover, the Z animation amount as a percent and its slot, laid out the
way Scale is, where CLAFI_TEXT_MOVING is 1. The captions share a stated width, so what each row
names starts at one left edge whichever word is the longer, and the slots carry no stepping
buttons - a percent is stepped by an arrow key, and an end button on a slot that resizes the form
it sits in walks out from under the pointer holding it. Then Always on top, which addresses the
window the menu stands on rather than the menu; and
Keep settings on this PC. THE SECTION GOES, NOT THE CHECK, where
the platform has no keep-above a client may ask for - see IPlatformWindow::canSetAlwaysOnTop. A
check that could be turned on and would not hold says something untrue about the window, and hiding
only the check would leave a Main Window heading over nothing.

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

THE Z-HOVER SLIDER WRITES THE CONFIG THE SAME WAY. A step writes AppContext::zAnimation and every
window repaints - see ZAnimationSwitchEvent. One unit of the slot is a hundredth of the amount, so
the readout is the amount as a percent, and 100% is the depth the theme states. At
CLAFI_TEXT_MOVING 0 there is no row, and every control takes that depth - see
AppContext::zAnimation.

THE MODE IS STATED, NOT TRIED ON. A mode button writes the config the way a pick writes the path,
and the node's change carries it to the theme - see `applyColorMode`. Auto hands the mode to the
desktop - see `wornColorMode`.

The page is built afresh every time the menu opens, so every option is read from where it
lives at the moment it is shown and there is no state here to keep in step with anything.

## ThemePick

The themes an application can wear, on one line - the built-in first, then the user's own, each
item drawn as a picture of its theme beside the name. A lane of the list takes ten themes before
it turns a second, and the themes are spread evenly over the lanes that many needs.

LOOKING AT AN ITEM WEARS IT, PICKING ONE STATES IT IN THE CONFIG. A theme under the pointer is
tried on and nothing is written for it. A list closing on the theme it opened on puts the stated
theme back on; closing on another states that one.

The themes folder changing builds the items again, back on the path the picker stood on, and the
application wears the stated theme as the list now holds it.

## ScaleSlider

The scale slider on the Settings page, and a Slider in every other respect. What it adds is a
hold: the form it stands in is drawn at the size this slider names, so for as long as the pointer
owns the slot that form is put on a scale of its own - FormBase::holdScale at the press,
FormBase::followScale at the release.

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

THE END BUTTONS AND THE ARROW KEYS ARE NOT HELD, and want no hold. Each names a step rather than
a place, so nothing is read back off the slot and there is nothing to swing - the form grows a
percent at a time under the gesture asking for it. A press on an end button stops propagating as
well, so it never reaches the hold; its release does reach followScale, which has nothing to let
go of and does nothing.

## InformationPage

The Information page of the application menu. From the top: the application's name with its
version at the far end of the same line; the update row - what the update check found, and a
button asking it - with a line under it; then the publisher, the application's own site as a
link, what the application does, the framework the application is built with, and the
framework's address as a link.

THE VERSION ENDS THE NAME'S LINE. Both are one paragraph, the version after a FlexSpace, so they
share a baseline and the version stands at the line's far end, where the update button stands
in the row under it. The FlexSpace keeps a gap between them on a line with nothing to spare.

THE APPLICATION'S OWN SITE STANDS UNDER THE PUBLISHER. AppParams::site states the address and
AppContext::site keeps it; the page links it and shows it without its scheme, the way the
framework's address is written. An application that states none gets no line for it.

WHAT THE APPLICATION DOES IS ITS OWN SENTENCE, stated in AppParams::description and kept by
AppContext::description. It is a Text like the name, so it may carry inks and styles, and may
spell the name inside it the way the name is spelled. An application that states none gets the
page without it: the lines under the name are followed by the framework line, with one blank
line between.

WHAT ELSE THE APPLICATION HAS TO SAY FOLLOWS AS PARAGRAPHS. AppContext::addInformation takes a
plain paragraph and AppContext::information answers them in the order added; the page prints them
after the description and before the framework line, one blank line between each. They are for
facts the application learns at startup rather than states in code - where a data file it read
came from, say - so they are strings, not Texts, and the page does the one dressing they need:
every address from https:// to the next blank is made a link, the punctuation closing the
sentence around it left in the text.

THE PAGE STATES HOW LONG ITS LINES RUN. The backstage is as wide as its widest page - see
PageSizing::WidestPage - and a text with no maximum is measured one line per paragraph, so an
unbounded description would widen the menu to fit it on one line. A maximum width breaks it into
lines there instead. The text under the line is Left-aligned beside that maximum, because a
maximum on a Fill axis is a caller error - see Control::align. The name's line and the update row
are one line each and state no maximum: they Fill, so they span the page whatever its width.

EVERY TEXT ON THE PAGE IS A READ-ONLY TEXT BOX, the way a message dialog's text is, because a
text box is what follows a link: the address underlines under the pointer and opens on a click.
The rest comes with it - the text takes the focus, shows a caret, and can be selected and copied.
The page itself is a vertical Stack holding the name's line, the update row, a Divider and
the text. The stack carries the page's padding, so the name starts where a page of options
starts.

THE UPDATE ROW STANDS WHEREVER AppContext::updateCheck IS AVAILABLE - the application states a
major.minor.patch version and AppParams::updates names a repository. Where it is not, the page
has neither the row nor the Divider under it. The row is a horizontal Stack: the status on
the left, a FlexSpacer, and the button at the end, both centred down the row.

THE STATUS SAYS WHAT THE CHECK FOUND, and the button stays whatever it found:

- Unchecked - no status at all, only the button. This is the state of an application that has
  never had an answer.
- Checking - "Checking for updates…", with the button greyed until the answer arrives.
- Latest - "You're up to date".
- Newer - "Version 0.2.0 is out".
- Failed - "Could not check for updates", and the button reads Try again.

Under the status, in the SubBody style, "last check:" and how long ago the last answer arrived -
"just now", then minutes, hours and days. A newer release names its page there instead, as a
link reading Release page on GitHub: the address in full runs under the button at the page's
usual width. A Failed or Checking status keeps the age of the last answer that did arrive; one
that never did has no line under it.

THE AGE IS WORKED OUT WHEN THE PAGE IS BUILT AND WHEN THE CHECK MOVES. The page is built each
time the menu opens, so the age is current on every opening; it does not tick while the menu
stays open.

THE CHECK OUTLIVES THE PAGE, AND THE ANSWER OUTLIVES THE RUN. The check is AppContext's, and the
page is built again each time the menu opens, so a check still running when the menu closes is
answered all the same and the next page reads what it found. The last answer is kept in the
config, so a page opened before anything is asked in a run shows the answer an earlier run got,
with its age - see UpdateCheck::keepIn. The page holds its connection to the check's event in a
ScopedEventConnection, which drops with the page.

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
        Stack& picked = page.addGroup(L"Picked Window");
        picked.add<CheckBox>(L"Highlight corners");
    });

## connectAppPage

Adds a page of the application's own at the top of the strip, by caption, once per
application - and more than once for more than one page, in the order asked. An application's
pages join the strip the menu already has rather than a second tab control inside Settings: one
click reaches any of them, the application is named where the eye goes first, and a page is free
to be a list where the Settings page is a column of small answers.

Nothing calls it for an application, and one that never does gets Settings alone.

## UiElement

The order is free. What is indexed by an element - `k_uiElements`, a theme's element lists,
the baked effects - follows the enum, and an editor presents elements in any order it likes by
naming them one at a time.

## UiElementDescriptor

What one element is: its three spellings, what it is painted on, whether it opens a window,
and - for the elements an ink names - the ThemeColors member holding that effect.

THE THREE SPELLINGS ARE NOT THE SAME WORD. `name` is the label an editor writes, `codeName` is
the member generated C++ assigns an effect to, and `token` is what stands in a theme file - the
key an element's list of rules stands under, and its effect's where it has one. Changing `token`
changes the file format: an unrecognised key is discarded with a SchemaErrorEvent, so a theme
saved under the old spelling loses those rules rather than failing to load.

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

## ThemesManager

THE APPLICATION'S OWN DOCUMENTS FOLDER, of theme files, built in ApplicationBase and answered by
appThemes: the Themes application browses it, the app menu picks from it, and the Settings page
lists it. A DocumentsFolder - see Documents - so the directory, the watch, the listeners and the
sorted read are the base's; what the manager adds is a parsed theme per file, rebuilt in
filesRead each time the base has read the directory afresh, and the built-in, answered from the
compiled-in colours whatever stands on the disk.

The four overrides read and write a theme the way the tab config holds it, as a Dom node.
readDocument seeds the node with the defaults and loads the file over them, because a file states
only what it changes; writeDocument writes what differs from the defaults, statedTheme. isEdited
is that same difference asked of a file: a theme that states nothing is one nobody has touched.
paintIcon looks the theme up by file name, and the built-in's page is named with its own name -
Default.theme - so a crumb, a tab and a tile draw the built-in through the same call as a user's
theme. checkNameShape adds the built-in names to what the base refuses: a user theme under one of
them would never be read.

## ThemesList

THE THEMES AN APPLICATION CAN WEAR, in two groups: the built-in theme and the themes in the
directory. A StackView, each group an expander whose body holds the tiles, so collapsing a header
takes its tiles with it. It answers IDocumentTiles - see Documents - so the layer's home page
works over it as it works over a flat list, and the Themes application's home page is that page
with a preview.

THE VIEW ALONE, WITH NO BOX AROUND IT. What scrolls the themes belongs to whatever holds them: a
page carries these sections on the box its own column stands on, and a window that gives the list
the room puts it in one of its own - `createBody<ScrollBoxWith<ThemesList>>`. A box here would be
a second bar inside the first, kept short by a stated height that cuts the tiles off. `view()`
answers the list itself, for a host saying which of the two it means.

The list listens to the manager and builds itself afresh whenever the directory reports. THE
FILE NAME IS WHAT IT COMES BACK TO, not the tile: every tile is taken down by a rebuild, and a
theme is the same theme under the same name. The built-in goes by the name its page goes by,
Default.theme, so a tab coming up from it lands on its tile. `selectFileNameAfterRebuild` is how a
caller names a theme whose tile does not exist yet - one just written to the disk, or the one that
will stand where a selection has been taken away - and it opens the user group as well, because a
tile made into a closed group is never laid out.

What the list does NOT do is act on the themes. Renaming a file, deleting one, opening one: a
tile raises it and the host answers, through `DocumentEditHandler` and `DocumentMenuHandler` and
through the click and double-click events that reach the list from its tiles.

`ItemSizing::Equal` is what keeps a lane's surplus from piling up at the right: the row is cut into
equal shares and each tile stands in the middle of its own, at the size it always had - a
DocumentTile takes the share of the lane it is handed, and the share is a box rather than a size.
The built-in theme takes the first share, so Default stands in the column the first user tile
does. See Item-Containers

`SelectionMode::Multi` is what makes the list more than a picker - a selection is asked for
where a command acts on several themes at once. Only a user tile can be held selected: a
selection here is a command over files, and a built-in answers no such command. That is also
what decides the check mark - a tile that can never join a selection carries none.

## ThemeTile

A DocumentTile - the mark over the file's name - with the theme beside the file: the palette inset
in the theme's own surface is painted from it, and only a user theme offers the caption's editor,
since a built-in has no file behind its name. The theme a tile holds is the manager's own, by
reference. That is load bearing: what an application is wearing is an address, so a tile's theme
is what a preview names, and it stands still for as long as the list does.
See AppContext::takeThemeColors
