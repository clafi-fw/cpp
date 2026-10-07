# SeeDocs

The tools that read the framework's design surface out of its source and put it to use. Two
programs so far: the scanner, which reads every module under a tree of sources, writes what it
finds into one database and holds the tree to the declaration routine; and the studio, a window
over that database showing the surface as the documentation it is to become.

    seedocs scan [tree] [--out file]    writes the database, the tree's .seedocs/Surface.cfg by default
    seedocs check [tree]                reports deviations, exit code 1 if any are errors
    seedocs-studio [tree]               opens the studio over the tree as a project

The tree is the folder of sources, read as it stands - for the framework, its `Source/` folder;
nothing is appended to the path given. For the scanner it is the working directory when left out.
The report reads `file:line: level: text`, one line per finding, in file and line order.

## Projects

A project is a tree with a `.seedocs` folder in it. Everything SeeDocs makes of the tree is kept
there - the database the scanner writes and `Project.cfg`, the project's properties, which is its
name so far, taken from the folder's own - so the tree itself is left as it was found. A scan into
the tree's own database makes the folder; the studio makes it with the user's consent, asked in a
dialog, and scans the tree at once so the project opens with its database in place. A `--out`
file written anywhere else makes no project.

## How it reads

Every `.cppm` and then every `.cpp` under the tree is lexed with the framework's own C++ lexer
(`ClaFi.Core.Syntax`) and read declaration by declaration over the tokens; a folder whose name
starts with a dot - `.seedocs`, `.git`, `.vs` - is skipped with everything under it. Nothing is
expanded and nothing is compiled: a macro of the declaration routine - `DECLARE_PROPERTY`,
`DECLARE_EVENT`, `BIND_PROPERTY_*`, `READ_PROPERTY`, `REQUIRE_PROPERTY` - is read as the
declaration it stands for, and a function body is skipped, read only for the binds a
constructor writes (`INIT_PROPERTY`, `BIND_MEMBER` and its kind) and for a `Props::get` written
by hand.

What is harvested: every exported type - class, struct, union, enum, alias, concept - with its
public and protected members (properties, events, functions, data members, enum members), every
exported free function and constant, and every module with its imports. A nested type is an entry
of its own, named `Outer::Inner`. Types are keyed by qualified name, so `Grids::Grid` and
`Grids::Dt::Grid` are two entries.

A declaration's comment is the `//` line touching it from above, or the one on its own line; a
class template's may sit between the `template` line and the `class` line, or above the
`template` line. The comment's first line is what the database carries, with a trailing
`See <note>#<anchor>` reference cut off into the entry's `Note`.

## The database

`Surface.cfg` is a document in the framework's own configuration format, written through the
Dom with the layout `SeeDocs_App::databaseLayout()` states, and read back through the same
layout by `readDatabase`, which answers the surface the scanner would have filled. An entry
states only what it has - a key at its default is left out - so a reader applies the layout and
takes the rest from the file.

Where an entry says the same thing a `Language.cfg` completion entry says, it uses the same key:
`Name`, `Kind`, `Signature`, `Hint`, `Type`, and a class's `Methods` and `Properties` as two lists.
The two files are meant to become one.

    Modules = [
        =[ Name, File, Interface, Imports = [...], ExportedImports = [...] ]
    ]
    Types = [
        =[
            Name, Namespace, Kind (class, struct, union, enum, type, concept), Module, File, Line,
            Hint, Note, Template, Bases = [...], Target, Control, Category,
            Properties = [ =[ Name, Type, Accepts = [...], Default, Setter, Target, Form,
                              ValueKind, Optional, Access, Line, Hint, Note ] ]
            Events = [ =[ Name, Alias, Method, Access, Line, Hint, Note ] ]
            Methods = [ =[ Name, Kind (function, constructor, destructor), Signature, Type,
                           Template, Static, Virtual, Deleted, Access, Line, Hint, Note ] ]
            Fields = [ =[ Name, Type, Value, Static, Constant, Access, Line, Hint, Note ] ]
            Members = [ =[ Name, Value, Line, Hint, Note ] ]
        ]
    ]
    Functions = [ =[ Name, Namespace, Kind, Signature, Type, Template, Module, File, Line, Hint, Note ] ]
    Constants = [ =[ Name, Namespace, Type, Value, Constant, Module, File, Line, Hint, Note ] ]

