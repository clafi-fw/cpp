# SeeDocs

The tools that read the framework's design surface out of its source and put it to use. This
first part is the scanner: it reads every module under `Source/`, writes what it finds into one
database, and holds the tree to the declaration routine.

    seedocs scan [tree] [--out file]    writes the database, Tools/SeeDocs/Surface.cfg by default
    seedocs check [tree]                reports deviations, exit code 1 if any are errors

The tree is the folder holding `Source/`; the working directory when left out. The report reads
`file:line: level: text`, one line per finding, in file and line order.

## How it reads

Every `.cppm` and then every `.cpp` under `Source/` is lexed with the framework's own C++ lexer
(`ClaFi.Core.Syntax`) and read declaration by declaration over the tokens. Nothing is expanded
and nothing is compiled: a macro of the declaration routine - `DECLARE_PROPERTY`, `DECLARE_EVENT`,
`BIND_PROPERTY_*`, `READ_PROPERTY` - is read as the declaration it stands for, and a function
body is skipped, read only for the binds a constructor writes (`INIT_PROPERTY`, `BIND_MEMBER` and
its kind) and for a `Props::get` written by hand.

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
layout. An entry states only what it has - a key at its default is left out - so a reader applies
the layout and takes the rest from the file.

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
walked as bases, which is what makes it a toolbox entry; `Category` is its folder under
`Source/`. A property's `Form` says how its line was written (`declared`, `writable`,
`reference`, `storage`, `member`, `call`, `value`, `action`, `read`) and `ValueKind` what a
designer shows the value as: `bool`, `int`, `float`, `text`, `float2`, `int2`, `float4`,
`int4`, `color`, `enum`, `number`, `object`, or `unknown`. A `std::optional` of a kind is that
kind with `Optional` set. Several bindings landing on one target are one property, the further
types under `Accepts`. An event's `Name` is its struct, whose payload is that struct's own
`Fields` entry.

## The checks

- a property with no `INIT_PROPERTY`; a `..._STORAGE` property without its getter and a
  `WRITABLE` one whose setter is not there, the base chain searched as well
- a `Props::get` written by hand rather than through `READ_PROPERTY`
- a comment of more than one line, one in both places at once, or none on a declaration of the
  design surface - a control, its properties and events, an exported enum
- a comment above a declaration it would fit on, and one riding a line past 100 columns
- an event whose three spellings disagree, or whose struct is not found
- a property whose type is neither an enum, a known value kind, nor a declared type
- a `See` reference to a note that does not exist under `Source/RawDocs`, or to a heading the
  note does not have - on every line of a comment
- a class declaring properties or events whose base chain reaches neither Control nor a type
  the scanner can read
- a value type declared under `Controls/Base`, where it reaches an application only through an
  import of the base

## Building

`SeeDocs.sln` builds it with Visual Studio against the shared items. The CMake target `SeeDocs`
builds it on Linux with or without a platform layer - the Dom, the Syntax lexer and the text
file reader reach no further down than `UiTypes` - so a `check` runs wherever the tree is
checked out:

    cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=none \
          -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target SeeDocs
    ./build/SeeDocs check .
