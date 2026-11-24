# Application

The words that do not fit above a declaration.

## Application

**The CPU backend is every application's**, and the GPU backend is the application's to name.

- `Application<Win32Platform>` draws on the CPU and offers the user no choice.
- `Application<Win32Platform, Direct2DBackend>` carries both, and starts on the one the config
  holds - see AppContext::backendChoice.

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

`<publisher>/Share/`, beside the folders of the publisher's applications. An application that sets
`usePublisherThemes` connects to the `Themes` folder there. The path ends with a separator.

## themesPath

The connected publisher theme folder. The path is empty when the application leaves that folder
disconnected or the platform supplies no config root.

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

WindowApplicationBase calls it during initialization. It connects the menu action and registers
the app's declared link scheme. Keep the returned connection for the lifetime of the menu action
binding.

## ApplicationProperties

**The app may declare a link scheme.** Set `linkScheme` to a scheme name without its colon.
The framework opens Settings for `scheme://settings`. A fragment names the control to focus,
for example `scheme://settings#graphics-backend`. Settings opens the control's containing
sections and scrolls it into view. An unavailable target leaves Settings at its usual starting
position. The optional `linkHandler` answers other destinations on that scheme.

## AppMenuPage

The page every backstage page is built on: a vertical Stack of sections, each an expander whose
caption heads a column of options. addGroup adds a section and returns its column. Collapsing a
caption hides its options, and the caption is drawn as a divider with no surface under the options.

## AppMenuPage addSection

**Hide the section, not its options.** A section draws its caption and line itself, so one whose
options are all hidden still shows them.

## AppMenuPage anchorOption

**An anchor names a control on the Settings page.** Register it after adding the control, and use
its name as the fragment in a Settings URL. Give each anchor a unique name within the page.

## SettingsPage

The options every application has, then the application's own groups - see connectAppSettings -
on a box that scrolls, with the Keep settings check at the foot. The page is built each time the
menu opens, so every option is read where it lives.

**The bar is always shown**, so a section opening or closing moves nothing across the page.

- **Theme** - a ThemePick and a button for each color mode setting. A theme
  tried on goes when the menu closes.
- **Scale** - a ScaleSlider, a percent per unit, writing AppContext::scale, and every window takes
  it. The readout shows the percent that was taken.
- **Z-Hover** - how far controls move in depth, writing AppContext::zAnimation. 100% is the depth
  the theme states. The row is there where CLAFI_TEXT_MOVING is 1, and at 0 every control takes
  that depth.
- **Always on top** - for the application's window. It is hidden where the platform has no
  keep-above - see IPlatformWindow::canSetAlwaysOnTop.
- **Always show focus** - writes AppContext::alwaysShowFocus: the focused control wears its ring
  under the mouse too, where by default only the keys put one on.
- **Backend** - writes AppContext::backendChoice, and every window switches backend.
  Its section is hidden where the application named no GPU backend.

**Keep settings on this PC is the config folder** - see AppContext::configFolderExists. Ticking
asks, then makes the folder and writes the config at once. Clearing asks, then deletes the folder
with everything in it. In each question, a part of the path whose folder exists is a link to it.

## ThemePick

A ComboBox of the themes an application can wear, each item a picture of its theme beside the
name. A lane of the list takes up to ten themes.

**Looking tries a theme on, picking states it.** A theme the pointer or the keys reach is worn,
and nothing is written. The list closing on another theme writes that one to the config; closing on
the one it opened on puts the stated theme back on. The items are rebuilt when a theme folder
changes.

## ScaleSlider

**The slider's own form is drawn at the scale it sets**, so from the press to the release the form
keeps a scale of its own - FormBase::holdScale, then FormBase::followScale. Arrow keys and end
buttons take no hold: each is a step, so the form grows a percent at a time.

## InformationPage

The Information page of the application menu, top down:

- the name, with the version beside it;
- the update rows, where AppContext::updateCheck is available - a major.minor.patch version and a
  repository in AppParams::updates;
- the publisher, AppParams::site, AppParams::description, the paragraphs from
  AppContext::addInformation, and the framework.

**The name is a Text**, and its inks show wherever the name does. The plain text is the identity -
the config folder, the application id and the native window title are made from it.

**Added paragraphs are plain strings**, for facts the application learns at startup. Every address
from https:// to the next blank in one becomes a link.

