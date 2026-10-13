# Events

The author's side of the footnote of the same name.

## EventParamOf

Only the primary template is exported; the partial specialization for a lambda or a functor, read
off its operator(), is not an exportable declaration and does not need to be - it is reachable from
the module wherever the primary is used.

## OnEventTag

A tag base rather than a template-id match, so that a caller can keep a thin named subclass for
compatibility and still have it recognised as a handler by the routing.

## OnEvent

One property type serves every event, so an emitter needs no named handler type per event it
raises. The event is already written in the handler, and a name for it would only say the same
thing twice.
