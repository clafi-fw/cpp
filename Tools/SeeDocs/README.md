# SeeDocs

The tools that read the framework's design surface out of its source and put it to use. Two programs
so far: SeeDocsCli, the scanner, which reads every module under a tree of sources, writes what it
finds into one database and holds the tree to the declaration routine; and SeeDocs, a window over
that database showing the surface as the documentation it is to become.

    SeeDocsCli scan [tree] [--out file]   writes the database, the tree's .seedocs/Surface.cfg by default
    SeeDocsCli check [tree]               reports deviations, exit code 1 if any are errors
    SeeDocs [tree]                        opens the tree as a project

The tree is the folder of sources, read as it stands - for the framework, its `Source/` folder;
nothing is appended to the path given. For SeeDocsCli it is the working directory when left out.
The report reads `file:line: level: text`, one line per finding, in file and line order.

A tree is read in full or in limited mode. In full, it is a tree written to the declaration
routine: its modules are read, its macros of the routine harvested, its comments held to the
routine's rules, and SeeDocs writes the hints and footnotes edited in it back into the sources.
Limited mode is for anyone else's C++ or Delphi - a third-party library - and reads what such a
tree declares without asking anything of it: headers and units as well as modules, nothing
written into the sources, nothing checked.

## Projects

A project is a tree with a `.seedocs` folder in it. Everything SeeDocs keeps of the tree stands
there - the database the scanner writes, `Project.cfg`, the project's properties - its name,
taken from the folder's own, and how its tree is read - and `footnotes`, the notes a comment's
`See` reaches, written by the tree's authors - so the tree itself is left as it was found. A scan
into the tree's own database makes the folder; SeeDocs makes it with the user's consent, asked in a
dialog, and scans the tree at once so the project opens with its database in place. A `--out`
file written anywhere else makes no project.

How the tree is read is a property of the project, `Limited` in `Project.cfg`. The dialog that
makes a project asks whether the tree is a third-party library, and the Open page switches it
afterwards. SeeDocsCli reads a project as its property says, and a tree that is no project, or
one it makes, in full.

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

### Limited mode

A limited read takes every module interface - `.cppm`, `.ixx`, `.mpp`, `.cxxm`, `.ccm` - every
header - `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, `.inl`, `.ipp`, `.tpp`, `.tcc` - and every Delphi
unit - `.pas`, `.pp` - and no implementation file. The tree's macros are gathered from all of its
C++ before any file is read, and every file is then read as the rules below make of it; a unit
as the Delphi rules after them make of it.

- A header stands as a module of its own, named by its path under the tree - its include
  spelling - and shown by its file name; its includes are its imports. A module interface is
  read as in full.
- What a header declares at namespace scope is public, as an export is in a module: everything
  but what stands in an anonymous namespace.
- Of a conditional, the first branch is read and the rest left out; a branch whose condition is a
  plain `0` gives way to the next. A library declares the same things in every branch as a rule,
  and one branch keeps the braces the branches open balanced.
- An object-like macro the tree defines is expanded where its name stands in code, its body
  expanded in turn; a function-like one is dropped with its arguments, since what it makes of
  them takes a preprocessor to know. The first definition met in a branch read stands, and a name
  defined only in branches left out takes the first of those. `__declspec(...)`,
  `__attribute__((...))`, `_Pragma(...)` and `__pragma(...)` are dropped the same way.
- An inline namespace adds nothing to a name, and neither does `extern "C"`: `fmt::v12::format`
  is `fmt::format`.
- A header included from inside a namespace declares into it, as an include puts the header's
  text where it stands; a header it includes in turn goes with it.
- A using-declaration at a header's namespace scope re-publishes what it names: the declaration
  stands in the using-declaration's namespace, and the namespace it was declared in is the
  library's implementation, shown nowhere - it and every namespace inside it. A declaration
  re-published into the namespace it stands in moves nowhere.
- A function declared again with the same parameters - a declaration and the definition after
  it - is one entry, the commented one. A deduction guide is no function. `typedef X Y;` is an
  alias as `using Y = X;` is.
- A comment is a doc comment however it is written - `//`, `///`, `//!`, `/** */`, `/*! */`, a
  trailing `///<` - and the comment above a declaration comes before the one on its line, which
  is as often a tool's directive as words. The hint is the `@brief` or `\brief` paragraph, or
  else the first sentence of the first paragraph of words; a block opening under other comment
  lines starts a paragraph. A `See` in a comment is words.
