# SeeDocs

SeeDocs is a set of tools that read a code base's design surface out of its sources and put it to
use. There are two programs so far:

- **SeeDocsCli**, the scanner. It reads every module in a project's source folders, writes what it
  finds into one database, and holds the sources to the declaration routine.
- **SeeDocs**, the window. It shows that database as the reference it is to become - the generated
  part of the documentation. The rest is written by hand.

```
SeeDocsCli scan [folder] [--out file] [--time]  writes the database, the project's Surface.cfg
SeeDocsCli check [folder]                       reports deviations, exit code 1 on errors
SeeDocs [folder]                                opens the project the folder stands for
```

- **The folder** is a project's own, the one its `Project.cfg` stands in. For the framework, that
  is `Source/.seedocs`. SeeDocsCli takes the working directory when the folder is left out.
- **A scan with `--out`** takes any folder, read as the one source folder of a project of ClaFi
  modules.
- **`--time`** prints how long each step of the scan took, then the write of the database and the
  read of it that SeeDocs shows a scan from.
- **The report** reads `file:line: level: text`, one line per finding, in file and line order.

## Two ways to read

A project reads its sources in one of two ways:

- **As ClaFi modules.** The sources are written to the declaration routine. Their modules are read,
  the routine's macros harvested, and the checks hold them to the routine.
- **As any C++ and Delphi.** This is for anyone else's code, such as a third-party library. Headers
  and units are read as well as modules, for what they declare, and nothing is checked.

