---
name: "cpp-code-style"
description: "Apply the ClaFi C++ (C++20) code style and formatting rules whenever writing, generating, editing, or refactoring C++ code - including modules, classes, functions, naming, bracing, whitespace, and comments."
---

# C++ Code Style and Formatting

Language: Modern C++ (C++20). Architecture context: Modules, Concepts, Strong Typing, Zero-Overhead Abstractions.

Apply every rule below when generating or refactoring C++ code. Follow the rules silently within code blocks. Do not explain style choices, and do not add conversational padding about optimization or architecture unless asked.

## 1. Decision and Design Protocol

**[MUST] Pre-Coding Discussion:** Discuss the selected implementation strategy before generating code if fulfilling the request involves:
- Tradeoffs that have not been pointed out yet.
- Public API changes that were not explicitly requested.
- Multiple implementation options with no clear winner.

**[MUST] Post-Coding Manual Update:** When a finished piece of work has changed the public API, update the subsystem's manual before reporting the job done. The manual is part of the change, not a follow-up task.

- WHEN: only once the coding job is over and the API is settled. Whoever asked for the work says so ("the coding job's well done, the API is established"), or the work reaches an equivalent close. Development is iterative, and the API moves back and forth across many turns. Those intermediate shapes are not documented, and that is correct - a manual written mid-iteration describes an API that stops existing on the next turn. Hold the manual until the shape stops moving, then write the settled result once.
- WHAT COUNTS: anything a caller writes or has to know - a name, a spelling, an argument order, a rule about ordering or lifetime, a constraint that stopped holding. Measure it across the whole job: the API as it stood before the work began against the API as it stands now. Churn that cancels out is not a change, and neither is an internal refactor no call site can observe.
- FIND IT: subsystem manuals live in `Documentation/` as self-contained HTML (`Dom-Manual.html`, `Events-Manual.html`, `TextEngine.html` and the rest). Match the existing one's structure and copy its inline stylesheet rather than restyling. The `.md` notes under `Source/RawDocs/` are the raw material for manuals, not manuals.
- REMOVE WHAT STOPPED BEING TRUE: if the change deleted a caveat, delete the warning about it. A manual that still warns about a fixed problem teaches a rule that no longer exists, and is worse than one that never mentioned it.
- DO NOT NARRATE THE CHANGE: the manual describes the API as it is now. See section 7 - no "used to", no "was removed". The reader has never seen the previous version.

## 2. Whitespace and File Encoding

The project carries an `.editorconfig` at the repository root. It is authoritative for Visual Studio and ReSharper C++ alike, and these rules restate it. Never introduce whitespace that contradicts it.

- **[MUST] Indent with 4 spaces. Never emit a tab character.** This holds regardless of how the surrounding lines are encoded. Do not "match the file" when the file uses tabs - a file mixing both is the defect being corrected, and matching it perpetuates the mix.
- **[MUST] One indent level is exactly 4 columns.** Nested blocks step by 4.
- **[MUST] No trailing whitespace** on any line, including otherwise-blank lines inside functions.
- **[MUST] End every file with a single newline.**
- **[MUST] Line endings are CRLF.** Preserve the existing endings of any file being edited.
- **[MUST] Files are UTF-8 without BOM.** If a file must contain non-ASCII characters, say so rather than silently relying on the compiler's default code page.

### Why this matters

Tabs and spaces can encode the same visible indent in an editor configured for 4-wide tabs, and then diverge everywhere else. `git diff`, code review web UIs, terminals and most external tools render a tab as 8. A file that mixes them looks correct locally and misaligned to everyone else. Spaces render identically in every tool, which is the reason for the rule.

### When editing a file that already mixes tabs and spaces

Do not silently reformat the whole file - that buries the real change in whitespace noise. Instead:
1. Write the lines being touched with spaces.
2. Point out that the file is mixed, and offer a separate whitespace-only pass.

Converting a tab-indented file is safe only after confirming no tab appears outside leading whitespace. A tab inside a string literal must survive. Expand using the tab width the file was written for. Do not assume the width - the right one makes the indent columns line up with brace nesting depth.

**[MUST] This caveat is scoped to leading whitespace only.** It is not a general licence to leave style violations in place. It does not exempt bracing, statement layout, or any rule in section 4. It does not apply to a file being newly authored, or to code being relocated into a new file. In both of those cases every line is a line being written, so every rule in this document applies to every line. When moving existing bodies into a new implementation unit, apply section 4 to the moved code as it lands.