**Every text is a read-only TextBox**, so its links work and it can be selected and copied.

**The first update row shows the status on the left** and the check button on the right.

- Unchecked - "Updates haven't been checked".
- Checking - "Checking for updates…", the button dimmed.
- Latest - "You're up to date".
- Newer - "Version 0.2.0 is available", a link to the release page.
- Failed - "Could not check for updates", and the button reads Try again.

**The second row shows the last-check age on the left** and Check automatically on the right.
It reads "last check: never" until a check returns a result. The age is worked out when the page
is built and when the check moves.
The answer is kept in the config, so a new run shows an earlier run's - see UpdateCheck::keepIn.

**Check automatically asks at the application's start**, at most once in 24 hours - see
UpdateCheck::startIfDue. The value is kept in the config, so ticking it where no settings folder
exists asks for one first - see allowSettingsStorage. The box stays clear where the answer is no.

**A newer release is told at start as a notice**, where Check automatically is on - found by the
check made then, or kept from an earlier run's answer where no check is due. Visit opens the
release page, and its tooltip shows the address. A check made later from the page raises no notice.

## AppPageBuilder

**Runs each time the menu opens**, so it reads every option where it lives. It calls addGroup for
each caption and fills the column that comes back:

    connectAppPage(L"Scene", Icons::DotMark::draw, [](OptionsPage& page){
        Stack& picked = page.addGroup(L"Picked Window");
        picked.add<CheckBox>(L"Highlight corners");
    });

## connectAppPage

**Once per application** for each page, in the order the strip shows them. The icon is drawn at the
size of the framework pages' icons, and `Icons::DotMark::draw` holds the slot for a page with no
picture, so its caption lines up with the others.

## connectAppSettings

**Once per application** for each builder, in the order asked. The groups follow the common
options, which keep one place in every application.

    connectAppSettings([](OptionsPage& page){
        Stack& editors = page.addGroup(L"External editors");
        editors.add<TextBox>(PlaceHolderText{ L"A command line" });
    });

## allowSettingsStorage

**The question Keep settings on this PC asks when it is ticked**, for any option that means nothing
unless the config is kept. Ok makes the folder and writes the config into it at once. The question
names the folder to be made, so ask it only where AppContext::configFolderExists is false.

## AppCommandHandler

**The menu stays up** unless the handler closes it with `event.closeForm()`. A command that opens a
window closes the menu first, then raises the window on `event.stamp` - a display server weighs a
raise against the click that asked for it.

## connectAppCommand