- A comment on a line goes on in the comment lines under it that open in its column, and those
  lines are its own, not the comment above the declaration under them. A separator - a run of
  three or more of one mark, `---`, `===`, `***`, `~~~`, `###`, `___`, `+++` - says nothing, and
  a hint that only names its declaration - a label, `// ----- logger -----` - is none.

None of the routine applies: no property, event or control is read, and nothing is checked.

#### Delphi units

A unit is read through its interface section - what it publishes - over the framework's own
Pascal lexer, as a module interface is read over the C++ one. The unit is a module named as the
unit is and shown by its whole name, every part of which tells it from the others; its `uses` are
its imports, and its name with `::` for each dot is the namespace its declarations are named in,
as C++Builder names them: `VirtualTrees.Header` is `VirtualTrees::Header`. Everything the
interface declares is public, a class member under the access its section gives it - `strict`
adding nothing to a page, `automated` reading as public.

- A class, a record, an object, an interface and a dispinterface are types with their members -
  a record is a struct, an interface a kind of its own - an enumeration is an enum, and any other
  type is an alias of what follows its equals sign, a method type's calling convention kept. A
  forward declaration is none; a helper is a class or a struct whose line says what it helps; a
  generic's parameters are its template's. A type nested in a class is `Outer::Inner`.
- A routine's signature is its declaration as written, from its first word - `class` where it
  has one - to the semicolon of its last directive; `virtual`, `dynamic`, `override` and
  `abstract` make it virtual, `class` static. A unit's constants and variables keep their
  declarations as written, under the word of the section they stand in.
- A property is read with its type, its `read` as where the value lands, its `write` as its
  setter, and its `default`. A property declared again with no type - Delphi's way of publishing
  a base's - takes the type, the default and the hint of the nearest base in the tree declaring
  it; one whose base stands outside the tree keeps its type unstated.
- A property of a method type - `procedure ... of object`, `function ... of object`,
  `reference to` - is an event, as the Object Inspector parts them. Where the tree declares no
  such type - the RTL's `TNotifyEvent` - the property is an event when it is named as Delphi
  names one, `On` and a capital.
- A conditional on a symbol neither the tree nor the compiler defines is read false -
  `{$IFDEF FPC}`, `{$IF Defined(VT_FMX)}` - and its `{$ELSE}` read; any other reads its first
  branch, as a C++ conditional does. The tree's symbols are those its units and its include files
  define with `{$DEFINE}` in a branch so read, an `.inc` being read for nothing else - a
  Lazarus-only `{$DEFINE}` under `{$IFDEF FPC}` defines nothing. The compiler's are those the
  newest one, Delphi 13, defines building for Windows, `VER370`, `MSWINDOWS`, `WIN32` and
  `WIN64` among them. An `{$I}` includes nothing.
- A comment is a doc comment however it is written - `//`, `///`, `{ }`, `(* *)` - the comment
  above coming first, as in C++, and read by the same rules of continued lines, separators and
  labels. The hint is an XML doc comment's `<summary>` where it has one, else the first sentence
  of the first paragraph, a line opening with a tag ending a paragraph as a command does;
  `<see cref="X"/>` and `@link(X)` read as their `X`.
- A routine the interface gives no comment takes the hint of the comment standing under its
  header in the implementation section, between the header and the `var`, `const` or `begin` the
  routine opens with. The header is matched to the routine by its name and the types it is a
  member of - `TOuter.TInner.Name` - read in any case, and where the name is overloaded, by how
  each parameter is passed and its type. A `forward` or an `external` routine has no body, and
  the lines under its header are not its own.

## The database

`Surface.cfg` is a document in the framework's own configuration format, written through the
Dom with the layout `SeeDocs_App::databaseLayout()` states, and read back through the same
layout by `readDatabase`, which answers the surface the scanner would have filled. An entry
states only what it has - a key at its default is left out - so a reader applies the layout and
takes the rest from the file.

