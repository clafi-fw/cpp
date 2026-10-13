# Application

The words that do not fit above a declaration.

## Application

**The CPU backend is every application's**, and the GPU backend is the application's to name.

- `Application<Win32Platform>` draws on the CPU and offers the user no choice.
- `Application<Win32Platform, Direct2DBackend>` carries both, and starts on the one the config
  holds - see AppContext::gpuAcceleration.

## ApplicationBase finalize

**The config is saved at exit** wherever the user allowed it - finalize calls `saveConfig(false)`.
An application that never saves still keeps its theme, its window placement and the diagnostic
window's state.

## ApplicationBase saveConfig

- `saveConfig(false)` writes only where the config folder exists, which is the user's permission
  to store. Saving early is harmless - the save at exit writes again.
- `saveConfig(true)` makes the folder first. It is for an application that stores without being
  allowed to.

## ApplicationBase config

**The config is loaded before the application's own code runs**, so it already holds what the
user chose. Write a default of the application's own only where `configFolderExists()` is false.

## configSharePath

`<publisher>/Share/`, beside the folders of the publisher's applications. Each of them reads its
user themes from `<publisher>/Share/Themes`. The path ends with a separator, so a name is appended
as it is.

## AppMenu

A popup holding a TabbedBox, under every AppButton. Its strip, top down:

- the application's pages - see connectAppPage;
- the application's commands - see connectAppCommand - then Show diagnostic, where
  Diagnostic::Options::enabled, and Exit;
- Information above Settings, at the foot.

**Exit asks the window to close**, as its close button does, so a page holding unsaved work can
refuse.

**It opens on the page it was left on**, stored by caption under AppMenuPage. A caption the strip
does not have opens Settings.

**The menu keeps the size it was placed at**, so a section opened on Settings never moves an edge
under the pointer. Its floor fits the Settings page with every section open.

## connectAppMenu

ApplicationBase::initialize calls it. Until then appMenuAction has no handler, and a button
presenting it shows as disabled.

## OptionsPage

The page every backstage page is built on: a vertical Stack of sections, each an expander whose
caption heads a column of options. addGroup adds a section and returns its column. Collapsing a
caption hides its options, and the caption is drawn as a divider with no surface under the options.

## OptionsPage addSection

**Hide the section, not its options.** A section draws its caption and line itself, so one whose
options are all hidden still shows them.

## SettingsPage

The options every application has, then the application's own groups - see connectAppSettings -
on a box that scrolls, with the Keep settings check at the foot. The page is built each time the
menu opens, so every option is read where it lives.

**The bar is always shown**, so a section opening or closing moves nothing across the page.

- **Theme** - a ThemePick, and a button for each colour mode setting - see applyColorMode. A theme
  tried on goes when the menu closes.
- **Scale** - a ScaleSlider, a percent per unit, writing AppContext::scale, and every window takes
  it. The readout shows the percent that was taken.
- **Z-Hover** - how far controls move in depth, writing AppContext::zAnimation. 100% is the depth
  the theme states. The row is there where CLAFI_TEXT_MOVING is 1, and at 0 every control takes
  that depth.
- **Always on top** - for the application's window. It is hidden where the platform has no
  keep-above - see IPlatformWindow::canSetAlwaysOnTop.
- **GPU acceleration** - writes AppContext::gpuAcceleration, and every window switches backend.
  Its section is hidden where the application named no GPU backend.

**Keep settings on this PC is the config folder** - see AppContext::configFolderExists. Ticking
asks, then makes the folder and writes the config at once. Clearing asks, then deletes the folder
with everything in it. In each question, a part of the path whose folder exists is a link to it.

## ThemePick

A ComboBox of the themes an application can wear, each item a picture of its theme beside the
name. A lane of the list takes up to ten themes.

