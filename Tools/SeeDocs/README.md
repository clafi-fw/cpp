# SeeDocs

The tools that read the framework's design surface out of its source and put it to use. Two programs
so far: SeeDocsCli, the scanner, which reads every module in a project's folders of sources, writes
what it finds into one database and holds the sources to the declaration routine; and SeeDocs, a
window over that database showing the surface as the reference it is to become - the part of the
documentation that is generated; the rest is written.

    SeeDocsCli scan [folder] [--out file]  writes the database, the project's Surface.cfg by default
    SeeDocsCli check [folder]              reports deviations, exit code 1 if any are errors
    SeeDocs [folder]                       opens the project the folder stands for

The folder is a project's own, or a tree with a `.seedocs` project in it - for the framework, its
`Source/` folder; for SeeDocsCli it is the working directory when left out. A scan with `--out`
takes any folder, read as the one folder of sources of a project of ClaFi modules. The report
reads `file:line: level: text`, one line per finding, in file and line order.

The sources are read as ClaFi modules or as any C++ and Delphi. As ClaFi modules, they are written
to the declaration routine: their modules are read, their macros of the routine harvested, and
the checks hold them to the routine. Any C++ and Delphi is for anyone else's code - a third-party
library - and reads what it declares without asking anything of it: headers and units as well as
modules, nothing checked. How a comment gives a declaration its hint is a choice of its own, and
so is whether a hint edited in SeeDocs is written back into the sources.

## Projects

A project is a folder of its own, stored wherever the user chooses - SeeDocs offers
`Documents/SeeDocs`. Everything SeeDocs keeps of a project stands in it: `Project.cfg`,
the project's properties; `Surface.cfg`, the database the scanner writes; and `footnotes`, the
notes the pages show beside the hints. The sources stand wherever they are, in any number of
folders, and are read as they stand.

    Name = ClaFi
    Sources = [
        =[ Path = ../ClaFi/Source, Subfolders = true ]
    ]
    Limited = false
    LooseHints = false
    EditHints = true

`Name` is what the project goes by; empty, it goes by its folder's name. `Sources` are the folders
it reads, each with the folders under it or without them. A path stands as it was typed and is read
as a folder when the sources are read: a relative one from the project's folder, one that starts
with a separator from the root of the project's volume, and a backslash parts folders on every
desktop, so a path typed on Windows names the same folder on Linux. A folder picked rather than
typed is written relative to the project's folder while the two share a folder under the root of
their volume, so that they move together and a project kept in a repository names no place outside
it, and as it stands otherwise. Folders of one name under two sources are one folder, and a file two
sources reach is read once, as the first of them found it. `Limited` reads the sources as any C++
and Delphi rather than as ClaFi modules, `LooseHints` takes every recognizable comment's hint rather
than the strict rules' - see Hints - and `EditHints` writes a hint edited on a page into its source.
A project the window makes is written `Limited` and `LooseHints`; one stating neither reads its
sources as ClaFi modules by the strict rules.

A project made inside a tree, in the tree's `.seedocs` folder, is a project all the same, and a
tree that has one stands for it: `SeeDocsCli check Source` checks the project in
`Source/.seedocs`. Such a project names its tree among its sources as `..`, the way any project
names a folder, and reads nothing it does not name: removing `..` leaves it reading none.

## How it reads

Every `.cppm` and then every `.cpp` in the folders of sources is lexed with the framework's own C++
lexer (`ClaFi.Core.Syntax`) and read declaration by declaration over the tokens; a folder whose
name starts with a dot - `.seedocs`, `.git`, `.vs` - is skipped with everything under it. Nothing is
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

### Hints

A declaration's hint is read by one of two sets of rules, whichever way the sources are read.

By the strict ClaFi rules, a declaration's comment is the `//` line on its own line, or else the
one touching it from above; a class template's may sit between the `template` line and the
`class` line, or above the `template` line. The comment's first line is the hint the database
carries, with a trailing `See <note>#<anchor>` reference cut off into the entry's `Note`. A
declaration with no comment owes one, and its pages say so.