**Once per application** for each command, in the order the strip shows them. A command that opens
a window states `EndIcon::OpensWindow`.

    connectAppCommand(L"New project", drawNewProjectIcon, EndIcon::OpensWindow,
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

**The main window shows each notice as a card** at its bottom right, the newest lowest. Tab reaches
the answers after the window's last stop, and a mouse click on one leaves the focus where the user
left it. The focus a card held goes back where it came from when the card goes. A notice stays up
until the user dismisses or answers it, or its owner withdraws it.

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

**An answer's tooltip says where it leads** where the caption does not - the address a Visit opens.

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

## UiElementDescriptor

- `name` - the label an editor shows.
- `codeName` - the member generated C++ assigns an effect to.
- `token` - the name the theme states the element's set under, and the key for its effect.

## bake

**Laid out by the application's elements**: the framework's tokens in UiElement order, then the
names the application states - see elementNamesOf. Each name is answered by the style of that
name in the theme, resolved and baked, or by nothing where the theme states none.

## resolveStyleMetrics

**Every element's sizes, laid out as bake lays the rules out**: the framework's tokens in
UiElement order, then the application's own. Each name is answered by ThemeStyles
resolvedMetrics, settled - see settle - and an element whose style the theme lacks holds the
defaults. ThemesManager resolves both when it applies the theme. AppContext takes them together.

## settle

**These sizes follow from others**, whatever a style states, and are worked out once the chain
has resolved:

- a selection or hover indicator's height and a scroll button's are their widths, so both are
  square
- an indicator's maximum is its minimum, so its size is fixed and a list item can skip measuring
  it

The Themes app leaves the rows these would write out of the pages they follow on.

## elementNamesOf

Every name a baked set has an entry for, in the order the entries stand.

## ApplicationProperties

- `elements` names the application's own elements. Use `appElement(i)` for the i-th name.
- `defaultTheme` holds the caption and theme that the windowed application starts with.
- `additionalThemes` lists the application's other in-memory themes in Settings order.
- `legacyThemeFileDefaults` supplies fields omitted from older theme files. The manager copies
  the pointed-to theme at construction.
- `usePublisherThemes` connects the publisher's shared theme folder to the manager. It defaults
  to false.
- `themesOffered` controls whether Settings limits publisher themes to those that state a style
  for every element the application names.

## NamedTheme

The default's name is its Settings caption. For an additional theme, the name is also its
Settings identifier. Keep it unique, free of slashes, and stable across releases. The path
`App/Default` is reserved for the declared default.

## ThemesOffered

StatingEveryElement filters publisher themes by the application's own elements. Every offers all
readable publisher themes. Both settings offer the application's declared themes and readable
themes from its two `Themes` folders.

## WindowApplication

Supply a named default theme in ApplicationProperties. The constructor copies the default
and additional themes into its ThemesManager before reading the saved selection.

## ControlStyle in a file

A style is a section of the `Styles` list, whole: `Name`, then `Base`, `StandsOn` and `Seed`
where stated, its own `Rules`, its `Dropped` selectors, and a `Metrics` block holding the sizes it
states and nothing else - a size left out is left to the chain. A pair is two numbers a space
apart, across then down (`Padding=8 4`), a border its weight's name (`Border=Thin`). A list in a
file is the whole list, so a theme states every style it has, and a style a file leaves out is one
the theme lacks.

## WindowApplicationBase themes

The windowed application owns the manager. Pass it to controls that list or preview themes.

## ThemesManager applyTheme

Puts a theme on without writing the config - the route a preview takes. Without a mode it is worn
in the mode resolvedColorMode answers.

## ThemesManager restoreSelectedTheme

Call it when a preview ends.

## ThemesManager selectedTheme

**A missing saved theme falls back to the application's declared default.** The config keeps the
path, so a theme that becomes available again can be restored.

## ThemesManager selectTheme

**Writing the path is choosing the theme**, and it is kept between sessions where the user allowed
storing. Use restoreSelectedTheme to end a preview when the selected path is already stored.

## ThemesManager setColorMode

Writes the setting and applies the selected theme in the new mode.

## AppContext::colorMode

**The application has a single color mode setting.** Write it with ThemesManager setColorMode and
ask ThemesManager resolvedColorMode for the mode Auto means.

## ThemesManager resolvedColorMode

Auto answers with the desktop's mode - see Platform::systemColorMode. **It is asked each time a
theme is put on**, so it keeps up with the desktop.

## ThemesManager

The manager holds the application's default and additional themes. It applies the selected theme
when the config path or mode changes. Under Auto it also follows changes to the desktop mode.
The manager discovers `.clafitheme`
files in `Themes` beside the executable and in `Themes` inside the application's config folder.
With `usePublisherThemes`, the manager also serves as the DocumentsFolder for the publisher's
shared themes. `userThemes()` on the manager lists the publisher folder's files.

ThemePick and themeByPath use these Settings paths.

- `App/Default` selects the declared default. `App/<name>` selects an additional theme.
- `AppExecutable/<stem>` selects a file in `Themes` beside the executable.
- `AppConfig/<stem>` selects a file in `Themes` inside the application's config folder.
- `User/<stem>` selects a file in the publisher's shared `Themes` folder.

The folder identifies a file independently of other themes with the same name. The Themes editor
lists theme documents from the selected document storage.

## ThemesList

The list shows storage folders or the selected storage's theme files as tiles. Pass a
DocumentStorageView to supply the folder and its document reader. The list implements IDocumentTiles
for DocumentsHomePageBase.

**The host scrolls the list.** A page puts it on its own scrolled column, and a window gives it a
box - `createBody<ScrollBoxWith<ThemesList>>`.

**A rebuild restores the current file name.** Use selectFileNameAfterRebuild to select a file
whose tile has yet to be built.

**The host acts on the themes.** A tile raises a rename, a delete or an open, and the host answers
through DocumentEditHandler, DocumentMenuHandler and the click events.

**The list is a picker by default.** A host states `SelectionMode::Multi` where a command acts on
several themes. Only a user tile can be selected.

## ThemeTile

The tile refers to the theme read from its file. The list recreates its tiles when the
storage changes.