Where an entry says the same thing a `Language.cfg` completion entry says, it uses the same key:
`Name`, `Kind`, `Signature`, `Hint`, `Type`, and a class's `Methods` and `Properties` as two lists.
The two files are meant to become one.

    Scanned = 2026-10-09T03:12:45Z
    Limited = true
    Modules = [
        =[ Name, File, Interface, Header, Language, Imports = [...], ExportedImports = [...] ]
    ]
    Types = [
        =[
            Name, Namespace, Kind (class, struct, union, enum, type, concept, interface), Module,
            File, Line,
            Hint, Note, Template, Bases = [...], Target, Control, Category,
            Properties = [ =[ Name, Type, Accepts = [...], Default, Setter, Target, Form,
                              ValueKind, Optional, Access, Line, Hint, Note ] ]
            Events = [ =[ Name, Alias, Method, Access, Line, Hint, Note ] ]
            Methods = [ =[ Name, Kind (function, constructor, destructor), Signature, Type,
                           Template, Static, Virtual, Deleted, Access, Line, Hint, Note ] ]
            Usings = [ =[ Name, Base, Access, Line, Hint, Note ] ]
            Fields = [ =[ Name, Type, Value, Static, Constant, Access, Line, Hint, Note ] ]
            Members = [ =[ Name, Value, Line, Hint, Note ] ]
        ]
    ]
    Functions = [ =[ Name, Namespace, Kind, Signature, Type, Template, Module, File, Line, Hint, Note ] ]
    Constants = [ =[ Name, Namespace, Type, Value, Constant, Declaration, Module, File, Line, Hint,
                     Note ] ]

`Scanned` is when the scan that wrote the file ran, in UTC to the second, and `Limited` that it
read the tree in limited mode. A hint or a footnote
written from SeeDocs keeps it, since such an edit reads nothing of the tree beyond what it
changes. `Header` marks a module that is a header, as a limited read takes one; its `Name` is
its path under the tree. `Language` is `pascal` for a Delphi unit, whose code a page reads and
colours as Pascal, and a C++ module states none; a Delphi interface's `Kind` is `interface`, and
a Delphi member's `Access` may be `published`.

`Control` marks a type whose base chain reaches `Control` or `RichControl`, template arguments
walked as bases, which is what makes it a toolbox entry; `Category` is its folder under the
tree. `Bases` are written as a reader can follow them: an alias the database does not
carry - a private `using ComboBoxBaseClass = WithInPlaceEdit<DropdownControlBase>` - is replaced
by what it names, and a base that is one of the type's own template parameters - a mixin's
`Base` - stays as spelled. A property's `Form` says how its line was written (`declared`, `writable`,
`reference`, `storage`, `member`, `call`, `value`, `action`, `read`, or `required` - a
`REQUIRE_PROPERTY`, which the pack has to carry and which therefore has no `Default`) and
`ValueKind` what a designer shows the value as: `bool`, `int`, `float`, `text`, `float2`,
`int2`, `float4`, `int4`, `color`, `enum`, `number`, `object`, `handler` (a `std::function`,
wired the way an event is) or `unknown`. A `std::optional` of a kind is that kind with
`Optional` set. Several bindings landing on one target are one property, the further types
under `Accepts`. An event's `Name` is its struct, whose payload is that struct's own `Fields`
entry. A `Usings` entry is a using-declaration naming a base's member, `Base` as spelled before
the last `::`: `Base::Base` makes the base's constructors the type's own, any other name
republishes that member under the entry's `Access`. A constant's `Value` is its initializer
folded onto one line, comments taken out; its `Declaration` is the whole declaration as written,
from its first specifier to the semicolon, lines, indents and comments kept, each line after the
first set back by the first one's indent.

## The window

`SeeDocs` opens a project - the tree named on its command line, else the project it was
last left on - and shows the project's database and its footnotes as pages. The
application menu's Open page is where projects are opened: a Browse button asks for a folder in
the desktop's own dialog, and the projects opened before stand under it, most recent first, one
click each. The project open heads the page: its name in a box that renames it - Return or leaving
the box keeps the name, Escape puts it back, and an emptied box gives the project its folder's
name - then its folder, and how long ago it was scanned, with a Rescan button that reads the tree
again while the menu stays open, and the Limited mode box, which reads it again in the mode it
is switched to. A folder with no `.seedocs` becomes a project there, once the user has agreed to
the folder and said whether it is a third-party library; the project's name heads the window
title. Which project is open
and which were opened before are kept with the application's settings, so they are remembered
once settings are kept on the computer.