Two more choices stand apart from that: how a comment gives a declaration its hint (see
[Hints](#hints)), and whether a hint edited in SeeDocs is written back into the sources.

## Projects

A project is a folder of its own, kept wherever the user chooses. SeeDocs offers
`Documents/SeeDocs`. The folder holds everything SeeDocs keeps of the project:

- `Project.cfg` - the project's properties, its sidebar among them
- `Surface.cfg` - the database the scanner writes
- `Scan.cache` - what the last scan read of each file - see [Scan cache](#scan-cache)
- `footnotes` - the notes the pages show beside the hints

The sources stay where they are, in any number of folders, and are read in place.

```
Name = ClaFi
Sources = [
    =[ Path = ../ClaFi/Source, Subfolders = true ]
]
ReadAs = ClaFiModules
LooseHints = false
EditHints = true
Sidebar = [
    =[ Kind = AllTheApis, Name = All the APIs, Subgroup = false, Items = [ ] ]
    =[ Kind = Divider, Name = , Subgroup = false, Items = [ ] ]
    =[ Kind = Category, Name = Guides, Subgroup = false, Items = [
        =[ Kind = Content, Name = Quick start, Subgroup = false, Items = [ ] ]
    ] ]
]
```

- **`Name`** is what the project goes by. Left empty, the project goes by its folder's name.
- **`Sources`** are the folders it reads, each with or without the folders under it.
- **`ReadAs`** is how the sources are read: `AnyCppAndDelphi` for anyone's C++ and Delphi, the
  default, or `ClaFiModules`.
- **`LooseHints`** takes the hint of every recognizable comment instead of following the strict
  rules - see [Hints](#hints).
- **`EditHints`** writes a hint edited on a page into its source.
- **`Sidebar`** is what the project's tree and home list - see [The sidebar](#the-sidebar): an
  entry per item in reading order, a category's items under it as `Items`, as deep as they go.
  `Subgroup` says whether an All the APIs item puts the folders under a node of its own; an item
  that does not state it has none. A sidebar listing nothing reads as the default, one All the
  APIs item without a subgroup.

A project the window makes is written with `ReadAs = AnyCppAndDelphi` and `LooseHints`. A project
stating neither reads any C++ and Delphi by the strict rules.

### Source paths

A path stands as it was typed, and is read as a folder when the sources are read.

- A relative path is read from the project's folder.
- A path starting with a separator is read from the root of the project's volume.
- A backslash separates folders on every desktop, so a path typed on Windows names the same folder
  on Linux.
- A folder picked rather than typed is written relative to the project's folder while the two share
  a folder under the root of their volume. They then move together, and a project kept in a
  repository names no place outside it. Otherwise the folder is written as it stands.
- Folders of one name under two sources are one folder.
- A file two sources reach is read once, as the first of them found it.

### A project inside its sources

A project's folder may stand inside the tree it reads, as the framework's does in
`Source/.seedocs`. It names the tree as `..`, the way any project names a folder, and the dot that
starts its name keeps it out of the scan. Commands name the project's folder itself:
`SeeDocsCli check Source/.seedocs` checks the framework.

## How it reads

### ClaFi modules

Every `.cppm` and then every `.cpp` in the source folders is lexed with the framework's own C++
lexer (`ClaFi.Core.Syntax`) and read declaration by declaration over the tokens.

- **Dot folders are skipped.** A folder whose name starts with a dot - `.git`, `.vs` - is skipped
  with everything under it.
- **Macros are read, not expanded.** Nothing is compiled. A macro of the declaration routine -
  `DECLARE_PROPERTY`, `DECLARE_EVENT`, `BIND_PROPERTY_*`, `READ_PROPERTY`, `REQUIRE_PROPERTY` - is
  read as the declaration it stands for.
- **Function bodies are skipped**, read only for three things: the binds a constructor writes
  (`INIT_PROPERTY`, `BIND_MEMBER` and its kind), a `Props::get` written by hand, and what a
  constructor hands its bases.

### What is harvested

- Every exported type - class, struct, union, enum, alias, concept - with its public and protected
  members: properties, events, functions, data members, enum members.
- Every exported free function and constant.
- Every module, with its imports.

A nested type is an entry of its own, named `Outer::Inner`. Types are keyed by qualified name, so
`Grids::Grid` and `Grids::Dt::Grid` are two entries.

### Names

**A name is read where it is written**, and looked up the way C++ looks it up. A type's members,
its hint and its footnote are read inside the type. Its bases, and what an alias or an enum's base
names, are read in the scope the type is declared in.

- **A bare name** is looked for in that scope, then in each scope around it out to the global one.
  A type's scope holds the types nested in it and the ones nested in its bases. A mixin's base is
  the argument its spelling hands it.
- **A type nested in another is named with its outer type everywhere else** -
  `ShadowPainter::Design`. A bare `Design` in another type's hint names nothing.
- **Where no scope holds a bare name**, it names the type of that name declared at a namespace's
  scope, the one nearest.
- **A qualified name** is matched by its tail: `Grids::Row` names `ClaFi::Controls::Grids::Row`.
  One that opens with `::` is looked for from the global scope.

### Restated properties

**A constructor restates a base's property by handing the base a value for it.** A pack reads the
last value it holds of a type, which gives two cases:

- **A value before the pack it forwards** - `Button{ params, Interactivity::Focusable,
  std::forward<Args>(args)... }` - is the default the type gives that property. The caller's own
  value overrides it.
- **A value after the pack** is fixed: the caller's value loses to it. Every value a constructor
  taking no pack hands on is fixed the same way.

The scanner reads each initializer naming a base of the type, argument by argument. It keeps the
arguments whose type it can tell from their spelling:

- A construction, `Padding{ 4.0f }`, is of the type it opens with.
- A qualified name alone, `Interactivity::Focusable`, is an enum's member or a class's constant.
- A bare name alone is a constant the file or the tree declares, of its declared type.

The caller's own values - `params`, any parameter of the constructor - are passed on, not restated.
An expression such as `params.themeMetrics().toolButton` cannot be typed without compiling, so it is
left out.

### Hints

A declaration's hint is read by one of two sets of rules, whichever way the sources are read.

**The strict ClaFi rules**

- A declaration's comment is the `//` comment on the declaration's own line, or else the one
  touching it from above.
- A class template's comment may sit between the `template` line and the `class` line, or above the
  `template` line.
- The comment's first line is the hint the database carries. A trailing `See <note>#<anchor>`
  reference is cut off into the entry's `Note`.
- A declaration with no comment owes one, and its pages say so.

**Every recognizable comment**

- Any comment is a doc comment, however it is written: `//`, `///`, `//!`, `/** */`, `/*! */`, a
  trailing `///<`.
- The comment above a declaration comes before the one on its line, which is as often a tool's
  directive as words.
- The hint is the `@brief` or `\brief` paragraph, or else the first sentence of the first paragraph
  of words. A block opening under other comment lines starts a paragraph.
- A `See` in a comment is plain words.
- A comment on a line goes on in the comment lines under it that open in its column. Those lines are
  its own, not a comment above the declaration under them.
- A separator says nothing: a run of three or more of one mark - `---`, `===`, `***`, `~~~`, `###`,
  `___`, `+++`.
- A hint that only names its declaration is none - a label such as `// ----- logger -----`.
- A declaration owes no hint.

### Any C++

A read of any C++ and Delphi takes these files, and no implementation file:

- module interfaces - `.cppm`, `.ixx`, `.mpp`, `.cxxm`, `.ccm`
- headers - `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, `.inl`, `.ipp`, `.tpp`, `.tcc`
- Delphi units - `.pas`, `.pp`

The sources' macros are gathered from all of their C++ before any file is read. Each C++ file is
then read by the rules below, and each unit by the Delphi rules after them. The declaration routine
stays with ClaFi modules: no property, event or control is read, and nothing is checked.

- **A header is a module of its own**, named by its path under its folder - its include spelling -
  and shown by its file name. Its includes are its imports. A module interface is read as a ClaFi
  module is.
- **A header's namespace scope is public**, as an export is in a module - everything but what stands
  in an anonymous namespace.
- **The first branch of a conditional is read** and the rest left out. A branch whose condition is a
  plain `0` gives way to the next. A library declares the same things in every branch as a rule, and
  one branch keeps the braces the branches open balanced.
- **An object-like macro the tree defines is expanded** where its name stands in code, its body
  expanded in turn. A function-like macro is dropped with its arguments, since what it makes of them
  takes a preprocessor to know. The first definition met in a branch read stands, and a name defined
  only in branches left out takes the first of those. `__declspec(...)`, `__attribute__((...))`,
  `_Pragma(...)` and `__pragma(...)` are dropped the same way.
- **An inline namespace adds nothing to a name**, and neither does `extern "C"`: `fmt::v12::format`
  is `fmt::format`.
- **A header included inside a namespace declares into it**, as an include puts the header's text
  where it stands. A header it includes in turn goes with it.
- **A using-declaration at a header's namespace scope re-publishes what it names.** The declaration
  stands in the using-declaration's namespace. The namespace it was declared in is the library's
  implementation and is shown nowhere, with every namespace inside it. A declaration re-published
  into its own namespace moves nowhere.
- **A function declared twice with the same parameters is one entry** - a declaration and the
  definition after it - and the commented one stands. A deduction guide is no function. `typedef X
  Y;` is an alias, as `using Y = X;` is.

### Delphi units

A unit is read through its interface section - what it publishes - over the framework's own Pascal
lexer, as a module interface is read over the C++ one.

- **A unit is a module**, named and shown by its whole name. Its `uses` are its imports. Units of
  one name in several folders are namesakes, told apart by their folders - see
  [Namesakes](#namesakes).
- **Its namespace is its name with `::` for each dot**, as C++Builder names them:
  `VirtualTrees.Header` is `VirtualTrees::Header`.
- **Everything the interface declares is public.** A class member takes the access its section gives
  it. `strict` adds nothing to a page, and `automated` reads as public.
- **Types.** A class, a record, an object, an interface and a dispinterface are types with their
  members. A record is a struct, and an interface a kind of its own. An enumeration is an enum. Any
  other type is an alias of what follows its equals sign, a method type's calling convention kept. A
  forward declaration is skipped. A helper is a class or a struct whose line says what it helps. A
  generic's parameters are its template's. A type nested in a class is `Outer::Inner`.
- **Routines.** A routine's signature is its declaration as written, from its first word - `class`
  where it has one - to the semicolon of its last directive. `virtual`, `dynamic`, `override` and
  `abstract` make it virtual, and `class` makes it static.
- **Constants and variables** keep their declarations as written, under the word of the section they
  stand in.
- **Properties.** A property is read with its type, its `read` as where the value lands, its `write`
  as its setter, and its `default`. A property declared again with no type - Delphi's way of
  publishing a base's - takes the type, the default and the hint of the nearest base in the tree
  declaring it. Where that base stands outside the tree, the type stays unstated.
- **Events.** A property of a method type - `procedure ... of object`, `function ... of object`,
  `reference to` - is an event, as the Object Inspector parts them. Where the tree declares no such
  type, as with the RTL's `TNotifyEvent`, a property named as Delphi names an event - `On` and a
  capital - is an event.
- **Controls.** A class is a control when its base chain reaches `TControl`, in any case. A chain
  leaving the tree ends at a class of the libraries Delphi code builds on, and SeeDocs knows which
  of their classes descend from `TControl` - `TCustomControl`, `TCustomEdit` and `THintWindow` among
  them.
  - A unit that uses an `FMX.` unit means FMX's classes. Any other means the VCL's, with the LCL's
    under a name the VCL leaves free. So `TMenuItem` is a control in a FireMonkey unit and a menu's
    item in a VCL one, and `TForm` the other way round.
  - The names come from Embarcadero's compiler output and class reference, and from the LCL's
    sources.
  - A tree holding the libraries' own sources is walked through them to their `TControl`.
- **Conditionals.** A conditional on a symbol neither the tree nor the compiler defines is read
  false - `{$IFDEF FPC}`, `{$IF Defined(VT_FMX)}` - and its `{$ELSE}` is read. Any other conditional
  reads its first branch, as a C++ one does.
  - The tree's symbols are those its units and include files define with `{$DEFINE}` in a branch
    read this way - a Lazarus-only `{$DEFINE}` under `{$IFDEF FPC}` defines nothing. An `.inc` is
    read for nothing else.
  - The compiler's symbols are those the newest compiler, Delphi 13, defines building for Windows -
    `VER370`, `MSWINDOWS`, `WIN32` and `WIN64` among them.
  - An `{$I}` includes nothing.
- **Comments** are any of `//`, `///`, `{ }`, `(* *)`.
  - By the strict rules, a unit's hints are read as a C++ file's are.
  - By every recognizable comment, the comment above comes first, as in C++, with the same rules for
    continued lines, separators and labels. The hint is an XML doc comment's `<summary>` where it
    has one, else the first sentence of the first paragraph. A line opening with a tag ends a
    paragraph, as a command does. `<see cref="X"/>` and `@link(X)` read as their `X`.
  - By every recognizable comment, a routine the interface gives no comment takes the comment
    standing under its header in the implementation section, between the header and the `var`,
    `const` or `begin` the routine opens with. The header is matched to the routine by its name and
    the types it is a member of - `TOuter.TInner.Name` - in any case. Where the name is overloaded,
    it is matched by how each parameter is passed and its type. A `forward` or an `external` routine
    has no body, and the lines under its header are not its own.

## The database

`Surface.cfg` is a document in the framework's own configuration format.

- It is built through the Dom, with the layout `SeeDocs_App::databaseLayout()` states.
- `Database` writes it, and also answers the surface the document states. SeeDocs shows a scan from
  that answer, with no read of the file.
- `readDatabase` reads the file through the same layout and the same reader.
- An entry states only what it has. A key at its default is left out: the reader answers the default
  itself and takes the rest from the document.

**Keys are shared with `Language.cfg`.** Where an entry says what a `Language.cfg` completion entry
says, it uses the same key: `Name`, `Kind`, `Signature`, `Hint`, `Type`, and a class's `Methods` and
`Properties` as two lists. The two files are meant to become one.

```
Scanned = 2026-10-09T03:12:45Z
ReadAs = ClaFiModules
LooseHints = true
Modules = [
    =[ Name, File, Root, Interface, Header, Language, Imports = [...], ExportedImports = [...] ]
]
Types = [
    =[
        Name, Namespace, Kind (class, struct, union, enum, type, concept, interface), Module,
        File, Root, Line,
        Hint, Note, Template, Bases = [...], Target, Scoped, Control, Folder,
        Properties = [ =[ Name, Type, Accepts = [...], Default, Setter, Target, Form,
                          ValueKind, Optional, Access, Line, Hint, Note ] ]
        Events = [ =[ Name, Alias, Method, Access, Line, Hint, Note ] ]
        Methods = [ =[ Name, Kind (function, constructor, destructor), Signature, Type,
                       Template, Static, Virtual, Deleted, Access, Line, Hint, Note ] ]
        Usings = [ =[ Name, Base, Access, Line, Hint, Note ] ]
        Fields = [ =[ Name, Type, Value, Static, Constant, Access, Line, Hint, Note ] ]
        Members = [ =[ Name, Value, Line, Hint, Note ] ]
        Restates = [ =[ Type, Value, Fixed, Line ] ]
    ]
]
Functions = [ =[ Name, Namespace, Kind, Signature, Type, Template, Module, File, Root, Line,
                 Hint, Note ] ]
Constants = [ =[ Name, Namespace, Type, Value, Constant, Declaration, Module, File, Root, Line,
                 Hint, Note ] ]
```

### The file and its modules

- **`Scanned`** is when the scan that wrote the file ran, in UTC to the second.
- **`ReadAs`** says how the scan read the sources, left out for any C++ and Delphi, and
  **`LooseHints`** that it took every recognizable comment's hint. A hint written from SeeDocs keeps
  both, since such an edit reads nothing of the sources beyond what it changes.
- **`File`** is the path under its source folder, and **`Root`** that folder, counted from zero in
  the order the project names them.
- **`Header`** marks a module that is a header, as a read of any C++ takes one. Its `Name` is its
  path under its source folder.
- **`Language`** is `pascal` for a Delphi unit, whose code a page reads and colours as Pascal. A C++
  module states none.

### Types

- A Delphi interface's **`Kind`** is `interface`, and a Delphi member's **`Access`** may be
  `published`.
- **`Scoped`** marks an enum declared `enum class` or `enum struct`.
- **`Control`** marks a type whose base chain reaches `Control`, `RichControl` or `TControl`,
  template arguments walked as bases. That is what makes it a toolbox entry.
- **`Folder`** is the type's folder under its source folder.
- **`Bases`** are written so that a reader can follow them. An alias the database does not carry - a
  private `using ComboBoxBaseClass = WithInPlaceEdit<DropdownControlBase>` - is replaced by what it
  names. A base that is one of the type's own template parameters - a mixin's `Base` - stays as
  spelled.

### Members

- A property's **`Form`** says how its line was written: `declared`, `writable`, `reference`,
  `storage`, `member`, `call`, `value`, `action`, `read`, or `required`. A `required` property is a
  `REQUIRE_PROPERTY`, which the pack has to carry, so it has no `Default`.
- **`ValueKind`** is what a designer shows the value as: `bool`, `int`, `float`, `text`, `float2`,
  `int2`, `float4`, `int4`, `color`, `enum`, `number`, `object`, `handler` (a `std::function`, wired
  the way an event is) or `unknown`. A `std::optional` of a kind is that kind with **`Optional`**
  set.
- Several bindings landing on one target are one property, with the further types under
  **`Accepts`**.
- An event's **`Name`** is its struct. The event's payload is that struct's own `Fields` entry.
- A **`Usings`** entry is a using-declaration naming a base's member, with **`Base`** as spelled
  before the last `::`. `Base::Base` makes the base's constructors the type's own. Any other name
  republishes that member under the entry's `Access`.
- A **`Restates`** entry is a value the type's constructor hands a base, as [Restated
  properties](#restated-properties) describes. **`Type`** is the property type it is of, **`Value`**
  is as spelled and folded onto one line, and **`Fixed`** marks a value standing after the pack.
- A reader applies a `Restates` entry to the base's property of that type, the further spellings
  under `Accepts` counted, by the pack's own rule. The nearest type's value wins, unless a fixed
  value stands anywhere on the chain - then the furthest fixed one wins.

### Constants

- A constant's **`Value`** is its initializer folded onto one line, comments taken out.
- Its **`Declaration`** is the whole declaration as written, from its first specifier to the
  semicolon, with lines, indents and comments kept. Each line after the first is set back by the
  first one's indent.

## Scan cache

**A scan reads only the files that changed since the last one.** SeeDocs keeps what each file gave
the last scan in the project's `Scan.cache`, and takes a file from there while its size and last
write are the ones the folder listing gave then. New and changed files are read, and a file gone
from the folders is dropped. The passes over the whole tree and the database are built as they
always are, and the database comes out as it does from a scan that reads every file.

A file the cache holds is read again where what it was read by has changed:

- **A class another file declares.** A definition such as `Button::Button()` is read against its
  class, and a constructor's initializers against the class's bases. Each file keeps the classes it
  found that way. Where a name now finds another class, or the class has other bases, the file is
  read again.
- **The tree's macros**, in a read of any C++. Each file keeps the macros it defines. Where the
  tree's macros come out different, every C++ file is read again.
- **The Delphi symbols.** Where a file holding a `{$DEFINE}` changed, every such file is gathered
  from again, and where the symbols come out different, every unit is read again.

**A cache serves one build of SeeDocs and one set of scan options.** One that another build wrote,
or that was written under other scan options, is put aside, and every file is read. The file is
binary and holds the last-write times of the PC it was written on, so a project kept in a
repository leaves it out - the next scan makes it again. SeeDocsCli reads every file and leaves the
cache as it is.

## Saving

**The window writes the database and the scan cache on a thread of their own**, so a scan's pages
show as soon as the scan is read. The pages come off the database built in memory, and the files
follow it to disk:

- **One write at a time**, in the order asked. A write still waiting gives way to a newer one of the
  same file.
- **Each file is written beside itself**, as `Surface.cfg.writing`, and renamed over the old one
  once whole, so a write cut short leaves the file as it was.
- **Reads wait for the writes**: opening a project, and the scan that reads the cache. Closing
  SeeDocs waits for them too, once the window is down.
- **A database that could not be written** puts up a notice naming the file, with Try again. The
  pages keep the last scan, and the file holds the one before it until a write succeeds. A scan
  cache that could not be written costs the next scan its speed, and says nothing.

A hint edited on a page is saved the same way. SeeDocsCli writes the database itself, before it
ends.

## The window

`SeeDocs` opens projects, as many at once as are asked for: the ones left open last time, and the
one its command line's folder stands for. It shows each project's database and footnotes as pages.

**The theme's spot marks what stands apart from the interface** - the mark of a project and of the
root that lists them, a path, the letters a search has typed, the types in a declaration and a
footnote's mark. The accent marks what answers to the interface: a link, the focus, a selection.

### The application menu

**New project** opens a window of its own over the main one.

- It asks for the project's name and where to store it, and shows the folder the two make.
- Create makes the folder and opens the project on its options page.
- Cancel, Escape or the title's close button lets it go.
- The window is moved by its title and stays up until it is answered. Asked for again while it
  stands, it comes forward as it was left.

**Open** holds Browse and the projects opened before.

- Browse asks for a project's folder, or a tree that keeps one, in the desktop's own dialog.
- The projects opened before are listed most recent first, one mouse click each.
- A project opens in a new tab on its home. A project open already gets a new tab on its home too.
- A folder that stands for no project is offered a project that reads it, made through New project
  with the folder as its first source.

The window title names the project the shown tab stands in. The open projects and the ones opened
before are kept with the application's settings, so they are remembered once settings are kept on
the computer.

### External editors

The menu's Settings page holds External editors under the framework's own options.

- **A command line each** for C++ and Delphi sources. `{file}` stands for a source file and `{line}`
  for a line in it. The command lines are kept with the application's settings as they are typed.
- **Suggestions.** The chevron at a box's end, or Down in the box, drops the editors most used for
  that language on the desktop SeeDocs runs on, each with its command as it is typed in a terminal.
- **Narrowing.** Typing at the box's end narrows the list to the editors with a word of their name,
  or their command, starting with what is typed. An editor picked puts its command in the box's
  place.
- **Default app**, on Windows, is the file alone, opened as Explorer opens it with the app set for
  its type. A RAD Studio already running takes a Delphi source this way.
- **Default app and Lazarus** reach no line, so they are marked no line and open the file alone.
- **Visual Studio** goes to the line when its command starts it. One already running opens the file
  alone, since `/command` is run by the instance it starts.

### The project's node and its own pages

Each open project is a node of the tree, named after the project, and the node opens its home. Under
it stand the items of the project's sidebar.

The home's head holds Options and Edit sidebar, which open the project's options page and its
sidebar page - see [The sidebar](#the-sidebar).

The options page is titled Project options. Under the title stand:

- **The project's name**, in heading type, in a box that renames it. Return or leaving the box keeps
  the name, Escape puts it back, and an emptied box gives the project its folder's name.
- **The project's folder.**
- **How long ago it was scanned**, with a Rescan button that reads the sources again.

The two pages' urls are the project's with `/.project` and `/.sidebar` after it. The scanner passes
over names that start with a dot, so no folder can take them.

### The sidebar

**The sidebar is what a project's tree and its home list**: items the user arranges, of four
kinds.

- **A category** holds items of its own. Its page lists them, and its node in the tree opens on
  them.
- **A divider** is a line between the items either side of it, in the tree and on the page that
  lists them, where it parts the list in two. It has no name and no page.
- **All the APIs** holds the folders the sources read, as the scan lists them, and the root's own
  modules after them. **Without a subgroup**, the default, they stand in the item's place: in the
  tree under the node the item stands in, and on the page that lists the item - the home or a
  category's - with what each folder holds and why there is nothing to document where there is
  none. **In a subgroup** they stand under a node of the item's own, whose page says the same.
- **Content** is a page of the project's own, with a name and, so far, nothing under its head.

A new project's sidebar holds one All the APIs item, without a subgroup. **The folders stand where
the sidebar's first All the APIs item puts them** - in its place, or under it in a subgroup - every
url of theirs starting with that path; a sidebar with none hangs them straight under the home,
unlisted, where a link still reaches them.

**Every item with a page is a segment of its url**, its name as the sidebar spells it -
`/project/Guides/Quick start`. The folders follow where they stand: `/project/Core/...` under an
All the APIs item at the top without a subgroup, `/project/All the APIs/Core/...` in one. Where an
item and a folder go by one name in one place, the url names the item. An item's name holds no
slash and is no other item's beside it, and renaming an item moves every url under it, as renaming
the project does. Switching the subgroup moves the folders' urls, and a tab left on the old ones
shows the gone page.

**The sidebar is edited on the sidebar page**, which Edit sidebar opens in the tab shown: on the
home, under the title, and in the tree, on the menu of the project's node and of a sidebar item's
row, where the page opens with the row's item selected. The page holds:

- **The tree** of the items under the project's own node, which always stands and cannot be
  deleted or moved. Every node stands open.
- **Add**, in the row under the title, drops the four kinds. The item is added at the end of the
  selected category, after the selected item otherwise, and at the end of the top level with the
  project's node selected; it is named by its kind - New category, New page, All the APIs - with a
  number where that name is taken, and opens for its name at once.
- **Delete** takes the selected item out with everything under it. A category holding items asks
  first. Delete on the keyboard does the same.
- **A name is typed where it stands**: a second click on the name, F2, or Rename from the row's
  menu opens an editor over it. An empty name, a slash, or a name another item beside it goes by is
  refused with the reason, the editor standing.
- **Kind**, at the right, is the selected item's kind, which a click on another answer changes. A
  category holding items keeps its kind. An item made a divider loses its name; a divider made
  anything else is named by its new kind.
- **In a subgroup**, under the kinds for an All the APIs item, puts the folders under a node and a
  page of the item's own. It starts unticked.
- **The items under one node are carried** to another place among themselves - a press on a row and
  a drag. Moving an item into or out of a category is not a drag: it is added where it goes.

Every edit is written into the project's properties at once, and every tab's tree and every page
the sidebar is read into are built again, the sidebar page standing. A sidebar emptied reads back as
the default, so the last item deleted gives way to All the APIs.

#### Sources

Under Sources stand the folders the project reads, in a grid with no header. Each row holds:

- the folder, in full, with a browse button beside it
- an Include subfolders box, saying whether the folders under it are read
- a cross that stops reading it

How the rows behave:

- **Order.** A row dragged moves among the others. The sources are read in the order the rows stand
  in.
- **Editing.** A mouse click on the folder of the row already picked edits it in place, on the words
  `Sources` writes - the words typed. An edit refused keeps the editor open with the reason.
- **Replacing.** A folder typed or browsed over a row is read in its place, one typed relative read
  from the project's folder. An edit emptied stops reading it. The same folder typed another way is
  kept as typed, with no scan.
- **Add source.** Under the folders stands a row reading Add source, always there. A folder typed or
  browsed into it is read as one more, and the row becomes that folder's with a new Add source row
  under it.
- **Refusals.** A folder that does not exist, or is read already, is refused with the reason - at
  the editor, or at the row when it was browsed.

#### Scan options and Edit hints

- **Scan options** asks whether the sources are read as any C++ and Delphi or as ClaFi modules, and
  whether hints come from every recognizable comment or follow the strict ClaFi rules. A new project
  starts on the first of each.
- **Edit hints** asks whether a hint edited on a page is not written at all, or written directly
  into the source, with no undo.

Every change reads the sources again, except Edit hints, which builds the pages again. The page
stays where it stands while the sources are read again.

#### Folder suggestions

**A folder typed lists the folders under it.** Two boxes do this: a source's editor, and the New
project window's box for where to store the project.

- The list holds the folders in the folder the text names up to its last separator, whose names
  start with what follows it - case aside, by name, at most 200.
- Folders whose names start with a dot are listed only once a dot is typed.
- Each is marked as the tree marks a folder, the letters typed in the spot.
- A source's editor reads a path typed relative from the project's folder. The New project window's
  box lists only a path from a root.
- A server named alone, as `\\server\` is, lists nothing, its shares being looked up over the
  network.
- The box lists only on an edit at its text's end. The editor lists on every edit.

Keys:

- **Down** or **Alt+Down** drops the list for the box's text as it stands.
- **F4** or **Alt+Down** over the editor lists the folders under the project's folder, whatever is
  typed.
- **Down, Up, Page Up and Page Down** walk the list. Nothing is current until Down, and Up from the
  first line goes back to the text.
- **Return**, or a mouse click on a line, takes that folder into the text with a separator after it,
  and the folders under it are listed in turn. Return with no line current takes what is typed, as
  it does with no list.
- **Escape** takes the list down, as Tab and leaving the box do.

### Tabs and urls

SeeDocs is a browser of the framework's own kind:

- tabs across the top
- under them the crumbs of the page shown, with a list dropping from each
- Back, Forward, Up and Rescan beside the crumbs
- a search box at the bar's end - see [Search](#search)

**Rescan reads the sources again**, from its button or from F5 anywhere in the window.

- It scans the project the shown tab stands in, and every open project from the root. Its tooltip
  names the project, or says all projects.
- An options page on screen is built again, so its last scan reads just now.

**Every page has a url** - `/project/item/folder/module/type/method`, the project named by its
name, the items of its sidebar after it as far as they go, and the folders where its All the APIs
item puts them.

- A tab goes back and forward through the pages it has visited: Alt+Left and Alt+Right, the mouse's
  side buttons, or a right click on either button for the list.
- A type's crumb lists its methods.
- Up climbs the path.
- A url no open project carries - a project closed, or a type gone since the sources were scanned -
  shows a page saying so.

**The root page**, `/`, lists the open projects.

- Each has a Close button that closes the project and every tab on it.
- A row dragged to another place puts the projects in that order, which the tree and the next start
  follow.

**A home page opens each project.**

- It lists the sidebar's items as the tree lists them, each marked with its kind's mark and linked
  to its page, a divider parting the list.
- Options and Edit sidebar stand in a row under the title, each marked with its page's icon, and
  Close project stands at the page's top right.
- In the place of an All the APIs item without a subgroup, the home - or the category page that
  lists the item - shows the folders under the root with what each holds, and the root's own
  modules; a project with nothing to document says why under the title. In a subgroup, the item's
  own page shows them.
- Modules standing in a source folder's own folder are the root's. They stand after the folders in
  the tree, and the page that shows the folders carries what they export, by module, as a folder's
  page does.

**Renaming and restoring**

- Renaming a project takes every url under it along, in the tabs and in their histories alike.
- A second project opened under a name already taken goes by that name with a number after it.
- Where a tab stands, the tabs open and each one's history are kept with the settings, so a run
  starts where the last one left off.

### The tree

Down the left, under each project's node, stand the items of its sidebar - a category a node
holding its items, a divider a line, a content item a row - and a tree of the sources' folders,
named as they stand on disk, where its All the APIs item puts them: in the item's place, or under
its node in a subgroup. The page of the row clicked shows on the
right, and Open in new tab on a row's menu opens it in a tab behind the one shown. Edit sidebar on
the menu of the project's node or of an item's row opens the sidebar page - see
[The sidebar](#the-sidebar).

- Each folder holds the folders under it, then its modules by their full names.
- Each module holds the types it exports, controls first.
- A module that exports functions or constants and no type is a row with nothing under it.

**Each tab has a tree of its own**, which follows wherever the tab goes:

- to the folder's node, for a folder's search or list
- to the item's row, for an item's page, or a search or a list of the whole project
- to the project's own node, for a page the tree has no row for, or a page gone

The nodes a tree has open and how far it is scrolled are the tab's. The open nodes are kept with the
tab, by project - an item by its path, a folder by its path and a module by its folder's path and
its name, each under the path of the item it hangs from - so a restored tab, or a project opened
again, comes back as it was left. A node's rows are made the first time it opens, out of the
sidebar and the project's contents, which are read once and shared by every tab's tree.

### Pages

A page is a column of controls:

- **The title**, marked with the mark its tree row wears, followed by what the page is - a project,
  a folder, a module, a function, a constant, a type's kind.
- **The hint**, under the title.
- **A grid of facts** - namespace, module, source.
- **A section per expander.** A section's heading says what the section is. How many elements it
  holds is the heading's tooltip.

Where the surface states no hint, the page says so in place of the words, so what the documentation
still lacks reads off the preview. Hints read from every recognizable comment owe nothing, and their
pages leave a missing one empty.

A page's trees start closed, and a node's rows are built the first time it opens.

#### Source and Open in editor

The source fact is the file and line a type, a method, a function or a constant stands at, and a
module's interface file. It names the file by its path in the project's tree, and its tooltip and
the cell's editor give the whole path.

**A path in the project's tree starts with the project's name** - `ClaFi/Controls/Button.cppm` -
then the folders under the source's folder, wherever a page or the search box writes one: source
rows, the lists, the search page and the search box's tooltips.

- **Open in editor**, at the row's end, runs the command line Settings keeps for the file's
  language. While there is none, it stands greyed, its tooltip naming Settings.
- **What goes wrong** is said in a notice over the window's bottom right, headed with Open in editor
  and the file's name - a page may be gone by the time the answer comes.
- **A file gone.** A file the last scan read that is missing from disk, or that stands in a folder
  the project does not read now, is said to be gone before anything runs, with Rescan as the answer.
  The project holds this notice, which goes when the project closes.
- **A program not found.** A command line whose program the shell does not find, or finds and cannot
  run, is said so by name, with Settings as the answer.
- **A program that ran** is left alone whatever it exits with. An editor's launcher hands the file
  to an editor already open and exits as it likes.
- A language holds one such notice, which a new run or an edit of its command line takes down.

#### Folder pages

A folder's page:

- gives its source as a module does, a row under each source holding it, with an Open folder button
  that shows it in the desktop's file manager
- says what it holds at every depth, each figure linked to its list
- leads with the folders directly under its own, as the tree lists and marks them, each with what it
  holds at every depth
- where the folder holds modules of its own, goes on with **Modules**

Modules is one grid holding a row per module, closed, the module's name linked to its page. Under it
stand the module's types, its functions and its constants, each run beside its word - Types,
Functions, Constants. Each row carries its hint, which runs over the cells its row leaves empty
before it.

#### Lists

The figures a folder's page counts - its controls, types, functions, modules and folders - each
open a list of everything of that kind under the folder at every depth, as many rows as the figure
counts. The project's home counts and lists the whole project the same way. A figure of none stays
plain text.

- The head names the list and, on its second row, the path of the folder it stands under. The row
  stays with the title while the list scrolls.
- The list is one grid, standing on the page with no expander round it, by name regardless of case.
- A type's row carries its kind at the end of its name cell, as the Modules grid has it. The
  controls list leaves the kind out - every row's mark says it.
- A name the list holds more than once is written after its namespace, from the last part the
  namesakes share: `Grids::Cell` and `Grids::Dt::Cell`, `::` alone for the global namespace.
- A type of one qualified name in several folders carries its folder at the end of its name cell,
  ahead of its kind.
- A function's row is its module page's: one per name, the overloads' signatures in the name's
  tooltip. A name several modules export in one namespace carries the module's name at the end of
  its name cell, and the module's folder after it where the modules go by one name too.
- A module and a folder stand beside a folder's path - the one the module stands in, the folder's
  own.
- Its url is its folder's with `/.controls`, `/.types`, `/.functions`, `/.modules` or `/.folders`
  after it, a dot segment as the options page's. The tree follows it to its folder's node.

#### Module pages

A module's page holds a grid of its own per kind of export. Its crumb lists its types, its functions
and its constants, and their pages stand under its url. A type's row carries its kind at the end of
its name's cell, as a tree row does.

#### Function and constant pages

- **A function's row** carries its name alone, one row per name, every overload's signature in the
  name's tooltip. The name links to the function's page.
- **A function's page** carries the hint under its title - the first commented overload's, as the
  row does. It holds every overload of the name, coloured, with its note and, where it differs, its
  own hint.
- **A constant's name** links to the constant's page. The grid's value cell shows the first lines of
  the folded initializer.
- **A constant's page** carries the hint under its title and holds the declaration as written,
  coloured and whole, with its note.

#### Type pages

**A class's title carries its bases under it as a reversed breadcrumb bar.**

- Each base has a crumb, its mark before it pointing back toward the type standing on it.
- The ways up run right, and part into a column of rows where a type has several bases. A mixin's
  host is reached through the argument it was given.
- A crumb opens its base's page. Its mark drops the types standing directly on that base, the one
  the way up came through marked. For a base the surface does not have, it drops the types whose
  bases spell it the same way, template arguments aside.
- Crumbs wider than the page come down to one common width, the widest first, and keep at least
  their mark and a few letters.

An alias's title line is what it stands for, after an equals sign. An enum's is its underlying type,
after a colon.

A type's page holds, in this order:

- **Derived types**, a tree. A row of it opens the type it names.
- **A grid per member kind** - properties, events, public methods and protected methods. The type's
  own stand under an Own expander, and each base's under a From expander of its own, up the whole
  chain of bases.
- **Fields or members.** A struct's or a union's fields stand ahead of its methods.
- Each row of these grids carries its hint.
- **The types using it** - the types with a property of this type and the types with an event of it,
  two trees. Each type has the types derived from it under it, closed.
- **Details**, the page's footnote - the note the type's comment references, as prose, its file and
  section in the heading's tooltip.
- **The types beside it in its module** - the module in its own folder, a namesake's left out.

**A base's methods grid holds what reaches the type.**

- Its constructors stand there only where the type takes them with `using Base::Base`, and its
  destructor never does.
- A method stands there while no type below declares its name again - C++'s own hiding, by name,
  which a `using Base::name` undoes.
- A republished method stands under the access its using-declaration gives it.

**Restated defaults.** A base's property whose default a type below restates shows that value in its
Default cell. The cell ends, past a flex space and in the SubBody style, with `by` and the type
handing it, linked and joined to `by` by a non-breaking space, and `fixed` where the caller's own
value loses to it. The declared default stays on the declaring type's own page.

**A property's getter and setter are its row**, and stand in no methods grid.

**Borrowed hints.** A method with no comment of its own - an override, as a rule - takes the hint
and the footnote of the method of the same name and parameter count in the nearest base that has
one.

#### Method pages

- A method's row carries its name alone, one row per name, every overload's signature in the name's
  tooltip, a line each.
- The name links to the method's page, titled `Type::name` with the type linked. A method's page
  picks the method's type in the tree.
- The page is built as a function's page is, the type and the access labels being what a method
  adds. The hint stands under the title - the first commented overload's. Then every overload of the
  name stands under its access label, written the C++ way - `public:` - with:
  - the declaration as code, coloured as the framework's code boxes colour C++
  - its own hint, where the title's is another overload's
  - the note the comment references

#### Footnotes on a page

- **Marks.** A property, an event, a field or a member whose comment references a note carries a
  mark in a column of its own, and the note stands as a footnote under the grid.
- **The mark leads to the footnote**, as an anchor of the page's url. Its tooltip shows the
  footnote's first lines over the row, as a link's tooltip shows the paragraph of the anchor it
  leads to.
- **A note's section stands in full once on a page.** Details holds its own, and otherwise the first
  footnote reaching it does. A later footnote reaching it names that place - `* OnSearch - see
  listKeptBy` - linked where the place is a footnote. An overload's reads "See the note above".
- **Matched by name.** A declaration whose comment references nothing takes the section named after
  it from its folder's note - see [Editing a footnote](#editing-a-footnote) for which note that is.
  The heading's tooltip marks it as matched by name.
  - A type's, a function's or a constant's heading is its name.
  - A member's heading is its type's name and its own - `Button onClick`.
  - A method's page shows its section under the first commented overload.
  - A constructor takes none by name: its name is its type's, and so is the section that name heads.

#### Delphi type pages

A Delphi type's page is spelled as a C++ type's is - its title, its bases as crumbs under it, the
access labels written `public:`. Its code is Delphi's own: a routine's signature and a constant's or
a variable's declaration stand as written, coloured as Pascal.

- Its properties and its events stand in a grid per access - published, public, protected.
- A base's member that a type below declares again is the type's own, counted once.
- A constructor reaches the type from its bases as a method does, and a type below declaring the
  name hides it as it hides a method.

### Editing hints and footnotes

**Footnotes are edited where they stand, in every project.** Hints are edited only where the project
writes them into the sources and follows the strict ClaFi rules - whose comment is the one the
writer puts words in - and only in C++ sources.

#### Editing a hint

- **A hint reads as a line of a note does**: words between `**` are bold, and words between
  backticks are code.
- **A hint cell** opens its editor with F2, Enter or a double click, on the comment's own words -
  its types unlinked, a missing hint empty.
- **The hint under a page's title**, and the one under each declaration of a method page, are typed
  into where they stand, a missing one an empty box. Ctrl+click follows a link there, and Escape
  puts back the words the page shows. The bold and the code a box shows are written back as their
  marks.
- **Enter writes the words** into the comment the hint is read from, and leaving one of those boxes
  writes them too. The `See` the comment ends with is kept.
- **The comment's place.** The comment stands on the declaration's line while the whole line fits
  100 columns, lined up with the comments of the lines of code round it. Otherwise it stands above -
  always above a class, a struct or an enum.
- **Overloads and overrides.** A method's row writes the overload whose words it shows. An override
  that shows its base's words writes the base's comment.

#### Editing a footnote

- **Editing.** A footnote opens its markdown in its place with F2, or Edit footnote on its menu.
  Each paragraph and each bullet stands on one line, the headings inside it and its code as written.
- **Saving.** Ctrl+S or Save writes it into its note. A paragraph or a bullet that reads as it did
  keeps its lines, and any other is broken at 100 columns again. Escape or Cancel leaves it.
- **Add footnote.** The row of a property, an event, a field or a member with no footnote offers Add
  footnote on its menu. The footnote goes after the type's own section in its note, or at the end of
  the folder's note, headed by the type's name and the member's. It is found by that heading, and
  the source is left as it is.
- **A folder's note** is the one its declarations cite the most, through the `See` of their
  comments. Where they cite none, it is the one named after the folder's own name - `Y-Base` for
  `Controls/Y-Base` - and the root's is `Home`. The folder of footnotes and the note are made with
  the first footnote written into them.

#### Writing

- **Every write reads its file again first**, and refuses, saying why, where the file holds
  something other than what the page was built from. A file changed in an editor since the scan is
  scanned again before it is edited here.
- **A file keeps its byte order mark**, and each line its own line end.
- **The words are written by `SeeDocs_App.Edits`.** The database follows every hint written - see
  [Saving](#saving) - and the page shown takes the new words where it stands. A footnote added
  builds the page again, at the new footnote.

### Links

**A type's name on a page is a link to its page**, in a grid cell as in prose. A type named in a
hint, in a note's prose or code, or in a signature - bare or qualified, as `Control` or
`Grids::Row` - links wherever the surface has a page for it, read where it is written as Names says.

**A link in a declaration keeps the declaration's colours** - a type is coloured as a type, and
underlined while the pointer is over it. Everywhere else a link is drawn in the accent.

**A link to an alias, a struct, a union or an enum previews its declaration** in its tooltip,
coloured.

- In C++: an alias's `using` line; a struct's or a union's head and bases over its public instance
  fields, a line each in braces; an enum's head over its members the same way.
- In Delphi: the name and an equals sign before what an alias or a built type stands for; a record's
  public fields down to its `end`; an enum's members in brackets on one line.
- A nested type is named with its outer type in both: `struct ShadowPainter::Design`.
- A link to a struct holding no such field keeps the link's own tooltip.

**Following a link**

- A mouse click follows a link.
- Return follows it too, on the grid cell or the header holding it, or with the caret standing in
  it. In a hint box that edits, Return writes the words, so Ctrl+Return follows the link.
- A right click on a link, or the menu key on a grid cell holding one or with the caret in one,
  offers Open and Open in new tab. Open in new tab opens behind the tab shown.
- A link to a member with no page of its own arrives at the member's row, with the row scrolled into
  view, as every first arrival does.

### Namesakes

**Modules of one name in several folders are namesakes**, and so are the types they declare under
one qualified name. In a tree of Delphi projects every form's unit may be `Unit1` and every form
`TForm1`. A url names the folder, so each namesake has a page of its own.

- **A link the page builds names its namesake by its folder**: a module's row and a type's row, the
  module a type stands in, a search's names, a list's rows, and the derived types a type page lists
  under a base crumb and in its tree.
- **A type a text names** - a hint, a note, a signature, a base - is read where it is written. The
  namesake in the page's own module is the one meant, and else the one in the page's folder.
- **Where neither settles it, the link asks.** A click on it, or Return, drops a menu of the
  namesakes, each with its folder muted at the line's end, and the one picked opens. Open and Open
  in new tab on the link's menu drop the same menu, and the pick opens as the command says. A
  search's page and a list stand in no module of their own, so their links ask wherever there are
  several. On the root's search, which spans the projects, such a name is left unlinked.
- A link that asks shows no preview in its tooltip.
- **A namesake reads apart by its folder** wherever names are listed - see [Lists](#lists), [The
  list](#the-list) and [The search's page](#the-searchs-page).

### Keys on a page

A page holds one selected item - a row of one of its grids, a section's header or a box of text -
and the keys walk the page as one.

- **Up and Down** move over rows, headers and boxes alike, from one section into the next.
- **Home and End** go to the ends of a row; Ctrl+Home and Ctrl+End to the page's first and last
  item.
- **PageUp and PageDown** go a screenful on.
- **Left and Right** stay on the row or the header.
- **Tab** leaves the page as a whole.

**A box of text is walked past like a row** until Return or a mouse click opens it for the caret.
The arrows then move the caret, Return follows the link the caret stands in, and Escape closes the
box again. A hint box typed into writes its words on Return and on leaving, and Escape puts the
words back.

A section or a tree's node shut over the selected row selects its header.

### Anchors

**Every item of a page has an anchor**, which the tab's url names it by after the `#`:

- a row by its name - a member's, a method's, a type's, a folder's, a module's, a project's, a
  fact's label
- a section's header or a group's by its heading
- a tree's row by its type
- a footnote by its mark's anchor
- the lead by `lead`
- a declaration box on a method's page by its access label

**A name standing twice on one page is spelled apart.** A member inherited from a base, whose name
the page already has, goes by `Base::name` - `RichControl::Padding` under From RichControl. Any
other repeat is numbered - `Padding.2`.

**Selecting an item puts its anchor into the tab's url.** That is a move on the page, which the
history does not step over, so the url a link leaves from names the row it was followed from.

**Back, Forward and a tab restored at the next start select that item again.** The section or group
holding it opens, a tree's nodes open down to it with their rows built on the way, and the item is
put back where it stood in view.

- The page is measured from the item. A window of another size, with the text wrapped anew, still
  shows the item at the same height.
- A page with no item selected comes back where it was scrolled.
- A page built again in place, after a rescan or an edit, keeps its place the same way.

### How the pages are built

`SeeDocs_App.Pages` builds the pages out of the surface and the notes - facts, sections, tables of
rows with their footnotes, branches of a tree, signatures and blocks of prose. SeeDocs builds
controls from them (`SeeDocs_App.PageView`), the browser (`SeeDocs_App.Browser`) shows one per tab,
and a generator renders the same pages as files.

## Search

A search is typed into the box at the crumb bar's end, and Ctrl+F puts the focus there from anywhere
in the window. It searches the project the shown tab stands in. On the root, which lists the
projects, it searches every one, and its lines and its rows name their project.

### What is found

A search finds by name every folder, module, type, function and constant the surface carries, and
every member a type's page lists: its properties, its events, its methods and its fields or members.

- Private members are left out.
- A constructor and a destructor are the type's own.
- A property's getter and setter are the property.
- A method with no comment of its own that a base declares too is the base's - the base's row stands
  for it.
- A method or a function is one name, however many overloads it has.

A name is found by every spelling it has: what its row shows - an event by its alias - and an
event's struct and its method, a folder's path and a module's whole name. A type is found by its
own name alone, never by the namespace or the module it stands in - its namespace counts only as a
query's first part.

### Matching and ranking

A name holds what is typed, case aside, in one of four ways, best first:

1. it is what is typed
2. it starts with it
3. a word of it starts with it
4. it holds it anywhere

A word starts after `_`, `:`, `.` or `/`, at a capital after a lower-case letter or a digit, and at
the last capital of a run before a lower-case letter.

**A query of two parts** is cut at its last `::`, else at its last space, as `Grids::Row` and
`Button onClick` are. It also finds a name its second part is found in whose owner holds the first:
a member's type, the namespace of a type, a function or a constant, and the folder of a module or a
folder. A member of a base of the type the first part names exactly is found too, after the type's
own, since that type's page lists it.

The names found are ranked by, in turn:

1. how they hold the words
2. how their owner holds a first part
3. types, folders and modules before the rest
4. the shorter name
5. the tree's own order

### The list

The box drops a list as it is typed into.

- **The searches come first**, the narrowest first:
  - Search "row" in Grids - the folder the shown page stands in, with the folders under it
  - Search "row" in ClaFi - its project
  - Search "row" in all projects
- **The ten best names** follow, under a divider. Each carries the mark of what it is - a member's
  line leaves the mark's room blank - a member's type muted before it, the words typed in the
  spot, and its kind at the line's end. A namesake carries its folder after its kind.
- **A tooltip** beside the list holds the current line's hint and where it stands.

A search that would search what the one above it does is left out: a project's home and its options
page offer no folder, one project open offers no all projects, and the root offers its own search
alone - all projects, or the one open.

**The list drops with no line current**, so Enter steps through the page's matches.

- Down enters the list at the first search's line, and Up on it leaves the list again.
- Enter on a line opens that name's page, or that search's.
- Alt+Enter or an Alt+click opens a line in a new tab behind the one shown. The list stays up, so
  that more lines can be opened.
- A member with no page of its own opens its type's page at its row. The url's anchor names the row
  by the member's name, the row's section opens and the row is selected.

### Searching the page shown

**The box searches the page shown as well.**

- Each word in it stands on the found band an editor's own search paints with, wherever the page
  shows it, case aside. The two parts of a query of two are a word each.
- It bands every page shown after - from the tree, Back and Forward or another tab - until the box
  is emptied.
- The count at the box's end is the page's: "15 on page" while no match is current, "4 of 15" once
  one is, and a muted "0 on page" where the page holds none, the project perhaps holding more.

Stepping through the matches:

- With no line of the list current, Enter and Shift+Enter take the list down and step to the page's
  next match and the one before.
- F3 and Shift+F3 do so from anywhere in the window.
- The arrows at the box's end step too, dimmed while the page holds no match. Its cross empties the
  box.
- A section a match stands in opens, and the match is selected - a cell as a mouse click selects it,
  the words in a footnote - and drawn over its band.
- A step taken on the page brings the focus with it, and one taken in the box leaves it there.

**A page the search opens goes on with it.** Opened from a name's line of the list or from a link on
the search's page, its first match from where it opens - the place its anchor names, else its top -
is made current and brought into view. Showing a search's page puts its words back in the box.

**Escape** takes the list down, and the next one hands the focus back: to the page's current match,
else to where it came into the box from - a row of the tree, a cell of the page. Where that has
gone, or stands on a tab now hidden, the focus goes to the row the page's anchor selected, else the
page's first row. Escape keeps the box's words: the search stands while the page is read, and goes
as its words are typed away or cleared with the cross.

### The search's page

The search's page is a page of the browser like any other.

- It is named as its line in the list is - on its tab, its crumb and in the history.
- Its head is titled with the words and marked `search`.
- Its url is the folder's or the project's with a segment of `.search=` and the words after it - a
  slash in them written `%2F`, a % `%25` - or the root's for the root's search. So Back and Forward
  step through searches, and a tab is restored onto one.

It holds:

- a section for each kind of name found, the names in the order they are ranked, a namesake's
  folder at the end of its name cell
- **In hints** - the declarations whose hints hold the words and whose names do not
- **In footnotes** - every footnote section holding them, named by its note and its heading, with
  the sentence the words stand in. Each is linked to the declaration it belongs to: the one its
  heading names, else the first whose See reaches it.

Hints and footnotes are searched as they read, their code and bold marks taken out. A section's
words are its own, up to the first heading under it. A section lists its first 200 rows, and its
heading's tooltip says how many there are.

**A folder's search holds what stands in the folder and in the folders under it**: their folders,
their modules and what the modules declare, and the footnote sections whose declaration stands
there. A section that belongs to no declaration is found by the searches of whole projects alone.

## The checks

`check` holds a project's sources read as ClaFi modules to the routine, and reports:

- a property with no `INIT_PROPERTY`; a `..._STORAGE` property without its getter, and a `WRITABLE`
  one whose setter is not there, the base chain searched as well
- a `Props::get` or `Props::find` written by hand rather than through `READ_PROPERTY` or
  `REQUIRE_PROPERTY`
- a comment of more than one line, one in both places at once, or none on a declaration of the
  design surface - a control, its properties and events, an exported enum
- a comment above a declaration it would fit on, and one riding a line past 100 columns
- an event whose three spellings disagree, or whose struct is not found
- a property whose type is neither an enum, a known value kind, nor a declared type
- a `See` reference to a footnote that does not exist in the project's `footnotes`, or to a heading
  the note does not have - on every line of a comment
- a class declaring properties or events whose base chain reaches neither Control nor a type the
  scanner can read
- a value type declared under `Controls/Base`, where it reaches an application only through an
  import of the base

A project reading any C++ and Delphi is held to nothing.

## Building

`SeeDocs.sln` builds both programs with Visual Studio against the shared items:

- `SeeDocs`, the window
- `SeeDocsCli`, the scanner

They share the Surface, Reading, Scanner, PascalReader, Database, Project, Notes and Pages modules
under `Src/`. The window's own modules stand under `Src/Ui/`, and the Edits and Search modules
beside the shared ones.

The CMake target `SeeDocs` builds the window on Wayland, beside the other applications. The CMake
target `SeeDocsCli` builds the scanner on Linux, with or without a platform layer. The Dom, the
Syntax lexer and the text file reader reach no further down than `UiTypes`, so a `check` runs
wherever the tree is checked out:

```
cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=none \
      -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_BUILD_TYPE=Release
cmake --build build --target SeeDocsCli
./build/SeeDocsCli check Source/.seedocs
```