**Looking tries a theme on, picking states it.** A theme the pointer or the keys reach is worn,
and nothing is written. The list closing on another theme writes that one to the config; closing on
the one it opened on puts the stated theme back on. The items are rebuilt when the themes folder
changes.

## ScaleSlider

**The slider's own form is drawn at the scale it sets**, so from the press to the release the form
keeps a scale of its own - FormBase::holdScale, then FormBase::followScale. Arrow keys and end
buttons take no hold: each is a step, so the form grows a percent at a time.

## InformationPage

The Information page of the application menu, top down:

- the name, and the version at the far end of its line;
- the update row, where AppContext::updateCheck is available - a major.minor.patch version and a
  repository in AppParams::updates;
- the publisher, AppParams::site, AppParams::description, the paragraphs from
  AppContext::addInformation, and the framework.

**The name is a Text**, and its inks show wherever the name does. The plain text is the identity -
the config folder, the application id and the native window title are made from it.

**Added paragraphs are plain strings**, for facts the application learns at startup. Every address
from https:// to the next blank in one becomes a link.

**Every text is a read-only TextBox**, so its links work and it can be selected and copied.

**The update row** shows what the check found:

- Unchecked - only the button.
- Checking - "Checking for updates…", the button dimmed.
- Latest - "You're up to date".
- Newer - "Version 0.2.0 is out", and a link to the release page.
- Failed - "Could not check for updates", and the button reads Try again.

Checking, Latest and Failed show "last check:" under them, with the age of the last answer that
arrived. The age is worked out when the page is built and when the check moves. The answer is kept
in the config, so a new run shows an earlier run's - see UpdateCheck::keepIn.

## AppPageBuilder

**Runs each time the menu opens**, so it reads every option where it lives. It calls addGroup for
each caption and fills the column that comes back:

    connectAppPage(L"Scene", Icons::DotMark::paint, [](OptionsPage& page){
        Stack& picked = page.addGroup(L"Picked Window");
        picked.add<CheckBox>(L"Highlight corners");
    });

## connectAppPage

**Once per application** for each page, in the order the strip shows them. The icon is drawn at the
size of the framework pages' icons, and `Icons::DotMark::paint` holds the slot for a page with no
picture, so its caption lines up with the others.

## connectAppSettings

**Once per application** for each builder, in the order asked. The groups follow the common
options, which keep one place in every application.

    connectAppSettings([](OptionsPage& page){
        Stack& editors = page.addGroup(L"External editors");
        editors.add<TextBox>(PlaceHolderText{ L"A command line" });
    });

## AppCommandHandler

**The menu stays up** unless the handler closes it with `event.closeForm()`. A command that opens a
window closes the menu first, then raises the window on `event.stamp` - a display server weighs a
raise against the click that asked for it.

## connectAppCommand

**Once per application** for each command, in the order the strip shows them. A command that opens
a window states `EndIcon::OpensWindow`.

    connectAppCommand(L"New project", paintNewProjectIcon, EndIcon::OpensWindow,
        [&workspace](ClickEvent& event){
            event.closeForm();
            workspace.createNew({}, event.stamp);
        });

## openAppMenuPage

Stores the caption as the page the menu opens on, then opens the menu as a click on the form's
AppButton does. A caption no page has opens Settings. The stamp is the press that asked, which the
display server weighs the menu against.

## Notices

What the application tells the user on its own time - the answer to work that ran after the click
that asked for it, or a newer release found by a check nobody is watching. `notices()` answers the
one list.

**The main window shows each notice as a card** at its bottom right, the newest lowest. The cards
answer mouse clicks only, so the focus stays where the user left it. A notice stays up until the
user dismisses or answers it, or its owner withdraws it.

**A notice is held by a handle**, and the handle by what the notice is about - a project holds the
notice about its source. The handle going takes the notice down, so an answer never outlives its
owner.

**Keep one handle per subject** and assign each new notice to it, which takes the one before down:

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

