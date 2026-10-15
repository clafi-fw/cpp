# Context

The words that do not fit above a declaration.

## FrameMargins

WindowFrame holds two: the `margins` around the window, where the shadow is drawn and the pointer
falls through, and the `overhang` the system places past the screen.

## PlacedWindow

**The size the window got, and no position** - a client is not always told where its window was
put. IPlatformWindow::standsAbove says which side of its anchor the window took.

## ThemeSwitchEvent

Raised on `AppContext::events()` - once for a set taken outright, and once per frame while a
crossing runs. `kind()` says which.

## ThemeSwitchKind

**The kind decides when a window paints.**

- `Taken` - an edit to the set worn, or a set with nothing to cross. A window waits for the
  scheduled paint, since the event is raised inside `AppContext::takeTheme`, part way through
  whatever took the theme.
- `CrossingFrame` - a window paints it at once, so every frame of the crossing is seen.

## BackendSwitchEvent

Raised on `AppContext::events()`. It carries nothing: every window is on the same kind, and
`AppContext::createBackend` answers with it.

## InputSwitchEvent

Raised on `AppContext::events()` at every step of either fade. `Input::mouse()` and
`Input::keyboard()` return the factors.

**Every visible window repaints whole**, since a rule reading `RuleInput::Keyboard` or
`RuleInput::Mouse` can sit on any control.

## ScaleSwitchEvent

Raised on `AppContext::events()`. `AppContext::scalePercent` returns the percent.

**A form can hold its scale.** FormBase::holdScale keeps it at the scale it is drawn at until
FormBase::followScale, as ScaleSlider does for the length of a drag. On a form with no parent
form, both calls do nothing.

## ZAnimationSwitchEvent

Raised on `AppContext::events()`. `AppContext::zAnimationAmount` returns the amount. Compiled only
where CLAFI_TEXT_MOVING is 1.

**Every visible window repaints whole**, since every control that takes the Z animation rests at a
new size. The layout stays: the press scale is a paint transform.

## SystemColorModeEvent

Raised on `Platform::events()`. It carries nothing - ask `Platform::systemColorMode` again.

One change raises one event, however many windows the platform heard it through.

## KeyDownEvent::isRepeat

**A key acts on every repeat** - that is what makes a held arrow travel. Only the first press
counts as a reach for the keyboard.

## KeyDownEvent::stamp

**Empty on a key nobody pressed** - a shortcut the framework raised for itself.

## KeyDownEvent during a drag

**The pressed control's chain is told first**, from the control the mouse went down on up to the
root, and the focused chain after it - what the first walk reached is not told twice. Escape
cancels a carry this way whichever control holds the focus.

## WindowFrame

**The form states the design through IPlatformWindow::setFrame**, and the platform answers with
what it applies in IForm::wnd_resize.

- Maximized, fullscreen or docked - no margins, no corners.
- Snapped or tiled - margins, no corners.
- A platform that cannot composite a frame - no corners.

The border is the root's own, painted with its content.

**The overhang is the platform's alone.** Win32 sizes a maximized window past the screen by its
resize borders, and the overhang says by how much. The form takes it off, so FormBase::geometry is
what the screen shows. Wayland reports none.

## WindowPlacement

**Every field is the form's own knowledge** - the anchor in the parent window's coordinates, and
what the content measured. Where the window lands is the platform's answer.

## Platform

**One platform per build.** `createWindow` is answered by the platform built for the display
server, Win32Platform or WaylandPlatform. It owns the clipboard, which a form reaches as
FormContext::clipboard.

## Platform::createFormsConfigSchema

**The shape is the platform's, and the platform alone reads it** - one section per form named.

- Win32 - the normal geometry in screen pixels, whether maximized, and whether held above the
  others.
- Wayland - the normal size in surface pixels and whether maximized, per form, plus one session id.
  Where a window was is the compositor's to keep.

## Platform::restoreFormPlacement

Called once per form, before its first placement, with the whole Forms section. It hands the
window what it read, and the first placement does the placing.

**On Wayland the order is the protocol's**: a toplevel is named to its session before the first
commit of its surface, which that placement makes.

## Platform::storeFormPlacement

**Called while the window is still up** - on hide, and a close hides first. That is the only time
its placement can be read.

## Platform::addFormToSession

**For the run in which the user allows storing.** A window made before then has no session, and
its surface is already committed, too late to restore. It is added to a session instead, which
keeps its place for the next launch.

Win32 tells the window manager nothing, and the call does nothing there.

## Platform systemColorMode

- Windows - the app mode of the personalisation settings.
- Wayland - the XDG desktop portal's colour scheme. A desktop stating no preference - GNOME's
  default look, or a session without a portal - reads as Light.

**On Wayland the first call waits up to a second for the portal**, so the first window opens in the
desktop's mode. A later answer arrives as a SystemColorModeEvent.

## Platform::events

**One dispatcher for the process**, there before the first window is made and after the last is
gone. It raises SystemColorModeEvent.

## Platform::pickFolder

**The dialog is the desktop's own**, with the places and shortcuts the user expects. It runs to its
close, and returns nothing when cancelled or when the platform has none.

- Windows - the form's window is disabled under it.
- Wayland - the XDG portal's dialog, modal to the form's window as far as the desktop allows.

**Ask from a toplevel.** A popup closes when the dialog takes the focus.

## ScopedWaitCursor

It puts back `Platform::cursor` as read when it was built, a system sizing arrow included. So
scopes nest, and a drag keeps its arrow.

## AppContext animator

**One controller steps every animation** - control state fades, glides, a hint's alpha, the theme
crossing - on one UiTimer. A control runs its own through Control::animate.

