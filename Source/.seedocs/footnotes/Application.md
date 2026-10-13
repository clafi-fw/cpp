# Application

The words that no longer fit above a declaration.

## Application

**The GPU backend is the application's to name, and naming none is an answer. The CPU** backend is
the core's own and every application has it, so `Application<Win32Platform>` draws on the CPU and
offers the user nothing, while `Application<Win32Platform, Direct2DBackend>` carries both and starts
on whichever the config holds - see AppContext::gpuAcceleration.

## ApplicationBase::finalize

**What the user allowed to be kept is kept**, whether or not the application saves for itself.
Keep settings on this PC is the framework's offer, so honouring it is the framework's work:
finalize calls `saveConfig(false)` before the diagnostic window goes, and an application that
never saves for itself still writes its theme, its window placement and the diagnostic
window's state once the folder is there. `false` is the whole of the permission - nothing is
written where no folder was allowed - so an application saving here is not an application
deciding to store.

An application that wants its config on disk **earlier** still saves it itself; the second write
costs a file and changes nothing. `saveConfig(true)` is a different thing entirely - it makes
the folder, which is the permission - and belongs only where an application means to store
without being allowed to.

**An application must not write over a loaded value** to state its own default. The config is
loaded before an application's own code runs, so `config() / L"Interface" / L"Theme"` already holds
what the user last chose; an application whose default differs from the schema's states it
behind `configFolderExists()`, which is false exactly when nothing was loaded.

## configSharePath

**What the publisher's applications share stands under** `Share`, one folder beside their own.
The themes directory is `<publisher>/Share/Themes`, and that is where every application of
the publisher reads its user themes from. The path ends with the separator, as
`configPublisherPath()` does, so a name is appended to it without one.

## AppMenu

The application menu, under the AppButton of every application: a popup of the menu role holding
a TabbedBox, its pages and commands down the left column. The strip reads top down: an
application's own pages first - see connectAppPage - then the commands, the application's own
ahead of the framework's - see connectAppCommand - and at the foot the framework's two pages,
Information above Settings. Show diagnostic stands only in a build with
Diagnostic::Options::enabled. Exit names the application, spelled as its name is spelled; it
closes the menu and asks the application's window to close, as the window's close button does, so
a page holding unsaved work can refuse.

**The menu keeps the window it was placed in.** It is `AutoFit::No`: a window placed again around
its pages would move under the pointer, a section opened or closed on the Settings page taking
the edge the pointer stands on somewhere else. Its floor is what holds the pages instead - the
height is the Settings page with every section open - so the placement never cuts the menu below
that, and a section opened again never meets the window's edge.

**The page it was left on is the page it next opens**, kept in the config under AppMenuPage **by
caption**: a page an application adds moves every index after it, and a config read by hand should
show a name. A caption the strip no longer has - an application that has stopped stating a page -
opens Settings, which is also where a first run opens. The answer is written once the menu has
closed, while the menu is still standing, since only the menu can be asked which tab is open.

## connectAppMenu

Connects appMenuAction's click to openAppMenu, once per application - ApplicationBase::initialize
calls it. Until it has run the action has no handler, and a button presenting it reads as
disabled.

## OptionsPage

A page of option sections, and a Stack itself - **the page is the stack**, a Panel with slots
having come up empty here. It is the shape every page of the backstage shares: the Settings page
below holds one, and so does each page an application asks for.

**A section is a group that can be put away.** addSection puts one down the page - an expander whose
header is the caption and whose body is the column the caller fills, so the two are one control
and collapsing the caption takes the options with it. addGroup is that column alone, which is all
a caller filling a section in needs; addSection is for a page that shows or hides the whole of
one, since a section draws itself and one whose options are all hidden is a card with nothing in
it.

The look is `ExpanderViewMode::Divider` with the caption in the heading style: a section reads
as a labeled line with its chevron at the end, and draws no surface of its own under the options,
so a page of sections is headings and lines down one column rather than a stack of cards.

## SettingsPage

The Settings page of the application menu: the options every application has, and under them
the groups the application adds - see connectAppSettings - on a box that scrolls, then a line and
the storage answer at its foot.

**The bar always stands** - `ScrollBars::Vertical`. Its strip is part of the page's width whether or
not there is anything to scroll, so a section opening or closing moves nothing across the page.
It has travel only where the options outgrow the box: past the box's stated maximum, which is how
tall the page grows, or on a screen too short for the menu's floor.