## 3. Structural Organization

- **[MUST] Strict Separation:** Class/Struct declarations and method definitions MUST be strictly separated.
- **[MUST] Definition Placement:** Implementations MUST live in the module's implementation unit - a `.cpp` carrying `module <ModuleName>;` - collected at the bottom of that file inside the standard namespace block (e.g., `namespace ClaFi::CfgEngine { ... }`). The module interface (`.cppm`) carries declarations, templates, and `constexpr` definitions only.
    - RATIONALE: a body defined in the interface is baked into the BMI, so editing it rebuilds every translation unit that imports the module. Moving bodies to the implementation unit means a body edit recompiles one file.
    - WHAT MUST STAY IN THE INTERFACE: templates, `constexpr` and `consteval` functions, and anything else the compiler needs a definition for at the point of use.
    - Do not reach for `module :private;` as a substitute. MSBuild invalidates consumers on the `.ifc` timestamp regardless of the private fragment, so it buys no incremental win, and it is illegal in any module that has partitions.
    - When a body moves out, the implementation's parameter names MUST match the declaration's, and default arguments MUST appear in the declaration only.
- **[EXCEPTION] Inline Exceptions:** The ONLY exceptions to the separation rule are:
    - The simplest 1-operation getters or setters.
    - Empty virtual functions (allowed to be inlined on the same line).
    - Simplest casts or simplest redirections to member properties (allowed to be inlined on the same line).
- **[MUST] Minimal Forward Declarations:** Order classes logically to minimize the need for forward declarations. Base classes and utility concepts go first.
- **[MUST] Class Member Order:** A class declares its members in the order below. Each group after the friends opens under its own access label, so the label repeats where two groups share an access.
    1. Friend declarations, above the first label.
    2. `public:` inherited constructors (`using Base::Base;`), re-published base members (`using Base::name;`), and the nested types and aliases the interface names.
    3. `public:` constructors, destructor, and copy and move operations, deleted ones included.
    4. `public:` properties (`DECLARE_*_PROPERTY`).
    5. `public:` events (`DECLARE_EVENT`).
    6. `public:` functions.
    7. `protected:` constructors, functions, then any protected data.
    8. `private:` nested types and aliases the implementation uses.
    9. `private:` constructors and functions.
    10. `private:` static constants and fields (`k_`, `s_`), then data members. Nothing follows the data.
    - Data members are listed in the order their lifetimes need. Members are built in declaration order and destroyed in reverse.
    - A declaration that names another comes after it, whatever group it belongs to.
- **[MUST] Struct Member Order:** Constructors first, then data members, then functions.
- **[MUST] Definition Order:** Definitions follow declaration order. Classes are defined in the order the interface declares them, and each class's members in the order the class declares them. This holds for definitions kept in the interface as well.
- **[MUST] Module Import Ordering:** Organize imports from most dependent/specific to most fundamental/core, with third-party libraries at the top:
    1. Third-party dependencies.
    2. Most specific or sibling module imports.
    3. Intermediate core framework imports.
    4. Most core framework imports and standard library wrappers last.

  GOOD EXAMPLE:
  ```cpp
  import Nlohmann.Json;

  import ClaFi.Controls.Grids;
  import ClaFi.Controls.Slider;
  import ClaFi.Core.Graphics;
  import ClaFi.Core.System.Utils;

  import ClaFi.StdLib; // Houses re-exported standard library
  ```

- **[MUST] Import What You Use:** A module implementation unit does not inherit the non-exported imports of its interface partitions. Where a name is used, import the module that declares it, rather than relying on it arriving transitively.
- **[MUST] Drop Imports That Are Not Used:** An import no name in the file depends on is deleted, not kept for safety. Every import in an interface deepens the module dependency graph and serializes the build.

## 4. Syntax and Formatting

- **[MUST] Allman Bracing:** Opening curly brackets MUST be on the same column/indentation level as the closing bracket. They MUST sit on their own line.
    - SCOPE EXCEPTION: This rule only applies to standard control blocks and functions.
    - INLINE EXCEPTIONS: The opening brace `{` sits on the same line for brace initialization declarations, after return statements, and for inline lambdas passed into functions.

  GOOD EXAMPLES:
  ```cpp
  IntPoint pt{
      .x = 0,
      .y = 0,
  };

  return {
      .x = 0,
      .y = 0,
  };

  callWithCallBack([](){
      Platform::beep();
  });
  ```
    - SIMPLE FUNCTIONS EXCEPTION: Simple inline getters/setters/casts may exist on a single line: `[[nodiscard]] int count() const { return m_count; }`

