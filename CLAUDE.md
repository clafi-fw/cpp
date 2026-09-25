# ClaFi

A C++20-modules UI framework. Windows (Win32, Direct2D/DirectWrite) and Linux (Wayland, CPU
rasterizer), sharing everything above the platform layer.

## Code style

The `cpp-code-style` skill (`.claude/skills/cpp-code-style/SKILL.md`) is the authoritative C++
style directive. Read it before writing or editing any C++ in this tree.

The declaration routine is how a control, its properties and its events are declared so that a
line scanner can harvest them. A comment on a harvested
declaration is one line, and the words that do not fit go into a note under `Source/RawDocs`,
referenced from the line. `python "Tools/Surface Scanner/scan.py" --check` reports every departure.

## Layout

Library code is under `Source/`, with the notes the code refers to beside it in
`Source/RawDocs/`; the manuals are under `Documentation/`. `Showcase/`
and `Tools/` are applications that import the library, not part of it.

## Two builds, and they are not equivalent

- **Visual Studio** - the solutions under `Showcase/` and `Tools/`, sharing
  `Visual Studio Shared Items/ClaFi.vcxitems`. MSVC, `stdcpp20`, `BuildStlModules`. The
  primary development environment.
- **CMake + clang + Ninja** - the Linux build:

      cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=wayland \
            -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++ \
            -DCMAKE_BUILD_TYPE=Release

  `CLAFI_PLATFORM=none` builds the library without the platform layer. Compiling is the whole
  point there; a link would only report an absence that is already known.

## Build facts that are load-bearing

- **Ninja.** The Visual Studio and Makefile generators do not scan module dependencies.
- **`-stdlib=libc++`.** Required for `import std` under clang.
- **C++23.** CMake wires `import std` only for targets at `cxx_std_23` or newer. MSVC serves it
  at C++20; CMake will not. `CMAKE_CXX_STANDARD 23` exists for that reason alone - the code is
  C++20.
- **A build type.** A single-config generator with none passes no `-O` and no `NDEBUG`, and the
  compile line still looks complete. The tree is a software rasterizer; unoptimised it measures
  about ten times slower than MSVC.
- **A fresh build directory after any import change.** A stale BMI keeps a name reachable that
  the source no longer asks for, and hides the error.
- **No `-fexperimental-library`.** CMake builds its synthesized `std` module without it, and
  clang refuses to load a BMI whose configuration differs from the consumer's.

## Invariants

- **`ClaFi.StdLib` is `export import std;` and nothing else.** Every std name is written
  `std::` at its use site, types included. Never add a using-declaration or alias back.
- **The bottom three layers are ordered** `StdLib` -> `Core.System.Utils` ->
  `Core.System.UiTypes`. Utils must not import UiTypes. Anything UiTypes itself uses has to be
  declared at or below Utils.
- **Import the specific declaring module, never a facade.** `*_Facade.cppm` and `ClaFi` are
  facades; importing one from inside a partition it re-exports closes a cycle.

## Manuals

Subsystem manuals are self-contained HTML in `Documentation/`.

`Source/RawDocs/` holds the prose that would not fit above a declaration, one note
per subsystem, referenced from the code by `See <note-stem>`. It is the raw material the end-user
documentation will be built out of, and nothing in it is shipped in that form.