The common options are Interface and Performance / Display. Interface is the theme, the size the
application is drawn at and how far its controls move in depth, a row each, and then the window's
place among others - Theme, a ThemePick over the themes the application can wear and a button for
each colour mode setting, Auto first; Scale, the percent and the slot that moves it; Z-Hover, the
Z animation amount as a percent and its slot, laid out the way Scale is, where CLAFI_TEXT_MOVING
is 1; and Always on top, which addresses the window the menu stands on rather than the menu. The
captions share a stated width, so what each row names starts at one left edge whichever word is
the longer, and the slots carry no stepping buttons - a percent is stepped by an arrow key, and an
end button on a slot that resizes the form it sits in walks out from under the pointer holding it.
Always on top is hidden where the platform has no keep-above a client may ask for - see
IPlatformWindow::canSetAlwaysOnTop - since a check that could be turned on and would not hold says
something untrue about the window.

**GPU acceleration is written, not acted on**, the way the colour mode is: the check writes
AppContext::gpuAcceleration, and the node's change is what states the backend and tells every
window. It is all Performance / Display holds, so the section goes whole where the application
named no GPU backend - there is nothing to turn on, and the section is what draws the heading.

**The pointer tries a theme on, the current tile chooses it.** The list is built
`PreviewMode::Hover`, so a tile under the pointer is worn through
`AppContext::takeThemeColors` and nothing is written; a click or an arrow key moves the current
item, and that is what states the path in the config. So looking costs nothing and choosing is
a separate gesture - and what the menu closing gives back is the stored theme, applied by
openAppMenu once the menu has gone.

**Keep settings on this PC is the config folder and nothing else.** It reads
AppContext::configFolderExists rather than a value of its own, since a value of its own
would have to be stored somewhere the user has not yet allowed. Ticking it asks first,
naming the folder about to be made, and writes the config as soon as the folder is there -
an empty directory keeps nothing. Clearing it asks again, naming the folder about to go,
and takes it away with everything in it - see AppContext::deleteConfigFolder.

**The folder is named in two inks.** The platform's application data root is the system's part and
is drawn muted; the publisher's folder and the application's under it are drawn in spot ink. A part
is a link only while the folder it names stands: the question before the folder is made links the
system's part, the question before it goes links the whole. A click opens the folder in the
system's file manager - see MessageDialog. The hint over the check says what ticking it is for,
led by the application's name, and once it is ticked, where the settings are kept.

**The slider writes the config and every window takes it.** A step writes AppContext::scale, and the
node's own change is what reaches the windows - see ScaleSwitchEvent. The menu is one of them,
except while the pointer is moving the thumb: the slider holds the menu's size for the length of
that gesture and lets it follow again at the release - see ScaleSlider. One unit of the slot is
one percent, so an arrow key and a click on an end button are each a step of one, and the readout
states the percent that was taken rather than the one the thumb names.

**The Z-Hover slider writes the config the same way.** A step writes AppContext::zAnimation and
every window repaints - see ZAnimationSwitchEvent. One unit of the slot is a hundredth of the
amount, so the readout is the amount as a percent, and 100% is the depth the theme states. At
CLAFI_TEXT_MOVING 0 there is no row, and every control takes that depth - see
AppContext::zAnimation.

**The mode is stated, not tried on.** A mode button writes the config the way a pick writes the
path, and the node's change carries it to the theme - see `applyColorMode`. Auto hands the mode to
the desktop - see `wornColorMode`.

The page is built afresh every time the menu opens, so every option is read from where it
lives at the moment it is shown and there is no state here to keep in step with anything.

## ThemePick

The themes an application can wear, on one line - the built-in first, then the user's own, each
item drawn as a picture of its theme beside the name. A lane of the list takes ten themes before
it turns a second, and the themes are spread evenly over the lanes that many needs.

**Looking at an item wears it, picking one states it in the config.** A theme under the pointer is
tried on and nothing is written for it. A list closing on the theme it opened on puts the stated
theme back on; closing on another states that one.

The themes folder changing builds the items again, back on the path the picker stood on, and the
application wears the stated theme as the list now holds it.

## ScaleSlider