`Control` marks a type whose base chain reaches `Control` or `RichControl`, template arguments
walked as bases, which is what makes it a toolbox entry; `Category` is its folder under the
tree. `Bases` are written as a reader can follow them: an alias the database does not
carry - a private `using ComboBoxBaseClass = WithInPlaceEdit<DropdownControlBase>` - is replaced
by what it names. A property's `Form` says how its line was written (`declared`, `writable`,
`reference`, `storage`, `member`, `call`, `value`, `action`, `read`, or `required` - a
`REQUIRE_PROPERTY`, which the pack has to carry and which therefore has no `Default`) and
`ValueKind` what a designer shows the value as: `bool`, `int`, `float`, `text`, `float2`,
`int2`, `float4`, `int4`, `color`, `enum`, `number`, `object`, `handler` (a `std::function`,
wired the way an event is) or `unknown`. A `std::optional` of a kind is that kind with
`Optional` set. Several bindings landing on one target are one property, the further types
under `Accepts`. An event's `Name` is its struct, whose payload is that struct's own `Fields`
entry.

## The studio

`SeeDocs Studio` opens a project - the tree named on its command line, else the project it was
last left on - and shows the project's database and the notes under its `RawDocs` as pages. The
application menu's Open page is where projects are opened: a Browse button asks for a folder in
the desktop's own dialog, and the projects opened before stand under it, most recent first, one
click each. A folder with no `.seedocs` becomes a project there, once the user has agreed to the
folder; the project's name heads the window title. Which project is open and which were opened
before are kept with the application's settings, so they are remembered once settings are kept on
the computer.

The pages: down the left a tree of chapters - the tree's folders, a reader's names for them
- each holding its modules and each module the types it exports, controls first; on the right the
page of whatever is picked. A page is a column of controls: its title with its hint under it,
a grid of facts - namespace, module, source - and then a section per expander. A type's title
carries the ways up through its bases under it, written the C++ way - a colon before each base,
a mixin's host through the argument it was given, a second line where the bases fork. A
chapter's or a module's page holds a grid of its types with their kinds and hints, the chapter's
grouped by module under held headers. A type's page holds the types derived from it as a tree,
the note its comment references as prose, and a grid per member kind:
properties, events, methods with the protected ones under a header of their own, fields or
members, each with its hint. A member whose comment references a note carries a mark in a column
of its own, and the note stands as a footnote under the grid; the mark leads to it. A type whose
comment references nothing takes the section named exactly after it from a note of its own
folder, marked as matched by name. Where the surface states no hint the page says so, in place of
the words, so what the documentation still lacks is read off the preview.

A type's name on a page is a link to its page, in a grid cell as in prose, and a row of the
derived types opens the type it names; the tree on the left follows. The pages are built by
`SeeDocs_App.Pages` - facts, sections, tables of rows with their footnotes, branches of a tree,
and blocks of prose - out of the surface and the notes; the studio builds controls from them
(`SeeDocs_App.Studio.PageView`) and a generator renders the same pages as files.

## The checks

- a property with no `INIT_PROPERTY`; a `..._STORAGE` property without its getter and a
  `WRITABLE` one whose setter is not there, the base chain searched as well
- a `Props::get` or `Props::find` written by hand rather than through `READ_PROPERTY` or
  `REQUIRE_PROPERTY`
- a comment of more than one line, one in both places at once, or none on a declaration of the
  design surface - a control, its properties and events, an exported enum
- a comment above a declaration it would fit on, and one riding a line past 100 columns
- an event whose three spellings disagree, or whose struct is not found
- a property whose type is neither an enum, a known value kind, nor a declared type
- a `See` reference to a note that does not exist under the tree's `RawDocs`, or to a heading the
  note does not have - on every line of a comment
- a class declaring properties or events whose base chain reaches neither Control nor a type
  the scanner can read
- a value type declared under `Controls/Base`, where it reaches an application only through an
  import of the base

## Building

`SeeDocs.sln` builds both programs with Visual Studio against the shared items: `SeeDocs` the
scanner and `SeeDocs Studio` the studio, which share the Surface, Scanner, Database, Project,
Notes and Pages modules under `Src/`, the studio's own standing under `Src/Studio/`. The CMake target
`SeeDocsStudio` builds the studio on Wayland beside the other applications. The CMake target
`SeeDocs` builds the scanner on Linux with or without a platform layer - the Dom, the Syntax lexer and the text
file reader reach no further down than `UiTypes` - so a `check` runs wherever the tree is
checked out:

    cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=none \
          -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target SeeDocs
    ./build/SeeDocs check Source