**The title says what the notice is about** - the command and the file, as in
`Open in editor: Main.cpp`. A notice may come long after the click that started it, and the title
ties it back. The message says what happened.

**An answer is something the application does** about the notice - Settings, Rescan, Release page.
Dismiss follows the answers on every notice.

## NoticeAnswerHandler

The click's `stamp` is the press behind the answer, for an answer that asks the display server for
something - a page in a browser, a menu. Its `form` is the window the notice is in.

## NoticeHandle

**Assigning another handle takes this one's notice down first.**

- `standing()` is false once the user has closed or answered the notice.
- `withdraw()` on a notice already down does nothing.

Keep a handle in an object that goes down with the application, never in a static.

## Notices::post

**The handle it returns is the notice's lifetime** - a handle dropped at once takes the notice down
at once.

## Notices::answer

**The notice is down before the answer runs**, so the answer may post a notice in its place, run a
loop of its own, or close the window. An answer to a notice already down does nothing.

## UiElement

**The order is free.** What is indexed by an element - `k_uiElements`, a theme's element lists, the
baked effects - follows the enum's order.

## UiElementDescriptor

- `name` - the label an editor shows.
- `codeName` - the member generated C++ assigns an effect to.
- `token` - the key in a theme file for the element's rules and its effect.

## appThemes

**The application's one ThemesManager**, over the themes folder the publisher's applications share.
It answers from before the first window until the application ends. Asking for it before
connectAppThemes has run is a precondition failure, not an empty list.

## connectAppThemes

ApplicationBase calls it, before the config is read. From then on a change of the config's theme
path or colour mode, or under Auto of the desktop's mode, puts on the stored theme.

## storedTheme

**A path naming nothing answers the built-in Default.** The config keeps the path, so a renamed or
deleted theme whose file comes back is worn again.

## applyTheme

Puts a theme on without writing the config - the route a preview takes. Without a mode it is worn
in the mode wornColorMode answers.

## applyStoredTheme

Call it wherever a theme was worn that the config does not state - the menu closing over a
preview, the Themes application turning its preview off.

## applyThemePath

**Writing the path is choosing the theme**, and it is kept between sessions where the user allowed
storing. A path equal to the stored one changes nothing, so take a preview back with
applyStoredTheme.

## applyColorMode

Writes the setting the way applyThemePath writes a path, and the stored theme is put on in the new
mode.

## AppContext::colorMode

**The mode is the application's, not the theme's**, so every theme is worn in it. Write it with
applyColorMode, and ask wornColorMode for the mode Auto means.

## wornColorMode

Auto answers with the desktop's mode - see Platform::systemColorMode. **It is asked each time a
theme is put on**, so it keeps up with the desktop.

## ThemesManager

A DocumentsFolder of theme files: the base keeps the directory, the watch and the listeners. The
manager adds a parsed theme for each file, read again with the directory, and the built-in,
answered from the compiled-in colours whatever is on the disk.

## ThemesList

The themes as tiles in two expanders - the built-in, and the user's files. It answers
IDocumentTiles, so a DocumentsHomePageBase works over it as over a flat list.

**The host scrolls the list.** A page puts it on its own scrolled column, and a window gives it a
box - `createBody<ScrollBoxWith<ThemesList>>`.

**A rebuild comes back to the file name**, since every tile is new. The built-in goes by
Default.theme. selectFileNameAfterRebuild sets the file to land on when its tile does not exist
yet, and opens the user group.

**The host acts on the themes.** A tile raises a rename, a delete or an open, and the host answers
through DocumentEditHandler, DocumentMenuHandler and the click events.

**The list is a picker by default.** A host states `SelectionMode::Multi` where a command acts on
several themes. Only a user tile can be selected.

## ThemeTile

**Only a user theme can be renamed** - a built-in has no file, so `userTheme()` is null. `theme()`
is the manager's own theme, by reference, so a preview from the tile puts on that theme itself, not
a copy. It stays valid while the list lives.