The scale slider on the Settings page, and a Slider in every other respect. What it adds is a
hold: the form it stands in is drawn at the size this slider names, so for as long as the pointer
owns the slot that form is put on a scale of its own - FormBase::holdScale at the press,
FormBase::followScale at the release.

**The end buttons and the arrow keys are not held**, and want no hold. Each names a step rather than
a place, so nothing is read back off the slot and there is nothing to swing - the form grows a
percent at a time under the gesture asking for it.

## InformationPage

The Information page of the application menu. From the top: the application's name with its
version at the far end of the same line; the update row - what the update check found, and a
button asking it - with a line under it; then the publisher, the application's own site as a
link, what the application does, the framework the application is built with, and the
framework's address as a link.

**The version ends the name's line.** Both are one paragraph, the version after a FlexSpace, so they
share a baseline and the version stands at the line's far end, where the update button stands
in the row under it. The FlexSpace keeps a gap between them on a line with nothing to spare.

**The application's own site stands under the publisher.** AppParams::site states the address and
AppContext::site keeps it; the page links it and shows it without its scheme, the way the
framework's address is written. An application that states none gets no line for it.

**What the application does is its own sentence**, stated in AppParams::description and kept by
AppContext::description. It is a Text like the name, so it may carry inks and styles, and may
spell the name inside it the way the name is spelled. An application that states none gets the
page without it: the lines under the name are followed by the framework line, with one blank
line between.

**What else the application has to say follows as paragraphs.** AppContext::addInformation takes a
plain paragraph and AppContext::information answers them in the order added; the page prints them
after the description and before the framework line, one blank line between each. They are for
facts the application learns at startup rather than states in code - where a data file it read
came from, say - so they are strings, not Texts, and the page does the one dressing they need:
every address from https:// to the next blank is made a link, the punctuation closing the
sentence around it left in the text.

**The page states how long its lines run.** The backstage is as wide as its widest page - see
PageSizing::WidestPage - and a text with no maximum is measured one line per paragraph, so an
unbounded description would widen the menu to fit it on one line. A maximum width breaks it into
lines there instead. The text under the line is Left-aligned beside that maximum, because a
maximum on a Fill axis is a caller error - see Control::align. The name's line and the update row
are one line each and state no maximum: they Fill, so they span the page whatever its width.

**Every text on the page is a read-only text box**, the way a message dialog's text is, because a
text box is what follows a link: the address underlines under the pointer and opens on a click.
The rest comes with it - the text takes the focus, shows a caret, and can be selected and copied.
The page itself is a vertical Stack holding the name's line, the update row, a Divider and
the text. The stack carries the page's padding, so the name starts where a page of options
starts.

**The update row stands wherever AppContext::updateCheck is available** - the application states a
major.minor.patch version and AppParams::updates names a repository. Where it is not, the page
has neither the row nor the Divider under it. The row is a horizontal Stack: the status on
the left, a FlexSpacer, and the button at the end, both centred down the row.

**The status says what the check found**, and the button stays whatever it found:

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

**The age is worked out when the page is built and when the check moves.** The page is built each
time the menu opens, so the age is current on every opening; it does not tick while the menu
stays open.

**The check outlives the page, and the answer outlives the run.** The check is AppContext's, and the
page is built again each time the menu opens, so a check still running when the menu closes is
answered all the same and the next page reads what it found. The last answer is kept in the config,
so a page opened before anything is asked in a run shows the answer an earlier run got, with its age -
see UpdateCheck::keepIn.

**The name is a text, and its spelling is shown wherever the name is.** The page, the title bar an
application builds from ApplicationBase::name, and the questions the Settings page asks all draw
the same inks and styles. The plain text is the identity: the config folder, the platform's
application id and the native window title are made from it, so a spelling that only changes ink
changes none of them.

## AppPageBuilder

What an application puts on a page of the backstage - a page of its own, or the Settings page -
run against that page as the menu is built. It is handed an OptionsPage and calls addGroup for each
caption it wants, filling the column that comes back:

    connectAppPage(L"Scene", Icons::DotMark::paint, [](OptionsPage& page){
        Stack& picked = page.addGroup(L"Picked Window");
        picked.add<CheckBox>(L"Highlight corners");
    });

## connectAppPage

Adds a page of the application's own at the top of the strip, by caption and icon, once per
application - and more than once for more than one page, in the order asked. The icon is drawn at
the size the framework's own pages wear theirs; `Icons::DotMark::paint` holds the slot for a page
with no picture of its own, so its caption still lines up with the others.

