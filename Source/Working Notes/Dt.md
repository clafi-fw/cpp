# Dt

The author's side of the footnote of the same name.

## Part

A part is deliberately not a node of any hierarchy. If it were a node of the one its container's
children belong to, a container could not tell its part apart from its first child - they would
arrive in the same vector. So a part keeps its argument pack and the enclosing container routes
it, which is also why the kit needs to know nothing about what any particular part holds.
