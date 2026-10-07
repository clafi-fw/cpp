# Dt

The description kit: the node, part and handler shapes a declarative tree - Dom's Dt, a grid's
Dt - is built out of, and the pack routing that sorts a constructor's arguments into children,
parts, handlers and properties.

## NodeBase

One node hierarchy per build context. A DSL that builds a single kind of thing has one context;
Dom's is the parent section a node applies itself to. A DSL that builds several kinds has one
context per kind. The context type keeps the hierarchies apart: a Grid RowNode will not bind
where a CellNode is expected, and the pack routing tells them apart by it.

## Reference

A node declared once, as a const member or a namespace constant, reused by several parents
without copying. The referent has to outlive the apply.

## Part

A part of a container, as opposed to a child sitting inside it:

    Expander{ Header{ Text{ ... } }, Rows{ ... } }
    Grid{ Columns{ ... }, Header{}, Rows{ ... } }

A part is deliberately not a node of any hierarchy. If it were a node of the one its container's
children belong to, a container could not tell its part apart from its first child - they would
arrive in the same vector. So a part keeps its argument pack and the enclosing container routes
it, which is also why the kit needs to know nothing about what any particular part holds.

The consequence worth knowing: a part only means anything to the container that directly
encloses it. One nested a level deeper, inside a Rows{} for instance, is not a child of anything
that looks for it. Containers that could receive one by mistake reject it rather than dropping
it.

Header is the part the kit names. A DSL names one of its own by deriving from Part, the way
Grids names a group's Span. `withPart` reads a named part out of an argument list and reports
whether there was one; the container then routes those arguments with the same propsOf /
makeNodeVector it uses for its own.

## Header

A container's own header. What a header accepts is the container's business: a grid's header
takes nothing, the columns already saying what belongs in it, and an expander's header is a
single text control, so it takes text and rejects cells.

## Init

`Init<T>{ lambda }` runs against the object right after it is created, for the setters that
have no constructor property - Slider::setMaxPosition and its kind. It goes anywhere in the
argument list.

## PostCreate

Everything a pack says about an object that is not a constructor argument: the events to
connect, and the initialiser to run.