Nothing calls it for an application, and one that never does gets Settings alone.

## connectAppSettings

Adds groups of the application's own to the Settings page, once per application - and more than
once, in the order asked. **The groups stand under the common options**, above the line and the
storage answer at the page's foot, so the common options keep one place in every application. The
builder is handed the page's column, as a page's builder is, and is run each time the menu opens -
so it reads every option from where it lives, as the common ones do.

    connectAppSettings([](OptionsPage& page){
        Stack& editors = page.addGroup(L"External editors");
        editors.add<TextBox>(PlaceHolderText{ L"A command line" });
    });

Nothing calls it for an application, and one that never does gets the common options alone.

## AppCommandHandler

What a command of the application's own does, handed the click that chose it. The menu stays up
unless the handler closes it with `event.closeForm()`. A command that opens a window closes the
menu first, so the window never stands over a menu going down under it, and raises the window on
`event.stamp`: raising a window is what a display server weighs against the click that asked.

## connectAppCommand

Adds a command of the application's own to the strip, under its pages and above Show diagnostic
and Exit, by caption and icon, once per application - and more than once for more than one command,
in the order asked. It wears the end mark it states as well: `EndIcon::OpensWindow` for a command
that opens a window.

    connectAppCommand(L"New project", paintNewProjectIcon, EndIcon::OpensWindow,
        [&workspace](ClickEvent& event){
            event.closeForm();
            workspace.createNew({}, event.stamp);
        });

## openAppMenuPage

Opens the application menu over the form on the page under that caption, the way the menu opens on
the page it was left on: the caption is written where the next menu reads its page. The menu opens
on the AppButton presenting appMenuAction in that form, as a click on the button opens it - under
the button, closed by a click elsewhere or by Escape. A caption no page answers to opens the
Settings page. The stamp is the press that asked, which a menu taking the pointer is weighed
against. A notice's answer opens Settings this way.

## Notices

**What the application tells the user on its own time** - the answer to something that ran after the
click that asked for it, a newer release found by a check nobody is watching. One list for the
application, oldest first, which `notices()` answers. The main window shows it as cards over its
content against its bottom right, the newest at the bottom - see NoticesLayer. A notice takes no
focus, and stands until the user dismisses it, presses one of its answers, or its owner withdraws
it.

**A notice is held by a handle**, and the handle by whatever the notice speaks for. A handle gone
takes its notice down, so a notice never outlives what it is about, and an answer never runs after
its owner has gone: a notice that a project's source is gone is held by the project, and closing the
project takes it down.

**One notice per subject.** An owner keeps one handle and assigns each new notice to it, which takes
the one before down - an editor run again speaks for the newest run alone.

    m_notice = notices().post(Notice{
        .icon = MessageIcon::Error,
        .title = title,
        .message = message,
        .answers = {
            NoticeAnswer{
                .caption = L"Settings",
                .run = [](ClickEvent& event) {
                    openAppMenuPage(event.form, AppMenu::k_settingsPageCaption, event.stamp);
                }
            }
        }
    });

## Notice

What a notice says: its kind, which the icon and the stripe beside the message show, its title, its
message, and the answers it offers, each a caption and what pressing it does.

**The title names what the notice is about** - the command that ran and the file it ran on, as
`Open in editor: Main.cpp` does - and stands in bold over the message, which says what happened. A
notice may come long after the click that started it, on another page, and the title is what ties
it back.

An answer is something the application does about the notice - Settings, Rescan, Release page - and
says so. Dismiss follows the answers on every notice, and is the one a notice with no answers
offers.

## NoticeAnswerHandler

Takes the click that pressed the answer. Its stamp is the press behind it, which is what an answer
asking the display server for something names - opening a page in a browser, raising a menu. Its
form is the window the notice stands in.

## NoticeHandle

A notice stands while its handle does. Taking another handle into this one takes this one's notice
down first. `standing()` answers false once the user has closed or answered the notice, and
`withdraw()` on a notice already down does nothing. A handle belongs to an object that goes down
with the application, never to a static, so the list is empty by the time it goes itself.

## Notices::post

Puts the notice up after those standing and raises NoticesChangeEvent. The handle it answers is the
notice's lifetime, so a handle dropped on the spot takes the notice down on the spot.