By every recognizable comment, a comment is a doc comment however it is written - `//`, `///`,
`//!`, `/** */`, `/*! */`, a trailing `///<` - and the comment above a declaration comes before
the one on its line, which is as often a tool's directive as words. The hint is the `@brief` or
`\brief` paragraph, or else the first sentence of the first paragraph of words; a block opening
under other comment lines starts a paragraph. A `See` in a comment is words. A comment on a line
goes on in the comment lines under it that open in its column, and those lines are its own, not
the comment above the declaration under them. A separator - a run of three or more of one mark,
`---`, `===`, `***`, `~~~`, `###`, `___`, `+++` - says nothing, and a hint that only names its
declaration - a label, `// ----- logger -----` - is none. No hint is owed.

### Any C++ and Delphi

A read of any C++ and Delphi takes every module interface - `.cppm`, `.ixx`, `.mpp`, `.cxxm`, `.ccm`
- every header - `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, `.inl`, `.ipp`, `.tpp`, `.tcc` - and every
Delphi unit - `.pas`, `.pp` - and no implementation file. The sources' macros are gathered from all
of their C++ before any file is read, and every file is then read as the rules below make of it; a
unit as the Delphi rules after them make of it.

- A header stands as a module of its own, named by its path under its folder - its include
  spelling - and shown by its file name; its includes are its imports. A module interface is
  read as a ClaFi module is.
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
- A comment is any of `//`, `///`, `{ }`, `(* *)`. By the strict rules a unit's hints are read as
  a C++ file's are. By every recognizable comment the comment above comes first, as in C++, read
  by the same rules of continued lines, separators and labels; the hint is an XML doc comment's
  `<summary>` where it has one, else the first sentence of the first paragraph, a line opening
  with a tag ending a paragraph as a command does; `<see cref="X"/>` and `@link(X)` read as
  their `X`.