SeeDocs is a browser of the framework's own kind: tabs across the top, under them the crumbs of
the page shown with a list dropping from each, and Back, Forward and Up beside them. Every page has
a url - `/chapter/module/type/method` - so a tab goes back and forward through the pages it has
visited (Alt+Left and Alt+Right, the side buttons of the mouse, or a right click on either button
for the list), a type's crumb lists its methods, and Up climbs the path. A home page opens the
project: its chapters, with what each holds. Modules standing in the tree's own folder are home's:
they stand at the top of the tree under its chapters, and the home page carries what they export,
by module, as a chapter's page does. Where a tab stands, the tabs open and each one's
history are kept with the settings, so a run starts where the last one left off. Opening another
project closes every tab and opens one on the new project's home; a url the project's database no
longer carries - a type gone since the tree was scanned - shows a page saying so.

The pages: down the left a tree of chapters - the tree's folders, a reader's names for them
- each holding its modules and each module the types it exports, controls first; a module that
exports functions or constants and no type is a row with nothing under it. On the right the
page of whatever is clicked, and the tree follows wherever a tab goes. A page is a column of
controls: its title, what the page is after it - a chapter, a module, a function, a constant,
a type's kind - and its hint under it,
a grid of facts - namespace, module, source - and then a section per expander. A type's title
carries the ways up through its bases under it, written the C++ way - a colon before each base,
a mixin's host through the argument it was given, a second line where the bases fork; an
alias's line is what it stands for after an equals sign, an enum's its underlying type after a
colon. A chapter's or a module's page holds a grid of its types with their kinds and hints, then its
functions and its constants; on a chapter's page each grid stands by module, the module's name
beside its rows and linked to its page. A function's row carries its name alone, one row per
name, every overload's signature in the name's hint; the name links to the function's page,
which carries the hint under its title - the first commented overload's, as the row does - and
holds every overload of the name, coloured, with its note and, where it differs, its own hint. A
constant's name links to the constant's page, which carries the hint under its title and holds
its declaration as written, coloured and whole, with its note; the grid's value cell shows the
first lines of the folded initializer. A module's page lists its types, its functions and its
constants in its crumb, and their pages stand under its url. A type's page holds the types
derived from it as a tree and a grid per member kind: properties, events,
public methods and protected methods, the type's own under an Own expander and each base's under
a From expander of its own, up the whole chain of bases; fields or members; each with its hint.
A base's methods grid holds what reaches the type: its
constructors only where the type takes them with `using Base::Base`, never its destructor, no
method whose name a type below declares again - C++'s own hiding, by name, which a
`using Base::name` undoes - and a republished method under the access that using-declaration
gives it. A property's getter and setter stand in no methods grid, since the property's row
says it; a method with no comment of its own - an override, as a rule - takes the hint and the
footnote of the method of the same name and parameter count in the nearest base that has one.
A method's row carries its name alone, one row per name, with every overload's signature as
the name's hint, a line each; the name is a
link to the method's page, titled `Type::name` with the type linked. It is built as a function's
page is, the type and the access labels being what a method adds: the hint under the title - the
first commented overload's - then every overload of the name under its access label written the
C++ way - `public:` - the declaration as code, coloured as the framework's code boxes colour C++,
its own hint where the title's is another overload's, and the note the comment references.
After them, the types with a property of this type and
the types with an event of it, two trees, each type with the types derived from it under it,
closed; then, as the page's footnote headed Details, the note the type's comment references as
prose, its file and section in the heading's hint; then
the types beside it in its module. A property, an event, a field or a member
whose comment references a note carries a mark in a column of its own, and the note stands as a
footnote under the grid; the mark leads to it, as an anchor of the page's url. A section's
heading says what the section is; how many elements it holds is the heading's hint.
A type whose comment references nothing takes the section named exactly after it from a note of
its own folder, marked as matched by name in that hint. Where the surface states no hint the page says so, in
place of the words, so what the documentation still lacks is read off the preview. A tree read in
limited mode owes no hints, and its pages leave a missing one empty.

A Delphi type's page is spelled as a C++ type's is - its title, its bases after colons, the
access labels written `public:` - and its code is Delphi's own: a routine's signature and a
constant's or a variable's declaration stand as written, coloured as Pascal. Its properties and
its events stand in a grid per access - published, public, protected - and a base's member that
a type below declares again is the type's own, counted once. A constructor reaches the type from
its bases as a method does, and a type below declaring the name hides it as it hides a method.