## Notices::answer

**The notice comes down before the answer runs.** The answer may put a notice up in this one's
place, run a loop of its own, or close the window the notice stands in, and in none of them is this
notice still standing. An answer whose notice is already down does nothing.

## notices

The application's one list, made on first use and kept the way the backstage keeps its pages.

## NoticeCard

A notice as the main window shows it: a MessageBar in the notice's kind, its title in bold over the
message, and its answers under the message against its right edge with Dismiss last, the way a
dialog's buttons stand. Every button on it is MouseOnly, so a press on one leaves the focus where it
was. As wide as the window allows, up to 440 design units, and as tall as its message at that width.

## NoticesLayer

The main window's top layer - see FormBase::topLayer. It holds a card for every notice standing,
stacked from the window's bottom right with the newest at the bottom, and keeps in step with the
list on every change. A card whose notice is down is hidden at once and deleted on a timer once no
press on it is still running: it goes from inside its own answer's click as often as not, an answer
may run a loop of its own, and a control is not deleted from under the click it is dispatching.

**Every card casts its own shadow**, coloured by the rule set it wears - MessageBar's Hint - as a
hint window's is, at the theme's secondary window shadow and around the card's corners. The shadows
are drawn under the cards from one FormBase::shadowPainter, and reach past the cards, so the layer
repaints whole on every change to the list.

## UiElement

The order is free. What is indexed by an element - `k_uiElements`, a theme's element lists,
the baked effects - follows the enum, and an editor presents elements in any order it likes by
naming them one at a time.

## UiElementDescriptor

What one element is: its three spellings, what it is painted on, whether it opens a window,
and - for the elements an ink names - the ThemeColors member holding that effect.

**The three spellings are not the same word.** `name` is the label an editor writes, `codeName` is
the member generated C++ assigns an effect to, and `token` is what stands in a theme file - the key
an element's list of rules stands under, and its effect's where it has one.

## appThemes

**The one manager an application makes**, over the themes directory the publisher's applications
share - `ApplicationBase` holds it and `connectAppThemes` names it here, so a page reaches the
themes without being handed them. It answers before any window exists and for as long as the
application runs; asking for it before `connectAppThemes` has run is a precondition failure, not
an empty list.

## storedTheme

**A path naming nothing is not an application without a theme.** A user theme can be renamed or
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

**The mode the application is drawn in**, which a ColorModeSetting is not: Dark and Light answer for
themselves, and Auto answers with the desktop's mode - see Platform::systemColorMode in Context.
Asked each time a theme is put on rather than kept, so it cannot fall behind the desktop.

## ThemesManager

**The application's own documents folder**, of theme files, built in ApplicationBase and answered by
appThemes: the Themes application browses it, the app menu picks from it, and the Settings page
lists it. A DocumentsFolder - see Documents - so the directory, the watch, the listeners and the
sorted read are the base's; what the manager adds is a parsed theme per file, rebuilt in
filesRead each time the base has read the directory afresh, and the built-in, answered from the
compiled-in colours whatever stands on the disk.

## ThemesList

**The themes an application can wear**, in two groups: the built-in theme and the themes in the
directory. A StackView, each group an expander whose body holds the tiles, so collapsing a header
takes its tiles with it. It answers IDocumentTiles - see Documents - so the layer's home page
works over it as it works over a flat list, and the Themes application's home page is that page
with a preview.

**The view alone, with no box around it.** What scrolls the themes belongs to whatever holds them: a
page carries these sections on the box its own column stands on, and a window that gives the list
the room puts it in one of its own - `createBody<ScrollBoxWith<ThemesList>>`. A box here would be
a second bar inside the first, kept short by a stated height that cuts the tiles off. `view()`
answers the list itself, for a host saying which of the two it means.

The list listens to the manager and builds itself afresh whenever the directory reports. **The
file name is what it comes back to**, not the tile: every tile is taken down by a rebuild, and a
theme is the same theme under the same name. The built-in goes by the name its page goes by,
Default.theme, so a tab coming up from it lands on its tile. `selectFileNameAfterRebuild` is how a
caller names a theme whose tile does not exist yet - one just written to the disk, or the one that
will stand where a selection has been taken away - and it opens the user group as well, because a
tile made into a closed group is never laid out.

What the list does **not** do is act on the themes. Renaming a file, deleting one, opening one: a
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