## AppContext createBackend

Returns a backend of the kind the application is on at that moment, so a window opened after a
switch matches the others.

The CPU backend is always there. The GPU backend is the one the application names - see
Application - and `gpuAvailable` says whether it did.

## AppContext gpuAcceleration

The `GpuAcceleration` node of the `Performance` section, true by default. **Writing it switches
every window** - the context raises BackendSwitchEvent.

Null where the application named no GPU backend. `usingGpu` is read from it as the context is
built, so the first window opens on the stored answer.

## AppContext scale

The `Scale` node of the `Interface` section: the percent of the design everything is drawn at, 100
by default. **Writing it rescales every window** - the context raises ScaleSwitchEvent.

`scalePercent` is the percent taken, held between `k_minScalePercent` and `k_maxScalePercent`
because the config is text the user may edit. It is read as the context is built, so the first
window opens at the stored size.

## AppContext::zAnimation

The `ZAnimation` node of the `Interface` section: how much of the theme's Z animation every control
takes, from 0 to 1, 0.5 by default. **Writing it repaints every window** - the context raises
ZAnimationSwitchEvent.

The amount scales `ControlMetrics::zDepthFactor` at paint, the resting and the held depth together.

**It exists only where CLAFI_TEXT_MOVING is 1.** Where the switch is 0, every control takes the
theme's full depth, and a `ZAnimation` key in a config is discarded as unrecognised.

## AppContext::configFolderExists

**Storing waits for the user's permission**, and the config folder is that permission. With the
folder there, the application reads its config on every start. The flag is read off the folder at
start, set by ensureConfigFolder and cleared by deleteConfigFolder.

A window's placement waits for it too: there is no config to restore from, and on Wayland no
session is asked of the compositor.

## AppContext::ensureConfigFolder

**The one place a config folder is made**, which is the user allowing storing. The question that
leads here names the folder, so what appears on the disk is what the user read. With no
appDataPath, it makes none.

## AppContext::deleteConfigFolder

**The whole folder goes**, because the folder is what the user was asked about.

- The publisher folder above it goes too once empty. Another application's settings, or the
  publisher's `Share` folder of themes, keep it.
- The application data root always stays - with no publisher named, it is the folder above.
- The first refusal stops the walk, so a folder held open elsewhere leaves the rest in place.

The config goes on answering from memory, with nowhere to be written.

## AppContext::k_mainFormName

**Two places must agree on it.** ApplicationBase names it in the Forms schema, and the application
hands it to FormBase::setConfigName before the main window's first show. A config name missing
from the schema has nowhere to be written.

## AppContext restoreFormPlacement

**Acts only while the config folder exists** - see configFolderExists. A form asks for it through
its config name, set with FormBase::setConfigName, before its first placement.

## AppContext storeFormPlacement

**Written whether or not the folder exists** - the folder decides when the config is saved. An
application that saves at its first exit, as the Themes app does, restores the placement from its
second run.

**While the folder exists, the window also joins the compositor's session** - see
Platform::addFormToSession. Ticking Keep settings on this PC needs no restart.

## IPlatformWindow::showWindowMenu

Maximize, Minimize, Move, Close and whatever else the desktop adds. A window drawing its own title
bar has to ask for it. The form asks on a right mouse click over the title zone that no control
claimed, with that press's stamp.

**The commands are the system's.** One chosen arrives as WM_SYSCOMMAND on Win32 and as configures
on Wayland, so a Close goes through wnd_closeRequested.

## AppContext updateCheck

Built from AppParams::updates and the version, and alive as long as the context, so an answer
outlives the page that asked.

**It asks only when the user does** - the Information page's button calls start.

**The last answer is kept in the config** under `UpdateCheck`, where the check is available and the
user allowed storing.

## UpdateSource

ClaFi's own applications publish from `clafi-fw/cpp` under tags such as `WhatsClip-v0.1.2`, so
WhatsClip's prefix is `WhatsClip-v`.

**Both parts go into addresses as written**, so neither may hold a character an address would
escape. An application naming no repository has no update check.

## UpdateCheckEvent

Raised on the UI thread each time the state moves:

- `Checking` when start asks, then `Latest`, `Newer` or `Failed`.
- `Latest` or `Newer` from a stored answer read back from the config.

An answer that changes nothing raises nothing.

**Call start from outside the handler** - start replaces the fetch whose done event the handler
may be running in.

## UpdateCheck

**One GET to GitHub** lists the tags that start with the prefix. A tag is taken for a published
release - the release recipe creates the two in one step.

Each version reads as major.minor.patch, and a tag that reads otherwise is skipped.

- `Newer` - a tag is above the running version.
- `Latest` - none is above it, a version bumped and not yet released included.
- `Failed` - no connection, a redirect, a status other than 200, or a body that is not a JSON
  array. GitHub answers 403 past sixty unauthenticated requests an hour from one address.

**A failure keeps the last `answeredAt`**, so it still says when an answer last arrived.

## UpdateCheck::createConfigSchema

- `AnsweredAt` - UTC to the second, as `2026-10-04T09:20:34Z`.
- `NewestVersion` - the newest published version as its tag spells it after the prefix, newer or
  not. Empty where no tag is published.

Both are text. AppContext keeps the section under `UpdateCheck`.

## UpdateCheck::keepIn

**A stored answer is compared with the running version**, so an update installed since the answer
arrived reads as `Latest`.

**A value that does not read is no answer** - a time spelled otherwise than the check writes it, or
a date that does not exist. The state stays `Unchecked`.

## UpdateCheck::start

**Asks only where `available()` is true.**

**Call it from outside an OnChange handler** - start replaces the fetch whose done event may be
raising that handler.
