# Diagnostic

The author's side of the footnote of the same name.

## Options

The diagnostic window asks more of the switch than `if constexpr` gives: the controls it is built
from - the FPS page's grids among them - must not reach an executable that never opens it. The two
linkers decide that differently, and the module is laid out for both.

- Clang links from a static library and pulls an object in as soon as anything references a
  symbol it defines. An object that imports a module calls that module's initializer, so it pulls
  the module's object in, and that object's vtables pull in the rest - an import with nothing
  used is enough. The entry points ApplicationBase calls therefore stand in
  `Diagnostic.Log.Switch.cpp`, which imports nothing of the window, and reach the window's half in
  `Diagnostic.Log.cpp` only from inside `if constexpr (enabled)`. Off, nothing references that
  object, and neither it nor the FPS page nor the grids are pulled in.
- MSVC links every object of the project and drops only the functions nothing references. An
  object's dynamic initializers always run, so a namespace-scope object keeps whatever its
  constructor and destructor reach. The window is held in a function-local static,
  `diagnosticWindow()`, and what it keeps - the copy action, the colours of the controls a line
  names - are members of its classes. Nothing in `Diagnostic.Log.cpp` stands at namespace scope
  with a constructor or destructor that does work.

createDiagnosticLogConfigSchema stands with the entry points in `Diagnostic.Log.Switch.cpp`,
outside `if constexpr`, so the schema is the same whichever build wrote the config.

The release recipe passes both switches - `.github/workflows/release.yml` for Windows,
`.github/release/build-linux.sh` for Linux - so an application published from GitHub carries no
diagnostic, however the source was left.

## createDiagnosticLogConfigSchema

The constructor selects no tab at all - which page the window opens on is the config's answer, and
restoreDiagnosticLogState is the first moment that answer exists.

The page is kept by its tab caption rather than by a number, so a page added between them moves
nothing. The captions are stated once and the tabs are built from them, so the name written and the
name matched cannot come apart.

## FpsPage

The drive is a repeater, which is a timer - the lowest priority message there is. A pointer
streaming moves, and the paints they cause, hold it off for as long as the pointer keeps moving, so
the whole frames are the idle thread's work.

A window that has gone is never reached, because the one pointer kept to it is compared and never
dereferenced - what the window grid says about it was copied while it was live.

**The numbers are text cells of moving columns** - see Grids::MovingText. Rewritten ten
times a second, they are shaped past the layout cache, which a text cell of an ordinary
column would cycle and empty for every other window.