Hints and footnotes are edited where they stand, in a project read in full - a project in limited
mode shows them and writes nothing. A hint cell opens its editor with F2, Enter or a
double click, on the comment's own words - its types unlinked, a missing hint empty. The hint under
a page's title and the one under each declaration of a method page are typed into where they stand,
a missing one an empty box: Ctrl+click follows a link there, and Escape puts back the words the page
shows. Enter writes the words into the comment the hint is read from - leaving one of those boxes
writes them too - the `See` it ends with kept: on the declaration's line while the whole line fits
100 columns, lined up with the comments of the lines of code round it, and above it otherwise -
always above a class, a struct or an enum. A method's row writes the overload whose words it shows,
and an override that shows its base's words writes the base's comment. A footnote opens its markdown
in its place with F2 or Edit footnote on its menu: each paragraph and each bullet on one line, the
headings inside it and its code as written. Ctrl+S or Save writes it into its note - a paragraph or
a bullet that reads as it did keeps its lines, any other is broken at 100 columns again - and Escape
or Cancel leaves it. The row of a property, an event, a field or a member with no footnote offers
Add footnote on its menu: the footnote goes after the type's own section in its note, or at the end
of the chapter's note, headed by the member's name, and the comment gains the `See` that reaches it.
Every write reads its file again first and refuses, saying why, where the file no longer holds what
the page was built from - a file changed in an editor since the scan is scanned again before it is
edited here. A file keeps its byte order mark and each line its own line end. The words are written
by `SeeDocs_App.Edits`, the database follows every write, and the page shown takes the new words
where it stands; a footnote added builds the page again at the new footnote.

A type's name on a page is a link to its page, in a grid cell as in prose: a type named in a
hint, in a note's prose or code, or in a signature - bare or qualified, as `Control` or
`Grids::Row` - links wherever the surface has a page for it. A right click on a link, or the menu
key on a grid cell holding one, offers Open and Open in new tab. A row of the
derived types opens the type it names; a method's page picks the method's type in the tree.
A page's trees start closed, and a node's rows are built the first time it opens.
The pages are built by
`SeeDocs_App.Pages` - facts, sections, tables of rows with their footnotes, branches of a tree,
signatures and blocks of prose - out of the surface and the notes; SeeDocs builds controls
from them (`SeeDocs_App.PageView`), the browser (`SeeDocs_App.Browser`) shows one per tab, and a generator renders the same pages as files.

## The checks

`check` holds a tree read in full to the routine, and a project in limited mode to nothing:

- a property with no `INIT_PROPERTY`; a `..._STORAGE` property without its getter and a
  `WRITABLE` one whose setter is not there, the base chain searched as well
- a `Props::get` or `Props::find` written by hand rather than through `READ_PROPERTY` or
  `REQUIRE_PROPERTY`
- a comment of more than one line, one in both places at once, or none on a declaration of the
  design surface - a control, its properties and events, an exported enum
- a comment above a declaration it would fit on, and one riding a line past 100 columns
- an event whose three spellings disagree, or whose struct is not found
- a property whose type is neither an enum, a known value kind, nor a declared type
- a `See` reference to a footnote that does not exist under the tree's `.seedocs/footnotes`, or
  to a heading the note does not have - on every line of a comment
- a class declaring properties or events whose base chain reaches neither Control nor a type
  the scanner can read
- a value type declared under `Controls/Base`, where it reaches an application only through an
  import of the base

## Building

`SeeDocs.sln` builds both programs with Visual Studio against the shared items: `SeeDocs` the window
and `SeeDocsCli` the scanner, which share the Surface, Reading, Scanner, PascalReader, Database,
Project, Notes and Pages modules under `Src/`, the window's own standing under `Src/Ui/` and the
Edits module beside the shared ones. The CMake target `SeeDocs` builds the window on Wayland beside
the other applications. The CMake target `SeeDocsCli` builds the scanner on Linux with or without a
platform layer - the Dom, the Syntax lexer and the text file reader reach no further down than
`UiTypes` - so a `check` runs wherever the tree is checked out:

    cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=none \
          -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target SeeDocsCli
    ./build/SeeDocsCli check Source