- **[MUST] Lambdas:** Lambda body statements must sit on their own lines and never be chained or inlined on a single line. Lambdas never use Allman bracing; their opening brace stays on the declaration line matching the brace initialization format.

  GOOD EXAMPLE:
  ```cpp
  callWithCallBack([](){
      Platform::beep();
  });
  ```

- **[MUST] Pointers and References:** Type modifiers (`&`, `&&`, and `*`) MUST stick to the type name, not the variable name.
    - GOOD: `const ValueType& value`, `std::unique_ptr<Node>&& node`, `Widget* view`
    - BAD: `const ValueType &value`, `Widget *view`

- **[MUST] Const Placement:** Always use West Const formatting.
    - GOOD: `const ValueType& value`
    - BAD: `ValueType const& value`

- **[MUST] Single Statement Per Line:** EVERY statement and operator MUST be on its own line. Never chain instructions.
    - BAD: `if (!first) { stream << L"\n"; writeIndent(); }`
    - GOOD:
  ```cpp
  if (!first)
  {
      stream << L"\n";
      writeIndent();
  }
  ```
    - This covers a guard and its body (`if (x) return;`), a call followed by `return` (`draw(); return;`), and paired assignments (`minX = a; maxX = b;`). Each goes on its own line.

- **[MUST] One Declaration Per Line:** Never comma-declare. `float l = r.left, t = r.top;` becomes two lines.

- **[MUST] Enums:** Every enum member MUST be on its own line.

- **[MUST] Switch Layout:** The `switch` brace follows Allman. `case` labels are indented one level inside it, and a braced case body follows Allman on the line after the label.

- **[MUST] Brace Initialization and Padding:** Prefer curly brackets for initialization.
    - PADDING: If there is content inside `{ }`, add a single space of padding: `{ good }`. If `{}` is empty, do not include any whitespace inside.
    - FUNCTION SCOPE: In functions (local variables), use the equals sign. Example: `Point pt = { 0, 0 };`
    - CLASS SCOPE: In class declarations (default members), skip the equals sign. Example: `Point pt{ 0, 0 };`

- **[MUST] Struct Initialization Layout:**
    - SHORT FORM: Points and other structs with obvious members must be initialized in the shortest possible way:
  ```cpp
  Point pt = { 0, 0 };
  Hsl color = { 0.5f, 0.6f, 0.83f };
  ```
    - DESIGNATED INITIALIZATION: If there is uncertainty regarding members, prefer designated initialization. The `=` is allowed to be skipped in code blocks for composite values, and MUST always be skipped in class declarations:
  ```cpp
  Color color{ .red = 255, .green = 0, .blue = 120 };
  ```
    - LINE BREAK RULES: Keep short initialization without calculated expressions or functions on a single line. If there are expressions or functions, place each member on a new line:
  ```cpp
  Point pt = {
      targetX * scaleFactor,
      targetY * scaleFactor,
  };
  ```

- **[MUST] Local Struct Definitions:** A struct defined inside a function is declared on its own, members one per line, before the variable that uses it. Never define the type and declare the variable in one statement.

- **[MUST] Constructor Initializer Lists:** The colon `:` MUST be on its own line. Each initializer MUST be on its own line, using brace initialization `{}` with internal space padding if populated.

  GOOD:
  ```cpp
  MyClass::MyClass(int param1, int param2)
      :
      m_param1{ param1 },
      m_param2{ param2 }
  {
  }
  ```

- **[MUST] Initializer List Order:** Initializers MUST appear in the same order as the members are declared. Initialization follows declaration order regardless of what is written, so any other order is misleading and warns under `-Wreorder`.

- **[MUST] No Stray Semicolons** after a function or class member body.

- **[MUST] Specifier Layout (noexcept / constexpr):** Specifiers such as `noexcept`, `constexpr` and `consteval` belong in the signature. They sit after the parameter list, before the trailing return type or the function body.

## 5. Naming Conventions and Args

