# Dt

The kit a declarative tree - Dom's Dt, a grid's Dt - is built from: the node, part and handler
shapes, and the pack routing that sorts a constructor's arguments.

## NodeBase

**A DSL has one build context per kind of thing it builds.** Dom has one, the parent section.
Grids has one each for columns, rows and cells, so a RowNode will not bind where a CellNode is
expected. The pack routing sorts by the same type.

## Reference

**The referent must outlive the apply** - a const member or a namespace constant, say. A node
passed as an lvalue is wrapped in a Reference by the pack routing.

## Part

    Expander{ Header{ Text{ ... } }, Rows{ ... } }
    Grid{ Columns{ ... }, Header{}, Rows{ ... } }

**A part means something only to the container that directly encloses it.** One nested deeper,
inside a Rows{} say, reaches nothing that looks for it. A container that could receive one by
mistake rejects it at compile time.

A DSL names a part of its own by deriving from Part, as Grids does with Span.

## Header

**The container decides what its header accepts.**

- A grid's header takes nothing - the columns say what belongs in it.
- An expander's header is a single text control, so it takes text and rejects cells.

## Init

`Init<T>{ lambda }` is for the setters with no constructor property, such as
Slider::setMaxPosition. It goes anywhere in the argument list.

**It runs after the pack's handlers are connected**, so a value it sets can fire them.

## PostCreate

postCreateOf collects it from a pack. Called on the new object, it connects the handlers first,
then runs the Init.