- By every recognizable comment, a routine the interface gives no comment takes the hint of the
  comment standing under its header in the implementation section, between the header and the `var`,
  `const` or `begin` the routine opens with. The header is matched to the routine by its name and
  the types it is a member of - `TOuter.TInner.Name` - read in any case, and where the name is
  overloaded, by how each parameter is passed and its type. A `forward` or an `external` routine has
  no body, and the lines under its header are not its own.

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
    LooseHints = true
    Modules = [
        =[ Name, File, Root, Interface, Header, Language, Imports = [...], ExportedImports = [...] ]
    ]
    Types = [
        =[
            Name, Namespace, Kind (class, struct, union, enum, type, concept, interface), Module,
            File, Root, Line,
            Hint, Note, Template, Bases = [...], Target, Control, Folder,
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
    Functions = [ =[ Name, Namespace, Kind, Signature, Type, Template, Module, File, Root, Line,
                     Hint, Note ] ]
    Constants = [ =[ Name, Namespace, Type, Value, Constant, Declaration, Module, File, Root, Line,
                     Hint, Note ] ]

`Scanned` is when the scan that wrote the file ran, in UTC to the second; `Limited` that it read the
sources as any C++ and Delphi, and `LooseHints` that it took every recognizable comment's hint. A
hint written from SeeDocs keeps them, since such an edit reads nothing of the sources beyond what it
changes. A `File` is the path under its folder of sources, and `Root` that folder, counted from zero
in the order the project names them. `Header` marks a module that is a header, as a read of any C++
takes one; its `Name` is its path under its folder of sources. `Language` is `pascal` for a Delphi
unit, whose code a page reads and colours as Pascal, and a C++ module states none; a Delphi
interface's `Kind` is `interface`, and a Delphi member's `Access` may be `published`.

`Control` marks a type whose base chain reaches `Control` or `RichControl`, template arguments
walked as bases, which is what makes it a toolbox entry; `Folder` is its folder under its
folder of sources. `Bases` are written as a reader can follow them: an alias the database does not
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

`SeeDocs` opens projects, as many at once as are asked for - the ones left open last time, and the
one the folder named on its command line stands for - and shows each one's database and its
footnotes as pages. The application menu makes them and opens them. Its New project command opens a
window of its own over the main one, which is moved by its title and stays up until it is answered:
it asks for the project's name and where to store it, showing the folder the two make, and Create
makes the folder and opens the project on its options page, while Cancel, Escape or the title's
close button lets it go; asked for again while it stands, it comes forward as it was left. The
menu's Open page holds Browse, which asks for a project's folder, or a tree that keeps one, in the
desktop's own dialog, and the projects opened before, most recent first, one click each. A project
opens in a new tab on its home, and one open already gets a new tab on its home too. A folder that
stands for no project is offered one that reads it, made through New project with the folder as its
first source. The window title names the project the tab shown stands in. Which projects are open
and which were opened before are kept with the application's settings, so they are remembered once
settings are kept on the computer. The menu's Settings page holds External editors under the
framework's own options: a command line each for C++ and Delphi sources, in which `{file}` stands
for a source file and `{line}` for a line in it, kept with the application's settings as it is
typed. The chevron at a box's end, or Down in the box, drops the editors most used for that language
on the desktop SeeDocs runs on, each with its command as it is typed in a terminal; typing at the
box's end narrows them to those whose name has a word, or whose command, starting with what is
typed, and an editor picked puts its command in the box's place. RAD Studio and Lazarus have no
switch for a line, so they are marked no line and open the file alone. Visual Studio goes to the
line where its command starts it; one already running opens the file alone, since `/command` is
run by the instance it starts.

Each open project is a node of the tree, named after the project, and the node opens its home. Under
it stand its Project options row and a line that parts the row from the project's folders. The
options page is titled as every page is, and under the title holds the project's name, in heading
type, in a box that renames it - Return or leaving the box keeps the name, Escape puts it back, and
an emptied box gives the project its folder's name - then its folder, and how long ago it was
scanned with a Rescan button that reads the sources again. Under Sources stand the folders it reads,
a row each: the folder in a box with a browse button at its end, Include subfolders, and a cross
that stops reading it. A box shows the folder it reads in full while the box is left, and while it
is edited the words `Sources` writes, which are the words typed (`..` for a tree's own folder from
its `.seedocs`); an edit refused keeps the words typed. A folder typed or browsed over it is read in
its place, one typed relative read from the project's folder, and an emptied box stops reading it;
the same folder typed another way is kept as typed, with no scan. Under the folders stands an empty
box reading Add source, always there: a folder typed or browsed into it is read as one more, and its
row becomes that folder's with a new empty box under it. The empty box's row keeps the room of a
folder's answers, so the two boxes end at one edge. A folder that does not exist or is read already
is refused with the reason under the box. Under Scan options, the page asks whether the sources are
read as any C++ and Delphi or as ClaFi modules, and whether hints come from every recognizable
comment or follow the strict ClaFi rules - a new project starts on the first of each; under Edit
hints, whether a hint edited on a page is not written at all or written directly into the source,
with no undo. Every change reads the sources again, but the last, which builds the pages again. The
page stays where it stands while the sources are read again. Its url is the project's with
`/.project` after it: the scanner passes over names that start with a dot, so no folder can take it.

**A folder typed lists the folders under it.** A source's box, and the New project window's box for
where to store the project, list as they are typed at their end the folders in the folder the text
names up to its last separator whose names start with what follows it - case aside, by name, at most
200, and those starting with a dot only once a dot is typed - each marked as the tree marks a
folder, the letters typed in the accent. A source's box reads a path typed relative from the
project's folder, while the New project window's lists only a path from a root; a server named
alone, as `\\server\` is, lists nothing, its shares being looked up over the network. Down or
Alt+Down drops the list over the text as it stands. Down, Up, Page Up and Page Down walk it, nothing
current until Down, and Up from the first line goes back to the box. Return or a click on a line
takes that folder into the box with a separator after it, and the folders under it are listed in
turn; Return with no line current takes what is typed, as it does with no list. Escape takes the
list down, as Tab and leaving the box do, and an edit further in than the text's end lists nothing.

SeeDocs is a browser of the framework's own kind: tabs across the top, under them the crumbs of
the page shown with a list dropping from each, Back, Forward and Up beside them, and a search box at
the bar's end - see Search. Every page has
a url - `/project/folder/module/type/method`, the project named by its name - so a tab goes back
and forward through the pages it has visited (Alt+Left and Alt+Right, the side buttons of the
mouse, or a right click on either button for the list), a type's crumb lists its methods, and Up
climbs the path. The root, `/`, lists the projects open, each with a Close button that closes the
project and every tab on it. A home page opens each project: a link to its Project options page,
then the folders under the root as the tree lists them, with what each holds at every depth; a
Close project button stands at the page's top right. A project with nothing to document has the same
home, saying why under its title. Modules standing in a source folder's own folder are home's: they
stand in the project's node under its folders, and the home page carries what they export, by
module, as a folder's page does. Renaming a project takes every url under it along, in the tabs and
in their histories alike, and a second project open under a name already taken goes by that name
with a number after it. Where a tab stands, the tabs open and each one's history are kept with the
settings, so a run starts where the last one left off. A url no project open carries - a project
closed, or a type gone since the sources were scanned - shows a page saying so.

The pages: down the left, under each project's node, a tree of the sources' folders, named as
they stand on disk, each holding the folders under it and then its modules by their full names,
and each module the types it exports, controls first; a module that exports functions or
constants and no type is a row with nothing under it. On the right the
page of whatever is clicked. Each tab has a tree of its own, which follows wherever the tab goes -
to the project's own node for a page the tree has no row for, a search's or one gone: the nodes it
has open and how far it is scrolled are the tab's, and the nodes open are kept with
the tab, by project, so a restored tab - or a project opened again - comes back as it was left.
A node's rows are made the first time it opens, out of the project's contents, which are read
once and shared by every tab's tree. A page is a column of
controls: its title, marked with the mark its tree row wears, what the page is after it - a
project, a folder, a module, a function, a constant, a type's kind - and its hint under it,
a grid of facts - namespace, module, source - and then a section per expander. The source is the
file and line a type, a method, a function or a constant stands at, and a module's interface file;
an Open in editor button at its row's end runs the command line Settings keeps for the file's
language, and stands greyed, its hint naming Settings, while there is none. A class's title
carries its bases under it as a breadcrumb bar reversed: a crumb per base, its mark before it
pointing back toward the type standing on it, the ways up running right and parting into a column of
rows where a type has several bases, a mixin's host reached through the argument it was given.
A crumb opens its base's page, and its mark drops the types standing directly on that base, the
one the way up came through marked - for a base the surface does not have, the types whose bases
spell it the same way, template arguments aside. Crumbs wider than the page come down to one
common width, the widest first, and keep at least their mark and a few letters. An alias's line
is what it stands for after an equals sign,
an enum's its underlying type after a colon. A folder's page says what it holds at every depth,
leads with the folders directly under
its own, as the tree lists them, each with what it holds at every depth, and where the folder
holds modules of its own goes on with a grid of their types with their kinds and hints, then
their functions and their constants, each grid by module, the module's name beside its rows and
linked to its page; a module's page holds the same grids of its own exports. A function's row
carries its name alone, one row per name, every overload's signature in the name's hint; the
name links to the function's page,
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
footnote under the grid; the mark leads to it, as an anchor of the page's url, and its hint shows
the footnote's first lines over the row, as a link's hint shows the paragraph of the anchor it
leads to. A section's heading says what the section is; how many elements it holds is the
heading's hint.
A declaration whose comment references nothing takes the section named after it from its
folder's note - the one the folder's declarations cite the most, as a new footnote's home is
chosen below - marked as matched by name in that hint: a type's, a function's or a constant's
heading is its name, a member's its type's name and its own - `Button onClick` - and a method's
page shows its section under the first commented overload. Where the surface states no hint
the page says so, in place of the words, so what the documentation still lacks is read off the
preview. Hints read from every recognizable comment owe nothing, and their pages leave a missing
one empty.

A Delphi type's page is spelled as a C++ type's is - its title, its bases as crumbs under it, the
access labels written `public:` - and its code is Delphi's own: a routine's signature and a
constant's or a variable's declaration stand as written, coloured as Pascal. Its properties and
its events stand in a grid per access - published, public, protected - and a base's member that
a type below declares again is the type's own, counted once. A constructor reaches the type from
its bases as a method does, and a type below declaring the name hides it as it hides a method.

Footnotes are edited where they stand in every project; hints only where the project writes them
into the sources, follows the strict ClaFi rules - whose comment is the one the writer puts words in
- and only in C++ sources. A hint reads as a line of a note does: words between `**` are bold, and
words between backticks are code. A hint cell opens its editor with F2, Enter or a double click, on
the comment's own words - its types unlinked, a missing hint empty. The hint under a page's title
and the one under each declaration of a method page are typed into where they stand, a missing one
an empty box: Ctrl+click follows a link there, and Escape puts back the words the page shows. The
bold and the code a box shows are written back as their marks. Enter writes the words into the
comment the hint is read from - leaving one of those boxes writes them too - the `See` it ends with
kept: on the declaration's line while the whole line fits 100 columns, lined up with the comments of
the lines of code round it, and above it otherwise - always above a class, a struct or an enum. A
method's row writes the overload whose words it shows, and an override that shows its base's words
writes the base's comment. A footnote opens its markdown in its place with F2 or Edit footnote on
its menu: each paragraph and each bullet on one line, the headings inside it and its code as
written. Ctrl+S or Save writes it into its note - a paragraph or a bullet that reads as it did keeps
its lines, any other is broken at 100 columns again - and Escape or Cancel leaves it. The row of a
property, an event, a field or a member with no footnote offers Add footnote on its menu: the
footnote goes after the type's own section in its note, or at the end of the folder's note, headed
by the type's name and the member's, and is found by that heading - the source is left as it is. A
folder's note is the one its declarations cite the most, through the `See` of their comments, or
where they cite none, one named after the folder's own name - `Y-Base` for `Controls/Y-Base` - and
the root's is `Home`; the folder of footnotes and the note are made with the first footnote written
into them. Every write reads its file again first and refuses, saying why, where the file no longer
holds what the page was built from - a file changed in an editor since the scan is scanned again
before it is edited here. A file keeps its byte order mark and each line its own line end. The words
are written by `SeeDocs_App.Edits`, the database follows every hint written, and the page shown
takes the new words where it stands; a footnote added builds the page again at the new footnote.

A type's name on a page is a link to its page, in a grid cell as in prose: a type named in a
hint, in a note's prose or code, or in a signature - bare or qualified, as `Control` or
`Grids::Row` - links wherever the surface has a page for it. A right click on a link, or the menu
key on a grid cell holding one, offers Open and Open in new tab, which opens behind the tab shown.
A row of the derived types opens the type it names; a method's page picks the method's type in the
tree. A page's trees start closed, and a node's rows are built the first time it opens.
The pages are built by
`SeeDocs_App.Pages` - facts, sections, tables of rows with their footnotes, branches of a tree,
signatures and blocks of prose - out of the surface and the notes; SeeDocs builds controls
from them (`SeeDocs_App.PageView`), the browser (`SeeDocs_App.Browser`) shows one per tab, and a generator renders the same pages as files.

## Search

A search is typed into the box at the crumb bar's end, and Ctrl+F puts the focus there from
anywhere in the window. It searches the project the shown tab stands in; on the root, which lists
the projects, it searches every one, and its lines and its rows name their project.

What a search finds by name is every folder, module, type, function and constant the surface
carries, and every member a type's page lists: its properties, its events, its methods and its
fields or members. A private member is none, a constructor and a destructor are the type's own, a
property's getter and setter are the property, and a method with no comment of its own that a base
declares too is the base's - the base's row stands for it. A method or a function is one name
however many overloads it has. A name is found by every spelling it has: what its row shows - an
event by its alias - and a type's qualified name, an event's struct and its method, a folder's path
and a module's whole name.

A name holds what is typed, case aside, in one of four ways, best first: it is what is typed; it
starts with it; a word of it starts with it, a word starting after `_`, `:`, `.` or `/`, at a
capital after a lower-case letter or a digit, and at the last capital of a run before a lower-case
letter; or it holds it anywhere. A query of two parts - cut at its last `::`, else at its last
space, as `Grids::Row` and `Button onClick` are - finds as well a name its second part is found in
whose owner holds the first: a member's type, the namespace of a type, a function or a constant,
and the folder of a module or a folder. A member of a base of the type the first part names exactly
is found too, after the type's own, since that type's page lists it. The names found are ranked by
how they hold the words, then by how their owner holds a first part, then types, folders and
modules before the rest, then the shorter name, and last the tree's own order.

The box drops a list as it is typed into: first the search itself - Search "row" in ClaFi, naming
the project searched, or all projects from the root - and under a divider the ten best names, each
with the mark of what it is - a member's line leaves the mark's room blank - a member's type muted
before it, the words typed in the accent and its kind at the line's end; while a line is current,
its hint and where it stands are beside the list. **No line is current as the list drops**, so Enter
steps through the page's matches; Down enters the list at the search's line, and Up on it leaves the
list again. Enter on a line opens that name's page, or the search's. Alt+Enter or an Alt+click opens
a line in a new tab behind the one shown, and the list stays up so that more lines can be opened. A
member with no page of its own opens its type's page at its row: the url's anchor names the row by
the member's name, and the row's section opens and the row is selected.

**The box searches the page shown as well.** Each word in it stands on the found band an editor's
own search paints with, wherever the page shows it, case aside - the two parts of a query of two
being a word each - and it bands every page shown after, from the tree, Back and Forward or another
tab, until the box is emptied. The count at the box's end is the page's: "15 on page" while no match
is current, "4 of 15" once one is, and a muted "0 on page" where the page holds none, the project
perhaps holding more. With no line of the list current, Enter and Shift+Enter take the list down and
step to the page's next match and the one before, and F3 and Shift+F3 do so from anywhere in the
window. The arrows at the box's end step too, dimmed while the page holds none, and its cross
empties the box. A section a match stands in opens, and the match is selected - a cell as a click
selects it, the words in a footnote - and drawn over its band. A step taken on the page brings the
focus with it, and one taken in the box leaves it there. **A page the search opens goes on with
it**: opened from a name's line of the list or from a link on the search's page, its first match
from where it opens - the place its anchor names, else its top - is made current and brought into
view. Showing a search's page puts its words back in the box.

Escape takes the list down, and the next one hands the focus back: to the page's current match, else
where it came into the box from - a row of the tree, a cell of the page - and where that has gone,
or stands on a tab no longer shown, to the row the page's anchor selected, else its first row.
Escape never empties the box: the search stands while the page is read, and goes as its words are
typed away or cleared with the cross.

The search's page is a page of the browser like any other, named as its line in the list is on its
tab, its crumb and in the history, its head titled with the words and marked `search`, its url the
project's with a segment of `.search=` and the words after it - a slash in them written `%2F`, a %
`%25` - or the root's for the root's search, so Back and Forward step through searches and a tab is
restored onto one. It holds a section for each kind of name found, the names in the order they are
ranked; then In hints, the declarations whose hints hold the words and whose names do not; then In
footnotes, every footnote section holding them, named by its note and its heading and linked to the
declaration it belongs to - the one its heading names, else the first whose See reaches it - with
the sentence the words stand in. Hints and footnotes are searched as they read, their code and bold
marks taken out, and a section's words are its own, up to the first heading under it. A section
lists its first 200 rows, and its heading's hint says how many there are.

## The checks

`check` holds a project's sources read as ClaFi modules to the routine, and a project reading
any C++ and Delphi to nothing:

- a property with no `INIT_PROPERTY`; a `..._STORAGE` property without its getter and a
  `WRITABLE` one whose setter is not there, the base chain searched as well
- a `Props::get` or `Props::find` written by hand rather than through `READ_PROPERTY` or
  `REQUIRE_PROPERTY`
- a comment of more than one line, one in both places at once, or none on a declaration of the
  design surface - a control, its properties and events, an exported enum
- a comment above a declaration it would fit on, and one riding a line past 100 columns
- an event whose three spellings disagree, or whose struct is not found
- a property whose type is neither an enum, a known value kind, nor a declared type
- a `See` reference to a footnote that does not exist in the project's `footnotes`, or to a
  heading the note does not have - on every line of a comment
- a class declaring properties or events whose base chain reaches neither Control nor a type
  the scanner can read
- a value type declared under `Controls/Base`, where it reaches an application only through an
  import of the base

## Building

`SeeDocs.sln` builds both programs with Visual Studio against the shared items: `SeeDocs` the window
and `SeeDocsCli` the scanner, which share the Surface, Reading, Scanner, PascalReader, Database,
Project, Notes and Pages modules under `Src/`, the window's own standing under `Src/Ui/` and the
Edits and Search modules beside the shared ones. The CMake target `SeeDocs` builds the window on
Wayland beside the other applications. The CMake target `SeeDocsCli` builds the scanner on Linux
with or without a platform layer - the Dom, the Syntax lexer and the text file reader reach no
further down than `UiTypes` - so a `check` runs wherever the tree is checked out:

    cmake -S . -B build -G Ninja -DCLAFI_PLATFORM=none \
          -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target SeeDocsCli
    ./build/SeeDocsCli check Source