- **[MUST] Type Aliasing and Boilerplate Reduction:** Always use type aliasing (`using`) to encapsulate template definitions if a specific template declaration is repeated across fields, arguments, or return values.
    - BAD:
  ```cpp
  std::vector<MyClass>& collection() { return m_collection; }
  std::vector<MyClass> m_collection;
  ```
    - GOOD:
  ```cpp
  using MyClassCollection = std::vector<MyClass>;
  MyClassCollection& collection() { return m_collection; }
  MyClassCollection m_collection;
  ```

- **[MUST] Variables:** camelCase (e.g., `parsedValue`). Prefer full, descriptive words. Avoid ambiguous abbreviations.
    - BAD: `ValueType res;`
    - GOOD: `ValueType result;`
    - EXCEPTION: Unambiguous, widely accepted contractions are allowed (e.g., `src`, `dst`, `pt`, and `i` for loop counters).
    - Never name a variable by appending a digit to distinguish it from another (`cellFrame2`, `width2`). Name the difference instead.
    - Never use snake_case for a local (`r_bound`). If a short name collides with something, lengthen it into a real word.

- **[MUST] Functions/Methods:** camelCase (e.g., `writeIndent`, `childValue`).

- **[MUST] Argument Naming in Declarations:** In separated function declarations:
    - If the argument name exactly reflects its type name, strip the argument name entirely: `void scale(ScaleFactor);`
    - Otherwise, always use a descriptive name. This name MUST match exactly between the declaration and the implementation: `void scale(float scaleFactor);`

- **[MUST] Classes/Structs/Enums:** PascalCase (e.g., `SectionNode`, `ColorMode`).
- **[MUST] Private Class Members:** MUST be prefixed with `m_` followed by camelCase (e.g., `m_childNodes`, `m_bound`).
- **[MUST] Public Struct Members:** MUST NOT use the `m_` prefix. Use standard camelCase (e.g., `bounds`, `maximized`).
- **[MUST] Static Fields:** MUST be prefixed with `s_` followed by camelCase (e.g., `s_instanceCount`, `s_globalState`).
- **[MUST] Compile-Time Constants:** MUST be prefixed with `k_` (e.g., `k_serializedFields`).
    - NAMESPACE EXCEPTION: Grouping a set of constants in its own dedicated namespace is preferred. When constants live in such a dedicated namespace, the `k_` prefix MUST be skipped - the namespace already provides the scoping context.
  ```cpp
  namespace SerializedFields
  {
      constexpr auto name = "name";
      constexpr auto version = "version";
  }
  ```
- **[MUST] Concepts:** PascalCase starting with an adjective or verb (e.g., `IsComposite`, `SerializableEnum`).

## 6. C++ Modernisms

- **[MUST] Auto Deduction Boundaries:** Use `auto` only where the right-hand side states the type, and in verbose iterator and range loops. Everywhere else the type is written out, so the declaration carries the architectural intent.
    - GOOD: `auto widget = std::make_unique<Widget>();`
    - GOOD: `float value = calculateScale();`
    - BAD: `auto value = calculateScale();`
- **[MUST] Standard Library:** Reach the standard library through `import ClaFi.StdLib;`, which is `export import std;` and nothing else.
    - Write every std name as `std::` at its use site, types included: `std::size_t`, `std::uint8_t`, `std::abs`. `import std;` exports names in namespace `std` only, so a global spelling that still compiles came from a C header, not from this module.
    - Never add a using-declaration or an alias to `ClaFi.StdLib`, and never switch it to `std.compat`. A translation unit that pulls `<windows.h>` has `::abs(int)` in scope, and an unqualified `abs(aFloat)` binds to it and compiles.
    - Do not import standard header units (`<cstdint>`, `<numeric>`, `<iostream>`) - `std` already covers them, and each header unit inflates the BMI of every consumer.
    - A heavy platform header such as `<immintrin.h>` is `#include`d in the global module fragment of the few files that use it, never from a widely imported module.
- **[MUST] Smart Pointers:** Raw pointers `*` are ONLY allowed for non-owning observing views. Ownership MUST use `std::unique_ptr`.
- **[MUST] Member Initialization:** Every non-static data member must be initialized - by a default member initializer or by every constructor. Never leave a scalar member indeterminate.
- **[MUST] [[nodiscard]]:** Use `[[nodiscard]]` liberally on getters, type queries, and any function where ignoring the return value is a logical bug.
- **[MUST] Type Traits:** Use C++20 concept and `requires` clauses instead of `std::enable_if` or SFINAE metaprogramming.
- **[MUST] Forward Once:** A forwarding reference pack may be forwarded exactly once. If the arguments must also be inspected, inspect them as lvalues (`args...`) and keep the single `std::forward` at the last use.
- **[MUST] Precondition Failures:** A violated precondition calls `unreachable(...)` with a message. Never guard the check with `#ifdef _DEBUG` while leaving the release build to dereference null or fall off the end.
- **[MUST] Const Correctness:** A `const` member function must not hand out mutable access to what it owns. Where const is deliberately not propagated - because the thing reached is structure rather than state - say so in a comment, so the missing `const` reads as a decision.

## 7. Linguistic and Documentation Style

- **[MUST] No Comment by Default:** A comment earns its line only by carrying what the code cannot say for itself - an invariant, a trap, a constraint imposed from outside, a rejected alternative, a reason. Strike the comment and read what is left. If the code is as clear and nothing true became unrecoverable, leave it struck. A clearer name or a named local retires a comment for good.
    - The public surface is the exception: a control, its properties and events, and the enums they take each carry one comment line. The designer tool and the manuals read it.
- **[MUST] End-User Comments Only:** Avoid comments only relevant to the current prompt/history that do not serve the end user of the code (e.g., do NOT write comments like `// Added!`, `// new member`, or `// <--- Added!`).
- **[MUST] Comment the Invariant, Not the Habit:** When a comment justifies why something is safe, state the property that actually makes it safe and what would break it. A justification that is merely true today misleads the next reader.
- **[MUST] State the Current Fact, Not the Change:** A comment or manual says what is true now, never what it replaced. No "used to", "was removed", "is gone", "no longer", "previously". This is not a delete pass. A property dressed as history is restated as the property: `"this used to be handed boundsInForm(), so it was counted twice"` becomes `"it must not be handed boundsInForm(), which already carries that topLeft"`. A rejected alternative is kept, phrased hypothetically. A comparison against a state the reader never saw - "one dispatcher rather than two" - is the same defect in shorter form. State the count, not the delta.
- **[MUST] Clarity Over Expressionism:** Avoid emotional, descriptive, or promotional adjectives. Strip words that add no technical value (such as completely, heavily, perfectly, incredibly, trivially).
- **[MUST] No Exclamation Marks:** Do not use exclamation marks in comments, documentation, or explanations. Do not use runs of `!!!!!` as attention markers; write a `TODO:` with the actual question instead.
- **[MUST] Simple Sentence Structure:** Keep sentences short and direct. Avoid complex nested sentences.
- **[MUST] Dash Formatting:** Use spaced hyphens ( - ) instead of long dashes.

### Comments on declarations

- **[MUST] One Line, One Place:** A class, struct, enum, member, property or event carries one comment line at most. Two lines is a violation wherever they sit, and so is a comment both above a declaration and on it.
- **[MUST] Placement:** The declaration decides where its comment goes.
    - A declaration that opens a scope - a class, a struct, an enum - takes its comment on the line above. That comment covers everything inside.
    - Every other declaration takes its comment on the declaration line while the whole line fits 100 columns, and on the line above when it does not.
    - An above-line comment touches its declaration, with no blank line between them.
    - A comment on the declaration line is a label: lower case, no full stop. A comment above a declaration is a sentence.
- **[MUST] Longer Explanations Go to RawDocs:** Where the mechanics need more than a line, the words go into the subsystem's note under `Source/RawDocs/`, and the comment ends with a reference: the word `See`, the note's file stem, an optional `#` anchor, and nothing after it. A reference costs columns, so a comment carrying one usually sits above.

  GOOD EXAMPLE:
  ```cpp
  // What a drag that starts on an item does. See Selection-Model
  export enum class DragMode
  {
      EasySelect,   // an item is one more place to start a selection rectangle from
      EasyDrag      // an item is the selection's handle, and the drag carries what is selected
  };

  // Whether the view holds a selection on top of the current item.
  DECLARE_PROPERTY(SelectionMode, selectionMode, SelectionMode::None)
  ```

- **[MUST] Run the Checker:** `python "Tools/Surface Scanner/scan.py" --check` reports every departure on the declarations it harvests - classes, properties, events, event payloads, enums and their members.
